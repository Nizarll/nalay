// allocators.hpp
#pragma once

#include <cstddef>
#include <list>
#include <memory>
#include <array>
#include <new>
#include <type_traits>

namespace nalay {

template <typename T>
struct arena
{
  arena() : m_storage(new aligned_storage) {}
  ~arena() = default;

  auto is_full() const -> bool { return m_current >= aligned_size; }
  auto current() const -> std::size_t { return m_current; }

  template <typename U>
  auto allocate_bytes(std::size_t n = 1) -> U* {
    const std::size_t total_bytes = n * sizeof(U);
    void* ptr = m_storage->buff.data() + m_current;
    std::size_t space = buffer_size - m_current;

    void* aligned = std::align(alignof(U), total_bytes, ptr, space);
    if (not aligned) return nullptr;

    auto* next = static_cast<std::byte*>(aligned) + total_bytes;
    m_current = static_cast<std::size_t>(next - m_storage->buff.data());
    return static_cast<U*>(aligned);
  }

  auto make(auto&&... args) -> T*
  {
    void*        ptr     = m_storage->buff.data() + m_current;
    std::size_t  space   = aligned_size - m_current;
    void*        aligned = std::align(alignof(T), sizeof(T), ptr, space);
    if (not aligned) return nullptr;
    auto* next = static_cast<std::byte*>(aligned) + sizeof(T);
    if (next > m_storage->buff.data() + aligned_size) return nullptr;
    T* obj = nullptr;
    try {
      obj = std::construct_at(static_cast<T*>(aligned),
                              std::forward<decltype(args)>(args)...);
    } catch (...) { return nullptr; }
    m_current = static_cast<std::size_t>(next - m_storage->buff.data());
    return obj;
  }
  
  auto make_raw(std::size_t n) -> T*
  {
    const std::size_t bytes = n * sizeof(T);
    void*             ptr   = m_storage->buff.data() + m_current;
    std::size_t       space = aligned_size - m_current;
    void*             aligned = std::align(alignof(T), bytes, ptr, space);
    if (not aligned) return nullptr;
    auto* next = static_cast<std::byte*>(aligned) + bytes;
    if (next > m_storage->buff.data() + aligned_size) return nullptr;
    m_current = static_cast<std::size_t>(next - m_storage->buff.data());
    return static_cast<T*>(aligned);
  }

  void for_each(auto&& func) const {
    auto* ptr = m_storage->buff.data();
    auto* end = m_storage->buff.data() + m_current;
    while (ptr < end) { func(*reinterpret_cast<T*>(ptr)); ptr += sizeof(T); }
  }

  void for_range(std::size_t start, std::size_t end, auto&& func) {
    auto* ptr  = m_storage->buff.data() + start;
    auto* stop = m_storage->buff.data() + end;
    while (ptr < stop) { func(reinterpret_cast<T*>(ptr)); ptr += sizeof(T); }
  }

  void reset() {
    for_each([](T& obj) { std::destroy_at(&obj); });
    m_current = 0;
  }

private:
  static constexpr auto buffer_size  = 8 * 1024 * 1024;
  static constexpr auto aligned_size =
  ((buffer_size + alignof(T) - 1) / alignof(T)) * alignof(T);

  struct alignas(alignof(T)) aligned_storage {
    std::array<std::byte, buffer_size> buff;
  };
  std::unique_ptr<aligned_storage> m_storage;
  std::size_t m_current{};
};

template <typename T>
struct slab_allocator
{
  using value_type      = T;
  using size_type       = std::size_t;
  using difference_type = std::ptrdiff_t;
  
  using propagate_on_container_copy_assignment = std::true_type;
  using propagate_on_container_move_assignment = std::true_type;
  using propagate_on_container_swap            = std::true_type;
  
  template <typename U>
  using rebind = slab_allocator<U>;

  slab_allocator() : m_pool(std::make_shared<pool_t>())
  {
    m_pool->emplace_back();
  }

  template <typename U>
  explicit slab_allocator(const slab_allocator<U>& other) noexcept
  : m_pool(other.m_pool_ptr()) {}

  slab_allocator(const slab_allocator&) noexcept = default;
  slab_allocator(slab_allocator&&)      noexcept = default;
  slab_allocator& operator=(const slab_allocator&) noexcept = default;
  slab_allocator& operator=(slab_allocator&&)      noexcept = default;

  [[nodiscard]] auto allocate(std::size_t n) -> T*
  {
    if (auto* ptr = current_slab().make_raw(n)) return ptr;
    m_pool->emplace_back();
    if (auto* ptr = current_slab().make_raw(n)) return ptr;
    return static_cast<T*>(::operator new(n * sizeof(T), std::align_val_t{alignof(T)})); //NOTE: fallback
  }

  template <typename U>
  [[nodiscard]] auto allocate_as(std::size_t n = 1) -> U* {
    if (auto* ptr = current_slab().template allocate_bytes<U>(n)) return ptr;
    m_pool->emplace_back();
    return current_slab().template allocate_bytes<U>(n);
  }
  void deallocate(T* ptr, std::size_t size) noexcept { }

  template <typename U>
  auto operator==(const slab_allocator<U>& rhs) const noexcept -> bool { return m_pool.get() == rhs.m_pool_ptr().get(); }
  template <typename U>
  auto operator!=(const slab_allocator<U>& rhs) const noexcept -> bool { return !(*this == rhs); }

  void reset()
  {
    for (auto& slab : *m_pool) slab.reset();
    m_pool->resize(1);
  }

  auto m_pool_ptr() const noexcept { return m_pool; }

  void construct(auto* p, auto&&... args) { std::construct_at(p, std::forward<decltype(args)>(args)...); }
  void destroy(auto* p) noexcept { std::destroy_at(p); }

private:
  using pool_t = std::list<arena<T>>;

  auto current_slab() -> arena<T>& { return m_pool->back(); }

  std::shared_ptr<pool_t> m_pool;
};

} // nalay
