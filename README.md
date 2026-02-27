Thread-Safe Custom Memory Allocator (C)

A high-performance, POSIX-compliant memory allocator implementing my_malloc, my_free, my_calloc, and my_realloc. This project serves as a deep dive into low-level memory management and the trade-offs between First-Fit and Best-Fit strategies in high-concurrency environments.
🚀 Key Features

    Modern Memory Mapping: Replaced legacy sbrk with mmap syscalls for modern, thread-safe kernel-level memory mapping.

    Concurrency Control: Implemented pthread_mutex synchronization to ensure heap integrity across multi-threaded workloads.

    Algorithmic Flexibility: Supports both First-Fit (optimized for speed) and Best-Fit (optimized for memory density) via compile-time macros.

    Memory Safety: Utilizes Magic Number Verification (0x12345678) to detect buffer overflows and catch invalid/double-free errors.

    Leak Verification: Features a print_heap_stats() utility to monitor fragmentation and ensure 100% memory reclamation upon cleanup.

📊 Performance Benchmarks

Tested on WSL (Ubuntu 22.04) | 5,000 Operations per Test
Single-Threaded Stress Test
Strategy	Throughput (ops/sec)	Fragmentation
First-Fit	66,581.74	28.53%
Best-Fit	69,439.44	27.67%
Multi-Threaded Stress Test (4 Threads)
Strategy	Throughput (ops/sec)	Fragmentation
First-Fit	13,160.31	6.78%
Best-Fit	10,981.15	4.32%
🔍 Engineering Analysis

    The Contention Bottleneck: As evidenced by the drop from 69k to 13k ops/sec, the system encounters Lock Contention. In multi-threaded mode, the threads spend more time waiting for the global mutex than performing allocations.

    Trade-off Insights: * First-Fit emerged as the winner for multi-threaded performance (20% faster) because it minimizes the duration the heap_lock is held.

        Best-Fit proved superior for memory density, achieving a remarkably low fragmentation of 4.32%.

    Integrity: The system successfully handled 20,000+ random concurrent operations with zero corruption and zero leaks.

🛠️ Usage
Compilation

Build all test executables using the provided Makefile:
Bash

make

Running the Suite
Command	Purpose
./main	Verifies basic logic, double-free detection, and coalescing.
./stress	Measures throughput and fragmentation under single-thread load.
./multi	Evaluates performance under 4-thread concurrency.
📂 Project Structure

    allocator.c: Core engine implementing splitting, coalescing, and search logic.

    allocator.h: Header containing block_t metadata structures and strategy macros.

    main.c: Unit tests and logic verification.

    stress_test.c: High-load performance testing.

    multi_thread_test.c: Concurrency and synchronization stress testing.

📝 Future Improvements

To move this from a prototype to a production-grade kernel-mode driver, future iterations would include:

    Slab Allocation: Requesting large pages (4KB) from the OS to reduce mmap syscall overhead.

    Per-Thread Arenas: Implementing local heaps to eliminate the Global Lock bottleneck and improve multi-core scaling.
