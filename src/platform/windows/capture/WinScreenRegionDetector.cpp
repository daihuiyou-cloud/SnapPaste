#include "platform/windows/capture/WinScreenRegionDetector.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <objbase.h>
#include <dwmapi.h>
#include <UIAutomationClient.h>
#include <wrl/client.h>
#endif

#include <QGuiApplication>
#include <QScreen>
#include <QtGlobal>

#include <algorithm>

namespace snappaste {

namespace {

constexpr int kMinSmartRegionSize = 16;

QRect normalizedCandidate(QRect rect, const QRect& bounds)
{
    rect = rect.normalized().intersected(bounds);
    if (rect.width() < kMinSmartRegionSize || rect.height() < kMinSmartRegionSize) {
        return {};
    }
    return rect;
}

void addCandidate(QVector<QRect>& regions, const QRect& candidate, const QRect& bounds)
{
    const auto normalized = normalizedCandidate(candidate, bounds);
    if (!normalized.isValid()) {
        return;
    }
    if (!regions.contains(normalized)) {
        regions.push_back(normalized);
    }
}

#ifdef Q_OS_WIN
QRect nativeGeometryFor(const QScreen* screen)
{
    if (screen == nullptr) {
        return {};
    }

    const auto geometry = screen->geometry();
    const auto dpr = screen->devicePixelRatio();
    return QRect(qRound(geometry.x() * dpr),
                 qRound(geometry.y() * dpr),
                 qRound(geometry.width() * dpr),
                 qRound(geometry.height() * dpr));
}

QScreen* screenForLogicalPoint(const QPoint& point)
{
    auto* screen = QGuiApplication::screenAt(point);
    return screen != nullptr ? screen : QGuiApplication::primaryScreen();
}

QScreen* screenForNativeRect(const RECT& rect)
{
    const QPoint center((rect.left + rect.right) / 2, (rect.top + rect.bottom) / 2);
    for (auto* screen : QGuiApplication::screens()) {
        if (screen != nullptr && nativeGeometryFor(screen).contains(center)) {
            return screen;
        }
    }
    return QGuiApplication::primaryScreen();
}

QPoint nativePointFromLogical(const QPoint& point)
{
    auto* screen = screenForLogicalPoint(point);
    if (screen == nullptr) {
        return point;
    }

    const auto logicalGeometry = screen->geometry();
    const auto nativeGeometry = nativeGeometryFor(screen);
    const auto dpr = screen->devicePixelRatio();
    return QPoint(nativeGeometry.x() + qRound((point.x() - logicalGeometry.x()) * dpr),
                  nativeGeometry.y() + qRound((point.y() - logicalGeometry.y()) * dpr));
}

QRect rectFromWinRect(const RECT& rect)
{
    auto* screen = screenForNativeRect(rect);
    if (screen == nullptr) {
        return QRect(QPoint(rect.left, rect.top), QPoint(rect.right - 1, rect.bottom - 1));
    }

    const auto logicalGeometry = screen->geometry();
    const auto nativeGeometry = nativeGeometryFor(screen);
    const auto dpr = screen->devicePixelRatio();
    const QPoint topLeft(logicalGeometry.x() + qRound((rect.left - nativeGeometry.x()) / dpr),
                         logicalGeometry.y() + qRound((rect.top - nativeGeometry.y()) / dpr));
    const QPoint bottomRight(logicalGeometry.x() + qRound((rect.right - 1 - nativeGeometry.x()) / dpr),
                             logicalGeometry.y() + qRound((rect.bottom - 1 - nativeGeometry.y()) / dpr));
    return QRect(topLeft, bottomRight).normalized();
}

bool isOwnProcessWindow(HWND hwnd)
{
    DWORD processId = 0;
    GetWindowThreadProcessId(hwnd, &processId);
    return processId == GetCurrentProcessId();
}

bool isUsableWindow(HWND hwnd)
{
    if (hwnd == nullptr || !IsWindow(hwnd) || !IsWindowVisible(hwnd) || isOwnProcessWindow(hwnd)) {
        return false;
    }

    RECT rect{};
    if (!GetWindowRect(hwnd, &rect) || rect.right <= rect.left || rect.bottom <= rect.top) {
        return false;
    }

    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    if ((exStyle & WS_EX_TOOLWINDOW) != 0) {
        return false;
    }

    BOOL cloaked = FALSE;
    if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked))) && cloaked) {
        return false;
    }

    return true;
}

HWND topWindowAt(const POINT& nativePoint)
{
    HWND hwnd = WindowFromPoint(nativePoint);
    if (hwnd) {
        HWND root = GetAncestor(hwnd, GA_ROOT);
        if (root && isUsableWindow(root) && !isOwnProcessWindow(root)) {
            RECT rect{};
            if (GetWindowRect(root, &rect) && PtInRect(&rect, nativePoint))
                return root;
        }
    }
    for (hwnd = GetTopWindow(nullptr); hwnd != nullptr; hwnd = GetWindow(hwnd, GW_HWNDNEXT)) {
        if (!isUsableWindow(hwnd)) continue;
        RECT rect{};
        if (GetWindowRect(hwnd, &rect) && PtInRect(&rect, nativePoint))
            return hwnd;
    }
    return nullptr;
}

