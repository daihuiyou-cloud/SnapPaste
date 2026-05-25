#include "domain/editor/AnnotationDocument.h"

#include <QJsonObject>

namespace snappaste {

void AnnotationDocument::add(Annotation annotation)
{
    annotations_.push_back(std::move(annotation));
}

void AnnotationDocument::clear()
{
    annotations_.clear();
}

const QVector<Annotation>& AnnotationDocument::annotations() const noexcept
{
    return annotations_;
}

Result<QJsonArray> AnnotationDocument::toJson() const
{
    QJsonArray array;
    for (const auto& annotation : annotations_) {
        QJsonObject object;
        object["tool"] = static_cast<int>(annotation.tool);
        object["x"] = annotation.bounds.x();
        object["y"] = annotation.bounds.y();
        object["width"] = annotation.bounds.width();
        object["height"] = annotation.bounds.height();
        object["text"] = annotation.text;
        object["color"] = annotation.color.name();
        object["strokeWidth"] = annotation.strokeWidth;
        array.append(object);
    }

    return Result<QJsonArray>::success(array);
}

} // namespace snappaste
