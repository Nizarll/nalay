#pragma once

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

#include "nalay/types.hpp"
#include "providers.hpp"
#include "defaults.hpp"

namespace nalay {

template<typename T>
struct is_string_type : std::false_type {};

template<typename CharT, typename Traits, typename Alloc>
struct is_string_type<std::basic_string<CharT, Traits, Alloc>> : std::true_type {};

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
  std::optional<vec2<unit>> size;
  std::optional<int> border_radius;
  std::optional<int> letter_spacing;
  std::optional<int> font_size;
};

struct resolved_style {
  insets padding;
  insets margin;
  int border_size;
  int border_radius;

  std::reference_wrapper<const nalay::font> font;
  int font_size;
  int letter_spacing;
  nalay::text_alignment text_alignment;

  nalay::color color;
  nalay::color background_color;
  nalay::color border_color;

  std::optional<vec2<nalay::unit>> size;
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
  std::reference_wrapper<const nalay::texture> texture;
  vec2i size;
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

[[maybe_unused]] static void place(ui::node& node, vec2i origin);
[[maybe_unused]] static void request_focus(ui::node* node);
[[maybe_unused]] static void clear_focus();

[[maybe_unused]] static auto focused_node() -> ui::node*;

namespace ui { struct node; }

template <typename T> requires std::derived_from<std::remove_cvref_t<T>, ui::node>
[[maybe_unused]] void poll(T&& root);

namespace ui { struct node; }

template <typename T> concept UIElement = std::derived_from<std::remove_cvref_t<T>, ui::node>;

namespace ui {

struct node {
  enum class children_direction { vertical, horizontal };
  using child_list = std::vector<std::unique_ptr<node>>;

  explicit node() : m_dir(children_direction::vertical), m_parent(nullptr) {}
  explicit node(children_direction dir) : m_dir(dir), m_parent(nullptr) {}

  explicit node(children_direction dir, auto&&... children) : m_dir(dir), m_parent(nullptr)
  {
    (add(std::make_unique<std::remove_cvref_t<decltype(children)>>(
      std::forward<decltype(children)>(children))), ...);
  }


  node(const node& other)
    : m_dir(other.m_dir)
    , m_style(other.m_style)
    , m_parent(nullptr)
    , m_id(other.m_id)
    , m_size(other.m_size)
    , m_pos(other.m_pos)
    , m_interactivity(other.m_interactivity)
    , m_hovered(false)
  {
    m_components.reserve(other.m_components.size());
    for (const auto& child : other.m_components) {
      m_components.push_back(std::make_unique<node>(*child));
    }
  }

  node(node&& other) noexcept
    : m_dir(other.m_dir)
    , m_components(std::move(other.m_components))
    , m_style(std::move(other.m_style))
    , m_parent(nullptr)
    , m_click_listener(std::move(other.m_click_listener))
    , m_hover_listener(std::move(other.m_hover_listener))
    , m_focus_listener(std::move(other.m_focus_listener))
    , m_unfocus_listener(std::move(other.m_unfocus_listener))
    , m_id(std::move(other.m_id))
    , m_size(other.m_size)
    , m_pos(other.m_pos)
    , m_interactivity(other.m_interactivity)
    , m_hovered(false)
  {
    for (auto& child : m_components) {
      child->m_parent = this;
    }
  }

  node& operator=(const node& other) {
    if (this != &other) {
      m_dir = other.m_dir;
      m_style = other.m_style;
      m_id = other.m_id;
      m_size = other.m_size;
      m_pos = other.m_pos;
      m_interactivity = other.m_interactivity;
      m_hovered = false;
      m_parent = nullptr;

      m_components.clear();
      m_components.reserve(other.m_components.size());
      for (const auto& child : other.m_components) {
        m_components.push_back(std::make_unique<node>(*child));
      }
    }
    return *this;
  }

  node& operator=(node&& other) noexcept {
    if (this != &other) {
      m_dir = other.m_dir;
      m_components = std::move(other.m_components);
      m_style = std::move(other.m_style);
      m_click_listener = std::move(other.m_click_listener);
      m_hover_listener = std::move(other.m_hover_listener);
      m_focus_listener = std::move(other.m_focus_listener);
      m_unfocus_listener = std::move(other.m_unfocus_listener);
      m_id = std::move(other.m_id);
      m_size = other.m_size;
      m_pos = other.m_pos;
      m_interactivity = other.m_interactivity;
      m_hovered = false;
      m_parent = nullptr;

      for (auto& child : m_components) {
        child->m_parent = this;
      }
    }
    return *this;
  }

