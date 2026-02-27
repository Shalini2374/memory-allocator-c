#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include <time.h>
#include "allocator.h"

#define THREADS 4
#define OPERATIONS 5000
#define MAX_POINTERS 200
#define MAX_ALLOC_SIZE 512

typedef struct {
    int thread_id;
} thread_arg_t;

void* thread_work(void *arg) {
    thread_arg_t *targ = (thread_arg_t *)arg;
    unsigned int seed = (unsigned int)time(NULL) ^ (targ->thread_id * 7919);
    void *ptrs[MAX_POINTERS] = {0};

    for (int i = 0; i < OPERATIONS; i++) {
        int index = rand_r(&seed) % MAX_POINTERS;
        int action = rand_r(&seed) % 3;

        if (action == 0) {
            size_t size = rand_r(&seed) % MAX_ALLOC_SIZE;
            ptrs[index] = my_malloc(size);
        }
        else if (action == 1) {
            if (ptrs[index] != NULL) {
                my_free(ptrs[index]);
                ptrs[index] = NULL;
            }
        }
        else {
            size_t size = rand_r(&seed) % MAX_ALLOC_SIZE;
            ptrs[index] = my_realloc(ptrs[index], size);
        }
    }

    for (int i = 0; i < MAX_POINTERS; i++) {
        if (ptrs[i] != NULL) {
            my_free(ptrs[i]);
            ptrs[i] = NULL;
        }
    }

    printf("Thread %d finished\n", targ->thread_id);
    return NULL;
}

int main() {
    pthread_t threads[THREADS];
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

    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    print_heap_stats();

    printf("Multi-thread test finished in %.4f seconds.\n", elapsed);
    printf("Total Throughput: %.2f ops/sec\n", (THREADS * OPERATIONS) / elapsed);

    return 0;
}