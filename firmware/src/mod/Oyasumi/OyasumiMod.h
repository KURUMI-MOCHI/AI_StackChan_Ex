#ifndef _OYASUMI_MOD_H
#define _OYASUMI_MOD_H

#include <Arduino.h>
#include "mod/ModBase.h"

class OyasumiMod : public ModBase {
private:
    enum class SleepState {
        WAIT_STROKE,    // なでられ待ち
        GOING_TO_SLEEP  // メッセージ表示後の電源切断待機
    };

    SleepState state;
    uint32_t sleepTimer;

public:
    OyasumiMod();

    void init(void) override;
    void pause(void) override;
    void btnA_pressed(void) override;
    void btnB_pressed(void) override;
    void btnC_pressed(void) override;
    void display_touched(int16_t x, int16_t y) override;
    void idle(void) override;

private:
    void executePowerOff(void);
};

#endif // _OYASUMI_MOD_H