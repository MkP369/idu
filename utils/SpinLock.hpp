#pragma once

#include <atomic>

class SpinLock {
public:
  [[gnu::always_inline]]
  void lock() noexcept {
    while (m_spinlock.test_and_set(std::memory_order_acquire)) [[likely]] {
      // If the lock is held by someone else, we must yield the CPU to prevent
      // burning cycles.
#if defined(__x86_64__) || defined(__i386__)
      __builtin_ia32_pause();
#elif defined(__aarch64__)
      asm volatile("yield" ::: "memory");
#endif
    }
  }

  [[gnu::always_inline]]
  void unlock() noexcept {
    m_spinlock.clear(std::memory_order_release);
  }

private:
  std::atomic_flag m_spinlock = ATOMIC_FLAG_INIT;
};
