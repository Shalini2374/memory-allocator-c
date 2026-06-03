#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stddef.h>
#include <pthread.h>

#define ALIGN8(x) (((x) + 7) & ~7)

// Switch between strategies here
#define FIRST_FIT
//#define BEST_FIT

#define MAGIC_ALLOC 0x12345678
#define MAGIC_FREE  0xDEADBEEF

typedef struct block {
    size_t size;
    int free;
    unsigned int magic;
    struct block *next;
} block_t;

/* ── Global allocator API ── */
void *my_malloc(size_t size);
void  my_free(void *ptr);
void *my_calloc(size_t nmemb, size_t size);
void *my_realloc(void *ptr, size_t size);
void  print_heap_stats(void);

/* ── Arena allocator API (per-thread, no contention) ── */
void *my_malloc_arena(block_t **head, pthread_mutex_t *lock, size_t size);
void  my_free_arena(block_t **head, pthread_mutex_t *lock, void *ptr);
void *my_realloc_arena(block_t **head, pthread_mutex_t *lock, void *ptr, size_t size);
void  print_heap_stats_arena(block_t **head, pthread_mutex_t *lock);

#endif