#include "infrastructure/logging/Logger.h"

#include <QDebug>
#include <QLoggingCategory>

namespace snappaste {

void Logger::install()
{
    QLoggingCategory::setFilterRules("qt.qpa.*=false\n");
}

void Logger::info(const QString& message)
{
    qInfo().noquote() << message;
}

void Logger::warning(const QString& message)
{
    qWarning().noquote() << message;
}

} // namespace snappaste
