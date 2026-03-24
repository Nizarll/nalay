#pragma once

#include <array>
#include <cassert>
#include <concepts>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

#include "allocators.hpp"
#include "providers.hpp"
#include "defaults.hpp"

//TODO: add layout computation for percentage sizing
//TODO: add defaults for all base_style members
//TODO: compute text wrap based on parent width

static thread_local layout_ctx* nalay_ctx = nullptr;

enum class render_cmd_type : uint8_t {
  img,
  rect,
  text,
  svg,
};

enum class alignment : uint8_t {
  none,
  left,
  right,
  center,
  top,
  bottom
};

// Struct Declarations

struct style {
  std::optional<insets>                        padding;
  std::optional<insets>                        margin;
  std::optional<int>                           border_size;
  std::optional<::color>                       border_color;
  std::optional<::color>                       background_color;
  std::optional<::color>                       color;
  std::optional<std::pair<::alignment, ::alignment>> alignment;
  std::optional<std::reference_wrapper<const font>> display_font;
  std::optional<vec2i>                         size;
  std::optional<int>                           border_radius;
  std::optional<int>                           letter_spacing;
  std::optional<int>                           font_size;
};

namespace primitive {

struct text {
  std::reference_wrapper<const ::font> font;
  std::string   text;
  ::color       text_color;
  ::color       outline_color;
  vec2i         size;
  vec2i         pos;
  int           outline_thickness;
  int           font_size;
  int           letter_spacing;
};

struct img {
  std::span<char> bytes;
  int             format;
};

struct rect {
  ::color color;
  ::color border_color;
  int     border_radius;
  int     border_size;
  vec2i   size;
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

struct node_record {
  std::size_t start;
  std::size_t end;
  vec2i       computed_size;
};

using renderer      = renderer_<render_cmd>;
using margin        = insets;
using padding       = insets;
using border_radius = insets;

[[maybe_unused]] static auto get_ctx() -> layout_ctx* { return nalay_ctx; }
[[maybe_unused]] static void set_ctx(layout_ctx* ctx) { nalay_ctx = ctx; }

// ─── function_variant ────────────────────────────────────────────────────────

template <typename... Overloads>
struct function_variant {
  function_variant() = delete;

  template <typename T> requires (not std::same_as<std::decay_t<T>, function_variant>)
  function_variant(T&& t) {
    constexpr auto idx = []<std::size_t... Is>(std::index_sequence<Is...>) {
      std::size_t result = sizeof...(Overloads);
      ((is_same_signature<std::decay_t<T>, Overloads>::value
        ? (result = std::min(result, Is), true) : false) || ...);
      return result;
    }(std::index_sequence_for<Overloads...>{});

    static_assert(idx < sizeof...(Overloads), "Type is not convertible to any overload");
    using TargetType = std::tuple_element_t<idx, std::tuple<Overloads...>>;
    m_index = idx;
    new (m_storage.buffer.data()) TargetType(std::forward<T>(t));
  }

  function_variant(const function_variant& other) : m_index(other.m_index) { copy_from(other); }
  function_variant(function_variant&& other) noexcept : m_index(other.m_index) { move_from(other); }

  auto operator=(const function_variant& other) -> function_variant& {
    if (this != &other) { this->~function_variant(); m_index = other.m_index; copy_from(other); }
    return *this;
  }
  auto operator=(function_variant&& other) noexcept -> function_variant& {
    if (this != &other) { this->~function_variant(); m_index = other.m_index; move_from(other); }
    return *this;
  }

  ~function_variant() {
    [this]<std::size_t... Is>(std::index_sequence<Is...>) {
      auto destroy = [this]<std::size_t I>() {
        if (m_index == I) {
          using T = std::tuple_element_t<I, std::tuple<Overloads...>>;
          std::launder(reinterpret_cast<T*>(m_storage.buffer.data()))->~T();
        }
      };
      (destroy.template operator()<Is>(), ...);
    }(std::index_sequence_for<Overloads...>{});
  }

