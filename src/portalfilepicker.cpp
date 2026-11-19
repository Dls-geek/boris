#include "portalfilepicker.h"

#include <QCoreApplication>
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMetaType>
#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDir>
#include <QFileInfo>
#include <QRandomGenerator>
#include <QStandardPaths>

namespace {
struct PortalFilterRule {
    uint type;
    QString pattern;
};

using PortalFilterRules = QList<PortalFilterRule>;

struct PortalFileFilter {
    QString name;
    PortalFilterRules rules;
};

using PortalFileFilters = QList<PortalFileFilter>;

QDBusArgument &operator<<(QDBusArgument &argument, const PortalFilterRule &rule)
{
    argument.beginStructure();
    argument << rule.type << rule.pattern;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, PortalFilterRule &rule)
{
    argument.beginStructure();
    argument >> rule.type >> rule.pattern;
    argument.endStructure();
    return argument;
}

QDBusArgument &operator<<(QDBusArgument &argument, const PortalFileFilter &filter)
{
    argument.beginStructure();
    argument << filter.name << filter.rules;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, PortalFileFilter &filter)
{
    argument.beginStructure();
    argument >> filter.name >> filter.rules;
    argument.endStructure();
    return argument;
}

void registerPortalFilterTypes()
{
    static const bool registered = [] {
        qDBusRegisterMetaType<PortalFilterRule>();
        qDBusRegisterMetaType<PortalFilterRules>();
        qDBusRegisterMetaType<PortalFileFilter>();
        qDBusRegisterMetaType<PortalFileFilters>();
        return true;
    }();
    Q_UNUSED(registered);
}

PortalFileFilter pictureFilter()
{
    return {
        QCoreApplication::translate("PortalFilePicker", "Pictures and GIFs"),
        {
            {1, QStringLiteral("image/*")},
            {0, QStringLiteral("*.png")},
            {0, QStringLiteral("*.jpg")},
            {0, QStringLiteral("*.jpeg")},
            {0, QStringLiteral("*.webp")},
            {0, QStringLiteral("*.svg")},
            {0, QStringLiteral("*.gif")},
        },
    };
}

PortalFileFilter gifFilter()
{
    return {
        QCoreApplication::translate("PortalFilePicker", "GIF"),
        {
            {0, QStringLiteral("*.gif")},
            {1, QStringLiteral("image/gif")},
        },
    };
}

QByteArray portalPathBytes(const QString &path)
{
    QByteArray bytes = path.toUtf8();
    bytes.append('\0');
    return bytes;
}

QString pictureFolder()
{
    const QString pictures = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    return QDir(pictures).exists() ? pictures : QDir::homePath();
}
}

PortalFilePicker::PortalFilePicker(QObject *parent) : FilePicker(parent)
{
    registerPortalFilterTypes();
}

void PortalFilePicker::openImage()
{
    QVariantMap options;
    options.insert(QStringLiteral("accept_label"), tr("Open"));
    options.insert(QStringLiteral("modal"), true);
    options.insert(QStringLiteral("multiple"), false);
    options.insert(QStringLiteral("current_folder"), portalPathBytes(pictureFolder()));
    const PortalFileFilter pictures = pictureFilter();
    const PortalFileFilter gifs = gifFilter();
    options.insert(QStringLiteral("filters"),
                   QVariant::fromValue(PortalFileFilters{
                       pictures,
                       gifs,
                       {tr("All files"), {{0, QStringLiteral("*")}}},
                   }));
    options.insert(QStringLiteral("current_filter"), QVariant::fromValue(pictures));
    requestFile(tr("Choose a picture or GIF"), options, Action::Image);
}

void PortalFilePicker::openDirectory()
{
    QVariantMap options;
    options.insert(QStringLiteral("accept_label"), tr("Open"));
    options.insert(QStringLiteral("modal"), true);
    options.insert(QStringLiteral("multiple"), false);
    options.insert(QStringLiteral("directory"), true);
    options.insert(QStringLiteral("current_folder"), portalPathBytes(pictureFolder()));
    requestFile(tr("Choose a frames folder"), options, Action::Directory);
}

bool PortalFilePicker::connectToRequestPath(const QString &path)
{
    m_pendingPath = path;
    return QDBusConnection::sessionBus().connect(
        QStringLiteral("org.freedesktop.portal.Desktop"), m_pendingPath,
        QStringLiteral("org.freedesktop.portal.Request"), QStringLiteral("Response"), this,
        SLOT(handleResponse(uint,QVariantMap)));
}

