#include <M5Unified.h>
#include "stack_chan_led.h"

#define PY32_I2C_ADDR 0x6F

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

// --- I2C ヘルパー関数 ---
static bool writeReg8(uint8_t reg, uint8_t val) {
    bool res = M5.In_I2C.writeRegister8(PY32_I2C_ADDR, reg, val, 400000);
    if (!res) {
        Serial.printf("[LED_ERR] writeReg8 failed! reg: 0x%02X, val: 0x%02X\n", reg, val);
    }
    return res;
}

static uint8_t readReg8(uint8_t reg) {
    return M5.In_I2C.readRegister8(PY32_I2C_ADDR, reg, 400000);
}

static void bitOn(uint8_t reg, uint8_t mask) {
    writeReg8(reg, readReg8(reg) | mask);
}

static void bitOff(uint8_t reg, uint8_t mask) {
    writeReg8(reg, readReg8(reg) & ~mask);
}

StackChanLED::StackChanLED()
    : _currentState(LedState::STANDBY),
      _currentEmotion(LedEmotion::NORMAL),
      _lastUpdate(0),
      _flickerPhase(0.0f),
      _baseR(255), _baseG(60), _baseB(0) {}

void StackChanLED::begin() {
    Serial.println("[LED] --- Initializing PY32 LED Controller ---");
    
    // I2Cの導通テスト（レジスタ0x00の読み出し）
    uint8_t testVal = readReg8(0x00);
    Serial.printf("[LED] PY32 Ping Test (Reg 0x00): 0x%02X\n", testVal);

    // 1. GPIO 13 (LED信号駆動ピン) の初期化設定
    uint8_t pin13_mask = (1 << 5);     // ピン13は High Byte の Bit 5
    bitOn(REG_GPIO_M_H, pin13_mask);   // Direction = Output
    bitOff(REG_GPIO_PD_H, pin13_mask); // Pull-down OFF
    bitOn(REG_GPIO_PU_H, pin13_mask);  // Pull-up ON
    bitOff(REG_GPIO_DRV_H, pin13_mask);// DriveMode = Push-pull

    // 2. LEDの個数を12個に設定
    writeReg8(REG_LED_CFG, 12);

    delay(200);

    setEmotion(LedEmotion::NORMAL);
    Serial.println("[LED] --- PY32 Initialization Done ---");
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
    if (!s_firstSend && s_lastR == r && s_lastG == g && s_lastB == b) {
        return;
    }

    // 1. RGB888 -> RGB565 (16bit) 変換
    uint16_t color565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);

    // 2. 12個分 (24バイト) の RAM データ
    uint8_t ramData[24];
    for (int i = 0; i < 12; i++) {
        ramData[i * 2 + 0] = color565 & 0xFF;        // Low Byte
        ramData[i * 2 + 1] = (color565 >> 8) & 0xFF; // High Byte
    }

    // 3. REG_LED_RAM (0x30) へデータ送信
    if (M5.In_I2C.writeRegister(PY32_I2C_ADDR, REG_LED_RAM, ramData, sizeof(ramData), 400000)) {
        // 4. リフレッシュトリガー (REG_LED_CFG の Bit 6 を 1 にセット)
        uint8_t cfg = readReg8(REG_LED_CFG);
        writeReg8(REG_LED_CFG, cfg | (1 << 6));

        s_lastR = r;
        s_lastG = g;
        s_lastB = b;
        s_firstSend = false;
    } else {
        Serial.printf("[LED_ERR] RAM Write Failed (Addr: 0x%02X)\n", PY32_I2C_ADDR);
    }
}