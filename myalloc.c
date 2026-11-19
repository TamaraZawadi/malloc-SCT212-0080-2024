#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "myalloc.h"


#define CHUNK_SIZE (64 * 1024)
#define MAGIC 0x4D59414C

/* Memory block header */
typedef struct block {
    size_t size;
    struct block *next;
    int free;
    int magic;
} block_t;
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
/* Find the first free block large enough for the request */
static block_t *find_fit(size_t need)
{
    block_t *current = head;

    while (current != NULL) {
        if (current->free && current->size >= need)
            return current;

        current = current->next;
    }

    return NULL;
}
/* Split a free block when the remaining space is sufficient */
static void split_block(block_t *block, size_t need)
{
    /* Check whether the remainder can hold a header and 16-byte payload */
    if (block->size < need || block->size - need < HDR + 16)
        return;

    /* Calculate the address of the new block header */
    block_t *new_block = (block_t *)((char *)block + HDR + need);

    /* Initialize the new free block */
    new_block->size = block->size - need - HDR;
    new_block->next = block->next;
    new_block->free = 1;
    new_block->magic = MAGIC;

    /* Update the original block */
    block->size = need;
    block->next = new_block;

    /* Update tail if the original block was the last block */
    if (tail == block)
        tail = new_block;
}

/* Allocate memory using the first-fit policy */
void *my_malloc(size_t size)
{
    if (size == 0)
        return NULL;

    if (size > SIZE_MAX - 15)
        return NULL;

    size = align_up(size);

    /* Search for a suitable free block */
    block_t *block = find_fit(size);

    /* Grow the heap if no suitable block exists */
    if (block == NULL) {
        if (grow(size) == NULL)
            return NULL;

        /* Retry the search after growing */
        block = find_fit(size);
    }

    if (block == NULL)
        return NULL;

    /* Split the block if enough space remains */
    split_block(block, size);

    /* Mark the selected block as allocated */
    block->free = 0;

    return payload(block);
}
/* Free an allocated memory block */
void my_free(void *ptr)
{
    /* Ignore NULL pointers */
    if (ptr == NULL)
        return;

    /* Find the block that owns this payload pointer */
    block_t *block = head;

    while (block != NULL && payload(block) != ptr)
        block = block->next;

    /* Reject pointers that do not belong to the allocator */
    if (block == NULL || block->magic != MAGIC) {
        const char message[] = "my_free: invalid pointer\n";
        write(STDERR_FILENO, message, sizeof(message) - 1);
        return;
    }

    /* Check whether the block has already been freed */
    if (block->free) {
        const char message[] = "my_free: double free detected\n";
        write(STDERR_FILENO, message, sizeof(message) - 1);
        return;
    }

    /* Mark the block as free */
    block->free = 1;

    /* Coalesce physically adjacent free blocks */
    block_t *current = head;

    while (current != NULL && current->next != NULL) {
        block_t *next = current->next;

        /* Check whether both blocks are free and physically adjacent */
        if (current->free && next->free &&
            (char *)current + HDR + current->size == (char *)next) {

            /* Merge the next block into the current block */
            current->size += HDR + next->size;
            current->next = next->next;
            next->magic = 0;

            /* Update tail if the last block was merged */
            if (tail == next)
                tail = current;
        } else {
            current = current->next;
        }
    }
}
/* Allocate memory for an array and initialize it to zero */
void *my_calloc(size_t nmemb, size_t size)
{
    /* Check for multiplication overflow */
    if (nmemb != 0 && size > SIZE_MAX / nmemb)
        return NULL;

    /* Calculate the total allocation size */
    size_t total = nmemb * size;

    /* Allocate memory */
    void *ptr = my_malloc(total);

    if (ptr == NULL)
        return NULL;

    /* Initialize allocated memory to zero */
    memset(ptr, 0, total);

    return ptr;
}

/* Resize an allocated memory block */
void *my_realloc(void *ptr, size_t size)
{
    /* If ptr is NULL, behave like my_malloc */
    if (ptr == NULL)
        return my_malloc(size);

    /* If size is zero, free the block */
    if (size == 0) {
        my_free(ptr);
        return NULL;
    }

    /* Check for size alignment overflow */
    if (size > SIZE_MAX - 15)
        return NULL;

    /* Align the requested size */
    size = align_up(size);

    /* Find the block that owns this pointer */
    block_t *block = head;

    while (block != NULL && payload(block) != ptr)
        block = block->next;

    /* Reject invalid or freed pointers */
    if (block == NULL || block->magic != MAGIC || block->free)
        return NULL;

    /* If the current block is large enough, reuse it */
    if (block->size >= size) {
        split_block(block, size);
        return ptr;
    }

    /* Try to absorb adjacent free blocks */
    while (block->next != NULL && block->next->free &&
           (char *)block + HDR + block->size == (char *)block->next) {

        block_t *next = block->next;

        /* Merge the adjacent free block */
        block->size += HDR + next->size;
        block->next = next->next;
        next->magic = 0;

        /* Update tail if the last block was absorbed */
        if (tail == next)
            tail = block;

        /* Stop when the combined block is large enough */
        if (block->size >= size)
            break;
    }

    /* Reuse the expanded block if it is now large enough */
    if (block->size >= size) {
        split_block(block, size);
        return ptr;
    }

    /* Otherwise, allocate a new block */
    void *new_ptr = my_malloc(size);

    if (new_ptr == NULL)
        return NULL;

    /* Copy the existing contents */
    size_t copy_size = block->size < size ? block->size : size;
    memcpy(new_ptr, ptr, copy_size);

    /* Free the old block */
    my_free(ptr);

    return new_ptr;
}
