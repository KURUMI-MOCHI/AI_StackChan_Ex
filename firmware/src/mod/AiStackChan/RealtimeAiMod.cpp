#if defined(REALTIME_API)

#include <Arduino.h>
#include <deque>
#include <SD.h>
#include <SPIFFS.h>
#include "mod/ModManager.h"
#include "RealtimeAiMod.h"
#include <Avatar.h>
#include "Robot.h"
#include "llm/ChatGPT/FunctionCall.h"
#include <WiFiClientSecure.h>
#include "Scheduler.h"
#include "MySchedule.h"
#include "share/SDUtil.h"
#include "driver/HeadTouchSensor.h"
#include "stack_chan_led.h"

using namespace m5avatar;

/// 外部参照 ///
extern Avatar avatar;
extern bool servo_home;
extern void sw_tone();
extern void alarm_tone();
///////////////

RealtimeAiMod::RealtimeAiMod(bool _isOffline)
  : isOffline{_isOffline}
{
  box_servo.setupBox(80, 120, 80, 80);
  box_stt.setupBox(0, 0, M5.Display.width(), 60);
  box_BtnA.setupBox(0, 100, 40, 60);
  box_BtnC.setupBox(280, 100, 40, 60);

  if (robot != nullptr && robot->llm != nullptr) {
    pRtLLM = (RealtimeLLMBase*)robot->llm;
  } else {
    pRtLLM = nullptr;
    Serial.println("[Warning] robot or robot->llm is null in RealtimeAiMod constructor!");
  }
}

void RealtimeAiMod::init(void)
{
  avatar.set_isSubWindowEnable(true);

  // ★ 起動時・モード切り替え時にサーボの状態に合わせてLED初期状態を設定
  if (servo_home) {
    LedController.setState(LedState::OFF);
  } else {
    LedController.setState(LedState::STANDBY);
  }

  if (pRtLLM != nullptr) {
    pRtLLM->invokeWebSocketLoopTask();
  }
}

void RealtimeAiMod::pause(void)
{
  avatar.set_isSubWindowEnable(false);
  if (pRtLLM != nullptr) {
    pRtLLM->suspendWebSocketLoopTask();
  }
}

void RealtimeAiMod::update(int page_no)
{

}

void RealtimeAiMod::btnA_pressed(void)
{
#if defined(ARDUINO_M5STACK_ATOMS3R)
  Serial.println("Btn A pressed");
  sw_tone();
  toggleRealtimeRecord();
#endif
}

void RealtimeAiMod::btnB_longPressed(void)
{

}

void RealtimeAiMod::btnC_pressed(void)
{
  static bool isQrDrawing = false;
  if(!isQrDrawing){
    avatar.setSpeechText("");
    String url = String("http://") + WiFi.localIP().toString();
    avatar.updateSubWindowQrcode(url);
    avatar.set_isSubWindowEnable(true);
    isQrDrawing = true;
  }else{
    avatar.set_isSubWindowEnable(false);
    isQrDrawing = false;
  }
}

void RealtimeAiMod::display_touched(int16_t x, int16_t y)
{
  if (box_stt.contain(x, y))
  {
    sw_tone();
    LedController.flashFeedback(); // ★長めの白2回点滅
    toggleRealtimeRecord();
  }
#ifdef USE_SERVO
  if (box_servo.contain(x, y))
  {
    sw_tone();
    LedController.flashFeedback(); // ★長めの白2回点滅
    servo_home = !servo_home;

    // サーボ停止中は消灯、動作中はスタンバイ（ゆらぎ）
    if (servo_home) {
      LedController.setState(LedState::OFF);
    } else {
      LedController.setState(LedState::STANDBY);
    }
  }
#endif
  if (box_BtnA.contain(x, y))
  {
    //sw_tone();
  }
  if (box_BtnC.contain(x, y))
  {
    btnC_pressed();
  }
}

