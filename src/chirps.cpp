#include "chirps.h"

#include <QSoundEffect>
#include <QUrl>

Chirps::Chirps(QObject *parent) : QObject(parent)
{
    m_talk = new QSoundEffect(this);
    m_excited = new QSoundEffect(this);
    prepare(m_talk, QStringLiteral("qrc:/sounds/talk.wav"));
    prepare(m_excited, QStringLiteral("qrc:/sounds/excited.wav"));
}

void Chirps::prepare(QSoundEffect *effect, const QString &resource)
{
    effect->setSource(QUrl(resource));
    effect->setVolume(0.55);
}

void Chirps::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

void Chirps::playTalk()
{
    if (!m_enabled)
        return;
    if (m_talk->status() == QSoundEffect::Error)
        return;
    m_talk->play();
}

void Chirps::playExcited()
{
    if (!m_enabled)
        return;
    if (m_excited->status() == QSoundEffect::Error)
        return;
    m_excited->play();
}
