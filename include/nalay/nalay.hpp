#pragma once

#include <cassert>
#include <cctype>
#include <chrono>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <print>
#include <stdexcept>
#include <string>
#include <variant>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>
#include <iostream>

#include <absl/container/flat_hash_map.h>

#include "allocators.hpp"
#include "providers.hpp"
#include "defaults.hpp"



namespace nalay {

template<typename T>
struct is_string_type : std::false_type {};

template<typename CharT, typename Traits, typename Alloc>
struct is_string_type<std::basic_string<CharT, Traits, Alloc>> : std::true_type {};

static thread_local context* nalay_ctx = nullptr;

enum class render_cmd_type : uint8_t {
  img,
  rect,
  text,
  svg
};

enum class alignment : uint8_t {
  none,
  left,
  right,
  center,
  top,
  bottom
};

enum class text_alignment : uint8_t {
  left,
  right,
  center,
  stretch
};


enum class interactivity : uint8_t {
  none       = 0,
  focusable  = 1 << 0,
  clickable  = 1 << 1,
  hoverable  = 1 << 2,
};


template <typename Signature>
struct event_listener;

template <typename Ret, typename... Args>
struct event_listener<Ret(Args...)> {
  std::vector<std::function<Ret(Args...)>> listeners;
 
  template <typename... T>
  auto notify_all(T&&... args) const -> void { for(const auto& listener : listeners) { listener(std::forward<T>(args)...); } }
  template <typename T>
  auto add(T&& functor) -> void { listeners.emplace_back(std::forward<T>(functor)); }
};

struct style {
  std::optional<insets> padding;
  std::optional<insets> margin;
  std::optional<int> border_size;
  std::optional<nalay::color> border_color;
  std::optional<nalay::color> background_color;
  std::optional<nalay::color> color;
  std::optional<std::pair<nalay::alignment, nalay::alignment>> alignment;
  std::optional<nalay::text_alignment> text_alignment;
  std::optional<std::reference_wrapper<const nalay::font>> display_font;
  std::optional<vec2i> size;
  std::optional<int> border_radius;
  std::optional<int> letter_spacing;
  std::optional<int> font_size;
};

struct resolved_style {
  insets padding;
  int    border_size;
  int    border_radius;

  std::reference_wrapper<const nalay::font> font;
  int font_size;
  int letter_spacing;
  nalay::text_alignment text_alignment;

  nalay::color color;
  nalay::color background_color;
  nalay::color border_color;

  std::optional<vec2i>                                    size;
  std::optional<std::pair<nalay::alignment, nalay::alignment>> alignment;
};

namespace primitive {

struct text {
  std::reference_wrapper<const nalay::font> font;
  std::string text;
  color text_color;
  color outline_color;
  vec2i size;
  vec2i pos;
  int outline_thickness;
  int font_size;
  int letter_spacing;
};

struct img {
  std::span<char> bytes;
  int format;
};

struct rect {
  nalay::color color;
  nalay::color border_color;
  int border_radius;
  int border_size;
  vec2i size;
};

} // namespace primitive

struct render_cmd {
  std::variant<
  primitive::text,
  primitive::img,
  primitive::rect
  > content;
  vec2i pos;
};

namespace ui { struct layout; }

struct render_queue {
  render_queue()                              = default;
  ~render_queue()                             = default;
  render_queue(const render_queue&)           = delete;
  render_queue(render_queue&&)                = delete;
  auto operator=(const render_queue&) -> void = delete;
  auto operator=(render_queue&&)      -> void = delete;

  auto emplace(auto&&... args) -> render_cmd* { return m_arena.make(std::forward<decltype(args)>(args)...); }
  void for_each(auto&& func) const { m_arena.for_each(std::forward<decltype(func)>(func)); }
  void reset() { m_arena.reset(); }

private:
  arena<render_cmd> m_arena;
};

using renderer      = renderer_<render_cmd>;
using margin        = insets;
using padding       = insets;
using border_radius = insets;

static constexpr auto operator|(interactivity a, interactivity b) -> interactivity;
static constexpr auto operator&(interactivity a, interactivity b) -> bool;
[[maybe_unused]] static auto get_ctx() -> context*;
[[maybe_unused]] static auto focused_node() -> ui::node*;

[[maybe_unused]] static void request_focus(ui::node* node);
[[maybe_unused]] static void clear_focus();
[[maybe_unused]] static void set_ctx(context* ctx);

template <typename T> requires std::derived_from<std::remove_cvref_t<T>, ui::layout>
[[maybe_unused]] void poll(T&& root);

namespace ui {

struct node {

