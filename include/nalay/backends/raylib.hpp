#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <raylib.h>
#include <rlgl.h>

#include "nalay/nalay.hpp"
#include "nalay/providers.hpp"

// utilities

static auto from_raylib_key(KeyboardKey k) -> nalay::key;
static auto to_raylib_key(nalay::key k) -> KeyboardKey;
static auto to_raylib_mouse(nalay::mouse_button btn) -> int;

//

struct raylib_font : public nalay::font {
  explicit raylib_font(std::string_view path) {
    m_font = LoadFont(path.data());
    if (m_font.texture.id == 0)
      throw std::runtime_error(std::string("raylib_font: failed to load '") + path.data() + "'");

    SetTextureFilter(m_font.texture, TEXTURE_FILTER_BILINEAR);
  }

  explicit raylib_font(Font f) : m_font(f) {
    if (m_font.texture.id == 0) throw std::runtime_error("raylib_font: failed to load ");
    SetTextureFilter(m_font.texture, TEXTURE_FILTER_BILINEAR);
  }

  ~raylib_font() override {
    UnloadFont(m_font);
  }

  auto measure(const std::string& text, int font_size, int spacing) const -> nalay::vec2i override {
    const Vector2 size  = MeasureTextEx(m_font, text.c_str(),
                                        static_cast<float>(font_size), static_cast<float>(spacing));
    return { static_cast<int>(size.x), static_cast<int>(size.y) };
  }

  auto get_metrics() const -> nalay::font_metrics override {
    const float base = static_cast<float>(m_font.baseSize);
    return nalay::font_metrics{
      .ascent   =  base * 0.8f,// ~80 % of em above baseline
      .descent  = -base * 0.2f,// ~20 % below baseline (negative convention)
      .line_gap =  base * 0.1f,
    };
  }

  auto raw() const -> const Font& { return m_font; }
private:
  Font m_font{};
};


struct raylib_font_provider : public nalay::font_provider {

  raylib_font_provider() { fonts_.emplace("default", std::make_unique<raylib_font>(GetFontDefault())); }
  ~raylib_font_provider() = default;

  void load(std::string_view name, std::string_view path) override {
    auto [it, inserted] = fonts_.try_emplace(std::string(name), std::make_unique<raylib_font>(path));
    if (!inserted) throw std::runtime_error(std::string("raylib_font_provider: '") + name.data() + "' is already loaded");
  }

  auto get(std::string_view name) const -> const nalay::font& override {
    const auto it = fonts_.find(std::string(name));
    if (it == fonts_.end()) throw std::runtime_error(std::string("raylib_font_provider: '") + name.data() + "' not found");
    return *it->second;
  }

  auto set_default(std::string_view name) { fonts_["default"] = std::make_unique<raylib_font>(name); }

  auto default_font() const -> const nalay::font& override { return get("default"); }

private:
  std::unordered_map<std::string, std::unique_ptr<raylib_font>> fonts_;
};

struct raylib_texture : public nalay::texture {
  explicit raylib_texture(std::string_view path) {
    Image img = LoadImage(path.data());
    if (img.data == nullptr)
      throw std::runtime_error(std::string("raylib_texture: failed to load '") + path.data() + "'");

    m_texture = LoadTextureFromImage(img);
    UnloadImage(img);

    if (m_texture.id == 0)
      throw std::runtime_error(std::string("raylib_texture: failed to create texture from '") + path.data() + "'");

    SetTextureFilter(m_texture, TEXTURE_FILTER_BILINEAR);
  }

  explicit raylib_texture(Texture2D tex) : m_texture(tex) {
    if (m_texture.id == 0)
      throw std::runtime_error("raylib_texture: invalid texture");
    SetTextureFilter(m_texture, TEXTURE_FILTER_BILINEAR);
  }

  ~raylib_texture() override {
    UnloadTexture(m_texture);
  }

  auto get_info() const -> nalay::image_info override {
    nalay::image_format format;
    switch (m_texture.format) {
      case PIXELFORMAT_UNCOMPRESSED_R8G8B8A8:
        format = nalay::image_format::rgba8;
        break;
      case PIXELFORMAT_UNCOMPRESSED_R8G8B8:
        format = nalay::image_format::rgb8;
        break;
      case PIXELFORMAT_UNCOMPRESSED_GRAYSCALE:
        format = nalay::image_format::alpha8;
        break;
      default:
        format = nalay::image_format::rgba8;
        break;
    }

    return nalay::image_info{
      .format  = format,
      .size    = nalay::vec2i{ m_texture.width, m_texture.height },
      .mipmaps = static_cast<uint32_t>(m_texture.mipmaps)
    };
  }