  auto which() const -> size_t { return m_index; }

  template <typename T>
  auto get(this auto&& self) {
    if (self.m_index != index_of<T>()) {
      std::cerr << "function_variant::get mismatch: stored=" << self.m_index
        << " requested=" << index_of<T>() << "\n";
      std::abort();
    }
    return *std::launder(reinterpret_cast<T*>(
      const_cast<std::byte*>(self.m_storage.buffer.data())));
  }

private:
  template <typename T> struct callable_args;
  template <typename R, typename... Args>
  struct callable_args<R(Args...)> { using type = std::tuple<Args...>; };
  template <typename R, typename... Args>
  struct callable_args<std::function<R(Args...)>> { using type = std::tuple<Args...>; };
  template <typename T, typename R, typename... Args>
  struct callable_args<R(T::*)(Args...)> { using type = std::tuple<Args...>; };
  template <typename T> requires requires { &T::operator(); }
  struct callable_args<T> : callable_args<decltype(&T::operator())> {};
  template <typename T, typename R, typename... Args>
  struct callable_args<R(T::*)(Args...) const> { using type = std::tuple<Args...>; };

  template <typename F1, typename F2>
  struct is_same_signature {
    using t1 = typename callable_args<F1>::type;
    using t2 = typename callable_args<F2>::type;
    static constexpr auto value = std::same_as<t1, t2>;
  };

  static constexpr std::size_t size      = std::max({sizeof(Overloads)...});
  static constexpr std::size_t alignment = std::max({alignof(Overloads)...});
  struct alignas(alignment) internal_storage { std::array<std::byte, size> buffer; };
  internal_storage m_storage;
  size_t           m_index;

  template <std::size_t I = 0>
  void copy_from(const function_variant& other) {
    if constexpr (I < sizeof...(Overloads)) {
      if (other.m_index == I) {
        using T = std::tuple_element_t<I, std::tuple<Overloads...>>;
        new (m_storage.buffer.data()) T(
          *std::launder(reinterpret_cast<const T*>(other.m_storage.buffer.data())));
      } else { copy_from<I + 1>(other); }
    }
  }

  template <std::size_t I = 0>
  void move_from(function_variant& other) {
    if constexpr (I < sizeof...(Overloads)) {
      if (other.m_index == I) {
        using T = std::tuple_element_t<I, std::tuple<Overloads...>>;
        new (m_storage.buffer.data()) T(
          std::move(*std::launder(reinterpret_cast<T*>(other.m_storage.buffer.data()))));
      } else { move_from<I + 1>(other); }
    }
  }

  template <typename T, size_t i, typename First, typename... Rest>
  static constexpr auto index_of_impl() -> size_t {
    if constexpr (std::same_as<T, First>) return i;
    else return index_of_impl<T, i + 1, Rest...>();
  }
  template <typename T, size_t i>
  static constexpr auto index_of_impl() -> size_t { return sizeof...(Overloads); }
  template <typename T, size_t i = 0>
  static constexpr auto index_of() -> size_t {
    static_assert(index_of_impl<T, i, Overloads...>() < sizeof...(Overloads),
      "Type is not a valid overload");
    return index_of_impl<T, i, Overloads...>();
  }
};

// ─── render_queue ────────────────────────────────────────────────────────────

struct render_queue {
  render_queue() { m_records.reserve(1000); }
  ~render_queue() = default;
  render_queue(const render_queue&)            = delete;
  render_queue(render_queue&&)                 = delete;
  auto operator=(const render_queue&) -> void  = delete;
  auto operator=(render_queue&&)      -> void  = delete;

  auto emplace(auto&&... args) -> render_cmd* {
    return m_arena.make(std::forward<decltype(args)>(args)...);
  }

  void push_record(size_t start, size_t end, vec2i size) {
    m_records.emplace_back(start, end, size);
  }

  auto records() const -> const std::vector<node_record>& { return m_records; }
  auto current() const -> size_t { return m_arena.current(); }

