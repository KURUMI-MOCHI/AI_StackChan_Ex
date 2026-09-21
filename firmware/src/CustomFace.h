#ifndef CUSTOMFACE_H_
#define CUSTOMFACE_H_

#include <M5Unified.h>
#include <Avatar.h>

using namespace m5avatar;

// --- カスタムの目 (CustomEye) ---
class CustomEye : public Drawable {
private:
  bool isRight;

public:
  CustomEye(bool isRight = true) : isRight(isRight) {}

  void draw(M5Canvas *canvas, BoundingRect rect, DrawContext *drawContext) override {
    // 1. m5avatar の DrawContext からパラメータを取得
    m5avatar::Gaze gaze = drawContext->getGaze();
    float gazeV = gaze.getVertical();   // 垂直方向の視線
    float gazeH = gaze.getHorizontal(); // 水平方向の視線

    float openRatio = drawContext->getEyeOpenRatio(); // 目の開き具合 (0.0 ~ 1.0)
    uint16_t color = drawContext->getColorPalette()->get(COLOR_PRIMARY);

    int cx = rect.getCenterX();
    int cy = rect.getCenterY();

    // 視線によるわずかな視点移動
    cx += (int)(gazeH * 5.0f);
    cy += (int)(gazeV * 5.0f);

    int eyeWidth = 50;
    int eyeHeight = (int)(60.0f * openRatio);

    // まばたき等で高さが極端に低い場合は描画をスキップ
    if (eyeHeight < 2) return;

    // 目の外枠と白目領域を描画
    canvas->fillEllipse(cx, cy, eyeWidth / 2, eyeHeight / 2, color);
    canvas->fillEllipse(cx, cy, eyeWidth / 2 - 4, eyeHeight / 2 - 4, TFT_WHITE);

    // 瞳の描画
    int pupilRadius = 10;
    canvas->fillCircle(cx, cy, pupilRadius, color);
  }
};

// --- カスタムの口 (CustomMouth) ---
class CustomMouth : public Drawable {
public:
  void draw(M5Canvas *canvas, BoundingRect rect, DrawContext *drawContext) override {
    float openRatio = drawContext->getMouthOpenRatio(); // 口の開き具合 (0.0 ~ 1.0)
    uint16_t color = drawContext->getColorPalette()->get(COLOR_PRIMARY);

    int cx = rect.getCenterX();
    int cy = rect.getCenterY();

    int mouthWidth = 60;
    int mouthHeight = (int)(40.0f * openRatio);

    if (mouthHeight < 4) {
      // 閉じている時は横線を描画
      canvas->drawFastHLine(cx - mouthWidth / 2, cy, mouthWidth, color);
    } else {
      // 開いている時は楕円を描画
      canvas->fillEllipse(cx, cy, mouthWidth / 2, mouthHeight / 2, color);
    }
  }
};

// --- カスタム顔 (CustomFace) ---
class CustomFace : public Face {
public:
  CustomFace()
    : Face(
        new CustomMouth(), new BoundingRect(163, 148),
        new CustomEye(true), new BoundingRect(93, 90),
        new CustomEye(false), new BoundingRect(93, 230),
        nullptr, nullptr,  // 左眉なし
        nullptr, nullptr   // 右眉なし
      ) {}
};

#endif // CUSTOMFACE_H_