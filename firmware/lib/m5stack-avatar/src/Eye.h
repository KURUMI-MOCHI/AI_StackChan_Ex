// Copyright (c) Shinya Ishikawa. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full
// license information.

#ifndef EYE_H_
#define EYE_H_

#define LGFX_USE_V1
#include <M5GFX.h>
#include "DrawContext.h"
#include "Drawable.h"
#include "stack_chan_led.h" // LED感情の定義を参照

namespace m5avatar {

class Eye final : public Drawable {
 private:
  uint16_t r;
  bool isLeft;

  // まばたきアニメーションと感情通知用の内部状態
  enum class BlinkState { IDLE, CLOSING, CLOSED, OPENING };
  BlinkState blinkState = BlinkState::IDLE;
  float currentRatio = 1.0f;
  Expression lastExp = static_cast<Expression>(-1);

 public:
  // constructor
  Eye() = delete;
  Eye(uint16_t x, uint16_t y, uint16_t r, bool isLeft); // deprecated
  Eye(uint16_t r, bool isLeft);
  ~Eye() = default;
  Eye(const Eye &other) = default;
  Eye &operator=(const Eye &other) = default;
  
  void draw(M5Canvas *spi, BoundingRect rect, DrawContext *drawContext) override;
};

} // namespace m5avatar

#endif // EYE_H_