  void for_each(auto&& func) const {
    m_arena.for_each(std::forward<decltype(func)>(func));
  }
  void for_range(size_t start, size_t end, auto&& func) {
    m_arena.for_range(start, end, std::forward<decltype(func)>(func));
  }

  void trim_records(size_t to) {
    m_records.erase(m_records.begin() + to, m_records.end());
  }

  void reset() {
    m_arena.reset();
    m_records.clear();
  }

private:
  arena<render_cmd>        m_arena;
  std::vector<node_record> m_records;
};

// ─── ui nodes ────────────────────────────────────────────────────────────────

namespace ui {

struct node {
  auto id(this auto&& self, std::string id)                              { self.m_id = std::move(id); return self; }
  auto style(this auto&& self, ::style s)                                { self.m_style = s; return self; }
  auto padding(this auto&& self, insets p)                               { self.m_style.padding = p; return self; }
  auto margin(this auto&& self, insets m)                                { self.m_style.margin = m; return self; }
  auto bg_color(this auto&& self, ::color c)                             { self.m_style.background_color = c; return self; }
  auto color(this auto&& self, ::color c)                                { self.m_style.color = c; return self; }
  auto size(this auto&& self, int w, int h)                              { self.m_style.size = vec2i{w, h}; return self; }
  auto border_radius(this auto&& self, int r)                            { self.m_style.border_radius = r; return self; }
  auto font_size(this auto&& self, int fs)                               { self.m_style.font_size = fs; return self; }
  auto display_font(this auto&& self, std::reference_wrapper<const font> f) { self.m_style.display_font = f; return self; }
  auto align(this auto&& self, std::pair<::alignment, ::alignment> a)   { self.x_align(a.first); self.y_align(a.second); return self; }

  auto x_align(this auto&& self, ::alignment a) {
    if (a == ::alignment::top || a == ::alignment::bottom)
      throw std::invalid_argument("x_align cannot be top or bottom");
    self.m_style.alignment = { a, self.m_style.alignment.value_or({}).second };
    return self;
  }
  auto y_align(this auto&& self, ::alignment a) {
    if (a == ::alignment::left || a == ::alignment::right)
      throw std::invalid_argument("y_align cannot be left or right");
    self.m_style.alignment = { self.m_style.alignment.value_or({}).first, a };
    return self;
  }

  auto get_size(this const auto& self) -> vec4i { return self.measure(nalay_ctx); }
  auto get_position(this const auto& self)     -> vec2i { (void)self; return {}; }

  virtual ~node() = default;
  virtual void create(render_queue& queue, node* parent = nullptr) = 0;
  virtual auto measure() -> vec2i = 0;

protected:

  auto resolve_font() const -> std::reference_wrapper<const font>
  {
    return m_style.display_font.value_or(nalay_ctx->fonts.get().default_font());
  }

  auto resolve_padding() const -> insets {
    return m_style.padding.value_or(insets{});
  }

  auto measure_text_with_padding(const std::string& text) const -> vec2i {
    const auto text_sz = resolve_font().get().measure(
      text,
      m_style.font_size.value_or(defaults::font_size),
      m_style.letter_spacing.value_or(0)
    );
    const auto pad = resolve_padding();
    return text_sz + vec2i{
      pad.get_left() + pad.get_right(),
      pad.get_top()  + pad.get_bottom()
    };
  }

  void emit_background(
    render_queue& queue,
    vec2i pos,
    vec2i sz,
    ::color bg,
    ::color border_col,
    int radius,
    int border_sz) const {
    queue.emplace(
      primitive::rect{
        .color         = bg,
        .border_color  = border_col,
        .border_radius = radius,
        .border_size   = border_sz,
        .size          = sz,
      },
      pos
    );
  }
  void emit_text(
    render_queue& queue,
    const std::string& text,
    vec2i container_pos,
    vec2i container_sz,
    vec2i text_sz) const {
    const auto pad = resolve_padding();
    // Inner content area after stripping padding.
    const vec2i inner_pos{
      container_pos.x + pad.get_left(),
      container_pos.y + pad.get_top()
    };
    const vec2i inner_sz{
      container_sz.x - pad.get_left() - pad.get_right(),
      container_sz.y - pad.get_top()  - pad.get_bottom()
    };
    const vec2i text_pos{
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
        .pos               = container_pos,
        .outline_thickness = 0, //TODO:
        .font_size         = m_style.font_size.value_or(defaults::font_size),
        .letter_spacing    = m_style.letter_spacing.value_or(0),
      },
      text_pos
    );
  }

