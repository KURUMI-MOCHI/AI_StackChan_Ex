#ifndef CUSTOM_FACE_H
#define CUSTOM_FACE_H

#include <Avatar.h>
#include <cmath>
#include "stack_chan_led.h"

using namespace m5avatar;

// --- 描画を行わないダミーパーツ（眉毛消去用） ---
class BlankDrawable : public Drawable {
public:
  void draw(M5Canvas *canvas, BoundingRect rect, DrawContext *drawContext) override {}
};

// --- 口パーツ ---
class CustomMouth : public Drawable {
public:
  void draw(M5Canvas *canvas, BoundingRect rect, DrawContext *drawContext) override {
    float open = drawContext->getMouthOpenRatio(); // 0.0 〜 1.0
    uint16_t color = drawContext->getColorPalette()->get(COLOR_PRIMARY);

    int w = 60 - (int)(20.0f * open);
    int h = 4 + (int)(28.0f * open);
    int r = (int)(10.0f * open);

    int x = rect.getCenterX() - (w / 2);
    int y = rect.getCenterY() - (h / 2);

    if (r > 0) {
      canvas->fillRoundRect(x, y, w, h, r, color);
    } else {
      canvas->fillRect(x, y, w, h, color);
    }
  }
};

// ★ メインタスク側から参照する通信通知変数
extern LedEmotion pendingLedEmotion; 
extern bool hasPendingLedEmotion;

// --- 笑顔表示と自然な上まぶた瞬きに対応した目パーツ ---
class CustomEye : public Drawable {
private:
  bool isLeft;
  enum class BlinkState { IDLE, CLOSING, CLOSED, OPENING };
  BlinkState blinkState = BlinkState::IDLE;
  float currentRatio = 1.0f;
  Expression lastExp = static_cast<Expression>(-1);

public:
  CustomEye(bool isLeft = true) : isLeft(isLeft) {}

  void draw(M5Canvas *canvas, BoundingRect rect, DrawContext *drawContext) override {
    const float eyeRadius = 10.5f;
    int cx = rect.getCenterX();
    int cy = rect.getCenterY();

    uint16_t primaryColor = drawContext->getColorPalette()->get(COLOR_PRIMARY);
    uint16_t bgColor = drawContext->getColorPalette()->get(COLOR_BACKGROUND);

    canvas->fillCircle(cx, cy, (int)eyeRadius, primaryColor);

    Expression exp = drawContext->getExpression();

    // ★ 左目かつ表情が変化した瞬間だけ、メインループへ通知を出す
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

    // 表情パラメータの設定
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

    if (!isLeft) {
      rotationDeg = -rotationDeg;
    }

    if (weight < 99.5f) {
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

      float L = 30.0f;
      float D = 30.0f;

      int x1 = (int)(p0x - L * dx);
      int y1 = (int)(p0y - L * dy);
      int x2 = (int)(p0x + L * dx);
      int y2 = (int)(p0y + L * dy);
      int x3 = (int)(p0x + L * dx + D * nx);
      int y3 = (int)(p0y + L * dy + D * ny);
      int x4 = (int)(p0x - L * dx + D * nx);
      int y4 = (int)(p0y - L * dy + D * ny);

      canvas->fillTriangle(x1, y1, x2, y2, x3, y3, bgColor);
      canvas->fillTriangle(x1, y1, x3, y3, x4, y4, bgColor);
    }

    // まばたき処理
    float targetRatio = drawContext->getEyeOpenRatio();

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

// ★ 修正箇所：塗りつぶし範囲を目（半径10.5px）の領域内に絞る
    if (currentRatio < 0.99f) {
      // 目の上端から、閉じる割合に応じた高さだけ上まぶたを覆う
      int fillHeight = (int)((eyeRadius * 2.0f) * (1.0f - currentRatio));
      if (fillHeight > 0) {
        int fillX = cx - (int)eyeRadius - 1;
        int fillY = cy - (int)eyeRadius - 1;
        int fillW = (int)(eyeRadius * 2.0f) + 2;
        
        // 目の領域内だけを背景色で覆う
        canvas->fillRect(fillX, fillY, fillW, fillHeight, bgColor);
      }
    }
  }
};

// --- レイアウト座標 ---
class CustomFace : public Face {
public:
  CustomFace()
      : Face(
            new CustomMouth(),
            new BoundingRect(148, 160), // 口
            new CustomEye(false),
            new BoundingRect(106, 225), // 右目
            new CustomEye(true),
            new BoundingRect(106, 95),  // 左目
            new BlankDrawable(),
            new BoundingRect(67, 192),  // 眉毛なし（★Y座標を67に設定して吹き出し画面の消滅を防止）
            new BlankDrawable(),
            new BoundingRect(67, 96)    // 眉毛なし（★Y座標を67に設定）
        ) {}
};

#endif // CUSTOM_FACE_H