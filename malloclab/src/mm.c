/*
 * The simple memory allocator package implementing basic malloc function:
 * malloc(size_t size),
 * free(void *p),
 * realloc(void *p, size_t size).
 *
 * It uses a memory alignment by 8 for the requested blocks.
 * Structure of the block is an explicit list with block boundary headers for
 * adjacent free block coalescing. Also each free block store two pointers to another free blocks
 * thus reducing malloc function search time to O(free blocks), instead of O(all blocks)
 * with implicit list structure.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
#define PTR_SIZE (ALIGN(sizeof(uintptr_t)))

#define CHUNKSIZE (1 << 12)

#define MAX(x, y) ((x) >= (y) ? (x) : (y))

#define GET(p) (*(size_t *)(p))
#define PUT(p, val) (*(size_t *)(p) = (val))

#define GETP(p) (*(void **)(p))
#define SETP(p, val) (*(void **)(p) = (val))

#define GET_SIZE(p) (GET(p) & ~0b111)
#define GET_ALLOC(p) (GET(p) & 0b01)

#define GET_PREV_ALLOC(p) ((GET(p) >> 1) & 0b01)
#define UNSET_PREV_ALLOC(p) (PUT((p), GET(p) & ~0b10))
#define SET_PREV_ALLOC(p) (PUT((p), GET(p) | 0b10))

#define HDRP(bp) ((char *)(bp)-SIZE_T_SIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - 2 * SIZE_T_SIZE)

#define PREV_FREEP(bp) ((char *)(bp))
#define NEXT_FREEP(bp) ((char *)(bp) + PTR_SIZE)

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE((char *)(bp)-2 * SIZE_T_SIZE))

#define PACK_HDR(size, prev_alloc, alloc) ((size) | (prev_alloc << 1) | (alloc))
#define PACK_FTR(size) (size)

static void *heap_listp = NULL;
static void *free_list_head = NULL;

static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void add_to_free_list(void *bp);
static void remove_from_free_list(void *bp);

#ifdef DEBUG
static void mm_check_heap(char *caller_name);
static void check_block(void *bp, char *caller_name);
static void check_placed(void *placed, char *caller_name);
static void check_free(void *freed, char *caller_name);
#endif

/*
 * mm_init - Initialize the malloc package.
 */
int mm_init(void)
{
    if ((heap_listp = mem_sbrk(3 * SIZE_T_SIZE)) == (void *)-1)
    {
        return -1;
    }

    PUT(heap_listp, PACK_HDR(2 * SIZE_T_SIZE, 1, 1));
    heap_listp += SIZE_T_SIZE;
    PUT(HDRP(NEXT_BLKP(heap_listp)), PACK_HDR(0, 1, 1));
    free_list_head = NULL;
#ifdef DEBUG
    mm_check_heap("mm_init");
#endif

    return 0;
}

/*
 * mm_malloc - Allocate a block by searching through the explicit free list,
 requesting additional heap memory if no block was found.
 */
void *mm_malloc(size_t size)
{
    if (size == 0)
    {
        return NULL;
    }

    if (heap_listp == NULL)
    {
        mm_init();
    }

    int asize = MAX(ALIGN(size + SIZE_T_SIZE), 2 * PTR_SIZE + 2 * SIZE_T_SIZE);
    void *bp;
    if ((bp = find_fit(asize)) != NULL)
    {
        place(bp, asize);
#ifdef DEBUG
        mm_check_heap("mm_malloc");
#endif
        return bp;
    }

    size_t extend_size = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extend_size)) == NULL)
    {
        return NULL;
    }

    place(bp, asize);
#ifdef DEBUG
    mm_check_heap("mm_malloc");
#endif

    return bp;
}

/*
 * mm_free - Free a block by removing it from the explicit free list
 and updating the boundary tags.
 */
void mm_free(void *bp)
{
    if (bp == NULL)
    {
        return;
    }

    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK_HDR(size, GET_PREV_ALLOC(HDRP(bp)), 0));
    PUT(FTRP(bp), PACK_FTR(size));
    UNSET_PREV_ALLOC(HDRP(NEXT_BLKP(bp)));

    coalesce(bp);
#ifdef DEBUG
    mm_check_heap("mm_free");
#endif
}

/*
 * mm_realloc - Keep the current block if the requested size is less or equal
 * than the current block size. Allocate a new block in the heap otherwise.
 * Th new block size is twice time bigger than the requested to facilitate further
 * block size requsts for this object.
 */
void *mm_realloc(void *bp, size_t size)
{
    if (bp == NULL)
    {
        return mm_malloc(size);
    }

    if (size == 0)
    {
        mm_free(bp);

        return NULL;
    }

    size_t asize = MAX(ALIGN(size + SIZE_T_SIZE), 2 * PTR_SIZE + 2 * SIZE_T_SIZE);
    size_t csize = GET_SIZE(HDRP(bp));
    if (asize > csize)
    {
        void *new_bp = mm_malloc(size * 2);
        if (new_bp == NULL)
        {
            return NULL;
        }

        size_t copy_size = csize - SIZE_T_SIZE;
        memcpy(new_bp, bp, copy_size);
        mm_free(bp);
#ifdef DEBUG
        mm_check_heap("mm_realloc");
#endif

        return new_bp;
    }

    return bp;
}

static void *find_fit(size_t asize)
{
    void *bp;
    for (bp = free_list_head; bp != NULL; bp = GETP(NEXT_FREEP(bp)))
    {
        if (asize <= GET_SIZE(HDRP(bp)))
        {
            return bp;
        }
    }

    return NULL;
}

