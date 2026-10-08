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

// オーバーレイ描画エントリーポイント（Avatarの描画ループ内から呼ばれる）
void PomodoroOverlay::draw(M5Canvas *canvas, m5avatar::BoundingRect rect) {
    if (mod) {
        mod->drawOverlayUI(canvas);
    }
}

PomodoroMod::PomodoroMod(bool _isOffline)
  : isOffline{_isOffline}, isSilentMode{true}, is_sliding{false}, last_touch_y{-1},
    overlay(this), is_active(false)
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
    is_active = true;

    avatar.setSpeechText("");   // 吹き出しクリア
    avatar.addEffect(&overlay); // Avatarのエフェクト（重ね描画）レイヤーに追加
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

// Avatarのキャンバス（背景顔描画完了後）に対する前面オーバーレイ描画
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

    // --- 1. ポンデライオン風 円形インジケーター描画 (外周 r=104〜118) ---
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

    // --- 2. 上部ヘッダーUI描画 (A:25, B:5) ---
    canvas->setTextSize(2);
    canvas->setTextColor(TFT_WHITE, TFT_BLACK);

    // 左上 A 表示文字列作成
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

    // 右上 B 表示文字列作成
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

    // 黒背景で文字ボックスを軽くクリアしてから上書き
    canvas->fillRect(0, 0, 90, 24, TFT_BLACK);
    canvas->fillRect(230, 0, 90, 24, TFT_BLACK);

    canvas->drawString(textA.c_str(), 5, 4);
    canvas->drawString(textB.c_str(), 235, 4);
}

// LEDのゆらぎ点滅
void PomodoroMod::updateBreathingLED(uint16_t themeColor) {
    if (!robot) return;

    uint8_t r = ((themeColor >> 11) & 0x1F) * 8;
    uint8_t g = ((themeColor >> 5) & 0x3F) * 4;
    uint8_t b = (themeColor & 0x1F) * 8;

    float brightness = (sin(millis() / 400.0f) + 1.0f) / 2.0f * 0.8f + 0.2f;
}

// タイムアップ時演出（首を上に5度 ＋ アラーム音）
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
    // 1. 上部A表示タップ：リセット
    if (box_top_A.contain(x, y)) {
        sw_tone();
        current_a_min = default_a_min;
        status = READY_A;
        return;
    }

    // 2. 上部B表示タップ：リセット
    if (box_top_B.contain(x, y)) {
        sw_tone();
        current_b_min = default_b_min;
        status = READY_B;
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
        return;
    }

    // 4. 画面両端スライド：時間変更
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

    // LED点滅更新
    if (status == RUNNING_A || status == RUNNING_B) {
        updateBreathingLED(theme_color);
    }

    // タイマー進行・判定処理（描画呼び出しは一切行わない）
    if (status == RUNNING_A || status == RUNNING_B) {
        uint32_t elapsed = millis() - start_time_ms;

        if (elapsed >= total_duration_ms) {
            // タイムアップ処理
            triggerNotification();

            // サイクル完了後の切り替え
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