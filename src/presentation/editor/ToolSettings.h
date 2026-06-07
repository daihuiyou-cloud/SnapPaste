#pragma once

#include "domain/editor/Annotation.h"

#include <QColor>
#include <QFont>
#include <QString>
#include <QVector>

namespace snappaste {

struct ToolSettings {
    ToolSettings();

    void addRecentColor(const QColor& color);

    AnnotationTool currentTool = AnnotationTool::Rectangle;
    QColor currentColor{"#ff3b30"};
    QColor currentFillColor;
    int currentStrokeWidth = 4;
    int strokeAlpha = 255;
    ArrowStyle arrowStyle = ArrowStyle::DefaultArrow;
    int cornerRadius = 0;
    int fontSize = 14;
    QString currentFontFamily;
    bool bold = false;
    bool italic = false;
    bool underline = false;
    int textAlignment = -1;
    bool textOutlineEnabled = true;
    bool filled = false;
    bool textBackgroundEnabled = false;
    QColor textBackgroundColor{0, 0, 0, 80};
    bool mosaicBlurred = false;
    bool pickingColor = false;
    double cropAspectRatio = 0.0;
    bool gridEnabled = false;
    double zoomFactor = 1.0;

    QVector<AnnotationTool> recentTools;
    QVector<QColor> customColors;

    struct FontCacheKey {
        QString fontFamily;
        int fontSize = 0;
        bool bold = false;
        bool italic = false;
        bool underline = false;
        bool operator==(const FontCacheKey& o) const {
            return fontFamily == o.fontFamily && fontSize == o.fontSize
                && bold == o.bold && italic == o.italic && underline == o.underline;
        }
    };
    FontCacheKey fontCacheKey;
    QFont cachedFont;
};

} // namespace snappaste
