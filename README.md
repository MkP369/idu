# idu (instant du)

![License](https://img.shields.io/badge/license-MIT-red.svg?style=flat-square)

A minimal multithreaded ***(fastest?)*** du -sh written in C++20

---

## Benchmarks

*These are all tested on my 22 core x86 laptop*

### Hot Cache

#### Tested on my home directory (~60 GB)

```bash
 hyperfine --warmup 3 -i 'idu' 'diskus' 'du -sh' 'gdu -s -c -p'  'dust -d0 -b -n0 -c -P' 'pdu -d1 --no-sort'
```

#### Results

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| **`idu`** | 66.1 ± 2.6 | 60.7 | 73.0 | 1.00 |
| `diskus` | 148.5 ± 6.9 | 138.8 | 159.7 | 2.25 ± 0.14 |
| `du -sh` | 533.5 ± 23.6 | 505.9 | 571.1 | 8.07 ± 0.48 |
| `gdu -s -c -p` | 192.4 ± 11.6 | 173.0 | 209.4 | 2.91 ± 0.21 |
| `dust -d0 -b -n0 -c -P` | 246.1 ± 17.6 | 224.1 | 270.5 | 3.72 ± 0.30 |
| `pdu -d1 --no-sort` | 76.5 ± 3.0 | 68.9 | 82.0 | 1.16 ± 0.06 |

### Cold Cache

#### Tested on linux kernel source (~5.5 GB)
```bash
hyperfine --prepare 'sync; echo 3 | sudo tee /proc/sys/vm/drop_caches' \
       'idu' 'diskus' 'du -sh' 'gdu -s -c -p'  'dust -d0 -b -n0 -c -P' 'pdu -d1 --no-sort'
```

#### Results

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| **`idu`** | 293.1 ± 7.7 | 280.5 | 302.3 | 1.00 |
| `diskus` | 308.9 ± 8.1 | 290.1 | 320.4 | 1.05 ± 0.04 |
| `du -sh` | 1722.9 ± 110.5 | 1573.0 | 1940.4 | 5.88 ± 0.41 |
| `gdu -s -c -p` | 381.8 ± 8.6 | 364.5 | 389.7 | 1.30 ± 0.05 |
| `dust -d0 -b -n0 -c -P` | 395.1 ± 11.2 | 383.8 | 417.7 | 1.35 ± 0.05 |
| `pdu -d1 --no-sort` | 300.8 ± 12.4 | 275.6 | 315.4 | 1.03 ± 0.05 |

---

## Usage

```bash
# Scan a specific directory
./idu /path/to/scan

# Scan the current directory
./idu
```

**Example Output:**
```bash
67.9 GB
```

---

## Building 

### Build Requirements
* Linux
* GCC 11+ (Must support C++20)
* CMake 3.20+
* [mold](https://github.com/rui314/mold) linker

### Release Profile

```bash
cmake -B build_release -DCMAKE_BUILD_TYPE=Release
cmake --build build_release -j$(nproc)
```

### Debug Profile

```bash
cmake -B build_debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build_debug -j$(nproc)
```

---

## TODO

- [ ] batch statx call
- [ ] unit tests
- [ ] add more display options
- [ ] benchmark clang vs gcc?

---

## Other Awesome Disk Usage Tools!

- [gdu](https://github.com/dundee/gdu) (Go)
- [ncdu](https://code.blicky.net/yorhel/ncdu) (Zig)
- [pdu](https://github.com/KSXGitHub/parallel-disk-usage) (Rust)
- [diskus](https://github.com/sharkdp/diskus) (Rust)
- [dust](https://github.com/bootandy/dust) (Rust)
- [dua](https://github.com/Byron/dua-cli) (Rust)


