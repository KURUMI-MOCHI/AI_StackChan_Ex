#include "PomodoroMod.h"
#include <M5Unified.h>
#include "Robot.h"
#include "mod/ModManager.h"

using namespace m5avatar;

/// 外部参照 ///
extern Avatar avatar;
extern Robot *robot;
extern bool servo_home;
extern void sw_tone();
extern void alarm_tone();
///////////////

PomodoroMod::PomodoroMod(bool _isOffline)
  : isOffline{_isOffline}, isSilentMode{true}, last_touch_y{-1},
    headTouchHappyActive{false}, headTouchHappyUntilMs{0}, prev_servo_home_state{true}
{
    default_a_min = DEFAULT_A_MIN;
    default_b_min = DEFAULT_B_MIN;
    current_a_min = default_a_min;
    current_b_min = default_b_min;
    status = READY_A;

    box_top_A.setupBox(0, 0, 100, 50);
    box_top_B.setupBox(220, 0, 100, 50);
    box_center.setupBox(50, 50, 220, 140);
}

void PomodoroMod::init(void) {
    status = READY_A;
    current_a_min = default_a_min;
    current_b_min = default_b_min;

    avatar.setSpeechText("");
    updateLEDState();
    update();
    avatar.set_isSubWindowEnable(true);
}

void PomodoroMod::pause(void) {
    avatar.set_isSubWindowEnable(false);
    avatar.setSpeechText("");
    LedController.setEmotion(LedEmotion::NORMAL);
    if (status == RUNNING_A || status == RUNNING_B) {
        status = (status == RUNNING_A) ? PAUSED_A : PAUSED_B;
    }
}

void PomodoroMod::btnA_pressed(void) {
    LedController.flashFeedback();
    display_touched(160, 120);
}

void PomodoroMod::btnB_pressed(void) {
    LedController.flashFeedback();
}

void PomodoroMod::btnC_pressed(void) {
    LedController.flashFeedback();
    isSilentMode = !isSilentMode;
}

void PomodoroMod::drawSubWindow(M5Canvas *spi, BoundingRect rect, DrawContext *ctx, void *userData) {
    if (userData == nullptr) return;
    static_cast<PomodoroMod*>(userData)->drawPomodoroUI(spi, rect, ctx);
}

void PomodoroMod::drawPomodoroUI(M5Canvas *spi, BoundingRect rect, DrawContext *ctx) {
    bool isModeA = (status == RUNNING_A || status == PAUSED_A || status == READY_A);
    uint16_t theme_color = isModeA ? COLOR_ORANGE : COLOR_TEAL;

    uint32_t target_total_min = isModeA ? current_a_min : current_b_min;
    uint32_t remaining_sec = 0;

    if (status == RUNNING_A || status == RUNNING_B) {
        uint32_t elapsed = millis() - start_time_ms;
        if (elapsed < total_duration_ms) {
            remaining_sec = (total_duration_ms - elapsed) / 1000;
        }
    } else if (status == PAUSED_A || status == PAUSED_B) {
        remaining_sec = paused_remaining_ms / 1000;
    } else {
        remaining_sec = target_total_min * 60;
    }

    // 1分ごとに1目盛り減る仕様 (全60分割セグメント)
    int active_bars = (remaining_sec + 59) / 60;
    if (active_bars > 60) active_bars = 60;

    // --- 1. 太めの円形インジケーター描画 (r=100〜118) ---
    int cx = 160;
    int cy = 120;
    int r_outer = 118;
    int r_inner = 100;

    for (int i = 0; i < 60; i++) {
        float angle = (-90.0f + i * 6.0f) * DEG_TO_RAD;
        uint16_t c = (i < active_bars) ? theme_color : COLOR_GRAY;

        int x1 = cx + cos(angle) * r_inner;
        int y1 = cy + sin(angle) * r_inner;
        int x2 = cx + cos(angle) * r_outer;
        int y2 = cy + sin(angle) * r_outer;

        spi->drawLine(x1, y1, x2, y2, c);
        spi->drawLine(x1 + (cos(angle) * 0.5f), y1 + (sin(angle) * 0.5f), x2, y2, c);
    }

    // --- 2. 上部ヘッダーUI描画 (A, Bの「分:秒」詳細表示) ---
    spi->setTextSize(1.5);
    spi->setTextColor(TFT_WHITE, TFT_BLACK);

    uint32_t disp_min = remaining_sec / 60;
    uint32_t disp_sec = remaining_sec % 60;
    char sec_buf[8];
    sprintf(sec_buf, "%02u", disp_sec);

    String textA = "A:" + String(status == RUNNING_A || status == PAUSED_A || status == READY_A ? String(disp_min) + ":" + sec_buf : String(current_a_min) + ":00");
    String textB = "B:" + String(status == RUNNING_B || status == PAUSED_B || status == READY_B ? String(disp_min) + ":" + sec_buf : String(current_b_min) + ":00");

    spi->fillRect(0, 0, 110, 24, TFT_BLACK);
    spi->fillRect(210, 0, 110, 24, TFT_BLACK);

    spi->drawString(textA.c_str(), 5, 6);
    spi->drawString(textB.c_str(), 215, 6);
}

void PomodoroMod::update(void) {
    avatar.updateSubWindowCustom(PomodoroMod::drawSubWindow, this, 0, 0, 320, 240);
}

