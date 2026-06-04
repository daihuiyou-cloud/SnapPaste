#include "infrastructure/clipboard/ClipboardImageProvider.h"

#include <QApplication>
#include <QClipboard>
#include <QColor>
#include <QFontMetrics>
#include <QImage>
#include <QMimeData>
#include <QPainter>
#include <QTextDocument>
#include <QVariant>

namespace snappaste {

Result<QImage> ClipboardImageProvider::imageFromClipboard()
{
    const auto* clipboard = QApplication::clipboard();
    if (clipboard == nullptr) {
        return Result<QImage>::failure(QCoreApplication::translate("AppErrors", "Cannot access clipboard."));
    }

    const auto* mimeData = clipboard->mimeData();
    if (mimeData == nullptr) {
        return Result<QImage>::failure(QCoreApplication::translate("AppErrors", "Nothing on clipboard \u2014 copy an image or text first, then press Paste."));
    }

    const auto image = clipboard->image();
    if (!image.isNull()) {
        return Result<QImage>::success(std::move(image));
    }

    if (mimeData->hasColor()) {
        const auto color = qvariant_cast<QColor>(mimeData->colorData());
        QImage output(240, 160, QImage::Format_ARGB32_Premultiplied);
        output.fill(color);
        return Result<QImage>::success(std::move(output));
    }

    if (mimeData->hasText()) {
        if (mimeData->hasHtml()) {
            return imageFromHtml(mimeData->html());
        }
        const auto colorResult = colorImageFromText(mimeData->text().trimmed());
        if (colorResult.isOk()) {
            return colorResult;
        }
        return imageFromText(mimeData->text());
    }

    return Result<QImage>::failure(QCoreApplication::translate("AppErrors", "Clipboard content is not supported. Try copying an image or text first."));
}

Result<QImage> ClipboardImageProvider::imageFromHtml(const QString& html)
{
    if (html.trimmed().isEmpty()) {
        return Result<QImage>::failure(QCoreApplication::translate("AppErrors", "Clipboard HTML is empty."));
    }

    QTextDocument document;
    document.setHtml(html.left(8000));
    document.setTextWidth(520);

    const QSize size(std::max(260, static_cast<int>(document.idealWidth()) + 36),
                     std::max(120, static_cast<int>(document.size().height()) + 36));
    QImage output(size, QImage::Format_ARGB32_Premultiplied);
    output.fill(QColor("#fffdf7"));

    QPainter painter(&output);
    painter.translate(18, 18);
    document.drawContents(&painter, QRectF(0, 0, size.width() - 36, size.height() - 36));
    return Result<QImage>::success(std::move(output));
}

Result<QImage> ClipboardImageProvider::imageFromText(const QString& text)
{
    if (text.trimmed().isEmpty()) {
        return Result<QImage>::failure(QCoreApplication::translate("AppErrors", "Clipboard text is empty."));
    }

    QFont font;
    font.setPointSize(14);
    const QFontMetrics metrics(font);
    const auto bounded = metrics.boundingRect(QRect(0, 0, 520, 800),
                                              Qt::TextWordWrap,
                                              text.left(1200));
    const QSize size(std::max(260, bounded.width() + 36), std::max(120, bounded.height() + 36));

    QImage output(size, QImage::Format_ARGB32_Premultiplied);
    output.fill(QColor("#fffdf7"));

    QPainter painter(&output);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setFont(font);
    painter.setPen(QColor("#20242a"));
    painter.drawText(output.rect().adjusted(18, 18, -18, -18),
                     Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                     text.left(1200));

    return Result<QImage>::success(std::move(output));
}

Result<QImage> ClipboardImageProvider::colorImageFromText(const QString& text)
{
    QColor color(text);
    if (!color.isValid()) {
        return Result<QImage>::failure(QCoreApplication::translate("AppErrors", "Clipboard text is not a color."));
    }

    QImage output(260, 180, QImage::Format_ARGB32_Premultiplied);
    output.fill(color);

    QPainter painter(&output);
    painter.setPen(color.lightness() < 120 ? Qt::white : Qt::black);
    painter.drawText(output.rect(), Qt::AlignCenter, color.name().toUpper());

    return Result<QImage>::success(std::move(output));
}

} // namespace snappaste
