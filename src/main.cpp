#include "Crawler.hpp"
#include "arg_parser/ArgParser.hpp"
#include "print_output/PrintOutput.hpp"
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <linux/stat.h>
#include <sys/stat.h>
namespace fs = std::filesystem;

int main(int argc, char *argv[]) {

  Args config;
  const bool ok = ArgParser::parse_args(argc, argv, config);
  if (!ok) {
    return 1;
  }
  if (config.paths.empty()) {
    config.paths.emplace_back(".");
  }

  for (auto &start_path : config.paths) {
    std::cout << "\n";
    fs::path const resolved_path = fs::absolute(start_path).lexically_normal();
    // cyan
    std::cout << "\033[36m" << resolved_path << "\033[0m" << "\n";

    struct statx base_stat;
    const int ret = statx(AT_FDCWD, start_path.c_str(),
                          AT_STATX_DONT_SYNC | AT_SYMLINK_NOFOLLOW,
                          STATX_BLOCKS | STATX_TYPE, &base_stat);
    if (ret < 0) {
      std::cerr << "Error: " << std::strerror(errno) << "\n";
      return 1;
    }

    if (S_ISDIR(base_stat.stx_mode)) {
      Crawler cw(start_path, &config);
      const auto total_data = cw.run();
      PrintOutput::print(&config, total_data);
    } else {
      PrintOutput::print(&config, {base_stat.stx_blocks, 0, 1});
    }
  }
}