void PomodoroMod::updateLEDState(void) {
    if (status == RUNNING_A || status == READY_A || status == PAUSED_A) {
        LedController.setEmotion(LedEmotion::HAPPY);
    } else {
        LedController.setEmotion(LedEmotion::CALM);
    }
}

void PomodoroMod::triggerNotification(void) {
    if (robot && robot->servo) {
        robot->servo->moveTo(0, -5);
    }
    LedController.flashFeedback();
    if (!isSilentMode) {
        alarm_tone();
    }
    delay(1000);
    if (robot && robot->servo) {
        robot->servo->moveTo(0, 0);
    }
}

void PomodoroMod::updateHeadTouchExpression(void) {
    HeadTouchSensor::Gesture gesture = HeadTouchSensor::update();

    if (HeadTouchSensor::isPetGesture(gesture)) {
        if (!headTouchHappyActive) {
            headTouchHappyActive = true;
            prev_servo_home_state = servo_home;
            servo_home = false;

            avatar.setExpression(Expression::Happy);
            LedController.setEmotion(LedEmotion::HAPPY);

            if (robot != nullptr && robot->servo != nullptr) {
                robot->servo->moveTo(0, -5);
            }
        }
        headTouchHappyUntilMs = millis() + 3000;
    }

    if (headTouchHappyActive && millis() >= headTouchHappyUntilMs) {
        headTouchHappyActive = false;
        avatar.setExpression(Expression::Neutral);
        updateLEDState();

        if (robot != nullptr && robot->servo != nullptr) {
            robot->servo->moveToOrigin();
        }
        servo_home = prev_servo_home_state;
    }
}

void PomodoroMod::display_touched(int16_t x, int16_t y) {
    if (box_top_A.contain(x, y)) {
        LedController.flashFeedback();
        current_a_min = default_a_min;
        status = READY_A;
        updateLEDState();
        update();
        return;
    }

    if (box_top_B.contain(x, y)) {
        LedController.flashFeedback();
        current_b_min = default_b_min;
        status = READY_B;
        updateLEDState();
        update();
        return;
    }

    if (box_center.contain(x, y)) {
        LedController.flashFeedback();
        if (status == READY_A) {
            start_time_ms = millis();
            total_duration_ms = current_a_min * 60 * 1000;
            status = RUNNING_A;
        } else if (status == RUNNING_A) {
            paused_remaining_ms = total_duration_ms - (millis() - start_time_ms);
            status = PAUSED_A;
        } else if (status == PAUSED_A) {
            start_time_ms = millis() - (total_duration_ms - paused_remaining_ms);
            status = RUNNING_A;
        } else if (status == READY_B) {
            start_time_ms = millis();
            total_duration_ms = current_b_min * 60 * 1000;
            status = RUNNING_B;
        } else if (status == RUNNING_B) {
            paused_remaining_ms = total_duration_ms - (millis() - start_time_ms);
            status = PAUSED_B;
        } else if (status == PAUSED_B) {
            start_time_ms = millis() - (total_duration_ms - paused_remaining_ms);
            status = RUNNING_B;
        }
        updateLEDState();
        update();
        return;
    }

    // スライド操作での時間変更
    if (status == READY_A || status == PAUSED_A || status == READY_B || status == PAUSED_B) {
        if (x < 40 || x > 280) {
            auto touch_detail = M5.Touch.getDetail();
            if (touch_detail.isPressed()) {
                if (last_touch_y >= 0) {
                    int16_t dy = touch_detail.y - last_touch_y;
                    if (abs(dy) > 15) {
                        if (status == READY_A || status == PAUSED_A) {
                            if (dy < 0 && current_a_min < 99) current_a_min++;
                            else if (dy > 0 && current_a_min > 1) current_a_min--;
                            if (status == PAUSED_A) {
                                paused_remaining_ms = current_a_min * 60 * 1000;
                                total_duration_ms = paused_remaining_ms;
                            }
                        } else {
                            if (dy < 0 && current_b_min < 99) current_b_min++;
                            else if (dy > 0 && current_b_min > 1) current_b_min--;
                            if (status == PAUSED_B) {
                                paused_remaining_ms = current_b_min * 60 * 1000;
                                total_duration_ms = paused_remaining_ms;
                            }
                        }
                        last_touch_y = touch_detail.y;
                        update();
                    }
                } else {
                    last_touch_y = touch_detail.y;
                }
            } else {
                last_touch_y = -1;
            }
        }
    }
}

void PomodoroMod::idle(void) {
    updateHeadTouchExpression();
    update();

    if (status == RUNNING_A || status == RUNNING_B) {
        uint32_t elapsed = millis() - start_time_ms;

        if (elapsed >= total_duration_ms) {
            triggerNotification();

            // A終了 -> すぐにBスタート、 B終了 -> すぐにAスタート
            if (status == RUNNING_A) {
                current_b_min = default_b_min;
                start_time_ms = millis();
                total_duration_ms = current_b_min * 60 * 1000;
                status = RUNNING_B;
            } else {
                current_a_min = default_a_min;
                start_time_ms = millis();
                total_duration_ms = current_a_min * 60 * 1000;
                status = RUNNING_A;
            }
            updateLEDState();
            update();
        }
    }
}