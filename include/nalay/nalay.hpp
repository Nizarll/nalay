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
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "providers.hpp"
#include "defaults.hpp"

//TODO: compute padding based on style's padding instead of defaults::pading on layout class
//TODO: add layout computation for THREE CASES: Fixed size *which shortcircuits size computation*, 
//                                              Automatic size, NOTE: fixed size and automatic size are done, percentage sizing
//                                                                      has yet to be implemented
//                                              Percentage,
//TODO: add defaults for all base_style members
//TODO: Fix padding computation issue ==> Padding has to be computed such that there should only be enough padding to not overflow
//                                        the parent container if it has a fixed size 

//TODO: change storage of nodes from tuple to slab allocator ? <==> Nodes might need to have persistent lifetime
//                                                                  EX: getting the size of an element at runtime or dynamic rendering

//TODO: compute text wrap based on parent width


//NOTE: figure out how styling of subelements should  work
//maybe button logic and text logic should be decoupled such that user can control the individual styling of the text
//inside buttons ???

// Enum Declarations

enum class render_cmd_type: uint8_t {
  img,
  rect,
  text,
  svg,
};

enum class alignment : uint8_t {
  left,
  right,
  center,
  top,
  bottom
};

// Struct Declarations

struct style {
  std::optional<insets> padding;
  std::optional<insets> margin;
  std::optional<int> border_size;
  std::optional<::color> border_color;
  std::optional<::color> background_color;
  std::optional<::color> color;
  std::optional<std::pair<::alignment,::alignment>> alignment;
  std::optional<std::reference_wrapper<const font>> display_font;
  std::optional<vec2i> size;
  std::optional<int> border_radius;
  std::optional<int> letter_spacing;
  std::optional<int> font_size;
};

namespace primitive {

struct text {
  std::reference_wrapper<const ::font> font;
  std::string text;
  ::color text_color;
  ::color outline_color;
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
  ::color color;
  ::color border_color;
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

struct node_record {
  std::size_t start;
  std::size_t end;
  vec2i computed_size;
};

//Aliases

using renderer      = renderer_<render_cmd>;
using margin        = insets;
using padding       = insets;
using border_radius = insets;

//Utilities

template <typename... Overloads>
struct function_variant {
  function_variant() = delete;
  template <typename T> requires (!std::same_as<std::decay_t<T>, function_variant>)
  function_variant(T&& t) {
    constexpr auto idx = []<std::size_t... Is>(std::index_sequence<Is...>) {
      std::size_t result = sizeof...(Overloads);
      ((is_same_signature<std::decay_t<T>, Overloads>::value ? (result = std::min(result, Is), true) : false) || ...);
      return result;
    }(std::index_sequence_for<Overloads...>{});
    
    static_assert(idx < sizeof...(Overloads), "Type is not convertible to any overload");
    
    using TargetType = std::tuple_element_t<idx, std::tuple<Overloads...>>;
    m_index = idx;
    new (m_storage.buffer.data()) TargetType(std::forward<T>(t));
  }

  function_variant(const function_variant& other) : m_index(other.m_index) { copy_from(other); }
  function_variant(function_variant&& other) noexcept : m_index(other.m_index) { move_from(other); }

  auto operator=(const function_variant& other) -> function_variant&  {
    if (this != &other) {
      this->~function_variant();
      m_index = other.m_index;
      copy_from(other);
    }
    return *this;
  }

  auto operator=(function_variant&& other) noexcept -> function_variant&  {
    if (this != &other) {
      this->~function_variant();
      m_index = other.m_index;
      move_from(other);
    }
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
      std::cerr << "function_variant::get mismatch: stored m_index=" << self.m_index 
                << ", requested m_index=" << index_of<T>() << "\n";
      std::abort();
    }
    return *std::launder(reinterpret_cast<T*>(const_cast<std::byte*>(self.m_storage.buffer.data())));
  }
private:
  template <typename T>
  struct callable_args;
  
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
    using t1 = callable_args<F1>::type;
    using t2 = callable_args<F2>::type;
    static constexpr auto value = std::same_as<t1, t2>;
  };

  static constexpr std::size_t size = std::max({sizeof(Overloads)...});
  static constexpr std::size_t alignment = std::max({alignof(Overloads)...});
  struct alignas(alignment) internal_storage { std::array<std::byte, size> buffer; };
  internal_storage m_storage;
  size_t m_index;

