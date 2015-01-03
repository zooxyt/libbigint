/*
   Big Integer Library - Memory Pool
   Copyright (c) 2013-2014 Cheryl Natsu 
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
   3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
   IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
   IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
   NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
   THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
   */

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "big_int_mem_pool.h"

#define PAGE_PER_BLOCK (32)
#define BIT_PER_BITMAP (8)
#define BLOCK_DATA_SIZE (PAGE_SIZE * PAGE_PER_BLOCK)
#define BLOCK_BITMAP_SIZE (PAGE_PER_BLOCK / BIT_PER_BITMAP)
#define POOL_BLOCK_COUNT(size) (((size) / BLOCK_DATA_SIZE) + (((size) % BLOCK_DATA_SIZE != 0) ? 1 : 0))
#define POOL_BITMAP_SIZE(size) (POOL_BLOCK_COUNT(size) / BIT_PER_BITMAP)

/* Create a new Memory Block */
static int mem_block_init(mem_block_t *block, int fill_with_zero)
{
    if (block == NULL) return -1;

    block->page_count = PAGE_PER_BLOCK;
    block->data = NULL;
    block->bitmap = NULL;

    block->data = (unsigned char *)malloc(sizeof(unsigned char) * BLOCK_DATA_SIZE);
    if (block->data == NULL) goto fail;
    if (fill_with_zero) memset(block->data, 0, BLOCK_DATA_SIZE);
    block->bitmap_size = BLOCK_BITMAP_SIZE;
    block->bitmap = (unsigned char *)malloc(sizeof(unsigned char) * BLOCK_BITMAP_SIZE);
    if (block->bitmap == NULL) goto fail;
    memset(block->bitmap, 0, BLOCK_BITMAP_SIZE);

    block->size_used = 0;
    block->size_free = BLOCK_DATA_SIZE;
    block->size_total = BLOCK_DATA_SIZE;
    return 0;
fail:
    if (block->bitmap != NULL) { free(block->bitmap); block->bitmap = NULL; }
    if (block->data != NULL) { free(block->data); block->data = NULL; }
    return -1;
}

int mem_block_uninit(mem_block_t *block)
{
    if (block->bitmap != NULL) free(block->bitmap);
    if (block->data != NULL) free(block->data);
    return 0;
}

/* Create a new Memory Pool with specified size (in bytess) */
mem_pool_t *mem_pool_new(size_t size, int fill_with_zero)
{
    unsigned int idx = 0;
    mem_pool_t *pool = NULL;
    pool = (mem_pool_t *)malloc(sizeof(mem_pool_t));
    if (pool == NULL) goto fail;
	pool->blocks = NULL;
    pool->block_count = POOL_BLOCK_COUNT(size);
	pool->size_used = 0;
	size = pool->block_count * BLOCK_DATA_SIZE;
	pool->size_free = size;
	pool->size_total = size;
    pool->blocks = (mem_block_t *)malloc(sizeof(mem_block_t) * (pool->block_count));
    if (pool->blocks == NULL) goto fail;
    for (idx = 0; idx < pool->block_count; idx++)
    {
        pool->blocks[idx].bitmap = NULL;
        pool->blocks[idx].data = NULL;
    }
    for (idx = 0; idx < pool->block_count; idx++)
        if (mem_block_init(&pool->blocks[idx], fill_with_zero) != 0) goto fail;
    return pool;
fail:
    if (pool != NULL)
    {
        if (idx != 0)
        {
            if (pool->blocks != NULL)
            {
                while (idx > 0)
                {
                    mem_block_uninit(&pool->blocks[idx]);
                    idx--;
                }
                free(pool->blocks);
            }
        }
        free(pool);
    }
    return NULL;
}

int mem_pool_destroy(mem_pool_t *pool)
{
    unsigned int idx;
    for (idx = 0; idx < pool->block_count; idx++)
    {
        mem_block_uninit(&pool->blocks[idx]);
    }
    free(pool->blocks);
    free(pool);
    return 0;
}

/*
int is_page_dirty(char *p)
{
    char *endp = p + PAGE_SIZE;
    while (p != endp)
    {
        if (*p != 0) return 1;
        p++;
    }
    return 0;
}
*/

void *mem_pool_malloc(mem_pool_t *pool, size_t size)
{
    unsigned int block_idx, page_idx, bitmap_idx;
    unsigned int bit_idx;
    void *p = NULL;

    if (size > PAGE_SIZE) return NULL;
    for (block_idx = 0; block_idx < pool->block_count; block_idx++)
    {
        for (page_idx = 0; page_idx < pool->blocks[block_idx].page_count && pool->blocks[block_idx].size_free >= PAGE_SIZE; page_idx++)
        {
            for (bitmap_idx = 0; bitmap_idx < pool->blocks[block_idx].bitmap_size; bitmap_idx++)
            {
                if (pool->blocks[block_idx].bitmap[bitmap_idx] != 0xFF)
                {
                    bit_idx = (pool->blocks[block_idx].bitmap[bitmap_idx] ^ (pool->blocks[block_idx].bitmap[bitmap_idx] + 1)) >> 1;
                    bit_idx = (bit_idx & 0x55) + ((bit_idx>>1)&0x55);
                    bit_idx = (bit_idx & 0x33) + ((bit_idx>>2)&0x33);
                    bit_idx = (bit_idx & 0xf) + ((bit_idx>>4)&0xf);
                    
                    p = pool->blocks[block_idx].data + (bitmap_idx * BIT_PER_BITMAP + bit_idx) * PAGE_SIZE;

                    pool->blocks[block_idx].bitmap[bitmap_idx] |= (1 << (bit_idx));
                    pool->blocks[block_idx].size_used += PAGE_SIZE;
                    pool->blocks[block_idx].size_free -= PAGE_SIZE;
                    pool->size_used += PAGE_SIZE;
                    pool->size_free -= PAGE_SIZE;
                    return p;
                }
            }
        }
    }
    return NULL;
}

int mem_pool_free(mem_pool_t *pool, void *ptr)
{
    unsigned int block_idx, page_idx, bitmap_idx, bit_idx;

    for (block_idx = 0; block_idx < pool->block_count; block_idx++)
    {
        if (((size_t)pool->blocks[block_idx].data <= (size_t)ptr) && ((size_t)ptr < (size_t)pool->blocks[block_idx].data + BLOCK_DATA_SIZE))
        {
            page_idx = (((size_t)ptr) - ((size_t)pool->blocks[block_idx].data)) / PAGE_SIZE;
            bitmap_idx = page_idx / BIT_PER_BITMAP;
            bit_idx = page_idx % BIT_PER_BITMAP;

            pool->blocks[block_idx].bitmap[bitmap_idx] &= ~(1 << (bit_idx));
            pool->blocks[block_idx].size_used -= PAGE_SIZE;
            pool->blocks[block_idx].size_free += PAGE_SIZE;
            pool->size_used -= PAGE_SIZE;
            pool->size_free += PAGE_SIZE;
            return 0;
        }
    }
    return -1;
}

