#include <M5Unified.h>
#include "stack_chan_led.h"

// アドレスを動的に保持するため変数化（初期値 0x6F）
static uint8_t g_py32_i2c_addr = 0x6F;

// PY32IOExpander レジスタ定義
static constexpr uint8_t REG_GPIO_M_H   = 0x04;
static constexpr uint8_t REG_GPIO_PU_H  = 0x0A;
static constexpr uint8_t REG_GPIO_PD_H  = 0x0C;
static constexpr uint8_t REG_GPIO_DRV_H = 0x14;
static constexpr uint8_t REG_LED_CFG    = 0x24;
static constexpr uint8_t REG_LED_RAM    = 0x30;

StackChanLED LedController;

static uint8_t s_lastR = 255, s_lastG = 255, s_lastB = 255;
static bool s_firstSend = true;

// --- 軽量化した I2C ヘルパー関数 ---
static inline bool writeReg8(uint8_t reg, uint8_t val) {
    return M5.In_I2C.writeRegister8(g_py32_i2c_addr, reg, val, 100000);
}

static inline uint8_t readReg8(uint8_t reg) {
    return M5.In_I2C.readRegister8(g_py32_i2c_addr, reg, 100000);
}

static void bitOn(uint8_t reg, uint8_t mask) {
    writeReg8(reg, readReg8(reg) | mask);
}

static void bitOff(uint8_t reg, uint8_t mask) {
    writeReg8(reg, readReg8(reg) & ~mask);
}

StackChanLED::StackChanLED()
    : _currentState(LedState::OFF),
      _currentEmotion(LedEmotion::NORMAL),
      _pendingEmotion(LedEmotion::NORMAL),
      _hasPendingEmotion(false),
      _lastUpdate(0),
      _chaosX(0.1f), // カオス変数の初期値（0以外に設定）
      _flashStep(0),
      _flashStepTime(0),
      _baseR(255), _baseG(60), _baseB(0) {}

void StackChanLED::begin() {
    Serial.println("[LED] --- Initializing PY32 LED Controller ---");
    
    // アドレスの自動判定 (0x6F -> 0x71)
    if (M5.In_I2C.writeRegister8(0x6F, 0x00, 0x00, 100000)) {
        g_py32_i2c_addr = 0x6F;
        Serial.println("[LED] Found PY32 at 0x6F");
    } else if (M5.In_I2C.writeRegister8(0x71, 0x00, 0x00, 100000)) {
        g_py32_i2c_addr = 0x71;
        Serial.println("[LED] Found PY32 at 0x71");
    } else {
        Serial.println("[LED_ERR] PY32 LED controller not found on In_I2C!");
        return;
    }

    // 1. GPIO 13 (LED信号駆動ピン) の初期化設定
    uint8_t pin13_mask = (1 << 5);     // ピン13は High Byte の Bit 5
    bitOn(REG_GPIO_M_H, pin13_mask);   // Direction = Output
    bitOff(REG_GPIO_PD_H, pin13_mask); // Pull-down OFF
    bitOn(REG_GPIO_PU_H, pin13_mask);  // Pull-up ON
    bitOff(REG_GPIO_DRV_H, pin13_mask);// DriveMode = Push-pull

    // 2. LEDの個数を12個に設定
    writeReg8(REG_LED_CFG, 12);

    delay(100);

    // 起動直後は前回の残色を消すため、強制的に全消灯
    writeHardwareLED(0, 0, 0);
    _currentState = LedState::OFF;

    Serial.println("[LED] --- PY32 Initialization Done ---");
}

void StackChanLED::setState(LedState state) {
    _currentState = state;
}

// 外部から呼ばれた時はPendingフラグを立ててキューイングする
void StackChanLED::setEmotion(LedEmotion emotion) {
    if (_currentEmotion != emotion) {
        _pendingEmotion = emotion;
        _hasPendingEmotion = true;
    }
}

