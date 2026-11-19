#pragma once

#include <QObject>
#include <QString>
#include <QUrl>
#include <QVariantList>

// The user's pictures are copied into the avatar store. Originals are never
// changed. kind is "parrot", "picture", or "frames". Named looks live in a
// small library so settings can switch between them.
class Avatar : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString kind READ kind NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)
    Q_PROPERTY(QUrl source READ source NOTIFY changed)
    Q_PROPERTY(bool animated READ animated NOTIFY changed)
    Q_PROPERTY(QString activeId READ activeId NOTIFY changed)
    Q_PROPERTY(QString displayName READ displayName NOTIFY changed)
    Q_PROPERTY(QVariantList catalog READ catalog NOTIFY changed)

public:
    explicit Avatar(const QString &directory = {}, QObject *parent = nullptr);

    QString kind() const { return m_kind; }
    QString error() const { return m_error; }
    QUrl source() const;
    bool animated() const;
    QString activeId() const { return m_activeId; }
    QString displayName() const;
    QVariantList catalog() const;

    void setPose(const QString &pose, int frame);

    QString importPicture(const QString &path);
    QString importFrames(const QString &directory);
    QString addNamedPicture(const QString &path, const QString &name);
    Q_INVOKABLE void useParrot();
    Q_INVOKABLE bool selectAvatar(const QString &id);
    Q_INVOKABLE bool removeAvatar(const QString &id);

signals:
    void changed();

private:
    QString fail(const QString &message);
    bool save() const;
    void load();
    bool loadLibrary();
    bool saveLibrary() const;
    QString copyFrame(const QString &sourcePath, const QString &stem);
    QString sanitizeName(const QString &name) const;
    QString makeId() const;

    struct Entry {
        QString id;
        QString name;
        QString file;
    };

    QString m_directory;
    QString m_kind = QStringLiteral("parrot");
    QString m_error;
    QString m_file;
    QString m_idle;
    QString m_walkA;
    QString m_walkB;
    QString m_talk;
    QString m_pose = QStringLiteral("idle");
    QString m_activeId = QStringLiteral("parrot");
    int m_frame = 0;
    QList<Entry> m_library;
};