  auto raw() const -> const Texture2D& { return m_texture; }

private:
  Texture2D m_texture{};
};

struct raylib_image_provider : public nalay::image_provider {
  raylib_image_provider() = default;
  ~raylib_image_provider() = default;

  void load(std::string_view name, std::string_view path) override {
    auto [it, inserted] = textures_.try_emplace(std::string(name), std::make_unique<raylib_texture>(path));
    if (!inserted)
      throw std::runtime_error(std::string("raylib_image_provider: '") + name.data() + "' is already loaded");
  }

  auto get(std::string_view name) const -> const nalay::texture& override {
    const auto it = textures_.find(std::string(name));
    if (it == textures_.end())
      throw std::runtime_error(std::string("raylib_image_provider: '") + name.data() + "' not found");
    return *it->second;
  }

private:
  std::unordered_map<std::string, std::unique_ptr<raylib_texture>> textures_;
};

struct raylib_renderer: public nalay::renderer {

  raylib_renderer() = default;

  void render(nalay::render_queue& queue)
  {
    queue.for_each([this](const nalay::render_cmd& cmd){ render(cmd); });
  }

  void render(const nalay::render_cmd& cmd) override {
    std::visit([this, cmd](auto&& arg) {
      using T = std::decay_t<decltype(arg)>;
      if constexpr(std::same_as<T, nalay::primitive::rect>)
        render_rect(cmd);
      else if constexpr(std::same_as<T, nalay::primitive::img>)
        render_img(cmd);
      else if constexpr(std::same_as<T, nalay::primitive::text>)
        render_text(cmd);
    },
               cmd.content);
  }
private:
  void render_rect(const nalay::render_cmd& cmd)
  {
    auto content = std::get<nalay::primitive::rect>(cmd.content);
    DrawRectangleRounded(
      {
        static_cast<float>(cmd.pos.x),
        static_cast<float>(cmd.pos.y),
        static_cast<float>(content.size.x),
        static_cast<float>(content.size.y)
      },
      static_cast<float>(content.border_radius) * 0.01f,//TODO: implement a DrawRectangleRounded function 
      20,
      Color{
        static_cast<unsigned char>(content.color.get_r()),
        static_cast<unsigned char>(content.color.get_g()),
        static_cast<unsigned char>(content.color.get_b()),
        static_cast<unsigned char>(content.color.get_a())
      }
    );
    if (content.border_size > 0)
      DrawRectangleRoundedLinesEx(
        {
          static_cast<float>(cmd.pos.x),
          static_cast<float>(cmd.pos.y),
          static_cast<float>(content.size.x),
          static_cast<float>(content.size.y)
        },
        static_cast<float>(content.border_radius) * 0.01f,
        //TODO: implement a DrawRectangleRounded function 
        20,
        content.border_size > 0 ? (static_cast<float>(content.border_size) + 0.1f) : 0.0f,
        Color{
          static_cast<unsigned char>(content.border_color.get_r()),
          static_cast<unsigned char>(content.border_color.get_g()),
          static_cast<unsigned char>(content.border_color.get_b()),
          static_cast<unsigned char>(content.border_color.get_a())
        }
      );
  }

  void render_img(const nalay::render_cmd& cmd)
  {
    auto content = std::get<nalay::primitive::img>(cmd.content);
    const auto* raylib_tex = dynamic_cast<const raylib_texture*>(&content.texture.get());

    if (!raylib_tex) {
      // Fallback: render a placeholder rectangle if texture isn't a raylib texture
      DrawRectangle(cmd.pos.x, cmd.pos.y, content.size.x, content.size.y, PINK);
      return;
    }

    const auto& tex = raylib_tex->raw();

    // Calculate scale to fit content.size
    float scale_x = static_cast<float>(content.size.x) / static_cast<float>(tex.width);
    float scale_y = static_cast<float>(content.size.y) / static_cast<float>(tex.height);

    // Use uniform scaling (fit image within bounds, preserving aspect ratio)
    float scale = std::min(scale_x, scale_y);

    // Center the image if it doesn't fill the entire area
    int scaled_w = static_cast<int>(tex.width * scale);
    int scaled_h = static_cast<int>(tex.height * scale);
    int offset_x = (content.size.x - scaled_w) / 2;
    int offset_y = (content.size.y - scaled_h) / 2;

    DrawTextureEx(
      tex,
      Vector2{
        static_cast<float>(cmd.pos.x + offset_x),
        static_cast<float>(cmd.pos.y + offset_y)
      },
      0.0f,  // rotation
      scale, // scale to fit
      WHITE
    );
  }

