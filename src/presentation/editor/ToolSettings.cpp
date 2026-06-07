#include "presentation/editor/ToolSettings.h"

#include <QApplication>
#include <QFont>
#include <QSettings>
#include <QVariant>
#include <QVariantList>

namespace snappaste {

ToolSettings::ToolSettings()
{
    struct EditorPrefs {
        int fontSize = 14;
        QString fontFamily;
        bool bold = false, italic = false, underline = false;
        int textAlignment = -1;
        QVector<QColor> recentColors;
    };
    const EditorPrefs prefs = [] {
        QSettings s;
        EditorPrefs p;
        p.fontSize = s.value("editor/fontSize", 14).toInt();
        p.fontFamily = s.value("editor/fontFamily", QApplication::font().family()).toString();
        p.bold = s.value("editor/bold", false).toBool();
        p.italic = s.value("editor/italic", false).toBool();
        p.underline = s.value("editor/underline", false).toBool();
        p.textAlignment = s.value("editor/textAlignment", -1).toInt();
        const auto saved = s.value("editor/recentColors").toList();
        for (const auto& v : saved) {
            QColor c(v.toString());
            if (c.isValid())
                p.recentColors.append(c);
        }
        return p;
    }();

    fontSize = prefs.fontSize;
    currentFontFamily = prefs.fontFamily;
    bold = prefs.bold;
    italic = prefs.italic;
    underline = prefs.underline;
    textAlignment = prefs.textAlignment;
    customColors = prefs.recentColors;
}

void ToolSettings::addRecentColor(const QColor& color)
{
    customColors.removeAll(color);
    customColors.prepend(color);
    if (customColors.size() > 6) {
        customColors.resize(6);
    }
    QVariantList saved;
    for (const auto& c : customColors)
        saved.append(c.name());
    QSettings().setValue("editor/recentColors", saved);
}

} // namespace snappaste
