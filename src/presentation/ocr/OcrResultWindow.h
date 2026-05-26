#pragma once

#include "domain/ocr/OcrTypes.h"

#include <QImage>
#include <QPoint>
#include <QRect>
#include <QSet>
#include <QString>
#include <QVector>
#include <QWidget>

class QLabel;

namespace snappaste {

class OcrResultWindow final : public QWidget {
    Q_OBJECT

public:
    explicit OcrResultWindow(const QImage& source, const QVector<OcrBlockInfo>& blocks,
                             const QString& fullText, QWidget* parent = nullptr);
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

private:
    void performCopy();
    void performPaste();
    void rebuildCache();
    int hitTest(const QPoint& pos) const;
    QString selectedText() const;

    QImage source_;
    QVector<OcrBlockInfo> blocks_;
    QString fullText_;
    QPixmap cachedPixmap_;
    QVector<QRect> scaledRects_;
    int hoveredIndex_ = -1;
    QSet<int> selectedIndices_;
    QPoint dragStart_;
    bool dragging_ = false;
    static constexpr int kCornerRadius = 8;
    static constexpr int kTitleBarHeight = 32;
    static constexpr int kMaxWidth = 680;
    static constexpr int kMaxHeight = 520;
    static constexpr int kBlockPadding = 3;
};

} // namespace snappaste