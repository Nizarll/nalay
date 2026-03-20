#pragma once

#include <type_traits>
#include <ostream>


template<typename T>
concept Number = std::is_arithmetic_v<T>;

template<Number T>
struct vec2 {
  T x, y;

  constexpr vec2() = default;
  constexpr vec2(T _x, T _y) : x(_x), y(_y) {}

  constexpr auto operator+(const vec2& rhs) const -> vec2  { return {x + rhs.x, y + rhs.y}; }
  constexpr auto operator-(const vec2& rhs) const -> vec2  { return {x - rhs.x, y - rhs.y}; }
  constexpr auto operator*(T scalar) const        -> vec2  { return {x * scalar, y * scalar}; }
  constexpr auto operator/(T scalar) const        -> vec2  { return {x / scalar, y / scalar}; }
  constexpr auto operator+=(const vec2& rhs)      -> vec2& { x += rhs.x; y += rhs.y; return *this; }
  constexpr auto operator-=(const vec2& rhs)      -> vec2& { x -= rhs.x; y -= rhs.y; return *this; }
  constexpr auto operator*=(T scalar)             -> vec2& { x *= scalar; y *= scalar; return *this; }
  constexpr auto operator/=(T scalar)             -> vec2& { x /= scalar; y /= scalar; return *this; }
  friend    auto operator<<(std::ostream& os, const vec2& v) -> std::ostream& { return os << "(" << v.x << ", " << v.y << ")"; }
};

template<Number T>
struct vec3 {
  T x, y, z;

  constexpr vec3() = default;
  constexpr vec3(T _x, T _y, T _z) : x(_x), y(_y), z(_z) {}

  constexpr auto operator+(const vec3& rhs) const -> vec3  { return {x + rhs.x, y + rhs.y, z + rhs.z}; }
  constexpr auto operator-(const vec3& rhs) const -> vec3  { return {x - rhs.x, y - rhs.y, z - rhs.z}; }
  constexpr auto operator*(T scalar) const        -> vec3  { return {x * scalar, y * scalar, z * scalar}; }
  constexpr auto operator/(T scalar) const        -> vec3  { return {x / scalar, y / scalar, z / scalar}; }
  constexpr auto operator+=(const vec3& rhs)      -> vec3& { x += rhs.x; y += rhs.y; z += rhs.z; return *this; }
  constexpr auto operator-=(const vec3& rhs)      -> vec3& { x -= rhs.x; y -= rhs.y; z -= rhs.z; return *this; }
  constexpr auto operator*=(T scalar)             -> vec3& { x *= scalar; y *= scalar; z *= scalar; return *this; }
  constexpr auto operator/=(T scalar)             -> vec3& { x /= scalar; y /= scalar; z /= scalar; return *this; }
  friend    auto operator<<(std::ostream& os, const vec3& v) -> std::ostream& { return os << "(" << v.x << ", " << v.y << ", " << v.z << ")"; }
};

template<Number T>
struct vec4 {
  T x, y, z, w;

  constexpr vec4() = default;
  constexpr vec4(T _x, T _y, T _z, T _w) : x(_x), y(_y), z(_z), w(_w) {}

  constexpr auto operator+(const vec4& rhs) const -> vec4  { return {x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w}; }
  constexpr auto operator-(const vec4& rhs) const -> vec4  { return {x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w}; }
  constexpr auto operator*(T scalar) const        -> vec4  { return {x * scalar, y * scalar, z * scalar, w * scalar}; }
  constexpr auto operator/(T scalar) const        -> vec4  { return {x / scalar, y / scalar, z / scalar, w / scalar}; }
  constexpr auto operator+=(const vec4& rhs)      -> vec4& { x += rhs.x; y += rhs.y; z += rhs.z; w += rhs.w; return *this; }
  constexpr auto operator-=(const vec4& rhs)      -> vec4& { x -= rhs.x; y -= rhs.y; z -= rhs.z; w -= rhs.w; return *this; }
  constexpr auto operator*=(T scalar)             -> vec4& { x *= scalar; y *= scalar; z *= scalar; w *= scalar; return *this; }
  constexpr auto operator/=(T scalar)             -> vec4& { x /= scalar; y /= scalar; z /= scalar; w /= scalar; return *this; }
  friend    auto operator<<(std::ostream& os, const vec4& v) -> std::ostream& { return os << "(" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ")"; }
};

