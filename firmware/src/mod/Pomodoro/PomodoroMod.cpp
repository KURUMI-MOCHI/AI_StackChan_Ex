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

// 画面外周に沿ったコの字型パスの定義（右下1分 -> 右上角 -> 左上角 -> 左下60分）
struct Point { float x; float y; };
static const Point PATH_POINTS[] = {
    {319.0f, 180.0f}, // 1分 (右下端)
    {319.0f,   0.0f}, // 右上角 (直角)
    {0.0f,     0.0f}, // 左上角 (直角)
    {0.0f,   180.0f}  // 60分 (左下端)
};
static const int NUM_PATH_POINTS = sizeof(PATH_POINTS) / sizeof(PATH_POINTS[0]);

PomodoroMod::PomodoroMod(bool _isOffline)
  : isOffline{_isOffline}, last_update_ms{0}, is_dial_dragging{false},
    headTouchHappyActive{false}, headTouchHappyUntilMs{0}, prev_servo_home_state{true}
{
    default_a_min = DEFAULT_A_MIN;
    default_b_min = DEFAULT_B_MIN;
    current_a_min = default_a_min;
    current_b_min = default_b_min;
    status = READY_A;

    memset(speech_buf, 0, sizeof(speech_buf));
    memset(last_speech_str, 0, sizeof(last_speech_str));

    // タッチエリア設定
    box_center.setupBox(80, 60, 160, 120);    // 顔中央（スタート/一時停止）
    box_balloon.setupBox(120, 180, 80, 45);   // 画面下部中央の狭い吹き出しエリア（A/B切替）
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
    updateLEDState();
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
    if (status == READY_A || status == PAUSED_A) {
        status = READY_B;
    } else if (status == READY_B || status == PAUSED_B) {
        status = READY_A;
    }
    current_a_min = default_a_min;
    current_b_min = default_b_min;

    updateLEDState();
    updateSpeechText();
    update();
}

void PomodoroMod::btnC_pressed(void) {
    LedController.flashFeedback();
}

// 吹き出し文字列の更新 (A:25m... のみ)
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

    snprintf(speech_buf, sizeof(speech_buf), "%s:%lum%s",
             isModeA ? "A" : "B",
             disp_min,
             (status == RUNNING_A || status == RUNNING_B) ? "..." : "");

    if (strcmp(last_speech_str, speech_buf) != 0) {
        strncpy(last_speech_str, speech_buf, sizeof(last_speech_str));
        avatar.setSpeechText(speech_buf);
    }
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

    // --- コの字型パスの総長計算 (合計679px: 右辺180 + 上辺319 + 左辺180) ---
    float total_length = 679.0f;
    float seg_lengths[] = { 180.0f, 319.0f, 180.0f };

    // 斜め45度（右上から左下へ向かう単位ベクトル）
    const float u_x = -0.70710678f;
    const float u_y =  0.70710678f;
    const float line_len = 12.0f; // 目盛りの長さ

    for (int i = 1; i <= active_bars; i++) {
        float target_d = ((float)(i - 1) / 59.0f) * total_length;
        
        float px = 0.0f, py = 0.0f;
        if (target_d <= 180.0f) {
            // 右辺 (319, 180) -> (319, 0)
            px = 319.0f;
            py = 180.0f - target_d;
        } else if (target_d <= 180.0f + 319.0f) {
            // 上辺 (319, 0) -> (0, 0)
            float d_top = target_d - 180.0f;
            px = 319.0f - d_top;
            py = 0.0f;
        } else {
            // 左辺 (0, 0) -> (0, 180)
            float d_left = target_d - (180.0f + 319.0f);
            px = 0.0f;
            py = d_left;
        }

        // 終点座標（すべての目盛りを右上から左下への斜め45度に統一）
        float ex = px + u_x * line_len;
        float ey = py + u_y * line_len;

        // ギザギザ感を解消するため、数本重ね描きして滑らかな太線にする
        for (float offset = -1.0f; offset <= 1.0f; offset += 1.0f) {
            float ox = offset * 0.6f;
            float oy = offset * 0.6f;
            spi->drawLine((int)(px + ox), (int)(py + oy), 
                          (int)(ex + ox), (int)(ey + oy), theme_color);
        }
    }
}

