#pragma once

#include "WorkerContext.hpp"
#include "directory_scanner/DirectoryScanner.hpp"
#include "hardlink_manager/HardlinkManager.hpp"
#include "utils/PathString.hpp"
#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <new>
#include <string>
#include <thread>
#include <utility>
#include <vector>

class Crawler {
public:
  Crawler(std::string start_path)
      : m_start_path(std::move(start_path)), m_workers(m_num_threads) {}

  uint64_t run() {
    seed_bfs();

    // to check if already completed in the seeding phase only
    bool initally_active = false;
    for (auto &ctx : m_workers) {
      if (!ctx.local_dir_queue.empty()) [[likely]] {
        initally_active = true;
        break;
      }
    }
    if (!initally_active) [[unlikely]] {
      return m_workers[0].total_blocks;
    }

    // we keep all the threads awake at the starting to prevent futex calls
    // because high chance that some work will come
    m_active_workers.store(m_num_threads, std::memory_order_relaxed);

    // Spin up the threads.
    std::vector<std::thread> threads(m_num_threads);
    for (uint32_t i = 0; i < m_num_threads; i++) {
      threads[i] = std::thread(&Crawler::worker_func, this, i);
    }

    // calc the total
    uint64_t total = 0;
    for (uint32_t i = 0; i < m_num_threads; ++i) {
      threads[i].join();
      total += m_workers[i].total_blocks;
    }
    return total;
  }

private:
  const uint32_t m_num_threads = (std::thread::hardware_concurrency() == 0)
                                     ? 8
                                     : std::thread::hardware_concurrency();
  const std::string m_start_path;
  std::vector<WorkerContext> m_workers;
  alignas(std::hardware_destructive_interference_size)
      std::atomic<uint32_t> m_active_workers = 0;
  alignas(std::hardware_destructive_interference_size)
      std::atomic<bool> m_done = false;
  alignas(std::hardware_destructive_interference_size)
      std::atomic<uint32_t> m_work_counter = 0;
  HardLinkManager m_hardlink_tracker;

  // seeding is done to prevent threads from going waking and sleeping abruptly
  // at the beginning by giving them some accumulated work to keep the alive
  // until other work comes
  void seed_bfs() {

    // if just 1 thread just put the start path as work and exit
    if (m_num_threads == 1) [[unlikely]] {
      m_workers[0].emplace_back(m_start_path.data(), m_start_path.length());
      return;
    }

    const uint32_t seed_target =
        m_num_threads * 8; // 8 threads * 8 = 64 initial dirs

    // reuse workers_[0] as the seed-phase context
    auto &seed_ctx = m_workers[0];
    seed_ctx.emplace_back(m_start_path.data(), m_start_path.length());

    uint32_t head = 0;
    // keep processing until we hit the seed_target or run out of directories.
    while (head < seed_ctx.local_dir_queue.size() &&
           seed_ctx.local_dir_queue.size() < seed_target) {
      const auto dir = std::move(seed_ctx.local_dir_queue[head++]);
      uint32_t pushed_dirs = 0;
      DirectoryScanner::process_dir(dir, seed_ctx, pushed_dirs,
                                    &m_hardlink_tracker);
    }

    // once the BFS is done, distribute the work evenly
    size_t write_index_0 = 0;
    auto i = head;
    while (i < seed_ctx.local_dir_queue.size()) {

      // it is for worker 0 to move the work to its spot
      seed_ctx.local_dir_queue[write_index_0++] =
          std::move(seed_ctx.local_dir_queue[i++]);

      // for other workers move it to their queue
      for (uint32_t j = 1; j < m_num_threads; j++) {
        if (i < seed_ctx.local_dir_queue.size()) [[likely]] {
          m_workers[j].push_back(seed_ctx.local_dir_queue[i++]);
        } else [[unlikely]] {
          break;
        }
      }
    }
    seed_ctx.local_dir_queue.resize(write_index_0);
  }

  bool try_steal(uint32_t worker_ind, PathString &out) {

    uint32_t c = m_num_threads - 1;
    uint32_t index = worker_ind + 1; // to prevent thudering herd
    while (c--) {
      if (index == m_num_threads) [[unlikely]] {
        index = 0;
      }
      if (m_workers[index].try_take(out)) {
        return true;
      }
      index++;
    }
    return false;
  }

  bool get_next_work(uint32_t id, PathString &out_dir) {
    auto &ctx = m_workers[id];
    while (true) {
      // take snapshot of work counter
      // this prevents missed wakeups if work arrives or the crawl completes
      // while we are inspecting the queues
      const uint32_t snap = m_work_counter.load(std::memory_order_acquire);

      // first, try to take from our own queue
      bool found = ctx.try_take(out_dir);

      // else try from other worker queues
      if (__builtin_expect(static_cast<long>(!found), 0) != 0) {
        found = try_steal(id, out_dir);
      }

      // if still no work
      if (!found) [[unlikely]] {
        // check if new work arrived in between
        if (snap != m_work_counter.load(std::memory_order_seq_cst)) {
          continue;
        }
        // decrement the global awake worker count.
        // If we are the last awake worker (1->0), the entire crawl is
        // finished
        if (m_active_workers.fetch_sub(1, std::memory_order_seq_cst) == 1) {
          m_done.store(true, std::memory_order_release);
          m_work_counter.fetch_add(1, std::memory_order_release);
          m_work_counter.notify_all(); // wake up all sleeping workers so that
                                       // they can exit
          return false;
        }

        // the crawl isn't done yet so we must sleep and wait for work
        if (!m_done.load(std::memory_order_acquire)) [[likely]] {

          // comparing with snap prevents race conditions when work might
          // arrive between load and wait
          m_work_counter.wait(snap, std::memory_order_relaxed);

          // other thread woke us up, if crawl is not done yet incr the active
          // worker counter
          if (!m_done.load(std::memory_order_acquire)) [[likely]] {
            m_active_workers.fetch_add(1, std::memory_order_acq_rel);
            continue; // try to get work again
          } else [[unlikely]] {
            return false; // crawling done
          }
        } else [[unlikely]] {
          return false; // crawling done
        }
      }

      return true;
    }
  }

  void worker_func(const uint32_t id) {
    auto &ctx = m_workers[id];

    // loop until crawling is done
    PathString dir_path;
    while (!m_done.load(std::memory_order_relaxed)) {
      if (!get_next_work(id, dir_path)) [[unlikely]] {
        break; // crawling done
      }

      uint32_t pushed_dirs = 0;
      DirectoryScanner::process_dir(dir_path, ctx, pushed_dirs,
                                    &m_hardlink_tracker);

      // wake up sleeping theads
      if (pushed_dirs > 0) [[likely]] {
        // using seq_cst to ensure this increment is globally visible before we
        // check active_workers else a thread could sleep forever
        m_work_counter.fetch_add(1, std::memory_order_seq_cst);

        const uint32_t awake = m_active_workers.load(std::memory_order_seq_cst);
        const uint32_t sleeping = m_num_threads - awake;

        if (sleeping > 0) {
          uint32_t to_wake = std::min(pushed_dirs, sleeping);
          while (to_wake--) {
            m_work_counter.notify_one();
          }
        }
      }
    }
  }
};
