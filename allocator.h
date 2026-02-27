#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stddef.h>

#define ALIGN8(x) (((x) + 7) & ~7)

// rm stress#define FIRST_FIT
#define BEST_FIT

#define MAGIC_ALLOC 0x12345678
#define MAGIC_FREE  0xDEADBEEF

typedef struct block {
    size_t size;
    int free;
    unsigned int magic;
    struct block *next;
} block_t;

void *my_malloc(size_t size);
void my_free(void *ptr);
void *my_calloc(size_t nmemb, size_t size);
void *my_realloc(void *ptr, size_t size);
void print_heap_stats(void);

#endif