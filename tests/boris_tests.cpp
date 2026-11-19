#include <QDir>
#include <QFile>
#include <QImage>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QUrl>
#include <QtTest>

#include "avatar.h"
#include "backend.h"
#include "config.h"
#include "cursor.h"
#include "filepicker.h"
#include "pet.h"
#include "theme.h"
#include "translator.h"

class FakePicker : public FilePicker {
public:
    explicit FakePicker(QObject *parent = nullptr) : FilePicker(parent) {}
    void openImage() override { imageOpened = true; }
    void openDirectory() override { directoryOpened = true; }
    bool imageOpened = false;
    bool directoryOpened = false;
};

class BorisTests : public QObject {
    Q_OBJECT

private slots:
    void clickHops()
    {
        Pet pet;
        pet.setStage(800, 600);
        pet.place(100, 0);
        const int top = 600 - Pet::BodyHeight;
        pet.press(100, top, Qt::LeftButton);
        pet.release();
        QCOMPARE(pet.mood(), QStringLiteral("react"));
        QCOMPARE(pet.x(), 100);
    }

    void hopRisesAndLands()
    {
        Pet pet;
        pet.setStage(800, 600);
        pet.place(20, 0);
        pet.poke();
        pet.tick(260);
        QCOMPARE(pet.lift(), Pet::HopHeight);
        pet.tick(260);
        QCOMPARE(pet.lift(), 0);
        QCOMPARE(pet.mood(), QStringLiteral("idle"));
    }

    void dragAlongTheFloor()
    {
        Pet pet;
        pet.setStage(800, 600);
        pet.place(100, 0);
        const int top = 600 - Pet::BodyHeight;
        pet.press(100, top, Qt::LeftButton);
        pet.updatePointer(103, top);
        pet.release();
        QCOMPARE(pet.mood(), QStringLiteral("react"));
        QCOMPARE(pet.x(), 100);

        pet.place(100, 0);
        pet.press(100, top, Qt::LeftButton);
        pet.updatePointer(180, top);
        pet.release();
        QCOMPARE(pet.x(), 180);
        QCOMPARE(pet.y(), 0);
        QCOMPARE(pet.mood(), QStringLiteral("idle"));
    }

    void dragUpPerches()
    {
        Pet pet;
        pet.setStage(800, 600);
        pet.place(100, 0);
        const int top = 600 - Pet::BodyHeight;
        pet.press(100, top, Qt::LeftButton);
        pet.updatePointer(100, top - 140);
        pet.release();
        QCOMPARE(pet.y(), 140);
        QVERIFY(pet.perched());
        QCOMPARE(pet.mood(), QStringLiteral("sit"));
        pet.tick(4000);
        QCOMPARE(pet.x(), 100);
    }

    void walkFacesTheTargetAndStops()
    {
        Pet pet;
        pet.setStage(800, 600);
        pet.place(100, 0);
        pet.startWalking(10);
        QCOMPARE(pet.direction(), -1);
        pet.tick(500);
        QCOMPARE(pet.x(), 55);
        QCOMPARE(pet.mood(), QStringLiteral("walk"));

        pet.place(0, 0);
        pet.startWalking(100);
        pet.tick(2000);
        QCOMPARE(pet.x(), 100);
        QCOMPARE(pet.mood(), QStringLiteral("idle"));
    }

    void placeClampsToTheStage()
    {
        Pet pet;
        pet.setStage(200, 400);
        pet.place(1000, 1000);
        QCOMPARE(pet.x(), 200 - Pet::BodyWidth);
        QCOMPARE(pet.y(), 400 - Pet::BodyHeight);
        QVERIFY(pet.perched());
    }

    void rightClickAsksForSettings()
    {
        Pet pet;
        pet.setStage(360, 800);
        pet.place(1000, 0);
        QCOMPARE(pet.x(), 360 - Pet::BodyWidth);
        QSignalSpy spy(&pet, &Pet::settingsRequested);
        pet.press(0, 0, Qt::RightButton);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(pet.windowWidth(), Pet::BodyWidth);
        QCOMPARE(pet.windowHeight(), Pet::BodyHeight);
    }

