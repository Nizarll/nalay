#pragma once

#include "types.hpp"

namespace nalay::defaults {
  static constexpr auto child_spacing        = vec2{16, 16};
  static constexpr auto padding              = 8;
  static constexpr auto font_size            = 18;
  static constexpr auto button_padding       = vec2i{ 16, 12 };
  static constexpr auto button_border_radius = 64;
  static constexpr auto button_border_size   = 1;
  static constexpr auto button_border_color  = color::from_hex(0x1e2021ff);
  static constexpr auto button_background    = color::from_hex(0xffffffff);
} // namespace nalay::defaults

