#pragma once

#include <Arduino.h>
#include <Wire.h>

// LEDの状態定義
enum class LedState {
    OFF,        // 消灯状態
    STANDBY,    // 待機中（1/f ゆらぎ発光）
    LISTENING,  // 聞き取り中（緑点灯）
    THINKING,   // 返答準備中（青点滅）
    SPEAKING    // 発話中（青点亮）
};

// 待機時の感情定義（色変更用）
enum class LedEmotion {
    NORMAL,  // オレンジ (RGB: 255, 60, 0)
    HAPPY,   // 黄色 (RGB: 255, 160, 0)
    ANGRY,   // 赤 (RGB: 255, 0, 0)
    SAD,     // 青 (RGB: 0, 100, 255)
    DOUBT,   // 紫 (RGB: 150, 0, 255)
    SLEEPY   // 紺 (RGB: 80, 0, 180)
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

    // カスタムカラーの設定
    void setCustomStandbyColor(uint8_t r, uint8_t g, uint8_t b);

    // タッチ時などのフラッシュフィードバック（ノンブロッキング）
    void flashFeedback();

    // 現在のLED状態を取得
    LedState getState() const { return _currentState; }

private:
    LedState _currentState;
    LedEmotion _currentEmotion;

    uint32_t _lastUpdate;
    float _flickerPhase;

    // フラッシュフィードバック用ステート変数（ノンブロッキング用）
    uint8_t _flashStep;        // 0:なし, 1:1回目白, 2:消灯, 3:2回目白
    uint32_t _flashStepTime;   // ステップ切り替え時刻

    // 待機時のベース色 (RGB)
    uint8_t _baseR, _baseG, _baseB;

    // ハードウェア送信（CoreS3 PY32 0x6F 宛てI2C送信）
    void writeHardwareLED(uint8_t r, uint8_t g, uint8_t b);

    // 1/fゆらぎの輝度計算
    float calculate1OverFFlicker();
};

extern StackChanLED LedController;