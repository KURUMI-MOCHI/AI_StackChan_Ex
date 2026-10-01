// Copyright (c) Shinya Ishikawa. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full
// license information.

#include "Eye.h"
#include <cmath>

namespace m5avatar {

Eye::Eye(uint16_t x, uint16_t y, uint16_t r, bool isLeft) : Eye(r, isLeft) {}

Eye::Eye(uint16_t r, bool isLeft) : r{r}, isLeft{isLeft} {}

void Eye::draw(M5Canvas *spi, BoundingRect rect, DrawContext *ctx) {
  Expression exp = ctx->getExpression();
  Gaze g = ctx->getGaze();
  
  // 標準の視線移動（gaze）オフセットを計算
  uint32_t offsetX = g.getHorizontal() * 3;
  uint32_t offsetY = g.getVertical() * 3;
  
  uint32_t cx = rect.getCenterX() + offsetX;
  // 見た目だけ口に近づけるため、Y座標を下方向へ移動（+18px）
  uint32_t cy = rect.getCenterY() + offsetY + 18;

  // 位置（cx, cy）を変えずに目だけを自然に大きく見せるための半径補正（約1.3倍）
  float eyeR = (float)r * 1.3f;

  uint16_t primaryColor = ctx->getColorDepth() == 1 ? 1 : ctx->getColorPalette()->get(COLOR_PRIMARY);
  uint16_t backgroundColor = ctx->getColorDepth() == 1 ? 0 : ctx->getColorPalette()->get(COLOR_BACKGROUND);

  // 1. ベースとなる丸目（黒目）を描画
  spi->fillCircle(cx, cy, (int)eyeR, primaryColor);

  // 2. 左目かつ表情が変化した瞬間のみ、LEDコントローラーへ直接感情変更を通知
  if (isLeft && exp != lastExp) {
    lastExp = exp;
    switch (exp) {
      case Expression::Happy:   LedController.setEmotion(LedEmotion::HAPPY); break;
      case Expression::Angry:   LedController.setEmotion(LedEmotion::ANGRY); break;
      case Expression::Sad:     LedController.setEmotion(LedEmotion::SAD); break;
      case Expression::Sleepy:  LedController.setEmotion(LedEmotion::SLEEPY); break;
      case Expression::Doubt:   LedController.setEmotion(LedEmotion::DOUBT); break;
      case Expression::Neutral:
      default:                  LedController.setEmotion(LedEmotion::NORMAL); break;
    }
  }

  // 3. まばたき（上まぶた）アニメーション状態の更新
  float targetRatio = ctx->getEyeOpenRatio();

  if (blinkState == BlinkState::IDLE && targetRatio < 0.8f) {
    blinkState = BlinkState::CLOSING;
  }

  const float closeStep = 0.40f;
  const float openStep  = 0.30f;

  if (blinkState == BlinkState::CLOSING) {
    currentRatio -= closeStep;
    if (currentRatio <= 0.0f) {
      currentRatio = 0.0f;
      blinkState = BlinkState::CLOSED;
    }
  } else if (blinkState == BlinkState::CLOSED) {
    blinkState = BlinkState::OPENING;
  } else if (blinkState == BlinkState::OPENING) {
    currentRatio += openStep;
    if (currentRatio >= 1.0f) {
      currentRatio = 1.0f;
      blinkState = BlinkState::IDLE;
    }
  }

  // 4. 製品版 (eyes.cpp) のパラメータ（Weight & Rotation）適用
  float weight = 100.0f;
  float rotationDeg = 0.0f;

  switch (exp) {
    case Expression::Happy:
      weight = 72.0f;       // 72%露出（28%だけカット）
      rotationDeg = 155.0f; // 製品版 1550 (155.0 deg)
      break;
    case Expression::Angry:
      weight = 70.0f;       // 70%露出
      rotationDeg = 45.0f;  // 製品版 450 (45.0 deg)
      break;
    case Expression::Sad:
      weight = 70.0f;       // 70%露出
      rotationDeg = -40.0f; // 製品版 -400 (-40.0 deg)
      break;
    case Expression::Sleepy:
      weight = 35.0f;       // 35%露出
      rotationDeg = -5.0f;  // 製品版 -50 (-5.0 deg)
      break;
    case Expression::Doubt:
      weight = 75.0f;
      rotationDeg = 0.0f;
      break;
    case Expression::Neutral:
    default:
      weight = 100.0f;
      rotationDeg = 0.0f;
      break;
  }

  // 右目の場合は回転角度を反転 (製品版: apply_style)
  if (!isLeft) {
    rotationDeg = -rotationDeg;
  }

  // 表情の Weight にまばたき開度 (currentRatio) を合成
  float effectiveWeight = weight * currentRatio;

  // 5. 回転する四角やまぶた（マスク）を LVGL 同等幾何で計算描画
  if (effectiveWeight < 99.5f) {
    float rad = rotationDeg * (3.14159265f / 180.0f);
    float cosA = cosf(rad);
    float sinA = sinf(rad);

    // 未回転状態での中心からのまぶた境界線距離
    // Weight=100 -> -eyeR（最上部、削りなし）
    // Weight=50  -> 0（中心、半月）
    // Weight=72  -> -0.44 * eyeR（中心より上の位置。端から28%だけ削れる）
    float limit = (1.0f - 2.0f * (effectiveWeight / 100.0f)) * eyeR;

    // 回転後の境界線の基準点 P0 (目の中心 cx, cy からの移動)
    float p0x = cx - limit * sinA;
    float p0y = cy + limit * cosA;

    // 境界線方向 T = (cosA, sinA)
    // マスク外側方向 N = (sinA, -cosA)
    float tx = cosA;
    float ty = sinA;
    float nx = sinA;
    float ny = -cosA;

    float L = eyeR * 4.0f; // 十分なマスク幅
    float D = eyeR * 4.0f; // 十分なマスク高さ

    // まぶたマスク矩形の4頂点
    int x1 = (int)(p0x - L * tx);
    int y1 = (int)(p0y - L * ty);
    int x2 = (int)(p0x + L * tx);
    int y2 = (int)(p0y + L * ty);
    int x3 = (int)(p0x + L * tx + D * nx);
    int y3 = (int)(p0y + L * ty + D * ny);
    int x4 = (int)(p0x - L * tx + D * nx);
    int y4 = (int)(p0y - L * ty + D * ny);

    spi->fillTriangle(x1, y1, x2, y2, x3, y3, backgroundColor);
    spi->fillTriangle(x1, y1, x3, y3, x4, y4, backgroundColor);
  }
}

} // namespace m5avatar