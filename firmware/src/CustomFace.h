#ifndef CUSTOM_FACE_H
#define CUSTOM_FACE_H

#include <Avatar.h>

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
    float open = drawContext->getMouthOpenRatio(); // 0.0 〜 1.0 (weight 0 〜 100)
    uint16_t color = drawContext->getColorPalette()->get(COLOR_PRIMARY);

    // mouth.cpp の定数から線形補間
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

// --- 製品版 eyes.cpp (まぶたマスク + 回転) を完全トレースした目パーツ ---
class CustomEye : public Drawable {
private:
  bool isLeft;

public:
  CustomEye(bool isLeft = true) : isLeft(isLeft) {}

  void draw(M5Canvas *canvas, BoundingRect rect, DrawContext *drawContext) override {
    // 1. Emotion（表情）に応じた weight と rotation (deg) の決定
    int weight = 100;
    float rotationDeg = 0.0f;

    Expression exp = drawContext->getExpression();
    switch (exp) {
      case Expression::Happy:
        weight = 72;
        rotationDeg = 15.5f; // eyes.cpp の 1550 (0.1度単位)
        break;
      case Expression::Angry:
        weight = 70;
        rotationDeg = 4.5f;  // eyes.cpp の 450
        break;
      case Expression::Sad:
        weight = 70;
        rotationDeg = -4.0f; // eyes.cpp の -400
        break;
      case Expression::Sleepy:
        weight = 35;
        rotationDeg = -0.5f; // eyes.cpp の -50
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

    // まばたき（EyeOpenRatio）による上書き処理
    float openRatio = drawContext->getEyeOpenRatio();
    if (openRatio < 1.0f) {
      weight = (int)(weight * openRatio);
    }

    // 右目の回転角度は反転 (eyes.cpp: setRotation(-rotation))
    if (!isLeft) {
      rotationDeg = -rotationDeg;
    }

    // 2. まぶたのY座標オフセット計算
    int eyelidOffsetY = -((weight * 32) / 100);

    // 3. カラーパレットの取得
    uint16_t primaryColor = drawContext->getColorPalette()->get(COLOR_PRIMARY);
    uint16_t bgColor = drawContext->getColorPalette()->get(COLOR_BACKGROUND);

    // 4. オフスクリーンスプライト (32x32) の生成とパーツ合成
    M5Canvas eyeSpr(canvas);
    eyeSpr.createSprite(32, 32);
    eyeSpr.fillSprite(bgColor);

    // 目（中心16,16 半径16の円）
    eyeSpr.fillCircle(16, 16, 16, primaryColor);

    // まぶた（上部からの遮蔽マスク）
    int coverHeight = 32 + eyelidOffsetY;
    if (coverHeight > 0) {
      eyeSpr.fillRect(0, 0, 32, coverHeight, bgColor);
    }

    // 5. 視線（Gaze）オフセット計算
    Gaze gaze = drawContext->getGaze();
    int offsetX = (int)(16.0f * gaze.getHorizontal());
    int offsetY = (int)(16.0f * gaze.getVertical());

    int cx = rect.getCenterX() + offsetX;
    int cy = rect.getCenterY() + offsetY;

    // 6. 指定のピボット中心で回転描画してメインキャンバスへ転送
    eyeSpr.pushRotateZoom(canvas, cx, cy, rotationDeg, 1.0f, 1.0f, bgColor);
    eyeSpr.deleteSprite();
  }
};

// --- 製品版のレイアウト座標を適用した CustomFace ---
class CustomFace : public Face {
public:
  CustomFace()
      : Face(
            new CustomMouth(),
            new BoundingRect(160, 146), // 口: _mouth_pos(0, 26) -> (160, 146)
            new CustomEye(false),
            new BoundingRect(230, 104), // 右目: _eye_pos(70, -16) -> (230, 104)
            new CustomEye(true),
            new BoundingRect(90, 104),  // 左目: _eye_pos(-70, -16) -> (90, 104)
            new BlankDrawable(),
            new BoundingRect(0, 0),     // 眉毛削除
            new BlankDrawable(),
            new BoundingRect(0, 0)      // 眉毛削除
        ) {}
};

#endif // CUSTOM_FACE_H