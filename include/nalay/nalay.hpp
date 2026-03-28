#pragma once

#include <cassert>
#include <concepts>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>
#include <string_view>
#include <cmath>

#include "allocators.hpp"
#include "providers.hpp"
#include "defaults.hpp"

namespace nalay {

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
enum class interactivity : uint8_t {
  none       = 0,
  focusable  = 1 << 0,
  clickable  = 1 << 1,
  hoverable  = 1 << 2,
};

using event_handler = nalay::function_variant<
std::function<void(mouse_event)>,
std::function<void(keyboard_event)>,
std::function<void()>
>;

struct style {
  std::optional<insets>   padding;
  std::optional<insets>   margin;
  std::optional<int>      border_size;
  std::optional<nalay::color> border_color;
  std::optional<nalay::color> background_color;
  std::optional<nalay::color> color;
  std::optional<std::pair<nalay::alignment, nalay::alignment>> alignment;
  std::optional<std::reference_wrapper<const font>> display_font;
  std::optional<nalay::vec2i> size;
  std::optional<int>      border_radius;
  std::optional<int>      letter_spacing;
  std::optional<int>      font_size;
};

namespace primitive {

struct text {
  std::reference_wrapper<const nalay::font> font;
  std::string     text;
  nalay::color    text_color;
  nalay::color    outline_color;
  nalay::vec2i    size;
  nalay::vec2i    pos;
  int             outline_thickness;
  int             font_size;
  int             letter_spacing;
};

struct img {
  std::span<char> bytes;
  int             format;
};

struct rect {
  nalay::color color;
  nalay::color border_color;
  int          border_radius;
  int          border_size;
  nalay::vec2i size;
};

} // namespace primitive

struct render_cmd {
  std::variant<primitive::text, primitive::img, primitive::rect> content;
  nalay::vec2i pos;
};

// ─── render_queue ─────────────────────────────────────────────────────────────

struct render_queue {
  render_queue() = default;
  ~render_queue() = default;
  render_queue(const render_queue&)           = delete;
  render_queue(render_queue&&)                = delete;
  auto operator=(const render_queue&) -> void = delete;
  auto operator=(render_queue&&)      -> void = delete;

  auto emplace(auto&&... args) -> render_cmd* {
    return m_arena.make(std::forward<decltype(args)>(args)...);
  }

  void for_each(auto&& func) const {
    m_arena.for_each(std::forward<decltype(func)>(func));
  }

  void reset() { m_arena.reset(); }

private:
  arena<nalay::render_cmd> m_arena;
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

// ─── ui nodes ─────────────────────────────────────────────────────────────────

namespace ui {

struct node {
  
//setters
  auto id(this auto&& self, std::string id)            { self.m_id    = std::move(id); return self; }
  auto style(this auto&& self, ::nalay::style s)       { self.m_style = s;             return self; }
  auto padding(this auto&& self, insets p)             { self.m_style.padding = p;          return self; }
  auto margin(this auto&& self, insets m)              { self.m_style.margin  = m;          return self; }
  auto bg_color(this auto&& self, ::nalay::color c)    { self.m_style.background_color = c; return self; }
  auto color(this auto&& self, ::nalay::color c)       { self.m_style.color = c;            return self; }
  auto size(this auto&& self, int w, int h)            { self.m_style.size = nalay::vec2i{w, h}; return self; }
  auto border_radius(this auto&& self, int r)          { self.m_style.border_radius = r;   return self; }
  auto font_size(this auto&& self, int fs)             { self.m_style.font_size = fs;       return self; }
  auto display_font(this auto&& self,
                    std::reference_wrapper<const font> f)              { self.m_style.display_font = f;     return self; }
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
  
//getters
  auto is_focused()  const -> bool { return nalay_ctx->focused_node == this; }
  auto is_hovered()  const -> bool { return m_hovered; }
  auto can_focus() const -> bool { return m_interactivity & interactivity::focusable; }
  auto can_click() const -> bool { return m_interactivity & interactivity::clickable; }
  auto can_hover() const -> bool { return m_interactivity & interactivity::hoverable; }
  auto computed_size() const -> nalay::vec2i { return m_size; }
  auto computed_pos()  const -> nalay::vec2i { return m_pos;  }

  virtual ~node() = default;
  virtual void measure() = 0;
  virtual void place(nalay::vec2i origin) { m_pos = origin; }
  virtual void emit(render_queue& queue) const = 0;
  virtual void on_focus() {}
  virtual void on_blur()  {}

