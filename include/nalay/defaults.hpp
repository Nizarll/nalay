#pragma once

#include "types.hpp"

//struct style {
//  std::optional<insets>       padding;
//  std::optional<insets>       margin;
//  std::optional<::color>      background_color;
//  std::optional<::color>      color;
//  std::optional<::alignment>  alignment;
//  std::optional<vec2i>        size;
//  std::optional<insets>       border_radius;
//  std::optional<int>          font_size;
//  std::optional<std::reference_wrapper<const font>> display_font;
//};

namespace defaults {
  static constexpr auto child_spacing        = vec2{16, 16};
  static constexpr auto padding              = 8;
  static constexpr auto font_size            = 18;
  static constexpr auto button_padding       = vec2i{ 16, 12 };
  static constexpr auto button_border_radius = 64;
  static constexpr auto button_border_size   = 1;
  static constexpr auto button_border_color  = color::from_hex(0x1e2021ff);
  static constexpr auto button_background    = color::from_hex(0xffffffff);
} // namespace defaults

