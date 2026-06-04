#pragma once

#include "domain/ocr/OcrTypes.h"

#include <QFrame>
#include <QImage>
#include <QLabel>
#include <QPoint>
#include <QRect>
#include <QSet>
#include <QString>
#include <QVector>
#include <QWidget>

class QPushButton;
class QScrollArea;

namespace snappaste {

class OcrResultWindow final : public QWidget {
    Q_OBJECT

public:
    explicit OcrResultWindow(QImage source, QVector<OcrBlockInfo> blocks,
                             QString fullText, QWidget* parent = nullptr);
    ~OcrResultWindow() override = default;

signals:
    void pasteRequested();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void changeEvent(QEvent* event) override;
    void timerEvent(QTimerEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void performCopy();
    void performPaste();
    void rebuildCache();
    void updateImageOverlay();
    QString selectedText() const;
    void toggleIndex(int idx);
    void selectAll();
    void deselectAll();
    void updateTextRow(int i);
    void updateTextRows();
    void updateToolbar();
    void updateTitleButtons();
    void delayedRebuild();

    QImage source_;
    QVector<OcrBlockInfo> blocks_;
    QString fullText_;
    QPixmap basePixmap_;
    QVector<QRect> scaledRects_;
    int hoveredIndex_ = -1;
    QSet<int> selectedIndices_;
    QPoint dragStart_;
    QPoint dragMaximizeCheck_;
    bool dragging_ = false;
    bool maximized_ = false;
    int rebuildTimerId_ = 0;
    QRect normalGeometry_;

    QLabel* imageLabel_ = nullptr;
    QLabel* selectionInfo_ = nullptr;
    QPushButton* copyBtn_ = nullptr;
    QPushButton* pasteBtn_ = nullptr;
    QPushButton* minimizeBtn_ = nullptr;
    QPushButton* maximizeBtn_ = nullptr;
    QScrollArea* imageScrollArea_ = nullptr;
    QScrollArea* textScrollArea_ = nullptr;
    QWidget* textListContainer_ = nullptr;
    QVector<QFrame*> textRows_;

    static constexpr int kCornerRadius = 10;
    static constexpr int kTitleBarHeight = 38;
    static constexpr int kDefaultWidth = 820;
    static constexpr int kDefaultHeight = 560;
    static constexpr int kMinWidth = 500;
    static constexpr int kMinHeight = 360;
    static constexpr int kBlockPadding = 3;
};

} // namespace snappaste
