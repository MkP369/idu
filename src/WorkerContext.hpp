#include "utils/PathString.hpp"
#include <array>
#include <atomic>
#include <cstdint>
#include <new>
#include <utility>
#include <vector>

struct linux_dirent64 {
  uint64_t d_ino;
  uint64_t d_off;
  unsigned short d_reclen;
  unsigned char d_type;
  char d_name[];
};

class WorkerContext {
public:
  WorkerContext()
      : m_local_dir_queue(1024) {} // preallocate to prevent
                                   // resizing

  [[gnu::always_inline]]
  bool try_take(PathString &out) {
    lock();
    if (!m_local_dir_queue.empty()) {
      out = std::move(m_local_dir_queue.back());
      m_local_dir_queue.pop_back();
      unlock();
      return true;
    }
    unlock();
    return false;
  }

  [[gnu::always_inline]]
  void push_back(PathString &dir) {
    lock();
    m_local_dir_queue.push_back(std::move(dir));
    unlock();
  }

  [[gnu::always_inline]]
  void emplace_back(const char *path, uint32_t len) {
    lock();
    m_local_dir_queue.emplace_back(path, len);
    unlock();
  }

private:
  static const uint32_t GETDENTS_BUF_SZ =
      256 * 1024; // 256 kb to fit in L2 Cache

  std::atomic_flag m_spinlock = ATOMIC_FLAG_INIT;
  std::vector<PathString> m_local_dir_queue;

  // aligned because another thread can invalidate
  // total_blocks cache for this thread while stealing
  // (accessing both spinlock and local_dir_queue)
  alignas(std::hardware_destructive_interference_size) uint64_t m_total_blocks =
      0;

  // aligned to the page size to make reads fast
  alignas(4096) std::array<uint8_t, GETDENTS_BUF_SZ> m_buf;

  [[gnu::always_inline]]
  void lock() noexcept {
    while (!m_spinlock.test_and_set(std::memory_order_acquire)) [[likely]] {
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
};
