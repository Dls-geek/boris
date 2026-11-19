#include "wakeword.h"

#include <QAudioDevice>
#include <QAudioFormat>
#include <QAudioSource>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QMediaDevices>
#include <QStandardPaths>
#include <QtMath>

#include <algorithm>
#include <cstring>

namespace {
constexpr int kCooldownMs = 2500;
constexpr float kEnergyGate = 0.018f;
constexpr int kFrameSamples = 512;

QString defaultWakeDirectory()
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return base.isEmpty() ? QDir::homePath() + QStringLiteral("/.local/share/boris") : base;
}
}

WakeWord::WakeWord(const QString &storageDirectory, QObject *parent)
    : QObject(parent), m_directory(storageDirectory.isEmpty() ? defaultWakeDirectory() : storageDirectory)
{
    QDir().mkpath(m_directory);
    m_heardClear.setSingleShot(true);
    connect(&m_heardClear, &QTimer::timeout, this, [this] {
        if (m_enabled)
            setStatus(tr("Listening for %1…").arg(m_phrase));
    });
    loadTemplate();
    setStatus(tr("Wake word off"));
}

WakeWord::~WakeWord()
{
    closeMic();
}

void WakeWord::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;
    m_enabled = enabled;
    emit enabledChanged();
    if (m_enabled) {
        openMic();
        if (m_source)
            setStatus(tr("Listening for %1…").arg(m_phrase));
        else
            setStatus(tr("No microphone"));
    } else {
        closeMic();
        m_teaching = false;
        setStatus(tr("Wake word off"));
    }
}

void WakeWord::setPhrase(const QString &phrase)
{
    QString next = phrase.trimmed();
    if (next.isEmpty())
        next = QStringLiteral("boris");
    if (next.size() > 40)
        next = next.left(40).trimmed();
    if (m_phrase == next)
        return;
    m_phrase = next;
    emit phraseChanged();
    if (m_enabled && !m_teaching)
        setStatus(tr("Listening for %1…").arg(m_phrase));
}

void WakeWord::teach()
{
    if (!m_enabled) {
        setEnabled(true);
        if (!m_enabled)
            return;
    }
    m_teaching = true;
    m_utterance.clear();
    m_inSpeech = false;
    m_speechMs = 0;
    m_silenceMs = 0;
    setStatus(tr("Say \"%1\" clearly…").arg(m_phrase));
    emit statusChanged();
}

void WakeWord::clearTemplate()
{
    const bool had = !m_template.isEmpty();
    m_template.clear();
    QFile::remove(templatePath());
    if (had)
        emit hasTemplateChanged();
    if (m_enabled)
        setStatus(tr("Listening for %1…").arg(m_phrase));
}

bool WakeWord::submitTranscript(const QString &text)
{
    if (!text.contains(m_phrase, Qt::CaseInsensitive))
        return false;
    emitHeard();
    return true;
}

void WakeWord::setStatus(const QString &status)
{
    if (m_status == status)
        return;
    m_status = status;
    emit statusChanged();
}

void WakeWord::setLevel(qreal level)
{
    level = std::clamp(level, 0.0, 1.0);
    if (qFuzzyCompare(m_level, level))
        return;
    m_level = level;
    emit levelChanged();
}

void WakeWord::emitHeard()
{
    if (m_cooldownMs > 0)
        return;
    m_cooldownMs = kCooldownMs;
    setStatus(tr("Heard you"));
    m_heardClear.start(1800);
    emit heard(m_phrase);
}

QString WakeWord::templatePath() const
{
    return m_directory + QStringLiteral("/wake-template.bin");
}

bool WakeWord::loadTemplate()
{
    QFile file(templatePath());
    if (!file.open(QIODevice::ReadOnly))
        return false;
    const QByteArray bytes = file.readAll();
    if (bytes.size() < int(sizeof(Frame)))
        return false;
    const int count = bytes.size() / int(sizeof(Frame));
    m_template.resize(count);
    std::memcpy(m_template.data(), bytes.constData(), count * sizeof(Frame));
    emit hasTemplateChanged();
    return true;
}

bool WakeWord::saveTemplate() const
{
    if (m_template.isEmpty())
        return false;
    QFile file(templatePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    const QByteArray bytes(reinterpret_cast<const char *>(m_template.constData()),
                           m_template.size() * int(sizeof(Frame)));
    return file.write(bytes) == bytes.size();
}

void WakeWord::openMic()
{
    closeMic();
    const QAudioDevice info = QMediaDevices::defaultAudioInput();
    if (info.isNull())
        return;

    QAudioFormat format;
    format.setSampleRate(16000);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);
    if (!info.isFormatSupported(format))
        format = info.preferredFormat();
    m_sampleRate = format.sampleRate() > 0 ? format.sampleRate() : 16000;

    m_source = new QAudioSource(info, format, this);
    m_device = m_source->start();
    if (!m_device) {
        delete m_source;
        m_source = nullptr;
        return;
    }
    connect(m_device, &QIODevice::readyRead, this, &WakeWord::onReadyRead);
}

void WakeWord::closeMic()
{
    if (m_device) {
        disconnect(m_device, nullptr, this, nullptr);
        m_device = nullptr;
    }
    if (m_source) {
        m_source->stop();
        m_source->deleteLater();
        m_source = nullptr;
    }
    m_utterance.clear();
    m_inSpeech = false;
    setLevel(0);
}

void WakeWord::onReadyRead()
{
    if (!m_device || !m_enabled)
        return;
    processPcm(m_device->readAll());
}

