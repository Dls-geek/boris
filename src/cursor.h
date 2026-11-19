#pragma once

#include <QObject>
#include <QPoint>
#include <QTimer>

// Global pointer for a glance while he walks. Wayland will not give clients the
// cursor, so on Hyprland we ask the compositor over its IPC socket. Elsewhere
// this reports unavailable and he only looks while the pointer is on him.
//
// Pattern adapted from Nala (https://github.com/yappologistic/Nala), MIT.
class Cursor : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool available READ available NOTIFY availableChanged)

public:
    explicit Cursor(QObject *parent = nullptr);

    bool available() const { return m_available; }
    QPoint position() const { return m_position; }

    void setActive(bool active);
    void inject(QPoint position);

signals:
    void moved(QPoint position);
    void availableChanged();

private:
    void poll();
    void setAvailable(bool available);

    QString m_socket;
    QPoint m_position;
    QTimer m_timer;
    bool m_available = false;
    bool m_active = false;
    int m_failures = 0;
};
