#include "Crawler.hpp"
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <linux/stat.h>
#include <sys/stat.h>
namespace fs = std::filesystem;

// 1 block = 512 bytes
void print_size(const uint64_t total_blocks) noexcept {
  double size_in_kb = (total_blocks + 1) / 2.0; // to KB
  std::cout << std::fixed << std::setprecision(1);
  if (size_in_kb < 1024) {
    std::cout << size_in_kb << " KB";
  } else if (size_in_kb < 1024.0 * 1024) {
    std::cout << (size_in_kb / 1024.0) << " MB";
  } else if (size_in_kb < 1024.0 * 1024 * 1024) {
    std::cout << (size_in_kb / (1024.0 * 1024.0)) << " GB";
  } else {
    std::cout << (size_in_kb / (1024.0 * 1024.0 * 1024.0)) << " TB";
  }
}

int main(int argc, char *argv[]) {

  // if no path is provided, take . dir
  fs::path start_path(fs::current_path());
  if (argc == 2) {
    start_path = argv[1];
  } else if (argc > 2) {
    std::cerr << "Usage: " << argv[0] << " " << "<path>\n";
    return 1;
  }

  struct statx base_stat;
  int ret = statx(AT_FDCWD, start_path.c_str(),
                  AT_STATX_DONT_SYNC | AT_SYMLINK_NOFOLLOW,
                  STATX_BLOCKS | STATX_TYPE, &base_stat);
  if (ret < 0) {
    std::cerr << "statx error " << std::strerror(errno) << "\n";
    return 1;
  }

  if (S_ISDIR(base_stat.stx_mode)) {
    Crawler cw(start_path);
    uint64_t dir_blocks = cw.run();
    uint64_t total_blocks = dir_blocks + base_stat.stx_blocks;
    print_size(total_blocks);
  } else {
    print_size(base_stat.stx_blocks);
  }
}
