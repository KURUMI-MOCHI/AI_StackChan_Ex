#include "PomodoroMod.h"
#include <M5Unified.h>
#include "Robot.h"
#include "mod/ModManager.h"

using namespace m5avatar;

/// 外部参照 ///
extern Avatar avatar;
extern Robot *robot;
extern void sw_tone();
extern void alarm_tone();
///////////////

PomodoroMod::PomodoroMod(bool _isOffline)
  : isOffline{_isOffline}, isSilentMode{true}, is_sliding{false}, last_touch_y{-1},
    is_active(false)
{
    default_a_min = DEFAULT_A_MIN;
    default_b_min = DEFAULT_B_MIN;
    current_a_min = default_a_min;
    current_b_min = default_b_min;
    status = READY_A;

    // タッチ領域定義
    box_top_A.setupBox(0, 0, 100, 50);        // 左上 A領域
    box_top_B.setupBox(220, 0, 100, 50);      // 右上 B領域
    box_center.setupBox(50, 50, 220, 140);    // 中央（顔エリア）
}

void PomodoroMod::init(void) {
    status = READY_A;
    current_a_min = default_a_min;
    current_b_min = default_b_min;
    is_active = true;

    avatar.setSpeechText("");
}

void PomodoroMod::pause(void) {
    is_active = false;
    avatar.setSpeechText("");
    if (status == RUNNING_A || status == RUNNING_B) {
        status = (status == RUNNING_A) ? PAUSED_A : PAUSED_B;
    }
}

void PomodoroMod::btnA_pressed(void) {
    sw_tone();
    display_touched(160, 120);
}

void PomodoroMod::btnB_pressed(void) {
    sw_tone();
}

void PomodoroMod::btnC_pressed(void) {
    sw_tone();
    isSilentMode = !isSilentMode;
}

// 画面（Canvas）に対するオーバーレイ描画
void PomodoroMod::drawOverlayUI(M5Canvas *canvas) {
    if (!is_active || !canvas) return;

    uint16_t theme_color = (status == RUNNING_A || status == PAUSED_A || status == READY_A) ? COLOR_ORANGE : COLOR_TEAL;
    float ratio = 1.0f;

    // 残り時間割合計算
    if (status == RUNNING_A || status == RUNNING_B) {
        uint32_t elapsed = millis() - start_time_ms;
        if (elapsed < total_duration_ms) {
            ratio = 1.0f - ((float)elapsed / (float)total_duration_ms);
        } else {
            ratio = 0.0f;
        }
    } else if (status == PAUSED_A || status == PAUSED_B) {
        ratio = (float)paused_remaining_ms / (float)total_duration_ms;
    }

    // --- 1. 円形インジケーター描画 (外周 r=104〜118) ---
    int cx = canvas->width() / 2;   // 160
    int cy = canvas->height() / 2;  // 120
    int r_outer = 118;
    int r_inner = 104;

    int active_bars = (int)(60.0f * ratio);

    for (int i = 0; i < 60; i++) {
        float angle = (-90.0f + i * 6.0f) * DEG_TO_RAD;
        uint16_t c = (i < active_bars) ? theme_color : COLOR_GRAY;

        int x1 = cx + cos(angle) * r_inner;
        int y1 = cy + sin(angle) * r_inner;
        int x2 = cx + cos(angle) * r_outer;
        int y2 = cy + sin(angle) * r_outer;

        canvas->drawLine(x1, y1, x2, y2, c);
    }

    // --- 2. 上部ヘッダーUI描画 (A, B) ---
    canvas->setTextSize(2);
    canvas->setTextColor(TFT_WHITE, TFT_BLACK);

    String textA = "A";
    if (status == RUNNING_A) {
        uint32_t elapsed = millis() - start_time_ms;
        uint32_t remaining_sec = (total_duration_ms > elapsed) ? (total_duration_ms - elapsed) / 1000 : 0;
        uint32_t remaining_min = (remaining_sec / 60) + 1;
        textA += ":" + String(remaining_min);
    } else if (status == PAUSED_A) {
        uint32_t remaining_min = (paused_remaining_ms / 1000 / 60) + 1;
        textA += ":" + String(remaining_min);
    } else if (status == READY_A) {
        textA += ":" + String(current_a_min);
    }

    String textB = "B";
    if (status == RUNNING_B) {
        uint32_t elapsed = millis() - start_time_ms;
        uint32_t remaining_sec = (total_duration_ms > elapsed) ? (total_duration_ms - elapsed) / 1000 : 0;
        uint32_t remaining_min = (remaining_sec / 60) + 1;
        textB += ":" + String(remaining_min);
    } else if (status == PAUSED_B) {
        uint32_t remaining_min = (paused_remaining_ms / 1000 / 60) + 1;
        textB += ":" + String(remaining_min);
    } else if (status == READY_B) {
        textB += ":" + String(current_b_min);
    }

    canvas->fillRect(0, 0, 90, 24, TFT_BLACK);
    canvas->fillRect(230, 0, 90, 24, TFT_BLACK);

    canvas->drawString(textA.c_str(), 5, 4);
    canvas->drawString(textB.c_str(), 235, 4);
}

void PomodoroMod::updateBreathingLED(uint16_t themeColor) {
    if (!robot) return;
    float brightness = (sin(millis() / 400.0f) + 1.0f) / 2.0f * 0.8f + 0.2f;
}

void PomodoroMod::triggerNotification(void) {
    if (robot && robot->servo) {
        robot->servo->moveTo(0, -5);
    }
    alarm_tone();
    delay(1000);
    if (robot && robot->servo) {
        robot->servo->moveTo(0, 0);
    }
}

void PomodoroMod::display_touched(int16_t x, int16_t y) {
    if (box_top_A.contain(x, y)) {
        sw_tone();
        current_a_min = default_a_min;
        status = READY_A;
        return;
    }

    if (box_top_B.contain(x, y)) {
        sw_tone();
        current_b_min = default_b_min;
        status = READY_B;
        return;
    }

    if (box_center.contain(x, y)) {
        sw_tone();
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
        return;
    }

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
    uint16_t theme_color = (status == RUNNING_A || status == PAUSED_A || status == READY_A) ? COLOR_ORANGE : COLOR_TEAL;

    if (status == RUNNING_A || status == RUNNING_B) {
        updateBreathingLED(theme_color);
    }

    if (status == RUNNING_A || status == RUNNING_B) {
        uint32_t elapsed = millis() - start_time_ms;

        if (elapsed >= total_duration_ms) {
            triggerNotification();

            if (status == RUNNING_A) {
                current_a_min = default_a_min;
                status = READY_B;
            } else {
                current_b_min = default_b_min;
                status = READY_A;
            }
        }
    }
}