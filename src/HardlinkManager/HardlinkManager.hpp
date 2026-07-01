#pragma once

#include "../../utils/SpinLock.hpp"
#include "../../utils/third_party/emhash/emhash_table6.hpp"
#include "/home/mkp/dev/cpp-projects/idu/utils/third_party/emhash/config.hpp"
#include <cstddef>
#include <cstdint>
#include <new>

// uses
// https://github.com/ktprime/emhash/blob/master/include/emhash/hash_table6.hpp
// for unordered_set

struct HardlinkKey {
  uint64_t dev;
  uint64_t ino;
  bool operator==(const HardlinkKey &o) const {
    return dev == o.dev && ino == o.ino;
  }
};

struct HardlinkHash {
  size_t operator()(const HardlinkKey &k) const {
    return static_cast<size_t>(emh_wyhash64(k.dev, k.ino));
  }
};

class HardLinkManager {
public:
  HardLinkManager() : m_hardlink_set(64) {};

  // returns true if successfully inserted
  bool insert(uint32_t dev_major, uint32_t dev_minor, uint64_t ino) {
    uint64_t dev = (static_cast<uint64_t>(dev_major) << 32) | dev_minor;
    HardlinkKey key{.dev = dev, .ino = ino};

    s.lock();
    bool not_exists = m_hardlink_set.insert({key, 1}).second;
    s.unlock();

    return not_exists;
  }

private:
  alignas(std::hardware_destructive_interference_size) SpinLock s;
  emhash6::HashMap<HardlinkKey, uint32_t, HardlinkHash> m_hardlink_set;
};