  void add(std::unique_ptr<node> child) {
    child->m_parent = this;
    m_components.push_back(std::move(child));
  } // TODO: ADD POISONED STATE, THIS IS A DANGEROUS OPERATION.

  template <typename T> requires std::derived_from<std::remove_cvref_t<T>, node>
  friend void nalay::poll(T&& root);

  void compute(render_queue& queue) {
    measure_intrinsic();
    resolve_flex(m_size);
    place({0, 0});
    emit(queue);
  }

  void set_parent(node* new_parent)
  {
    if (m_parent) {
      auto& siblings = m_parent->m_components;
      auto it = std::find_if(siblings.begin(), siblings.end(), [&](const auto& ptr){
        return ptr.get() == this;
      });
      if (it != siblings.end()) siblings.erase(it);
    }

    if (not new_parent) return;

    for (node* p = new_parent; p; p = p->m_parent)
      NALAY_ASSERT(p != this, "set_parent would create a cycle");

    new_parent->m_components.push_back(std::unique_ptr<node>(this));
    m_parent = new_parent;
  }

  auto clicked(this auto&& self, auto&& func) -> decltype(auto)   { self.m_click_listener.add(std::forward<decltype(func)>(func));   return std::forward<decltype(self)>(self); }
  auto hovered(this auto&& self, auto&& func) -> decltype(auto) { self.m_hover_listener.add(std::forward<decltype(func)>(func));   return std::forward<decltype(self)>(self); }
  auto focused(this auto&& self, auto&& func) -> decltype(auto) { self.m_focus_listener.add(std::forward<decltype(func)>(func));   return std::forward<decltype(self)>(self); }
  auto unfocused(this auto&& self, auto&& func) -> decltype(auto){ self.m_unfocus_listener.add(std::forward<decltype(func)>(func)); return std::forward<decltype(self)>(self); }

  auto id(this auto&& self, std::string id)         -> decltype(auto)  { self.m_id                     = std::move(id);            return std::forward<decltype(self)>(self); }
  auto style(this auto&& self, ::nalay::style s)    -> decltype(auto)  { self.m_style                  = s;                        return std::forward<decltype(self)>(self); }
  auto padding(this auto&& self, insets p)          -> decltype(auto)  { self.m_style.padding          = p;                        return std::forward<decltype(self)>(self); }
  auto margin(this auto&& self, insets m)           -> decltype(auto)  { self.m_style.margin           = m;                        return std::forward<decltype(self)>(self); }
  auto bg_color(this auto&& self, ::nalay::color c) -> decltype(auto)  { self.m_style.background_color = c;                        return std::forward<decltype(self)>(self); }
  auto color(this auto&& self, ::nalay::color c)    -> decltype(auto)  { self.m_style.color            = c;                        return std::forward<decltype(self)>(self); }
  auto size(this auto&& self, vec2<unit> sz)        -> decltype(auto)  { self.m_style.size             = sz;                       return std::forward<decltype(self)>(self); }
  auto size(this auto&& self, unit w, unit h)       -> decltype(auto)  { self.m_style.size             = vec2<unit>{w, h};         return std::forward<decltype(self)>(self); }
  auto border_radius(this auto&& self, int r)       -> decltype(auto)  { self.m_style.border_radius    = r;                        return std::forward<decltype(self)>(self); }
  auto font_size(this auto&& self, int fs)          -> decltype(auto)  { self.m_style.font_size        = fs;                       return std::forward<decltype(self)>(self); }
  auto display_font(this auto&& self,
                    std::reference_wrapper<const nalay::font> f) -> decltype(auto) { self.m_style.display_font = f; return std::forward<decltype(self)>(self); }
  auto align(this auto&& self,
             std::pair<::nalay::alignment, ::nalay::alignment> a) -> decltype(auto) { self.x_align(a.first); self.y_align(a.second); return std::forward<decltype(self)>(self); }

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

  auto is_focused()    const -> bool  { return nalay::context::instance().focused_node == this; }
  auto is_hovered()    const -> bool  { return m_hovered; }
  auto can_focus()     const -> bool  { return m_interactivity & interactivity::focusable; }
  auto can_click()     const -> bool  { return m_interactivity & interactivity::clickable; }
  auto can_hover()     const -> bool  { return m_interactivity & interactivity::hoverable; }
  auto computed_size() const -> vec2i { return m_size; }
  auto computed_pos()  const -> vec2i { return m_pos;  }

