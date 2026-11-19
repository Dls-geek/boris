#include "avatar.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QVariantMap>

namespace {
const QStringList kExtensions{QStringLiteral("png"), QStringLiteral("jpg"), QStringLiteral("jpeg"),
                              QStringLiteral("webp"), QStringLiteral("svg"), QStringLiteral("gif")};

QString defaultDirectory()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/avatar");
}

bool allowedExtension(const QString &path)
{
    return kExtensions.contains(QFileInfo(path).suffix().toLower());
}

bool safeRelative(const QString &relative)
{
    return !relative.isEmpty() && !QFileInfo(relative).isAbsolute() && !relative.contains(QStringLiteral(".."));
}

bool copyAtomically(const QString &from, const QString &to)
{
    if (QFileInfo(from).absoluteFilePath() == QFileInfo(to).absoluteFilePath())
        return true;
    const QString temp = to + QStringLiteral(".tmp");
    QFile::remove(temp);
    if (!QFile::copy(from, temp))
        return false;
    QFile::remove(to);
    if (QFile::rename(temp, to))
        return true;
    QFile::remove(temp);
    return false;
}

QString findFrame(const QDir &dir, const QStringList &stems)
{
    const QStringList entries = dir.entryList(QDir::Files);
    for (const QString &entry : entries) {
        const QFileInfo info(entry);
        if (!kExtensions.contains(info.suffix().toLower()))
            continue;
        if (stems.contains(info.completeBaseName().toLower()))
            return entry;
    }
    return {};
}
}

Avatar::Avatar(const QString &directory, QObject *parent)
    : QObject(parent), m_directory(directory.isEmpty() ? defaultDirectory() : directory)
{
    QDir().mkpath(m_directory);
    QDir().mkpath(m_directory + QStringLiteral("/library"));
    loadLibrary();
    load();
}

QUrl Avatar::source() const
{
    QString relative;
    if (m_kind == QLatin1String("picture"))
        relative = m_file;
    else if (m_kind == QLatin1String("frames")) {
        relative = m_idle;
        // talk / excited use the talk frame; hop stays on idle.
        // "react" kept as a legacy alias for older tests / callers.
        if ((m_pose == QLatin1String("talk") || m_pose == QLatin1String("excited")
             || m_pose == QLatin1String("react"))
            && !m_talk.isEmpty())
            relative = m_talk;
        else if (m_pose == QLatin1String("walk"))
            relative = (m_frame == 0 ? m_walkA : m_walkB);
        if (relative.isEmpty())
            relative = m_idle;
    }
    if (!safeRelative(relative))
        return {};
    return QUrl::fromLocalFile(m_directory + QLatin1Char('/') + relative);
}

bool Avatar::animated() const
{
    if (m_kind != QLatin1String("picture"))
        return false;
    return QFileInfo(source().toLocalFile()).suffix().compare(QLatin1String("gif"), Qt::CaseInsensitive) == 0;
}

QString Avatar::displayName() const
{
    if (m_activeId == QLatin1String("parrot") || m_kind == QLatin1String("parrot"))
        return tr("Parrot");
    if (m_kind == QLatin1String("frames"))
        return tr("Frames");
    for (const Entry &entry : m_library) {
        if (entry.id == m_activeId)
            return entry.name;
    }
    if (m_kind == QLatin1String("picture"))
        return tr("Custom picture");
    return tr("Parrot");
}

QVariantList Avatar::catalog() const
{
    QVariantList list;
    QVariantMap parrot;
    parrot.insert(QStringLiteral("id"), QStringLiteral("parrot"));
    parrot.insert(QStringLiteral("name"), tr("Parrot"));
    parrot.insert(QStringLiteral("kind"), QStringLiteral("parrot"));
    parrot.insert(QStringLiteral("preview"), QUrl());
    parrot.insert(QStringLiteral("animated"), false);
    parrot.insert(QStringLiteral("active"), m_activeId == QLatin1String("parrot"));
    list.append(parrot);

    for (const Entry &entry : m_library) {
        const QUrl preview = QUrl::fromLocalFile(m_directory + QLatin1Char('/') + entry.file);
        QVariantMap item;
        item.insert(QStringLiteral("id"), entry.id);
        item.insert(QStringLiteral("name"), entry.name);
        item.insert(QStringLiteral("kind"), QStringLiteral("picture"));
        item.insert(QStringLiteral("preview"), preview);
        item.insert(QStringLiteral("animated"),
                    QFileInfo(entry.file).suffix().compare(QLatin1String("gif"), Qt::CaseInsensitive) == 0);
        item.insert(QStringLiteral("active"), entry.id == m_activeId);
        list.append(item);
    }
    return list;
}

