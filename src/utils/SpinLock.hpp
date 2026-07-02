#pragma once

#include <atomic>

class SpinLock {
public:
  // TTAS lock
  [[gnu::always_inline]]
  void lock() noexcept {
    while (true) {
      if (!m_spinlock.test_and_set(std::memory_order_acquire)) [[likely]] {
        return;
      }
      while (m_spinlock.test(std::memory_order_relaxed)) [[likely]] {
#if defined(__x86_64__) || defined(__i386__)
        __builtin_ia32_pause();
#elif defined(__aarch64__)
        asm volatile("yield" ::: "memory");
#endif
      }
    }
  }

  [[gnu::always_inline]]
  void unlock() noexcept {
    m_spinlock.clear(std::memory_order_release);
  }

private:
  std::atomic_flag m_spinlock = ATOMIC_FLAG_INIT;
};
