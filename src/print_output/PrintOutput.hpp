#pragma once

#include "arg_parser/ArgParser.hpp"
#include <array>
#include <cstdint>
#include <iomanip>
#include <iostream>

// a block is 512 bytes
class PrintOutput {
public:
  static void print(const Args *config, const std::array<uint64_t, 3> &data) {

    std::cout << std::fixed << std::setprecision(2);

    if (config->display_format == "" || config->display_format == "binary")
        [[likely]] {
      print_human_readable(data[0], 1024);
    } else if (config->display_format == "metric") {
      print_human_readable(data[0], 1000);
    } else if (config->display_format == "bytes") {
      std::cout << (data[0] * 512) << " bytes";
    } else if (config->display_format == "kib") {
      std::cout << data[0] / 2.0 << " KiB";
    } else if (config->display_format == "kb") {
      std::cout << (data[0] * 512) / 1000.0 << " KB";
    } else if (config->display_format == "mib") {
      std::cout << data[0] / (2.0 * 1024) << " MiB";
    } else if (config->display_format == "mb") {
      std::cout << data[0] * 512 / (1000.0 * 1000) << " MB";
    } else if (config->display_format == "gib") {
      std::cout << data[0] / (2.0 * 1024 * 1024) << " GiB";
    } else if (config->display_format == "gb") {
      std::cout << data[0] * 512 / (1000.0 * 1000 * 1000) << " GB";
    } else if (config->display_format == "tib") {
      std::cout << data[0] / (2.0 * 1024 * 1024 * 1024) << " TiB";
    } else if (config->display_format == "tb") {
      std::cout << data[0] * 512 / (1000.0 * 1000 * 1000 * 1000) << " TB";
    }
    std::cout << "\n";

    if (config->count_items) {
      std::cout << "Total sub-directories: " << data[1] << "\n";
      std::cout << "Total files: " << data[2] << "\n";
    }
  }

private:
  static void print_human_readable(const uint64_t total_blocks,
                                   const uint64_t base) {

    const double bytes = total_blocks * 512;
    if (bytes < base) {
      std::cout << bytes << " bytes";
    } else if (bytes < base * base) {
      std::cout << bytes / base;
      if (base == 1024) [[likely]] {
        std::cout << " KiB";
      } else {
        std::cout << " KB";
      }
    } else if (bytes < base * base * base) {
      std::cout << bytes / (base * base);
      if (base == 1024) [[likely]] {
        std::cout << " MiB";
      } else {
        std::cout << " MB";
      }
    } else if (bytes < base * base * base * base) {
      std::cout << bytes / (base * base * base);
      if (base == 1024) [[likely]] {
        std::cout << " GiB";
      } else {
        std::cout << " GB";
      }
    } else {
      std::cout << bytes / (base * base * base * base);
      if (base == 1024) [[likely]] {
        std::cout << " TiB";
      } else {
        std::cout << " TB";
      }
    }
  }
};