  template <std::size_t I = 0>
  void copy_from(const function_variant& other) {
    if constexpr (I < sizeof...(Overloads)) {
      if (other.m_index == I) {
        using T = std::tuple_element_t<I, std::tuple<Overloads...>>;
        new (m_storage.buffer.data()) T(*std::launder(reinterpret_cast<const T*>(other.m_storage.buffer.data())));
      } else {
        copy_from<I + 1>(other);
      }
    }
  }

  template <std::size_t I = 0>
  void move_from(function_variant& other) {
    if constexpr (I < sizeof...(Overloads)) {
      if (other.m_index == I) {
        using T = std::tuple_element_t<I, std::tuple<Overloads...>>;
        new (m_storage.buffer.data()) T(std::move(*std::launder(reinterpret_cast<T*>(other.m_storage.buffer.data()))));
      } else {
        move_from<I + 1>(other);
      }
    }
  }

  template <typename T, size_t i, typename First, typename... Rest>
  static constexpr auto index_of_impl() -> size_t
  {
    if constexpr (std::same_as<T, First>) return i;
    else return index_of_impl<T, i + 1, Rest...>();
  }

  template <typename T, size_t i>
  static constexpr auto index_of_impl() -> size_t { return sizeof...(Overloads); }

  template <typename T, size_t i = 0>
  static constexpr auto index_of() -> size_t
  {
    static_assert(index_of_impl<T, i, Overloads...>() < sizeof...(Overloads), "Type is not a valid overload");
    return index_of_impl<T, i, Overloads...>();
  }

};

struct render_queue
{
  render_queue() : m_storage(new aligned_storage) { m_records.resize(1000); }
  ~render_queue() = default;

  render_queue(const render_queue&)           = delete;
  render_queue(render_queue&&)                = delete;
  auto operator=(const render_queue&) -> void = delete;
  auto operator=(render_queue&&)      -> void = delete;

  auto emplace(auto&&... args) -> render_cmd* {
    void* ptr         = m_storage->buff.data() + m_current;
    std::size_t space = aligned_size - m_current;
    void* aligned     = std::align(alignof(render_cmd), sizeof(render_cmd), ptr, space);
    std::byte* next   = static_cast<std::byte*>(aligned) + sizeof(render_cmd);
    if(next > m_storage->buff.data() + aligned_size) return nullptr;

    render_cmd* obj = nullptr;
    try {
      obj = std::construct_at(
        static_cast<render_cmd*>(aligned),
        std::forward<decltype(args)>(args)...
      );
    } catch(...) { return nullptr; }
    m_current = static_cast<size_t>(next - m_storage->buff.data());
    return obj;
  }

  void push_record(size_t start, size_t end, vec2i size) {

    m_records.emplace_back(start, end, size);
  }

  auto records() const -> const std::vector<node_record>& { return m_records; }
  auto current() const -> size_t { return m_current; }

  void for_each(auto&& func) const {
    std::byte* ptr = m_storage->buff.data();
    std::byte* end = m_storage->buff.data() + m_current;

    while (ptr < end) {
      auto* cmd = reinterpret_cast<render_cmd*>(ptr);
      func(*cmd);
      ptr += sizeof(*cmd);
    }
  }

  void for_range(size_t start, size_t end, auto&& func) {
    std::byte* ptr  = m_storage->buff.data() + start;
    std::byte* stop = m_storage->buff.data() + end;
    while (ptr < stop) {
      auto* cmd = reinterpret_cast<render_cmd*>(ptr);
      func(cmd);
      ptr += sizeof(render_cmd);
    }
  }

  void trim_records(size_t to) {
    m_records.erase(m_records.begin() + to, m_records.end());
  }

  void reset() {
    for_each([](render_cmd& cmd) -> void {
      std::destroy_at(&cmd);
    });
    m_current = 0;
    m_records.clear();
  }
private:
  static constexpr auto buffer_size  = 8 * 1024 * 1024;
  static constexpr auto aligned_size = ((buffer_size + alignof(render_cmd) - 1) / alignof(render_cmd)) * alignof(render_cmd);
  struct alignas(alignof(render_cmd)) aligned_storage { std::array<std::byte, buffer_size> buff; };

  std::vector<node_record> m_records;
  std::unique_ptr<aligned_storage> m_storage;
  size_t m_current{};
};

template<typename Tuple, typename Func, std::size_t... Is>
void tuple_for_each_impl(Tuple&& t, Func&& f, std::index_sequence<Is...>) {
  (f(std::get<Is>(t)), ...);
}

template<typename Tuple, typename Func>
void tuple_for_each(Tuple&& t, Func&& f) {
  tuple_for_each_impl(
    std::forward<Tuple>(t),
    std::forward<Func>(f),
    std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple>>>{}
  );
}

