#pragma once

#include "domain/editor/Annotation.h"

#include <QColor>
#include <QIcon>

namespace snappaste {

QIcon iconForTool(AnnotationTool tool);

QIcon makeColorIcon(const QColor& color);

}