  friend class layout;


  auto clicked(this auto&& self, auto&& func)  { self.m_click_listener.add(std::forward<decltype(func)>(func)); return self; }
  auto hovered(this auto&& self, auto&& func)  { self.m_hover_listener.add(std::forward<decltype(func)>(func)); return self; }
  auto focused(this auto&& self, auto&& func)  { self.m_focus_listener.add(std::forward<decltype(func)>(func)); return self; }
  auto unfocused(this auto&& self, auto&& func)  { self.m_unfocus_listener.add(std::forward<decltype(func)>(func)); return self; }

  auto id(this auto&& self, std::string id)            { self.m_id    = std::move(id); return self; }
  auto style(this auto&& self, ::nalay::style s)       { self.m_style = s;             return self; }
  auto padding(this auto&& self, insets p)             { self.m_style.padding = p;          return self; }
  auto margin(this auto&& self, insets m)              { self.m_style.margin  = m;          return self; }
  auto bg_color(this auto&& self, ::nalay::color c)    { self.m_style.background_color = c; return self; }
  auto color(this auto&& self, ::nalay::color c)       { self.m_style.color = c;            return self; }
  auto size(this auto&& self, int w, int h)            { self.m_style.size = vec2i{w, h}; return self; }
  auto border_radius(this auto&& self, int r)          { self.m_style.border_radius = r;    return self; }
  auto font_size(this auto&& self, int fs)             { self.m_style.font_size = fs;       return self; }
  auto display_font(this auto&& self,
                    std::reference_wrapper<const nalay::font> f) { self.m_style.display_font = f; return self; }
  auto align(this auto&& self,
             std::pair<::nalay::alignment, ::nalay::alignment> a) { self.x_align(a.first); self.y_align(a.second); return self; }

  auto x_align(this auto&& self, ::nalay::alignment a) {
    if (a == ::nalay::alignment::top || a == ::nalay::alignment::bottom)
      throw std::invalid_argument("x_align cannot be top or bottom");
    self.m_style.alignment = { a, self.m_style.alignment.value_or({}).second };
    return self;
  }
  auto y_align(this auto&& self, ::nalay::alignment a) {
    if (a == ::nalay::alignment::left || a == ::nalay::alignment::right)
      throw std::invalid_argument("y_align cannot be left or right");
    self.m_style.alignment = { self.m_style.alignment.value_or({}).first, a };
    return self;
  }

  auto interactive(this auto&& self, interactivity flags) { self.m_interactivity = flags; return self; }

  auto is_focused()    const -> bool         { return nalay_ctx->focused_node == this; }
  auto is_hovered()    const -> bool         { return m_hovered; }
  auto can_focus()     const -> bool         { return m_interactivity & interactivity::focusable; }
  auto can_click()     const -> bool         { return m_interactivity & interactivity::clickable; }
  auto can_hover()     const -> bool         { return m_interactivity & interactivity::hoverable; }
  
  auto computed_size() const -> vec2i { return m_size; }
  auto computed_pos()  const -> vec2i { return m_pos;  }

  virtual ~node() = default;
  virtual void measure() = 0;
  virtual void place(vec2i origin) { m_pos = origin; }
  virtual void emit(render_queue& queue) const = 0;
  virtual void on_focus() {}
  virtual void on_blur()  {}

  auto default_style() const -> nalay::style { return {}; }

  auto resolve(this auto const& self) -> resolved_style {
    auto def = self.default_style();
    return {
      .padding       = self.m_style.padding.or_else([&]{ return def.padding; }).value_or(insets{}),
      .border_size   = self.m_style.border_size.or_else([&]{ return def.border_size; }).value_or(0),
      .border_radius = self.m_style.border_radius.or_else([&]{ return def.border_radius; }).value_or(0),

      .font           = self.m_style.display_font.or_else([&]{ return def.display_font; }).value_or(nalay_ctx->fonts.get().default_font()),
      .font_size      = self.m_style.font_size.or_else([&]{ return def.font_size; }).value_or(defaults::font_size),
      .letter_spacing = self.m_style.letter_spacing.or_else([&]{ return def.letter_spacing; }).value_or(0),
      .text_alignment = self.m_style.text_alignment.or_else([&]{ return def.text_alignment; }).value_or(text_alignment::center),

      .color            = self.m_style.color.or_else([&]{ return def.color; }).value_or(color::black()),
      .background_color = self.m_style.background_color.or_else([&]{ return def.background_color; }).value_or(nalay::color{}),
      .border_color     = self.m_style.border_color.or_else([&]{ return def.border_color; }).value_or(nalay::color{}),

      .size      = self.m_style.size.or_else([&]{ return def.size; }),
      .alignment = self.m_style.alignment.or_else([&]{ return def.alignment; }),
    };
  }

protected:

  virtual void _on_click() {}
  virtual void _on_key(keyboard_event event) { (void) event; }

  virtual void poll() {
    auto inputs = nalay_ctx->inputs;
    if (m_interactivity == interactivity::none) return;

    auto mp  = inputs.get().mouse_pos();
    bool hit = hit_test(mp);

    if (can_hover()) {
      m_hovered = hit;
      if (m_hovered) m_hover_listener.notify_all();
    }


    if (hit && can_click() && inputs.get().mouse_pressed(mouse_button::left)) {
      if (can_focus()) {
        request_focus(this);
        m_focus_listener.notify_all();
      }
      _on_click();
      m_click_listener.notify_all();
    }
    if (!hit && inputs.get().mouse_pressed(mouse_button::left) && is_focused()) {
      clear_focus();
      m_unfocus_listener.notify_all();
    }

    if (not is_focused()) return;

    for (auto event : inputs.get().events()) {
      if (auto* kev = std::get_if<keyboard_event>(&event)) {
        _on_key(*kev);
      }
    }
  }

  void compute(render_queue& queue) { measure(); place({0, 0}); emit(queue); }

  void emit_background(render_queue& queue, const resolved_style& rs) const {
    queue.emplace(
      primitive::rect{
        .color         = rs.background_color,
        .border_color  = rs.border_color,
        .border_radius = rs.border_radius,
        .border_size   = rs.border_size,
        .size          = m_size,
      },
      m_pos
    );
  }

  void emit_text(render_queue& queue, const std::string& text, const resolved_style& rs) const {
    auto text_sz = rs.font.get().measure(text, rs.font_size, rs.letter_spacing);

    vec2i inner_pos{
      m_pos.x + rs.padding.get_left(),
      m_pos.y + rs.padding.get_top()
    };
    vec2i inner_sz{
      m_size.x - rs.padding.get_left() - rs.padding.get_right(),
      m_size.y - rs.padding.get_top()  - rs.padding.get_bottom()
    };

    vec2i alignment_offset{};

    if (rs.text_alignment == text_alignment::center) {
      alignment_offset = vec2i{ (inner_sz.x - text_sz.x) / 2, 0 };
    } else if (rs.text_alignment == text_alignment::right) {
      alignment_offset = vec2i{ inner_sz.x - text_sz.x, 0 };
    }

    vec2i text_pos{
      inner_pos.x + alignment_offset.x,
      inner_pos.y
    };

    queue.emplace(
      primitive::text{
        .font              = rs.font,
        .text              = text,
        .text_color        = rs.color,
        .outline_color     = rs.border_color,
        .size              = text_sz,
        .pos               = m_pos,
        .outline_thickness = 0,
        .font_size         = rs.font_size,
        .letter_spacing    = rs.letter_spacing,
      },
      text_pos
    );
  }
 
  auto hit_test(vec2i mp) const -> bool {
    return mp.x >= m_pos.x && mp.x < m_pos.x + m_size.x
    && mp.y >= m_pos.y && mp.y < m_pos.y + m_size.y;
  }

  auto measure_text_size(this auto const& self, const std::string& text) -> vec2i {
    auto rs = self.resolve();
    return rs.font.get().measure(text, rs.font_size, rs.letter_spacing);
  }

  auto measure_text_with_padding(this auto const& self, const std::string& text) -> vec2i {
    auto rs      = self.resolve();
    auto text_sz = rs.font.get().measure(text, rs.font_size, rs.letter_spacing);
    return text_sz + vec2i{
      rs.padding.get_left() + rs.padding.get_right(),
      rs.padding.get_top()  + rs.padding.get_bottom()
    };
  }

