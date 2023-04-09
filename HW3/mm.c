/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"
/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your information in the following struct.
 ********************************************************/
team_t team = {
    /* Your student ID */
	"20181693",
    /* Your full name*/
    "Wonbin Choi",
    /* Your email address */
    "gogo4261@naver.com",
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* $begin mallocmacros */
/* Basic constants and macros */
#define WSIZE       4       /* Word and header/footer size (bytes) */ //line:vm:mm:beginconst
#define DSIZE       8       /* Doubleword size (bytes) */
#define CHUNKSIZE  (1 << 10)  /* Extend heap by this amount (bytes) *///1024가 best

#define MAX(x, y) ((x) > (y)? (x) : (y))  

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc)) //line:vm:mm:pack

/* Read and write a word at address p */
#define GET(p)       (*(unsigned int *)(p))//line:vm:mm:get
#define PUT(p, val)  (*(unsigned int *)(p) = (val))//line:vm:mm:put

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)//line:vm:mm:getsize
#define GET_ALLOC(p) (GET(p) & 0x1)//line:vm:mm:getalloc

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((char *)(bp) - WSIZE)                      //line:vm:mm:hdrp
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) //line:vm:mm:ftrp

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))//line:vm:mm:nextblkp
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))//line:vm:mm:prevblkp
/* $end mallocmacros */

/* Global variables */
static char* heap_listp = 0;  /* Pointer to first block */

/* Function prototypes for internal helper routines */
static void* extend_heap(size_t words);
static void place(void* bp, size_t asize);
static void* find_fit(size_t asize);
static void* coalesce(void* bp);

static int checkblock(void* bp);
int mm_check(void);

//더 필요한 것들
#define MINIMUM 24//default 느낌. H F 0아니면 최소 8 P N

#define NEXT_FP(bp)  (*(void **)(bp + DSIZE))//다음 프리 블록
#define PREV_FP(bp)  (*(void **)(bp)) //이전

static char* free_listp = 0;

static void insert_front(void* bp);//앞에 넣는 걸로 하자
static void remove_block(void* bp);//
static void insert_ordered(void* bp);//주소순으로(역순)->공간사용률 2증가, 처리량 4정도 떨어짐

/*
 * mm_init - Initialize the memory manager
 */
 /* $begin mminit */
int mm_init(void)
{
    /* Create the initial empty heap */
    if ((heap_listp = mem_sbrk(MINIMUM + DSIZE)) == (void*)-1) //처음에 8단위 맞춰줌. 0 H |
        return -1;
    PUT(heap_listp, 0);/* Alignment padding */
    PUT(heap_listp + (1 * WSIZE), PACK(MINIMUM, 1)); /* Prologue header */
    PUT(heap_listp + (2 * WSIZE), 0); /* Previous pointer */
    PUT(heap_listp + (3 * WSIZE), 0);/* Next pointer */
    PUT(heap_listp + MINIMUM, PACK(MINIMUM, 1));//Put the footer block of the prologue
    PUT(heap_listp + WSIZE + MINIMUM, PACK(0, 1));//Put the header block of the epilogue

    free_listp = heap_listp + 2 * WSIZE;//처음 fee_listp (블록포인트임 헤더x)
    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)//적당히 -> 24가 좋은듯
        return -1;
    return 0;
}
/* $end mminit */

/*
 * mm_malloc - Allocate a block with at least size bytes of payload
 */
 /* $begin mmmalloc */
void* mm_malloc(size_t size)
{
    size_t asize;      /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char* bp;

    if (size == 0)//근데 사이즈가 0보다 작은 경우 -> 안 들어오는듯
        return NULL;

    asize = MAX(ALIGN(size) + DSIZE, MINIMUM);//최소24, 혹은 얼라인된 사이즈에 헤더푸터, 포인터 필요 x
    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {//line:vm:mm:findfitcall //찾음
        place(bp, asize);//line:vm:mm:findfitplace
        return bp;
    }
    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize, CHUNKSIZE);//line:vm:mm:growheap1 //못 찾음 늘림

    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;//line:vm:mm:growheap2
    place(bp, asize);//line:vm:mm:growheap3
    return bp;
}
/* $end mmmalloc */

/*
 * mm_free - Free a block
 */
 /* $begin mmfree */