  virtual ~node() = default;
  auto default_style() const -> nalay::style { return {}; }

  auto resolve(this auto const& self) -> resolved_style {
    auto def = self.default_style();
    return {
      .padding        = self.m_style.padding       .or_else([&]{ return def.padding;        }).value_or(insets{}),
      .margin         = self.m_style.margin        .or_else([&]{ return def.margin;         }).value_or(insets{}),
      .border_size    = self.m_style.border_size   .or_else([&]{ return def.border_size;    }).value_or(0),
      .border_radius  = self.m_style.border_radius .or_else([&]{ return def.border_radius;  }).value_or(0),

      .font           = self.m_style.display_font  .or_else([&]{ return def.display_font;   }).value_or(nalay::context::instance().fonts().get().default_font()),
      .font_size      = self.m_style.font_size     .or_else([&]{ return def.font_size;      }).value_or(defaults::font_size),
      .letter_spacing = self.m_style.letter_spacing.or_else([&]{ return def.letter_spacing; }).value_or(0),
      .text_alignment = self.m_style.text_alignment.or_else([&]{ return def.text_alignment; }).value_or(text_alignment::center),

      .color            = self.m_style.color           .or_else([&]{ return def.color;            }).value_or(color::black()),
      .background_color = self.m_style.background_color.or_else([&]{ return def.background_color; }).value_or(nalay::color{}),
      .border_color     = self.m_style.border_color    .or_else([&]{ return def.border_color;     }).value_or(nalay::color{}),

      .size      = self.m_style.size     .or_else([&]{ return def.size;      }),
      .alignment = self.m_style.alignment.or_else([&]{ return def.alignment; }),
    };
  }

private:

