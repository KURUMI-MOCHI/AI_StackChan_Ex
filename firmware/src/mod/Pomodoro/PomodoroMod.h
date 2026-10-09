#ifndef _POMODORO_MOD_H
#define _POMODORO_MOD_H

#include <Arduino.h>
#include "mod/ModBase.h"
#include <Avatar.h>
#include <M5GFX.h>
#include "driver/HeadTouchSensor.h"
#include "stack_chan_led.h"

#define COLOR_ORANGE   0xFDA0  // モードA (オレンジ)
#define COLOR_TEAL     0x0410  // モードB (ティールグリーン)

#define DEFAULT_A_MIN 25
#define DEFAULT_B_MIN 5
#define MAX_MIN       60  // インジケーター上限 (60分)

typedef enum {
    READY_A,
    RUNNING_A,
    PAUSED_A,
    READY_B,
    RUNNING_B,
    PAUSED_B
} PomodoroStatus;

class PomodoroMod : public ModBase {
private:
    box_t box_center;
    box_t box_balloon; // 画面下部中央の狭い吹き出しエリア（モード切替用）

    PomodoroStatus status;
    bool isOffline;

    uint32_t default_a_min;
    uint32_t default_b_min;
    uint32_t current_a_min;
    uint32_t current_b_min;

    uint32_t start_time_ms;
    uint32_t total_duration_ms;
    uint32_t paused_remaining_ms;

    // 描画・吹き出し更新用変数
    char speech_buf[64];
    char last_speech_str[64];
    uint32_t last_update_ms;

    // ダイヤル操作判定用
    bool is_dial_dragging;

    // 頭頂部なでなでリアクション用変数
    bool headTouchHappyActive;
    uint32_t headTouchHappyUntilMs;
    bool prev_servo_home_state;

public:
    PomodoroMod(bool _isOffline = false);

    void init(void) override;
    void pause(void) override;
    void btnA_pressed(void) override;
    void btnB_pressed(void) override;
    void btnC_pressed(void) override;
    void display_touched(int16_t x, int16_t y) override;
    void idle(void) override;

    void update(void);
    void drawPomodoroUI(M5Canvas *spi, m5avatar::BoundingRect rect, m5avatar::DrawContext *ctx);

private:
    void triggerNotification(void);
    void updateHeadTouchExpression(void);
    void updateLEDState(void);
    void updateSpeechText(void);
    bool isRingArea(int16_t x, int16_t y);
    void handleDialTouch(int16_t x, int16_t y);
    uint32_t getMinuteFromTouchPos(int16_t x, int16_t y);
    static void drawSubWindow(M5Canvas *spi, m5avatar::BoundingRect rect, m5avatar::DrawContext *ctx, void *userData);
};

#endif // _POMODORO_MOD_H