void buildChildWindowCache(HWND parent, QVector<QRect>& outRects)
{
    struct Context {
        QVector<QRect>* rects;
    } context{&outRects};

    EnumChildWindows(parent, [](HWND child, LPARAM value) -> BOOL {
        auto* ctx = reinterpret_cast<Context*>(value);
        if (!IsWindowVisible(child)) return TRUE;
        RECT rect{};
        if (GetWindowRect(child, &rect)) {
            ctx->rects->push_back(rectFromWinRect(rect));
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&context));
}

void buildUiAutomationCache(HWND hwnd, QVector<QRect>& outRects)
{
    using Microsoft::WRL::ComPtr;
    ComPtr<IUIAutomation> automation;
    if (FAILED(CoCreateInstance(CLSID_CUIAutomation, nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(automation.GetAddressOf())))) {
        return;
    }

    ComPtr<IUIAutomationElement> root;
    if (SUCCEEDED(automation->ElementFromHandle(hwnd, root.GetAddressOf()))) {
        ComPtr<IUIAutomationCondition> condition;
        if (SUCCEEDED(automation->CreateTrueCondition(condition.GetAddressOf()))) {
            ComPtr<IUIAutomationElementArray> descendants;
            if (SUCCEEDED(root->FindAll(TreeScope_Descendants, condition.Get(), descendants.GetAddressOf()))) {
                int length = 0;
                descendants->get_Length(&length);
                for (int i = 0; i < length && i < 250; ++i) {
                    ComPtr<IUIAutomationElement> element;
                    if (FAILED(descendants->GetElement(i, element.GetAddressOf()))) {
                        continue;
                    }
                    RECT rect{};
                    if (SUCCEEDED(element->get_CurrentBoundingRectangle(&rect))) {
                        outRects.push_back(rectFromWinRect(rect));
                    }
                }
            }
        }
    }
}
#endif

} // namespace

namespace detail {

ComInitializer::ComInitializer()
{
#ifdef Q_OS_WIN
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    initialized_ = SUCCEEDED(hr);
#endif
}

ComInitializer::~ComInitializer()
{
#ifdef Q_OS_WIN
    if (initialized_) {
        CoUninitialize();
    }
#endif
}

} // namespace detail

WinScreenRegionDetector::WinScreenRegionDetector() = default;
WinScreenRegionDetector::~WinScreenRegionDetector() = default;

#ifdef Q_OS_WIN
void WinScreenRegionDetector::rebuildCache(HWND hwnd, const QRect& desktopBounds)
{
    cachedChildRects_.clear();
    cachedUiRects_.clear();
    buildChildWindowCache(hwnd, cachedChildRects_);
    buildUiAutomationCache(hwnd, cachedUiRects_);
}
#endif

QVector<QRect> WinScreenRegionDetector::regionsAt(const QPoint& globalPosition, const QRect& desktopBounds)
{
#ifdef Q_OS_WIN
    const auto mappedPoint = nativePointFromLogical(globalPosition);
    const POINT nativePoint{mappedPoint.x(), mappedPoint.y()};
    const auto hwnd = topWindowAt(nativePoint);

    QVector<QRect> result;
    result.reserve(16);

    if (hwnd) {
        RECT windowRect{};
        if (GetWindowRect(hwnd, &windowRect))
            addCandidate(result, rectFromWinRect(windowRect), desktopBounds);

        if (hwnd != cachedHwnd_ || desktopBounds != cachedBounds_) {
            rebuildCache(hwnd, desktopBounds);
            cachedHwnd_ = hwnd;
            cachedBounds_ = desktopBounds;
        }

        for (const auto& rect : cachedChildRects_) {
            if (rect.contains(globalPosition))
                addCandidate(result, rect, desktopBounds);
        }
        for (const auto& rect : cachedUiRects_) {
            if (rect.contains(globalPosition))
                addCandidate(result, rect, desktopBounds);
        }

        std::sort(result.begin(), result.end(), [](const QRect& a, const QRect& b) {
            return a.width() * a.height() < b.width() * b.height();
        });
        result.erase(std::unique(result.begin(), result.end()), result.end());
    } else {
        cachedHwnd_ = nullptr;
        cachedBounds_ = {};
        cachedChildRects_.clear();
        cachedUiRects_.clear();
    }

    return result;
#else
    Q_UNUSED(globalPosition)
    Q_UNUSED(desktopBounds)
    return {};
#endif
}

} // namespace snappaste
