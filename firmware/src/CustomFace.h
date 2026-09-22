#ifndef CUSTOM_FACE_H
#define CUSTOM_FACE_H

#include <Avatar.h>
#include <cmath>

using namespace m5avatar;

// --- 描画を行わないダミーパーツ（眉毛消去用） ---
class BlankDrawable : public Drawable {
public:
  void draw(M5Canvas *canvas, BoundingRect rect, DrawContext *drawContext) override {}
};

// --- 中間サイズに調整した口パーツ ---
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

// --- 笑顔：浅めの斜め45度カット・中間サイズ・視線固定の目パーツ ---
class CustomEye : public Drawable {
private:
  bool isLeft;

public:
  CustomEye(bool isLeft = true) : isLeft(isLeft) {}

  void draw(M5Canvas *canvas, BoundingRect rect, DrawContext *drawContext) override {
    const float eyeRadius = 10.5f; // 半径10.5px (直径21px)

    int weight = 100;
    float rotationDeg = 0.0f;
    bool cutFromBottom = false;

    Expression exp = drawContext->getExpression();
    switch (exp) {
      case Expression::Happy:
        weight = 85;          // ★削る量を少なく（15%だけカット）
        rotationDeg = -45.0f; // ★斜め45度のカットライン
        cutFromBottom = true; // 下側（口側）をカット
        break;
      case Expression::Angry:
        weight = 70;
        rotationDeg = 12.0f;  // 怒り: つり目
        cutFromBottom = false;
        break;
      case Expression::Sad:
        weight = 70;
        rotationDeg = -8.0f;  // 悲しい: タレ目
        cutFromBottom = false;
        break;
      case Expression::Sleepy:
        weight = 35;
        rotationDeg = 0.0f;
        cutFromBottom = false;
        break;
      case Expression::Doubt:
        weight = 75;
        rotationDeg = 0.0f;
        cutFromBottom = false;
        break;
      case Expression::Neutral:
      default:
        weight = 100;
        rotationDeg = 0.0f;
        cutFromBottom = false;
        break;
    }

    // まばたき（EyeOpenRatio）処理
    float openRatio = drawContext->getEyeOpenRatio();
    if (openRatio < 1.0f) {
      weight = (int)(weight * openRatio);
      cutFromBottom = false; // まばたき時は通常通り上まぶたを閉じる
    }

    // 右目の回転角度を反転
    if (!isLeft) {
      rotationDeg = -rotationDeg;
    }

    int cx = rect.getCenterX();
    int cy = rect.getCenterY();

    uint16_t primaryColor = drawContext->getColorPalette()->get(COLOR_PRIMARY);
    uint16_t bgColor = drawContext->getColorPalette()->get(COLOR_BACKGROUND);

    // 1. 目（円）の描画
    canvas->fillCircle(cx, cy, (int)eyeRadius, primaryColor);

    // 2. まぶたによる遮蔽カット (weight < 100 の場合)
    if (weight < 100) {
      float visibleHeight = (eyeRadius * 2.0f) * ((float)weight / 100.0f);
      float yOffset;
      float uDirection;

      if (cutFromBottom) {
        yOffset = -eyeRadius + visibleHeight;
        uDirection = 1.0f; // 下方向 (+Y) にマスク領域を拡張
      } else {
        yOffset = eyeRadius - visibleHeight;
        uDirection = -1.0f; // 上方向 (-Y) にマスク領域を拡張
      }

      float rad = rotationDeg * (3.14159265f / 180.0f);
      float cosA = cosf(rad);
      float sinA = sinf(rad);

      float p0x = cx - yOffset * sinA;
      float p0y = cy + yOffset * cosA;

      float dx = 25.0f * cosA;
      float dy = 25.0f * sinA;
      float ux = uDirection * 25.0f * sinA;
      float uy = uDirection * 25.0f * cosA;

      int x1 = (int)(p0x - dx), y1 = (int)(p0y - dy);
      int x2 = (int)(p0x + dx), y2 = (int)(p0y + dy);
      int x3 = (int)(p0x + dx + ux), y3 = (int)(p0y + dy + uy);
      int x4 = (int)(p0x - dx + ux), y4 = (int)(p0y - dy + uy);

      canvas->fillTriangle(x1, y1, x2, y2, x3, y3, bgColor);
      canvas->fillTriangle(x1, y1, x3, y3, x4, y4, bgColor);
    }
  }
};

// --- レイアウト座標 (Y, X) ---
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
            new BoundingRect(0, 0),     // 眉毛なし
            new BlankDrawable(),
            new BoundingRect(0, 0)      // 眉毛なし
        ) {}
};

#endif // CUSTOM_FACE_H