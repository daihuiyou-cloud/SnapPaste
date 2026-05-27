#pragma once

#include "domain/editor/Annotation.h"

#include <functional>

#include <QCloseEvent>
#include <QImage>
#include <QMainWindow>

#include "presentation/editor/AnnotationCanvas.h"

namespace snappaste {



class EditorWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit EditorWindow(QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

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
