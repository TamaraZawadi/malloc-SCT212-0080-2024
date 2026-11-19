# Task 8: Experiment and Report

## 1. Experimental Setup (bench.c)

The benchmark executed 50,000 predetermined allocation and free operations using a fixed random seed (srand) of
 20261119. Allocation sizes ranged from 1 to 4096 bytes. The same workload was executed using
FIT_FIRST, FIT_BEST, and the C library's malloc/free.

Timing was measured using `clock_gettime(CLOCK_MONOTONIC)`.

## 2. Results

| Measurement | FIT_FIRST | FIT_BEST | C Library |
|---|---:|---:|---:|
| Time (seconds) | 0.404611 | 0.225194 | 0.026086 |
| heap_bytes | 1311360 | 1311360 | N/A |
| free_bytes | 236096 | 237792 | N/A |
| free_blocks | 247 | 158 | N/A |
| largest_free | 62352 | 70304 | N/A |
| External fragmentation | 0.735904 | 0.704347 | N/A |

External fragmentation was calculated using: `1 - (largest_free / free_bytes)`

The C library statistics are marked N/A because heap_stats() reports statistics for the custom allocator,
 not the system allocator.
## 3. Discussion

I would choose FIT_BEST for this workload because it produced better fragmentation results than FIT_FIRST.
It had 158 free blocks compared to 247, a larger contiguous free region of 70,304 bytes compared to 62,352 bytes,
and a lower external fragmentation value of 0.704347 compared to 0.735904. It also completed the workload faster
in this experiment.

However, the C library's malloc/free was significantly faster, taking 0.026086 seconds compared to 0.225194 seconds
for FIT_BEST. This is because the custom allocator uses a relatively simple linked-list search to locate suitable
free blocks. Searching through block metadata introduces overhead, whereas glibc uses more sophisticated
allocation structures and optimizations. Therefore, FIT_BEST is preferable between my two policies for
this measured workload, but the C library remains considerably more efficient in execution time.
