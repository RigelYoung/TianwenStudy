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
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "Rigel's Team",
    /* First member's full name */
    "Rigel 青青",
    /* First member's email address */
    "565981292@qq.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};
/*
free chunk的结构
    +-------------+
    | chunksize|00|   <- chunk size 包含两个标志位，表示当前chunk以及上一个chunk是否使用
    +-------------+
    |       fd    |   <- 用户所使用的空间，包括对齐的空间。该空间以8字节为对齐标准
    |       bk    |
    |   (padding) |
    +-------------+
    |  chunksize  |   <- chunk size，无标志位，可空间复用
    +-------------+
*/


/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8
/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE           4
#define DSIZE           (2*WSIZE)
#define MIN_CHUNKSIZE   4*WSIZE
#define CHUNKSIZE       0

/* Pack a size and allocated bit into a word. */
#define ALLOC_MASK      1
#define PREV_FREE_MASK  2
//#define PACK(size, alloc, prev_free) ((size) | (alloc) | (prev_free))

/* Read and write a word at address p */
#define GET(p)              (*(unsigned int*)(p))

/* Read the size and allocate fields form address p */
/* 此时的p应指向chunk头部 */

#define GET_RAW_SIZE(p)         (GET(p))
#define GET_SIZE(p)             (GET_RAW_SIZE(p) & ~0x7)
#define GET_PREV_SIZE(p)        (GET_SIZE((p) - WSIZE))
// 这里的SET_SIZE将会保留最后2位
#define SET_SIZE(p, val)        (GET_RAW_SIZE(p) = (val | (GET_RAW_SIZE(p) & (ALLOC_MASK | PREV_FREE_MASK))))


#define GET_ALLOC(p)                (GET_RAW_SIZE(p) & ALLOC_MASK)
#define SET_ALLOC(p)                (GET_RAW_SIZE(p) |= ALLOC_MASK)
#define SET_FREE(p)              (GET_RAW_SIZE(p) &= ~ALLOC_MASK)

#define GET_PREV_FREE(p)     (GET_RAW_SIZE(p) & PREV_FREE_MASK ) 
#define SET_PREV_FREE(p)            (GET_RAW_SIZE(p) |= PREV_FREE_MASK ) 
#define SET_PREV_ALLOC(p)          (GET_RAW_SIZE(p) &= ~PREV_FREE_MASK ) 

#define getListIndx(chunkSize) \
    ((chunkSize) >= (1 << 12) ? 8 :  \
    ((chunkSize) >= (1 << 11) ? 7 :  \
    ((chunkSize) >= (1 << 10) ? 6 :  \
    ((chunkSize) >= (1 << 9) ? 5 :   \
    ((chunkSize) >= (1 << 8) ? 4 :   \
    ((chunkSize) >= (1 << 7) ? 3 :   \
    ((chunkSize) >= (1 << 6) ? 2 :   \
    (chunkSize) >= (1 << 5) ? 1 :    \
    (chunkSize) >= (1 << 4) ? 0 : -1 \
    )))))))


// footer可以空间复用
#define request2chunksize(size) \
     ((size) > (MIN_CHUNKSIZE - WSIZE) ? ALIGN(size+WSIZE) : MIN_CHUNKSIZE)

#define FD(p)           (*(void**)((void*)(p) + WSIZE))
// 传入指向fd的指针，传出指向该chunk的header的指针
#define FD2HD(p)        ((void*)((void*)(p) - WSIZE))
#define BK(p)           (*(void**)((void*)(p) + DSIZE))
#define NEXT_CHUNK(p)   ((void*)((void*)(p) + GET_SIZE(p)))
// PREV_CHUNK当且仅当上一个chunk是free chunk才能使用
#define PREV_CHUNK(p)   (((void*)(p) - GET_PREV_SIZE(p)))
#define FOOTER(p)       (NEXT_CHUNK(p) - WSIZE)
#define HD2MEM(p)       ((void*)((void*)(p) + WSIZE))
#define MEM2HD(p)       (FD2HD(p))


