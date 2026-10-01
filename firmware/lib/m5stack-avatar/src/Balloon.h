// Copyright (c) Shinya Ishikawa. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full
// license information.

#ifndef BALLOON_H_
#define BALLOON_H_
#define LGFX_USE_V1
#include <M5Unified.h>
#include "DrawContext.h"
#include "Drawable.h"

const int16_t TEXT_HEIGHT = 8;
const int16_t TEXT_SIZE = 2;   // フォントサイズを 2 に戻す
const int cy = 220;            // 画面下部 Y座標

namespace m5avatar {
class Balloon final : public Drawable {
 public:
  Balloon() = default;
  ~Balloon() = default;
  Balloon(const Balloon &other) = default;
  Balloon &operator=(const Balloon &other) = default;

  void draw(M5Canvas *spi, BoundingRect rect,
            DrawContext *drawContext) override {
    const char *text = drawContext->getspeechText();
    const lgfx::IFont *font = drawContext->getSpeechFont();
    if (strlen(text) == 0) {
      return;
    }

    ColorPalette* cp = drawContext->getColorPalette();
    uint16_t primaryColor = cp->get(COLOR_BALLOON_FOREGROUND);
    uint16_t backgroundColor = cp->get(COLOR_BALLOON_BACKGROUND);

    spi->setTextSize(TEXT_SIZE);
    spi->setTextDatum(MC_DATUM);

    M5.Lcd.setFont(font);
    M5.Lcd.setTextSize(TEXT_SIZE);

    // 1. 吹き出し幅を 260px に拡大し、右側に大きく延長
    // (左端 X=40 〜 右端 X=300)
    int boxHeight = 36;
    int boxWidth = 260;

    int x = 40;
    int y = cy - (boxHeight / 2);
    int radius = boxHeight / 2; // カプセル形状

    // 2. 三角形（しっぽ）の位置を右側（X=195）へ移動
    int arrowX = 195;

    // 外枠の精密計算
    int outerX = x - 2;
    int outerY = y - 2;
    int outerW = boxWidth + 4;
    int outerH = boxHeight + 4;
    int outerR = outerH / 2;

    // 3. 外枠（黒枠）の描画
    spi->fillRoundRect(outerX, outerY, outerW, outerH, outerR, primaryColor);
    spi->fillTriangle(arrowX - 8, y,
                      arrowX + 8, y,
                      arrowX,     y - 8,
                      primaryColor);

    // 4. 内側（白背景）の描画
    spi->fillRoundRect(x, y, boxWidth, boxHeight, radius, backgroundColor);
    spi->fillTriangle(arrowX - 6, y + 2,
                      arrowX + 6, y + 2,
                      arrowX,     y - 5,
                      backgroundColor);

    // 5. テキスト描画（拡大したカプセルの中心）
    int textCenterX = x + (boxWidth / 2);
    spi->setTextColor(primaryColor, backgroundColor);
    spi->drawString(text, textCenterX, cy, font);
  }
};

}  // namespace m5avatar

#endif  // BALLOON_H_