void WakeWord::processPcm(const QByteArray &chunk)
{
    if (chunk.size() < 2)
        return;
    const qsizetype count = chunk.size() / 2;
    const auto *samples = reinterpret_cast<const qint16 *>(chunk.constData());

    qreal sum = 0;
    for (qsizetype i = 0; i < count; ++i) {
        const qreal s = samples[i] / 32768.0;
        sum += s * s;
        m_utterance.append(samples[i]);
    }
    const qreal rms = qSqrt(sum / qreal(count));
    setLevel(rms);

    const int ms = int((count * 1000.0) / qMax(1, m_sampleRate));
    if (m_cooldownMs > 0)
        m_cooldownMs = qMax(0, m_cooldownMs - ms);

    if (rms >= kEnergyGate) {
        m_inSpeech = true;
        m_speechMs += ms;
        m_silenceMs = 0;
    } else if (m_inSpeech) {
        m_silenceMs += ms;
        if (m_silenceMs >= 350)
            finishUtterance();
    }

    // Cap buffer (~3s) so memory stays bounded.
    const int maxSamples = m_sampleRate * 3;
    if (m_utterance.size() > maxSamples)
        m_utterance = m_utterance.mid(m_utterance.size() - maxSamples);
}

void WakeWord::finishUtterance()
{
    m_inSpeech = false;
    m_silenceMs = 0;
    if (m_speechMs < 180 || m_utterance.isEmpty()) {
        m_utterance.clear();
        m_speechMs = 0;
        return;
    }

    const QVector<Frame> frames = extractFeatures(m_utterance, m_sampleRate);
    m_utterance.clear();
    const int spokenMs = m_speechMs;
    m_speechMs = 0;
    if (frames.isEmpty())
        return;

    if (m_teaching) {
        m_template = frames;
        m_teaching = false;
        saveTemplate();
        emit hasTemplateChanged();
        setStatus(tr("Name saved — listening"));
        m_heardClear.start(1500);
        return;
    }

    if (!m_template.isEmpty()) {
        const float score = matchScore(frames, m_template);
        if (score >= 0.72f && looksLikeNameCall(frames)) {
            emitHeard();
            return;
        }
    }

    // Optional external STT hook (empty unless a helper is installed).
    const QString transcript = tryExternalTranscript({});
    if (!transcript.isEmpty() && transcript.contains(m_phrase, Qt::CaseInsensitive)) {
        emitHeard();
        return;
    }

    // Without a template, accept short name-like bursts so mic wake still works.
    if (m_template.isEmpty() && looksLikeNameCall(frames) && spokenMs < 1200)
        emitHeard();
}

bool WakeWord::looksLikeNameCall(const QVector<Frame> &frames) const
{
    if (frames.size() < 3)
        return false;
    const int expected = estimateSyllables(m_phrase);
    // Rough duration / energy envelope check — short call, not a long sentence.
    return frames.size() >= expected && frames.size() <= expected * 8 + 6;
}

int WakeWord::estimateSyllables(const QString &phrase)
{
    int count = 0;
    bool inVowel = false;
    for (QChar ch : phrase.toLower()) {
        const bool vowel = QStringLiteral("aeiouy").contains(ch);
        if (vowel && !inVowel)
            ++count;
        inVowel = vowel;
    }
    return qMax(1, count);
}

QVector<WakeWord::Frame> WakeWord::extractFeatures(const QVector<qint16> &samples, int sampleRate)
{
    QVector<Frame> frames;
    if (samples.isEmpty() || sampleRate <= 0)
        return frames;
    for (int start = 0; start + kFrameSamples <= samples.size(); start += kFrameSamples / 2) {
        Frame frame;
        float energy = 0;
        int crossings = 0;
        float bandEnergy[4] = {0, 0, 0, 0};
        qint16 prev = samples.at(start);
        for (int i = 0; i < kFrameSamples; ++i) {
            const qint16 sample = samples.at(start + i);
            const float n = sample / 32768.0f;
            energy += n * n;
            if ((prev >= 0 && sample < 0) || (prev < 0 && sample >= 0))
                ++crossings;
            const int band = std::clamp(i * 4 / kFrameSamples, 0, 3);
            bandEnergy[band] += n * n;
            prev = sample;
        }
        frame.energy = energy / float(kFrameSamples);
        frame.zcr = float(crossings) / float(kFrameSamples);
        for (int b = 0; b < 4; ++b)
            frame.bands[b] = bandEnergy[b] / float(kFrameSamples);
        frames.append(frame);
    }
    return frames;
}

float WakeWord::matchScore(const QVector<Frame> &a, const QVector<Frame> &b)
{
    if (a.isEmpty() || b.isEmpty())
        return 0;
    const int n = qMin(a.size(), b.size());
    float score = 0;
    for (int i = 0; i < n; ++i) {
        const Frame &fa = a.at(i * a.size() / n);
        const Frame &fb = b.at(i * b.size() / n);
        const float de = qAbs(fa.energy - fb.energy);
        const float dz = qAbs(fa.zcr - fb.zcr);
        float db = 0;
        for (int k = 0; k < 4; ++k)
            db += qAbs(fa.bands[k] - fb.bands[k]);
        const float local = 1.0f - std::clamp(de * 8.0f + dz * 4.0f + db * 6.0f, 0.0f, 1.0f);
        score += local;
    }
    return score / float(n);
}

QString WakeWord::tryExternalTranscript(const QVector<qint16> &samples) const
{
    Q_UNUSED(samples);
    // No cloud. Optional local helpers can be wired here later without changing Backend.
    return {};
}
