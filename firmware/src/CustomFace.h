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

// --- 中間サイズに調整した口パーツ ---
class CustomMouth : public Drawable {
public:
  void draw(M5Canvas *canvas, BoundingRect rect, DrawContext *drawContext) override {
    float open = drawContext->getMouthOpenRatio(); // 0.0 〜 1.0
    uint16_t color = drawContext->getColorPalette()->get(COLOR_PRIMARY);

    // 幅 60〜40px、高さ 4〜32px にサイズダウン
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

// --- タレ目方向修正・視線固定・中間サイズの目パーツ ---
class CustomEye : public Drawable {
private:
  bool isLeft;

public:
  CustomEye(bool isLeft = true) : isLeft(isLeft) {}

  void draw(M5Canvas *canvas, BoundingRect rect, DrawContext *drawContext) override {
    const float eyeRadius = 13.0f; // 目を小さめ（直径26px）に設定

    // 1. 表情に応じた weight と rotation (deg)
    int weight = 100;
    float rotationDeg = 0.0f;

    Expression exp = drawContext->getExpression();
    switch (exp) {
      case Expression::Happy:
        weight = 70;
        rotationDeg = -15.5f; // 照れ・笑顔: タレ目（目尻を下げる）
        break;
      case Expression::Angry:
        weight = 70;
        rotationDeg = 12.0f;  // 怒り: つり目（目尻を上げる）
        break;
      case Expression::Sad:
        weight = 70;
        rotationDeg = -8.0f;  // 悲しい: 少しタレ目
        break;
      case Expression::Sleepy:
        weight = 35;
        rotationDeg = 0.0f;
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

    // 右目の回転角度を反転（左右対称にする）
    if (!isLeft) {
      rotationDeg = -rotationDeg;
    }

    // 2. 視線オフセットは 0 に固定（キョロキョロ移動を完全停止）
    int cx = rect.getCenterX();
    int cy = rect.getCenterY();

    uint16_t primaryColor = drawContext->getColorPalette()->get(COLOR_PRIMARY);
    uint16_t bgColor = drawContext->getColorPalette()->get(COLOR_BACKGROUND);

    // 3. 目（円）の描画
    canvas->fillCircle(cx, cy, (int)eyeRadius, primaryColor);

    // 4. まぶたによる遮蔽カット (weight < 100 の場合)
    if (weight < 100) {
      float yOffset = eyeRadius - ((eyeRadius * 2.0f) * weight / 100.0f);

      float rad = rotationDeg * (3.14159265f / 180.0f);
      float cosA = cosf(rad);
      float sinA = sinf(rad);

      // まぶたの基準位置
      float p0x = cx - yOffset * sinA;
      float p0y = cy + yOffset * cosA;

      float dx = 30.0f * cosA;
      float dy = 30.0f * sinA;
      float ux = -30.0f * sinA;
      float uy = -30.0f * cosA;

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

// --- レイアウト座標 (Y, X) ---
class CustomFace : public Face {
public:
  CustomFace()
      : Face(
            new CustomMouth(),
            new BoundingRect(148, 160), // 口: Y=148, X=160
            new CustomEye(false),
            new BoundingRect(106, 225), // 右目: Y=106, X=225
            new CustomEye(true),
            new BoundingRect(106, 95),  // 左目: Y=106, X=95
            new BlankDrawable(),
            new BoundingRect(0, 0),     // 眉毛削除
            new BlankDrawable(),
            new BoundingRect(0, 0)      // 眉毛削除
        ) {}
};

#endif // CUSTOM_FACE_H