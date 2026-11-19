#include "pet.h"

#include <QRandomGenerator>

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace {
constexpr double kPi = 3.141592653589793;
}

Pet::Pet(QObject *parent) : QObject(parent) {}

int Pet::bodyWidth() const
{
    return std::max(1, BodyWidth * m_scale / 100);
}

int Pet::bodyHeight() const
{
    return std::max(1, BodyHeight * m_scale / 100);
}

int Pet::floorReach() const
{
    return std::max(16, FloorReach * m_scale / 100);
}

int Pet::hopHeight() const
{
    return std::max(8, HopHeight * m_scale / 100);
}

int Pet::walkSpeed() const
{
    return std::max(24, WalkSpeed * m_scale / 100);
}

void Pet::setScalePercent(int percent)
{
    percent = std::clamp(percent, 40, 200);
    if (percent == m_scale)
        return;
    m_scale = percent;
    clamp();
    emit changed();
}

int Pet::windowWidth() const
{
    return bodyWidth();
}

int Pet::windowHeight() const
{
    return bodyHeight();
}

int Pet::windowX() const
{
    const int width = windowWidth();
    int origin = m_x;
    if (origin + width > m_stageWidth)
        origin = std::max(0, m_stageWidth - width);
    return std::max(0, origin);
}

int Pet::windowY() const
{
    return std::max(0, m_y + m_lift);
}

int Pet::insetX() const
{
    return m_x - windowX();
}

void Pet::setStage(int width, int height)
{
    m_stageWidth = std::max(width, 1);
    m_stageHeight = std::max(height, 1);
    m_hasStage = true;
    clamp();
    emit changed();
}

void Pet::place(int x, int y)
{
    m_x = x;
    m_y = y;
    m_lift = 0;
    m_walking = false;
    m_walkDebt = 0;
    m_pointerDown = false;
    m_dragging = false;
    m_reactLeft = 0;
    clamp();
    m_goal = 5000;
    setMood(m_y > floorReach() ? QStringLiteral("sit") : QStringLiteral("idle"));
    emit changed();
}

void Pet::startWalking(int targetX)
{
    const int maxX = std::max(0, m_stageWidth - bodyWidth());
    m_targetX = std::clamp(targetX, 0, maxX);
    m_y = 0;
    m_lift = 0;
    m_walkDebt = 0;
    m_frameMs = 0;
    m_goal = 20000;
    m_reactLeft = 0;
    m_direction = m_targetX < m_x ? -1 : 1;
    m_walking = m_targetX != m_x;
    setMood(m_walking ? QStringLiteral("walk") : QStringLiteral("idle"));
    emit changed();
}

void Pet::clamp()
{
    const int maxX = std::max(0, m_stageWidth - bodyWidth());
    const int maxY = std::max(0, m_stageHeight - bodyHeight());
    m_x = std::clamp(m_x, 0, maxX);
    m_y = std::clamp(m_y, 0, maxY);
}

void Pet::setMood(const QString &mood)
{
    if (m_mood == mood)
        return;
    m_mood = mood;
    emit changed();
}

bool Pet::canRoam() const
{
    return !m_pointerDown && !napping() && m_mood != QLatin1String("preen")
           && m_y <= floorReach() && m_mood != QLatin1String("drag");
}

void Pet::setAttention(bool followCursor, bool idleAntics, bool sleepWhenIdle)
{
    m_followCursor = followCursor;
    m_idleAntics = idleAntics;
    m_sleepWhenIdle = sleepWhenIdle;
    if (!m_followCursor)
        clearGaze();
    if (!m_sleepWhenIdle && napping()) {
        m_idleMs = 0;
        setMood(QStringLiteral("sit"));
    }
}

void Pet::clearGaze()
{
    if (m_gazeX == 0 && m_gazeY == 0)
        return;
    m_gazeX = 0;
    m_gazeY = 0;
    emit changed();
}

