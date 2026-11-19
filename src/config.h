#pragma once

#include <QObject>
#include <QString>

// Settings the window writes. Size and Nala-style attention toggles live here
// so the next launch still feels like him.
class Config : public QObject {
    Q_OBJECT
    Q_PROPERTY(int scalePercent READ scalePercent WRITE setScalePercent NOTIFY scalePercentChanged)
    Q_PROPERTY(bool followCursor READ followCursor WRITE setFollowCursor NOTIFY attentionChanged)
    Q_PROPERTY(bool idleAntics READ idleAntics WRITE setIdleAntics NOTIFY attentionChanged)
    Q_PROPERTY(bool sleepWhenIdle READ sleepWhenIdle WRITE setSleepWhenIdle NOTIFY attentionChanged)
    Q_PROPERTY(bool chirpsEnabled READ chirpsEnabled WRITE setChirpsEnabled NOTIFY aliveChanged)
    Q_PROPERTY(bool wakeWordEnabled READ wakeWordEnabled WRITE setWakeWordEnabled NOTIFY aliveChanged)
    Q_PROPERTY(QString wakePhrase READ wakePhrase WRITE setWakePhrase NOTIFY aliveChanged)
    Q_PROPERTY(QString replyApiKey READ replyApiKey WRITE setReplyApiKey NOTIFY aliveChanged)
    Q_PROPERTY(QString replyApiUrl READ replyApiUrl WRITE setReplyApiUrl NOTIFY aliveChanged)

public:
    static constexpr int MinimumScale = 40;
    static constexpr int MaximumScale = 200;
    static constexpr int DefaultScale = 100;

    explicit Config(const QString &directory = {}, QObject *parent = nullptr);

    int scalePercent() const { return m_scalePercent; }
    void setScalePercent(int percent);

    bool followCursor() const { return m_followCursor; }
    void setFollowCursor(bool enabled);

    bool idleAntics() const { return m_idleAntics; }
    void setIdleAntics(bool enabled);

    bool sleepWhenIdle() const { return m_sleepWhenIdle; }
    void setSleepWhenIdle(bool enabled);

    bool chirpsEnabled() const { return m_chirpsEnabled; }
    void setChirpsEnabled(bool enabled);

    bool wakeWordEnabled() const { return m_wakeWordEnabled; }
    void setWakeWordEnabled(bool enabled);

    QString wakePhrase() const { return m_wakePhrase; }
    void setWakePhrase(const QString &phrase);

    QString replyApiKey() const { return m_replyApiKey; }
    void setReplyApiKey(const QString &key);

    QString replyApiUrl() const { return m_replyApiUrl; }
    void setReplyApiUrl(const QString &url);

    static int clampScale(int percent);

signals:
    void scalePercentChanged();
    void attentionChanged();
    void aliveChanged();

private:
    bool save() const;
    void load();

    QString m_path;
    int m_scalePercent = DefaultScale;
    bool m_followCursor = true;
    bool m_idleAntics = true;
    bool m_sleepWhenIdle = true;
    bool m_chirpsEnabled = true;
    bool m_wakeWordEnabled = false;
    QString m_wakePhrase = QStringLiteral("boris");
    QString m_replyApiKey;
    QString m_replyApiUrl;
};
