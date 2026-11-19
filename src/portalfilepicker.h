#pragma once

#include <QVariantMap>

#include "filepicker.h"

class QDBusPendingCallWatcher;

// Portal file choosing follows Omacut (MIT, David Heinemeier Hansson).
class PortalFilePicker : public FilePicker {
    Q_OBJECT

public:
    explicit PortalFilePicker(QObject *parent = nullptr);

    void openImage() override;
    void openDirectory() override;

private slots:
    void handleResponse(uint response, const QVariantMap &results);

private:
    enum class Action {
        None,
        Image,
        Directory
    };

    void requestFile(const QString &title, const QVariantMap &options, Action action);
    bool connectToRequestPath(const QString &path);
    void clearPending();
    void closePendingRequest();

    QString m_pendingPath;
    Action m_pendingAction = Action::None;
    int m_pendingGeneration = 0;
    QDBusPendingCallWatcher *m_pendingWatcher = nullptr;
};
