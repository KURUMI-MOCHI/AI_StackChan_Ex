#include "PomodoroMod.h"
#include <M5Unified.h>
#include "Robot.h"
#include "mod/ModManager.h"

using namespace m5avatar;

/// 外部参照 ///
extern Avatar avatar;
extern Robot *robot;
extern bool servo_home;
///////////////

PomodoroMod::PomodoroMod(bool _isOffline)
  : isOffline{_isOffline}, is_dial_dragging{false},
    headTouchHappyActive{false}, headTouchHappyUntilMs{0}, prev_servo_home_state{true}
{
    default_a_min = DEFAULT_A_MIN;
    default_b_min = DEFAULT_B_MIN;
    current_a_min = default_a_min;
    current_b_min = default_b_min;
    status = READY_A;

    box_top_A.setupBox(20, 20, 80, 40);
    box_top_B.setupBox(220, 20, 80, 40);
    box_center.setupBox(80, 60, 160, 120);
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

    // 残り分数の計算 (1分刻み)
    int active_bars = (remaining_sec + 59) / 60;
    if (active_bars > MAX_MIN) active_bars = MAX_MIN;

    // --- 1. 画面外周に沿った角丸四角形インジケーター (60分割) ---
    const float total_len = 1056.0f;
    const float step_len = total_len / 60.0f;
    const int bar_size = 16;

    for (int i = 1; i <= 60; i++) {
        uint16_t c = (i <= active_bars) ? theme_color : COLOR_GRAY;

        float d = i * step_len;
        int x1 = 0, y1 = 0, x2 = 0, y2 = 0;

        if (d <= 152.0f) { // 上辺（右半分）
            x1 = 160 + (int)d; y1 = 8;
            x2 = x1;           y2 = y1 + bar_size;
        } else if (d <= 376.0f) { // 右辺
            x1 = 312;          y1 = 8 + (int)(d - 152.0f);
            x2 = x1 - bar_size; y2 = y1;
        } else if (d <= 680.0f) { // 下辺
            x1 = 312 - (int)(d - 376.0f); y1 = 232;
            x2 = x1;                      y2 = y1 - bar_size;
        } else if (d <= 904.0f) { // 左辺
            x1 = 8;            y1 = 232 - (int)(d - 680.0f);
            x2 = x1 + bar_size; y2 = y1;
        } else { // 上辺（左半分）
            x1 = 8 + (int)(d - 904.0f); y1 = 8;
            x2 = x1;                    y2 = y1 + bar_size;
        }

        spi->drawLine(x1, y1, x2, y2, c);
        if (x1 == x2) {
            spi->drawLine(x1 + 1, y1, x2 + 1, y2, c);
        } else {
            spi->drawLine(x1, y1 + 1, x2, y2 + 1, c);
        }
    }

    // --- 2. 内側にシフトした上部ヘッダーUI ---
    spi->setTextSize(2);
    spi->setTextColor(TFT_WHITE, TFT_BLACK);

    uint32_t disp_min = (remaining_sec + 59) / 60;
    if (status == READY_A) disp_min = current_a_min;
    if (status == READY_B) disp_min = current_b_min;

    String textA = "A:" + String(isModeA ? disp_min : current_a_min);
    if (status == RUNNING_A) textA += "...";

    String textB = "B:" + String(!isModeA ? disp_min : current_b_min);
    if (status == RUNNING_B) textB += "...";

    spi->fillRect(20, 24, 85, 24, TFT_BLACK);
    spi->fillRect(210, 24, 85, 24, TFT_BLACK);

    spi->drawString(textA.c_str(), 25, 26);
    spi->drawString(textB.c_str(), 215, 26);
}

void PomodoroMod::update(void) {
    avatar.updateSubWindowCustom(PomodoroMod::drawSubWindow, this, 0, 0, 320, 240);
}

void PomodoroMod::updateLEDState(void) {
    if (status == RUNNING_A || status == READY_A || status == PAUSED_A) {
        LedController.setRawColor(255, 100, 0);   // モードA: オレンジ
    } else {
        LedController.setRawColor(0, 200, 160);   // モードB: ティールグリーン
    }
}

