#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <raylib.h>
#include <rlgl.h>

#include "nalay.hpp"
#include "nalay/defaults.hpp"

struct raylib_font : public font {
  explicit raylib_font(std::string_view path) {
    m_font = LoadFont(path.data());
    if (m_font.texture.id == 0)
      throw std::runtime_error(std::string("raylib_font: failed to load '") + path.data() + "'");
  }

  explicit raylib_font(Font font) : m_font(font) {
    if (m_font.texture.id == 0) throw std::runtime_error("raylib_font: failed to load ");
  }

  ~raylib_font() override {
    UnloadFont(m_font);
  }

  auto measure(const std::string& text, int font_size, int spacing) const -> vec2i override {
    const Vector2 size  = MeasureTextEx(m_font, text.c_str(),
                                        static_cast<float>(font_size), static_cast<float>(spacing));
    return { static_cast<int>(size.x), static_cast<int>(size.y) };
  }

  auto get_metrics() const -> font_metrics override {
    const float base = static_cast<float>(m_font.baseSize);
    return font_metrics{
      .ascent   =  base * 0.8f,// ~80 % of em above baseline
      .descent  = -base * 0.2f,// ~20 % below baseline (negative convention)
      .line_gap =  base * 0.1f,
    };
  }

  auto raw() const -> const Font& { return m_font; }
private:
  Font m_font{};
};

struct raylib_font_provider : public font_provider {

  raylib_font_provider() { fonts_.emplace("default", std::make_unique<raylib_font>(GetFontDefault())); }
  ~raylib_font_provider() = default;

  void load(std::string_view name, std::string_view path) override {
    auto [it, inserted] = fonts_.try_emplace(std::string(name), std::make_unique<raylib_font>(path));
    if (!inserted) throw std::runtime_error(std::string("raylib_font_provider: '") + name.data() + "' is already loaded");
  }

  auto get(std::string_view name) const -> const font& override {
    const auto it = fonts_.find(std::string(name));
    if (it == fonts_.end()) throw std::runtime_error(std::string("raylib_font_provider: '") + name.data() + "' not found");
    return *it->second;
  }

  auto default_font() const -> const font& override { return get("default"); }

private:
  std::unordered_map<std::string, std::unique_ptr<raylib_font>> fonts_;
};

struct raylib_renderer: public renderer {

  raylib_renderer() = default;

  void render(render_queue& queue)
  {
    queue.for_each([this](const render_cmd& cmd){ render(cmd); });
  }

  void render(const render_cmd& cmd) override {
    std::visit([this, cmd](auto&& arg) {
      using T = std::decay_t<decltype(arg)>;
      if constexpr(std::same_as<T, primitive::rect>)
        render_rect(cmd);
      else if(std::same_as<T, primitive::img>)
        render_img(cmd);
      else if(std::same_as<T, primitive::text>)
        render_text(cmd);
    },
               cmd.content);
  }
private:
  void render_rect(const render_cmd& cmd)
  {
    auto content = std::get<primitive::rect>(cmd.content);
    DrawRectangleRounded(
      {
        static_cast<float>(cmd.pos.x),
        static_cast<float>(cmd.pos.y),
        static_cast<float>(content.size.x),
        static_cast<float>(content.size.y)
      },
      content.border_radius * 0.01,//TODO: implement a DrawRectangleRounded function 
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
        content.border_radius * 0.01,//TODO: implement a DrawRectangleRounded function 
        20,
        content.border_size > 0 ? (content.border_size + 0.1f) : 0.0f,
        Color{
          static_cast<unsigned char>(content.border_color.get_r()),
          static_cast<unsigned char>(content.border_color.get_g()),
          static_cast<unsigned char>(content.border_color.get_b()),
          static_cast<unsigned char>(content.border_color.get_a())
        }
      );
  }
  
  void render_img (const render_cmd& cmd)
  {
    //TODO:
  }

  void render_text(const render_cmd& cmd)
  {
    auto content = std::get<primitive::text>(cmd.content);
    DrawTextEx(
      dynamic_cast<const raylib_font*>(&content.font.get())->raw(),
      std::get<primitive::text>(cmd.content).text.data(),
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

