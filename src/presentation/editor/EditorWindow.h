#pragma once

#include "domain/editor/Annotation.h"

#include <functional>

#include <QImage>
#include <QMainWindow>

namespace snappaste {

class AnnotationCanvas;

class EditorWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit EditorWindow(QWidget* parent = nullptr);

public slots:
    void setImage(const QImage& image);
    void onToolChanged(AnnotationTool tool);

signals:
    void imageEdited(const QImage& image);
    void saveRequested();
    void copyRequested();
    void pinRequested(const QImage& image);

private:
    void createToolbar();

    AnnotationCanvas* canvas_ = nullptr;
    std::function<void(AnnotationTool)> updateToolActions_;
};

} // namespace snappaste
