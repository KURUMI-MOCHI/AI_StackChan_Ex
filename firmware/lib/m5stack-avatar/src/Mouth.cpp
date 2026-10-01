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

  // 1. 口のサイズ定義（画像に合わせて閉じ口を横長90pxに調整）
  const float closedWidth = 90.0f;  // 閉じている時の横幅（元は60）
  const float openWidth   = 55.0f;  // 開いた時の横幅
  const float closedHeight = 4.0f;  // 閉じている時の線の太さ
  const float openHeight   = 32.0f; // 開いた時の縦幅

  // 2. 口開度に応じた可変計算
  int w = (int)(closedWidth - (closedWidth - openWidth) * openRatio);
  int h = (int)(closedHeight + (openHeight - closedHeight) * openRatio);
  int r = (int)(10.0f * openRatio); // 開くにつれて角丸になる

  int x = rect.getCenterX() - (w / 2);
  int y = rect.getCenterY() - (h / 2);

  if (r > 0) {
    spi->fillRoundRect(x, y, w, h, r, primaryColor);
  } else {
    spi->fillRect(x, y, w, h, primaryColor);
  }
}

}  // namespace m5avatar