// 内部で実際にベースカラーを書き換える処理
void StackChanLED::applyEmotion(LedEmotion emotion) {
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

// 間欠性カオスによる1/fゆらぎ輝度計算
float StackChanLED::calculateIntermittentChaos() {
    float x = _chaosX;

    // 1次元非線形写像 (Pomeau-Manneville型)
    // x < 0.5 の領域では緩やかに増加（層流：静かな炎）
    // x >= 0.5 の領域で引き戻され急速に変化（乱流：風によるチラつき）
    if (x < 0.5f) {
        x = x + 2.0f * x * x;
    } else {
        x = x - 2.0f * (1.0f - x) * (1.0f - x);
    }

    // 端点 (0.0, 1.0) への落とし込み・固定化を防ぐ摂動処理
    if (x <= 0.001f) {
        x = 0.001f + (static_cast<float>(random(1, 100)) / 10000.0f);
    } else if (x >= 0.999f) {
        x = 0.999f - (static_cast<float>(random(1, 100)) / 10000.0f);
    }

    _chaosX = x;

    // カオス変数 x (0.0 ~ 1.0) を LED の輝度係数 (0.25 ~ 1.00) にスケーリング
    float brightness = 0.25f + (_chaosX * 0.75f);

    return constrain(brightness, 0.15f, 1.0f);
}

// タッチ検出時のフィードバック（delayを使わない完全ノンブロッキング化）
void StackChanLED::flashFeedback() {
    _flashStep = 1;
    _flashStepTime = millis();
    writeHardwareLED(255, 255, 255); // 1回目点灯
}

void StackChanLED::update() {
    uint32_t now = millis();

    // --- 0. 感情のPending変更があれば適用 ---
    if (_hasPendingEmotion) {
        _hasPendingEmotion = false;
        applyEmotion(_pendingEmotion);
        s_firstSend = true; // 色変更時は通信ガードを解除して即時反映を許可
    }

    // --- 1. ノンブロッキング・フラッシュ点滅処理の更新 ---
    if (_flashStep > 0) {
        uint32_t elapsed = now - _flashStepTime;
        if (_flashStep == 1 && elapsed >= 120) { // 120ms経過で消灯
            _flashStep = 2;
            _flashStepTime = now;
            writeHardwareLED(0, 0, 0);
        } else if (_flashStep == 2 && elapsed >= 80) { // 80ms消灯後、2回目点灯
            _flashStep = 3;
            _flashStepTime = now;
            writeHardwareLED(255, 255, 255);
        } else if (_flashStep == 3 && elapsed >= 120) { // 120ms経過でフラッシュ終了
            _flashStep = 0;
            s_firstSend = true; // 通常描画へ即時復帰
        }
        return; // フラッシュ実行中は通常LEDアニメーションをスキップ
    }

    // --- 2. 通常のLED状態更新 (50ms周期) ---
    if (now - _lastUpdate < 50) {
        return;
    }
    _lastUpdate = now;

    switch (_currentState) {
        case LedState::OFF:
            writeHardwareLED(0, 0, 0);
            break;

        case LedState::STANDBY: {
            float factor = calculateIntermittentChaos();
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
            float pulse = (sinf(now * 0.008f) + 1.0f) * 0.5f;
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
    // 前回と同じ色なら通信しない（通信削減ガード）
    if (!s_firstSend && s_lastR == r && s_lastG == g && s_lastB == b) {
        return;
    }

    // 1. RGB888 -> RGB565 (16bit) 変換
    uint16_t color565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);

    // 2. 12個分 (24バイト) の RAM データ生成
    uint8_t ramData[24];
    for (int i = 0; i < 12; i++) {
        ramData[i * 2 + 0] = color565 & 0xFF;        // Low Byte
        ramData[i * 2 + 1] = (color565 >> 8) & 0xFF; // High Byte
    }

    // 3. REG_LED_RAM (0x30) へデータ一括送信 (100kHz)
    if (M5.In_I2C.writeRegister(g_py32_i2c_addr, REG_LED_RAM, ramData, sizeof(ramData), 100000)) {
        
        // LED数12個 (0x0C) + リフレッシュビット (0x40) = 0x4C を直接書き込み
        writeReg8(REG_LED_CFG, 0x4C);

        s_lastR = r;
        s_lastG = g;
        s_lastB = b;
        s_firstSend = false;
    } else {
        Serial.printf("[LED_ERR] RAM Write Failed (Addr: 0x%02X)\n", g_py32_i2c_addr);
    }
}