namespace ui {
struct node {
  auto id(this auto&& self, std::string id) {
    self.m_id = id;
    return self;
  }

  auto style(this auto&& self, ::style s) {
    self.m_style = s;
    return self;
  }

  auto padding(this auto&& self, insets p) { self.m_style.padding = p; return self; }
  auto margin(this auto&& self, insets m)  { self.m_style.margin = m; return self; }
  auto bg_color(this auto&& self, ::color color) { self.m_style.background_color = color; return self; }
  auto color(this auto&& self, ::color color) { self.m_style.color = color; return self; }
  auto size(this auto&& self, int width, int height) { self.m_style.size = vec2i{width, height}; return self; }
  auto border_radius(this auto&& self, int border_radius) { self.m_style.border_radius = border_radius; return self; }
  auto font_size(this auto&& self, int font_size) { self.m_style.font_size = font_size; return self; }
  auto display_font(this auto&& self, std::reference_wrapper<const font> display_font) { self.m_style.display_font = display_font; return self; }
  auto align(this auto&& self, std::pair<::alignment, ::alignment> alignment) { self.x_align(alignment.first); self.y_align(alignment.second); return self; }

  auto x_align(this auto&& self, ::alignment alignment) {
    if (alignment == ::alignment::top or alignment == ::alignment::bottom)
      throw std::invalid_argument("y alignment cannot be left or right");
    self.m_style.alignment = { alignment, self.m_style.alignment.value_or({}).second }; //TODO: fix this mess
    return self;
  }
  auto y_align(this auto&& self, ::alignment alignment) {
    if (alignment == ::alignment::left or alignment == ::alignment::right)
      throw std::invalid_argument("y alignment cannot be left or right");
    self.m_style.alignment->second = alignment;
    self.m_style.alignment = { self.m_style.alignment.value_or({}).first, alignment  };
    return self;
  }

  auto get_bounding_box(this const auto& self) -> vec4i {
    (void)self;
    return {};
  }

  auto get_position(this const auto& self) -> vec2i {
    (void)self;
    return {};
  }

  virtual ~node() = default;
  virtual void create(render_queue& queue, const layout_ctx& ctx, node* parent = nullptr) = 0;
  virtual auto measure(const layout_ctx& ctx) -> vec2i = 0;

protected:
  ::style m_style;
  std::string m_id;
};

template <typename T>
concept UIElement = std::derived_from<T, node>;

struct button : node {
  std::string text;
  button(std::string s) : text(std::move(s)) {}

  void create(render_queue& queue, const layout_ctx& ctx, node* parent) override
  {
    (void) parent;
    vec2i sz{};

    size_t start          = queue.current();
    auto size             = measure(ctx);
    auto text_size        = m_style.display_font.value_or(ctx.fonts.get().default_font()).get().measure(
      text,
      m_style.font_size.value_or(defaults::font_size),
      m_style.letter_spacing.value_or(0)
    );
    auto bg_pos           = vec2i{};
    auto text_pos         = vec2i{
      static_cast<int>(bg_pos.x + (size.x - text_size.x) * 0.5f),
      static_cast<int>(bg_pos.y + (size.y - text_size.y) * 0.5f)
    };
  
    auto bg = queue.emplace(
      primitive::rect{
        .color         = m_style.background_color.value_or(defaults::button_background),
        .border_color  = m_style.border_color.value_or(defaults::button_border_color),
        .border_radius = m_style.border_radius.value_or(defaults::button_border_radius),
        .border_size   = m_style.border_size.value_or(defaults::button_border_size),
        .size          = size,
      },
      bg_pos
    );

    auto fg = queue.emplace(
      primitive::text{
        .font              = m_style.display_font.value_or(ctx.fonts.get().default_font()),
        .text              = text,
        .text_color        = m_style.color.value_or(color::black()), //TODO: define defaults here
        .outline_color     = m_style.border_color.value_or(color::black()),
        .size              = text_size,
        .pos               = bg_pos,
        .outline_thickness = 0,//TODO: figure out this shiiih
        .font_size         = m_style.font_size.value_or(defaults::font_size),
        .letter_spacing    = m_style.letter_spacing.value_or(0),
      },
      text_pos
    );
    queue.push_record(start, queue.current(), measure(ctx));
  }