  void render_text(const nalay::render_cmd& cmd)
  {
    auto content = std::get<nalay::primitive::text>(cmd.content);
    DrawTextEx(
      dynamic_cast<const raylib_font*>(&content.font.get())->raw(),
      std::get<nalay::primitive::text>(cmd.content).text.data(),
      Vector2{
        static_cast<float>(cmd.pos.x),
        static_cast<float>(cmd.pos.y)
      },
      static_cast<float>(content.font_size),
      static_cast<float>(content.letter_spacing),
      Color{
        static_cast<unsigned char>(content.text_color.get_r()),
        static_cast<unsigned char>(content.text_color.get_g()),
        static_cast<unsigned char>(content.text_color.get_b()),
        static_cast<unsigned char>(content.text_color.get_a())
      }
    );
  }
};

struct raylib_input_provider : public nalay::input_provider {
  void poll() override {
    m_events.clear();

    for (auto [nb, rb] : {
      std::pair{nalay::mouse_button::left,   MOUSE_BUTTON_LEFT},
      std::pair{nalay::mouse_button::right,  MOUSE_BUTTON_RIGHT},
      std::pair{nalay::mouse_button::middle, MOUSE_BUTTON_MIDDLE}
    }) {
      if (IsMouseButtonPressed(rb)) {
        m_events.emplace_back(nalay::mouse_event{
          .pos    = mouse_pos(),
          .delta  = mouse_delta(),
          .button = nb,
          .action = nalay::key_action::press
        });
      } else if (IsMouseButtonReleased(rb)) {
        m_events.emplace_back(nalay::mouse_event{
          .pos    = mouse_pos(),
          .delta  = mouse_delta(),
          .button = nb,
          .action = nalay::key_action::release
        });
      }
    }

    int cp = 0;
    while ((cp = GetCharPressed()) != 0) {
      m_events.emplace_back(nalay::keyboard_event{
        .codepoint = static_cast<char32_t>(cp),
        .key       = from_raylib_key(static_cast<KeyboardKey>(cp)),
        .action    = nalay::key_action::press,
      });
    }

    int key = 0;
    while ((key = GetKeyPressed()) != 0) {
      if (key >= 32 && key < 256) continue; // already captured by GetCharPressed
      m_events.emplace_back(nalay::keyboard_event{
        .codepoint = 0,
        .key       = from_raylib_key(static_cast<KeyboardKey>(key)),
        .action    = nalay::key_action::press,
      });
    }
  }

  auto mouse_pos()   const -> nalay::vec2i override {
    auto pos = GetMousePosition();
    return { static_cast<int>(pos.x), static_cast<int>(pos.y) };
  }

  auto mouse_delta() const -> nalay::vec2i override {
    auto delta = GetMouseDelta();
    return { static_cast<int>(delta.x), static_cast<int>(delta.y) };
  }

  auto mouse_pressed (nalay::mouse_button b) const -> bool override { return IsMouseButtonPressed (to_raylib_mouse(b)); }
  auto mouse_released(nalay::mouse_button b) const -> bool override { return IsMouseButtonReleased(to_raylib_mouse(b)); }
  auto mouse_down    (nalay::mouse_button b) const -> bool override { return IsMouseButtonDown    (to_raylib_mouse(b)); }

  auto key_pressed (nalay::key k) const -> bool override { return IsKeyPressed(to_raylib_key(k));  }
  auto key_released(nalay::key k) const -> bool override { return IsKeyReleased(to_raylib_key(k)); }
  auto key_down    (nalay::key k) const -> bool override { return IsKeyDown(to_raylib_key(k));     }

  auto events() const -> std::span<const std::variant<nalay::keyboard_event, nalay::mouse_event>> override
  {
    return m_events;
  }

private:
  std::vector<std::variant<nalay::keyboard_event, nalay::mouse_event>> m_events;
};

