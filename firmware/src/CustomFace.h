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

// --- 笑顔表示と自然な上まぶた瞬きに対応した目パーツ ---
// --- 笑顔表示と自然な上まぶた瞬きに対応した目パーツ ---
class CustomEye : public Drawable {
private:
  bool isLeft;
  float currentRatio = 1.0f; // 描画側でアニメーションを滑らかにするための変数

public:
  CustomEye(bool isLeft = true) : isLeft(isLeft) {}

  void draw(M5Canvas *canvas, BoundingRect rect, DrawContext *drawContext) override {
    const float eyeRadius = 10.5f; // 半径10.5px (直径21px)

    int cx = rect.getCenterX();
    int cy = rect.getCenterY();

    uint16_t primaryColor = drawContext->getColorPalette()->get(COLOR_PRIMARY);
    uint16_t bgColor = drawContext->getColorPalette()->get(COLOR_BACKGROUND);

    // 1. 基本の目（黒円）を描画
    canvas->fillCircle(cx, cy, (int)eyeRadius, primaryColor);

    // 2. 表情パラメータの設定
    float weight = 100.0f;
    float rotationDeg = 0.0f;
    bool cutFromBottom = false;

    Expression exp = drawContext->getExpression();
    switch (exp) {
      case Expression::Happy:
        weight = 80.0f;       // 20%削る（浅めのカット）
        rotationDeg = -45.0f; // 斜め45度
        cutFromBottom = true; // 下側（口側）をカット
        break;
      case Expression::Angry:
        weight = 70.0f;
        rotationDeg = 12.0f;  // 怒り: つり目
        cutFromBottom = false;
        break;
      case Expression::Sad:
        weight = 70.0f;
        rotationDeg = -8.0f;  // 悲しい: タレ目
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

    // 右目の回転角度を反転（左右対称）
    if (!isLeft) {
      rotationDeg = -rotationDeg;
    }

    // 表情によるカット処理 (weight < 100 の場合)
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

    // 3. まばたき処理（上まぶた降下の補間処理のみ）
    float targetRatio = drawContext->getEyeOpenRatio();
    
    // 標準機能の一瞬の変化を、描画周期に合わせてじわっと追従させる
    currentRatio += (targetRatio - currentRatio) * 0.2f;

    if (currentRatio < 0.98f) {
      // 上まぶたのY座標を下ろしていく
      float eyelidY = (cy - eyeRadius) + (eyeRadius * 2.0f) * (1.0f - currentRatio);
      
      // 完全に閉じたときは目を覆い尽くす
      if (currentRatio <= 0.05f) {
        eyelidY = cy + eyeRadius + 5.0f;
      }

      // 上まぶたより上の領域を背景色で塗りつぶす
      int topY = cy - 35;
      int h = (int)(eyelidY - topY);
      if (h > 0) {
        canvas->fillRect(cx - 25, topY, 50, h, bgColor);
      }
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