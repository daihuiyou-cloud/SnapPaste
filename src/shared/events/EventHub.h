#pragma once

#include <QImage>
#include <QObject>
#include <QString>

namespace snappaste {

class EventHub final : public QObject {
    Q_OBJECT

public:
    explicit EventHub(QObject* parent = nullptr)
        : QObject(parent)
    {
    }

    // --- Capture domain ---
signals:
    void captureCompleted(const QImage& image);
    void captureError(const QString& message);
    void captureCopied();
    void captureSaved(const QString& filePath);

    // --- Pin domain ---
signals:
    void pinCreated(qint64 id);
    void pinClosed(qint64 id);
    void pinStateChanged(qint64 id);
    void allPinsHidden();
    void allPinsShown();

    // --- History domain ---
signals:
    void historyChanged();
    void historyCleared();

    // --- Settings domain ---
signals:
    void settingsChanged();
    void themeChanged();

    // --- OCR domain ---
signals:
    void ocrCompleted(const QString& text);
    void ocrError(const QString& message);
};

} // namespace snappaste
