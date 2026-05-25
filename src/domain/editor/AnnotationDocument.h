#pragma once

#include "domain/editor/Annotation.h"
#include "shared/result/Result.h"

#include <QJsonArray>
#include <QVector>

namespace nanosnap {

class AnnotationDocument final {
public:
    void add(Annotation annotation);
    void clear();
    const QVector<Annotation>& annotations() const noexcept;

    Result<QJsonArray> toJson() const;

private:
    QVector<Annotation> annotations_;
};

} // namespace nanosnap
