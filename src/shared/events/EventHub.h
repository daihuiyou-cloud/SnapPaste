#pragma once

#include <QObject>

namespace nanosnap {

class EventHub final : public QObject {
    Q_OBJECT

public:
    explicit EventHub(QObject* parent = nullptr)
        : QObject(parent)
    {
    }

signals:
    void historyChanged();
    void settingsChanged();
};

} // namespace nanosnap
