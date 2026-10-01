// Copyright (c) Shinya Ishikawa. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full
// license information.

#include "Mouth.h"

namespace m5avatar {

Mouth::Mouth(uint16_t minWidth, uint16_t maxWidth, uint16_t minHeight,
             uint16_t maxHeight)
    : minWidth{minWidth},
      maxWidth{maxWidth},
      minHeight{minHeight},
      maxHeight{maxHeight} {}

void Mouth::draw(M5Canvas *spi, BoundingRect rect, DrawContext *ctx) {
  uint16_t primaryColor = ctx->getColorDepth() == 1 ? 1 : ctx->getColorPalette()->get(COLOR_PRIMARY);
  float openRatio = ctx->getMouthOpenRatio(); // 0.0 〜 1.0

  // CustomMouth と同等の可変計算（閉: 横60/縦4px線 -> 開: 横40/縦32px角丸）
  int w = 60 - (int)(20.0f * openRatio);
  int h = 4 + (int)(28.0f * openRatio);
  int r = (int)(10.0f * openRatio);

  int x = rect.getCenterX() - (w / 2);
  int y = rect.getCenterY() - (h / 2);

  if (r > 0) {
    spi->fillRoundRect(x, y, w, h, r, primaryColor);
  } else {
    spi->fillRect(x, y, w, h, primaryColor);
  }
}

}  // namespace m5avatar