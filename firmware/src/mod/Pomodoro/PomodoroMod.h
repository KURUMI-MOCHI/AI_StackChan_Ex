#ifndef _POMODORO_MOD_H
#define _POMODORO_MOD_H

#include <Arduino.h>
#include "mod/ModBase.h"
#include <Avatar.h>

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

class PomodoroMod;

// Avatarの描画キャンバス上に重ね描画するためのオーバーレイ描画クラス
class PomodoroOverlay : public m5avatar::Drawable {
private:
    PomodoroMod* mod;
public:
    PomodoroOverlay(PomodoroMod* _mod) : mod(_mod) {}
    void draw(m5avatar::M5Canvas *canvas, m5avatar::BoundingRect rect) override;
};

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
    uint32_t current_a_min;
    uint32_t current_b_min;

    // タイマー計測用
    uint32_t start_time_ms;
    uint32_t total_duration_ms;
    uint32_t paused_remaining_ms;

    // スライド操作用
    int16_t last_touch_y;
    bool is_sliding;

    // オーバーレイ描画管理
    PomodoroOverlay overlay;
    bool is_active;

public:
    PomodoroMod(bool _isOffline = false);

    void init(void) override;
    void pause(void) override;
    void btnA_pressed(void) override;
    void btnB_pressed(void) override;
    void btnC_pressed(void) override;
    void display_touched(int16_t x, int16_t y) override;
    void idle(void) override;

    // Avatarキャンバスへのオーバーレイ描画用関数
    void drawOverlayUI(m5avatar::M5Canvas *canvas);

private:
    void triggerNotification(void);
    void updateBreathingLED(uint16_t themeColor);
};

#endif // _POMODORO_MOD_H