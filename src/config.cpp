#include "config.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

#include <algorithm>

namespace {
QString defaultDirectory()
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return base.isEmpty() ? QDir::homePath() + QStringLiteral("/.local/share/boris") : base;
}
}

Config::Config(const QString &directory, QObject *parent)
    : QObject(parent), m_path((directory.isEmpty() ? defaultDirectory() : directory) + QStringLiteral("/settings.json"))
{
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    load();
}

int Config::clampScale(int percent)
{
    return std::clamp(percent, MinimumScale, MaximumScale);
}

void Config::setScalePercent(int percent)
{
    percent = clampScale(percent);
    if (percent == m_scalePercent)
        return;
    m_scalePercent = percent;
    if (!save())
        qWarning("Boris could not save his size.");
    emit scalePercentChanged();
}

void Config::setFollowCursor(bool enabled)
{
    if (m_followCursor == enabled)
        return;
    m_followCursor = enabled;
    if (!save())
        qWarning("Boris could not save his attention settings.");
    emit attentionChanged();
}

void Config::setIdleAntics(bool enabled)
{
    if (m_idleAntics == enabled)
        return;
    m_idleAntics = enabled;
    if (!save())
        qWarning("Boris could not save his attention settings.");
    emit attentionChanged();
}

void Config::setSleepWhenIdle(bool enabled)
{
    if (m_sleepWhenIdle == enabled)
        return;
    m_sleepWhenIdle = enabled;
    if (!save())
        qWarning("Boris could not save his attention settings.");
    emit attentionChanged();
}

void Config::setChirpsEnabled(bool enabled)
{
    if (m_chirpsEnabled == enabled)
        return;
    m_chirpsEnabled = enabled;
    if (!save())
        qWarning("Boris could not save his alive settings.");
    emit aliveChanged();
}

void Config::setWakeWordEnabled(bool enabled)
{
    if (m_wakeWordEnabled == enabled)
        return;
    m_wakeWordEnabled = enabled;
    if (!save())
        qWarning("Boris could not save his alive settings.");
    emit aliveChanged();
}

void Config::setWakePhrase(const QString &phrase)
{
    QString next = phrase.trimmed();
    if (next.isEmpty())
        next = QStringLiteral("boris");
    if (next.size() > 40)
        next = next.left(40).trimmed();
    if (m_wakePhrase == next)
        return;
    m_wakePhrase = next;
    if (!save())
        qWarning("Boris could not save his alive settings.");
    emit aliveChanged();
}

void Config::setReplyApiKey(const QString &key)
{
    if (m_replyApiKey == key)
        return;
    m_replyApiKey = key;
    if (!save())
        qWarning("Boris could not save his alive settings.");
    emit aliveChanged();
}

void Config::setReplyApiUrl(const QString &url)
{
    if (m_replyApiUrl == url)
        return;
    m_replyApiUrl = url;
    if (!save())
        qWarning("Boris could not save his alive settings.");
    emit aliveChanged();
}

bool Config::save() const
{
    QJsonObject object;
    object.insert(QStringLiteral("scale"), m_scalePercent);
    object.insert(QStringLiteral("followCursor"), m_followCursor);
    object.insert(QStringLiteral("idleAntics"), m_idleAntics);
    object.insert(QStringLiteral("sleepWhenIdle"), m_sleepWhenIdle);
    object.insert(QStringLiteral("chirpsEnabled"), m_chirpsEnabled);
    object.insert(QStringLiteral("wakeWordEnabled"), m_wakeWordEnabled);
    object.insert(QStringLiteral("wakePhrase"), m_wakePhrase);
    object.insert(QStringLiteral("replyApiKey"), m_replyApiKey);
    object.insert(QStringLiteral("replyApiUrl"), m_replyApiUrl);
    const QByteArray bytes = QJsonDocument(object).toJson(QJsonDocument::Compact);
    const QString temp = m_path + QStringLiteral(".tmp");
    QFile file(temp);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size())
        return false;
    file.close();
    QFile::remove(m_path);
    if (QFile::rename(temp, m_path))
        return true;
    QFile::remove(temp);
    return false;
}

void Config::load()
{
    QFile file(m_path);
    if (!file.open(QIODevice::ReadOnly))
        return;
    const QJsonObject object = QJsonDocument::fromJson(file.readAll()).object();
    const QJsonValue scale = object.value(QStringLiteral("scale"));
    if (scale.isDouble())
        m_scalePercent = clampScale(scale.toInt());
    if (object.contains(QStringLiteral("followCursor")))
        m_followCursor = object.value(QStringLiteral("followCursor")).toBool(true);
    if (object.contains(QStringLiteral("idleAntics")))
        m_idleAntics = object.value(QStringLiteral("idleAntics")).toBool(true);
    if (object.contains(QStringLiteral("sleepWhenIdle")))
        m_sleepWhenIdle = object.value(QStringLiteral("sleepWhenIdle")).toBool(true);
    if (object.contains(QStringLiteral("chirpsEnabled")))
        m_chirpsEnabled = object.value(QStringLiteral("chirpsEnabled")).toBool(true);
    if (object.contains(QStringLiteral("wakeWordEnabled")))
        m_wakeWordEnabled = object.value(QStringLiteral("wakeWordEnabled")).toBool(false);
    else if (object.contains(QStringLiteral("wakeMicEnabled")))
        m_wakeWordEnabled = object.value(QStringLiteral("wakeMicEnabled")).toBool(false);
    if (object.contains(QStringLiteral("wakePhrase"))) {
        const QString phrase = object.value(QStringLiteral("wakePhrase")).toString().trimmed();
        if (!phrase.isEmpty())
            m_wakePhrase = phrase.left(40);
    }
    if (object.contains(QStringLiteral("replyApiKey")))
        m_replyApiKey = object.value(QStringLiteral("replyApiKey")).toString();
    if (object.contains(QStringLiteral("replyApiUrl")))
        m_replyApiUrl = object.value(QStringLiteral("replyApiUrl")).toString();
}