  nalay::style  m_style;
  event_listener<void()> m_click_listener;
  event_listener<void()> m_hover_listener;
  event_listener<void()> m_focus_listener;
  event_listener<void()> m_unfocus_listener;
  std::string   m_id;
  vec2i         m_size{};
  vec2i         m_pos{};
  interactivity m_interactivity{
    interactivity::focusable |
    interactivity::clickable |
    interactivity::hoverable
  };
  bool          m_hovered{false};
};

template <typename T> concept UIElement = std::derived_from<std::remove_cvref_t<T>, node>;

template <typename T> requires (std::is_arithmetic_v<T> or std::same_as<T, std::string>)
struct input : public node {
  T m_value{};
  T m_placeholder{};

  auto placeholder(this auto&& self, T&& placeholder) { self.m_placeholder = std::forward<T>(placeholder); return self; }
  auto value_changed(this auto&& self, auto&& func)  { self.m_changed_listeners.add(std::forward<decltype(func)>(func)); return self; }

  auto default_style() const -> nalay::style {
    return {
      .padding          = insets{ defaults::button_padding.x, defaults::button_padding.y },
      .border_size      = defaults::button_border_size,
      .border_color     = defaults::button_border_color,
      .background_color = defaults::button_background,
      .color            = defaults::button_foreground,
      .text_alignment   = text_alignment::left,
      .border_radius    = defaults::button_border_radius,
    };
  }

  void erase_character() {
    if (!m_value.empty()) {
       do { m_value.pop_back(); }
       while (!m_value.empty() && (m_value.back() & 0xC0) == 0x80);
    }
  }

  auto value() const -> const T& { return m_value; }
  auto clear() { m_value = {}; }

  void poll() override {
    node::poll();
    auto inputs = nalay_ctx->inputs;
    if (not inputs.get().key_down(nalay::key::backspace)) {
      m_pressed_timer = 0.0f;
      return;
    }
    if (m_pressed_timer < 0.5f) {
      m_pressed_timer += nalay_ctx->delta_time;
      return;
    }
    if (m_erased_timer > .01f) {
      erase_character();
      m_erased_timer = 0.0f;
    } else {
      m_erased_timer += nalay_ctx->delta_time;
    }
  }

  void _on_key(keyboard_event event) override {
    if constexpr (std::is_arithmetic_v<T>) {
      if (not std::isdigit(static_cast<char>(event.key))) return;
      if (m_value >= (std::numeric_limits<T>::max() - event.key) / 10) {
        m_value = std::numeric_limits<T>::max();
      } else {
        m_value = m_value * 10 + static_cast<T>(static_cast<char>(event.key) - '0');
      }
      m_changed_listeners.notify_all(m_value);
    }
    else if constexpr (std::same_as<T, std::string>) {
      if (event.key == nalay::key::backspace) { erase_character(); return; }
      
      auto padding = m_style.padding.value_or({});
      auto container_width =  m_style.size.value().x;

      if (measure_text_size(m_value).x + 2 * m_style.font_size.value_or(defaults::font_size) > container_width) { return; }
      if (event.codepoint >= 0x20) {
        if (event.codepoint < 0x80) {
          m_value += static_cast<char>(event.codepoint);
        } else if (event.codepoint < 0x800) {
          m_value += static_cast<char>(0xC0 | (event.codepoint >> 6));
          m_value += static_cast<char>(0x80 | (event.codepoint & 0x3F));
        } else if (event.codepoint < 0x10000) {
          m_value += static_cast<char>(0xE0 | (event.codepoint >> 12));
          m_value += static_cast<char>(0x80 | ((event.codepoint >> 6) & 0x3F));
          m_value += static_cast<char>(0x80 | (event.codepoint & 0x3F));
        } else {
          m_value += static_cast<char>(0xF0 | (event.codepoint >> 18));
          m_value += static_cast<char>(0x80 | ((event.codepoint >> 12) & 0x3F));
          m_value += static_cast<char>(0x80 | ((event.codepoint >> 6) & 0x3F));
          m_value += static_cast<char>(0x80 | (event.codepoint & 0x3F));
        }
      }

      m_changed_listeners.notify_all(std::ref(m_value));
    }
  }

  void _on_click() override {
    //Set carret
  }

