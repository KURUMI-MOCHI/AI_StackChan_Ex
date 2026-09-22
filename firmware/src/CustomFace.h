#ifndef CUSTOM_FACE_H
#define CUSTOM_FACE_H

#include <Avatar.h>
#include <cmath>

using namespace m5avatar;

// --- 描画を行わないダミーパーツ（眉毛消去用） ---
class BlankDrawable : public Drawable {
public:
  void draw(M5Canvas *canvas, BoundingRect rect, DrawContext *drawContext) override {
    // 眉毛は描画しない
  }
};

// --- 製品版 mouth.cpp を完全トレースした口パーツ ---
class CustomMouth : public Drawable {
public:
  void draw(M5Canvas *canvas, BoundingRect rect, DrawContext *drawContext) override {
    float open = drawContext->getMouthOpenRatio(); // 0.0 〜 1.0
    uint16_t color = drawContext->getColorPalette()->get(COLOR_PRIMARY);

    int w = 90 - (int)(30.0f * open);
    int h = 6 + (int)(44.0f * open);
    int r = (int)(16.0f * open);

    int x = rect.getCenterX() - (w / 2);
    int y = rect.getCenterY() - (h / 2);

    if (r > 0) {
      canvas->fillRoundRect(x, y, w, h, r, color);
    } else {
      canvas->fillRect(x, y, w, h, color);
    }
  }
};

// --- スプライト無しで直接高精度描画する目パーツ ---
class CustomEye : public Drawable {
private:
  bool isLeft;

public:
  CustomEye(bool isLeft = true) : isLeft(isLeft) {}

  void draw(M5Canvas *canvas, BoundingRect rect, DrawContext *drawContext) override {
    // 1. 表情に応じた weight と rotation の決定
    int weight = 100;
    float rotationDeg = 0.0f;

    Expression exp = drawContext->getExpression();
    switch (exp) {
      case Expression::Happy:
        weight = 72;
        rotationDeg = 15.5f;
        break;
      case Expression::Angry:
        weight = 70;
        rotationDeg = 4.5f;
        break;
      case Expression::Sad:
        weight = 70;
        rotationDeg = -4.0f;
        break;
      case Expression::Sleepy:
        weight = 35;
        rotationDeg = -0.5f;
        break;
      case Expression::Doubt:
        weight = 75;
        rotationDeg = 0.0f;
        break;
      case Expression::Neutral:
      default:
        weight = 100;
        rotationDeg = 0.0f;
        break;
    }

    // まばたき（EyeOpenRatio）
    float openRatio = drawContext->getEyeOpenRatio();
    if (openRatio < 1.0f) {
      weight = (int)(weight * openRatio);
    }

    // 右目の回転角度は反転
    if (!isLeft) {
      rotationDeg = -rotationDeg;
    }

    // 2. 視線（Gaze）位置
    Gaze gaze = drawContext->getGaze();
    int offsetX = (int)(16.0f * gaze.getHorizontal());
    int offsetY = (int)(16.0f * gaze.getVertical());

    int cx = rect.getCenterX() + offsetX;
    int cy = rect.getCenterY() + offsetY;

    uint16_t primaryColor = drawContext->getColorPalette()->get(COLOR_PRIMARY);
    uint16_t bgColor = drawContext->getColorPalette()->get(COLOR_BACKGROUND);

    // 3. 目（円）を描画
    canvas->fillCircle(cx, cy, 16, primaryColor);

    // 4. まぶたによる遮蔽カット (weight < 100 の場合)
    if (weight < 100) {
      float yOffset = 16.0f - (32.0f * weight / 100.0f);

      float rad = rotationDeg * (3.14159265f / 180.0f);
      float cosA = cosf(rad);
      float sinA = sinf(rad);

      // まぶたの基準位置
      float p0x = cx - yOffset * sinA;
      float p0y = cy + yOffset * cosA;

      float dx = 40.0f * cosA;
      float dy = 40.0f * sinA;
      float ux = -40.0f * sinA;
      float uy = -40.0f * cosA;

      int x1 = (int)(p0x - dx), y1 = (int)(p0y - dy);
      int x2 = (int)(p0x + dx), y2 = (int)(p0y + dy);
      int x3 = (int)(p0x + dx + ux), y3 = (int)(p0y + dy + uy);
      int x4 = (int)(p0x - dx + ux), y4 = (int)(p0y - dy + uy);

      // 背景色でポリゴン塗りつぶし
      canvas->fillTriangle(x1, y1, x2, y2, x3, y3, bgColor);
      canvas->fillTriangle(x1, y1, x3, y3, x4, y4, bgColor);
    }
  }
};

// --- 製品版のレイアウト座標を適用した CustomFace ---
class CustomFace : public Face {
public:
  CustomFace()
      : Face(
            new CustomMouth(),
            new BoundingRect(160, 146), // 口
            new CustomEye(false),
            new BoundingRect(230, 104), // 右目 (X=230)
            new CustomEye(true),
            new BoundingRect(90, 104),  // 左目 (X=90)
            new BlankDrawable(),
            new BoundingRect(0, 0),     // 眉毛削除
            new BlankDrawable(),
            new BoundingRect(0, 0)      // 眉毛削除
        ) {}
};

#endif // CUSTOM_FACE_H