    void scaleChangesHisBody()
    {
        Pet pet;
        pet.setStage(800, 600);
        pet.place(10, 0);
        pet.setScalePercent(50);
        QCOMPARE(pet.bodyWidth(), 84);
        QCOMPARE(pet.bodyHeight(), 80);
        QCOMPARE(pet.hopHeight(), 18);
        pet.poke();
        pet.tick(260);
        QCOMPARE(pet.lift(), 18);

        pet.setScalePercent(10);
        QCOMPARE(pet.scalePercent(), 40);
        QCOMPARE(pet.bodyWidth(), 67);
        QCOMPARE(pet.bodyHeight(), 64);
    }

    void sizeIsRemembered()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        Config config(dir.path());
        QCOMPARE(config.scalePercent(), 100);
        config.setScalePercent(140);
        Config again(dir.path());
        QCOMPARE(again.scalePercent(), 140);
        again.setScalePercent(5);
        QCOMPARE(again.scalePercent(), Config::MinimumScale);
    }

    void attentionSettingsPersist()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        Config config(dir.path());
        QVERIFY(config.followCursor());
        config.setFollowCursor(false);
        config.setIdleAntics(false);
        config.setSleepWhenIdle(false);
        Config again(dir.path());
        QVERIFY(!again.followCursor());
        QVERIFY(!again.idleAntics());
        QVERIFY(!again.sleepWhenIdle());
    }

    void gazeFollowsANoticedPointer()
    {
        Pet pet;
        pet.setStage(800, 600);
        pet.place(100, 0);
        pet.setAttention(true, true, true);
        pet.noticePointer(400, 100);
        QVERIFY(pet.gazeX() > 0.2);
        pet.clearGaze();
        QCOMPARE(pet.gazeX(), 0);
    }

    void napsAfterSittingStill()
    {
        Pet pet;
        pet.setStage(800, 600);
        pet.place(40, 200);
        pet.setAttention(true, true, true);
        QVERIFY(pet.perched());
        pet.tick(Pet::NapAfterMs + 50);
        QVERIFY(pet.napping());
        pet.noticePointer(60, 200);
        QVERIFY(!pet.napping());
    }

    void cursorInjectReportsAMove()
    {
        Cursor cursor;
        QSignalSpy spy(&cursor, &Cursor::moved);
        cursor.inject(QPoint(12, 34));
        QVERIFY(cursor.available());
        QCOMPARE(spy.count(), 1);
        QCOMPARE(cursor.position(), QPoint(12, 34));
    }

    void walkTheFloorLeavesAPerch()
    {
        Pet pet;
        pet.setStage(800, 600);
        pet.place(40, 200);
        QVERIFY(pet.perched());
        pet.walkTheFloor();
        QCOMPARE(pet.y(), 0);
        QVERIFY(!pet.perched());
    }

    void pictureImportCopiesAndReloads()
    {
        QTemporaryDir dir;
        const QString store = dir.filePath(QStringLiteral("store"));
        const QString source = dir.filePath(QStringLiteral("bird.png"));
        QImage image(8, 8, QImage::Format_ARGB32);
        image.fill(Qt::red);
        QVERIFY(image.save(source));

        Avatar avatar(store);
        QCOMPARE(avatar.importPicture(source), QString());
        QCOMPARE(avatar.kind(), QStringLiteral("picture"));
        QVERIFY(avatar.source().toLocalFile().contains(QStringLiteral("/library/")));
        QVERIFY(avatar.source().toLocalFile().endsWith(QStringLiteral(".png")));
        QVERIFY(!avatar.animated());
        QVERIFY(QFileInfo::exists(source));
        QCOMPARE(avatar.catalog().size(), 2);

        Avatar again(store);
        QCOMPARE(again.kind(), QStringLiteral("picture"));
        again.useParrot();
        QCOMPARE(again.kind(), QStringLiteral("parrot"));
        Avatar restored(store);
        QCOMPARE(restored.kind(), QStringLiteral("parrot"));
    }

    void gifImportKeepsExtensionAndMarksAnimated()
    {
        QTemporaryDir dir;
        const QString store = dir.filePath(QStringLiteral("store"));
        const QString source = dir.filePath(QStringLiteral("spin.gif"));
        // Minimal GIF89a: header + logical screen + trailer. Enough for copy + suffix checks.
        QFile gif(source);
        QVERIFY(gif.open(QIODevice::WriteOnly));
        gif.write(QByteArray::fromHex("47494638396101000100800000ffffff0000002c000000000100010000020144003b"));
        gif.close();

        Avatar avatar(store);
        QCOMPARE(avatar.addNamedPicture(source, QStringLiteral("Spin")), QString());
        QCOMPARE(avatar.kind(), QStringLiteral("picture"));
        QVERIFY(avatar.source().toLocalFile().endsWith(QStringLiteral(".gif")));
        QVERIFY(avatar.animated());
        QCOMPARE(avatar.displayName(), QStringLiteral("Spin"));
        QVERIFY(avatar.selectAvatar(QStringLiteral("parrot")));
        QVERIFY(!avatar.animated());
        const QVariantList catalog = avatar.catalog();
        QCOMPARE(catalog.size(), 2);
        QCOMPARE(catalog.at(1).toMap().value(QStringLiteral("animated")).toBool(), true);
        QVERIFY(avatar.selectAvatar(catalog.at(1).toMap().value(QStringLiteral("id")).toString()));
        QVERIFY(avatar.animated());
    }

    void pictureRejectsTheWrongTypeAndAMissingFile()
    {
        QTemporaryDir dir;
        const QString store = dir.filePath(QStringLiteral("store"));
        QFile text(dir.filePath(QStringLiteral("notes.txt")));
        QVERIFY(text.open(QIODevice::WriteOnly));
        text.write("nope");
        text.close();

        Avatar avatar(store);
        QCOMPARE(avatar.importPicture(text.fileName()), QStringLiteral("That file type cannot be an avatar."));
        QCOMPARE(avatar.kind(), QStringLiteral("parrot"));
        QCOMPARE(avatar.importPicture(dir.filePath(QStringLiteral("missing.png"))),
                 QStringLiteral("The picture could not be read."));
    }

    void brokenPictureFallsBackToTheParrot()
    {
        QTemporaryDir dir;
        const QString store = dir.filePath(QStringLiteral("store"));
        const QString source = dir.filePath(QStringLiteral("bird.png"));
        QImage image(4, 4, QImage::Format_ARGB32);
        image.fill(Qt::blue);
        QVERIFY(image.save(source));

        Avatar avatar(store);
        QVERIFY(avatar.importPicture(source).isEmpty());
        const QString saved = avatar.source().toLocalFile();
        QVERIFY(saved.contains(QStringLiteral("/library/")));
        QVERIFY(QFile::remove(saved));

        Avatar again(store);
        QCOMPARE(again.kind(), QStringLiteral("parrot"));
        QCOMPARE(again.error(), QStringLiteral("The picture could not be read."));
    }

    void namedAvatarDraftAndPages()
    {
        QTemporaryDir dir;
        const QString store = dir.filePath(QStringLiteral("store"));
        const QString source = dir.filePath(QStringLiteral("bird.png"));
        QImage image(4, 4, QImage::Format_ARGB32);
        image.fill(Qt::cyan);
        QVERIFY(image.save(source));

        FakePicker picker;
        Backend backend(store, &picker, dir.filePath(QStringLiteral("config")));
        QCOMPARE(backend.settingsPage(), QStringLiteral("home"));
        backend.openSettings();
        backend.openAppsMenu();
        QCOMPARE(backend.settingsPage(), QStringLiteral("menu"));
        backend.openPetPage();
        QCOMPARE(backend.settingsPage(), QStringLiteral("pet"));
        backend.openAvatarsPage();
        QCOMPARE(backend.settingsPage(), QStringLiteral("avatars"));

        backend.beginNamedAvatarImport();
        QVERIFY(picker.imageOpened);
        emit picker.imageSelected(QUrl::fromLocalFile(source));
        QCOMPARE(backend.avatarDraft(), QUrl::fromLocalFile(source));
        backend.saveNamedAvatar(QStringLiteral("Cyan bird"));
        QCOMPARE(backend.avatar()->displayName(), QStringLiteral("Cyan bird"));
        QVERIFY(backend.avatarDraft().isEmpty());
        backend.closeSettings();
        QCOMPARE(backend.settingsPage(), QStringLiteral("home"));
        QVERIFY(!backend.settingsOpen());
    }

    void framesNeedAnIdlePose()
    {
        QTemporaryDir dir;
        const QString store = dir.filePath(QStringLiteral("store"));
        const QString folder = dir.filePath(QStringLiteral("frames"));
        QVERIFY(QDir().mkpath(folder));
        QImage image(4, 4, QImage::Format_ARGB32);
        image.fill(Qt::green);
        QVERIFY(image.save(folder + QStringLiteral("/walk-a.png")));

        Avatar avatar(store);
        QCOMPARE(avatar.importFrames(folder), QStringLiteral("That folder needs an idle.png."));
        QCOMPARE(avatar.importFrames(dir.filePath(QStringLiteral("absent"))),
                 QStringLiteral("Could not open that folder."));

        QVERIFY(image.save(folder + QStringLiteral("/idle.png")));
        QVERIFY(image.save(folder + QStringLiteral("/walk-b.png")));
        QVERIFY(image.save(folder + QStringLiteral("/talk.png")));
        QCOMPARE(avatar.importFrames(folder), QString());
        QCOMPARE(avatar.kind(), QStringLiteral("frames"));
        avatar.setPose(QStringLiteral("walk"), 0);
        QVERIFY(avatar.source().toLocalFile().endsWith(QStringLiteral("walk-a.png")));
        avatar.setPose(QStringLiteral("walk"), 1);
        QVERIFY(avatar.source().toLocalFile().endsWith(QStringLiteral("walk-b.png")));
        avatar.setPose(QStringLiteral("react"), 0);
        QVERIFY(avatar.source().toLocalFile().endsWith(QStringLiteral("talk.png")));
        avatar.setPose(QStringLiteral("idle"), 0);
        QVERIFY(avatar.source().toLocalFile().endsWith(QStringLiteral("idle.png")));
    }

    void pickerHandsThePictureToTheStore()
    {
        QTemporaryDir dir;
        const QString store = dir.filePath(QStringLiteral("store"));
        const QString source = dir.filePath(QStringLiteral("bird.png"));
        QImage image(4, 4, QImage::Format_ARGB32);
        image.fill(Qt::yellow);
        QVERIFY(image.save(source));

        FakePicker picker;
        Backend backend(store, &picker, dir.filePath(QStringLiteral("config")));
        backend.importPicture();
        QVERIFY(picker.imageOpened);
        emit picker.imageSelected(QUrl::fromLocalFile(source));
        QCOMPARE(backend.avatar()->kind(), QStringLiteral("picture"));
        QCOMPARE(backend.message(), QString());

        backend.importFrames();
        QVERIFY(picker.directoryOpened);

        QCOMPARE(backend.scalePercent(), 100);
        backend.setScalePercent(80);
        QCOMPARE(backend.pet()->bodyWidth(), 134);
        backend.openSettings();
        QVERIFY(backend.settingsOpen());
        backend.closeSettings();
        QVERIFY(!backend.settingsOpen());
        Config stored(dir.filePath(QStringLiteral("config")));
        QCOMPARE(stored.scalePercent(), 80);
    }

    void themeFallsBack()
    {
        QTemporaryDir dir;
        Theme theme(dir.path());
        QCOMPARE(theme.accent(), QStringLiteral("#FFD60A"));
        QCOMPARE(theme.accentForeground(), QStringLiteral("black"));
    }

    void themeReadsAccent()
    {
        QTemporaryDir dir;
        QDir(dir.path()).mkpath(QStringLiteral("theme"));
        QFile colors(dir.path() + QStringLiteral("/theme/colors.toml"));
        QVERIFY(colors.open(QIODevice::WriteOnly));
        colors.write("accent = \"#112233\"\n");
        colors.close();

        Theme theme(dir.path());
        QCOMPARE(theme.accent(), QStringLiteral("#112233"));
        QCOMPARE(theme.accentForeground(), QStringLiteral("white"));
    }

    void themeFollowsHyprlandRounding()
    {
        QTemporaryDir dir;
        const QString response = dir.filePath(QStringLiteral("response.json"));
        const QString command = dir.filePath(QStringLiteral("hyprctl"));
        auto write = [](const QString &path, const QByteArray &data) {
            QFile file(path);
            return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
        };
        QVERIFY(write(command, "#!/bin/sh\ncat '" + response.toUtf8() + "'\n"));
        QVERIFY(QFile::setPermissions(command, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
        QVERIFY(write(response, "{\"int\":0}"));

        Theme theme(dir.path(), nullptr, command);
        QCOMPARE(theme.radius(), 0);
        QVERIFY(write(response, "{\"int\":6}"));
        QTRY_COMPARE(theme.radius(), 6);
    }

    void translatorFallsBackToEnglish()
    {
        QTemporaryDir dir;
        Translator translator(dir.path() + QStringLiteral("/language"), dir.path());
        QCOMPARE(translator.language(), QStringLiteral("en"));
        QVERIFY(translator.translate("Main", "Save").isNull());
    }

    void translatorFollowsTheLanguageFile()
    {
        QTemporaryDir dir;
        write(dir.path() + QStringLiteral("/language"), "pt-BR\n");
        write(dir.path() + QStringLiteral("/pt.json"), R"({"Save": "Guardar", "Open": "Abrir"})");
        write(dir.path() + QStringLiteral("/pt-BR.json"), R"({"Save": "Salvar"})");

        Translator translator(dir.path() + QStringLiteral("/language"), dir.path());
        QCOMPARE(translator.language(), QStringLiteral("pt-BR"));
        QCOMPARE(translator.translate("Main", "Save"), QStringLiteral("Salvar"));
        QCOMPARE(translator.translate("Main", "Open"), QStringLiteral("Abrir"));
        QVERIFY(translator.translate("Main", "Quit").isNull());
    }

    void translatorPicksPluralForms()
    {
        QTemporaryDir dir;
        write(dir.path() + QStringLiteral("/language"), "ru\n");
        write(dir.path() + QStringLiteral("/en.json"),
              R"({"%n clips": {"one": "%n clip", "other": "%n clips"}})");
        write(dir.path() + QStringLiteral("/ru.json"),
              R"({"%n clips": {"one": "%n клип", "few": "%n клипа", "many": "%n клипов", "other": "%n клипа"}})");

        Translator translator(dir.path() + QStringLiteral("/language"), dir.path());
        QCOMPARE(translator.translate("Main", "%n clips", nullptr, 21), QStringLiteral("%n клип"));
        QCOMPARE(translator.translate("Main", "%n clips", nullptr, 22), QStringLiteral("%n клипа"));
        QCOMPARE(translator.translate("Main", "%n clips", nullptr, 25), QStringLiteral("%n клипов"));
        QCOMPARE(Translator::pluralCategory(QStringLiteral("ar"), 11), QStringLiteral("many"));
        QCOMPARE(Translator::pluralCategory(QStringLiteral("ja"), 1), QStringLiteral("other"));
    }

private:
    static void write(const QString &path, const QByteArray &contents)
    {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(contents);
    }
};

QTEST_MAIN(BorisTests)
#include "boris_tests.moc"
