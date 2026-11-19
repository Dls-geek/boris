#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QVector>
#include <QTimer>

class QAudioSource;
class QIODevice;

// Local mic wake-word: capture with QAudioSource, energy VAD, then match the
// configured name against an enrolled acoustic template (no cloud STT).
class WakeWord : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(QString phrase READ phrase WRITE setPhrase NOTIFY phraseChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(qreal level READ level NOTIFY levelChanged)
    Q_PROPERTY(bool hasTemplate READ hasTemplate NOTIFY hasTemplateChanged)
    Q_PROPERTY(bool teaching READ teaching NOTIFY statusChanged)

public:
    explicit WakeWord(const QString &storageDirectory = {}, QObject *parent = nullptr);
    ~WakeWord() override;

    bool enabled() const { return m_enabled; }
    bool active() const { return m_enabled; }
    void setEnabled(bool enabled);

    QString phrase() const { return m_phrase; }
    void setPhrase(const QString &phrase);

    QString status() const { return m_status; }
    qreal level() const { return m_level; }
    bool hasTemplate() const { return !m_template.isEmpty(); }
    bool teaching() const { return m_teaching; }

    Q_INVOKABLE void teach();
    Q_INVOKABLE void clearTemplate();
    // Test helper / typed path: if text contains the wake phrase, emit heard.
    Q_INVOKABLE bool submitTranscript(const QString &text);

signals:
    void enabledChanged();
    void phraseChanged();
    void statusChanged();
    void levelChanged();
    void hasTemplateChanged();
    void heard(const QString &phrase);

private:
    struct Frame {
        float energy = 0;
        float zcr = 0;
        float bands[4] = {0, 0, 0, 0};
    };

    void openMic();
    void closeMic();
    void onReadyRead();
    void processPcm(const QByteArray &chunk);
    void finishUtterance();
    void setStatus(const QString &status);
    void setLevel(qreal level);
    void emitHeard();
    bool loadTemplate();
    bool saveTemplate() const;
    QString templatePath() const;
    static QVector<Frame> extractFeatures(const QVector<qint16> &samples, int sampleRate);
    static float matchScore(const QVector<Frame> &a, const QVector<Frame> &b);
    static int estimateSyllables(const QString &phrase);
    bool looksLikeNameCall(const QVector<Frame> &frames) const;
    QString tryExternalTranscript(const QVector<qint16> &samples) const;

    QString m_directory;
    QString m_phrase = QStringLiteral("boris");
    QString m_status = QStringLiteral("off");
    QAudioSource *m_source = nullptr;
    QIODevice *m_device = nullptr;
    QVector<qint16> m_utterance;
    QVector<Frame> m_template;
    QTimer m_heardClear;
    qreal m_level = 0;
    int m_sampleRate = 16000;
    int m_silenceMs = 0;
    int m_speechMs = 0;
    int m_cooldownMs = 0;
    bool m_enabled = false;
    bool m_inSpeech = false;
    bool m_teaching = false;
};
