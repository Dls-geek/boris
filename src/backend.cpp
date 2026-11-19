#include "backend.h"

#include <QCursor>
#include <QGuiApplication>
#include <QLocalSocket>
#include <QRegion>
#include <QScreen>
#include <QStandardPaths>
#include <QWindow>

#include <algorithm>

#include <LayerShellQt/Window>

#include "filepicker.h"
#include "portalfilepicker.h"

Backend::Backend(const QString &avatarDirectory, FilePicker *picker, const QString &configDirectory, QObject *parent)
    : QObject(parent), m_avatar(avatarDirectory), m_config(configDirectory),
      m_picker(picker ? picker : new PortalFilePicker(this))
{
    if (!m_avatar.error().isEmpty())
        m_message = m_avatar.error();
    m_pet.setScalePercent(m_config.scalePercent());
    applyAttention();

    connect(&m_pet, &Pet::changed, this, [this] {
        m_avatar.setPose(m_pet.mood(), m_pet.frame());
        syncWindow();
    });
    connect(&m_pet, &Pet::settingsRequested, this, &Backend::openSettings);
    connect(&m_config, &Config::scalePercentChanged, this, [this] {
        m_pet.setScalePercent(m_config.scalePercent());
        emit scalePercentChanged();
    });
    connect(&m_config, &Config::attentionChanged, this, [this] {
        applyAttention();
        emit attentionChanged();
    });
    connect(&m_cursor, &Cursor::moved, this, [this](QPoint position) {
        const QPoint local = position - screenOrigin();
        m_pet.noticePointer(local.x(), local.y());
    });
    connect(m_picker, &FilePicker::imageSelected, this, [this](const QUrl &url) {
        if (m_namedAvatarPick) {
            m_namedAvatarPick = false;
            m_avatarDraft = url;
            emit avatarDraftChanged();
            setMessage({});
            return;
        }
        setMessage(m_avatar.importPicture(url.toLocalFile()));
    });
    connect(m_picker, &FilePicker::directorySelected, this, [this](const QUrl &url) {
        setMessage(m_avatar.importFrames(url.toLocalFile()));
    });
    connect(m_picker, &FilePicker::failed, this, [this](const QString &message) { setMessage(message); });
}

void Backend::attachWindow(QWindow *window)
{
    m_window = window;
    window->setFlags(Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);

    const bool wayland = QGuiApplication::platformName() == QLatin1String("wayland");
    if (wayland) {
        m_layer = LayerShellQt::Window::get(window);
        m_layer->setLayer(LayerShellQt::Window::LayerTop);
        m_layer->setAnchors(LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorLeft)
                            | LayerShellQt::Window::AnchorBottom);
        m_layer->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityNone);
        m_layer->setExclusiveZone(0);
        m_layer->setActivateOnShow(false);
        m_layer->setScope(QStringLiteral("boris"));

        QScreen *screen = QGuiApplication::screenAt(QCursor::pos());
        if (!screen)
            screen = QGuiApplication::primaryScreen();
        if (screen)
            m_layer->setScreen(screen);
        followScreen(screen);
    } else if (!m_pet.hasStage()) {
        QScreen *screen = window->screen() ? window->screen() : QGuiApplication::primaryScreen();
        followScreen(screen);
    }
    syncWindow();
}

void Backend::followScreen(QScreen *screen)
{
    if (!screen)
        return;
    if (m_screenConnection)
        disconnect(m_screenConnection);
    m_screen = screen;
    m_screenConnection = connect(screen, &QScreen::geometryChanged, this, [this](const QRect &rect) {
        m_pet.setStage(rect.width(), rect.height());
    });
    m_pet.setStage(screen->geometry().width(), screen->geometry().height());
    if (m_settingsLayer)
        m_settingsLayer->setScreen(screen);
}

void Backend::reveal()
{
    if (m_revealed)
        return;
    m_revealed = true;
    emit revealedChanged();
    // The compositor can drop the first size it is given, before the surface exists.
    QTimer::singleShot(0, this, &Backend::syncWindow);
    QTimer::singleShot(200, this, &Backend::syncWindow);
}