void PomodoroMod::triggerNotification(void) {
    if (robot && robot->servo) {
        robot->servo->moveTo(0, -5);
    }
    LedController.flashFeedback();
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

// タッチ座標から 1〜60分 の数値を計算
uint32_t PomodoroMod::getMinuteFromTouchPos(int16_t x, int16_t y) {
    float d = 0;
    if (y < 40) { // 上辺
        if (x >= 160) d = x - 160;
        else d = 1056 - (160 - x);
    } else if (x > 260) { // 右辺
        d = 152 + (y - 8);
    } else if (y > 200) { // 下辺
        d = 376 + (312 - x);
    } else { // 左辺
        d = 680 + (232 - y);
    }

    int min_val = (int)round(d / (1056.0f / 60.0f));
    if (min_val < 1) min_val = 1;
    if (min_val > MAX_MIN) min_val = MAX_MIN;
    return min_val;
}

// 現在のモードに対してダイヤル位置の分数を適用（リアルタイム更新）
void PomodoroMod::handleDialTouch(int16_t x, int16_t y) {
    if (status != READY_A && status != PAUSED_A && status != READY_B && status != PAUSED_B) {
        return;
    }

    uint32_t selected_min = getMinuteFromTouchPos(x, y);

    if (status == READY_A || status == PAUSED_A) {
        if (current_a_min != selected_min) {
            current_a_min = selected_min;
            if (status == PAUSED_A) {
                paused_remaining_ms = current_a_min * 60 * 1000;
                total_duration_ms = paused_remaining_ms;
            }
            update();
        }
    } else if (status == READY_B || status == PAUSED_B) {
        if (current_b_min != selected_min) {
            current_b_min = selected_min;
            if (status == PAUSED_B) {
                paused_remaining_ms = current_b_min * 60 * 1000;
                total_duration_ms = paused_remaining_ms;
            }
            update();
        }
    }
}

void PomodoroMod::display_touched(int16_t x, int16_t y) {
    // 1. 左上タップ：モードAに切り替えて標準値(25分)リセット
    if (box_top_A.contain(x, y)) {
        LedController.flashFeedback();
        current_a_min = default_a_min;
        status = READY_A;
        updateLEDState();
        update();
        return;
    }

    // 2. 右上タップ：モードBに切り替えて標準値(5分)リセット
    if (box_top_B.contain(x, y)) {
        LedController.flashFeedback();
        current_b_min = default_b_min;
        status = READY_B;
        updateLEDState();
        update();
        return;
    }

    // 3. 中央タップ：スタート / 一時停止
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
}

void PomodoroMod::idle(void) {
    updateHeadTouchExpression();

    // 外周ダイヤルの追従スライド操作判定 (STOP/PAUSE時のみ)
    if (status == READY_A || status == PAUSED_A || status == READY_B || status == PAUSED_B) {
        auto touch_detail = M5.Touch.getDetail();
        if (touch_detail.isPressed()) {
            // ヘッダーボタンや顔中央以外を触っている時にダイヤル追従
            if (!box_center.contain(touch_detail.x, touch_detail.y) &&
                !box_top_A.contain(touch_detail.x, touch_detail.y) &&
                !box_top_B.contain(touch_detail.x, touch_detail.y)) {

                if (!is_dial_dragging) {
                    LedController.flashFeedback();
                    is_dial_dragging = true;
                }
                handleDialTouch(touch_detail.x, touch_detail.y);
            }
        } else {
            is_dial_dragging = false;
        }
    } else {
        is_dial_dragging = false;
    }

    update();

    if (status == RUNNING_A || status == RUNNING_B) {
        uint32_t elapsed = millis() - start_time_ms;

        if (elapsed >= total_duration_ms) {
            triggerNotification();

            // タイムアップ時の自動切り替え（ワンショット適用後に標準値へ自動復帰）
            if (status == RUNNING_A) {
                current_a_min = default_a_min;
                current_b_min = default_b_min;
                start_time_ms = millis();
                total_duration_ms = current_b_min * 60 * 1000;
                status = RUNNING_B;
            } else {
                current_b_min = default_b_min;
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