static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));
    if (csize >= asize + 2 * SIZE_T_SIZE + 2 * PTR_SIZE)
    {
        PUT(HDRP(bp), PACK_HDR(asize, 1, 1));
        remove_from_free_list(bp);
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK_HDR(csize - asize, 1, 0));
        PUT(FTRP(bp), PACK_FTR(csize - asize));
        add_to_free_list(bp);
    }
    else
    {
        PUT(HDRP(bp), PACK_HDR(csize, 1, 1));
        SET_PREV_ALLOC(HDRP(NEXT_BLKP(bp)));
        remove_from_free_list(bp);
    }
}

static void *extend_heap(size_t size)
{
    size_t asize = ALIGN(size);
    void *bp;
    if ((bp = mem_sbrk(asize)) == (void *)-1)
    {
        return NULL;
    }

    PUT(HDRP(bp), PACK_HDR(asize, GET_PREV_ALLOC(HDRP(bp)), 0));
    PUT(FTRP(bp), PACK_FTR(asize));
    PUT(HDRP(NEXT_BLKP(bp)), PACK_HDR(0, 0, 1));

    return coalesce(bp);
}

static void add_to_free_list(void *bp)
{
    SETP(PREV_FREEP(bp), NULL);
    SETP(NEXT_FREEP(bp), free_list_head);
    if (free_list_head != NULL)
    {
        SETP(PREV_FREEP(free_list_head), bp);
    }

    free_list_head = bp;
#ifdef DEBUG
    check_free(bp, "add_to_free_list");
#endif
}

static void remove_from_free_list(void *bp)
{
    void *prev = GETP(PREV_FREEP(bp));
    void *next = GETP(NEXT_FREEP(bp));
    if (prev != NULL)
    {
        SETP(NEXT_FREEP(prev), next);
    }
    else
    {
        free_list_head = next;
    }

    if (next != NULL)
    {
        SETP(PREV_FREEP(next), prev);
    }
#ifdef DEBUG
    check_placed(bp, "remove_from_free_list");
#endif
}

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_PREV_ALLOC(HDRP(bp));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    if (prev_alloc && !next_alloc)
    {
        remove_from_free_list(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK_HDR(size, 1, 0));
        PUT(FTRP(bp), PACK_FTR(size));
    }
    else if (!prev_alloc && next_alloc)
    {
        remove_from_free_list(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK_FTR(size));
        PUT(HDRP(PREV_BLKP(bp)), PACK_HDR(size, 1, 0));
        bp = PREV_BLKP(bp);
    }
    else if (!prev_alloc && !next_alloc)
    {
        remove_from_free_list(PREV_BLKP(bp));
        remove_from_free_list(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
                GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK_HDR(size, 1, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK_FTR(size));

        bp = PREV_BLKP(bp);
    }

    add_to_free_list(bp);

    return bp;
}

#ifdef DEBUG
static void mm_check_heap(char *caller_name)
{
    char *bp = heap_listp;
    if ((GET_SIZE(HDRP(heap_listp)) != 2 * SIZE_T_SIZE) || !GET_ALLOC(HDRP(heap_listp)))
    {
        printf("Error %s: Bad prologue header\n", caller_name);
    }

    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
    {
        check_block(bp, caller_name);
    }

    if ((GET_SIZE(HDRP(bp)) != 0) || !(GET_ALLOC(HDRP(bp))))
    {
        printf("Error %s: Bad epilogue header\n", caller_name);
    }
}

static void check_free(void *freed, char *caller_name)
{
    int free_found = 0;
    char *bp;
    for (bp = free_list_head; bp != NULL; bp = GETP(NEXT_FREEP(bp)))
    {
        if (!GET_ALLOC(HDRP(bp)))
        {
            if (bp == freed)
            {
                free_found += 1;
            }
        }
        else
        {
            printf("Error %s: allocated in free list\n", caller_name);
        }
    }

    if (free_found != 1)
    {
        printf("Error %s: freed block not added to free list\n", caller_name);
    }
}

static void check_placed(void *placed, char *caller_name)
{
    int placed_found = 0;
    char *bp;
    for (bp = free_list_head; bp != NULL; bp = GETP(NEXT_FREEP(bp)))
    {
        if (!GET_ALLOC(HDRP(bp)))
        {
            if (bp == placed)
            {
                placed_found += 1;
            }
        }
        else
        {
            printf("Error %s: allocated in free list\n", caller_name);
        }
    }

    if (placed_found != 0)
    {
        printf("Error %s: placed block in free list\n", caller_name);
    }
}

static void check_block(void *bp, char *caller_name)
{
    if ((size_t)bp % 8)
    {
        printf("Error %s: wrong double word aligned\n", caller_name);
    }

    if (!GET_ALLOC(HDRP(bp)))
    {
        if (GET_SIZE(HDRP(bp)) != GET_SIZE(FTRP(bp)))
        {
            printf("Error %s: header does not match footer\n", caller_name);
        }

        void *prev = GETP(PREV_FREEP(bp));
        void *next = GETP(NEXT_FREEP(bp));
        if (prev != NULL && GET_ALLOC(HDRP(prev)))
        {

            printf("Error %s: prev points to allocated\n", caller_name);
        }

        if (next != NULL && GET_ALLOC(HDRP(next)))
        {
            printf("Error %s: next points to allocated\n", caller_name);
        }
    }
}
#endif