  auto compute_children_extent() const -> vec2i {
    vec2i extent{};
    for (auto& child : m_components) {
      vec2i cs = child->computed_size();
      if (m_dir == children_direction::vertical) {
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
    std::pair<alignment, alignment> align_pair,
    vec2i content_size,
    vec2i children_extent
  ) const -> vec2i {
    vec2i cursor{};
    if (m_dir == children_direction::vertical) {
      if      (align_pair.second == alignment::bottom) cursor.y = content_size.y - children_extent.y;
      else if (align_pair.second == alignment::center) cursor.y = (content_size.y - children_extent.y) / 2;
    } else {
      if      (align_pair.first == alignment::right)  cursor.x = content_size.x - children_extent.x;
      else if (align_pair.first == alignment::center) cursor.x = (content_size.x - children_extent.x) / 2;
    }
    return cursor;
  }

  auto cross_offset(
    std::pair<alignment, alignment> align_pair,
    vec2i content_size,
    vec2i child_size
  ) const -> vec2i {
    vec2i cross{};
    if (m_dir == children_direction::vertical) {
      if      (align_pair.first == alignment::right)  cross.x = content_size.x - child_size.x;
      else if (align_pair.first == alignment::center) cross.x = (content_size.x - child_size.x) / 2;
    } else {
      if      (align_pair.second == alignment::bottom) cross.y = content_size.y - child_size.y;
      else if (align_pair.second == alignment::center) cross.y = (content_size.y - child_size.y) / 2;
    }
    return cross;
  }

protected:

  friend void nalay::place(ui::node& root, vec2i origin);
  friend void nalay::poll(auto&& root);
  friend void nalay::request_focus(ui::node *node);
  friend void nalay::clear_focus();

  virtual void _on_focus() {}
  virtual void _on_blur()  {}
  virtual void _on_click() {}
  virtual void _on_key(keyboard_event event) { (void) event; }

  auto margin_extent() const -> vec2i {
    auto rs = resolve();
    return { rs.margin.get_left() + rs.margin.get_right(),
      rs.margin.get_top()  + rs.margin.get_bottom() };
  }
  auto padding_extent() const -> vec2i {
    auto rs = resolve();
    return { rs.padding.get_left() + rs.padding.get_right(),
      rs.padding.get_top()  + rs.padding.get_bottom() };
  }

  auto outer_size() const -> vec2i { return m_size + margin_extent(); }

  virtual void measure_intrinsic() {
    for (auto& child : m_components) child->measure_intrinsic();

    auto rs      = resolve();
    auto content = compute_children_extent();
    auto pad     = padding_extent();
    auto marg    = margin_extent();
    auto intrinsic = content + pad;

    int w = intrinsic.x;
    int h = intrinsic.y;
    if (rs.size) {
      if (rs.size->x.type == unit::kind::fixed) w = rs.size->x.value;
      if (rs.size->y.type == unit::kind::fixed) h = rs.size->y.value;
    }

    m_size = vec2i{ w, h } + marg;
  }

  virtual void resolve_flex(vec2i avail) {
    auto rs = resolve();

    auto resolve_self_axis = [](int intrinsic, int avail_axis, unit u) -> int {
      switch (u.type) {
        case unit::kind::fixed:   return u.value;
        case unit::kind::percent: return std::max(intrinsic,
                                                  static_cast<int>(static_cast<float>(avail_axis) * u.factor));
        case unit::kind::fill:    return std::max(intrinsic, avail_axis);
        case unit::kind::grow:    return std::max(intrinsic, avail_axis);
      }
      return intrinsic;
    };

    if (rs.size) {
      m_size.x = resolve_self_axis(m_size.x, avail.x, rs.size->x);
      m_size.y = resolve_self_axis(m_size.y, avail.y, rs.size->y);
    }

    auto marg = margin_extent();
    auto pad  = padding_extent();
    vec2i inner{ m_size.x - marg.x - pad.x, m_size.y - marg.y - pad.y };

    const bool vertical  = (m_dir == children_direction::vertical);
    const int  n         = static_cast<int>(m_components.size());
    const int  gaps      = n > 0 ? (n - 1) * defaults::padding : 0;
    const int  main_ext  = vertical ? inner.y : inner.x;
    const int  cross_ext = vertical ? inner.x : inner.y;

    auto main_unit_of  = [&](const std::unique_ptr<node>& c) -> unit {
      auto cs = c->m_style.size;
      if (!cs) return unit::fill(); 
      return vertical ? cs->y : cs->x;
    };
    auto cross_unit_of = [&](const std::unique_ptr<node>& c) -> unit {
      auto cs = c->m_style.size;
      if (!cs) return unit::fill();
      return vertical ? cs->x : cs->y;
    };

    int   claimed    = 0;
    float total_grow = 0.f;

    for (auto& child : m_components) {
      unit u = main_unit_of(child);
      int  child_intrinsic_main = vertical ? child->m_size.y : child->m_size.x;
      switch (u.type) {
        case unit::kind::fixed:   claimed    += u.value; break;
        case unit::kind::percent: claimed    += static_cast<int>(static_cast<float>(main_ext) * u.factor); break;
        case unit::kind::fill:    claimed    += child_intrinsic_main; break;
        case unit::kind::grow:    total_grow += u.factor; break;
      }
    }

    const int leftover = std::max(0, main_ext - claimed - gaps);

    for (const auto& child : m_components) {
      unit main_u  = main_unit_of(child);
      unit cross_u = cross_unit_of(child);

      int main_size = vertical ? child->m_size.y : child->m_size.x;
      switch (main_u.type) {
        case unit::kind::fixed:
          main_size = main_u.value;
          break;
        case unit::kind::percent:
          main_size = std::max(main_size, static_cast<int>(static_cast<float>(main_ext) * main_u.factor));
          break;
        case unit::kind::fill:
          main_size = std::max(main_size, main_ext);
          break;
        case unit::kind::grow:
          if (total_grow > 0.f) {
            main_size = std::max(
              main_size,
              static_cast<int>(std::round(static_cast<float>(leftover) * (main_u.factor / total_grow)))
            );
          }
          break;
      }

      int cs_size = vertical ? child->m_size.x : child->m_size.y;
      switch (cross_u.type) {
        case unit::kind::fixed:   cs_size = cross_u.value; break;
        case unit::kind::percent: cs_size = std::max(cs_size, static_cast<int>(static_cast<float>(cross_ext) * cross_u.factor)); break;
        case unit::kind::fill:    cs_size = std::max(cs_size, cross_ext); break;
        case unit::kind::grow:    cs_size = std::max(cs_size, cross_ext); break;
      }

      vec2i child_avail = vertical
        ? vec2i{ cs_size, main_size }
        : vec2i{ main_size, cs_size };

      child->resolve_flex(child_avail);
    }
  }

  virtual void place(vec2i origin) {
    m_pos = origin;

    auto rs         = resolve();
    auto& pad       = rs.padding;
    auto& marg      = rs.margin;
    auto  layout_al = rs.alignment.value_or({});

    vec2i content_origin{
      m_pos.x + marg.get_left() + pad.get_left(),
      m_pos.y + marg.get_top() + pad.get_top()
    };
    vec2i content_size{
      m_size.x - marg.get_left() - marg.get_right() - pad.get_left() - pad.get_right(),
      m_size.y - marg.get_top() - marg.get_bottom() - pad.get_top()  - pad.get_bottom()
    };

    auto children_extent = compute_children_extent();
    auto cursor          = initial_cursor(layout_al, content_size, children_extent);

    for (auto& child : m_components) {
      auto cs    = child->computed_size();
      auto cross = cross_offset(layout_al, content_size, cs);
      child->place(content_origin + cursor + cross);

      if (m_dir == children_direction::vertical)
        cursor.y += cs.y + defaults::padding;
      else
        cursor.x += cs.x + defaults::padding;
    }
  }

  virtual void emit(render_queue& queue) const {
    auto rs = resolve();
    emit_background(queue, rs);
    for (auto& child : m_components) child->emit(queue);
  }

  virtual void poll() {
    auto inputs = nalay::context::instance().inputs();
    auto mp  = inputs.get().mouse_pos();

    auto* focused_before = nalay::context::instance().focused_node;

    bool child_handled_event = false;
    for (auto& child : m_components) {
      child->poll();

      if (nalay::context::instance().focused_node != focused_before) {
        child_handled_event = true;
        focused_before = nalay::context::instance().focused_node;
      }
    }

    if (m_interactivity == interactivity::none) return;

    bool hit = hit_test(mp);

    if (can_hover()) {
      m_hovered = hit;
      if (m_hovered) m_hover_listener.notify_all();
    }

    if (hit && can_click() && inputs.get().mouse_pressed(mouse_button::left) && !child_handled_event) {
      if (can_focus()) {
        request_focus(this);
        m_focus_listener.notify_all();
      }
      _on_click();
      m_click_listener.notify_all();
    }

    if (!hit && !child_handled_event && inputs.get().mouse_pressed(mouse_button::left) && is_focused()) {
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

  void emit_background(render_queue& queue, const resolved_style& rs) const {
    auto marg_ext = vec2i{
      rs.margin.get_left() + rs.margin.get_right(),
      rs.margin.get_top() + rs.margin.get_bottom()
    };
    auto bg_pos = vec2i{
      m_pos.x + rs.margin.get_left(),
      m_pos.y + rs.margin.get_top()
    };
    auto bg_size = vec2i{
      m_size.x - marg_ext.x,
      m_size.y - marg_ext.y
    };

    queue.emplace(
      primitive::rect{
        .color         = rs.background_color,
        .border_color  = rs.border_color,
        .border_radius = rs.border_radius,
        .border_size   = rs.border_size,
        .size          = bg_size,
      },
      bg_pos
    );
  }

  void emit_text(render_queue& queue, const std::string& text, const resolved_style& rs) const {
    auto text_sz = rs.font.get().measure(text, rs.font_size, rs.letter_spacing);

    vec2i inner_pos{
      m_pos.x + rs.margin.get_left() + rs.padding.get_left(),
      m_pos.y + rs.margin.get_top() + rs.padding.get_top()
    };
    vec2i inner_sz{
      m_size.x - rs.margin.get_left() - rs.margin.get_right() - rs.padding.get_left() - rs.padding.get_right(),
      m_size.y - rs.margin.get_top() - rs.margin.get_bottom() - rs.padding.get_top()  - rs.padding.get_bottom()
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
    auto rs = resolve();
    vec2i hit_pos{
      m_pos.x + rs.margin.get_left(),
      m_pos.y + rs.margin.get_top()
    };
    vec2i hit_size{
      m_size.x - rs.margin.get_left() - rs.margin.get_right(),
      m_size.y - rs.margin.get_top() - rs.margin.get_bottom()
    };
    return mp.x >= hit_pos.x && mp.x < hit_pos.x + hit_size.x
        && mp.y >= hit_pos.y && mp.y < hit_pos.y + hit_size.y;
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

  children_direction m_dir;
  child_list    m_components;
  nalay::style  m_style;

  node*         m_parent;

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

template <typename T> requires (std::is_arithmetic_v<T> or std::same_as<T, std::string>)
struct input : public node {
  T m_value{};
  T m_placeholder{};

  auto placeholder(this auto&& self, T&& placeholder)  { self.m_placeholder = std::forward<T>(placeholder); return std::forward<decltype(self)>(self); }
  auto value_changed(this auto&& self, auto&& func)    { self.m_changed_listeners.add(std::forward<decltype(func)>(func)); return std::forward<decltype(self)>(self); }

  auto default_style() const -> nalay::style {
    return {
      .padding          = insets{ defaults::button_padding.x, defaults::button_padding.y },
      .margin           = std::nullopt,
      .border_size      = defaults::button_border_size,
      .border_color     = defaults::button_border_color,
      .background_color = defaults::button_background,
      .color            = defaults::button_foreground,
      .alignment        = std::nullopt,
      .text_alignment   = text_alignment::left,
      .display_font     = std::nullopt,
      .size             = std::nullopt,
      .border_radius    = defaults::button_border_radius,
      .letter_spacing   = std::nullopt,
      .font_size        = std::nullopt,
    };
  }

  void erase_character() requires std::same_as<T, std::string> {
    if (!m_value.empty()) {
      do { m_value.pop_back(); }
      while (!m_value.empty() && (m_value.back() & 0xC0) == 0x80);
    }
  }

  void erase_digit() requires std::is_arithmetic_v<T> {
    m_value = m_value / 10;
  }

  auto value() const -> const T& { return m_value; }
  auto clear() { m_value = {}; }

  void poll() override {
    node::poll();
    auto inputs = nalay::context::instance().inputs();
    if (not inputs.get().key_down(nalay::key::backspace)) {
      m_pressed_timer = 0.0f;
      return;
    }
    if (m_pressed_timer < 0.5f) {
      m_pressed_timer += nalay::context::instance().delta_time;
      return;
    }
    if (m_erased_timer > .01f) {
      if constexpr (std::same_as<T, std::string>) {
        erase_character();
      } else if constexpr (std::is_arithmetic_v<T>) {
        erase_digit();
      }
      m_erased_timer = 0.0f;
    } else {
      m_erased_timer += nalay::context::instance().delta_time;
    }
  }

  void _on_key(keyboard_event event) override {
    if constexpr (std::is_arithmetic_v<T>) {
      if (event.key == nalay::key::backspace) { erase_digit(); return; }
      if (not std::isdigit(static_cast<char>(event.key))) return;
      auto digit = static_cast<char>(event.key) - '0';
      if (m_value >= (std::numeric_limits<T>::max() - digit) / 10) {
        m_value = std::numeric_limits<T>::max();
      } else {
        m_value = m_value * 10 + static_cast<T>(digit);
      }
      m_changed_listeners.notify_all(m_value);
    }
    else if constexpr (std::same_as<T, std::string>) {
      if (event.key == nalay::key::backspace) { erase_character(); return; }
      auto container_width = m_size.x;

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
    // Set caret
  }

  void measure_intrinsic() override {
    auto rs   = resolve();
    auto pad  = padding_extent();
    auto marg = margin_extent();

    vec2i text_sz;
    if constexpr (std::same_as<T, std::string>) {
      text_sz = measure_text_size(m_value);
    } else {
      auto num_digits = m_value == 0
        ? 1
        : static_cast<int>(std::floor(std::log10(std::abs(static_cast<double>(m_value)))) + 1);
      auto font_size  = m_style.font_size.value_or(defaults::font_size);
      text_sz         = vec2i{ font_size * num_digits, font_size };
    }

    int w = text_sz.x + pad.x;
    int h = text_sz.y + pad.y;
    if (rs.size) {
      if (rs.size->x.type == unit::kind::fixed) w = rs.size->x.value;
      if (rs.size->y.type == unit::kind::fixed) h = rs.size->y.value;
    }
    m_size = vec2i{ w, h } + marg;
  }

  void emit(render_queue& queue) const override {
    auto rs = resolve();
    emit_background(queue, rs);
    if constexpr (std::same_as<T, std::string>) {
      auto container_width = m_size.x;
      if (measure_text_size(m_value).x + 2 * m_style.font_size.value_or(defaults::font_size) > container_width) {
        auto truncated = m_value.size() > 3 ? m_value.substr(0, m_value.size() - 3) + ".." : m_value;
        emit_text(queue, truncated, rs);
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
  reactive<std::string> text;
  explicit button(std::string s) : text(std::move(s)) {}
  explicit button(reactive<std::string> reactive) : text(reactive) {}

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

  void measure_intrinsic() override {
    auto rs      = resolve();
    auto text_sz = measure_text_size(text.get());
    auto pad     = padding_extent();
    auto marg    = margin_extent();

    int w = text_sz.x + pad.x;
    int h = text_sz.y + pad.y;
    if (rs.size) {
      if (rs.size->x.type == unit::kind::fixed) w = rs.size->x.value;
      if (rs.size->y.type == unit::kind::fixed) h = rs.size->y.value;
    }
    m_size = vec2i{ w, h } + marg;
  }

  void emit(render_queue& queue) const override {
    auto rs = resolve();
    emit_background(queue, rs);
    emit_text(queue, text.get(), rs);
  }
};

struct label : public node {
  reactive<std::string> text;

  explicit label(std::string s) : text(std::move(s)) {}
  explicit label(reactive<std::string> reactive) : text(reactive) {}

  void measure_intrinsic() override {
    auto rs      = resolve();
    auto text_sz = measure_text_size(text.get());
    auto pad     = padding_extent();
    auto marg    = margin_extent();

    int w = text_sz.x + pad.x;
    int h = text_sz.y + pad.y;
    if (rs.size) {
      if (rs.size->x.type == unit::kind::fixed) w = rs.size->x.value;
      if (rs.size->y.type == unit::kind::fixed) h = rs.size->y.value;
    }
    m_size = vec2i{ w, h } + marg;
  }

  void emit(render_queue& queue) const override {
    auto rs = resolve();
    emit_background(queue, rs);
    emit_text(queue, text.get(), rs);
  }
};

struct image : public node {
  std::reference_wrapper<const nalay::texture> m_texture;

  explicit image(const nalay::texture& tex) : m_texture(tex) {}

  void measure_intrinsic() override {
    auto rs   = resolve();
    auto info = m_texture.get().get_info();
    auto pad  = padding_extent();
    auto marg = margin_extent();

    int w = info.size.x + pad.x;
    int h = info.size.y + pad.y;
    if (rs.size) {
      if (rs.size->x.type == unit::kind::fixed) w = rs.size->x.value;
      if (rs.size->y.type == unit::kind::fixed) h = rs.size->y.value;
    }
    m_size = vec2i{ w, h } + marg;
  }

  void emit(render_queue& queue) const override {
    auto rs = resolve();

    emit_background(queue, rs);

    vec2i img_pos{
      m_pos.x + rs.margin.get_left() + rs.padding.get_left(),
      m_pos.y + rs.margin.get_top() + rs.padding.get_top()
    };
    vec2i img_size{
      m_size.x - rs.margin.get_left() - rs.margin.get_right() - rs.padding.get_left() - rs.padding.get_right(),
      m_size.y - rs.margin.get_top() - rs.margin.get_bottom() - rs.padding.get_top() - rs.padding.get_bottom()
    };

    queue.emplace(
      primitive::img{
        .texture = m_texture,
        .size    = img_size
      },
      img_pos
    );
  }
};

template <UIElement... Elements>
struct columns final : public node {
  columns(Elements&&... args) : node(children_direction::horizontal, std::forward<Elements>(args)...) {}
};

template <UIElement... Elements>
struct rows final : public node {
  rows(Elements&&... args) : node(children_direction::vertical, std::forward<Elements>(args)...) {}
};

} // namespace ui

static void request_focus(ui::node* node) {
  if (nalay::context::instance().focused_node == node) return;
  if (nalay::context::instance().focused_node) nalay::context::instance().focused_node->_on_blur();
  nalay::context::instance().focused_node = node;
  if (nalay::context::instance().focused_node) nalay::context::instance().focused_node->_on_focus();
}

static void place(ui::node& root, vec2i origin) { root.place(origin); }
static void clear_focus() { request_focus(nullptr); }
static auto focused_node() -> ui::node* { return nalay::context::instance().focused_node; }

static constexpr auto operator|(interactivity a, interactivity b) -> interactivity {
  return static_cast<interactivity>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
static constexpr auto operator&(interactivity a, interactivity b) -> bool {
  return static_cast<uint8_t>(a) & static_cast<uint8_t>(b);
}

template <typename T> requires std::derived_from<std::remove_cvref_t<T>, ui::node>
void poll(T&& root) {
  std::forward<T>(root).poll();
}

} // namespace nalay