void Pet::noticePointer(int screenX, int screenY)
{
    if (!m_followCursor || m_pointerDown || m_mood == QLatin1String("drag"))
        return;

    const int headX = m_x + bodyWidth() * 2 / 3;
    const int headY = m_stageHeight - (m_y + m_lift) - bodyHeight() / 2;
    const qreal dx = (screenX - headX) / qreal(std::max(120, bodyWidth() * 2));
    const qreal dy = (screenY - headY) / qreal(std::max(120, bodyHeight() * 2));
    const qreal gx = std::clamp(dx, -1.0, 1.0);
    const qreal gy = std::clamp(dy, -1.0, 1.0);
    const bool near = std::abs(screenX - headX) < bodyWidth() * 2
                      && std::abs(screenY - headY) < bodyHeight() * 2;

    if (napping() && near)
        wakeFromNap();

    if (std::abs(gx - m_gazeX) < 0.04 && std::abs(gy - m_gazeY) < 0.04)
        return;
    m_gazeX = gx;
    m_gazeY = gy;
    m_idleMs = 0;
    emit changed();
}

void Pet::wakeFromNap()
{
    if (!napping())
        return;
    m_idleMs = 0;
    setMood(m_y > floorReach() ? QStringLiteral("sit") : QStringLiteral("idle"));
}

void Pet::tick(int ms)
{
    if (ms < 0)
        ms = 0;

    bool dirty = false;
    if (m_mood == QLatin1String("react"))
        dirty = advanceReact(ms);
    else if (m_mood == QLatin1String("preen")) {
        m_preenLeft -= ms;
        if (m_preenLeft <= 0) {
            m_preenLeft = 0;
            m_goal = 800;
            setMood(QStringLiteral("idle"));
            dirty = true;
        }
    } else if (canRoam())
        dirty = advanceRoam(ms);

    if (!m_pointerDown && m_mood != QLatin1String("react") && m_mood != QLatin1String("drag")
        && m_mood != QLatin1String("preen"))
        dirty = advanceAttention(ms) || dirty;

    if (m_mood == QLatin1String("walk"))
        dirty = advanceFrame(ms) || dirty;
    else if (m_frame != 0 && m_mood != QLatin1String("preen")) {
        m_frame = 0;
        m_frameMs = 0;
        dirty = true;
    }

    if (dirty)
        emit changed();
}

bool Pet::advanceAttention(int ms)
{
    if (m_pointerDown || m_mood == QLatin1String("drag") || m_mood == QLatin1String("react"))
        return false;

    m_idleMs += ms;
    if (!m_sleepWhenIdle || napping())
        return false;
    if (m_idleMs < NapAfterMs)
        return false;
    if (!(m_mood == QLatin1String("sit") || m_mood == QLatin1String("idle") || perched()))
        return false;

    m_walking = false;
    setMood(QStringLiteral("nap"));
    return true;
}

bool Pet::advanceReact(int ms)
{
    m_reactLeft -= ms;
    const int elapsed = ReactMs - std::max(0, m_reactLeft);
    const double t = std::clamp(elapsed / double(ReactMs), 0.0, 1.0);
    const int hop = int(std::lround(std::sin(t * kPi) * hopHeight()));
    const bool moved = hop != m_lift;
    m_lift = hop;
    if (m_reactLeft > 0)
        return moved;

    m_lift = 0;
    m_reactLeft = 0;
    if (m_y > floorReach())
        setMood(QStringLiteral("sit"));
    else if (m_walking)
        setMood(QStringLiteral("walk"));
    else {
        m_goal = 600;
        setMood(QStringLiteral("idle"));
    }
    return true;
}

bool Pet::advanceRoam(int ms)
{
    if (m_walking) {
        m_goal -= ms;
        m_walkDebt += walkSpeed() * ms;
        const int step = m_walkDebt / 1000;
        m_walkDebt %= 1000;
        if (step == 0 && m_goal > 0)
            return false;

        const int maxX = std::max(0, m_stageWidth - bodyWidth());
        const int next = m_x + m_direction * step;
        const bool arrived = m_direction > 0 ? next >= m_targetX : next <= m_targetX;
        if (arrived || m_goal <= 0) {
            if (arrived)
                m_x = std::clamp(m_targetX, 0, maxX);
            m_walking = false;
            m_walkDebt = 0;
            m_goal = 700;
            setMood(QStringLiteral("idle"));
            return true;
        }

        m_x = std::clamp(next, 0, maxX);
        if (m_x == 0 || m_x == maxX) {
            m_walking = false;
            m_walkDebt = 0;
            m_goal = 700;
            setMood(QStringLiteral("idle"));
        }
        return true;
    }

    m_goal -= ms;
    if (m_goal > 0)
        return false;
    chooseNextAct();
    return true;
}

