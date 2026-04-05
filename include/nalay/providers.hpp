#pragma once

#include <variant>
#include "types.hpp"
#include "allocators.hpp"

namespace nalay {

enum class image_format {
  rgba8,
  rgb8,
  alpha8
};

enum class mouse_button : uint8_t { left, right, middle };
enum class key_action   : uint8_t { press, release, repeat };

enum class key : uint16_t {
  null = 0,
  space         = ' ',
  apostrophe    = '\'',
  comma         = ',',
  minus         = '-',
  period        = '.',
  slash         = '/',
  zero          = '0',
  one           = '1',
  two           = '2',
  three         = '3',
  four          = '4',
  five          = '5',
  six           = '6',
  seven         = '7',
  eight         = '8',
  nine          = '9',
  semicolon     = ';',
  equal         = '=',
  a             = 'a',
  b             = 'b',
  c             = 'c',
  d             = 'd',
  e             = 'e',
  f             = 'f',
  g             = 'g',
  h             = 'h',
  i             = 'i',
  j             = 'j',
  k             = 'k',
  l             = 'l',
  m             = 'm',
  n             = 'n',
  o             = 'o',
  p             = 'p',
  q             = 'q',
  r             = 'r',
  s             = 's',
  t             = 't',
  u             = 'u',
  v             = 'v',
  w             = 'w',
  x             = 'x',
  y             = 'y',
  z             = 'z',
  uppercase_a   = 'A',
  uppercase_b   = 'B',
  uppercase_c   = 'C',
  uppercase_d   = 'D',
  uppercase_e   = 'E',
  uppercase_f   = 'F',
  uppercase_g   = 'G',
  uppercase_h   = 'H',
  uppercase_i   = 'I',
  uppercase_j   = 'J',
  uppercase_k   = 'K',
  uppercase_l   = 'L',
  uppercase_m   = 'M',
  uppercase_n   = 'N',
  uppercase_o   = 'O',
  uppercase_p   = 'P',
  uppercase_q   = 'Q',
  uppercase_r   = 'R',
  uppercase_s   = 'S',
  uppercase_t   = 'T',
  uppercase_u   = 'U',
  uppercase_v   = 'V',
  uppercase_w   = 'W',
  uppercase_x   = 'X',
  uppercase_y   = 'Y',
  uppercase_z   = 'Z',
  left_bracket  = '[',
  backslash     = '\\',
  right_bracket = ']',
  grave         = '`',
  escape        = 256,
  enter,
  tab,
  backspace,
  insert,
  del,
  right,
  left,
  down,
  up,
  page_up,
  page_down,
  home,
  end,
  caps_lock,
  scroll_lock,
  num_lock,
  print_screen,
  pause,
  f1, f2, f3, f4, f5,  f6,
  f7, f8, f9, f10, f11, f12,
  left_shift,
  left_control,
  left_alt,
  left_super,
  right_shift,
  right_control,
  right_alt,
  right_super,
};

struct mouse_event {
  vec2i pos;
  vec2i delta;
  mouse_button button;
  key_action action;
};

struct keyboard_event {
  char32_t codepoint;
  nalay::key key;
  key_action action;
};


struct font_metrics {
  float ascent;
  float descent;
  float line_gap;
};

struct image_info {
  image_format format;
  vec2i size;
  uint32_t mipmaps;
};

struct texture {
  texture()                      = default;
  virtual ~texture()             = default;
  texture(const texture&)        = delete;
  texture(texture&&)             = delete;
  void operator=(const texture&) = delete;
  void operator=(texture&&)      = delete;

  virtual auto get_info()      const -> image_info = 0;
};

struct font {
  font()                      = default;
  virtual ~font()             = default;
  font(const font&)           = delete;
  font(font&&)                = delete;
  void operator=(const font&) = delete;
  void operator=(font&&)      = delete;

  virtual auto measure(const std::string& text, int font_size, int spacing) const -> vec2i        = 0;
  virtual auto get_metrics()                                                const -> font_metrics = 0;
};

struct font_provider {
  font_provider()                      = default;
  virtual ~font_provider()             = default;
  font_provider(const font_provider&)  = delete;
  font_provider(font_provider&&)       = delete;
  void operator=(const font_provider&) = delete;
  void operator=(font_provider&&)      = delete;

  virtual void load(std::string_view name, std::string_view path) = 0;
  virtual auto get(std::string_view name) const -> const font&   = 0;
  virtual auto default_font()             const -> const font&   = 0;
};

struct image_provider {
  image_provider()                      = default;
  virtual ~image_provider()             = default;
  image_provider(const image_provider&) = delete;
  image_provider(image_provider&&)      = delete;
  void operator=(const image_provider&) = delete;
  void operator=(image_provider&&)      = delete;

  virtual auto load(std::string_view path) -> const font& = 0;
};

struct input_provider {
  input_provider()                       = default;
  virtual ~input_provider()              = default;
  input_provider(const input_provider&)  = delete;
  input_provider(input_provider&&)       = delete;
  void operator=(const input_provider&)  = delete;
  void operator=(input_provider&&)       = delete;

  virtual void poll() = 0;

  virtual auto mouse_pos()                  const -> vec2i = 0;
  virtual auto mouse_delta()                const -> vec2i = 0;
  virtual auto mouse_pressed(mouse_button)  const -> bool  = 0;
  virtual auto mouse_released(mouse_button) const -> bool  = 0;
  virtual auto mouse_down(mouse_button) const     -> bool  = 0;

  virtual auto key_pressed(nalay::key key)  const -> bool = 0;
  virtual auto key_released(nalay::key key) const -> bool = 0;
  virtual auto key_down(nalay::key key)     const -> bool = 0;

  virtual auto events() const -> std::span<const std::variant<keyboard_event, mouse_event>> = 0;
};

namespace ui { struct node; } // hacky fix

struct context {

  std::reference_wrapper<font_provider>  fonts;
  std::reference_wrapper<input_provider> inputs;
  slab_allocator<ui::node*> node_allocator;
  
  ui::node* focused_node = nullptr;
  float delta_time = .0f;

  template <typename T, typename... Args> requires std::derived_from<T, ui::node>
  auto make_node(Args&&... args) -> T* {
    T* ptr = node_allocator.allocate_as<T>(1);
    if (not ptr) throw std::bad_alloc();
    return std::construct_at(ptr, std::forward<Args>(args)...);
  }
};

template<typename T>
struct renderer_ {
  renderer_()                      = default;
  virtual ~renderer_()             = default;
  renderer_(const font_provider&)  = delete;
  renderer_(font_provider&&)       = delete;
  void operator=(const renderer_&) = delete;
  void operator=(renderer_&&)      = delete;
  virtual void render(const T& data) = 0;
};

} // namespace nalay
