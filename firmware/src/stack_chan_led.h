#ifndef STACK_CHAN_LED_H
#endif
#pragma once

#include <Arduino.h>
#include <Wire.h>

// LEDの状態定義
enum class LedState {
    STANDBY,    // 待機中（1/f ゆらぎ発光）
    LISTENING,  // 聞き取り中（緑点灯）
    THINKING,   // 返答準備中（青点滅）
    SPEAKING    // 発話中（青点亮）
};

// 待機時の感情定義（色変更用）
enum class LedEmotion {
    NORMAL,  // オレンジ (RGB: 255, 60, 0)
    HAPPY,   // 黄緑/黄色 (RGB: 255, 180, 0)
    ANGRY,   // 赤 (RGB: 255, 0, 0)
    SAD,     // 水色/青紫 (RGB: 0, 100, 255)
    DOUBT,    // 紫 (RGB: 180, 0, 255)
    SLEEPY   // 紺 (RGB: 75, 0, 130)
};

class StackChanLED {
public:
    StackChanLED();

    // 初期化
    void begin();

    // メインループで毎フレーム呼び出す更新処理
    void update();

    // 状態の切り替え
    void setState(LedState state);

    // 感情の切り替え（STANDBY時のベース色を変更）
    void setEmotion(LedEmotion emotion);

    // カスタムカラーの設定（感情以外の直接指定用）
    void setCustomStandbyColor(uint8_t r, uint8_t g, uint8_t b);

private:
    LedState _currentState;
    LedEmotion _currentEmotion;

    uint32_t _lastUpdate;
    float _flickerPhase;

    // 待機時のベース色 (RGB)
    uint8_t _baseR, _baseG, _baseB;

    // ハードウェア送信（CoreS3 PY32 0x6F 宛てI2C送信）
    void writeHardwareLED(uint8_t r, uint8_t g, uint8_t b);

    // 1/fゆらぎの輝度計算 (0.2 ～ 1.0 の倍率を返す)
    float calculate1OverFFlicker();
};

extern StackChanLED LedController;