void Avatar::setPose(const QString &pose, int frame)
{
    if (m_pose == pose && m_frame == frame)
        return;
    const QUrl before = source();
    m_pose = pose;
    m_frame = frame;
    if (source() != before)
        emit changed();
}

QString Avatar::fail(const QString &message)
{
    m_error = message;
    emit changed();
    return m_error;
}

QString Avatar::sanitizeName(const QString &name) const
{
    QString cleaned = name.simplified();
    if (cleaned.isEmpty())
        return tr("Untitled");
    if (cleaned.size() > 40)
        cleaned = cleaned.left(40).trimmed();
    return cleaned;
}

QString Avatar::makeId() const
{
    return QStringLiteral("%1-%2")
        .arg(QDateTime::currentMSecsSinceEpoch())
        .arg(QRandomGenerator::global()->bounded(100000, 999999));
}

QString Avatar::importPicture(const QString &path)
{
    return addNamedPicture(path, tr("Custom picture"));
}

QString Avatar::addNamedPicture(const QString &path, const QString &name)
{
    if (!QFileInfo::exists(path) || !QFileInfo(path).isFile())
        return fail(tr("The picture could not be read."));
    if (!allowedExtension(path))
        return fail(tr("That file type cannot be an avatar."));
    if (!QDir().mkpath(m_directory + QStringLiteral("/library")))
        return fail(tr("Could not save the avatar."));

    const QString id = makeId();
    const QString ext = QFileInfo(path).suffix().toLower();
    const QString relative = QStringLiteral("library/%1.%2").arg(id, ext);
    if (!copyAtomically(path, m_directory + QLatin1Char('/') + relative))
        return fail(tr("Could not save the avatar."));

    Entry entry;
    entry.id = id;
    entry.name = sanitizeName(name);
    entry.file = relative;
    m_library.append(entry);
    if (!saveLibrary())
        return fail(tr("Could not save the avatar."));

    m_kind = QStringLiteral("picture");
    m_file = relative;
    m_activeId = id;
    m_idle.clear();
    m_walkA.clear();
    m_walkB.clear();
    m_talk.clear();
    m_error.clear();
    if (!save())
        return fail(tr("Could not save the avatar."));
    emit changed();
    return {};
}

QString Avatar::copyFrame(const QString &sourcePath, const QString &stem)
{
    const QString ext = QFileInfo(sourcePath).suffix().toLower();
    const QString relative = QStringLiteral("frames/%1.%2").arg(stem, ext);
    const QString absolute = m_directory + QLatin1Char('/') + relative;
    if (!QDir().mkpath(QFileInfo(absolute).absolutePath()))
        return {};
    if (!copyAtomically(sourcePath, absolute))
        return {};
    return relative;
}

