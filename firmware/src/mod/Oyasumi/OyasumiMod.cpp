#include "OyasumiMod.h"
#include <Avatar.h>
#include "Robot.h"
#include "driver/HeadTouchSensor.h"

extern Avatar avatar;

OyasumiMod::OyasumiMod()
    : state{SleepState::WAIT_STROKE}, sleepTimer{0}
{
}

void OyasumiMod::init(void)
{
    state = SleepState::WAIT_STROKE;
    avatar.setExpression(Expression::Sleepy);
    avatar.setSpeechText("Can I sleep?"); // 初期表示の吹き出し
}

void OyasumiMod::pause(void)
{
    avatar.setSpeechText("");
}

void OyasumiMod::btnA_pressed(void) {}
void OyasumiMod::btnB_pressed(void) {}
void OyasumiMod::btnC_pressed(void) {}

// 画面タッチは処理せず、Mod切り替え（スワイプ）などのシステム動作に任せる
void OyasumiMod::display_touched(int16_t x, int16_t y) {}

void OyasumiMod::idle(void)
{
    // 1. なでられ待ち状態
    if (state == SleepState::WAIT_STROKE) {
        HeadTouchSensor::Gesture gesture = HeadTouchSensor::update();

        // 頭部タッチセンサーで「なでる」動作を検知した場合
        if (HeadTouchSensor::isPetGesture(gesture)) {
            Serial.println("[OyasumiMod] Head stroked! Going to sleep...");

            state = SleepState::GOING_TO_SLEEP;
            sleepTimer = millis();

            avatar.setExpression(Expression::Sleepy);
            avatar.setSpeechText("Good night, master."); // なでられた後の吹き出し
        }
    }
    // 2. 電源切断待ち状態
    else if (state == SleepState::GOING_TO_SLEEP) {
        // メッセージを読めるように2.5秒待ってから電源オフ
        if (millis() - sleepTimer > 2500) {
            executePowerOff();
        }
    }
}

void OyasumiMod::executePowerOff(void)
{
    Serial.println("[OyasumiMod] Powering off...");
    avatar.setSpeechText("");

    // M5Stackの電源を切る
    M5.Power.powerOff();
}