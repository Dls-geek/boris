#pragma once

#include <QObject>
#include <QUrl>

// The real picker talks to the desktop portal. Tests supply their own.
class FilePicker : public QObject {
    Q_OBJECT

public:
    explicit FilePicker(QObject *parent = nullptr) : QObject(parent) {}
    ~FilePicker() override = default;

    virtual void openImage() = 0;
    virtual void openDirectory() = 0;

signals:
    void imageSelected(const QUrl &url);
    void directorySelected(const QUrl &url);
    void failed(const QString &message);
};