struct insets {
  constexpr insets() = default;
  constexpr insets(int top_left) : m_val{top_left, 0, 0, 0} {}
  constexpr insets(int top_left, int top_right) : m_val{top_left, top_right, 0, 0} {}
  constexpr insets(int top_left, int top_right, int bottom_left) : m_val{top_left, top_right, bottom_left, 0} {}
  constexpr insets(int top_left, int top_right, int bottom_left, int bottom_right) : m_val{top_left, top_right, bottom_left, bottom_right} {}

  static constexpr auto all(int val)                -> insets { return insets{val, val, val, val}; }
  static constexpr auto top(int val)                -> insets { return insets{ val, val }; }
  static constexpr auto top(int left, int right)    -> insets { return insets{ left, right }; }
  static constexpr auto bottom(int val)             -> insets { return insets{0, 0, val, val }; }
  static constexpr auto bottom(int left, int right) -> insets { return insets{ 0, 0, left, right }; }

  auto top_left()     const -> int { return m_val.x; }
  auto top_right()    const -> int { return m_val.y; }
  auto bottom_left()  const -> int { return m_val.z; }
  auto bottom_right() const -> int { return m_val.w; }
private:
  vec4<int> m_val;
};

struct color {
  constexpr color() = default;
  constexpr color(uint8_t r, uint8_t g, uint8_t b) : m_val(r, g, b, 255) {}
  constexpr color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) : m_val(r, g, b, a) {}

  static constexpr auto from_float(float r, float g, float b) {
    return color{
      static_cast<uint8_t>(r * 255),
      static_cast<uint8_t>(g * 255),
      static_cast<uint8_t>(b * 255),
    };
  }

  static constexpr auto from_hex(uint32_t value) -> color {
    return color{
      static_cast<uint8_t>((value >> 24) & 0x000000ff),
      static_cast<uint8_t>((value >> 16) & 0x000000ff),
      static_cast<uint8_t>((value >> 8 ) & 0x000000ff),
      static_cast<uint8_t>((value >> 0 ) & 0x000000ff)
    };
  }

  constexpr auto with_alpha(uint8_t alpha) -> color {
    return color{
      get_r(),
      get_g(),
      get_b(),
      alpha
    };
  }

  auto get_r() const -> uint8_t { return m_val.x; }
  auto get_g() const -> uint8_t { return m_val.y; }
  auto get_b() const -> uint8_t { return m_val.z; }
  auto get_a() const -> uint8_t { return m_val.w; }

  auto get_r_float() const -> float { return m_val.x / 255.0f; }
  auto get_g_float() const -> float { return m_val.y / 255.0f; }
  auto get_b_float() const -> float { return m_val.z / 255.0f; }
  auto get_a_float() const -> float { return m_val.w / 255.0f; }

  static constexpr auto black()  -> color { return color{0, 0, 0, 0}; }
  static constexpr auto white()  -> color { return color{255, 255, 255, 255}; }
  static constexpr auto red()    -> color { return color{255, 0, 0, 255}; }
  static constexpr auto green()  -> color { return color{0, 255, 0, 255}; }
  static constexpr auto blue()   -> color { return color{0, 0, 255, 255}; }
  static constexpr auto cyan()   -> color { return color{0, 255, 255, 255}; }
  static constexpr auto yellow() -> color { return color{255, 255, 0, 255}; }
  static constexpr auto purple() -> color { return color{255, 0, 255, 255}; }
  static constexpr auto pink()   -> color { return color{255, 125, 220, 255}; }

private:
  vec4<uint8_t> m_val;
};

using vec4f = vec4<float>;
using vec4d = vec4<double>;
using vec4i = vec4<int>;

using vec3f = vec3<float>;
using vec3d = vec3<double>;
using vec3i = vec3<int>;

using vec2f = vec2<float>;
using vec2d = vec2<double>;
using vec2i = vec2<int>;


