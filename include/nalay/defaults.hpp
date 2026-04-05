#pragma once

#include "types.hpp"

namespace nalay::defaults {

// ── layout ─────────────────────────────────────────────────────────────────
static constexpr auto child_spacing        = vec2i{ 16, 16 };
static constexpr auto padding              = 8;

// ── typography ─────────────────────────────────────────────────────────────
static constexpr auto font_size            = 18;

// ── button ─────────────────────────────────────────────────────────────────
static constexpr auto button_padding       = vec2i{ 16, 12 };
static constexpr auto button_border_radius = 64;
static constexpr auto button_border_size   = 1;
static constexpr auto button_border_color  = color::from_hex(0x1e2021ff);
static constexpr auto button_background    = color::from_hex(0xffffffff);
static constexpr auto button_foreground    = color::from_hex(0x1e1011ff);

// ── input (text / number) ─────────────────────────────────────────────────
static constexpr auto input_background     = color::from_hex(0xffffffff);
static constexpr auto input_border_color   = color::from_hex(0xd0d3d4ff);
static constexpr auto input_border_radius  = 6;
static constexpr auto input_border_size    = 1;
static constexpr auto input_padding        = vec2i{ 12, 8 };

// ── focus ──────────────────────────────────────────────────────────────────
static constexpr auto focus_border_color   = color::from_hex(0x0078d4ff);  // blue focus ring

// ── cursor ─────────────────────────────────────────────────────────────────
static constexpr auto cursor_width         = 1;

// ── toggle (bool) ────────────────────────────────────────────────────
static constexpr auto toggle_on_color      = color::from_hex(0x0078d4ff);
static constexpr auto toggle_off_color     = color::from_hex(0xe1e4e5ff);
static constexpr auto toggle_border_radius = 64;

} // namespace nalay::defaults
