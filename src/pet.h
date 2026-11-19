#pragma once

#include <QObject>
#include <QString>

// Boris is a small layer-shell window, not a fullscreen overlay. His body is
// the whole window, so clicks anywhere else still reach the desktop. A hop
// moves that window up; talk and excited are separate moods (no shared "react").
// x is the distance from the left of his screen, y from the bottom.
// Pointer coordinates are in that same screen, with the origin at its top left.
class Pet : public QObject {
    Q_OBJECT
    Q_PROPERTY(int x READ x NOTIFY changed)
    Q_PROPERTY(int y READ y NOTIFY changed)
    Q_PROPERTY(int lift READ lift NOTIFY changed)
    Q_PROPERTY(int direction READ direction NOTIFY changed)
    Q_PROPERTY(QString mood READ mood NOTIFY changed)
    Q_PROPERTY(int frame READ frame NOTIFY changed)
    Q_PROPERTY(bool perched READ perched NOTIFY changed)
    Q_PROPERTY(int scalePercent READ scalePercent WRITE setScalePercent NOTIFY changed)
    Q_PROPERTY(int insetX READ insetX NOTIFY changed)
    Q_PROPERTY(int windowX READ windowX NOTIFY changed)
    Q_PROPERTY(int windowY READ windowY NOTIFY changed)
    Q_PROPERTY(int windowWidth READ windowWidth NOTIFY changed)
    Q_PROPERTY(int windowHeight READ windowHeight NOTIFY changed)
    Q_PROPERTY(int bodyWidth READ bodyWidth NOTIFY changed)
    Q_PROPERTY(int bodyHeight READ bodyHeight NOTIFY changed)
    Q_PROPERTY(qreal gazeX READ gazeX NOTIFY changed)
    Q_PROPERTY(qreal gazeY READ gazeY NOTIFY changed)
    Q_PROPERTY(bool napping READ napping NOTIFY changed)
    Q_PROPERTY(bool listening READ listening NOTIFY changed)

public:
    static constexpr int BodyWidth = 168;
    static constexpr int BodyHeight = 160;
    static constexpr int FloorReach = 64;
    static constexpr int DragSlop = 6;
    static constexpr int WalkSpeed = 90;
    static constexpr int HopHeight = 36;
    static constexpr int ReactMs = 520;
    static constexpr int TalkMs = 900;
    static constexpr int NapAfterMs = 9000;
    static constexpr int PreenMs = 900;

    explicit Pet(QObject *parent = nullptr);

    int x() const { return m_x; }
    int y() const { return m_y; }
    int lift() const { return m_lift; }
    int direction() const { return m_direction; }
    QString mood() const { return m_mood; }
    int frame() const { return m_frame; }
    bool perched() const { return m_y > floorReach(); }
    bool napping() const { return m_mood == QLatin1String("nap"); }
    bool listening() const { return m_listening; }
    bool pointerDown() const { return m_pointerDown; }
    bool hasStage() const { return m_hasStage; }
    int scalePercent() const { return m_scale; }
    qreal gazeX() const { return m_gazeX; }
    qreal gazeY() const { return m_gazeY; }

    int bodyWidth() const;
    int bodyHeight() const;
    int floorReach() const;
    int hopHeight() const;
    int walkSpeed() const;
    void setScalePercent(int percent);

    int windowX() const;
    int windowY() const;
    int windowWidth() const;
    int windowHeight() const;
    int insetX() const;

    void setStage(int width, int height);
    void place(int x, int y);
    void startWalking(int targetX);
    void tick(int ms);

    void setAttention(bool followCursor, bool idleAntics, bool sleepWhenIdle);
    void noticePointer(int screenX, int screenY);
    void clearGaze();

    void press(int x, int y, int button);
    void updatePointer(int x, int y);
    void release();

    // Hop lifts him. Talk / excited do not share that hop animation.
    Q_INVOKABLE void hop();
    Q_INVOKABLE void talk();
    Q_INVOKABLE void excite();
    Q_INVOKABLE void poke(); // alias for hop (Space / older callers)
    Q_INVOKABLE void walkTheFloor();
    Q_INVOKABLE void beginListen();
    Q_INVOKABLE void endListen();

signals:
    void changed();
    void settingsRequested();
    void hopped();
    void talked();
    void excited();

private:
    void clamp();
    void setMood(const QString &mood);
    bool advanceHop(int ms);
    bool advanceTalk(int ms);
    bool advanceRoam(int ms);
    bool advanceFrame(int ms);
    bool advanceAttention(int ms);
    void chooseNextAct();
    bool canRoam() const;
    void wakeFromNap();
    void finishAction();

    int m_stageWidth = 1280;
    int m_stageHeight = 720;
    bool m_hasStage = false;
    int m_x = 64;
    int m_y = 0;
    int m_lift = 0;
    int m_direction = 1;
    int m_targetX = 64;
    QString m_mood = QStringLiteral("idle");
    int m_frame = 0;
    int m_scale = 100;
    bool m_pointerDown = false;
    bool m_dragging = false;
    bool m_walking = false;
    bool m_listening = false;
    int m_pressX = 0;
    int m_pressY = 0;
    int m_grabDx = 0;
    int m_grabDy = 0;
    int m_goal = 900;
    int m_walkDebt = 0;
    int m_frameMs = 0;
    int m_reactLeft = 0;
    int m_talkLeft = 0;
    int m_idleMs = 0;
    int m_preenLeft = 0;
    qreal m_gazeX = 0;
    qreal m_gazeY = 0;
    bool m_followCursor = true;
    bool m_idleAntics = true;
    bool m_sleepWhenIdle = true;
};
