#include "stack_chan_led.h"

#define PY32_I2C_ADDR 0x6F

StackChanLED LedController;

StackChanLED::StackChanLED()
    : _currentState(LedState::STANDBY),
      _currentEmotion(LedEmotion::NORMAL),
      _lastUpdate(0),
      _flickerPhase(0.0f),
      _baseR(255), _baseG(60), _baseB(0) {}

void StackChanLED::begin() {
    Wire.begin();
    setEmotion(LedEmotion::NORMAL);
}

void StackChanLED::setState(LedState state) {
    _currentState = state;
}

void StackChanLED::setEmotion(LedEmotion emotion) {
    _currentEmotion = emotion;
    switch (emotion) {
        case LedEmotion::NORMAL: // オレンジ
            _baseR = 255; _baseG = 60; _baseB = 0;
            break;
        case LedEmotion::HAPPY:  // 黄色
            _baseR = 255; _baseG = 160; _baseB = 0;
            break;
        case LedEmotion::ANGRY:  // 赤
            _baseR = 255; _baseG = 0; _baseB = 0;
            break;
        case LedEmotion::SAD:    // 青
            _baseR = 0; _baseG = 100; _baseB = 255;
            break;
        case LedEmotion::DOUBT:  // 紫
            _baseR = 150; _baseG = 0; _baseB = 255;
            break;
        case LedEmotion::SLEEPY:  // 紺
            _baseR = 75; _baseG = 0; _baseB = 130;
            break;
    }
}

void StackChanLED::setCustomStandbyColor(uint8_t r, uint8_t g, uint8_t b) {
    _baseR = r;
    _baseG = g;
    _baseB = b;
}

// 複数周波数の波形を重ね合わせて1/fゆらぎ輝度(0.2~1.0)を作る
float StackChanLED::calculate1OverFFlicker() {
    _flickerPhase += 0.05f;
    if (_flickerPhase > 628.3f) _flickerPhase = 0.0f; // 100 * 2PIでリセット

    float wave1 = sin(_flickerPhase);
    float wave2 = sin(_flickerPhase * 2.3f) * 0.5f;
    float wave3 = sin(_flickerPhase * 5.7f) * 0.25f;

    float norm = (wave1 + wave2 + wave3) / 1.75f; // -1.0 ~ 1.0 に正規化
    float brightness = 0.6f + (norm * 0.4f);      // 0.2 ~ 1.0 の範囲に収める

    return constrain(brightness, 0.1f, 1.0f);
}

void StackChanLED::update() {
    uint32_t now = millis();
    // 20ms（50Hz）間隔で更新
    if (now - _lastUpdate < 20) {
        return;
    }
    _lastUpdate = now;

    switch (_currentState) {
        case LedState::STANDBY: {
            // ① 待機中：1/f ゆらぎで発光
            float factor = calculate1OverFFlicker();
            uint8_t r = static_cast<uint8_t>(_baseR * factor);
            uint8_t g = static_cast<uint8_t>(_baseG * factor);
            uint8_t b = static_cast<uint8_t>(_baseB * factor);
            writeHardwareLED(r, g, b);
            break;
        }

        case LedState::LISTENING: {
            // ②-1 AI聞き取り中：緑色固定点灯
            writeHardwareLED(0, 255, 0);
            break;
        }

        case LedState::THINKING: {
            // ②-2 AI返答準備中：青色点滅（約500ms周期で明暗パルス）
            float pulse = (sin(now * 0.008f) + 1.0f) / 2.0f; // 0.0 ~ 1.0
            uint8_t b = static_cast<uint8_t>(255 * pulse);
            writeHardwareLED(0, 0, b);
            break;
        }

        case LedState::SPEAKING: {
            // ②-3 AI発話中：青色固定点灯
            writeHardwareLED(0, 0, 255);
            break;
        }
    }
}

// 本体上部LED（PY32アドレス0x6F）への12個一括RGB送信
void StackChanLED::writeHardwareLED(uint8_t r, uint8_t g, uint8_t b) {
    Wire.beginTransmission(PY32_I2C_ADDR);
    Wire.write(0x00); // レジスタ先頭アドレス
    for (int i = 0; i < 12; i++) {
        Wire.write(r);
        Wire.write(g);
        Wire.write(b);
    }
    Wire.endTransmission();
}