// debug专用宏定义
#define DBG(s) assert(s)
// 输出block里的数据(传入的是mem指针)
#define PRINT_BLOCK
#ifndef PRINT_BLOCK
#define PRINT_BLOCK(ptr, size, msg)        \
    printf("\n\n"msg":\n");                 \
    for(size_t i = 0; i < size; i++)        \
        printf("%02x ", *(((char*)ptr) + i));  \
    printf("\n\n");
#endif
/*
ChunkSize:
0x10-0x20       2^4 - 2^5
0x20-0x40       2^5
0x40-0x80       2^6
0x80-0x100      2^7
0x100-0x200     2^8
0x200-0x400     2^9
0x400-0x800     2^106
0x800-0x1000    2^11
0x1000-~        2^12
9 bins(void*)
*/
#define HEAP_LIST_NUM 9
void* heap_listp[HEAP_LIST_NUM][2];
// top_chunk指向的chunk不存在heap_listp中，同时其指向的chunk一定在堆的最高
void* top_chunk = NULL;


// 将chunk从原来的list中断开
void unlink_chunk(void* chunk)
{
    void* bk = BK(chunk);
    void* fd = FD(chunk);
    DBG((bk && fd) && "unlink_chunk: bk fd 不可能为空");
    FD(bk) = fd;
    BK(fd) = bk;
}

// 空闲内存合并
// 该函数会修改传入chunk的footer
static void* coalesce(void* chunk)
{
    int chunksize = GET_SIZE(chunk);
    DBG(chunksize >= MIN_CHUNKSIZE && "coalesce: chunk大小不可能小于最小值");

    int* newptr = chunk;
    // 向前合并
    if(GET_PREV_FREE(chunk))
    {
        // 被合并的那个chunk要从原来的list链上断开
        void* prevChunk = PREV_CHUNK(chunk);
        unlink_chunk(prevChunk);
        int prev_chunksize = GET_PREV_SIZE(chunk);

        chunksize += prev_chunksize;
        newptr = prevChunk;
    }
    // 向后合并（排除当前chunk是top_chunk
    if(!GET_ALLOC(NEXT_CHUNK(chunk)) && chunk != top_chunk && NEXT_CHUNK(chunk) != top_chunk)
    {
        chunksize += GET_SIZE(NEXT_CHUNK(chunk));
        unlink_chunk(NEXT_CHUNK(chunk));
    }

    // 重新设置chunk的header与footer。只设置了大小，没有修改任何标志位
    SET_SIZE(newptr, chunksize);
    SET_SIZE(FOOTER(newptr), chunksize);
    return newptr;
}


static void* extend_heap(size_t words)
{
    void* bp;
    size_t size;

    // DSIZE对齐
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    return bp;
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    // 初始化链表
    for(int i = 0; i < HEAP_LIST_NUM; i++)
    {
        void* headPtr = FD2HD(&heap_listp[i][0]);
        FD(headPtr) = headPtr;
        BK(headPtr) = headPtr;
    }
    // 将brk抬高4字节，这样每个chunk的用户空间就满足八字节对齐标准
    // 同时，设置top chunk
    int topSize = (CHUNKSIZE > MIN_CHUNKSIZE ? CHUNKSIZE : MIN_CHUNKSIZE);
    top_chunk = mem_sbrk(4 + topSize) + 4;
    SET_SIZE(top_chunk, topSize);
    SET_FREE(top_chunk);
    // 第一个chunk，设置前面的空间为不可合并
    SET_PREV_ALLOC(top_chunk);

    return 0;
}
// 在使用该函数前需要先设置size
void insertChunk2List(void* chunk)
{
    // 在插入前先对周围的内存块进行合并
    chunk = coalesce(chunk);
    DBG(!GET_ALLOC(chunk) && "insertChunk2List: 所插入的块，不可能是已经分配了的块");
    // 插入剩余的块到链表中
    int size = GET_SIZE(chunk);
    int listIdx = getListIndx(size);
    DBG(listIdx >= 0 && "insertChunk2List: listIdx出错，chunkSize可能存在问题");
    void* headPtr = FD2HD(&heap_listp[listIdx][0]);
    void * ptr = FD(headPtr);

    // 从list里的第一个chunk开始，从前到后遍历，遍历回list则停止
    // 注意，当list里一个chunk都没有时仍然成立
    while(ptr != headPtr && GET_SIZE(ptr) < size)
        ptr = FD(ptr);
    // 找到插入点
    FD(chunk) = ptr;
    BK(chunk) = BK(ptr);

    FD(BK(ptr)) = chunk;
    BK(ptr) = chunk;
}