void Backend::start()
{
    listenForLaunches();
    applyAttention();
    m_clock.start();
    m_lastMs = 0;
    m_timer.setTimerType(Qt::PreciseTimer);
    m_timer.setInterval(16);
    connect(&m_timer, &QTimer::timeout, this, &Backend::onTick);
    m_timer.start();
}

void Backend::applyAttention()
{
    m_pet.setAttention(m_config.followCursor(), m_config.idleAntics(), m_config.sleepWhenIdle());
    m_cursor.setActive(m_config.followCursor());
}

void Backend::setFollowCursor(bool enabled)
{
    m_config.setFollowCursor(enabled);
}

void Backend::setIdleAntics(bool enabled)
{
    m_config.setIdleAntics(enabled);
}

void Backend::setSleepWhenIdle(bool enabled)
{
    m_config.setSleepWhenIdle(enabled);
}

QString Backend::socketPath()
{
    const QString runtime = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    const QString directory = runtime.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::TempLocation) : runtime;
    return directory + QStringLiteral("/boris.sock");
}

void Backend::listenForLaunches()
{
    const QString path = socketPath();
    QLocalServer::removeServer(path);
    m_server.setSocketOptions(QLocalServer::UserAccessOption);
    if (!m_server.listen(path)) {
        qWarning("Boris could not listen for the settings window: %s", qPrintable(m_server.errorString()));
        return;
    }
    connect(&m_server, &QLocalServer::newConnection, this, [this] {
        while (QLocalSocket *socket = m_server.nextPendingConnection()) {
            openSettings();
            socket->disconnectFromServer();
            socket->deleteLater();
        }
    });
}

void Backend::onTick()
{
    const qint64 now = m_clock.elapsed();
    int ms = int(now - m_lastMs);
    m_lastMs = now;
    if (ms < 0)
        ms = 0;
    if (ms > 100)
        ms = 100;

    const QPoint origin = screenOrigin();
    const QPoint cursor = QCursor::pos() - origin;
    if (m_pet.pointerDown())
        m_pet.updatePointer(cursor.x(), cursor.y());
    if (m_settingsDrag && m_screen) {
        const QRect area = m_screen->geometry();
        m_settingsX = std::clamp(cursor.x() - m_settingsGrabDx, 0, std::max(0, area.width() - SettingsWidth));
        m_settingsY = std::clamp(cursor.y() - m_settingsGrabDy, 0, std::max(0, area.height() - SettingsHeight));
        syncSettings();
    }
    m_pet.tick(ms);
}

QPoint Backend::screenOrigin() const
{
    return m_screen ? m_screen->geometry().topLeft() : QPoint();
}

void Backend::syncWindow()
{
    if (!m_window)
        return;

    const int width = m_pet.windowWidth();
    const int height = m_pet.windowHeight();
    if (m_window->width() != width)
        m_window->setWidth(width);
    if (m_window->height() != height)
        m_window->setHeight(height);

    if (m_layer) {
        m_layer->setDesiredSize(QSize(width, height));
        m_layer->setMargins(QMargins(m_pet.windowX(), 0, 0, m_pet.windowY()));
    }

    // The window is exactly his body, so the whole surface is him.
    m_window->setMask(QRegion(0, 0, width, height));
}

void Backend::attachSettings(QWindow *window)
{
    m_settings = window;
    window->setFlags(Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    if (QGuiApplication::platformName() == QLatin1String("wayland")) {
        m_settingsLayer = LayerShellQt::Window::get(window);
        m_settingsLayer->setLayer(LayerShellQt::Window::LayerOverlay);
        m_settingsLayer->setAnchors(LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorLeft)
                                    | LayerShellQt::Window::AnchorTop);
        m_settingsLayer->setExclusiveZone(0);
        m_settingsLayer->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityOnDemand);
        m_settingsLayer->setActivateOnShow(true);
        m_settingsLayer->setScope(QStringLiteral("boris-settings"));
        if (m_screen)
            m_settingsLayer->setScreen(m_screen);
    }
    if (m_settingsOpen)
        syncSettings();
}

void Backend::openSettings()
{
    if (!m_settingsOpen) {
        m_settingsOpen = true;
        emit settingsOpenChanged();
    }
    if (!m_settings)
        return;
    if (m_settingsX < 0 && m_screen) {
        const QRect area = m_screen->geometry();
        m_settingsX = std::max(0, (area.width() - SettingsWidth) / 2);
        m_settingsY = std::max(0, (area.height() - SettingsHeight) / 2);
    }
    syncSettings();
    m_settings->requestActivate();
}