auto from_raylib_key(KeyboardKey k) -> nalay::key {
  if (k >= 'A' && k <= 'Z') return static_cast<nalay::key>(k + 32);

  switch (k) {
    case KEY_SPACE:         return nalay::key::space;
    case KEY_APOSTROPHE:    return nalay::key::apostrophe;
    case KEY_COMMA:         return nalay::key::comma;
    case KEY_MINUS:         return nalay::key::minus;
    case KEY_PERIOD:        return nalay::key::period;
    case KEY_SLASH:         return nalay::key::slash;
    case KEY_ZERO:          return nalay::key::zero;
    case KEY_ONE:           return nalay::key::one;
    case KEY_TWO:           return nalay::key::two;
    case KEY_THREE:         return nalay::key::three;
    case KEY_FOUR:          return nalay::key::four;
    case KEY_FIVE:          return nalay::key::five;
    case KEY_SIX:           return nalay::key::six;
    case KEY_SEVEN:         return nalay::key::seven;
    case KEY_EIGHT:         return nalay::key::eight;
    case KEY_NINE:          return nalay::key::nine;
    case KEY_SEMICOLON:     return nalay::key::semicolon;
    case KEY_EQUAL:         return nalay::key::equal;
    case KEY_LEFT_BRACKET:  return nalay::key::left_bracket;
    case KEY_BACKSLASH:     return nalay::key::backslash;
    case KEY_RIGHT_BRACKET: return nalay::key::right_bracket;
    case KEY_GRAVE:         return nalay::key::grave;
    case KEY_ESCAPE:        return nalay::key::escape;
    case KEY_ENTER:         return nalay::key::enter;
    case KEY_TAB:           return nalay::key::tab;
    case KEY_BACKSPACE:     return nalay::key::backspace;
    case KEY_INSERT:        return nalay::key::insert;
    case KEY_DELETE:        return nalay::key::del;
    case KEY_RIGHT:         return nalay::key::right;
    case KEY_LEFT:          return nalay::key::left;
    case KEY_DOWN:          return nalay::key::down;
    case KEY_UP:            return nalay::key::up;
    case KEY_PAGE_UP:       return nalay::key::page_up;
    case KEY_PAGE_DOWN:     return nalay::key::page_down;
    case KEY_HOME:          return nalay::key::home;
    case KEY_END:           return nalay::key::end;
    case KEY_CAPS_LOCK:     return nalay::key::caps_lock;
    case KEY_SCROLL_LOCK:   return nalay::key::scroll_lock;
    case KEY_NUM_LOCK:      return nalay::key::num_lock;
    case KEY_PRINT_SCREEN:  return nalay::key::print_screen;
    case KEY_PAUSE:         return nalay::key::pause;
    case KEY_F1:            return nalay::key::f1;
    case KEY_F2:            return nalay::key::f2;
    case KEY_F3:            return nalay::key::f3;
    case KEY_F4:            return nalay::key::f4;
    case KEY_F5:            return nalay::key::f5;
    case KEY_F6:            return nalay::key::f6;
    case KEY_F7:            return nalay::key::f7;
    case KEY_F8:            return nalay::key::f8;
    case KEY_F9:            return nalay::key::f9;
    case KEY_F10:           return nalay::key::f10;
    case KEY_F11:           return nalay::key::f11;
    case KEY_F12:           return nalay::key::f12;
    case KEY_LEFT_SHIFT:    return nalay::key::left_shift;
    case KEY_LEFT_CONTROL:  return nalay::key::left_control;
    case KEY_LEFT_ALT:      return nalay::key::left_alt;
    case KEY_LEFT_SUPER:    return nalay::key::left_super;
    case KEY_RIGHT_SHIFT:   return nalay::key::right_shift;
    case KEY_RIGHT_CONTROL: return nalay::key::right_control;
    case KEY_RIGHT_ALT:     return nalay::key::right_alt;
    case KEY_RIGHT_SUPER:   return nalay::key::right_super;
    default:                return nalay::key::null;
  }
}

