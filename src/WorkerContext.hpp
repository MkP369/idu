#pragma once
#include "../utils/PathString.hpp"
#include "../utils/SpinLock.hpp"
#include <array>
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
  static constexpr uint32_t GETDENTS_BUF_SZ =
      256 * 1024; // 256 kb to fit in L2 Cache

  SpinLock sl;
  std::vector<PathString> local_dir_queue;

  // aligned because another thread can invalidate
  // total_blocks cache for this thread while stealing
  // (accessing both spinlock and local_dir_queue)
  alignas(std::hardware_destructive_interference_size) uint64_t total_blocks =
      0;
  // aligned to the page size to make reads fast
  alignas(4096) std::array<uint8_t, GETDENTS_BUF_SZ> dents_buf;

  // A stack-allocated buffer used to construct full child paths
  // linux path names length have a max limit of 4096 chars
  // else the syscalls give long name errors
  std::array<char, 4096>
      path_buf; // for storing current file paths to be put into queue

  WorkerContext() { local_dir_queue.reserve(1024); }

  [[gnu::always_inline]]
  bool try_take(PathString &out) {
    sl.lock();
    if (!local_dir_queue.empty()) {
      out = std::move(local_dir_queue.back());
      local_dir_queue.pop_back();
      sl.unlock();
      return true;
    }
    sl.unlock();
    return false;
  }

  [[gnu::always_inline]]
  void push_back(PathString &dir) {
    sl.lock();
    local_dir_queue.push_back(std::move(dir));
    sl.unlock();
  }

  [[gnu::always_inline]]
  void emplace_back(const char *path, uint32_t len) {
    sl.lock();
    local_dir_queue.emplace_back(path, len);
    sl.unlock();
  }
};
