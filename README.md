# Thread-Safe Custom Memory Allocator (C)

A high-performance, POSIX-compliant memory allocator implementing `my_malloc`, `my_free`, `my_calloc`, and `my_realloc` — built as a deep dive into low-level memory management and the trade-offs between **First-Fit** and **Best-Fit** allocation strategies in high-concurrency environments.

---

## 🚀 Key Features

**Modern Memory Mapping** — Replaced legacy `sbrk` with `mmap` syscalls for modern, thread-safe kernel-level memory mapping.

**Concurrency Control** — Implemented `pthread_mutex` synchronization to ensure heap integrity across multi-threaded workloads.

**Algorithmic Flexibility** — Supports both First-Fit (optimized for speed) and Best-Fit (optimized for memory density) via compile-time macros.

**Memory Safety** — Magic Number Verification (`0x12345678`) detects buffer overflows and catches invalid or double-free errors.

**Leak Verification** — A `print_heap_stats()` utility monitors fragmentation and verifies 100% memory reclamation upon cleanup.

---

## 📊 Performance Benchmarks

> Tested on WSL (Ubuntu 22.04) | 5,000 operations per test

### Single-Threaded Stress Test

| Strategy   | Throughput (ops/sec) | Fragmentation |
|------------|----------------------|---------------|
| First-Fit  | 66,581.74            | 28.53%        |
| Best-Fit   | 69,439.44            | 27.67%        |

### Multi-Threaded Stress Test (4 Threads)

| Strategy   | Throughput (ops/sec) | Fragmentation |
|------------|----------------------|---------------|
| First-Fit  | 13,160.31            | 6.78%         |
| Best-Fit   | 10,981.15            | 4.32%         |

---

## 🔍 Engineering Analysis

**The Contention Bottleneck** — The drop from ~69k to ~13k ops/sec in multi-threaded mode is a direct consequence of lock contention. Threads spend more time waiting for the global `heap_lock` mutex than performing actual allocations.

**First-Fit wins on throughput** — In multi-threaded workloads, First-Fit is ~20% faster because it terminates its search earlier, minimizing the time the mutex is held and reducing contention across threads.

**Best-Fit wins on memory density** — Best-Fit consistently achieves lower fragmentation (4.32% vs 6.78% under concurrency), making it the superior choice when memory efficiency outweighs raw speed.

**Integrity** — The allocator successfully handled 20,000+ random concurrent operations with zero corruption and zero memory leaks.

---

## 🛠️ Usage

### Compilation

Build all test executables using the provided Makefile:

```bash
make
```

### Running the Test Suite

| Command     | Purpose                                                         |
|-------------|-----------------------------------------------------------------|
| `./main`    | Verifies basic logic, double-free detection, and coalescing.   |
| `./stress`  | Measures throughput and fragmentation under single-thread load. |
| `./multi`   | Evaluates performance under 4-thread concurrency.              |

---

## 📂 Project Structure

```
.
├── allocator.c          # Core engine: splitting, coalescing, and search logic
├── allocator.h          # block_t metadata structures and strategy macros
├── main.c               # Unit tests and logic verification
├── stress_test.c        # High-load single-threaded performance testing
├── multi_thread_test.c  # Concurrency and synchronization stress testing
└── Makefile
```

---

## 🔮 Future Improvements

To evolve this from a prototype toward a production-grade allocator:

1. **Slab Allocation** — Request large pages (4KB+) from the OS to amortize `mmap` syscall overhead across many small allocations.
2. **Per-Thread Arenas** — Implement thread-local heaps to eliminate the global lock bottleneck entirely, enabling true multi-core scaling without contention.
