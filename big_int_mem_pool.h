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


#ifndef _BIG_INT_MEM_POOL_H_
#define _BIG_INT_MEM_POOL_H_

#define PAGE_SIZE (4096)

/* A block contains PAGE_SIZE * page_count */
struct mem_block
{
    unsigned char *data; /* data area, contains 'page_count' pages of memory */
    unsigned char *bitmap; /* bitmap to data area, takes (page_count / 8) bytes */
    unsigned int bitmap_size; /* size of space bitmap takes */
    size_t page_count; /* number of page in this block */

    size_t size_used;
    size_t size_free;
    size_t size_total;
};

/* Memory Pool data structure */
struct mem_pool
{
    struct mem_block *blocks;
    size_t block_count;

    size_t size_used;
    size_t size_free;
    size_t size_total;
};

typedef struct mem_block mem_block_t;
typedef struct mem_pool mem_pool_t;

mem_pool_t *mem_pool_new(size_t size, int fill_with_zero);
int mem_pool_destroy(mem_pool_t *pool);

void *mem_pool_malloc(mem_pool_t *pool, size_t size);
int mem_pool_free(mem_pool_t *pool, void *ptr);

#endif