  void measure() override {
    auto rs = resolve();
    if (rs.size.has_value()) { m_size = rs.size.value(); return; }
    if constexpr(std::same_as<T, std::string>) {
      auto text_sz = measure_text_size(m_value);
      m_size = text_sz + vec2i{
        rs.padding.get_left() + rs.padding.get_right(),
        rs.padding.get_top()  + rs.padding.get_bottom()
      };
    } else {
      auto num_digits = std::floor(std::log10(std::abs(m_value))) + 1;
      auto font_size  = m_style.font_size.value_or(defaults::font_size);
      auto text_sz    = vec2i { font_size * num_digits, font_size };
      m_size = text_sz + vec2i{
        rs.padding.get_left() + rs.padding.get_right(),
        rs.padding.get_top()  + rs.padding.get_bottom()
      };
    }
  }

  void emit(render_queue& queue) const override {
    auto rs = resolve();
    emit_background(queue, rs);
    if constexpr (std::same_as<T, std::string>) {
      auto padding = m_style.padding.value_or({});
      auto container_width =  m_style.size.value().x;
      if (measure_text_size(m_value).x + 2 * m_style.font_size.value_or(defaults::font_size) > container_width) {
        emit_text(queue, m_value.substr(0, m_value.size() - 3) + "..", rs);
      } else {
        emit_text(queue, m_value, rs);
      }
    } else {
      emit_text(queue, std::to_string(m_value), rs);
    }
  }
private:
  event_listener<void(T)> m_changed_listeners;

  float m_pressed_timer = .0f;
  float m_erased_timer  = .0f;
};

struct button : public node {
  std::string text;
  explicit button(std::string s) : text(std::move(s)) {}

auto default_style() const -> nalay::style {
  return {
    .padding          = insets{ defaults::button_padding.x, defaults::button_padding.y },
    .margin           = std::nullopt,
    .border_size      = defaults::button_border_size,
    .border_color     = defaults::button_border_color,
    .background_color = defaults::button_background,
    .color            = defaults::button_foreground,
    .alignment        = std::nullopt,
    .text_alignment   = text_alignment::center,
    .display_font     = std::nullopt,
    .size             = std::nullopt,
    .border_radius    = defaults::button_border_radius,
    .letter_spacing   = std::nullopt,
    .font_size        = std::nullopt,
  };
}

  void measure() override {
    auto rs = resolve();
    if (rs.size.has_value()) { m_size = rs.size.value(); return; }
    auto text_sz = measure_text_size(text);
    m_size = text_sz + vec2i{
      rs.padding.get_left() + rs.padding.get_right(),
      rs.padding.get_top()  + rs.padding.get_bottom()
    };
  }

  void emit(render_queue& queue) const override {
    auto rs = resolve();
    emit_background(queue, rs);
    emit_text(queue, text, rs);
  }
};

struct label : public node {
  std::string text;
  explicit label(std::string s) : text(std::move(s)) {}

  void measure() override {
    auto rs = resolve();
    if (rs.size.has_value()) { m_size = rs.size.value(); return; }
    m_size = measure_text_with_padding(text);
  }

  void emit(render_queue& queue) const override {
    auto rs = resolve();
    emit_background(queue, rs);
    emit_text(queue, text, rs);
  }
};

struct layout : public node {
  enum class direction { vertical, horizontal };

  using child_list = std::vector<node*, slab_allocator<node*>>;

  explicit layout(direction dir)
    : dir_(dir)
    , components(slab_allocator<node*>{ nalay_ctx->node_allocator }) {}

  explicit layout(direction dir, auto&&... children)
    : dir_(dir)
    , components(slab_allocator<node*>{ nalay_ctx->node_allocator })
  {
    (components.push_back(
      nalay_ctx->make_node<std::decay_t<decltype(children)>>(
        std::forward<decltype(children)>(children))
    ), ...);
  }

  void add(node& child) { components.push_back(&child); }

  template <UIElement T> requires (!std::is_reference_v<T>)
  void add(T&& child) { components.push_back(nalay_ctx->make_node<T>(std::forward<T>(child))); }

  void measure() override {
    for (node* child : components)
    child->measure();

    auto rs = resolve();
    if (rs.size.has_value()) { m_size = rs.size.value(); return; }

    auto  content = compute_children_extent();
    auto& pad     = rs.padding;

    m_size = content + vec2i{
      pad.get_left() + pad.get_right(),
      pad.get_top()  + pad.get_bottom()
    };
  }

