#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include "allocator.h"

#define THREADS 4
#define OPERATIONS 5000
#define MAX_POINTERS 200
#define MAX_ALLOC_SIZE 512

/* ── Per-thread arena ── */
typedef struct block block_t;   // forward-decl (already in allocator.h)

typedef struct {
    block_t         *head;
    pthread_mutex_t  lock;
} arena_t;

static arena_t arenas[THREADS];

/* thread-local index so allocator helpers know which arena to use */
static __thread int my_arena_id = -1;

/* ── Arena-local allocator wrappers ── */
/*
 * These call the same allocator logic but each thread only ever
 * touches its own arena_t, so there is zero cross-thread contention.
 */
static void *arena_malloc(size_t size) {
    return my_malloc_arena(&arenas[my_arena_id].head,
                           &arenas[my_arena_id].lock, size);
}
static void arena_free(void *ptr) {
    my_free_arena(&arenas[my_arena_id].head,
                  &arenas[my_arena_id].lock, ptr);
}
static void *arena_realloc(void *ptr, size_t size) {
    return my_realloc_arena(&arenas[my_arena_id].head,
                            &arenas[my_arena_id].lock, ptr, size);
}

/* ── Thread work ── */
typedef struct { int thread_id; } thread_arg_t;

void *thread_work(void *arg) {
    thread_arg_t *targ = (thread_arg_t *)arg;
    my_arena_id = targ->thread_id;          // bind thread to its arena

    unsigned int seed = (unsigned int)time(NULL) ^ (targ->thread_id * 7919);
    void *ptrs[MAX_POINTERS] = {0};

    for (int i = 0; i < OPERATIONS; i++) {
        int index  = rand_r(&seed) % MAX_POINTERS;
        int action = rand_r(&seed) % 3;

        if (action == 0) {
            size_t size = (rand_r(&seed) % MAX_ALLOC_SIZE) + 1;
            if (ptrs[index] != NULL) {
                arena_free(ptrs[index]);
                ptrs[index] = NULL;
            }
            ptrs[index] = arena_malloc(size);
        }
        else if (action == 1) {
            if (ptrs[index] != NULL) {
                arena_free(ptrs[index]);
                ptrs[index] = NULL;
            }
        }
        else {
            size_t size = (rand_r(&seed) % MAX_ALLOC_SIZE) + 1;
            void *new_ptr = arena_realloc(ptrs[index], size);
            if (new_ptr != NULL)
                ptrs[index] = new_ptr;
        }
    }

    /* cleanup */
    for (int i = 0; i < MAX_POINTERS; i++) {
        if (ptrs[i] != NULL) {
            arena_free(ptrs[i]);
            ptrs[i] = NULL;
        }
    }

    printf("Thread %d finished\n", targ->thread_id);
    return NULL;
}

/* ── Main ── */
int main() {
    /* initialise all arenas */
    for (int i = 0; i < THREADS; i++) {
        arenas[i].head = NULL;
        pthread_mutex_init(&arenas[i].lock, NULL);
    }

    pthread_t    threads[THREADS];
    thread_arg_t args[THREADS];
    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < THREADS; i++) {
        args[i].thread_id = i;
        if (pthread_create(&threads[i], NULL, thread_work, &args[i]) != 0) {
            perror("pthread_create failed");
            return 1;
        }
    }
    for (int i = 0; i < THREADS; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join failed");
            return 1;
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) +
                     (end.tv_nsec - start.tv_nsec) / 1e9;

    /* print stats for each arena separately */
    for (int i = 0; i < THREADS; i++) {
        printf("Arena %d stats:\n", i);
        print_heap_stats_arena(&arenas[i].head, &arenas[i].lock);
        pthread_mutex_destroy(&arenas[i].lock);
    }

    printf("Multi-thread test finished in %.4f seconds.\n", elapsed);
    printf("Total Throughput: %.2f ops/sec\n",
           (double)(THREADS * OPERATIONS) / elapsed);
    return 0;
}