  ::style     m_style;
  std::string m_id;
};

template <typename T>
concept UIElement = std::derived_from<T, node>;

// ─── button ──────────────────────────────────────────────────────────────────

struct image : public node {

};

struct button : public node {
  std::string text;
  explicit button(std::string s) : text(std::move(s)) {}

  void create(render_queue& queue, node* parent) override {
    (void)parent;
    const size_t start    = queue.current();
    const vec2i  sz       = measure();
    const vec2i  text_sz  = resolve_font().get().measure(
      text,
      m_style.font_size.value_or(defaults::font_size),
      m_style.letter_spacing.value_or(0)
    );

    emit_background(
      queue,
      {},
      sz,
      m_style.background_color.value_or(defaults::button_background),
      m_style.border_color.value_or(defaults::button_border_color),
      m_style.border_radius.value_or(defaults::button_border_radius),
      m_style.border_size.value_or(defaults::button_border_size)
    );
    emit_text(queue, text, {}, sz, text_sz);

    queue.push_record(start, queue.current(), sz);
  }

  auto measure() -> vec2i override {
    if (m_style.size.has_value()) return m_style.size.value();
    const auto pad = m_style.padding.value_or(
      insets{ defaults::button_padding.x, defaults::button_padding.y }
    );
    const auto text_sz = resolve_font().get().measure(
      text,
      m_style.font_size.value_or(defaults::font_size),
      m_style.letter_spacing.value_or(0)
    );
    return text_sz + vec2i{
      pad.get_left() + pad.get_right(),
      pad.get_top()  + pad.get_bottom()
    };
  }
};

// ─── label ───────────────────────────────────────────────────────────────────

struct label : public node {
  std::string text;
  explicit label(std::string s) : text(std::move(s)) {}

  void create(render_queue& queue, node* parent) override {
    (void)parent;
    const size_t start   = queue.current();
    const vec2i  sz      = measure();
    const vec2i  text_sz = resolve_font().get().measure(
      text,
      m_style.font_size.value_or(defaults::font_size),
      m_style.letter_spacing.value_or(0)
    );

    emit_background(
      queue,
      {},
      sz,
      m_style.background_color.value_or({}),
      m_style.border_color.value_or({}),
      m_style.border_radius.value_or(0),
      m_style.border_size.value_or(0)
    );
    emit_text(queue, text, {}, sz, text_sz);

    queue.push_record(start, queue.current(), sz);
  }