void mm_free(void* bp)
{
    /* $end mmfree */
    if (bp == 0)
        return;

    /* $begin mmfree */
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

/* $end mmfree */
/*
 * coalesce - Boundary tag coalescing. Return ptr to coalesced block
 */
static void* coalesce(void* bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))) || PREV_BLKP(bp) == bp;
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {/* Case 1 */
        insert_front(bp);//추가해주고 보내기
     //   insert_ordered(bp);
        return bp;
    }
    else if (prev_alloc && !next_alloc) {/* Case 2 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        remove_block(NEXT_BLKP(bp));//Remove the next block
        PUT(HDRP(bp), PACK(size, 0)); //다음것을 지우고 사이즈를 늘림, 헤더 푸터 표시
        PUT(FTRP(bp), PACK(size, 0));
    }
    else if (!prev_alloc && next_alloc) {/* Case 3 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        bp = PREV_BLKP(bp);//Update the block pointer to the previous block
        remove_block(bp);// 이전 것일 경우 bp 이전 것으로 이동, 지금 bp엔 아직 포인터x, 이전거지워줌, 나중에 새로추가
        PUT(HDRP(bp), PACK(size, 0));//위에 이래야 한줄이라도 아낌 -> 성능 좋아진 것 같기도? 아 이거 밑에임
        PUT(FTRP(bp), PACK(size, 0));
    }
    else {/* Case 4 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
        remove_block(NEXT_BLKP(bp));//Remove the block next to the current block
        bp = PREV_BLKP(bp);//Update the block pointer to the previous block
        remove_block(bp);//Remove the block previous to the current block
        PUT(HDRP(bp), PACK(size, 0));//Update the new block's header
        PUT(FTRP(bp), PACK(size, 0));
    }
    insert_front(bp);//한번에 넣기, 따로따로 x
//    insert_ordered(bp);
    /* $begin mmfree */
//   check_block(bp)//체크 여기서 이전포인터가 힙최대 넘어간거 잡았었음 -> seg 실패한 이유 알아냄
    return bp;
}
/* $end mmfree */

static void insert_front(void* bp) {
    PREV_FP(bp) = NULL;
    NEXT_FP(bp) = free_listp;
    PREV_FP(free_listp) = bp;
    free_listp = bp;
    //원래 처음것이 현재의 다음것, 원래 처음것의 이전 것이 bp, bp의 이전을 NULL로 free_listp가 bp를 가리키게(새로운처음)
}

static void insert_ordered(void* bp) {
    void* temp;
    if (bp > free_listp) {
        PREV_FP(bp) = NULL;
        NEXT_FP(bp) = free_listp;
        PREV_FP(free_listp) = bp;
        free_listp = bp;
    }
    else if (bp == free_listp) {
        free_listp = bp;
    }
    else {
        for (temp = free_listp; GET_ALLOC(HDRP(temp)) == 0; temp = NEXT_FP(temp)) {//
            if (bp > NEXT_FP(temp) && bp < temp) {
                NEXT_FP(bp) = NEXT_FP(temp);
                PREV_FP(NEXT_FP(temp)) = bp;
                NEXT_FP(temp) = bp;
                PREV_FP(bp) = temp;
                break;
            }
        }
    }
}

static void remove_block(void* bp) {
    if (PREV_FP(bp)) {//free list의 이전것이 할당 -> 이전 것이 다음 것 가리키게
        NEXT_FP(PREV_FP(bp)) = NEXT_FP(bp);//Set the next pointer of the previous block to next block
    }
    else {//이전 것이 존재x -> 처음임 -> 다음것이 처음으로
        free_listp = NEXT_FP(bp);//Set the free list to the next block
    }
    PREV_FP(NEXT_FP(bp)) = PREV_FP(bp);//다음 것의 이전 것은 현재 것의 이전 것, NULL이 될수도
}


/*
 * mm_realloc - Naive implementation of realloc
 */
