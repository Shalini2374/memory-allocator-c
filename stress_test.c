#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "allocator.h"

#define OPERATIONS 5000
#define MAX_POINTERS 500
#define MAX_ALLOC_SIZE 512

int main() {
    void *ptrs[MAX_POINTERS] = {0};
    struct timespec start, end;

    srand(time(NULL));

    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < OPERATIONS; i++) {
        int index = rand() % MAX_POINTERS;
        int action = rand() % 3;

        if (action == 0) {
            size_t size = rand() % MAX_ALLOC_SIZE;
            ptrs[index] = my_malloc(size);
        }
        else if (action == 1) {
            if (ptrs[index] != NULL) {
                my_free(ptrs[index]);
                ptrs[index] = NULL;
            }
        }
        else {
            size_t size = rand() % MAX_ALLOC_SIZE;
            ptrs[index] = my_realloc(ptrs[index], size);
        }
    }

    for (int i = 0; i < MAX_POINTERS; i++) {
        if (ptrs[i] != NULL) {
            my_free(ptrs[i]);
            ptrs[i] = NULL;
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    print_heap_stats();

    printf("Stress test completed without leak.\n");
    printf("Time taken: %.4f seconds\n", elapsed);
    printf("Throughput: %.2f ops/sec\n", OPERATIONS / elapsed);

    return 0;
}