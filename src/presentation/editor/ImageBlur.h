#pragma once

#include <QImage>
#include <QRect>

namespace snappaste {

QImage blurImage(const QImage& source, int radius);
QImage blurImageRegion(const QImage& source, const QRect& region, int radius);

}
