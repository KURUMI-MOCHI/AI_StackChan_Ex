#include <Arduino.h>
#include "mod/ModManager.h"
#include "VolumeSettingMod.h"
#include "Robot.h"
#include "Scheduler.h"
#include "MySchedule.h"
#include "llm/ChatGPT/ChatGPT.h"
#include "llm/ChatGPT/FunctionCall.h"
#include <WiFiClientSecure.h>
#include <Avatar.h>

using namespace m5avatar;

/// 外部参照 ///
extern Avatar avatar;
extern void sw_tone();

///////////////

// ★追加：サブウィンドウに文字を確実に描画する関数（背景：黒 / 文字：白）
static void drawVolumeTxt(M5Canvas *spi, BoundingRect rect, DrawContext *ctx, void *userData)
{
  int vol = *static_cast<int*>(userData);
  spi->fillScreen(TFT_BLACK);              // 背景を黒で塗る（顔と重なっても消えないようにする）
  spi->setTextColor(TFT_WHITE, TFT_BLACK);   // 文字色を白に指定
  spi->setTextSize(2);                     // 文字サイズ
  spi->drawString("Volume: " + String(vol), 5, 10);
}

VolumeSettingMod::VolumeSettingMod(void)
{
  box_BtnA.setupBox(0, 100, 40, 60);
  box_BtnB.setupBox(140, 100, 40, 60);
  box_BtnC.setupBox(280, 100, 40, 60);
  box_BtnUA.setupBox(0, 0, 80, 60);
  box_BtnUC.setupBox(240, 0, 80, 60);
}

void VolumeSettingMod::init(void)
{
  avatar.setSpeechText("Volume Setting");
  delay(1000);
  update();
  avatar.set_isSubWindowEnable(true);
}

void VolumeSettingMod::pause(void)
{
  avatar.set_isSubWindowEnable(false);
}

void VolumeSettingMod::update()
{
  avatar.setSpeechText("A:- B:Test C:+");

  // ★修正：updateSubWindowTxt の代わりに updateSubWindowCustom を使い、指定座標(0,0,150,50)に確実に描画
  avatar.updateSubWindowCustom(drawVolumeTxt, &(robot->spk_volume), 0, 0, 150, 50);
}

void VolumeSettingMod::btnA_pressed(void)
{
  if(robot->spk_volume >= 10){
    robot->spk_volume -= 10;
  }
  else{
    robot->spk_volume = 0;
  }
  M5.Speaker.setVolume(robot->spk_volume);
  sw_tone();
  update(); // ボタン押下時に画面表示を更新
}

void VolumeSettingMod::btnB_pressed(void)
{
  robot->speech("volume test");
}

void VolumeSettingMod::btnC_pressed(void)
{
  if(robot->spk_volume <= 245){
    robot->spk_volume += 10;
  }
  else{
    robot->spk_volume = 255;
  }
  M5.Speaker.setVolume(robot->spk_volume);
  sw_tone();
  update(); // ボタン押下時に画面表示を更新
}

void VolumeSettingMod::display_touched(int16_t x, int16_t y)
{
  if (box_BtnA.contain(x, y))
  {
    btnA_pressed();
  }

  if (box_BtnB.contain(x, y))
  {
    btnB_pressed();
  }

  if (box_BtnC.contain(x, y))
  {
    btnC_pressed();
  }

  if (box_BtnUA.contain(x, y))
  {

  }

  if (box_BtnUC.contain(x, y))
  {

  }
}

void VolumeSettingMod::idle(void)
{
  // 毎フレーム update() を呼ぶとチカチカするため空にしています
}