  auto measure(const layout_ctx& ctx) -> vec2i override {
    auto get_default = [&] -> std::reference_wrapper<const font> {
      return ctx.fonts.get().default_font();
    };
    const auto& f       = m_style.display_font.value_or(get_default());
    int   fs      = m_style.font_size.value_or(defaults::font_size);
    vec2i text_sz = f.get().measure(text, fs, m_style.letter_spacing.value_or(0));
    auto padding  = m_style.padding.value_or(insets{ defaults::button_padding.x, defaults::button_padding.y });
    return { text_sz.x + padding.left() + padding.right(), text_sz.y + padding.top() + padding.bottom() };
  }
};

struct label : node {
  std::string text;
  label(std::string s) : text(std::move(s)) {}
  void  create(render_queue& queue, const layout_ctx& ctx, node* parent) override { (void)parent; }
  auto measure(const layout_ctx& ctx) -> vec2i override { return {}; }
};

template <typename ...Components>
struct layout : node {
  enum class direction {
    vertical,
    horizontal
  };
  direction dir_;
  std::tuple<std::decay_t<Components>...> components;

  explicit layout(direction dir, Components&&... args) : dir_(dir),
    components(std::forward<Components>(args)...) { }

  void create(render_queue& queue, const layout_ctx& ctx, node* parent) override
  {
    size_t byte_start   = queue.current();
    size_t record_start = queue.records().size();
    
    auto size = measure(ctx);

    auto bg = queue.emplace(
      primitive::rect{
        .color         = m_style.background_color.value_or({}),
        .border_color  = m_style.border_color.value_or({}),
        .border_radius = m_style.border_radius.value_or(0),
        .size          = size,
      },
      vec2i{}
    );

    create_children(queue, ctx);
    layout_children(queue, ctx, { record_start, queue.records().size() });

    queue.trim_records(record_start);
    queue.push_record(byte_start, queue.current(), measure(ctx));
  }

  auto measure(const layout_ctx& ctx) -> vec2i override {
    vec2i size{};

    if (m_style.size.has_value()) {
      return m_style.size.value();
    }

    tuple_for_each(components, [&](auto& comp) -> void {
      const vec2i child = comp.measure(ctx);
      if (dir_ == direction::vertical) {
        size.x  = std::max(size.x, child.x);
        size.y += child.y + defaults::padding;
      } else {
        size.x += child.x + defaults::padding;
        size.y  = std::max(size.y, child.y);
      }
    });
    return size;
  }

  void create_children(render_queue& queue, const layout_ctx& ctx) {
    tuple_for_each(components, [&](auto& comp) {
      comp.create(queue, ctx, this);
    });
  }

  void layout_children(render_queue& queue, const layout_ctx& ctx, std::pair<size_t, size_t> range) {
    auto children_size = vec2i{};
    auto accumulator   = vec2i{};
    auto size = measure(ctx);
    for (auto i = range.first; i < range.second; i++) {
      auto record = queue.records().at(i);
      if (dir_ == direction::vertical) {
        children_size.y += record.computed_size.y + defaults::padding;
        children_size.x = std::max(children_size.x, record.computed_size.x);
      } else {
        children_size.x += record.computed_size.x + defaults::padding;
        children_size.y = std::max(children_size.y, record.computed_size.y);
      }
    }

    for (auto i = range.first; i < range.second; i++) {
      auto record = queue.records().at(i);
      queue.for_range(record.start, record.end, [&](render_cmd* cmd) {
        vec2i alignment_offset{};
        
        if (m_style.alignment.value_or({}).first == alignment::right) {
          alignment_offset.x = size.x - children_size.x;
        } else if (m_style.alignment.value_or({}).first == alignment::center) {
          alignment_offset.x = size.x / 2 -  children_size.x;
        }
        
        if (m_style.alignment.value_or({}).second == alignment::bottom) {
          alignment_offset.y = size.y - children_size.y;
        } else if (m_style.alignment.value_or({}).second == alignment::center) {
          alignment_offset.y = size.y / 2 -  children_size.y;
        }

        cmd->pos += accumulator + alignment_offset;
      });

      if (dir_ == direction::vertical) {

        accumulator.y += record.computed_size.y + defaults::padding;
      }
      else {
        accumulator.x += record.computed_size.x + defaults::padding;
      }
    }
  }

  auto compute(render_queue& queue, const layout_ctx& ctx) { create(queue, ctx, nullptr); }
};

template <UIElement ...Components>
struct columns final : layout<Components...> {
  using super = layout<Components...>;
  explicit columns(Components&&... args) : super(
    super::direction::horizontal,
    std::forward<Components>(args)...
  ){  }
};

template <UIElement ...Components>
struct rows final : layout<Components...> {
  using super = layout<Components...>;
  rows(Components&&... args) : super(
    super::direction::vertical,
    std::forward<Components>(args)...
  ){  }
};

} // namespace ui