void* mm_realloc(void* ptr, size_t size)
{
    size_t oldsize;
    void* newptr;

    /* If size == 0 then this is just free, and we return NULL. */
    if (size == 0) {
        mm_free(ptr);
        return 0;
    }

    /* If oldptr is NULL, then this is just malloc. */
    if (ptr == NULL) {
        return mm_malloc(size);
    }

    size_t asize = MAX(ALIGN(size) + DSIZE, MINIMUM);

    /* Copy the old data. */
    oldsize = GET_SIZE(HDRP(ptr));//원래 사이즈

    if (oldsize == asize) {//같으면 그대로 반환
        return ptr;
    }

    if (asize <= oldsize) {//조정한 사이즈가 원래보다 작음
        if (oldsize - asize <= MINIMUM) {//차가 오버헤드보다 작음
            return ptr;
        }
        PUT(HDRP(ptr), PACK(asize, 1));
        PUT(FTRP(ptr), PACK(asize, 1));
        PUT(HDRP(NEXT_BLKP(ptr)), PACK(oldsize - asize, 1));//더크면 그 부분 프리, 여기 부분 최적화 가능할듯?
        mm_free(NEXT_BLKP(ptr));
        return ptr;
    }

    newptr = mm_malloc(size);//더크면 새로 생성

    /* If realloc() fails the original block is left untouched  */
    if (!newptr)
        return 0;

    if (size < oldsize)//ex. 17 -> 32, 24
        oldsize = size;

    memcpy(newptr, ptr, oldsize);//한줄이라도 아끼자

    /* Free the old block. */
    mm_free(ptr);

    return newptr;
}
 /*
  * extend_heap - Extend heap with free block and return its block pointer
  */
  /* $begin mmextendheap */
static void* extend_heap(size_t words)
{
    char* bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = MAX(ALIGN(words*WSIZE), MINIMUM);//이게 제일 효율적일듯

    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;//line:vm:mm:endextend
    //바로 printf 하면 안됨, 못 알아봄
    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));/* Free block header */   //line:vm:mm:freeblockhdr
    PUT(FTRP(bp), PACK(size, 0));/* Free block footer */   //line:vm:mm:freeblockftr
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));/* New epilogue header */ //line:vm:mm:newepihdr

    /* Coalesce if the previous block was free */
    return coalesce(bp);//line:vm:mm:returnblock
}
/* $end mmextendheap */

/*
 * place - Place block of asize bytes at start of free block bp
 *         and split if remainder would be at least minimum block size
 */
 /* $begin mmplace */
 /* $begin mmplace-proto */
static void place(void* bp, size_t asize)
/* $end mmplace-proto */
{
    size_t csize = GET_SIZE(HDRP(bp));//원래 있던 것

    if ((csize - asize) >= (MINIMUM)) {// 차가 최소보다 큼 -> 프리블록 하나 추가생성 가능
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        remove_block(bp);//할당되었으니 빼주기
        bp = NEXT_BLKP(bp);//다음 것(남은 것)은 추가
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
        coalesce(bp);//Coalesce the new free block with the adjacent free blocks
    }
    else {//남은 것 애매하면 그대로 써주기
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
        remove_block(bp);
    }
}
/* $end mmplace */

/*
 * find_fit - Find a fit for a block with asize bytes
 */
 /* $begin mmfirstfit */
static void* find_fit(size_t asize)
{
    /* First fit search */
    void* bp;

    for (bp = free_listp; GET_ALLOC(HDRP(bp)) == 0; bp = NEXT_FP(bp)) {//리스트 마지막(내가 설정해 놓은 것) 할당되어있음.
        if (asize <= GET_SIZE(HDRP(bp))) {//사이즈 들어가는 것 찾으면 리턴
            return bp;
        }
    }
    return NULL; /* No fit */
/* $end mmfirstfit */
}
/*static int checkblock(void* bp)
{
    if ((size_t)bp % 8) {
        printf("Error: %p is not doubleword aligned\n", bp);
        return -1;
    }
    if (GET(HDRP(bp)) != GET(FTRP(bp))) {
        printf("Error: header does not match footer\n");
        return -1;
    }
    if (NEXT_FP(bp) < mem_heap_lo() || NEXT_FP(bp) > mem_heap_hi()) {
        printf("Error: Next free pointer %p is out of bound\n", NEXT_FP(bp));
        return -1;
    }
    if (PREV_FP(bp) < mem_heap_lo() || PREV_FP(bp) > mem_heap_hi()) {
        printf("Error: Previous free pointer %p is out of bound", PREV_FP(bp));
        return -1;
    }
}
int mm_check(void) {
    void* bp = free_listp;
    printf("Heap (%p): \n", free_listp);
    if (check_block(free_listp) == -1) {
        return -1;
    }
    for (bp = free_listp; GET_ALLOC(HDRP(bp)) == 0; bp = NEXT_FP(bp)) {
        if (check_block(bp) == -1) {
            return -1;
        }
    }
    return 1;
}*/