# ISC 2305 - Systems Programming Assignment Question 2

# Dynamic Memory Allocator

**Name:** Tamara Zawadi  
**Reg No:** SCT212-0080/2024  
**Repository:** https://github.com/TamaraZawadi/malloc-SCT212-0080-2024
## 1. Project Overview

This project implements a custom dynamic memory allocator in C using `sbrk()`. It provides memory allocation, deallocation, zero-initialized allocation, memory resizing, block splitting, coalescing, and two memory placement policies.

The allocator also includes heap statistics, heap inspection, and consistency checking.

## 2. Build and Run

### Requirements
- GCC compiler
- GNU Make
- Linux environment

### Build and execute the official tests

```bash
make
```

### Clean generated files

```bash
make clean
```

### Run the sanitizer tests

```bash
gcc -std=c11 -g -fsanitize=address,undefined -o t_asan myalloc.c test_myalloc.c
./t_asan
```

### Run the benchmark

Compile:

```bash
gcc -std=c11 -Wall -Wextra -g -O0 -o bench bench.c myalloc.c
```

Run the three benchmark configurations:

```bash
./bench first
./bench best
./bench libc
```

## 3. Implementation Progress

| Task | Description | Status |
|---|---|---|
| Task 1 | Block header and helpers | Complete |
| Task 2 | Growing the heap with `sbrk()` | Complete |
| Task 3 | `my_malloc()` with first-fit | Complete |
| Task 4 | Splitting blocks | Complete |
| Task 5 | `my_free()` and coalescing | Complete |
| Task 6 | `my_calloc()` and `my_realloc()` | Complete |
| Task 7 | Best-fit, statistics and heap dump | Complete |
| Task 8 | Experiment and report | Complete |

## 4. Design Explanation

The allocator uses a block header containing four fields: `size`, `next`, `free`, and `magic`. The size field records the payload capacity, the next pointer links to the following block,
the free flag indicates whether the block is available, and the magic value helps detect invalid or corrupted headers. The header size is rounded up to a multiple of 16 bytes using `align_up()`,
ensuring that the payload begins at a 16-byte-aligned address. Requested payload sizes are also rounded to 16 bytes.

Blocks are organized in a singly linked list maintained through global `head` and `tail` pointers. The blocks are appended as the heap grows, and the list is maintained in increasing memory-address order.
Allocation searches this list using either first-fit, which selects the first sufficiently large free block, or best-fit, which selects the smallest suitable free block. If a free block has enough remaining
space for another header and at least 16 payload bytes, it is split into an allocated block and a smaller free block.

When memory is freed, the allocator marks the block as available and scans the list to coalesce physically adjacent free blocks. It checks actual memory addresses rather than assuming that
consecutive list entries are adjacent in memory. Merging updates the current block's size and next pointer, invalidates the merged header, and updates the tail pointer when necessary. 
Heap growth requests at least 64 KB through `sbrk()`, or more when a larger allocation is required. This design supports reuse of freed memory and reduces fragmentation.

## 5. Test Results

The provided test harness was executed using `make`.

T1 basic allocation
T2 blocks do not overlap
T3 reuse and splitting
T4 coalescing
T5 calloc and realloc
T6 heap growth via sbrk
T7 best-fit vs first-fit
T8 randomized stress test

```
make                                                                                                           
gcc -std=c11 -Wall -Wextra -g -O0 -o test_myalloc myalloc.c test_myalloc.c
./test_myalloc
T1 basic allocation
  PASS  returns non-NULL
  PASS  16-byte aligned
  PASS  heap consistent
  PASS  malloc(0) is NULL
  PASS  free(NULL) is safe
T2 blocks do not overlap
  PASS  data intact in all blocks
  PASS  heap consistent after frees
T3 reuse and splitting
  PASS  freed space is reused (no growth)
  PASS  heap did not grow
  PASS  block was split (remainder is free)
T4 coalescing
  PASS  all memory is free
  PASS  adjacent free blocks merged into one
  PASS  merged block satisfies a 550-byte request
T5 calloc and realloc
  PASS  calloc zeroes memory
  PASS  calloc overflow returns NULL
  PASS  realloc preserves contents
  PASS  shrinking keeps prefix
  PASS  realloc(NULL, n) acts like malloc
  PASS  realloc(p, 0) frees and returns NULL
  PASS  heap consistent
T6 heap growth via sbrk
  PASS  large request succeeds
  PASS  heap grew
  PASS  heap consistent
T7 best-fit vs first-fit
  PASS  first-fit takes the first hole
  PASS  best-fit takes the tightest hole
T8 randomized stress test
  PASS  no data corruption during 20000 operations
  PASS  everything freed
  PASS  fully coalesced (few free blocks remain)
  PASS  heap consistent

Result: 29 passed, 0 failed

```

