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

bool Config::save() const
{
    QJsonObject object;
    object.insert(QStringLiteral("scale"), m_scalePercent);
    object.insert(QStringLiteral("followCursor"), m_followCursor);
    object.insert(QStringLiteral("idleAntics"), m_idleAntics);
    object.insert(QStringLiteral("sleepWhenIdle"), m_sleepWhenIdle);
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
}
