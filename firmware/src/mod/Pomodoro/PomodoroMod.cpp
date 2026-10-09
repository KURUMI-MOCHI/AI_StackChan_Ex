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

    // 顔中央のタップ検出ボックス (スタート/一時停止用)
    box_center.setupBox(80, 60, 160, 120);
}

void PomodoroMod::init(void) {
    status = READY_A;
    current_a_min = default_a_min;
    current_b_min = default_b_min;

    updateLEDState();
    updateSpeechText();
    update();
    avatar.set_isSubWindowEnable(true);
}

void PomodoroMod::pause(void) {
    avatar.set_isSubWindowEnable(false);
    avatar.setSpeechText("");
    LedController.setEmotion(LedEmotion::NORMAL);
    LedController.setState(LedState::STANDBY);
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
    // モード切替トグル
    if (status == READY_A || status == PAUSED_A) {
        status = READY_B;
    } else if (status == READY_B || status == PAUSED_B) {
        status = READY_A;
    }
    updateLEDState();
    updateSpeechText();
    update();
}

void PomodoroMod::btnC_pressed(void) {
    LedController.flashFeedback();
}

// 吹き出し（SpeechText）の更新処理
void PomodoroMod::updateSpeechText(void) {
    bool isModeA = (status == RUNNING_A || status == PAUSED_A || status == READY_A);
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

    uint32_t disp_min = (remaining_sec + 59) / 60;
    if (status == READY_A) disp_min = current_a_min;
    if (status == READY_B) disp_min = current_b_min;

    String speech = isModeA ? "Focus " : "Break ";
    speech += String(disp_min) + "m";

    if (status == RUNNING_A || status == RUNNING_B) {
        speech += "...";
    }

    avatar.setSpeechText(speech.c_str());
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

    int active_bars = (remaining_sec + 59) / 60;
    if (active_bars > MAX_MIN) active_bars = MAX_MIN;

    // --- 1. 横長楕円弧インジケーター描画 (吹き出し回避・極太仕様) ---
    int cx = 160;
    int cy = 120;
    float rx_outer = 145.0f; // 外半径 (以前より大幅拡大)
    float ry_outer = 105.0f;
    float rx_inner = 120.0f; // 内半径 (厚み約25pxの極太仕様)
    float ry_inner = 80.0f;

    for (int i = 1; i <= active_bars; i++) {
        // 1分刻み: 12時方向 (angle = -90 deg) から時計周り
        float angle_deg = -90.0f + (i * 6.0f);
        float angle_rad = angle_deg * DEG_TO_RAD;

        // 目盛り線の始点と終点
        int x_o = cx + (int)(cos(angle_rad) * rx_outer);
        int y_o = cy + (int)(sin(angle_rad) * ry_outer);

        // ★ 吹き出し領域 (右上エリア: x >= 160 かつ y <= 95) は描画をスキップして楕円弧にする
        if (x_o >= 160 && y_o <= 95) {
            continue;
        }

        int x_i = cx + (int)(cos(angle_rad) * rx_inner);
        int y_i = cy + (int)(sin(angle_rad) * ry_inner);

        // 太いセグメントを描画 (幅5pxのマルチライン描画)
        for (int w = -2; w <= 2; w++) {
            float offset_angle = (angle_deg + (w * 0.8f)) * DEG_TO_RAD;
            int xo = cx + (int)(cos(offset_angle) * rx_outer);
            int yo = cy + (int)(sin(offset_angle) * ry_outer);
            int xi = cx + (int)(cos(offset_angle) * rx_inner);
            int yi = cy + (int)(sin(offset_angle) * ry_inner);
            spi->drawLine(xi, yi, xo, yo, theme_color);
        }
    }
}

void PomodoroMod::update(void) {
    updateSpeechText();
    avatar.updateSubWindowCustom(PomodoroMod::drawSubWindow, this, 0, 0, 320, 240);
}

void PomodoroMod::updateLEDState(void) {
    if (status == RUNNING_A || status == READY_A || status == PAUSED_A) {
        LedController.setCustomStandbyColor(255, 60, 0);   // モードA: オレンジ
    } else {
        LedController.setCustomStandbyColor(0, 200, 160);   // モードB: ティールグリーン
    }
    LedController.setState(LedState::STANDBY);
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

// 触った位置が外周インジケーター（リング領域）内かどうかの判定
bool PomodoroMod::isRingArea(int16_t x, int16_t y) {
    float dx = (float)(x - 160) / 132.5f;
    float dy = (float)(y - 120) / 92.5f;
    float dist = sqrt(dx * dx + dy * dy);
    // 正規化距離が 0.70 〜 1.25 の範囲であればインジケーターリング上
    return (dist >= 0.70f && dist <= 1.25f);
}

// 楕円上のタッチ角度から 1〜60分 の数値を計算
uint32_t PomodoroMod::getMinuteFromTouchPos(int16_t x, int16_t y) {
    float angle_rad = atan2((float)(y - 120), (float)(x - 160));
    float angle_deg = angle_rad * RAD_TO_DEG + 90.0f; // 12時位置を 0 deg に補正
    if (angle_deg < 0) angle_deg += 360.0f;

    int min_val = (int)round(angle_deg / 6.0f);
    if (min_val < 1) min_val = 1;
    if (min_val > MAX_MIN) min_val = MAX_MIN;
    return min_val;
}

// ダイヤルなぞり操作のリアルタイム適用
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
    // 1. 中央タップ：スタート / 一時停止
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

    // インジケーターリングなぞり判定 (READY/PAUSED 時)
    if (status == READY_A || status == PAUSED_A || status == READY_B || status == PAUSED_B) {
        auto touch_detail = M5.Touch.getDetail();
        if (touch_detail.isPressed()) {
            if (isRingArea(touch_detail.x, touch_detail.y)) {
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

            // タイムアップ時の自動連続切り替え（カスタム時間は1回限りで標準値に自動復帰）
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