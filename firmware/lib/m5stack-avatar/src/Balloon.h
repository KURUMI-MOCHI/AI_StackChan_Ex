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
const float TEXT_SIZE = 1.5f;   // フォントサイズのみ少し小さめに変更
const int16_t MIN_WIDTH = 110;  // 最小幅
const int16_t MAX_WIDTH = 190;  // 右端の上限幅
const int cy = 220;             // 画面下部

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
    int textWidth = M5.Lcd.textWidth(text);

    // 吹き出しサイズ（縦幅は34pxのまま維持）
    int boxHeight = 34;
    int boxWidth = textWidth + 28;
    if (boxWidth < MIN_WIDTH) boxWidth = MIN_WIDTH;
    if (boxWidth > MAX_WIDTH) boxWidth = MAX_WIDTH;

    int x = 110; // 左端固定（右へ伸びる）
    int y = cy - (boxHeight / 2);
    int radius = boxHeight / 2; // カプセル形状

    int arrowX = 160; // △は口の直下（画面中央）

    // 1. 外枠（黒枠）の描画
    spi->fillRoundRect(x - 2, y - 2, boxWidth + 4, boxHeight + 4, radius + 2, primaryColor);
    spi->fillTriangle(arrowX - 8, y,
                      arrowX + 8, y,
                      arrowX,     y - 8,
                      primaryColor);

    // 2. 内側（白背景）の描画
    spi->fillRoundRect(x, y, boxWidth, boxHeight, radius, backgroundColor);
    spi->fillTriangle(arrowX - 6, y + 2,
                      arrowX + 6, y + 2,
                      arrowX,     y - 5,
                      backgroundColor);

    // 3. テキスト描画
    int textCenterX = x + (boxWidth / 2);
    spi->setTextColor(primaryColor, backgroundColor);
    spi->drawString(text, textCenterX, cy, font);
  }
};

}  // namespace m5avatar

#endif  // BALLOON_H_