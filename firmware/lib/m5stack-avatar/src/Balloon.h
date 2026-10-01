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
const int16_t MIN_WIDTH = 120;  // 画像のようなスリムな横長感を出す最小幅
const int16_t MAX_WIDTH = 280;  // 画面幅に収まる最大幅

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
    // primaryColor: 吹き出し本体（白）, backgroundColor: 文字色（黒）
    uint16_t primaryColor = cp->get(COLOR_BALLOON_FOREGROUND);
    uint16_t backgroundColor = cp->get(COLOR_BALLOON_BACKGROUND);

    spi->setTextSize(TEXT_SIZE);
    spi->setTextDatum(MC_DATUM);
    M5.Lcd.setFont(font);
    M5.Lcd.setTextSize(TEXT_SIZE);

    int textWidth = M5.Lcd.textWidth(text);

    // 1. 製品版画像に合わせたサイズと位置の計算
    int boxHeight = 38;                     // 画像のようなスマートな高さ
    int boxWidth = textWidth + 24;          // 横の余白
    if (boxWidth < MIN_WIDTH) boxWidth = MIN_WIDTH;
    if (boxWidth > MAX_WIDTH) boxWidth = MAX_WIDTH;

    int centerX = 160;                      // 画面水平中央
    int centerY = 150;                      // 口のすぐ下に配置

    int x = centerX - (boxWidth / 2);
    int y = centerY - (boxHeight / 2);
    int radius = boxHeight / 2;             // 完全なカプセル形状

    // 2. カプセル本体の描画
    spi->fillRoundRect(x, y, boxWidth, boxHeight, radius, primaryColor);

    // 3. 上面のシッポ（画像の口に向かう中央上向き矢印）
    int arrowX = centerX;                   // 中央（口のすぐ下）
    int arrowY = y;                         // 吹き出しの上面
    spi->fillTriangle(arrowX - 6, arrowY + 2, 
                      arrowX + 6, arrowY + 2, 
                      arrowX,     arrowY - 7, 
                      primaryColor);

    // 4. テキスト描画
    spi->setTextColor(backgroundColor, primaryColor);
    spi->drawString(text, centerX, centerY, font);
  }
};

}  // namespace m5avatar

#endif  // BALLOON_H_