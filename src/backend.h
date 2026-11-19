#pragma once

#include <QElapsedTimer>
#include <QLocalServer>
#include <QObject>
#include <QPoint>
#include <QTimer>
#include <QUrl>

#include "avatar.h"
#include "config.h"
#include "cursor.h"
#include "pet.h"

class FilePicker;
class QScreen;
class QWindow;

namespace LayerShellQt {
class Window;
}

class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(Pet *pet READ pet CONSTANT)
    Q_PROPERTY(Avatar *avatar READ avatar CONSTANT)
    Q_PROPERTY(QString message READ message NOTIFY messageChanged)
    Q_PROPERTY(bool revealed READ revealed NOTIFY revealedChanged)
    Q_PROPERTY(bool settingsOpen READ settingsOpen NOTIFY settingsOpenChanged)
    Q_PROPERTY(int scalePercent READ scalePercent WRITE setScalePercent NOTIFY scalePercentChanged)
    Q_PROPERTY(bool followCursor READ followCursor WRITE setFollowCursor NOTIFY attentionChanged)
    Q_PROPERTY(bool idleAntics READ idleAntics WRITE setIdleAntics NOTIFY attentionChanged)
    Q_PROPERTY(bool sleepWhenIdle READ sleepWhenIdle WRITE setSleepWhenIdle NOTIFY attentionChanged)
    Q_PROPERTY(int settingsWidth READ settingsWidth CONSTANT)
    Q_PROPERTY(int settingsHeight READ settingsHeight CONSTANT)
    Q_PROPERTY(QString settingsPage READ settingsPage WRITE setSettingsPage NOTIFY settingsPageChanged)
    Q_PROPERTY(QUrl avatarDraft READ avatarDraft NOTIFY avatarDraftChanged)

public:
    explicit Backend(const QString &avatarDirectory = {}, FilePicker *picker = nullptr,
                     const QString &configDirectory = {}, QObject *parent = nullptr);

    Pet *pet() { return &m_pet; }
    const Pet *pet() const { return &m_pet; }
    Avatar *avatar() { return &m_avatar; }
    const Avatar *avatar() const { return &m_avatar; }
    QString message() const { return m_message; }
    bool revealed() const { return m_revealed; }
    bool settingsOpen() const { return m_settingsOpen; }
    int scalePercent() const { return m_config.scalePercent(); }
    Q_INVOKABLE void setScalePercent(int percent);
    bool followCursor() const { return m_config.followCursor(); }
    Q_INVOKABLE void setFollowCursor(bool enabled);
    bool idleAntics() const { return m_config.idleAntics(); }
    Q_INVOKABLE void setIdleAntics(bool enabled);
    bool sleepWhenIdle() const { return m_config.sleepWhenIdle(); }
    Q_INVOKABLE void setSleepWhenIdle(bool enabled);
    int settingsWidth() const { return SettingsWidth; }
    int settingsHeight() const { return SettingsHeight; }
    QString settingsPage() const { return m_settingsPage; }
    void setSettingsPage(const QString &page);
    QUrl avatarDraft() const { return m_avatarDraft; }

    static constexpr int SettingsWidth = 440;
    static constexpr int SettingsHeight = 900;

    static QString socketPath();

    void attachWindow(QWindow *window);
    void attachSettings(QWindow *window);
    void reveal();
    void start();
    Q_INVOKABLE void syncWindow();
    Q_INVOKABLE void openSettings();
    Q_INVOKABLE void closeSettings();
    Q_INVOKABLE void beginSettingsDrag(int globalX, int globalY);
    Q_INVOKABLE void endSettingsDrag();

    Q_INVOKABLE void pressAt(int globalX, int globalY, int button);
    Q_INVOKABLE void releasePointer();
    Q_INVOKABLE void importPicture();
    Q_INVOKABLE void importFrames();
    Q_INVOKABLE void useParrot();
    Q_INVOKABLE void walkTheFloor();
    Q_INVOKABLE void note(const QString &message);
    Q_INVOKABLE void openAppsMenu();
    Q_INVOKABLE void openHomePage();
    Q_INVOKABLE void openAvatarsPage();
    Q_INVOKABLE void openPetPage();
    Q_INVOKABLE void beginNamedAvatarImport();
    Q_INVOKABLE void clearAvatarDraft();
    Q_INVOKABLE void saveNamedAvatar(const QString &name);
    Q_INVOKABLE void selectAvatar(const QString &id);
    Q_INVOKABLE void removeAvatar(const QString &id);

signals:
    void messageChanged();
    void revealedChanged();
    void settingsOpenChanged();
    void scalePercentChanged();
    void attentionChanged();
    void settingsPageChanged();
    void avatarDraftChanged();

private:
    void applyAttention();
    void followScreen(QScreen *screen);
    void onTick();
    void setMessage(const QString &message);
    void syncSettings();
    void listenForLaunches();
    QPoint screenOrigin() const;

    Pet m_pet;
    Avatar m_avatar;
    Config m_config;
    Cursor m_cursor;
    FilePicker *m_picker = nullptr;
    QWindow *m_window = nullptr;
    QWindow *m_settings = nullptr;
    LayerShellQt::Window *m_layer = nullptr;
    LayerShellQt::Window *m_settingsLayer = nullptr;
    QScreen *m_screen = nullptr;
    QMetaObject::Connection m_screenConnection;
    QLocalServer m_server;
    QTimer m_timer;
    QElapsedTimer m_clock;
    qint64 m_lastMs = 0;
    QString m_message;
    QString m_settingsPage = QStringLiteral("home");
    QUrl m_avatarDraft;
    bool m_revealed = false;
    bool m_settingsOpen = false;
    bool m_settingsDrag = false;
    bool m_namedAvatarPick = false;
    int m_settingsX = -1;
    int m_settingsY = -1;
    int m_settingsGrabDx = 0;
    int m_settingsGrabDy = 0;
};
