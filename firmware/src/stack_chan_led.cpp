#include <M5Unified.h>
#include "stack_chan_led.h"

#define PY32_I2C_ADDR 0x6F

StackChanLED LedController;

// 前回の送信値を保持して無駄なI2C通信を防ぐ
static uint8_t s_lastR = 255, s_lastG = 255, s_lastB = 255;
static bool s_firstSend = true;

StackChanLED::StackChanLED()
    : _currentState(LedState::STANDBY),
      _currentEmotion(LedEmotion::NORMAL),
      _lastUpdate(0),
      _flickerPhase(0.0f),
      _baseR(255), _baseG(60), _baseB(0) {}

void StackChanLED::begin() {
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
        case LedEmotion::SLEEPY: // 紺系
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
    // 音声処理タスク（I2C）との衝突を防ぐため、書き込み間隔を50ms（毎秒20回）に緩和
    if (now - _lastUpdate < 50) {
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
            writeHardwareLED(0, 255, 0); // 緑
            break;
        case LedState::THINKING: {
            float pulse = (sin(now * 0.008f) + 1.0f) / 2.0f;
            uint8_t b = static_cast<uint8_t>(255 * pulse);
            writeHardwareLED(0, 0, b);
            break;
        }
        case LedState::SPEAKING:
            writeHardwareLED(0, 0, 255); // 青
            break;
    }
}

void StackChanLED::writeHardwareLED(uint8_t r, uint8_t g, uint8_t b) {
    // 初回送信が成功済みで、かつ値が変わっていない場合はスキップ
    if (!s_firstSend && s_lastR == r && s_lastG == g && s_lastB == b) {
        return;
    }

    uint8_t ledData[36];
    for (int i = 0; i < 12; i++) {
        ledData[i * 3 + 0] = r;
        ledData[i * 3 + 1] = g;
        ledData[i * 3 + 2] = b;
    }

    // 書き込みが成功した場合のみ送信済みフラグを立てる
    if (M5.In_I2C.writeRegister(PY32_I2C_ADDR, 0x00, ledData, sizeof(ledData), 400000)) {
        s_lastR = r;
        s_lastG = g;
        s_lastB = b;
        s_firstSend = false; // 通信成功！
    }
}