void RealtimeAiMod::doubleTapped(float ax, float ay, float az)
{
  Serial.printf("Mod double tapped. ax=%.3f ay=%.3f az=%.3f\n", ax, ay, az);
#if defined(ARDUINO_M5STACK_ATOMS3R)
  sw_tone();
  toggleRealtimeRecord();
#endif
}

void RealtimeAiMod::idle(void)
{
  bool isSpeaking = false;

#ifdef REALTIME_API_WITH_TTS
  if (robot != nullptr && pRtLLM != nullptr) {
    if(robot->asyncPlaying || (pRtLLM->getOutputTextQueueSize() != 0)){
      isSpeaking = true;
      pRtLLM->setSpeaking(true);
    }
    else{
      pRtLLM->setSpeaking(false);
    }
  }
#endif  //REALTIME_API_WITH_TTS

  // ★ 青点滅（THINKING）スタック防止＆待機状態復帰処理
  if (pRtLLM != nullptr) {
    bool isRecording = pRtLLM->isRealtimeRecording();
    
    if (!isRecording && !isSpeaking) {
      LedState currentState = LedController.getState();
      
      if (currentState == LedState::THINKING || currentState == LedState::LISTENING) {
        if (servo_home) {
          LedController.setState(LedState::OFF);     // サーボ停止中は消灯
        } else {
          LedController.setState(LedState::STANDBY); // サーボ動作中はオレンジ
        }
      }
    }
  }

  // Alarm (Function Calling)
  alarmEventHandler();
  updateHeadTouchExpression();

#if 0 
  //スケジューラ処理
  if(!isOffline){
    run_schedule();
  }
#endif
}

void RealtimeAiMod::alarmEventHandler()
{
  if(xAlarmTimer != NULL){
    TickType_t xRemainingTime;

    xRemainingTime = xTimerGetExpiryTime( xAlarmTimer ) - xTaskGetTickCount();
    avatarText = "Alarm countdown: " + String(xRemainingTime / 1000);
    avatar.set_isSubWindowEnable(true);
    avatar.updateSubWindowTxt(avatarText, 0, 0, 200, 50);
  }

  if (alarmTimerCallbacked) {
    alarmTimerCallbacked = false;
    avatar.set_isSubWindowEnable(false);
    alarm_tone();
  }

  if (alarmTimerCanceled) {
    alarmTimerCanceled = false;
    avatar.set_isSubWindowEnable(false);
  }
}

void RealtimeAiMod::updateHeadTouchExpression(void)
{
  HeadTouchSensor::Gesture gesture = HeadTouchSensor::update();
  if (HeadTouchSensor::isPetGesture(gesture)) {
    headTouchHappyUntilMs = millis() + 3000;
    headTouchHappyActive = true;
    avatar.setExpression(Expression::Happy);
    LedController.setEmotion(LedEmotion::HAPPY); // ★ なでられた時：HAPPY発色
    Serial.printf("[HeadTouch] pet gesture=%s\n", HeadTouchSensor::gestureName(gesture));
  }

  if (headTouchHappyActive && millis() < headTouchHappyUntilMs) {
    avatar.setExpression(Expression::Happy);
    return;
  }

  if (headTouchHappyActive) {
    headTouchHappyActive = false;
    avatar.setExpression(Expression::Neutral);
    LedController.setEmotion(LedEmotion::NORMAL); // ★ 復帰時：NORMAL発色
  }
}

bool RealtimeAiMod::isBusy(void)
{
  if(pRtLLM->isRealtimeRecording() || pRtLLM->isSpeaking()){
    return true;
  }else{
    return false;
  }
}

void RealtimeAiMod::toggleRealtimeRecord(void)
{
  if(pRtLLM->isRealtimeRecording()){
    pRtLLM->stopRealtimeRecord();
    LedController.setState(LedState::THINKING);  // ★ 録音停止時：処理中（青パルス）
  }else{
    pRtLLM->startRealtimeRecord();
    LedController.setState(LedState::LISTENING); // ★ 録音開始時：聞き取り中（緑固定）
  }
}

#endif //REALTIME_API