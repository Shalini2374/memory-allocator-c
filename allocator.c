#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
#include "allocator.h"

static block_t *heap_head = NULL;
static pthread_mutex_t heap_lock = PTHREAD_MUTEX_INITIALIZER;

/* ===================== Helpers ===================== */

static block_t *find_free_block(size_t size) {
#ifdef FIRST_FIT
    block_t *curr = heap_head;
    while (curr) {
        if (curr->free && curr->size >= size)
            return curr;
        curr = curr->next;
    }
    return NULL;
#endif
#ifdef BEST_FIT
    block_t *curr = heap_head;
    block_t *best = NULL;
    while (curr) {
        if (curr->free && curr->size >= size) {
            if (!best || curr->size < best->size)
                best = curr;
        }
        curr = curr->next;
    }
    return best;
#endif
}

static block_t *request_space(size_t size) {
    size_t total_size = sizeof(block_t) + size;
    void *request = mmap(NULL, total_size, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (request == MAP_FAILED)
        return NULL;

    block_t *block = (block_t *)request;
    block->size  = size;
    block->free  = 0;
    block->magic = MAGIC_ALLOC;
    block->next  = NULL;
    return block;
}

static void split_block(block_t *block, size_t size) {
    if (block->size <= size + sizeof(block_t))
        return;

    block_t *new_block = (block_t *)((char *)(block + 1) + size);
    new_block->size  = block->size - size - sizeof(block_t);
    new_block->free  = 1;
    new_block->magic = MAGIC_FREE;
    new_block->next  = block->next;

    block->size = size;
    block->next = new_block;
}

static void coalesce_blocks(void) {
    block_t *curr = heap_head;
    while (curr && curr->next) {
        char *curr_end = (char *)(curr + 1) + curr->size;
        if (curr->free &&
            curr->next->free &&
            curr_end == (char *)curr->next) {
            curr->size += sizeof(block_t) + curr->next->size;
            curr->next  = curr->next->next;
            curr->magic = MAGIC_FREE;
        } else {
            curr = curr->next;
        }
    }
}

/* ===================== Public API ===================== */

void *my_malloc(size_t size) {
    if (size == 0)
        return NULL;

    pthread_mutex_lock(&heap_lock);
    size = ALIGN8(size);
    block_t *block;

    if (!heap_head) {
        block = request_space(size);
        if (!block) {
            pthread_mutex_unlock(&heap_lock);
            return NULL;
        }
        heap_head = block;
    } else {
        block = find_free_block(size);
        if (!block) {
            block_t *last = heap_head;
            while (last->next) last = last->next;
            block = request_space(size);
            if (!block) {
                pthread_mutex_unlock(&heap_lock);
                return NULL;
            }
            last->next = block;
        } else {
            split_block(block, size);
            block->free  = 0;
            block->magic = MAGIC_ALLOC;
        }
    }

    pthread_mutex_unlock(&heap_lock);
    return (void *)(block + 1);
}

void my_free(void *ptr) {
    if (!ptr)
        return;

    pthread_mutex_lock(&heap_lock);
    block_t *block = (block_t *)ptr - 1;

    if (block->magic != MAGIC_ALLOC) {
        printf("Error: Invalid or double free detected!\n");
        pthread_mutex_unlock(&heap_lock);
        return;
    }

    block->free  = 1;
    block->magic = MAGIC_FREE;
    coalesce_blocks();
    pthread_mutex_unlock(&heap_lock);
}

void *my_calloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    void *ptr = my_malloc(total);
    if (ptr)
        memset(ptr, 0, total);
    return ptr;
}

void *my_realloc(void *ptr, size_t size) {
    if (!ptr)
        return my_malloc(size);
    if (size == 0) {
        my_free(ptr);
        return NULL;
    }

    pthread_mutex_lock(&heap_lock);
    block_t *block = (block_t *)ptr - 1;

    if (block->magic != MAGIC_ALLOC) {
        printf("Error: Invalid realloc detected!\n");
        pthread_mutex_unlock(&heap_lock);
        return NULL;
    }

    size = ALIGN8(size);
    if (block->size >= size) {
        pthread_mutex_unlock(&heap_lock);
        return ptr;
    }
    pthread_mutex_unlock(&heap_lock);

    void *new_ptr = my_malloc(size);
    if (!new_ptr)
        return NULL;
    memcpy(new_ptr, ptr, block->size);
    my_free(ptr);
    return new_ptr;
}

