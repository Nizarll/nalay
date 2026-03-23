#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string_view>
#include "types.hpp"
#include "allocators.hpp"

enum class image_format {
  rgba8,
  rgb8,
  alpha8
};

struct font_metrics {
  float ascent;
  float descent;
  float line_gap;
};

struct image_info {
  uint32_t width;
  uint32_t height;
  image_format format;
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

  virtual void load(std::string_view name, std::string_view path) = 0;
  virtual auto size_of(std::string_view name)      const -> vec2i = 0;
};

namespace ui { struct node; } // hacky fix

struct layout_ctx {
  slab_allocator<ui::node*> node_allocator; // Storage is shared via shared_ptr
  std::reference_wrapper<font_provider> fonts;

  template <typename T, typename... Args> 
  requires std::derived_from<T, ui::node>
  auto make_node(Args&&... args) -> T* {
    T* ptr = node_allocator.allocate_as<T>(1);
    if (!ptr) throw std::bad_alloc();
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

