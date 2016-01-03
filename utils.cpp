#include "utils.h"

#include <QDateTime>

namespace utils
{
QString getTimestampFileName(QString prefix, QString extention)
{
    const  QDateTime now = QDateTime::currentDateTime();
    const  QString timestamp = now.toString("yyyyMMdd-hhmmsszzz");
    return QString("%1-%2.%3").arg(prefix).arg(timestamp).arg(extention);
}
}