auto to_raylib_key(nalay::key k) -> KeyboardKey {
  auto v = static_cast<uint16_t>(k);

  if (v >= 'a' && v <= 'z') return static_cast<KeyboardKey>(v - 32);
  if (v >= 'A' && v <= 'Z') return static_cast<KeyboardKey>(v);

  switch (k) {
    case nalay::key::space:         return KEY_SPACE;
    case nalay::key::apostrophe:    return KEY_APOSTROPHE;
    case nalay::key::comma:         return KEY_COMMA;
    case nalay::key::minus:         return KEY_MINUS;
    case nalay::key::period:        return KEY_PERIOD;
    case nalay::key::slash:         return KEY_SLASH;
    case nalay::key::zero:          return KEY_ZERO;
    case nalay::key::one:           return KEY_ONE;
    case nalay::key::two:           return KEY_TWO;
    case nalay::key::three:         return KEY_THREE;
    case nalay::key::four:          return KEY_FOUR;
    case nalay::key::five:          return KEY_FIVE;
    case nalay::key::six:           return KEY_SIX;
    case nalay::key::seven:         return KEY_SEVEN;
    case nalay::key::eight:         return KEY_EIGHT;
    case nalay::key::nine:          return KEY_NINE;
    case nalay::key::semicolon:     return KEY_SEMICOLON;
    case nalay::key::equal:         return KEY_EQUAL;
    case nalay::key::left_bracket:  return KEY_LEFT_BRACKET;
    case nalay::key::backslash:     return KEY_BACKSLASH;
    case nalay::key::right_bracket: return KEY_RIGHT_BRACKET;
    case nalay::key::grave:         return KEY_GRAVE;
    case nalay::key::escape:        return KEY_ESCAPE;
    case nalay::key::enter:         return KEY_ENTER;
    case nalay::key::tab:           return KEY_TAB;
    case nalay::key::backspace:     return KEY_BACKSPACE;
    case nalay::key::insert:        return KEY_INSERT;
    case nalay::key::del:           return KEY_DELETE;
    case nalay::key::right:         return KEY_RIGHT;
    case nalay::key::left:          return KEY_LEFT;
    case nalay::key::down:          return KEY_DOWN;
    case nalay::key::up:            return KEY_UP;
    case nalay::key::page_up:       return KEY_PAGE_UP;
    case nalay::key::page_down:     return KEY_PAGE_DOWN;
    case nalay::key::home:          return KEY_HOME;
    case nalay::key::end:           return KEY_END;
    case nalay::key::caps_lock:     return KEY_CAPS_LOCK;
    case nalay::key::scroll_lock:   return KEY_SCROLL_LOCK;
    case nalay::key::num_lock:      return KEY_NUM_LOCK;
    case nalay::key::print_screen:  return KEY_PRINT_SCREEN;
    case nalay::key::pause:         return KEY_PAUSE;
    case nalay::key::f1:            return KEY_F1;
    case nalay::key::f2:            return KEY_F2;
    case nalay::key::f3:            return KEY_F3;
    case nalay::key::f4:            return KEY_F4;
    case nalay::key::f5:            return KEY_F5;
    case nalay::key::f6:            return KEY_F6;
    case nalay::key::f7:            return KEY_F7;
    case nalay::key::f8:            return KEY_F8;
    case nalay::key::f9:            return KEY_F9;
    case nalay::key::f10:           return KEY_F10;
    case nalay::key::f11:           return KEY_F11;
    case nalay::key::f12:           return KEY_F12;
    case nalay::key::left_shift:    return KEY_LEFT_SHIFT;
    case nalay::key::left_control:  return KEY_LEFT_CONTROL;
    case nalay::key::left_alt:      return KEY_LEFT_ALT;
    case nalay::key::left_super:    return KEY_LEFT_SUPER;
    case nalay::key::right_shift:   return KEY_RIGHT_SHIFT;
    case nalay::key::right_control: return KEY_RIGHT_CONTROL;
    case nalay::key::right_alt:     return KEY_RIGHT_ALT;
    case nalay::key::right_super:   return KEY_RIGHT_SUPER;
    default:                        return KEY_NULL;
  }
}

auto to_raylib_mouse(nalay::mouse_button btn) -> int {
  switch (btn) {
    case nalay::mouse_button::left:   return MOUSE_BUTTON_LEFT;
    case nalay::mouse_button::right:  return MOUSE_BUTTON_RIGHT;
    case nalay::mouse_button::middle: return MOUSE_BUTTON_MIDDLE;
  }
  return MOUSE_BUTTON_LEFT; // default fallback
}
