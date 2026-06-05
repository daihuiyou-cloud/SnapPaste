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
#pragma region Screen geometry cache

struct ScreenCacheEntry {
    QScreen* const screen;
    const QRect logicalGeo;
    const QRect nativeGeo;
    const qreal dpr;
};

static QVector<ScreenCacheEntry> s_screenCache;
static bool s_screenCacheValid = false;

static void ensureScreenCache()
{
    const auto screens = QGuiApplication::screens();
    if (s_screenCacheValid && s_screenCache.size() == screens.size()) {
        return;
    }
    s_screenCache.clear();
    for (auto* screen : screens) {
        if (screen == nullptr) continue;
        const auto geo = screen->geometry();
        const auto dpr = screen->devicePixelRatio();
        s_screenCache.push_back({screen, geo,
            QRect(qRound(geo.x() * dpr), qRound(geo.y() * dpr),
                  qRound(geo.width() * dpr), qRound(geo.height() * dpr)),
            dpr});
    }
    s_screenCacheValid = true;
}

static const ScreenCacheEntry* findEntryForScreen(const QScreen* screen)
{
    ensureScreenCache();
    for (const auto& e : s_screenCache) {
        if (e.screen == screen) return &e;
    }
    return nullptr;
}

static const ScreenCacheEntry* findEntryForNativePoint(const RECT& winRect)
{
    ensureScreenCache();
    const QPoint center((winRect.left + winRect.right) / 2, (winRect.top + winRect.bottom) / 2);
    for (const auto& e : s_screenCache) {
        if (e.nativeGeo.contains(center)) return &e;
    }
    return s_screenCache.isEmpty() ? nullptr : &s_screenCache.first();
}

static QPoint nativePointFromLogical(const QPoint& point)
{
    auto* screen = QGuiApplication::screenAt(point);
    if (screen == nullptr) screen = QGuiApplication::primaryScreen();
    if (screen == nullptr) return point;

    const auto* entry = findEntryForScreen(screen);
    if (entry == nullptr) return point;

    return QPoint(entry->nativeGeo.x() + qRound((point.x() - entry->logicalGeo.x()) * entry->dpr),
                  entry->nativeGeo.y() + qRound((point.y() - entry->logicalGeo.y()) * entry->dpr));
}

static QRect rectFromWinRect(const RECT& winRect)
{
    const auto* entry = findEntryForNativePoint(winRect);
    if (entry == nullptr) {
        return QRect(QPoint(winRect.left, winRect.top), QPoint(winRect.right - 1, winRect.bottom - 1));
    }

    const QPoint topLeft(entry->logicalGeo.x() + qRound((winRect.left - entry->nativeGeo.x()) / entry->dpr),
                         entry->logicalGeo.y() + qRound((winRect.top - entry->nativeGeo.y()) / entry->dpr));
    const QPoint bottomRight(entry->logicalGeo.x() + qRound((winRect.right - 1 - entry->nativeGeo.x()) / entry->dpr),
                             entry->logicalGeo.y() + qRound((winRect.bottom - 1 - entry->nativeGeo.y()) / entry->dpr));
    return QRect(topLeft, bottomRight).normalized();
}

#pragma endregion

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
        if (root == nullptr) {
            root = hwnd;
        }
        if (isOwnProcessWindow(root) || !isUsableWindow(root)) {
            HWND sibling = GetWindow(root, GW_HWNDPREV);
            if (sibling == nullptr) sibling = GetWindow(root, GW_HWNDNEXT);
            if (sibling) {
                HWND sibRoot = GetAncestor(sibling, GA_ROOT);
                RECT sr{};
                if (sibRoot && isUsableWindow(sibRoot)
                    && GetWindowRect(sibRoot, &sr) && PtInRect(&sr, nativePoint))
                    return sibRoot;
            }
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
WinScreenRegionDetector::~WinScreenRegionDetector()
{
#ifdef Q_OS_WIN
    if (uiWorker_.joinable()) {
        uiWorker_.join();
    }
#endif
}

#ifdef Q_OS_WIN
void WinScreenRegionDetector::rebuildCache(HWND hwnd, const QRect& desktopBounds)
{
    cachedChildRects_.clear();
    buildChildWindowCache(hwnd, cachedChildRects_);
    launchUiScan(hwnd);
}

void WinScreenRegionDetector::launchUiScan(HWND hwnd)
{
    if (uiWorker_.joinable()) {
        uiWorker_.join();
    }
    uiWorker_ = std::thread([this, hwnd]() {
        detail::ComInitializer comInit;
        QVector<QRect> rects;
        buildUiAutomationCache(hwnd, rects);
        std::lock_guard<std::mutex> lock(uiCacheMutex_);
        cachedUiRects_ = std::move(rects);
    });
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
        {
            std::lock_guard<std::mutex> lock(uiCacheMutex_);
            for (const auto& rect : cachedUiRects_) {
                if (rect.contains(globalPosition))
                    addCandidate(result, rect, desktopBounds);
            }
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
