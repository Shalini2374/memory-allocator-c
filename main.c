#include <stdio.h>
#include <string.h>
#include <time.h>
#include "allocator.h"

int passed = 0;
int failed = 0;

void check(const char *test_name, int condition) {
    if (condition) {
        printf("  PASS: %s\n", test_name);
        passed++;
    } else {
        printf("  FAIL: %s\n", test_name);
        failed++;
    }
}

int main() {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    /* ===================== Test 1: Basic malloc ===================== */
    printf("=== Test 1: Basic Allocation ===\n");

    void *p1 = my_malloc(64);
    check("my_malloc returns non-NULL", p1 != NULL);

    // write a value and read it back
    int *num = (int *)my_malloc(sizeof(int));
    *num = 42;
    check("write and read back int value", *num == 42);
    my_free(num);

    // write a string and read it back
    char *str = (char *)my_malloc(20);
    strcpy(str, "hello");
    check("write and read back string", strcmp(str, "hello") == 0);
    my_free(str);
    my_free(p1);

    /* ===================== Test 2: Calloc zeroing ===================== */
    printf("\n=== Test 2: Calloc Zeroing ===\n");

    int *arr = (int *)my_calloc(5, sizeof(int));
    check("my_calloc returns non-NULL", arr != NULL);
    int all_zero = 1;
    for (int i = 0; i < 5; i++) {
        if (arr[i] != 0) { all_zero = 0; break; }
    }
    check("calloc memory is zeroed", all_zero);
    my_free(arr);

    /* ===================== Test 3: Realloc ===================== */
    printf("\n=== Test 3: Realloc ===\n");

    int *r = (int *)my_malloc(sizeof(int));
    *r = 99;
    r = (int *)my_realloc(r, sizeof(int) * 10);
    check("my_realloc returns non-NULL", r != NULL);
    check("realloc preserves original value", r[0] == 99);
    my_free(r);

    /* ===================== Test 4: Double Free Detection ===================== */
    printf("\n=== Test 4: Double Free Detection ===\n");

    void *x = my_malloc(128);
    check("malloc for double free test", x != NULL);
    my_free(x);
    printf("  Attempting double free (expect error message below):\n  ");
    my_free(x); // should print error, not crash
    printf("  PASS: double free caught without crash\n");
    passed++;

    /* ===================== Test 5: Fragmentation + Coalescing ===================== */
    printf("\n=== Test 5: Fragmentation and Coalescing ===\n");

    void *blocks[5];
    for (int i = 0; i < 5; i++) {
        blocks[i] = my_malloc(100);
    }
    // free alternate blocks to create fragmentation
    for (int i = 0; i < 5; i += 2) {
        if (blocks[i] != NULL) {
            my_free(blocks[i]);
            blocks[i] = NULL;
        }
    }
    printf("  Heap mid-fragmentation:\n");
    print_heap_stats();

    // free remaining blocks — coalescing should kick in
    for (int i = 0; i < 5; i++) {
        if (blocks[i] != NULL) {
            my_free(blocks[i]);
            blocks[i] = NULL;
        }
    }
    printf("  Heap after full cleanup:\n");
    print_heap_stats();

    /* ===================== Test 6: NULL safety ===================== */
    printf("=== Test 6: NULL Safety ===\n");

    void *n = my_malloc(0);
    check("my_malloc(0) returns NULL", n == NULL);
    my_free(NULL); // should not crash
    printf("  PASS: my_free(NULL) did not crash\n");
    passed++;

    /* ===================== Summary ===================== */
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) +
                     (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("\n=============================\n");
    printf("Results: %d passed, %d failed\n", passed, failed);
    printf("Test completed in %.6f seconds.\n", elapsed);
    printf("=============================\n");

    return failed > 0 ? 1 : 0;
}