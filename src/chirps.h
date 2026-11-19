#pragma once

#include <QObject>

class QSoundEffect;

// Short WAV chirps for talk and excited — hop stays silent.
class Chirps : public QObject {
    Q_OBJECT

public:
    explicit Chirps(QObject *parent = nullptr);

    bool enabled() const { return m_enabled; }
    void setEnabled(bool enabled);

    Q_INVOKABLE void playTalk();
    Q_INVOKABLE void playExcited();

private:
    void prepare(QSoundEffect *effect, const QString &resource);

    QSoundEffect *m_talk = nullptr;
    QSoundEffect *m_excited = nullptr;
    bool m_enabled = true;
};
