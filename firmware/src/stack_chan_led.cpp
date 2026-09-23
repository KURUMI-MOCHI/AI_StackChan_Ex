#include <M5Unified.h>  // ★ これを追加
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
    // ★重要: Wire.begin() や Wire1.begin() は絶対に呼ばない（M5.begin() が管理しているため）
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
        case LedEmotion::SLEEPY: // 紫・紺系
            _baseR = 80; _baseG = 0; _baseB = 180;
            break;
    }
}

void StackChanLED::setCustomStandbyColor(uint8_t r, uint8_t g, uint8_t b) {
    _baseR = r;
    _baseG = g;
    _baseB = b;
}

float StackChanLED::calculate1OverFFlicker() {
    _flickerPhase += 0.05f;
    if (_flickerPhase > 628.3f) _flickerPhase = 0.0f;
    float wave1 = sin(_flickerPhase);
    float wave2 = sin(_flickerPhase * 2.3f) * 0.5f;
    float wave3 = sin(_flickerPhase * 5.7f) * 0.25f;
    float norm = (wave1 + wave2 + wave3) / 1.75f;
    float brightness = 0.6f + (norm * 0.4f);
    return constrain(brightness, 0.1f, 1.0f);
}

void StackChanLED::update() {
    uint32_t now = millis();
    if (now - _lastUpdate < 20) {
        return;
    }
    _lastUpdate = now;

    switch (_currentState) {
        case LedState::STANDBY: {
            float factor = calculate1OverFFlicker();
            uint8_t r = static_cast<uint8_t>(_baseR * factor);
            uint8_t g = static_cast<uint8_t>(_baseG * factor);
            uint8_t b = static_cast<uint8_t>(_baseB * factor);
            writeHardwareLED(r, g, b);
            break;
        }
        case LedState::LISTENING:
            writeHardwareLED(0, 255, 0);
            break;
        case LedState::THINKING: {
            float pulse = (sin(now * 0.008f) + 1.0f) / 2.0f;
            uint8_t b = static_cast<uint8_t>(255 * pulse);
            writeHardwareLED(0, 0, b);
            break;
        }
        case LedState::SPEAKING:
            writeHardwareLED(0, 0, 255);
            break;
    }
}

// M5CoreS3 の内部 I2C (M5.In_I2C) を使用して安全に書き込む
void StackChanLED::writeHardwareLED(uint8_t r, uint8_t g, uint8_t b) {
    uint8_t ledData[36];
    for (int i = 0; i < 12; i++) {
        ledData[i * 3 + 0] = r;
        ledData[i * 3 + 1] = g;
        ledData[i * 3 + 2] = b;
    }
    // M5Unifiedの内部I2C制御(M5.In_I2C)を使ってバッファ上限を回避しつつ送信
    M5.In_I2C.writeRegister(PY32_I2C_ADDR, 0x00, ledData, sizeof(ledData), 400000);
}