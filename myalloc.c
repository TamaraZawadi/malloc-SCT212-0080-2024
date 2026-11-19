#define _DEFAULT_SOURCE
#include <stddef.h>
#include <stdint.h>

/* Memory block header */
typedef struct block {
    size_t size;
    struct block *next;
    int free;
    int magic;
}
 block_t;

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
