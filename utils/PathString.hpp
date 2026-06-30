#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <utility>

// a custom lightweight string class designed to reduce heap allocations
// std::string has an sso of size of 15
// but paths easily grow bigger than that so this class uses a bigger capacity

class PathString {
public:
  PathString() noexcept { m_buf[0] = '\0'; }

  PathString(const char *s, const uint32_t n) {
    if (n < SSO_CAP) [[likely]] {
      memcpy(m_buf.data(), s, n);
      m_buf[n] = '\0';
    } else {
      m_heap_ptr = std::make_unique<char[]>(n + 1);
      memcpy(m_heap_ptr.get(), s, n);
      m_heap_ptr[n] = '\0';
    }
    m_len = n;
  }

  PathString(PathString &&o) noexcept
      : m_len(o.m_len), m_heap_ptr(std::move(o.m_heap_ptr)) {
    if (m_heap_ptr == nullptr) {
      memcpy(m_buf.data(), o.m_buf.data(), m_len + 1);
    }
    o.m_len = 0;
  }

  PathString &operator=(PathString &&o) noexcept {
    if (this != &o) {
      m_heap_ptr = std::move(o.m_heap_ptr);
      m_len = o.m_len;
      if (m_heap_ptr == nullptr) {
        memcpy(m_buf.data(), o.m_buf.data(), m_len + 1);
      }
      o.m_len = 0;
    }
    return *this;
  }

  ~PathString() = default;

  // we never want to copy
  PathString(const PathString &) = delete;
  PathString &operator=(const PathString &) = delete;

  [[nodiscard]] [[gnu::always_inline]]
  const char *c_str() const noexcept {
    return (m_heap_ptr != nullptr) ? m_heap_ptr.get() : m_buf.data();
  }
  [[nodiscard]] [[gnu::always_inline]]
  uint32_t length() const noexcept {
    return m_len;
  }
  [[nodiscard]] [[gnu::always_inline]]
  bool empty() const noexcept {
    return m_len == 0;
  }

private:
  static constexpr uint32_t SSO_CAP = 256;
  uint32_t m_len = 0;
  std::unique_ptr<char[]> m_heap_ptr;
  std::array<char, SSO_CAP> m_buf;
};
