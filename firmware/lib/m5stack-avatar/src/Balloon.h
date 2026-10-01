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
const int16_t TEXT_SIZE = 2;
const int16_t MIN_WIDTH = 100;
const int16_t MAX_WIDTH = 280;
const int cx = 160;  // 画面水平中央
const int cy = 220;  // 元のコードと同じY座標（画面最下部）

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

    // カプセルのサイズ計算
    int boxHeight = 34;
    int boxWidth = textWidth + 24;
    if (boxWidth < MIN_WIDTH) boxWidth = MIN_WIDTH;
    if (boxWidth > MAX_WIDTH) boxWidth = MAX_WIDTH;

    int x = cx - (boxWidth / 2);
    int y = cy - (boxHeight / 2);
    int radius = boxHeight / 2; // 完全なカプセル形状

    // 1. 外枠（primaryColor：枠線・シッポ枠）
    spi->fillRoundRect(x - 2, y - 2, boxWidth + 4, boxHeight + 4, radius + 2, primaryColor);
    spi->fillTriangle(cx - 8, y,
                      cx + 8, y,
                      cx,     y - 8,
                      primaryColor);

    // 2. 内側の塗りつぶし（backgroundColor：白背景）
    spi->fillRoundRect(x, y, boxWidth, boxHeight, radius, backgroundColor);
    spi->fillTriangle(cx - 6, y + 2,
                      cx + 6, y + 2,
                      cx,     y - 5,
                      backgroundColor);

    // 3. テキスト描画（カプセル中央）
    spi->setTextColor(primaryColor, backgroundColor);
    spi->drawString(text, cx, cy, font);
  }
};

}  // namespace m5avatar

#endif  // BALLOON_H_