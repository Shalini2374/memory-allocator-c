# Thread-Safe Custom Memory Allocator (C)

A high-performance, POSIX-compliant memory allocator implementing `my_malloc`, `my_free`, `my_calloc`, and `my_realloc` — built as a deep dive into low-level memory management and the trade-offs between **First-Fit** and **Best-Fit** allocation strategies in high-concurrency environments.

---

## 🚀 Key Features

**Modern Memory Mapping** — Replaced legacy `sbrk` with `mmap` syscalls for modern, thread-safe kernel-level memory mapping.

**Per-Thread Arenas** — Each thread operates on its own independent heap with its own mutex, eliminating global lock contention and enabling true multi-core scaling.

**Algorithmic Flexibility** — Supports both First-Fit (optimized for speed) and Best-Fit (optimized for memory density) via compile-time macros.

**Memory Safety** — Magic Number Verification (`0x12345678`) detects buffer overflows and catches invalid or double-free errors before heap corruption occurs.

**Leak Verification** — A `print_heap_stats()` utility monitors fragmentation and verifies 100% memory reclamation upon cleanup.

---

## ✅ Test Coverage

11 correctness tests across all public functions — all passing:

| Test | What it verifies |
|------|-----------------|
| Basic malloc | Returns non-NULL, memory is writable and readable |
| Write/read int | Stores and retrieves integer value correctly |
| Write/read string | Stores and retrieves string value correctly |
| Calloc zeroing | All allocated bytes initialised to zero |
| Realloc non-NULL | Returns valid pointer after resize |
| Realloc preserves data | Original value intact after reallocation |
| Double-free detection | Magic number catches invalid free without crash |
| Fragmentation visible | Heap stats show fragmentation mid-test |
| Coalescing cleanup | Used memory returns to 0 after full free |
| my_malloc(0) | Returns NULL safely |
| my_free(NULL) | Handled without crash |

```bash
./main
# Results: 11 passed, 0 failed
```

---

## 📊 Performance Benchmarks

> Tested on WSL (Ubuntu 22.04) | 5,000 operations per test

### Single-Threaded Stress Test

| Strategy | Throughput (ops/sec) | Fragmentation |
|----------|---------------------|---------------|
| First-Fit | 119,642 | 28.87% |
| Best-Fit | 119,403 | 27.67% |

### Multi-Threaded Stress Test (4 Threads, Per-Thread Arenas)

| Strategy | Throughput (ops/sec) | Notes |
|----------|---------------------|-------|
| First-Fit | 641,080 | Faster due to shorter lock hold time |
| Best-Fit | 567,985 | Better memory density, slightly slower |

### Before vs After Per-Thread Arenas

| Mode | Before (global lock) | After (per-thread arenas) | Improvement |
|------|---------------------|--------------------------|-------------|
| 4-thread | 13,171 ops/sec | 641,080 ops/sec | **48x** |

---

## 🔍 Engineering Analysis

**Per-Thread Arenas eliminate contention** — Replaced the single global lock with per-thread arenas, each with its own heap and mutex. Multi-threaded throughput jumped from 13k to 641k ops/sec (48x improvement), matching the architectural approach used by `jemalloc` and `tcmalloc`.

**First-Fit wins on multi-threaded throughput** — 641k vs 567k ops/sec because it terminates search at the first suitable block, holding the mutex for less time and reducing contention across threads.

**Best-Fit wins on memory density** — Marginally lower fragmentation (27.67% vs 28.87%) due to more precise block selection, making it the better choice when memory efficiency outweighs raw speed.

**Zero leaks under concurrency** — All 4 threads independently clean up their arenas. Each arena reports 0 bytes used after completion across 20,000 total concurrent operations.

---

## 🛠️ Usage

### Compilation

Build all test executables using the provided Makefile:

```bash
make
```

To switch allocation strategy, edit `allocator.h`:

```c
#define FIRST_FIT   // faster multi-threaded throughput
//#define BEST_FIT  // lower fragmentation
```

Then rebuild:

```bash
make clean && make
```

### Running the Test Suite

| Command | Purpose |
|---------|---------|
| `./main` | 11 PASS/FAIL correctness tests across all public functions |
| `./stress` | Single-threaded throughput and fragmentation benchmark |
| `./multi` | 4-thread concurrency test with per-thread arenas |
## 📂 Project Structure
.
├── allocator.c             # Core engine: malloc, free, calloc, realloc, splitting, coalescing, global + arena APIs
├── allocator.h             # block_t metadata, strategy macros, function declarations
├── main.c                  # 11 PASS/FAIL correctness tests
├── stress_test.c           # Single-threaded performance benchmark
├── multi_thread_test.c     # 4-thread concurrency test with per-thread arenas
└── Makefile                # Build all executables: main, stress, multi
---

## 🔮 Future Improvements

1. **Slab Allocation** — Request large pages (4KB+) from the OS to amortize `mmap` syscall overhead across many small allocations.
2. **Lock-Free Allocation** — Replace mutex locks with atomic CAS operations for even lower contention overhead.
3. **System malloc comparison** — Benchmark against `glibc malloc` to quantify real-world performance gaps. 
---

