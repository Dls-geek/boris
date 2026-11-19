#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

class QNetworkAccessManager;
class QNetworkReply;

// Short pirate lines only — not a chatbot. Local canned lines by default;
// optional user-supplied OpenAI-compatible API key upgrades one line.
class Personality : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString lastLine READ lastLine NOTIFY lastLineChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
    explicit Personality(QObject *parent = nullptr);

    QString lastLine() const { return m_lastLine; }
    bool busy() const { return m_busy; }

    QString apiKey() const { return m_apiKey; }
    Q_INVOKABLE void setApiKey(const QString &key);
    QString apiUrl() const { return m_apiUrl; }
    Q_INVOKABLE void setApiUrl(const QString &url);

    // Sets lastLine to a local line immediately. With an API key, may replace it.
    Q_INVOKABLE void replyTo(const QString &heard);
    Q_INVOKABLE QString pickLocalLine() const;
    Q_INVOKABLE void clearLine();

signals:
    void lastLineChanged();
    void busyChanged();
    void lineReady(const QString &line);

private:
    void setLastLine(const QString &line);
    void setBusy(bool busy);
    void requestRemote(const QString &heard, const QString &fallback);

    QNetworkAccessManager *m_network = nullptr;
    QNetworkReply *m_reply = nullptr;
    QString m_lastLine;
    QString m_apiKey;
    QString m_apiUrl = QStringLiteral("https://api.openai.com/v1/chat/completions");
    bool m_busy = false;
};
