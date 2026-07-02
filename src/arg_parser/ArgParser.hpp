#pragma once

#include "cxxopts/cxxopts.hpp"
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

struct Args {
  std::vector<std::string> paths;
  std::string display_format;
  uint16_t num_threads = 0;
  bool ignore_errors = false;
  bool count_items = false;
};

class ArgParser {
public:
  // returns true if successfully parsed else prints help message
  static bool parse_args(const int argc, char *argv[], Args &args) {
    cxxopts::Options options("idu", "instant du");

    options.add_options()(
        "i,ignore-errors", "Don't display file-system errors",
        cxxopts::value<bool>(args.ignore_errors)->default_value("false"))(
        "f,format",
        "Set the display format [possible values: metric, binary, bytes, kb, "
        "kib, mb, mib, gb, gib, tb, tib]",
        cxxopts::value<std::string>(args.display_format)
            ->default_value("binary"),
        "<FORMAT>")(
        "j,threads",
        "Set the number of threads (0 means total logical processors)",
        cxxopts::value<uint16_t>(args.num_threads)->default_value("0"), "<NUM>")(
        "c,count-items", "Display total count of files and sub-directories",
        cxxopts::value<bool>(args.count_items)->default_value("false"))(
        "paths", "The path(s) to query",
        cxxopts::value<std::vector<std::string>>(args.paths),
        "<PATHS>")("h,help", "Print usage");

    options.positional_help("[PATHS...]");
    options.show_positional_help();
    options.parse_positional("paths");

    try {
      const auto parsed_args = options.parse(argc, argv);
      if (parsed_args.contains("help")) [[unlikely]] {
        std::cout << options.help();
        return false;
      }
      if (parsed_args.contains("format")) {
        if (!check_display_format(args.display_format)) {
          std::cerr << "Error parsing: incorrect format (-f) provided\n\n";
          std::cout << options.help();
          return false;
        }
      }
      return true;
    } catch (const cxxopts::exceptions::exception &e) {
      std::cerr << "Error parsing: " << e.what() << "\n\n";
      std::cout << options.help();
      return false;
    }
  }

private:
  static bool check_display_format(const std::string &s) {
    if (!s.empty()) {
      return s == "bytes" || s == "kb" || s == "kib" || s == "mb" ||
             s == "mib" || s == "gb" || s == "gib" || s == "tb" || s == "tib" ||
             s == "metric" || s == "binary";
    }
    return true;
  }
};