QString Avatar::importFrames(const QString &directory)
{
    const QDir dir(directory);
    if (!dir.exists())
        return fail(tr("Could not open that folder."));

    const QString idleFile = findFrame(dir, {QStringLiteral("idle")});
    if (idleFile.isEmpty())
        return fail(tr("That folder needs an idle.png."));

    const QString walkAFile = findFrame(dir, {QStringLiteral("walk-a"), QStringLiteral("walk_a"),
                                              QStringLiteral("walk1"), QStringLiteral("walka")});
    const QString walkBFile = findFrame(dir, {QStringLiteral("walk-b"), QStringLiteral("walk_b"),
                                              QStringLiteral("walk2"), QStringLiteral("walkb")});
    const QString talkFile = findFrame(dir, {QStringLiteral("talk"), QStringLiteral("speak")});

    const QString idle = copyFrame(dir.filePath(idleFile), QStringLiteral("idle"));
    if (idle.isEmpty())
        return fail(tr("Could not save the avatar."));
    const QString walkA = walkAFile.isEmpty() ? idle : copyFrame(dir.filePath(walkAFile), QStringLiteral("walk-a"));
    const QString walkB = walkBFile.isEmpty() ? walkA : copyFrame(dir.filePath(walkBFile), QStringLiteral("walk-b"));
    QString talk;
    if (!talkFile.isEmpty())
        talk = copyFrame(dir.filePath(talkFile), QStringLiteral("talk"));
    if (walkA.isEmpty() || walkB.isEmpty() || (!talkFile.isEmpty() && talk.isEmpty()))
        return fail(tr("Could not save the avatar."));

    m_kind = QStringLiteral("frames");
    m_file.clear();
    m_idle = idle;
    m_walkA = walkA;
    m_walkB = walkB;
    m_talk = talk;
    m_activeId = QStringLiteral("frames");
    m_error.clear();
    if (!save())
        return fail(tr("Could not save the avatar."));
    emit changed();
    return {};
}

void Avatar::useParrot()
{
    selectAvatar(QStringLiteral("parrot"));
}

bool Avatar::selectAvatar(const QString &id)
{
    if (id == QLatin1String("parrot")) {
        m_kind = QStringLiteral("parrot");
        m_file.clear();
        m_idle.clear();
        m_walkA.clear();
        m_walkB.clear();
        m_talk.clear();
        m_activeId = QStringLiteral("parrot");
        m_error.clear();
        save();
        emit changed();
        return true;
    }

    for (const Entry &entry : m_library) {
        if (entry.id != id)
            continue;
        if (!safeRelative(entry.file)
            || !QFileInfo::exists(m_directory + QLatin1Char('/') + entry.file)) {
            fail(tr("The picture could not be read."));
            return false;
        }
        m_kind = QStringLiteral("picture");
        m_file = entry.file;
        m_idle.clear();
        m_walkA.clear();
        m_walkB.clear();
        m_talk.clear();
        m_activeId = entry.id;
        m_error.clear();
        save();
        emit changed();
        return true;
    }
    return false;
}

bool Avatar::removeAvatar(const QString &id)
{
    if (id == QLatin1String("parrot") || id == QLatin1String("frames"))
        return false;

    int index = -1;
    for (int i = 0; i < m_library.size(); ++i) {
        if (m_library.at(i).id == id) {
            index = i;
            break;
        }
    }
    if (index < 0)
        return false;

    const QString file = m_library.at(index).file;
    m_library.removeAt(index);
    if (safeRelative(file))
        QFile::remove(m_directory + QLatin1Char('/') + file);
    saveLibrary();
    if (m_activeId == id)
        useParrot();
    else
        emit changed();
    return true;
}

bool Avatar::saveLibrary() const
{
    QJsonArray array;
    for (const Entry &entry : m_library) {
        QJsonObject object;
        object.insert(QStringLiteral("id"), entry.id);
        object.insert(QStringLiteral("name"), entry.name);
        object.insert(QStringLiteral("file"), entry.file);
        array.append(object);
    }
    if (!QDir().mkpath(m_directory + QStringLiteral("/library")))
        return false;
    const QString path = m_directory + QStringLiteral("/library/index.json");
    const QString temp = path + QStringLiteral(".tmp");
    QFile file(temp);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    const QByteArray bytes = QJsonDocument(array).toJson(QJsonDocument::Compact);
    if (file.write(bytes) != bytes.size())
        return false;
    file.close();
    QFile::remove(path);
    if (QFile::rename(temp, path))
        return true;
    QFile::remove(temp);
    return false;
}

