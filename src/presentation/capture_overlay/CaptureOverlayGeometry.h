#pragma once

#include <QRect>
#include <QWidget>

namespace snappaste {

constexpr int kHandleSize = 8;
constexpr int kHandleHitSlop = 5;
constexpr int kMinSelectionSize = 8;
constexpr int kNudgeStep = 1;
constexpr int kFastNudgeStep = 10;
constexpr int kDragThreshold = 4;
constexpr int kOverlayFrameIntervalMs = 6;
constexpr int kSmartCandidateIntervalMs = 120;
constexpr int kFullScreenSelectionInset = 1;
constexpr int kOverlayMaskAlpha = 96;
constexpr int kSizeLabelHeight = 20;
constexpr int kSizeLabelPaddingX = 10;
constexpr int kSizeLabelRadius = 3;
constexpr int kOverlayMargin = 8;

QRect normalizedWithMinimum(QRect rect);
QRect clampedTo(QRect rect, const QRect& bounds);
QRect insetIfFullScreen(QRect rect, const QRect& desktopBounds);
QRect selectableRegion(QRect rect, const QRect& bounds);
void applyNativeDesktopBounds(QWidget& widget);

}