All eight test groups passed.

### Additional Sanitizer Testing

The allocator was also tested using AddressSanitizer and UndefinedBehaviorSanitizer:

```bash
gcc -std=c11 -g -fsanitize=address,undefined -o t_asan myalloc.c test_myalloc.c
./t_asan
```

**Result:** 29 passed, 0 failed, with no reported AddressSanitizer or UndefinedBehaviorSanitizer errors.

## 6. Written Questions

### 1. Why must the header size be a multiple of 16 as well as the payload sizes?

This is because the payload address is calculated by adding the header size to the block's starting address. If the header size were not a multiple of 16, the payload could become misaligned even when the header was aligned.
Aligning both the header size and payload sizes ensures that subsequent block headers and payloads maintain the required 16-byte alignment.

### 2. Why can `printf()` inside `my_malloc()` cause infinite recursion?

`printf()` may internally request memory for buffering or other library operations. If it invokes the system allocator, which is implemented using `my_malloc()`, the allocator can call `printf()` again creating 
recursive allocation calls that may continue until the program crashes due to stack exhaustion.

### 3. Your `my_free()` is O(n). Which extra field would let you find a block's previous neighbor in O(1), and what is the name of the technique?

Adding a `struct block *prev` pointer to each block header would allow the allocator to access the previous list node in constant time, changing the singly linked list into a doubly linked list. 

### 4. Explain the difference between internal and external fragmentation, and name one place in your allocator where each one comes from.

Internal fragmentation refers to wasted space within an allocated block, such as the extra bytes introduced when allocation sizes are rounded up to multiples of 16. 
External fragmentation is when free memory is divided into separate non-contiguous blocks, preventing a large allocation even when the total free space is sufficient. 
In this allocator, repeated allocations and frees can create separated free blocks.

### 5. Why does `sbrk()` memory never get returned to the OS in your allocator? Describe how you could give back memory at the end of the heap.

The allocator only increases the program break using `sbrk()` and never decreases it. Memory is retained for reuse because releasing memory from the middle of the heap could invalidate other allocations. 
To return memory, the allocator could decrease the program break when the final block at the end of the heap becomes free, provided it can safely release the entire trailing region.

### 6. Two blocks are consecutive in your list but not adjacent in memory. How did this happen, and what does your coalescing code do about it?

This happens when another operation changes the program break between heap-growth requests, or when separate heap regions have alignment padding or gaps. 
The coalescing code compares the end address of the first block with the starting address of the next block. If the addresses are not equal, it leaves the blocks separate rather than merging non-contiguous memory.

## 7. Known Limitations

- The allocator uses `sbrk()`, which is deprecated and not portable to every operating system.
- The allocator is not thread-safe.
- `my_free()` performs a linear search through the linked list, giving it O(n) time complexity.
- The allocator does not return unused heap memory to the operating system.
- The singly linked list does not provide direct access to the previous block.
- The implementation is intended for educational use and is not a replacement for a production memory allocator.

## 8. Sources and AI Declaration

**Sources:**
- Systems Programming assignment instructions (https://piazza.com/class_profile/get_resource/mtvs7m59uyn4ue/mwd8lia7rgm711).
- Malloc starter pack (https://piazza.com/class_profile/get_resource/mtvs7m59uyn4ue/mwd8air55n5573).
- Linux Manual Pages (`sbrk(2)`, `write(2)`) for OS heap extension and `stdio`-free descriptor output. 
- C standard library and POSIX interfaces documentation for the functions and system calls used.
- GCC documentation for compilation and sanitizer options.

**AI tools declaration:**

OpenAI ChatGPT was used as an AI assistance tool for in depth explanations of memory allocation concepts, debugging guidance, review of implementation logic, and assistance with documentation.
Gemini AI was used for debugging and code explanations during the testing phase.
