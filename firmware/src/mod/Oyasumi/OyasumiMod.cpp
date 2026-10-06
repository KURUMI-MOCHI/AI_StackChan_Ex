#include "OyasumiMod.h"
#include <M5Unified.h>
#include <Avatar.h>
#include "Robot.h"
#include "driver/HeadTouchSensor.h"

// m5avatarの名前空間を使用
using namespace m5avatar;

extern Avatar avatar;

OyasumiMod::OyasumiMod()
    : state{SleepState::WAIT_STROKE}, sleepTimer{0}
{
}

void OyasumiMod::init(void)
{
    state = SleepState::WAIT_STROKE;
    avatar.setExpression(Expression::Sleepy);
    avatar.setSpeechText("Can I sleep?");
}

void OyasumiMod::pause(void)
{
    avatar.setSpeechText("");
}

void OyasumiMod::btnA_pressed(void) {}
void OyasumiMod::btnB_pressed(void) {}
void OyasumiMod::btnC_pressed(void) {}

void OyasumiMod::display_touched(int16_t x, int16_t y) {}

void OyasumiMod::idle(void)
{
    if (state == SleepState::WAIT_STROKE) {
        HeadTouchSensor::Gesture gesture = HeadTouchSensor::update();

        if (HeadTouchSensor::isPetGesture(gesture)) {
            Serial.println("[OyasumiMod] Head stroked! Going to sleep...");

            state = SleepState::GOING_TO_SLEEP;
            sleepTimer = millis();

            avatar.setExpression(Expression::Sleepy);
            avatar.setSpeechText("Yes, master...");
        }
    }
    else if (state == SleepState::GOING_TO_SLEEP) {
        if (millis() - sleepTimer > 2500) {
            executePowerOff();
        }
    }
}

void OyasumiMod::executePowerOff(void)
{
    Serial.println("[OyasumiMod] Powering off...");
    avatar.setSpeechText("");

    M5.Power.powerOff();
}