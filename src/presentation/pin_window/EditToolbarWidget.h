#pragma once

#include "domain/editor/Annotation.h"
#include "presentation/icons/IIconProvider.h"

#include <QPoint>
#include <QWidget>

class QToolButton;

namespace snappaste {

class EditToolbarWidget final : public QWidget {
    Q_OBJECT

public:
    explicit EditToolbarWidget(IIconProvider& iconProvider, QWidget* parent = nullptr);

    void setCurrentTool(AnnotationTool tool);
    void setCanUndo(bool enable);
    void setCanRedo(bool enable);

signals:
    void toolSelected(AnnotationTool tool);
    void undoRequested();
    void redoRequested();
    void doneRequested();
    void dragFinished();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    QToolButton* createToolButton(IconName icon, const QString& text, const QString& tooltip);
    void updateButtonStates();

    IIconProvider& iconProvider_;
    AnnotationTool currentTool_ = AnnotationTool::Select;
    bool dragging_ = false;
    QPoint dragOffset_;
    QVector<QToolButton*> toolButtons_;
    QToolButton* undoBtn_ = nullptr;
    QToolButton* redoBtn_ = nullptr;
    QToolButton* doneBtn_ = nullptr;
};

} // namespace snappaste
