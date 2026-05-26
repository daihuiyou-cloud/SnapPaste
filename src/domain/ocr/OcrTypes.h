#pragma once

#include <QImage>
#include <QRect>
#include <QString>
#include <QVector>

namespace snappaste {

struct OcrBlockInfo {
    QString text;
    QRect rect;
};

struct OcrResult {
    bool ok = false;
    QString text;
    QString message;
    QImage image;
    QVector<OcrBlockInfo> blocks;
};

} // namespace snappaste