void Backend::closeSettings()
{
    if (!m_settingsOpen)
        return;
    m_settingsDrag = false;
    m_namedAvatarPick = false;
    clearAvatarDraft();
    setSettingsPage(QStringLiteral("home"));
    m_settingsOpen = false;
    emit settingsOpenChanged();
}

void Backend::setSettingsPage(const QString &page)
{
    QString next = page;
    if (next != QLatin1String("home") && next != QLatin1String("menu") && next != QLatin1String("avatars")
        && next != QLatin1String("pet"))
        next = QStringLiteral("home");
    if (m_settingsPage == next)
        return;
    m_settingsPage = next;
    emit settingsPageChanged();
}

void Backend::openAppsMenu()
{
    setSettingsPage(QStringLiteral("menu"));
}

void Backend::openHomePage()
{
    setSettingsPage(QStringLiteral("home"));
}

void Backend::openAvatarsPage()
{
    setSettingsPage(QStringLiteral("avatars"));
}

void Backend::openPetPage()
{
    setSettingsPage(QStringLiteral("pet"));
}

void Backend::beginNamedAvatarImport()
{
    m_namedAvatarPick = true;
    m_picker->openImage();
}

void Backend::clearAvatarDraft()
{
    if (m_avatarDraft.isEmpty())
        return;
    m_avatarDraft = QUrl();
    emit avatarDraftChanged();
}

void Backend::saveNamedAvatar(const QString &name)
{
    if (m_avatarDraft.isEmpty()) {
        setMessage(tr("Choose a picture first."));
        return;
    }
    setMessage(m_avatar.addNamedPicture(m_avatarDraft.toLocalFile(), name));
    clearAvatarDraft();
}

void Backend::selectAvatar(const QString &id)
{
    if (!m_avatar.selectAvatar(id))
        setMessage(tr("That look could not be used."));
    else
        setMessage({});
}

void Backend::removeAvatar(const QString &id)
{
    if (!m_avatar.removeAvatar(id))
        setMessage(tr("That look could not be removed."));
    else
        setMessage({});
}

void Backend::beginSettingsDrag(int globalX, int globalY)
{
    const QPoint local = QPoint(globalX, globalY) - screenOrigin();
    m_settingsGrabDx = local.x() - std::max(0, m_settingsX);
    m_settingsGrabDy = local.y() - std::max(0, m_settingsY);
    m_settingsDrag = true;
}

void Backend::endSettingsDrag()
{
    m_settingsDrag = false;
}

void Backend::syncSettings()
{
    if (!m_settings)
        return;
    if (m_settings->width() != SettingsWidth)
        m_settings->setWidth(SettingsWidth);
    if (m_settings->height() != SettingsHeight)
        m_settings->setHeight(SettingsHeight);
    if (!m_settingsLayer)
        return;
    m_settingsLayer->setDesiredSize(QSize(SettingsWidth, SettingsHeight));
    m_settingsLayer->setMargins(QMargins(std::max(0, m_settingsX), std::max(0, m_settingsY), 0, 0));
}

void Backend::setScalePercent(int percent)
{
    m_config.setScalePercent(percent);
}

void Backend::pressAt(int globalX, int globalY, int button)
{
    const QPoint local = QPoint(globalX, globalY) - screenOrigin();
    m_pet.press(local.x(), local.y(), button);
}

void Backend::releasePointer()
{
    m_pet.release();
}

void Backend::importPicture()
{
    m_picker->openImage();
}

void Backend::importFrames()
{
    m_picker->openDirectory();
}

void Backend::useParrot()
{
    m_avatar.useParrot();
    setMessage({});
}

void Backend::walkTheFloor()
{
    m_pet.walkTheFloor();
}

void Backend::note(const QString &message)
{
    setMessage(message);
}

void Backend::setMessage(const QString &message)
{
    if (m_message == message)
        return;
    m_message = message;
    if (!message.isEmpty())
        qWarning("%s", qPrintable(message));
    emit messageChanged();
}
