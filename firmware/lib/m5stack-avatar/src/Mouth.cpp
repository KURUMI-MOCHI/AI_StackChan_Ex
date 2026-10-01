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

  // 口の可変サイズ定義（閉じ口の高さは4pxに保持）
  const float closedWidth  = 90.0f;
  const float openWidth    = 60.0f;
  const float closedHeight = 4.0f;  // ★ 4pxに設定
  const float openHeight   = 50.0f; // 製品版の開き高さ
  const float maxRadius    = 16.0f;

  int w = (int)(closedWidth - (closedWidth - openWidth) * openRatio);
  int h = (int)(closedHeight + (openHeight - closedHeight) * openRatio);
  int r = (int)(maxRadius * openRatio);

  int x = rect.getCenterX() - (w / 2);
  int y = rect.getCenterY() - (h / 2);

  if (r > 0) {
    spi->fillRoundRect(x, y, w, h, r, primaryColor);
  } else {
    spi->fillRect(x, y, w, h, primaryColor);
  }
}

}  // namespace m5avatar