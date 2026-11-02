#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <deque>

using namespace geode::prelude;

class $modify(AssistLayer, GJBaseGameLayer) {
    struct Fields {
        float time = 0.f;
        float lastPress = -100.f;
        bool buffered = false;
        bool holding = false;
        bool internal = false;
        bool spamReleased = false;
        float nextClick = 0.f;
        std::deque<float> presses;
    };

    bool assistActive() {
        if (Mod::get()->getSettingValue<bool>("practice-only") && !m_isPracticeMode) return false;
        return true;
    }

    void handleButton(bool down, int button, bool isPlayer1) {
        auto f = m_fields.self();
        if (!f->internal && button == 1 && isPlayer1) {
            if (down) {
                f->lastPress = f->time;
                f->buffered = true;
                f->holding = true;
                f->presses.push_back(f->time);
                while (!f->presses.empty() && f->time - f->presses.front() > 0.6f)
                    f->presses.pop_front();
            } else {
                f->holding = false;
                f->buffered = false;
                f->spamReleased = false;
            }
        }
        GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }

    void processCommands(float dt) {
        auto f = m_fields.self();
        f->time += dt;

        if (assistActive() && m_player1) {
            if (Mod::get()->getSettingValue<bool>("buffer-enabled") && f->buffered) {
                float window = Mod::get()->getSettingValue<double>("buffer-ms") / 1000.f;
                if (f->time - f->lastPress > window) {
                    f->buffered = false;
                } else if (m_player1->m_touchingRings && m_player1->m_touchingRings->count() > 0) {
                    f->internal = true;
                    this->handleButton(false, 1, true);
                    this->handleButton(true, 1, true);
                    f->internal = false;
                    f->buffered = false;
                }
            }

            if (Mod::get()->getSettingValue<bool>("spam-enabled") && f->holding && f->presses.size() >= 3) {
                float span = f->presses.back() - f->presses.front();
                float interval = span / (f->presses.size() - 1);
                float minInterval = 1.f / Mod::get()->getSettingValue<double>("spam-max-cps");
                if (interval < minInterval) interval = minInterval;
                if (f->time - f->presses.back() > interval * 1.5f && f->time >= f->nextClick) {
                    f->internal = true;
                    if (!f->spamReleased) {
                        this->handleButton(false, 1, true);
                        f->spamReleased = true;
                    } else {
                        this->handleButton(true, 1, true);
                        f->spamReleased = false;
                        f->nextClick = f->time + interval;
                    }
                    f->internal = false;
                }
            }
        }

        GJBaseGameLayer::processCommands(dt);
    }
};
