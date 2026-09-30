// Copyright (c) Shinya Ishikawa. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full
// license information.

#include "Eyeblow.h"

namespace m5avatar {

Eyeblow::Eyeblow(uint16_t w, uint16_t h, bool isLeft)
    : width{w}, height{h}, isLeft{isLeft} {}

void Eyeblow::draw(M5Canvas *spi, BoundingRect rect, DrawContext *ctx) {
  // 描画処理を行わずに即時リターン
  // クラスの存在や draw() の呼び出しチェーンは維持されるため、レイアウト崩れや状態不整合を防ぎつつ眉毛だけを消去できます
  return;
}

}  // namespace m5avatar