  void poll() {
    auto inputs = nalay_ctx->inputs;
    if (m_interactivity == interactivity::none) return;

    const auto mp = inputs.get().mouse_pos();
    const bool hit = mp.x >= m_pos.x && mp.x < m_pos.x + m_size.x
                  && mp.y >= m_pos.y && mp.y < m_pos.y + m_size.y;

    if (can_hover()) m_hovered = hit;

    if (hit && can_click() && inputs.get().mouse_pressed(mouse_button::left)) {
      if (can_focus()) {
        request_focus(this);
      }
      on_click();
    }
    if (!hit && inputs.get().mouse_pressed(mouse_button::left) && is_focused()) {
      clear_focus();
    }

    if (not is_focused()) { return; }

    for (auto event : inputs.get().events()) {
      if (auto* kev = std::get_if<keyboard_event>(&event)) {
        on_key(*kev);
      }
    }
  }

  void compute(render_queue& queue) {
    measure();
    place({0, 0});
    emit(queue);
  }

protected:
  virtual void on_click() {}
  virtual void on_key(keyboard_event event)   { (void) event; }

  auto resolve_font() const -> std::reference_wrapper<const font> {
    return m_style.display_font.value_or(nalay_ctx->fonts.get().default_font());
  }

  auto resolve_padding() const -> insets {
    return m_style.padding.value_or(insets{});
  }

  auto measure_text_size(const std::string& text) const -> nalay::vec2i {
    return resolve_font().get().measure(
      text,
      m_style.font_size.value_or(defaults::font_size),
      m_style.letter_spacing.value_or(0)
    );
  }

  auto measure_text_with_padding(const std::string& text) const -> nalay::vec2i {
    const auto text_sz = measure_text_size(text);
    const auto pad     = resolve_padding();
    return text_sz + nalay::vec2i{
      pad.get_left() + pad.get_right(),
      pad.get_top()  + pad.get_bottom()
    };
  }

  void emit_background(render_queue& queue) const {
    queue.emplace(
      primitive::rect{
        .color         = m_style.background_color.value_or({}),
        .border_color  = m_style.border_color.value_or({}),
        .border_radius = m_style.border_radius.value_or(0),
        .border_size   = m_style.border_size.value_or(0),
        .size          = m_size,
      },
      m_pos
    );
  }

  void emit_text_centered(render_queue& queue, const std::string& text) const {
    const auto pad     = resolve_padding();
    const auto text_sz = measure_text_size(text);

    const nalay::vec2i inner_pos{
      m_pos.x + pad.get_left(),
      m_pos.y + pad.get_top()
    };
    const nalay::vec2i inner_sz{
      m_size.x - pad.get_left() - pad.get_right(),
      m_size.y - pad.get_top()  - pad.get_bottom()
    };
    const nalay::vec2i text_pos{
      inner_pos.x + (inner_sz.x - text_sz.x) / 2,
      inner_pos.y + (inner_sz.y - text_sz.y) / 2
    };

    queue.emplace(
      primitive::text{
        .font              = resolve_font(),
        .text              = text,
        .text_color        = m_style.color.value_or(color::black()),
        .outline_color     = m_style.border_color.value_or(color::black()),
        .size              = text_sz,
        .pos               = m_pos,
        .outline_thickness = 0,
        .font_size         = m_style.font_size.value_or(defaults::font_size),
        .letter_spacing    = m_style.letter_spacing.value_or(0),
      },
      text_pos
    );
  }

  nalay::style m_style;
  std::string m_id;
  vec2i m_size{};
  vec2i m_pos{};
  interactivity m_interactivity{interactivity::none};
  bool m_hovered{false};
};

template <typename T> concept UIElement = std::derived_from<T, node>;

// ─── button ───────────────────────────────────────────────────────────────────


struct button : public node {
  std::string text;
  explicit button(std::string s) : text(std::move(s)) {}

  void measure() override {
    if (m_style.size) { m_size = *m_style.size; return; }
    const auto pad = m_style.padding.value_or(
      insets{ defaults::button_padding.x, defaults::button_padding.y }
    );
    const auto text_sz = measure_text_size(text);
    m_size = text_sz + nalay::vec2i{
      pad.get_left() + pad.get_right(),
      pad.get_top()  + pad.get_bottom()
    };
  }

  void emit(render_queue& queue) const override {
    queue.emplace(
      primitive::rect{
        .color         = m_style.background_color.value_or(defaults::button_background),
        .border_color  = m_style.border_color.value_or(defaults::button_border_color),
        .border_radius = m_style.border_radius.value_or(defaults::button_border_radius),
        .border_size   = m_style.border_size.value_or(defaults::button_border_size),
        .size          = m_size,
      },
      m_pos
    );
    emit_text_centered(queue, text);
  }
};

// ─── label ────────────────────────────────────────────────────────────────────

struct label : public node {
  std::string text;
  explicit label(std::string s) : text(std::move(s)) {}

