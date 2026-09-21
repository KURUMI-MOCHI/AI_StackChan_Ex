#ifndef CUSTOM_FACE_H
#define CUSTOM_FACE_H

#include <Avatar.h>

using namespace m5avatar;

// --- カスタム口パーツ（角丸アニメーション付き） ---
class CustomMouth : public Drawable {
public:
  void draw(M5Canvas *canvas, BoundingRect rect, DrawContext *drawContext) override {
    float open = drawContext->getMouthOpenRatio(); // 開口率 (0.0 〜 1.0)
    uint16_t color = drawContext->getColorPalette()->get(COLOR_PRIMARY);

    // 公式パラメータに基づく開口アニメーション計算
    // 閉じ口: 幅90, 高さ6, 角丸0
    // 開き口: 幅60, 高さ50, 角丸16
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

// --- カスタム目パーツ（公式基準サイズ） ---
class CustomEye : public Drawable {
private:
  bool isLeft;

public:
  CustomEye(bool isLeft = true) : isLeft(isLeft) {}

  void draw(M5Canvas *canvas, BoundingRect rect, DrawContext *drawContext) override {
    float gazeX, gazeY;
    drawContext->getGaze(&gazeY, &gazeX); // 視線移動

    uint16_t color = drawContext->getColorPalette()->get(COLOR_PRIMARY);

    // 視線オフセット（最大±16px）
    int offsetX = (int)(16.0f * gazeX);
    int offsetY = (int)(16.0f * gazeY);

    int cx = rect.getCenterX() + offsetX;
    int cy = rect.getCenterY() + offsetY;
    int r = 16; // 公式基準半径 16px

    canvas->fillCircle(cx, cy, r, color);
  }
};

// --- カスタム Face クラスの定義 ---
class CustomFace : public Face {
public:
  CustomFace()
      : Face(
            new CustomMouth(),
            new BoundingRect(160, 146), // 口の配置位置 (X=160, Y=146)
            new CustomEye(true),
            new BoundingRect(90, 104),  // 左目の配置位置 (X=90, Y=104)
            new CustomEye(false),
            new BoundingRect(230, 104)  // 右目の配置位置 (X=230, Y=104)
        ) {}
};

#endif // CUSTOM_FACE_H