  auto measure() -> vec2i override {
    if (m_style.size.has_value()) return m_style.size.value();
    return measure_text_with_padding(text);
  }
};

// ─── layout ──────────────────────────────────────────────────────────────────

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
      nalay_ctx->make_node<std::decay_t<decltype(children)>>(std::forward<decltype(children)>(children))
    ), ...);
  }

  void add(node& child) { components.push_back(&child); }

  template <UIElement T> requires (not std::is_reference_v<T>)
  void add(T&& child) {
    components.push_back(
      nalay_ctx->make_node<T>(std::forward<T>(child))
    );
  }

  void create(render_queue& queue, node* parent) override {
    (void)parent;
    const size_t byte_start   = queue.current();
    const size_t record_start = queue.records().size();
    const vec2i  sz           = measure();
    const auto   pad          = resolve_padding();
    const vec2i  padded_sz {
      sz.x + pad.get_left() + pad.get_right(),
      sz.y + pad.get_top()  + pad.get_bottom()
    };

    emit_background(
      queue,
      {},
      padded_sz,
      m_style.background_color.value_or({}),
      m_style.border_color.value_or({}),
      m_style.border_radius.value_or(0),
      m_style.border_size.value_or(0)
    );

    for (node* child : components)
      child->create(queue, this);

    layout_children(queue, sz, pad, { record_start, queue.records().size() });

    queue.trim_records(record_start);
    queue.push_record(byte_start, queue.current(), padded_sz);
  }

  auto measure() -> vec2i override {
    if (m_style.size.has_value()) return m_style.size.value();
    vec2i sz{};
    for (node* child : components) {
      const vec2i child_sz = child->measure();
      if (dir_ == direction::vertical) {
        sz.x  = std::max(sz.x, child_sz.x);
        sz.y += child_sz.y + defaults::padding;
      } else {
        sz.x += child_sz.x + defaults::padding;
        sz.y  = std::max(sz.y, child_sz.y);
      }
    }
    return sz;
  }

  void compute(render_queue& queue) {
    create(queue, nullptr);
  }

private:
  void layout_children(
    render_queue& queue,
    vec2i sz,
    const insets& pad,
    std::pair<size_t, size_t> range) {
    // Accumulate total children extent along the main axis.
    vec2i children_size{};
    for (auto i = range.first; i < range.second; ++i) {
      const auto& rec = queue.records().at(i);
      if (dir_ == direction::vertical) {
        children_size.y += rec.computed_size.y + defaults::padding;
        children_size.x  = std::max(children_size.x, rec.computed_size.x);
      } else {
        children_size.x += rec.computed_size.x + defaults::padding;
        children_size.y  = std::max(children_size.y, rec.computed_size.y);
      }
    }

    const auto layout_alignment = m_style.alignment.value_or({});

    // Starting offset along the main axis (honours alignment).
    vec2i accumulator{};
    if (dir_ == direction::vertical) {
      if      (layout_alignment.second == alignment::bottom) accumulator.y = sz.y - children_size.y;
      else if (layout_alignment.second == alignment::center) accumulator.y = (sz.y - children_size.y) / 2;
    } else {
      if      (layout_alignment.first == alignment::right)  accumulator.x = sz.x - children_size.x;
      else if (layout_alignment.first == alignment::center) accumulator.x = (sz.x - children_size.x) / 2;
    }

    // Padding origin: top-left corner of the content area.
    const vec2i pad_origin{ pad.get_left(), pad.get_top() };

    for (auto i = range.first; i < range.second; ++i) {
      const auto& rec = queue.records().at(i);

      // Cross-axis offset (centres or aligns perpendicular to flow direction).
      vec2i cross_offset{};
      if (dir_ == direction::vertical) {
        if      (layout_alignment.first == alignment::right)  cross_offset.x = sz.x - rec.computed_size.x;
        else if (layout_alignment.first == alignment::center) cross_offset.x = (sz.x - rec.computed_size.x) / 2;
      } else {
        if      (layout_alignment.second == alignment::bottom) cross_offset.y = sz.y - rec.computed_size.y;
        else if (layout_alignment.second == alignment::center) cross_offset.y = (sz.y - rec.computed_size.y) / 2;
      }

      queue.for_range(rec.start, rec.end, [&](render_cmd* cmd) {
        cmd->pos += pad_origin + accumulator + cross_offset;
      });

      if (dir_ == direction::vertical)
        accumulator.y += rec.computed_size.y + defaults::padding;
      else
        accumulator.x += rec.computed_size.x + defaults::padding;
    }
  }

  direction  dir_;
  child_list components;
};

// ─── columns / rows ──────────────────────────────────────────────────────────

template <UIElement... Elements>
struct columns final : public layout {
  columns(Elements&&... args)
    : layout(direction::horizontal, args...) {}
};

template <UIElement... Elements>
struct rows final : public layout {
  rows(Elements&&... args)
    : layout(direction::vertical, args...) {}
};

} // namespace ui
