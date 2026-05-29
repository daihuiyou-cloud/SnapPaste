#pragma once

#include "domain/editor/Annotation.h"

#include <functional>

#include <QAction>
#include <QCloseEvent>
#include <QImage>
#include <QLabel>
#include <QMainWindow>
#include <QToolButton>

namespace snappaste {

class AnnotationCanvas;

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
    void createToolPanel();
    void rebuildColorMenu();
    void updateColorWell(const QColor& c);

    AnnotationCanvas* canvas_ = nullptr;
    QToolButton* colorBtn_ = nullptr;
    QLabel* contextHint_ = nullptr;
    QLabel* imageInfoLabel_ = nullptr;
    QAction* eyeAction_ = nullptr;
    std::function<void(AnnotationTool)> updateToolActions_;
};

} // namespace snappaste
