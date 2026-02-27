#include <stdio.h>
#include <time.h>
#include "allocator.h"

int main() {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    printf("=== Double Free Test ===\n");
    void *x = my_malloc(128);
    my_free(x);
    my_free(x);

    printf("\n=== Fragmentation Test ===\n");
    void *arr[5];
    for (int i = 0; i < 5; i++) {
        arr[i] = my_malloc(100);
    }

    for (int i = 0; i < 5; i += 2) {
        if (arr[i] != NULL) {
            my_free(arr[i]);
            arr[i] = NULL;
        }
    }

    print_heap_stats();

    for (int i = 0; i < 5; i++) {
        if (arr[i] != NULL) {
            my_free(arr[i]);
            arr[i] = NULL;
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("Final Clean Stats:\n");
    print_heap_stats();
    printf("Test completed in %.6f seconds.\n", elapsed);

    return 0;
}