void print_heap_stats(void) {
    pthread_mutex_lock(&heap_lock);
    block_t *curr = heap_head;
    size_t used = 0, free_mem = 0;
    int blocks = 0;

    while (curr) {
        blocks++;
        if (curr->free) free_mem += curr->size;
        else            used     += curr->size;
        curr = curr->next;
    }

    printf("\n=== Heap Statistics ===\n");
    printf("Total Blocks: %d\n", blocks);
    printf("Used Memory: %zu bytes\n", used);
    printf("Free Memory: %zu bytes\n", free_mem);
    if (used + free_mem > 0) {
        double frag = (100.0 * free_mem) / (used + free_mem);
        printf("Fragmentation: %.2f%%\n", frag);
    }
    printf("========================\n\n");
    pthread_mutex_unlock(&heap_lock);
}

/* ===================== Arena Helpers ===================== */

static block_t *find_free_block_arena(block_t *head, size_t size) {
#ifdef FIRST_FIT
    block_t *curr = head;
    while (curr) {
        if (curr->free && curr->size >= size)
            return curr;
        curr = curr->next;
    }
    return NULL;
#endif
#ifdef BEST_FIT
    block_t *curr = head;
    block_t *best = NULL;
    while (curr) {
        if (curr->free && curr->size >= size) {
            if (!best || curr->size < best->size)
                best = curr;
        }
        curr = curr->next;
    }
    return best;
#endif
}

static void coalesce_blocks_arena(block_t *head) {
    block_t *curr = head;
    while (curr && curr->next) {
        char *curr_end = (char *)(curr + 1) + curr->size;
        if (curr->free &&
            curr->next->free &&
            curr_end == (char *)curr->next) {
            curr->size += sizeof(block_t) + curr->next->size;
            curr->next  = curr->next->next;
            curr->magic = MAGIC_FREE;
        } else {
            curr = curr->next;
        }
    }
}

/* ===================== Arena API (per-thread) ===================== */

void *my_malloc_arena(block_t **head, pthread_mutex_t *lock, size_t size) {
    if (size == 0)
        return NULL;

    pthread_mutex_lock(lock);
    size = ALIGN8(size);
    block_t *block;

    if (!*head) {
        block = request_space(size);
        if (!block) { pthread_mutex_unlock(lock); return NULL; }
        *head = block;
    } else {
        block = find_free_block_arena(*head, size);
        if (!block) {
            block_t *last = *head;
            while (last->next) last = last->next;
            block = request_space(size);
            if (!block) { pthread_mutex_unlock(lock); return NULL; }
            last->next = block;
        } else {
            split_block(block, size);
            block->free  = 0;
            block->magic = MAGIC_ALLOC;
        }
    }

    pthread_mutex_unlock(lock);
    return (void *)(block + 1);
}

void my_free_arena(block_t **head, pthread_mutex_t *lock, void *ptr) {
    if (!ptr) return;

    pthread_mutex_lock(lock);
    block_t *block = (block_t *)ptr - 1;

    if (block->magic != MAGIC_ALLOC) {
        printf("Error: Invalid or double free detected!\n");
        pthread_mutex_unlock(lock);
        return;
    }

    block->free  = 1;
    block->magic = MAGIC_FREE;
    coalesce_blocks_arena(*head);
    pthread_mutex_unlock(lock);
}

void *my_realloc_arena(block_t **head, pthread_mutex_t *lock,
                       void *ptr, size_t size) {
    if (!ptr)      return my_malloc_arena(head, lock, size);
    if (size == 0) { my_free_arena(head, lock, ptr); return NULL; }

    pthread_mutex_lock(lock);
    block_t *block = (block_t *)ptr - 1;

    if (block->magic != MAGIC_ALLOC) {
        printf("Error: Invalid realloc detected!\n");
        pthread_mutex_unlock(lock);
        return NULL;
    }

    size = ALIGN8(size);
    if (block->size >= size) {
        pthread_mutex_unlock(lock);
        return ptr;
    }
    pthread_mutex_unlock(lock);

    void *new_ptr = my_malloc_arena(head, lock, size);
    if (!new_ptr) return NULL;
    memcpy(new_ptr, ptr, block->size);
    my_free_arena(head, lock, ptr);
    return new_ptr;
}

void print_heap_stats_arena(block_t **head, pthread_mutex_t *lock) {
    pthread_mutex_lock(lock);
    block_t *curr = *head;
    size_t used = 0, free_mem = 0;
    int blocks = 0;

    while (curr) {
        blocks++;
        if (curr->free) free_mem += curr->size;
        else            used     += curr->size;
        curr = curr->next;
    }

    printf("  Total Blocks : %d\n", blocks);
    printf("  Used Memory  : %zu bytes\n", used);
    printf("  Free Memory  : %zu bytes\n", free_mem);
    if (used + free_mem > 0) {
        double frag = (100.0 * free_mem) / (used + free_mem);
        printf("  Fragmentation: %.2f%%\n", frag);
    }
    pthread_mutex_unlock(lock);
}