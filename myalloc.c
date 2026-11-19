#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#define CHUNK_SIZE (64 * 1024)
#define MAGIC 0x4D59414C

/* Memory block header */
typedef struct block {
    size_t size;
    struct block *next;
    int free;
    int magic;
}
 block_t;
/* Global allocator state */
static block_t *head = NULL;
static block_t *tail = NULL;
static size_t heap_bytes = 0;

/* Round a size up to the nearest multiple of 16 */
static size_t align_up(size_t n)
{
    return (n + 15) & ~(size_t)15;
}
#define HDR (align_up(sizeof(block_t)))
/* Return the address of the usable payload */
static void *payload(block_t *block)
{
    return (char *)block + HDR;
}
/* Recover the block header from a payload pointer */
static block_t *header(void *ptr)
{
    return (block_t *)((char *)ptr - HDR);
}
/* Request more memory from the operating system */
static block_t *grow(size_t need)
{
    /* Get the current program break */
    void *current_break = sbrk(0);

    if (current_break == (void *)-1)
        return NULL;

    /* Calculate padding for 16-byte alignment */
    size_t padding = (16 - ((uintptr_t)current_break % 16)) % 16;

    /* Determine the payload size to obtain */
    if (need > SIZE_MAX - 15)
        return NULL;

    size_t chunk = need > CHUNK_SIZE ? need : CHUNK_SIZE;
    chunk = align_up(chunk);

    /* Calculate the total memory required */
    if (chunk > SIZE_MAX - HDR - 15)
        return NULL;

    size_t total = padding + HDR + chunk;

    if (total > (size_t)INTPTR_MAX)
        return NULL;
    if (heap_bytes > SIZE_MAX - total)
        return NULL;

    /* Request memory from the operating system */
    void *memory = sbrk((intptr_t)total);

    if (memory == (void *)-1)
        return NULL;

    /* Create the new block at an aligned address */
    block_t *new_block = (block_t *)((char *)memory + padding);

    new_block->size = chunk;
    new_block->next = NULL;
    new_block->free = 1;
    new_block->magic = MAGIC;

    /* Append the block to the linked list */
    if (head == NULL) {
        head = new_block;
        tail = new_block;
    } else {
        tail->next = new_block;
        tail = new_block;
    }

    /* Update total memory obtained */
    heap_bytes += total;

    return new_block;
}