void PomodoroMod::update(void) {
    updateSpeechText();
    avatar.updateSubWindowCustom(PomodoroMod::drawSubWindow, this, 0, 0, 320, 240);
}

// LEDカラー設定（モードA: オレンジ / モードB: ティールグリーン）
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

// なでなで時はLED色を変えず表情とサーボのみ動かす
void PomodoroMod::updateHeadTouchExpression(void) {
    HeadTouchSensor::Gesture gesture = HeadTouchSensor::update();

    if (HeadTouchSensor::isPetGesture(gesture)) {
        if (!headTouchHappyActive) {
            headTouchHappyActive = true;
            prev_servo_home_state = servo_home;
            servo_home = false;

            avatar.setExpression(Expression::Happy);

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

// 外周部（解像度エッジ付近）のタッチ判定
bool PomodoroMod::isRingArea(int16_t x, int16_t y) {
    if (y > 185) return false;
    return (x <= 25 || x >= 295 || y <= 25);
}

// タッチ座標から コの字パス上の最も近い位置を求めて 1〜60分 を算出
uint32_t PomodoroMod::getMinuteFromTouchPos(int16_t x, int16_t y) {
    float total_length = 679.0f;
    float min_dist_sq = 1e9f;
    float best_progress = 0.0f;

    // パスを細かいステップでサンプリングして最も近い位置を探索
    for (int step = 0; step <= 200; step++) {
        float target_d = ((float)step / 200.0f) * total_length;
        float px = 0.0f, py = 0.0f;

        if (target_d <= 180.0f) {
            px = 319.0f;
            py = 180.0f - target_d;
        } else if (target_d <= 180.0f + 319.0f) {
            float d_top = target_d - 180.0f;
            px = 319.0f - d_top;
            py = 0.0f;
        } else {
            float d_left = target_d - (180.0f + 319.0f);
            px = 0.0f;
            py = d_left;
        }

        float dist_sq = (x - px)*(x - px) + (y - py)*(y - py);
        if (dist_sq < min_dist_sq) {
            min_dist_sq = dist_sq;
            best_progress = target_d / total_length;
        }
    }

    int min_val = (int)round(best_progress * 59.0f) + 1;
    if (min_val < 1) min_val = 1;
    if (min_val > MAX_MIN) min_val = MAX_MIN;
    return min_val;
}

// ダイヤルなぞり操作の適用（今のサイクルのみ一時変更）
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
    // 1. 画面下部中央の狭い吹き出しエリアをタップ：A/B モード切り替え
    if (box_balloon.contain(x, y)) {
        LedController.flashFeedback();
        if (status == READY_A || status == PAUSED_A || status == RUNNING_A) {
            status = READY_B;
        } else {
            status = READY_A;
        }
        current_a_min = default_a_min;
        current_b_min = default_b_min;

        updateLEDState();
        updateSpeechText();
        update();
        return;
    }

    // 2. 中央タップ：スタート / 一時停止
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
    updateLEDState();

    // コの字型インジケーターのなぞり判定 (READY/PAUSED 時)
    if (status == READY_A || status == PAUSED_A || status == READY_B || status == PAUSED_B) {
        auto touch_detail = M5.Touch.getDetail();
        if (touch_detail.isPressed()) {
            if (!box_center.contain(touch_detail.x, touch_detail.y) &&
                !box_balloon.contain(touch_detail.x, touch_detail.y)) {
                if (isRingArea(touch_detail.x, touch_detail.y)) {
                    handleDialTouch(touch_detail.x, touch_detail.y);
                }
            }
        }
    }

    // 描画更新間隔を制限 (200ms周期) して画面明滅を防止
    uint32_t now = millis();
    if (now - last_update_ms >= 200) {
        last_update_ms = now;
        update();
    }

    if (status == RUNNING_A || status == RUNNING_B) {
        uint32_t elapsed = now - start_time_ms;

        if (elapsed >= total_duration_ms) {
            triggerNotification();

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