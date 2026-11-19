#include "personality.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QUrl>

namespace {
const QStringList kLines{
    QStringLiteral("Aye, captain?"),
    QStringLiteral("Crackers?"),
    QStringLiteral("Squawk! Fair winds."),
    QStringLiteral("I see you."),
    QStringLiteral("The deck is mine."),
    QStringLiteral("Hop is hop. Talk is talk."),
    QStringLiteral("Zzz— wait, I'm awake."),
    QStringLiteral("Polly wants nothing. Boris wants fun."),
    QStringLiteral("Aye aye!"),
    QStringLiteral("Land ho — or just lunch?"),
    QStringLiteral("Feathers first, questions later."),
    QStringLiteral("Keep the ship steady."),
};

QString clipShortLine(QString text, int maxWords = 12)
{
    text = text.simplified();
    if (text.isEmpty())
        return {};
    // Strip wrapping quotes some models add.
    if ((text.startsWith(QLatin1Char('"')) && text.endsWith(QLatin1Char('"')))
        || (text.startsWith(QLatin1Char('\'')) && text.endsWith(QLatin1Char('\'')))) {
        text = text.mid(1, text.size() - 2).trimmed();
    }
    const QStringList words = text.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
    if (words.isEmpty())
        return {};
    if (words.size() <= maxWords)
        return words.join(QLatin1Char(' '));
    return words.mid(0, maxWords).join(QLatin1Char(' '));
}
}

Personality::Personality(QObject *parent) : QObject(parent)
{
    m_network = new QNetworkAccessManager(this);
}

void Personality::setApiKey(const QString &key)
{
    m_apiKey = key.trimmed();
}

void Personality::setApiUrl(const QString &url)
{
    const QString trimmed = url.trimmed();
    m_apiUrl = trimmed.isEmpty() ? QStringLiteral("https://api.openai.com/v1/chat/completions") : trimmed;
}

QString Personality::pickLocalLine() const
{
    if (kLines.isEmpty())
        return QStringLiteral("Aye!");
    const int index = QRandomGenerator::global()->bounded(kLines.size());
    return kLines.at(index);
}

void Personality::clearLine()
{
    setLastLine({});
}

void Personality::setLastLine(const QString &line)
{
    if (m_lastLine == line)
        return;
    m_lastLine = line;
    emit lastLineChanged();
    if (!line.isEmpty())
        emit lineReady(line);
}

void Personality::setBusy(bool busy)
{
    if (m_busy == busy)
        return;
    m_busy = busy;
    emit busyChanged();
}

void Personality::replyTo(const QString &heard)
{
    const QString local = pickLocalLine();
    setLastLine(local);
    if (m_apiKey.isEmpty())
        return;
    requestRemote(heard, local);
}

void Personality::requestRemote(const QString &heard, const QString &fallback)
{
    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }

    QJsonObject messageSystem;
    messageSystem.insert(QStringLiteral("role"), QStringLiteral("system"));
    messageSystem.insert(QStringLiteral("content"),
                         QStringLiteral("You are Boris, a cheeky pirate parrot desktop pet. "
                                        "Reply in one short sentence, max 12 words. No emoji. "
                                        "Stay in character. Never mention Cursor or brands."));

    QJsonObject messageUser;
    messageUser.insert(QStringLiteral("role"), QStringLiteral("user"));
    const QString prompt = heard.trimmed().isEmpty()
                               ? QStringLiteral("Someone just got your attention. Greet them briefly.")
                               : heard.trimmed();
    messageUser.insert(QStringLiteral("content"), prompt);

    QJsonObject body;
    body.insert(QStringLiteral("model"), QStringLiteral("gpt-4o-mini"));
    body.insert(QStringLiteral("temperature"), 0.8);
    body.insert(QStringLiteral("max_tokens"), 40);
    body.insert(QStringLiteral("messages"), QJsonArray{messageSystem, messageUser});

    QNetworkRequest request{QUrl(m_apiUrl)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Authorization", QByteArray("Bearer ") + m_apiKey.toUtf8());
    request.setTransferTimeout(4000);

    setBusy(true);
    m_reply = m_network->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(m_reply, &QNetworkReply::finished, this, [this, fallback] {
        QNetworkReply *reply = m_reply;
        m_reply = nullptr;
        setBusy(false);
        if (!reply)
            return;
        reply->deleteLater();
        // Keep the local canned line on any network / parse failure.
        if (reply->error() != QNetworkReply::NoError)
            return;
        const QJsonObject root = QJsonDocument::fromJson(reply->readAll()).object();
        const QJsonArray choices = root.value(QStringLiteral("choices")).toArray();
        if (choices.isEmpty())
            return;
        const QString content =
            choices.at(0).toObject().value(QStringLiteral("message")).toObject().value(QStringLiteral("content")).toString();
        const QString line = clipShortLine(content);
        if (line.isEmpty() || line == fallback)
            return;
        setLastLine(line);
    });
}