bool Avatar::loadLibrary()
{
    m_library.clear();
    QFile file(m_directory + QStringLiteral("/library/index.json"));
    if (!file.open(QIODevice::ReadOnly))
        return false;
    const QJsonArray array = QJsonDocument::fromJson(file.readAll()).array();
    for (const QJsonValue &value : array) {
        const QJsonObject object = value.toObject();
        Entry entry;
        entry.id = object.value(QStringLiteral("id")).toString();
        entry.name = object.value(QStringLiteral("name")).toString();
        entry.file = object.value(QStringLiteral("file")).toString();
        if (entry.id.isEmpty() || entry.file.isEmpty())
            continue;
        if (!safeRelative(entry.file)
            || !QFileInfo::exists(m_directory + QLatin1Char('/') + entry.file))
            continue;
        if (entry.name.isEmpty())
            entry.name = tr("Untitled");
        m_library.append(entry);
    }
    return true;
}

bool Avatar::save() const
{
    QJsonObject object;
    object.insert(QStringLiteral("kind"), m_kind);
    object.insert(QStringLiteral("activeId"), m_activeId);
    if (m_kind == QLatin1String("picture"))
        object.insert(QStringLiteral("file"), m_file);
    if (m_kind == QLatin1String("frames")) {
        object.insert(QStringLiteral("idle"), m_idle);
        object.insert(QStringLiteral("walkA"), m_walkA);
        object.insert(QStringLiteral("walkB"), m_walkB);
        object.insert(QStringLiteral("talk"), m_talk);
    }

    if (!QDir().mkpath(m_directory))
        return false;
    const QString path = m_directory + QStringLiteral("/avatar.json");
    const QString temp = path + QStringLiteral(".tmp");
    QFile file(temp);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    const QByteArray bytes = QJsonDocument(object).toJson(QJsonDocument::Compact);
    if (file.write(bytes) != bytes.size())
        return false;
    file.close();
    QFile::remove(path);
    if (QFile::rename(temp, path))
        return true;
    QFile::remove(temp);
    return false;
}

void Avatar::load()
{
    QFile file(m_directory + QStringLiteral("/avatar.json"));
    if (!file.open(QIODevice::ReadOnly))
        return;
    const QJsonObject object = QJsonDocument::fromJson(file.readAll()).object();
    const QString kind = object.value(QStringLiteral("kind")).toString();
    m_activeId = object.value(QStringLiteral("activeId")).toString();

    auto exists = [this](const QString &relative) {
        return safeRelative(relative) && QFileInfo::exists(m_directory + QLatin1Char('/') + relative);
    };

    if (kind == QLatin1String("picture")) {
        const QString relative = object.value(QStringLiteral("file")).toString();
        if (!exists(relative)) {
            m_kind = QStringLiteral("parrot");
            m_file.clear();
            m_activeId = QStringLiteral("parrot");
            save();
            m_error = tr("The picture could not be read.");
            return;
        }
        m_kind = kind;
        m_file = relative;
        if (m_activeId.isEmpty()) {
            for (const Entry &entry : m_library) {
                if (entry.file == relative) {
                    m_activeId = entry.id;
                    break;
                }
            }
            if (m_activeId.isEmpty())
                m_activeId = QStringLiteral("custom");
        }
        return;
    }

    if (kind == QLatin1String("frames")) {
        const QString idle = object.value(QStringLiteral("idle")).toString();
        const QString walkA = object.value(QStringLiteral("walkA")).toString();
        const QString walkB = object.value(QStringLiteral("walkB")).toString();
        const QString talk = object.value(QStringLiteral("talk")).toString();
        if (!exists(idle) || !exists(walkA) || !exists(walkB) || (!talk.isEmpty() && !exists(talk))) {
            m_kind = QStringLiteral("parrot");
            m_file.clear();
            m_idle.clear();
            m_walkA.clear();
            m_walkB.clear();
            m_talk.clear();
            m_activeId = QStringLiteral("parrot");
            save();
            m_error = tr("The picture could not be read.");
            return;
        }
        m_kind = kind;
        m_idle = idle;
        m_walkA = walkA;
        m_walkB = walkB;
        m_talk = talk;
        m_activeId = QStringLiteral("frames");
        return;
    }

    m_activeId = QStringLiteral("parrot");
}