void PortalFilePicker::closePendingRequest()
{
    if (m_pendingPath.isEmpty())
        return;
    QDBusInterface request(QStringLiteral("org.freedesktop.portal.Desktop"), m_pendingPath,
                           QStringLiteral("org.freedesktop.portal.Request"), QDBusConnection::sessionBus());
    request.asyncCall(QStringLiteral("Close"));
}

void PortalFilePicker::requestFile(const QString &title, const QVariantMap &initialOptions, Action action)
{
    // A request whose Response never arrives must not wedge every later dialog.
    if (m_pendingAction != Action::None) {
        closePendingRequest();
        clearPending();
    }

    QDBusConnection bus = QDBusConnection::sessionBus();
    QDBusInterface portal(QStringLiteral("org.freedesktop.portal.Desktop"),
                          QStringLiteral("/org/freedesktop/portal/desktop"),
                          QStringLiteral("org.freedesktop.portal.FileChooser"), bus);
    if (!portal.isValid()) {
        emit failed(tr("The XDG desktop portal file chooser is not available."));
        return;
    }

    // Subscribe before the call, at the path the portal derives from handle_token,
    // so a fast response cannot arrive before the match rule exists.
    QVariantMap options = initialOptions;
    const QString token = QStringLiteral("boris_%1").arg(QRandomGenerator::global()->generate());
    options.insert(QStringLiteral("handle_token"), token);
    QString sender = bus.baseService().mid(1);
    sender.replace(QLatin1Char('.'), QLatin1Char('_'));
    const QString predictedPath =
        QStringLiteral("/org/freedesktop/portal/desktop/request/%1/%2").arg(sender, token);

    m_pendingAction = action;
    if (!connectToRequestPath(predictedPath)) {
        clearPending();
        emit failed(tr("Could not listen for the portal file picker response."));
        return;
    }

    const int generation = ++m_pendingGeneration;
    auto *watcher = new QDBusPendingCallWatcher(portal.asyncCall(QStringLiteral("OpenFile"), QString(), title, options),
                                                this);
    m_pendingWatcher = watcher;
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher, generation] {
        QDBusPendingReply<QDBusObjectPath> reply = *watcher;
        watcher->deleteLater();
        if (m_pendingWatcher == watcher)
            m_pendingWatcher = nullptr;
        if (generation != m_pendingGeneration || m_pendingAction == Action::None)
            return;
        if (reply.isError()) {
            clearPending();
            emit failed(tr("The portal file picker failed: %1").arg(reply.error().message()));
            return;
        }

        const QString actualPath = reply.value().path();
        if (actualPath != m_pendingPath) {
            QDBusConnection::sessionBus().disconnect(
                QStringLiteral("org.freedesktop.portal.Desktop"), m_pendingPath,
                QStringLiteral("org.freedesktop.portal.Request"), QStringLiteral("Response"), this,
                SLOT(handleResponse(uint,QVariantMap)));
            if (!connectToRequestPath(actualPath)) {
                clearPending();
                emit failed(tr("Could not listen for the portal file picker response."));
            }
        }
    });
}

void PortalFilePicker::handleResponse(uint response, const QVariantMap &results)
{
    const Action action = m_pendingAction;
    clearPending();
    if (response != 0)
        return;

    const QStringList uris = results.value(QStringLiteral("uris")).toStringList();
    if (uris.isEmpty())
        return;
    const QUrl url(uris.first());
    if (action == Action::Image)
        emit imageSelected(url);
    else if (action == Action::Directory)
        emit directorySelected(url);
}

void PortalFilePicker::clearPending()
{
    if (m_pendingWatcher) {
        m_pendingWatcher->deleteLater();
        m_pendingWatcher = nullptr;
    }
    if (!m_pendingPath.isEmpty()) {
        QDBusConnection::sessionBus().disconnect(
            QStringLiteral("org.freedesktop.portal.Desktop"), m_pendingPath,
            QStringLiteral("org.freedesktop.portal.Request"), QStringLiteral("Response"), this,
            SLOT(handleResponse(uint,QVariantMap)));
    }
    m_pendingPath.clear();
    m_pendingAction = Action::None;
}
