#pragma once

#include "domain/editor/Annotation.h"
#include "presentation/icons/IIconProvider.h"

#include <QColor>
#include <QIcon>

namespace snappaste {

QIcon iconForTool(AnnotationTool tool, IIconProvider& iconProvider);

QIcon makeColorIcon(const QColor& color);

}
