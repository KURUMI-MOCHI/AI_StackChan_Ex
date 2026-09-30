// Copyright (c) Shinya Ishikawa. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full
// license information.

#include "Eye.h"
#include <cmath>

// メインタスク連携用の通信変数（グローバル参照）
extern LedEmotion pendingLedEmotion;
extern bool hasPendingLedEmotion;

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
  uint32_t cy = rect.getCenterY() + offsetY;

  uint16_t primaryColor = ctx->getColorDepth() == 1 ? 1 : ctx->getColorPalette()->get(COLOR_PRIMARY);
  uint16_t backgroundColor = ctx->getColorDepth() == 1 ? 0 : ctx->getColorPalette()->get(COLOR_BACKGROUND);

  // 1. ベースとなる丸目（黒目）を描画
  spi->fillCircle(cx, cy, r, primaryColor);

  // 2. 左目かつ表情が変化した瞬間のみ、LEDコントローラーへ感情変更を通知
  if (isLeft && exp != lastExp) {
    lastExp = exp;
    switch (exp) {
      case Expression::Happy:   pendingLedEmotion = LedEmotion::HAPPY; break;
      case Expression::Angry:   pendingLedEmotion = LedEmotion::ANGRY; break;
      case Expression::Sad:     pendingLedEmotion = LedEmotion::SAD; break;
      case Expression::Sleepy:  pendingLedEmotion = LedEmotion::SLEEPY; break;
      case Expression::Doubt:   pendingLedEmotion = LedEmotion::DOUBT; break;
      case Expression::Neutral:
      default:                  pendingLedEmotion = LedEmotion::NORMAL; break;
    }
    hasPendingLedEmotion = true;
  }

  // 3. 表情に応じた「目の切り欠き（三角マスク）」計算
  float weight = 100.0f;
  float rotationDeg = 0.0f;
  bool cutFromBottom = false;

  switch (exp) {
    case Expression::Happy:
      weight = 80.0f;
      rotationDeg = -45.0f;
      cutFromBottom = true;
      break;
    case Expression::Angry:
      weight = 70.0f;
      rotationDeg = 12.0f;
      cutFromBottom = false;
      break;
    case Expression::Sad:
      weight = 70.0f;
      rotationDeg = -8.0f;
      cutFromBottom = false;
      break;
    case Expression::Sleepy:
      weight = 35.0f;
      rotationDeg = 0.0f;
      cutFromBottom = false;
      break;
    case Expression::Doubt:
      weight = 75.0f;
      rotationDeg = 0.0f;
      cutFromBottom = false;
      break;
    case Expression::Neutral:
    default:
      weight = 100.0f;
      rotationDeg = 0.0f;
      cutFromBottom = false;
      break;
  }

  // 右目の場合は回転角度を反転
  if (!isLeft) {
    rotationDeg = -rotationDeg;
  }

  // 切り欠きが必要な場合（weight < 100%）、背景色でマスク三角形を描画
  if (weight < 99.5f) {
    float eyeRadius = (float)r;
    float visibleHeight = (eyeRadius * 2.0f) * (weight / 100.0f);
    float rad = rotationDeg * (3.14159265f / 180.0f);
    float cosA = cosf(rad);
    float sinA = sinf(rad);

    float dx = cosA;
    float dy = sinA;

    float nx, ny;
    if (cutFromBottom) {
      nx = -sinA;
      ny = cosA;
    } else {
      nx = sinA;
      ny = -cosA;
    }

    float shift = eyeRadius - visibleHeight;
    float p0x = cx - shift * nx;
    float p0y = cy - shift * ny;

    float L = eyeRadius * 3.0f; // 余裕を持たせたマスクサイズ
    float D = eyeRadius * 3.0f;

    int x1 = (int)(p0x - L * dx);
    int y1 = (int)(p0y - L * dy);
    int x2 = (int)(p0x + L * dx);
    int y2 = (int)(p0y + L * dy);
    int x3 = (int)(p0x + L * dx + D * nx);
    int y3 = (int)(p0y + L * dy + D * ny);
    int x4 = (int)(p0x - L * dx + D * nx);
    int y4 = (int)(p0y - L * dy + D * ny);

    spi->fillTriangle(x1, y1, x2, y2, x3, y3, backgroundColor);
    spi->fillTriangle(x1, y1, x3, y3, x4, y4, backgroundColor);
  }

  // 4. 上から下へ閉じる「まばたき（上まぶた）」アニメーション処理
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

  // 上からシャッターのように降りてくるまぶたの描画
  if (currentRatio < 0.99f) {
    int fillHeight = (int)(((float)r * 2.0f) * (1.0f - currentRatio));
    if (fillHeight > 0) {
      int fillX = cx - (int)r - 1;
      int fillY = cy - (int)r - 1;
      int fillW = (int)((float)r * 2.0f) + 2;
      
      spi->fillRect(fillX, fillY, fillW, fillHeight, backgroundColor);
    }
  }
}

} // namespace m5avatar