  void place(vec2i origin) override {
    m_pos = origin;

    auto rs         = resolve();
    auto& pad       = rs.padding;
    auto  layout_al = rs.alignment.value_or({});

    vec2i content_origin{
      m_pos.x + pad.get_left(),
      m_pos.y + pad.get_top()
    };
    vec2i content_size{
      m_size.x - pad.get_left() - pad.get_right(),
      m_size.y - pad.get_top()  - pad.get_bottom()
    };

    vec2i children_extent = compute_children_extent();
    vec2i cursor          = initial_cursor(layout_al, content_size, children_extent);

    for (node* child : components) {
      vec2i cs    = child->computed_size();
      vec2i cross = cross_offset(layout_al, content_size, cs);
      child->place(content_origin + cursor + cross);

      if (dir_ == direction::vertical)
        cursor.y += cs.y + defaults::padding;
      else
        cursor.x += cs.x + defaults::padding;
    }
  }

  void emit(render_queue& queue) const override {
    auto rs = resolve();
    emit_background(queue, rs);
    for (auto* child : components) child->emit(queue);
  }

  void compute(render_queue& queue) { node::compute(queue); }

  template <typename T> requires std::derived_from<std::remove_cvref_t<T>, layout>
  friend void nalay::poll(T&& root);

protected:
  void poll() override {
    node::poll();
    for (auto* child : components) { child->poll(); }
  }

private:
  auto compute_children_extent() const -> vec2i {
    vec2i extent{};
    for (node* child : components) {
      vec2i cs = child->computed_size();
      if (dir_ == direction::vertical) {
        extent.y += cs.y + defaults::padding;
        extent.x  = std::max(extent.x, cs.x);
      } else {
        extent.x += cs.x + defaults::padding;
        extent.y  = std::max(extent.y, cs.y);
      }
    }
    return extent;
  }

  auto initial_cursor(
    std::pair<alignment, alignment> alignment,
    vec2i content_size,
    vec2i children_extent
  ) const -> vec2i {
    vec2i cursor{};
    if (dir_ == direction::vertical) {
      if      (alignment.second == alignment::bottom) cursor.y = content_size.y - children_extent.y;
      else if (alignment.second == alignment::center) cursor.y = (content_size.y - children_extent.y) / 2;
    } else {
      if      (alignment.first == alignment::right)  cursor.x = content_size.x - children_extent.x;
      else if (alignment.first == alignment::center) cursor.x = (content_size.x - children_extent.x) / 2;
    }
    return cursor;
  }

  auto cross_offset(
    std::pair<alignment, alignment> alignment,
    vec2i content_size,
    vec2i child_size
  ) const -> vec2i {
    vec2i cross{};
    if (dir_ == direction::vertical) {
      if      (alignment.first == alignment::right)  cross.x = content_size.x - child_size.x;
      else if (alignment.first == alignment::center) cross.x = (content_size.x - child_size.x) / 2;
    } else {
      if      (alignment.second == alignment::bottom) cross.y = content_size.y - child_size.y;
      else if (alignment.second == alignment::center) cross.y = (content_size.y - child_size.y) / 2;
    }
    return cross;
  }

  direction  dir_;
  child_list components;
};

template <UIElement... Elements>
struct columns final : public layout {
  columns(Elements&&... args) : layout(direction::horizontal, std::forward<Elements&&>(args)...) {}
};

template <UIElement... Elements>
struct rows final : public layout {
  rows(Elements&&... args) : layout(direction::vertical, std::forward<Elements&&>(args)...) {}
};

} // namespace ui

static auto get_ctx() -> context* { return nalay_ctx; }
static void set_ctx(context* ctx)  { nalay_ctx = ctx;  }

static void request_focus(ui::node* node) {
  if (nalay_ctx->focused_node == node) return;
  if (nalay_ctx->focused_node) nalay_ctx->focused_node->on_blur();
  nalay_ctx->focused_node = node;
  if (nalay_ctx->focused_node) nalay_ctx->focused_node->on_focus();
}

static void clear_focus() { request_focus(nullptr); }
static auto focused_node() -> ui::node* { return nalay_ctx->focused_node; }
static constexpr auto operator|(interactivity a, interactivity b) -> interactivity { return static_cast<interactivity>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b)); }
static constexpr auto operator&(interactivity a, interactivity b) -> bool { return static_cast<uint8_t>(a) & static_cast<uint8_t>(b); }

template <typename T> requires std::derived_from<std::remove_cvref_t<T>, ui::layout>
void poll(T&& root) { std::forward<T>(root).poll(); }

} // namespace nalay