// 传入的targetChunk必须是一块已经断开链表， 同时有意分配给用户的free chunk，
// 返回的是一块切剩下的空闲chunk, 其中已经设置相关的标志位
void* splitChunk(void* targetChunk, size_t targetChunksize)
{
    int current_chunkSize = GET_SIZE(targetChunk);
    int remainSize = current_chunkSize - targetChunksize;
    DBG(remainSize >= MIN_CHUNKSIZE && "splitChunk: chunk过小，无法切割");
    // SET_SIZE足够精巧，以至于能保存上一个chunk的INUSE_BIT
    SET_SIZE(targetChunk, targetChunksize);
    SET_ALLOC(targetChunk);
    // 设置剩余chunk的size
    void* remainChunk = NEXT_CHUNK(targetChunk);
    SET_SIZE(remainChunk, remainSize);
    SET_FREE(remainChunk);
    SET_PREV_ALLOC(remainChunk);
    
    SET_SIZE(FOOTER(remainChunk), remainSize);
    // 将剩余的chunk插入至适合的位置
    return remainChunk;
}
/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    void* targetMemPtr = NULL;
    int targetChunksize = request2chunksize(size);
    // 查找链表中是否存在需要的chunk
    for(int listIdx = getListIndx(targetChunksize); listIdx < HEAP_LIST_NUM; listIdx++)
    {
        // 如果当前遍历到的链表非空，并且该链条中最大的chunk不小于所申请空间的大小
        if(heap_listp[listIdx][0] != FD2HD(&heap_listp[listIdx][0]) 
            && GET_SIZE(heap_listp[listIdx][1]) >= targetChunksize)
        {
            // 遍历向下查找
            void* ptr = heap_listp[listIdx][0];
            while(GET_SIZE(ptr) < targetChunksize)
                ptr = FD(ptr);
            // 此时已经获取到了所需要的chunk，但需要判断是否分割处理
            int current_chunkSize = GET_SIZE(ptr);

            int remainSize = current_chunkSize - targetChunksize;
            // 先断开链表   
            unlink_chunk(ptr);
            if(remainSize >= MIN_CHUNKSIZE)
            {
                void* remainChunk = splitChunk(ptr, targetChunksize);
                insertChunk2List(remainChunk);
            }
            else
            {
                // 设置目标chunk的header
                SET_SIZE(ptr, current_chunkSize);
                SET_ALLOC(ptr);
                SET_PREV_ALLOC(NEXT_CHUNK(ptr));
            }
            // 返回用户空间
            DBG(GET_ALLOC(ptr) && "mm_malloc: bad alloc status");
            targetMemPtr = HD2MEM(ptr);
            return targetMemPtr;
        }
    }

    // 如果所有的链表中的chunk都不满足，则需要向top chunk申请
    DBG(top_chunk && "mm_malloc： top_chunk不可能为空");
    // 必须保证top_chunk在任何情况下都有空间
    if(GET_SIZE(top_chunk) >= targetChunksize + MIN_CHUNKSIZE)
    {
        void* ptr = top_chunk;
        top_chunk = splitChunk(ptr, targetChunksize);
        DBG(GET_ALLOC(ptr) && "mm_malloc: bad alloc status");
        return HD2MEM(ptr);
    }

    // 如果所有的chunk都不满足，则需要向brk申请
    int extend_size = (targetChunksize + MIN_CHUNKSIZE > CHUNKSIZE ? 
        targetChunksize + MIN_CHUNKSIZE : CHUNKSIZE);
    void* extend_ptr = extend_heap(extend_size / WSIZE);
    // 如果没内存了，则返回空
    if(!extend_ptr)
        return NULL;
    SET_SIZE(extend_ptr, extend_size);
    SET_FREE(extend_ptr);
    SET_PREV_ALLOC(extend_ptr);
    // 将旧的top chunk放回链表中
    DBG(GET_SIZE(top_chunk) >= MIN_CHUNKSIZE && "mm_malloc: chunk大小不可能小于最小值");
    insertChunk2List(top_chunk);
    top_chunk = extend_ptr;
    // 再次查找
    targetMemPtr = mm_malloc(size);
    //printf("address: %p, chunkSize: %x, reqSize: %x\n", 
      //  targetMemPtr, targetChunksize, size);
    return targetMemPtr;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    void* chunk = MEM2HD(bp);
    DBG(GET_ALLOC(chunk) && "mm_free: double Free detected");
    SET_FREE(chunk);
    SET_PREV_FREE(NEXT_CHUNK(chunk));
    DBG(GET_SIZE(chunk) >= MIN_CHUNKSIZE && "mm_free: chunk大小不可能小于最小值");
    insertChunk2List(chunk);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    DBG(GET_ALLOC(MEM2HD(ptr)) && "mm_realloc: double free / UAF detected");
    //return NULL;
    /*
        void* newp = mm_malloc(size);
        if(newp == NULL)
            return NULL;
        memcpy(newp, ptr, size);
        mm_free(ptr);
        return newp;
    */

   // oldptr 和 newptr始终指向chunk的header，而不是mem
    void* oldptr = MEM2HD(ptr);
    void* newptr = oldptr;
    size_t targetChunkSize = request2chunksize(size);
    // 确定要复制的字节数，避免越界读取
    size_t copySize = GET_SIZE(oldptr) - WSIZE;
    if (size < copySize)
        copySize = size;
    
    PRINT_BLOCK(ptr, copySize, "Before realloc:");

    // 先合并可能存在的空闲块, 并设置为已分配
    // 这里的合并操作不能使用coalesce，因为这个函数是面向空闲内存的，而oldptr并非空闲
    // newptr = coalesce(oldptr);

    int chunksize = GET_SIZE(oldptr);

    // 向前合并
    if(GET_PREV_FREE(oldptr))
    {
        void* prevChunk = PREV_CHUNK(oldptr);
        unlink_chunk(prevChunk);
        int prev_chunksize = GET_PREV_SIZE(oldptr);
        chunksize += prev_chunksize;
        newptr = prevChunk;
    }
    // 向后合并
    if(!GET_ALLOC(NEXT_CHUNK(oldptr)) && NEXT_CHUNK(oldptr) != top_chunk)
    {
        chunksize += GET_SIZE(NEXT_CHUNK(oldptr));
        unlink_chunk(NEXT_CHUNK(oldptr));
    }

    // 设置大小
    SET_SIZE(newptr, chunksize);
    // 设置被分配
    SET_ALLOC(newptr);

    // 如果合并后的大小仍然不够
    if(chunksize < targetChunkSize)
    {
        oldptr = newptr;
        // 分配一块新的
        void* newmem = mm_malloc(size);
        if (newmem == NULL)
            return NULL;
        newptr = MEM2HD(newmem);
        // 复制用户区域
        memcpy(newmem, HD2MEM(oldptr), copySize);
        // 同时释放之前那块
        mm_free(HD2MEM(oldptr));

        PRINT_BLOCK(HD2MEM(newptr), copySize, "new alloc:");
    }
    // 如果可以使用
    else
    {
        // 如果不是原来的内存
        if(newptr != oldptr)
        {
            // 复制用户区域
            PRINT_BLOCK(HD2MEM(newptr), copySize, "Before copy newptr :");
            PRINT_BLOCK(HD2MEM(oldptr), copySize, "Before copy oldptr :");
            // 注意这里需要使用memmove，而不是memcpy，因为新的chunk和原来的chunk之间会存在重叠（正常现象）
            memmove(HD2MEM(newptr), HD2MEM(oldptr), copySize);
            PRINT_BLOCK(HD2MEM(newptr), copySize, "After copy:");
        }
        if(chunksize >= targetChunkSize + MIN_CHUNKSIZE)
        {
            // 切割，将剩余的存到链表中
            void* remainChunk = splitChunk(newptr, targetChunkSize);
            PRINT_BLOCK(HD2MEM(newptr), copySize, "After Split:");
            insertChunk2List(remainChunk);
        }

        PRINT_BLOCK(HD2MEM(newptr), copySize, "After All:");
    }
    return HD2MEM(newptr);
}














