#include "presentation/editor/ImageBlur.h"

#include <QtGlobal>

namespace snappaste {

namespace {

void blurHorizontal(const QImage& src, QImage& dst, int radius)
{
    const int w = src.width(), h = src.height();
    for (int y = 0; y < h; ++y) {
        const auto* in = reinterpret_cast<const QRgb*>(src.constScanLine(y));
        auto* out = reinterpret_cast<QRgb*>(dst.scanLine(y));
        int a = 0, r = 0, g = 0, b = 0, cnt = 0;
        for (int x = 0; x <= radius && x < w; ++x) {
            auto px = in[x]; a += qAlpha(px); r += qRed(px); g += qGreen(px); b += qBlue(px); ++cnt;
        }
        for (int x = 0; x < w; ++x) {
            if (cnt > 0) out[x] = qRgba(r / cnt, g / cnt, b / cnt, a / cnt);
            int left = x - radius;
            if (left >= 0) { auto px = in[left]; a -= qAlpha(px); r -= qRed(px); g -= qGreen(px); b -= qBlue(px); --cnt; }
            int right = x + radius + 1;
            if (right < w) { auto px = in[right]; a += qAlpha(px); r += qRed(px); g += qGreen(px); b += qBlue(px); ++cnt; }
        }
    }
}

void blurVertical(const QImage& src, QImage& dst, int radius)
{
    const int w = src.width(), h = src.height();
    for (int x = 0; x < w; ++x) {
        int a = 0, r = 0, g = 0, b = 0, cnt = 0;
        for (int y = 0; y <= radius && y < h; ++y) {
            auto px = reinterpret_cast<const QRgb*>(src.constScanLine(y))[x];
            a += qAlpha(px); r += qRed(px); g += qGreen(px); b += qBlue(px); ++cnt;
        }
        for (int y = 0; y < h; ++y) {
            if (cnt > 0) reinterpret_cast<QRgb*>(dst.scanLine(y))[x] = qRgba(r / cnt, g / cnt, b / cnt, a / cnt);
            int top = y - radius;
            if (top >= 0) { auto px = reinterpret_cast<const QRgb*>(src.constScanLine(top))[x]; a -= qAlpha(px); r -= qRed(px); g -= qGreen(px); b -= qBlue(px); --cnt; }
            int bottom = y + radius + 1;
            if (bottom < h) { auto px = reinterpret_cast<const QRgb*>(src.constScanLine(bottom))[x]; a += qAlpha(px); r += qRed(px); g += qGreen(px); b += qBlue(px); ++cnt; }
        }
    }
}

} // namespace

QImage blurImage(QImage source, int radius)
{
    if (radius <= 0 || source.isNull()) return source;
    source = source.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    QImage tmp(source.size(), QImage::Format_ARGB32_Premultiplied);
    for (int i = 0; i < 3; ++i) {
        blurHorizontal(source, tmp, radius);
        blurVertical(tmp, source, radius);
    }
    return source;
}

}
