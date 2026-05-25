#pragma once

#include "domain/editor/Annotation.h"

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

signals:
    void imageEdited(const QImage& image);
    void saveRequested();
    void copyRequested();

private:
    void createToolbar();

    AnnotationCanvas* canvas_ = nullptr;
};

} // namespace snappaste
