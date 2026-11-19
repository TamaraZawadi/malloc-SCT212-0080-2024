
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "myalloc.h"

#define OPERATIONS 50000
#define SLOT_COUNT 1024
#define SEED 20261119

typedef struct {
    int allocate;
    unsigned int slot;
    size_t size;
} operation_t;

/* Workload storage, shared by all benchmark modes */
static operation_t workload[OPERATIONS];
static void *slots[SLOT_COUNT];

/* Generate the same random workload for every run */
static void generate_workload(void)
{
    srand(SEED);

    for (size_t i = 0; i < OPERATIONS; i++) {
        workload[i].allocate = rand() % 2;
        workload[i].slot = (unsigned int)(rand() % SLOT_COUNT);
        workload[i].size = 1 + (size_t)(rand() % 4096);
    }
}

/* Return elapsed time in seconds */
static double elapsed_time(struct timespec start,
                           struct timespec end)
{
    return (double)(end.tv_sec - start.tv_sec) +
           (double)(end.tv_nsec - start.tv_nsec) / 1000000000.0;
}

int main(int argc, char *argv[])
{
    if (argc != 2 ||
        (strcmp(argv[1], "first") != 0 &&
         strcmp(argv[1], "best") != 0 &&
         strcmp(argv[1], "libc") != 0)) {

        const char *usage =
            "Usage: ./bench first | best | libc\n";
        write(STDERR_FILENO, usage, strlen(usage));
        return 1;
    }

    int use_libc = strcmp(argv[1], "libc") == 0;

    if (!use_libc) {
        set_policy(strcmp(argv[1], "best") == 0
                   ? FIT_BEST : FIT_FIRST);
    }

    generate_workload();

    size_t successful_allocations = 0;
    size_t successful_frees = 0;

    struct timespec start, end;

    if (clock_gettime(CLOCK_MONOTONIC, &start) == -1)
        return 1;

    /* Execute the predetermined workload */
    for (size_t i = 0; i < OPERATIONS; i++) {
        unsigned int slot = workload[i].slot;

        if (workload[i].allocate) {

            if (slots[slot] == NULL) {
                void *ptr;

                if (use_libc)
                    ptr = malloc(workload[i].size);
                else
                    ptr = my_malloc(workload[i].size);

                if (ptr != NULL) {
                    slots[slot] = ptr;
                    successful_allocations++;
                }
            }

        } else {

            if (slots[slot] != NULL) {

                if (use_libc)
                    free(slots[slot]);
                else
                    my_free(slots[slot]);

                slots[slot] = NULL;
                successful_frees++;
            }
        }
    }

    if (clock_gettime(CLOCK_MONOTONIC, &end) == -1)
        return 1;

    double seconds = elapsed_time(start, end);

    /* Collect allocator statistics before final cleanup */
    if (!use_libc) {
        heap_stats_t stats;
        heap_stats(&stats);

        double fragmentation = 0.0;

        if (stats.free_bytes > 0) {
            fragmentation =
                1.0 - (double)stats.largest_free /
                      (double)stats.free_bytes;
        }

        printf("Policy: %s\n",
               strcmp(argv[1], "best") == 0
                   ? "FIT_BEST" : "FIT_FIRST");

        printf("Seed: %d\n", SEED);
        printf("Operations: %d\n", OPERATIONS);
        printf("Successful allocations: %zu\n",
               successful_allocations);
        printf("Successful frees: %zu\n",
               successful_frees);
        printf("Time: %.9f seconds\n", seconds);
        printf("heap_bytes: %zu\n", stats.heap_bytes);
        printf("free_bytes: %zu\n", stats.free_bytes);
        printf("free_blocks: %zu\n", stats.free_blocks);
        printf("largest_free: %zu\n", stats.largest_free);
        printf("External fragmentation: %.6f\n",
               fragmentation);

    } else {
        printf("Allocator: C library malloc/free\n");
        printf("Seed: %d\n", SEED);
        printf("Operations: %d\n", OPERATIONS);
        printf("Successful allocations: %zu\n",
               successful_allocations);
        printf("Successful frees: %zu\n",
               successful_frees);
        printf("Time: %.9f seconds\n", seconds);
    }

    /* Release any allocations remaining in the slots */
    for (size_t i = 0; i < SLOT_COUNT; i++) {
        if (slots[i] != NULL) {
            if (use_libc)
                free(slots[i]);
            else
                my_free(slots[i]);

            slots[i] = NULL;
        }
    }

    return 0;
}
