// Boris — a parrot that walks on the desktop.

#include <QCursor>
#include <QEventLoop>
#include <QFile>
#include <QGuiApplication>
#include <QIcon>
#include <QImage>
#include <QLocalSocket>
#include <QLockFile>
#include <QPainter>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QSurfaceFormat>
#include <QTemporaryDir>
#include <QTimer>
#include <QUrl>
#include <QWindow>

#include <cstdio>

#include "backend.h"
#include "theme.h"
#include "translator.h"

namespace {
struct Options {
    bool preview = false;
    bool panel = false;
    bool settings = false;
    QString previewPath;
    QString language;
};

Options parseOptions(int argc, char *argv[])
{
    Options options;
    for (int i = 1; i < argc; ++i) {
        const QString arg = QString::fromLocal8Bit(argv[i]);
        if ((arg == QLatin1String("--preview")) && i + 1 < argc) {
            options.preview = true;
            options.previewPath = QString::fromLocal8Bit(argv[++i]);
        } else if (arg == QLatin1String("--language") && i + 1 < argc) {
            options.language = QString::fromLocal8Bit(argv[++i]);
        } else if (arg == QLatin1String("--panel")) {
            options.panel = true;
        } else if (arg == QLatin1String("--settings")) {
            options.settings = true;
        } else if (arg == QLatin1String("--help") || arg == QLatin1String("-h")) {
            std::fputs("Usage: boris [--settings] [--preview FILE [--language CODE] [--panel]]\n", stdout);
            options.previewPath = QStringLiteral("-");
            options.preview = true;
        } else {
            std::fprintf(stderr, "Unknown argument: %s\n", argv[i]);
            options.previewPath = QStringLiteral("!");
        }
    }
    return options;
}
}

int main(int argc, char *argv[])
{
    const Options options = parseOptions(argc, argv);
    if (options.previewPath == QLatin1String("!"))
        return 1;
    if (options.preview && options.previewPath == QLatin1String("-"))
        return 0;

    qputenv("QT_FORCE_STDERR_LOGGING", "1");
    if (options.preview) {
        qputenv("QT_QPA_PLATFORM", "offscreen");
        qputenv("QT_QUICK_BACKEND", "software");
    } else {
        // One window, and it has to be a layer surface before the first show.
        qputenv("QT_WAYLAND_SHELL_INTEGRATION", "layer-shell");
    }

    QSurfaceFormat format;
    format.setAlphaBufferSize(8);
    QSurfaceFormat::setDefaultFormat(format);
    if (options.preview)
        QQuickWindow::setGraphicsApi(QSGRendererInterface::Software);

    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("boris"));
    app.setApplicationVersion(QStringLiteral("0.1.0"));
    app.setDesktopFileName(QStringLiteral("boris"));
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("boris")));
    QQuickStyle::setStyle(QStringLiteral("Material"));

    QTemporaryDir previewTheme;
    QTemporaryDir previewLanguageDir;
    QString languageFile;
    if (options.preview && !options.language.isEmpty()) {
        if (!previewLanguageDir.isValid())
            return 1;
        languageFile = previewLanguageDir.filePath(QStringLiteral("language"));
        QFile file(languageFile);
        if (!file.open(QIODevice::WriteOnly))
            return 1;
        file.write(options.language.toUtf8());
        file.write("\n");
    }

    Translator translator(languageFile);
    app.installTranslator(&translator);

    QLockFile lock(QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation).isEmpty()
                       ? QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QStringLiteral("/boris.lock")
                       : QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation) + QStringLiteral("/boris.lock"));
    if (!options.preview && !lock.tryLock(100)) {
        QLocalSocket socket;
        socket.connectToServer(Backend::socketPath());
        if (socket.waitForConnected(400)) {
            socket.write("open\n");
            socket.flush();
            socket.waitForBytesWritten(400);
        } else {
            qWarning("Boris is already on the desktop.");
        }
        return 0;
    }

    if (options.preview && !previewTheme.isValid())
        return 1;
    Theme theme(options.preview ? previewTheme.path() : QString());
    Backend backend;

    QQmlApplicationEngine engine;
    QObject::connect(&translator, &Translator::changed, &engine, &QQmlEngine::retranslate);
    engine.rootContext()->setContextProperty(QStringLiteral("translator"), &translator);
    engine.rootContext()->setContextProperty(QStringLiteral("theme"), &theme);
    engine.rootContext()->setContextProperty(QStringLiteral("backend"), &backend);
    engine.load(QUrl(QStringLiteral("qrc:/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return 1;

    auto *window = qobject_cast<QWindow *>(engine.rootObjects().constFirst());
    if (!window)
        return 1;

    QQmlComponent settingsComponent(&engine, QUrl(QStringLiteral("qrc:/SettingsWindow.qml")));
    if (settingsComponent.isError()) {
        qWarning("%s", qPrintable(settingsComponent.errorString()));
        return 1;
    }
    QObject *settingsObject = settingsComponent.create();
    auto *settingsWindow = qobject_cast<QWindow *>(settingsObject);
    if (!settingsWindow)
        return 1;

    if (options.preview)
        backend.pet()->setStage(800, 600);
    backend.attachWindow(window);
    backend.attachSettings(settingsWindow);
    if (options.preview) {
        backend.pet()->place(48, 0);
        backend.pet()->startWalking(420);
        backend.pet()->tick(200);
        if (options.panel)
            backend.openSettings();
    } else if (options.settings) {
        backend.openSettings();
    }
    backend.reveal();

    if (options.preview) {
        QEventLoop loop;
        QTimer::singleShot(120, &loop, &QEventLoop::quit);
        loop.exec();
        auto *quick = qobject_cast<QQuickWindow *>(options.panel ? settingsWindow : window);
        if (!quick)
            return 1;
        const QImage shot = quick->grabWindow();
        if (shot.isNull()) {
            qWarning("Boris could not draw a preview.");
            return 1;
        }
        QImage canvas(shot.size(), QImage::Format_ARGB32_Premultiplied);
        canvas.fill(QColor(QStringLiteral("#101418")));
        QPainter painter(&canvas);
        painter.drawImage(0, 0, shot);
        painter.end();
        if (!canvas.save(options.previewPath)) {
            qWarning("Boris could not save the preview.");
            return 1;
        }
        return 0;
    }

    backend.start();
    qInfo("Boris is walking the desktop.");
    return app.exec();
}
