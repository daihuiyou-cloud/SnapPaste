#include "domain/editor/AnnotationDocument.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QPoint>

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

static QJsonArray pointsToJson(const QVector<QPoint>& points)
{
    QJsonArray arr;
    for (const auto& pt : points) {
        QJsonObject obj;
        obj["x"] = pt.x();
        obj["y"] = pt.y();
        arr.append(obj);
    }
    return arr;
}

static QVector<QPoint> pointsFromJson(const QJsonArray& array)
{
    QVector<QPoint> points;
    points.reserve(array.size());
    for (const auto& val : array) {
        const auto obj = val.toObject();
        points.push_back(QPoint(obj["x"].toInt(), obj["y"].toInt()));
    }
    return points;
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
        object["points"] = pointsToJson(annotation.points);
        array.append(object);
    }
    return Result<QJsonArray>::success(array);
}

Result<void> AnnotationDocument::fromJson(const QJsonArray& array)
{
    annotations_.clear();
    annotations_.reserve(array.size());

    for (const auto& val : array) {
        if (!val.isObject()) {
            return Result<void>::failure("Invalid annotation entry in JSON");
        }
        const auto object = val.toObject();
        Annotation ann;
        ann.tool = static_cast<AnnotationTool>(object["tool"].toInt());
        ann.bounds = QRect(object["x"].toInt(), object["y"].toInt(),
                           object["width"].toInt(), object["height"].toInt());
        ann.text = object["text"].toString();
        if (object.contains("color")) {
            const auto c = QColor(object["color"].toString());
            if (c.isValid()) {
                ann.color = c;
            }
        }
        ann.strokeWidth = object["strokeWidth"].toInt(3);
        ann.points = pointsFromJson(object["points"].toArray());
        annotations_.push_back(std::move(ann));
    }
    return Result<void>::success();
}

} // namespace snappaste