  void measure() override {
    if (m_style.size) { m_size = *m_style.size; return; }
    m_size = measure_text_with_padding(text);
  }

  void emit(render_queue& queue) const override {
    emit_background(queue);
    emit_text_centered(queue, text);
  }
};

// ─── layout ───────────────────────────────────────────────────────────────────

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

  void add(node& child)  { components.push_back(&child); }

  template <UIElement T> requires (!std::is_reference_v<T>)
  void add(T&& child) {
    components.push_back(
      nalay_ctx->make_node<T>(std::forward<T>(child))
    );
  }

  // measure bottom-up, then sum/max them.
  void measure() override {
    for (node* child : components)
    child->measure();

    if (m_style.size) { m_size = *m_style.size; return; }

    const auto pad = resolve_padding();
    nalay::vec2i content{};

    for (node* child : components) {
      const nalay::vec2i cs = child->computed_size();
      if (dir_ == direction::vertical) {
        content.x  = std::max(content.x, cs.x);
        content.y += cs.y + defaults::padding;
      } else {
        content.x += cs.x + defaults::padding;
        content.y  = std::max(content.y, cs.y);
      }
    }

    m_size = content + nalay::vec2i{
      pad.get_left() + pad.get_right(),
      pad.get_top()  + pad.get_bottom()
    };
  }

  // Pass 2: placement top down.
  void place(nalay::vec2i origin) override {
    m_pos = origin;

    const auto pad         = resolve_padding();
    const auto layout_al   = m_style.alignment.value_or({});

    const nalay::vec2i content_origin{
      m_pos.x + pad.get_left(),
      m_pos.y + pad.get_top()
    };
    const nalay::vec2i content_size{
      m_size.x - pad.get_left() - pad.get_right(),
      m_size.y - pad.get_top()  - pad.get_bottom()
    };

    nalay::vec2i children_extent{};
    for (node* child : components) {
      const nalay::vec2i cs = child->computed_size();
      if (dir_ == direction::vertical) {
        children_extent.y += cs.y + defaults::padding;
        children_extent.x  = std::max(children_extent.x, cs.x);
      } else {
        children_extent.x += cs.x + defaults::padding;
        children_extent.y  = std::max(children_extent.y, cs.y);
      }
    }

    nalay::vec2i cursor{};
    if (dir_ == direction::vertical) {
      if      (layout_al.second == alignment::bottom) cursor.y = content_size.y - children_extent.y;
      else if (layout_al.second == alignment::center) cursor.y = (content_size.y - children_extent.y) / 2;
    } else {
      if      (layout_al.first == alignment::right)  cursor.x = content_size.x - children_extent.x;
      else if (layout_al.first == alignment::center) cursor.x = (content_size.x - children_extent.x) / 2;
    }

    for (node* child : components) {
      const nalay::vec2i cs = child->computed_size();

      // Cross-axis alignment.
      nalay::vec2i cross{};
      if (dir_ == direction::vertical) {
        if      (layout_al.first == alignment::right)  cross.x = content_size.x - cs.x;
        else if (layout_al.first == alignment::center) cross.x = (content_size.x - cs.x) / 2;
      } else {
        if      (layout_al.second == alignment::bottom) cross.y = content_size.y - cs.y;
        else if (layout_al.second == alignment::center) cross.y = (content_size.y - cs.y) / 2;
      }

      child->place(content_origin + cursor + cross);

      if (dir_ == direction::vertical)
        cursor.y += cs.y + defaults::padding;
      else
        cursor.x += cs.x + defaults::padding;
    }
  }

  void poll() {
    node::poll();
    for (auto* child : components) {
      child->poll();
    }
  }

  void emit(render_queue& queue) const override {
    emit_background(queue);
    for (const node* child : components)
    child->emit(queue);
  }

  void compute(render_queue& queue) {   // root-level entry point
    measure();
    place({0, 0});
    emit(queue);
  }

private:
  direction  dir_;
  child_list components;
};

// ─── columns / rows ───────────────────────────────────────────────────────────

template <UIElement... Elements>
struct columns final : public layout {
  columns(Elements&&... args) : layout(direction::horizontal, args...) {}
};

template <UIElement... Elements>
struct rows final : public layout {
  rows(Elements&&... args) : layout(direction::vertical, args...) {}
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

static constexpr auto operator|(interactivity a, interactivity b) -> interactivity {
  return static_cast<interactivity>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

static constexpr auto operator&(interactivity a, interactivity b) -> bool {
  return static_cast<uint8_t>(a) & static_cast<uint8_t>(b);
}

} // namespace nalay
