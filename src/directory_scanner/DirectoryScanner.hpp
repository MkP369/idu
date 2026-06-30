#pragma once

#include "../../utils/PathString.hpp"
#include "../WorkerContext.hpp"
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <linux/stat.h>
#include <sys/syscall.h>
#include <system_error>
#include <unistd.h>

// scans a single dir
class DirectoryScanner {

public:
  // returns total files processed, pushes new dirs into ctx and
  // updates pushed_dirs
  static uint32_t process_dir(const PathString &dir_path, WorkerContext &ctx,
                              uint32_t &pushed_dirs) {
    pushed_dirs = 0;

    const long fd = syscall(SYS_openat, AT_FDCWD, dir_path.c_str(),
                            O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) [[unlikely]] { // skip
      print_enoent_error();
      return 0;
    }

    const uint32_t dir_len = dir_path.length();
    memcpy(ctx.path_buf.data(), dir_path.c_str(), dir_len);
    uint32_t parent_len = dir_len;

    // Ensure the parent path ends with a trailing slash so we can append child
    // names cleanly.
    if (ctx.path_buf[parent_len - 1] != '/') [[likely]] {
      ctx.path_buf[parent_len++] = '/';
    }

    uint32_t total_files = 0;
    uint64_t local_blocks = 0;

    // Main loop reading directory entries.
    while (true) {
      const long nread = syscall(SYS_getdents64, fd, ctx.dents_buf.data(),
                                 WorkerContext::GETDENTS_BUF_SZ);
      if (nread == 0) [[likely]] {
        break; // finished all the entries
      } else if (nread < 0) [[unlikely]] {
        print_enoent_error();
        break;
      }
      for (long pos = 0; pos < nread;) {
        const auto *entry = reinterpret_cast<const linux_dirent64 *>(
            ctx.dents_buf.data() + pos);
        const long next_pos = pos + entry->d_reclen;

        // check for . and ..
        // since the linux dirent struct needs to be padded, it will always have
        // atleast 3 bytes for d_name
        if (entry->d_name[0] == '.' &&
            (entry->d_name[1] == '\0' ||
             (entry->d_name[1] == '.' && entry->d_name[2] == '\0')))
            [[unlikely]] {
          pos = next_pos;
          continue;
        }

        // d_type == 4 is DT_DIR
        if (entry->d_type == 4) {
          const size_t nlen = strlen(entry->d_name);
          memcpy(ctx.path_buf.data() + parent_len, entry->d_name,
                 nlen + 1); // Build the absolute path.
          ctx.emplace_back(ctx.path_buf.data(), parent_len + nlen);
          pushed_dirs++;
        } else { // if file, symlink, device node, etc., we stat it
                 // immediately
          struct statx stx;
          if (syscall(SYS_statx, fd, entry->d_name,
                      AT_STATX_DONT_SYNC | AT_SYMLINK_NOFOLLOW |
                          AT_NO_AUTOMOUNT,
                      STATX_BLOCKS, &stx) == 0) [[likely]] {
            local_blocks += stx.stx_blocks;
          }
        }
        pos = next_pos; // Move to the next dirent entry.
        total_files++;
      }
    }

    syscall(SYS_close, fd);
    ctx.total_blocks += local_blocks;
    return total_files;
  }

private:
  static void print_enoent_error() {
    std::error_code ec(errno, std::system_category());
    std::cout << "Error: " << ec.message() << '\n';
  }
};
