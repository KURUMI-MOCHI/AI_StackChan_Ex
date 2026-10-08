#ifndef _POMODORO_MOD_H
#define _POMODORO_MOD_H

#include <Arduino.h>
#include "mod/ModBase.h"

// テーマカラー定義 (RGB565)
#define COLOR_ORANGE  0xFDA0  // モードA (オレンジ)
#define COLOR_TEAL    0x0410  // モードB (ティール)
#define COLOR_GRAY    0x39E7  // 消滅後・背景用グレー

#define DEFAULT_A_MIN 25
#define DEFAULT_B_MIN 5

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
    box_t box_top_A;  // 左上 (Aモード設定・リセット)
    box_t box_top_B;  // 右上 (Bモード設定・リセット)
    box_t box_center; // 画面中央 (スタート / 一時停止)

    PomodoroStatus status;
    bool isOffline;
    bool isSilentMode;

    // 時間設定 (分)
    uint32_t default_a_min;
    uint32_t default_b_min;
    uint32_t current_a_min; // 今サイクルの設定時間
    uint32_t current_b_min;

    // タイマー計測用
    uint32_t start_time_ms;
    uint32_t total_duration_ms;
    uint32_t paused_remaining_ms; // 一時停止時の残り時間

    // スライド操作用
    int16_t last_touch_y;
    bool is_sliding;

public:
    PomodoroMod(bool _isOffline = false);

    void init(void);
    void pause(void);
    void btnA_pressed(void);
    void btnB_pressed(void);
    void btnC_pressed(void);
    void display_touched(int16_t x, int16_t y);
    void idle(void);

private:
    void triggerNotification(void);
    void drawCircleGauge(float ratio, uint16_t color);
    void drawHeaderUI(void);
    void updateBreathingLED(uint16_t themeColor);
};

#endif // _POMODORO_MOD_H