bool Pet::advanceFrame(int ms)
{
    m_frameMs += ms;
    if (m_frameMs < 140)
        return false;
    m_frameMs %= 140;
    m_frame = m_frame == 0 ? 1 : 0;
    return true;
}

void Pet::chooseNextAct()
{
    const int maxX = std::max(0, m_stageWidth - bodyWidth());
    m_frameMs = 0;
    m_idleMs = 0;
    if (maxX < 24) {
        m_walking = false;
        m_goal = 2000;
        setMood(QStringLiteral("sit"));
        return;
    }

    // Nala deals idle antics from a bag. We keep a small three-way draw so
    // walk / sit / preen-or-hop each show up without needing a shuffle list yet.
    const int roll = QRandomGenerator::global()->bounded(m_idleAntics ? 5 : 4);
    if (roll == 0) {
        m_walking = false;
        m_goal = 1600 + QRandomGenerator::global()->bounded(2200);
        setMood(QStringLiteral("sit"));
        return;
    }
    if (m_idleAntics && roll == 1) {
        m_walking = false;
        m_preenLeft = PreenMs;
        m_frame = 1;
        setMood(QStringLiteral("preen"));
        return;
    }
    if (m_idleAntics && roll == 2) {
        m_walking = false;
        poke();
        return;
    }

    m_targetX = QRandomGenerator::global()->bounded(maxX + 1);
    if (std::abs(m_targetX - m_x) < 24)
        m_targetX = m_x > maxX / 2 ? 0 : maxX;
    m_direction = m_targetX < m_x ? -1 : 1;
    m_walking = true;
    m_walkDebt = 0;
    m_goal = 20000;
    setMood(QStringLiteral("walk"));
}

void Pet::press(int x, int y, int button)
{
    if (button == Qt::RightButton) {
        emit settingsRequested();
        return;
    }
    if (button != Qt::LeftButton)
        return;

    wakeFromNap();
    m_idleMs = 0;
    m_pointerDown = true;
    m_dragging = false;
    m_pressX = x;
    m_pressY = y;
    m_grabDx = x - m_x;
    const int petTop = m_stageHeight - (m_y + m_lift) - bodyHeight();
    m_grabDy = y - petTop;
}

void Pet::updatePointer(int x, int y)
{
    if (!m_pointerDown)
        return;
    if (!m_dragging) {
        if (std::abs(x - m_pressX) < DragSlop && std::abs(y - m_pressY) < DragSlop)
            return;
        m_dragging = true;
        m_walking = false;
        m_lift = 0;
        m_reactLeft = 0;
        setMood(QStringLiteral("drag"));
    }

    m_x = x - m_grabDx;
    const int petTop = y - m_grabDy;
    m_y = m_stageHeight - petTop - bodyHeight();
    clamp();
    emit changed();
}

void Pet::release()
{
    if (!m_pointerDown)
        return;
    m_pointerDown = false;
    if (!m_dragging) {
        poke();
        return;
    }

    m_dragging = false;
    m_lift = 0;
    if (m_y <= floorReach()) {
        m_y = 0;
        m_walking = false;
        m_goal = 700;
        setMood(QStringLiteral("idle"));
    } else {
        m_walking = false;
        setMood(QStringLiteral("sit"));
    }
    emit changed();
}

void Pet::poke()
{
    if (m_pointerDown)
        return;
    wakeFromNap();
    m_idleMs = 0;
    m_frame = 0;
    m_reactLeft = ReactMs;
    m_lift = 0;
    if (m_mood != QLatin1String("react"))
        setMood(QStringLiteral("react"));
    else
        emit changed();
}

void Pet::walkTheFloor()
{
    m_y = 0;
    m_lift = 0;
    m_pointerDown = false;
    m_dragging = false;
    m_walking = false;
    m_reactLeft = 0;
    m_preenLeft = 0;
    m_idleMs = 0;
    m_goal = 200;
    setMood(QStringLiteral("idle"));
    emit changed();
}
