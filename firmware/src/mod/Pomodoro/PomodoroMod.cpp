#include "PomodoroMod.h"
#include <Avatar.h>
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
  : isOffline{_isOffline}, isSilentMode{true}, is_sliding{false}, last_touch_y{-1}
{
    default_a_min = DEFAULT_A_MIN;
    default_b_min = DEFAULT_B_MIN;
    current_a_min = default_a_min;
    current_b_min = default_b_min;
    status = READY_A;

    // タッチ領域定義
    box_top_A.setupBox(0, 0, 100, 50);       // 左上 A領域
    box_top_B.setupBox(220, 0, 100, 50);     // 右上 B領域
    box_center.setupBox(50, 50, 220, 140);   // 中央（顔エリア）
}

void PomodoroMod::init(void) {
    status = READY_A;
    current_a_min = default_a_min;
    current_b_min = default_b_min;
    avatar.setSpeechText(""); // 吹き出し非表示
    drawHeaderUI();
}

void PomodoroMod::pause(void) {
    avatar.setSpeechText("");
    if (status == RUNNING_A || status == RUNNING_B) {
        status = (status == RUNNING_A) ? PAUSED_A : PAUSED_B;
    }
}

void PomodoroMod::btnA_pressed(void) {
    sw_tone();
    // ボタンA: スタート / 一時停止のトグル
    display_touched(160, 120);
}

void PomodoroMod::btnB_pressed(void) {
    sw_tone();
}

void PomodoroMod::btnC_pressed(void) {
    sw_tone();
    isSilentMode = !isSilentMode;
}

// 60分割 円形ドーナツインジケーター描画
void PomodoroMod::drawCircleGauge(float ratio, uint16_t color) {
    int cx = M5.Display.width() / 2;   // 160
    int cy = M5.Display.height() / 2;  // 120
    int r_outer = 118;
    int r_inner = 108;

    int active_bars = (int)(60.0f * ratio);

    for (int i = 0; i < 60; i++) {
        // -90度（頂点）から時計回りに配置
        float angle = (-90.0f + i * 6.0f) * DEG_TO_RAD;
        uint16_t c = (i < active_bars) ? color : COLOR_GRAY;

        int x1 = cx + cos(angle) * r_inner;
        int y1 = cy + sin(angle) * r_inner;
        int x2 = cx + cos(angle) * r_outer;
        int y2 = cy + sin(angle) * r_outer;

        M5.Display.drawLine(x1, y1, x2, y2, c);
    }
}

// 上部テキスト描画 (A:25, B 等)
void PomodoroMod::drawHeaderUI(void) {
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);

    // 左上 A 表示計算
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

    // 右上 B 表示計算
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

    // クリアして描画
    M5.Display.fillRect(0, 0, 100, 30, TFT_BLACK);
    M5.Display.fillRect(220, 0, 100, 30, TFT_BLACK);

    M5.Display.drawString(textA.c_str(), 10, 5);
    M5.Display.drawString(textB.c_str(), 230, 5);
}

// LEDのゆらぎ点滅（テーマカラーと同色）
void PomodoroMod::updateBreathingLED(uint16_t themeColor) {
    if (!robot) return;

    uint8_t r = ((themeColor >> 11) & 0x1F) * 8;
    uint8_t g = ((themeColor >> 5) & 0x3F) * 4;
    uint8_t b = (themeColor & 0x1F) * 8;

    float brightness = (sin(millis() / 400.0f) + 1.0f) / 2.0f * 0.8f + 0.2f;
    robot->setLedColor(r * brightness, g * brightness, b * brightness);
}

// タイムアップ時演出（首を上に5度 ＋ LED白点滅 2回）
void PomodoroMod::triggerNotification(void) {
    // 首を上に5度上げる
    if (robot && robot->getServo()) {
        robot->getServo()->moveTo(0, -5);
    }

    // 白点滅 2回 (alarm_tone / LedControllerを利用)
    alarm_tone();
    M5.Display.fillScreen(TFT_WHITE);
    delay(150);
    M5.Display.fillScreen(TFT_BLACK);
    delay(150);

    alarm_tone();
    M5.Display.fillScreen(TFT_WHITE);
    delay(150);
    M5.Display.fillScreen(TFT_BLACK);
    delay(150);

    delay(2500); // 演出保持

    // 通常位置へ戻す
    if (robot && robot->getServo()) {
        robot->getServo()->moveTo(0, 0);
    }
}

void PomodoroMod::display_touched(int16_t x, int16_t y) {
    // 1. 上部A表示タップ：停止 ＆ Aデフォルト(25分)へリセット
    if (box_top_A.contain(x, y)) {
        sw_tone();
        current_a_min = default_a_min;
        status = READY_A;
        drawHeaderUI();
        return;
    }

    // 2. 上部B表示タップ：停止 ＆ Bデフォルト(5分)へリセット
    if (box_top_B.contain(x, y)) {
        sw_tone();
        current_b_min = default_b_min;
        status = READY_B;
        drawHeaderUI();
        return;
    }

    // 3. 画面中央タップ：スタート / 一時停止
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
        drawHeaderUI();
        return;
    }

    // 4. 画面両端スライド：停止中または一時停止中のみカスタム時間変更が可能
    if (status == READY_A || status == PAUSED_A || status == READY_B || status == PAUSED_B) {
        if (x < 40 || x > 280) {
            auto touch_detail = M5.Touch.getDetail();
            if (touch_detail.isPressed()) {
                if (last_touch_y >= 0) {
                    int16_t dy = touch_detail.y - last_touch_y;
                    if (abs(dy) > 15) { // 閾値を超えたら変更
                        if (status == READY_A || status == PAUSED_A) {
                            if (dy < 0 && current_a_min < 99) current_a_min++; // 上スライド：増加
                            else if (dy > 0 && current_a_min > 1) current_a_min--; // 下スライド：減少
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
                        drawHeaderUI();
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

    // 1. LEDゆらぎ点滅更新 (動作中のみ)
    if (status == RUNNING_A || status == RUNNING_B) {
        updateBreathingLED(theme_color);
    }

    // 2. タイマー進行・判定処理
    if (status == RUNNING_A || status == RUNNING_B) {
        uint32_t elapsed = millis() - start_time_ms;

        if (elapsed >= total_duration_ms) {
            // タイムアップ処理
            triggerNotification();

            // 1サイクル完了後は自動的に初期値へリセット＆モード交代
            if (status == RUNNING_A) {
                current_a_min = default_a_min; // A初期値(25分)へ復元
                status = READY_B;              // 自動的にBモード準備へ
            } else {
                current_b_min = default_b_min; // B初期値(5分)へ復元
                status = READY_A;              // 自動的にAモード準備へ
            }
            drawHeaderUI();
            drawCircleGauge(1.0f, (status == READY_A) ? COLOR_ORANGE : COLOR_TEAL);
        } else {
            // ゲージ＆UI描画更新
            float ratio = 1.0f - ((float)elapsed / (float)total_duration_ms);
            drawCircleGauge(ratio, theme_color);
            drawHeaderUI();
        }
    } else if (status == PAUSED_A || status == PAUSED_B) {
        float ratio = (float)paused_remaining_ms / (float)total_duration_ms;
        drawCircleGauge(ratio, theme_color);
        drawHeaderUI();
    } else {
        // READY 状態
        drawCircleGauge(1.0f, theme_color);
        drawHeaderUI();
    }
}