#include "presentation/editor/ImageBlur.h"

#include <QtGlobal>

#include <array>
#include <algorithm>

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

// Process vertical blur in cache-friendly tiles (64 columns at a time).
// The accumulator arrays for each column stay hot in L1 cache, and scanLine
// accesses within a row are sequential in x.
void blurVertical(const QImage& src, QImage& dst, int radius)
{
    const int w = src.width(), h = src.height();
    constexpr int kTileW = 64;

    for (int x0 = 0; x0 < w; x0 += kTileW) {
        int xEnd = (std::min)(x0 + kTileW, w);

        std::array<int, kTileW> a{}, r{}, g{}, b{}, cnt{};

        for (int y = 0; y <= radius && y < h; ++y) {
            const auto* line = reinterpret_cast<const QRgb*>(src.constScanLine(y));
            for (int x = x0; x < xEnd; ++x) {
                auto px = line[x]; int idx = x - x0;
                a[idx] += qAlpha(px); r[idx] += qRed(px); g[idx] += qGreen(px); b[idx] += qBlue(px); cnt[idx]++;
            }
        }

        for (int y = 0; y < h; ++y) {
            auto* dstLine = reinterpret_cast<QRgb*>(dst.scanLine(y));
            for (int x = x0; x < xEnd; ++x) {
                int idx = x - x0;
                if (cnt[idx] > 0)
                    dstLine[x] = qRgba(r[idx] / cnt[idx], g[idx] / cnt[idx], b[idx] / cnt[idx], a[idx] / cnt[idx]);
            }

            for (int x = x0; x < xEnd; ++x) {
                int idx = x - x0;
                int top = y - radius;
                if (top >= 0) {
                    auto px = reinterpret_cast<const QRgb*>(src.constScanLine(top))[x];
                    a[idx] -= qAlpha(px); r[idx] -= qRed(px); g[idx] -= qGreen(px); b[idx] -= qBlue(px); cnt[idx]--;
                }
                int bottom = y + radius + 1;
                if (bottom < h) {
                    auto px = reinterpret_cast<const QRgb*>(src.constScanLine(bottom))[x];
                    a[idx] += qAlpha(px); r[idx] += qRed(px); g[idx] += qGreen(px); b[idx] += qBlue(px); cnt[idx]++;
                }
            }
        }
    }
}

} // namespace

QImage blurImage(const QImage& source, int radius)
{
    if (radius <= 0 || source.isNull()) return source;
    QImage result = source.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    QImage tmp(result.size(), QImage::Format_ARGB32_Premultiplied);
    for (int i = 0; i < 3; ++i) {
        blurHorizontal(result, tmp, radius);
        blurVertical(tmp, result, radius);
    }
    return result;
}

QImage blurImageRegion(const QImage& source, const QRect& region, int radius)
{
    if (source.isNull()) return QImage();
    auto clipped = region.intersected(QRect(QPoint(0, 0), source.size()));
    if (clipped.isEmpty() || radius <= 0) {
        return source.copy(clipped);
    }
    QImage subImage = source.copy(clipped);
    return blurImage(subImage, radius);
}

}
