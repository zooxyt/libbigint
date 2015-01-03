/*
   Big Integer Library
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
#include <stdio.h>
#include <string.h>

#include "big_int.h"
#include "big_int_rand.h"

#define ALLOCATE_SLOT_SIZE (256)

#define BIT_TO_BYTE_FLOOR(x) ((x)>>3)
#define BIT_TO_DBYTE_FLOOR(x) ((x)>>4)
#define BIT_TO_QBYTE_FLOOR(x) ((x)>>5)
#define BIT_TO_SLOT_FLOOR(x) ((x)>>5)

#define BIT_TO_BYTE(x) (((x)>>3)+((x)%8?1:0))
#define BIT_TO_DBYTE(x) (((x)>>4)+((x)%16?1:0))
#define BIT_TO_QBYTE(x) (((x)>>5)+((x)%32?1:0))
#define BIT_TO_SLOT(x) (((x)>>5)+((x)%32?1:0))

#define BYTE_TAIL(x) ((x)%8)
#define DBYTE_TAIL(x) ((x)%16)
#define QBYTE_TAIL(x) ((x)%32)
#define SLOT_TAIL(x) ((x)%32)

#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))

#define BIT_PER_BYTE (8)
#define BIT_PER_DBYTE (16)
#define BIT_PER_QBYTE (32)
#define BIT_PER_SLOT (32)

#define BIT_MASK_BYTE ((1<<(8))-1)
#define BIT_MASK_DBYTE ((1<<(16))-1)
#define BIT_MASK_QBYTE (((uint64_t)1<<(32))-1)
#define BIT_MASK_SLOT (((uint64_t)1<<(32))-1)

#define MUL_BYTE(x) ((x)<<3)
#define MUL_DBYTE(x) ((x)<<4)
#define MUL_QBYTE(x) ((x)<<5)
#define MUL_SLOT(x) ((uint64_t)(x)<<5)

#define BIT(x) (1<<(x))

/* memory pool for contain big integers */
#include "big_int_mem_pool.h"
static mem_pool_t *big_num_pool = NULL;

static inline int hbidx_16(unsigned int value);
static inline int hbidx_32(unsigned int value);

int big_int_mem_pool_initialize(size_t size)
{
    /* current not support reinitialize */
    if (big_num_pool != NULL) return 0; 
    big_num_pool = mem_pool_new(size, 1);
    return big_num_pool != NULL ? 0 : -1;
}

int big_int_mem_pool_uninitialize(void)
{
    int ret;
    ret = mem_pool_destroy(big_num_pool);
    big_num_pool = NULL;
    return ret;
}

inline static void *__big_int_mem_pool_malloc(size_t size, int *in_pool)
{
    void *p;
    *in_pool = 0;
    if ((big_num_pool != NULL) && (size <= PAGE_SIZE))
    {
        p = mem_pool_malloc(big_num_pool, size);
        if (p != NULL)
        {
            *in_pool = 1;
        }
        else
        {
            p = (void *)malloc(size);
        }
    }
    else
    {
        p = (void *)malloc(size);
    }
    return p;
}

inline static int __big_int_mem_pool_free(void *p, int in_pool)
{
    if ((big_num_pool != NULL) && in_pool)
    {
        mem_pool_free(big_num_pool, p);
    }
    else
    {
        free(p);
    }
    return 0;
}

inline static int __big_int_clean_slots(slot_t *slot, size_t slot_length)
{
    while (slot_length-- > 0)
    {
        *slot = 0;
        slot++;
    }
    return 0;
}

/* internal use only */
inline static big_int_t *__big_int_new(size_t bit_length)
{
    big_int_t *new_int = (big_int_t *)malloc(sizeof(big_int_t));
    if (new_int == NULL) return NULL;
    new_int->bit_length = bit_length;
    new_int->slot_length = BIT_TO_SLOT(bit_length);
    new_int->allocated_slot_length = MAX(new_int->slot_length, ALLOCATE_SLOT_SIZE); /* 256 bytes can contains 4096 bit int */
    new_int->sign = BIG_NUMBER_POSITIVE;
    new_int->slot = (slot_t *)__big_int_mem_pool_malloc(sizeof(slot_t) * new_int->allocated_slot_length, &new_int->in_pool);
    if (new_int->slot == NULL) 
    {
        free(new_int);
        return NULL;
    }
    return new_int;
}

/* internal use only */
inline static big_int_t *__big_int_new_zero(size_t bit_length)
{
    unsigned int idx;
    big_int_t *new_int = __big_int_new(MAX(bit_length, 1));

    if (new_int == NULL) return NULL;
    /* pages from pool are "clean" */
    if (new_int->in_pool == 0)
    {
        for (idx = 0; idx < new_int->allocated_slot_length; idx++) new_int->slot[idx] = 0;
    }
    return new_int;
}

/* generate a big int from unsigned int and specificed sign */
big_int_t *big_int_new_from_int(unsigned int value)
{
	big_int_t *new_int;
    int bit_length = BIT_PER_QBYTE - 1;

    while ((((value >> bit_length) & 0x1) == 0) && bit_length >= 0) bit_length--;
    bit_length++;
    new_int = __big_int_new_zero(bit_length);
    if (new_int == NULL) return NULL;
    /*if (bit_length > BIT_PER_DBYTE) new_int->slot[1] = (value >> BIT_PER_DBYTE) & ((1 << BIT_PER_DBYTE) - 1);*/
    /*new_int->slot[0] = value & ((1 << BIT_PER_DBYTE) - 1);*/
    new_int->slot[0] = value;
    return new_int;
}

/* generate a int with specified sign and value */
big_int_t *big_int_new_from_int_with_sign(int sign, unsigned int value)
{
    big_int_t *new_int = big_int_new_from_int(value);
    if (new_int != NULL)
    {
        new_int->sign = sign;
    }
    return new_int;
}

/* convert hex literal char to decimal int value */
int hex_to_int(char ch)
{
    if (ch >= '0' && ch <= '9') return (int)ch - '0';
    else if (ch >= 'a' && ch <= 'f') return (int)ch - 'a' + 10;
    else if (ch >= 'A' && ch <= 'F') return (int)ch - 'A' + 10;
    return 0;
}

inline int __trim_tail(slot_t *slot, size_t *slot_length, size_t *bit_length)
{
    /* OPTIMIZE ME */
    slot_t *slot_p = slot + *slot_length - 1;
    while ((*slot_length > 0) && (*slot_p == 0))
    {
        (*slot_length)--;
        slot_p--;
    }
    /*(*slot_length)++;*/
    *bit_length = MUL_SLOT((*slot_length - 1)) + hbidx_32(*slot_p);
    return 0;
}

/* generate a int from a string contains a hex int */
big_int_t *big_int_new_from_str(char *value)
{
    char *value_p = value, *value_endp = value_p + strlen(value_p);
    int bit_length = (value_endp - value_p) * 4;
    int slot_length = bit_length / BIT_PER_SLOT;
    big_int_t *new_int = __big_int_new(bit_length);
    int slot_idx;
    for (slot_idx = slot_length - 1; slot_idx >= 0; slot_idx--)
    {
        new_int->slot[slot_idx] = (hex_to_int(*value_p) << 28) |\
                                     (hex_to_int(*(value_p + 1)) << 24) |\
                                     (hex_to_int(*(value_p + 2)) << 20) |\
                                     (hex_to_int(*(value_p + 3)) << 16) |\
                                     (hex_to_int(*(value_p + 4)) << 12) |\
                                     (hex_to_int(*(value_p + 5)) << 8) |\
                                     (hex_to_int(*(value_p + 6)) << 4) |\
                                     (hex_to_int(*(value_p + 7)));
        value_p += 8;
    }
    __trim_tail(new_int->slot, &new_int->slot_length, &new_int->bit_length);
    return new_int;
}

/* judge the least int of bit to contain a value */
inline int hbidx_16(unsigned int value)
{
    if (value & 0xFF00)
        if (value & 0xF000)
            if (value & 0xC000)
                if (value & 0x8000) return 16;
                else return 15;
            else
                if (value & 0x2000) return 14;
                else return 13;
        else
            if (value & 0x0C00)
                if (value & 0x0800) return 12;
                else return 11;
            else
                if (value & 0x0200) return 10;
                else return 9;
    else
        if (value & 0xF0)
            if (value & 0xC0)
                if (value & 0x80) return 8;
                else return 7;
            else
                if (value & 0x20) return 6;
                else return 5;
        else
            if (value & 0x0C)
                if (value & 0x08) return 4;
                else return 3;
            else
                if (value & 0x02) return 2;
                else 
                    if (value != 0) return 1;
                    else return 0;
}

inline int hbidx_32(unsigned int value)
{
    int test = hbidx_16(value >> 16);
    if (test > 0) return test + 16; else return hbidx_16(value & 0xFFFF);
}

/* generate a random int with specified bit length */
big_int_t *big_int_new_random(size_t bit_length)
{
    big_int_t *new_int;
    int idx;

	new_int = __big_int_new(bit_length);
    if (new_int == NULL) return NULL;
    for (idx = 0; idx < (signed int)new_int->slot_length; idx++)
    {
        new_int->slot[idx] = rand_get_32bit();
        if (bit_length < BIT_PER_SLOT)
        {
            new_int->slot[idx] &= (1 << bit_length) - 1;
            break;
        }
        bit_length -= BIT_PER_SLOT;
    }
    for (idx = new_int->slot_length - 1; idx >= 0; idx--)
    {
        if (new_int->slot[idx] != 0)
        {
            new_int->bit_length -= BIT_PER_SLOT - hbidx_32(new_int->slot[idx]);
            break;
        }
        else new_int->slot_length--;
    }
    if (new_int->bit_length == 0) 
    {
        new_int->slot_length = 1;
        new_int->bit_length = 1;
    }
    return new_int;
}

/* Print the value of big int with heximal format */
int big_int_print(const big_int_t *big_int)
{
    int idx;
    if (big_int->sign == BIG_NUMBER_NEGATIVE) printf("-");
    printf("0x");
    for (idx = big_int->slot_length - 1; idx >= 0; idx--)
    {
        printf("%08X", big_int->slot[idx]);
    }
    return 0;
}

/* Print the value of big int in heximal format and it's bit length */
int big_int_print_detail(const big_int_t *big_int)
{
    big_int_print(big_int);
    printf("(slot_len=%u,bit_len=%u)", (unsigned int)big_int->slot_length, (unsigned int)big_int->bit_length);
    return 0;
}

/* Destroy a big int */
int big_int_destroy(big_int_t *big_int)
{
    unsigned int slot_idx;

    if (big_int == NULL) return -1;
    /* erase 'dirty' slots before return it to pool */
    for (slot_idx = 0; slot_idx != big_int->slot_length; slot_idx++) big_int->slot[slot_idx] = 0;
    __big_int_mem_pool_free(big_int->slot, big_int->in_pool);
    free(big_int);
    return 0;
}

/* 1  : num1 > num2;
 * 0  : num1 = num2;
 * -1 : num1 < num2;
 * -2 : error; */

/* Compare the value part of two big integers (ignore sign) */
inline int big_int_compare_raw(big_int_t *num1, big_int_t *num2)
{
    int slot_idx;
    if (num1->bit_length > num2->bit_length) return 1;
    else if (num1->bit_length < num2->bit_length) return -1;
    else
    {
        slot_idx = num1->slot_length - 1;
        while (slot_idx >= 0)
        {
            if (num1->slot[slot_idx] > num2->slot[slot_idx]) return 1;
            else if (num1->slot[slot_idx] < num2->slot[slot_idx]) return -1;
            slot_idx--;
        }
        return 0;
    }
}

/* compare two big integers */
inline int big_int_compare(big_int_t *num1, big_int_t *num2)
{
    if (num1->sign == BIG_NUMBER_POSITIVE)
    {
        if (num2->sign == BIG_NUMBER_POSITIVE)
            return big_int_compare_raw(num1, num2);
        else
            return 1; /* + > - */
    }
    else
    {
        if (num2->sign == BIG_NUMBER_NEGATIVE)
            return big_int_compare_raw(num2, num1);
        else
            return -1;
    }
}

/* generate int with specified existent int */
big_int_t *big_int_assign(big_int_t *num)
{
    unsigned int idx;
    big_int_t *new_num;

	new_num = (big_int_t *)malloc(sizeof(big_int_t));
    if (new_num == NULL) return NULL;
    new_num->slot = (slot_t *)__big_int_mem_pool_malloc(sizeof(slot_t) * num->allocated_slot_length, &new_num->in_pool);
    if (new_num->slot == NULL)
    {
        free(new_num);
        return NULL;
    }
    for (idx = 0; idx < num->slot_length; idx++) new_num->slot[idx] = num->slot[idx];
    /* pages from pool are "clean" */
    if (new_num->in_pool == 0)
    {
        for (;idx < num->allocated_slot_length; idx++) new_num->slot[idx] = 0;
    }
    new_num->allocated_slot_length = num->allocated_slot_length;
    new_num->bit_length = num->bit_length;
    new_num->slot_length = num->slot_length;
    new_num->sign = num->sign;
    return new_num;
}

/* copy value between big integers */
int big_int_assign_to(big_int_t *num_dst, big_int_t *num_src)
{
    int idx;
    if (num_dst->allocated_slot_length < num_src->slot_length)
    {
        /* no enough space, realloc */
        __big_int_clean_slots(num_dst->slot, num_dst->slot_length);
        __big_int_mem_pool_free(num_dst->slot, num_dst->in_pool);
        num_dst->slot = (slot_t *)__big_int_mem_pool_malloc(sizeof(slot_t) * num_src->allocated_slot_length, &num_dst->in_pool);
        if (num_dst->slot == NULL)
        {
            return -1;
        }
        num_dst->allocated_slot_length = num_src->allocated_slot_length;
        /* pages from pool are "clean" */
        if (num_dst->in_pool == 0)
        {
            for (idx = num_src->slot_length;idx < (signed int)num_dst->allocated_slot_length; idx++) num_dst->slot[idx] = 0;
        }
    }
    else
    {
        for (idx = num_src->slot_length; idx < (signed int)num_dst->slot_length; idx++) num_dst->slot[idx] = 0;
    }
    for (idx = 0; idx < (signed int)num_src->slot_length; idx++) num_dst->slot[idx] = num_src->slot[idx];
    num_dst->bit_length = num_src->bit_length;
    num_dst->slot_length = num_src->slot_length;
    num_dst->sign = num_src->sign;
    return 0;
}

/************************SEPERATOR*************************/

/* 'raw' version of add */
/* add value of num2 into num1 */
int big_int_add_to_raw(big_int_t *num1, big_int_t *num2)
{
    unsigned int neccessary_bit, neccessary_slot, operation_slot_length;
    slot_t *new_slot;
    int new_in_pool;
    int slot_idx;
    int carry;
    /*int new_carry;*/
    uint64_t tmp;

    neccessary_bit = MAX(num1->bit_length, num2->bit_length) + 1;
    neccessary_slot = BIT_TO_SLOT(neccessary_bit);
    /* allocated space size check */
    if (neccessary_slot > num1->allocated_slot_length)
    {
        /* allocate space for new slot */
        new_slot = (slot_t *)__big_int_mem_pool_malloc(sizeof(slot_t) * (neccessary_slot + ALLOCATE_SLOT_SIZE), &new_in_pool);
        if (new_slot == NULL) return -1;
        /* restore data to new slot */
        for (slot_idx = 0; slot_idx < (signed int)num1->slot_length; slot_idx++) new_slot[slot_idx] = num1->slot[slot_idx];
        if (new_in_pool == 0)
        {
            for (slot_idx = num1->slot_length; slot_idx < (signed int)neccessary_slot + ALLOCATE_SLOT_SIZE; slot_idx++) new_slot[slot_idx] = 0;
        }
        num1->allocated_slot_length = neccessary_slot + ALLOCATE_SLOT_SIZE;
        /* updata slot */
        __big_int_clean_slots(num1->slot, num1->slot_length);
        __big_int_mem_pool_free(num1->slot, num1->in_pool);
        num1->slot = new_slot;
        num1->in_pool = new_in_pool;
    }
    /* add operation */
    operation_slot_length = MAX(num1->slot_length, num2->slot_length);
    carry = 0;
    for (slot_idx = 0; slot_idx < (signed int)operation_slot_length; slot_idx++)
    {
        tmp = (uint64_t)(num1->slot[slot_idx]) + num2->slot[slot_idx] + carry;
        carry = tmp >> BIT_PER_SLOT;
        num1->slot[slot_idx] = tmp & BIT_MASK_SLOT;
    }
    num1->slot[slot_idx] = carry;
    num1->slot_length = slot_idx + carry;
    num1->bit_length = (MUL_SLOT(num1->slot_length - 1)) + hbidx_32(num1->slot[num1->slot_length - 1]);
    if (num1->bit_length == 0) num1->bit_length = 1;
    return 0;
}

/* 'raw' version of substance, num1 >= num2 requires */
int big_int_sub_to_raw(big_int_t *num1, big_int_t *num2)
{
    slot_t *new_slot;
    int new_in_pool;
    int slot_idx;
    unsigned int carry, new_carry;
    if (num2->slot_length == 1 && num2->slot[0] == 0) 
    {
        /* X - 0 = X */
        return 0;
    }
    else if (big_int_compare(num1, num2) == 0)
    {
        /* X - X = 0 */
        for (slot_idx = 0; slot_idx < (signed int)num1->slot_length; slot_idx++)
            num1->slot[slot_idx] = 0;
        num1->slot_length = 1;
        num1->bit_length = 1;
        return 0;
    }
    if (num2->slot_length + 1 > num1->allocated_slot_length)
    {
        /* Extend num1 */
        new_slot = (slot_t *)__big_int_mem_pool_malloc(sizeof(slot_t) * (num2->allocated_slot_length + ALLOCATE_SLOT_SIZE), &new_in_pool);
        if (new_slot == NULL) return -1;
        for (slot_idx = 0; slot_idx != (signed int)num1->slot_length; slot_idx++) new_slot[slot_idx] = num1->slot[slot_idx];
        if (new_in_pool == 0)
        {
            for (; slot_idx != (signed int)num2->allocated_slot_length + ALLOCATE_SLOT_SIZE; slot_idx++) new_slot[slot_idx] = 0;
        }
        __big_int_clean_slots(num1->slot, num1->slot_length);
        __big_int_mem_pool_free(num1->slot, num1->in_pool);
        num1->slot = new_slot;
        num1->in_pool = new_in_pool;
        num1->allocated_slot_length = num2->allocated_slot_length + ALLOCATE_SLOT_SIZE;
    }
    carry = 0;
    for (slot_idx = 0; slot_idx != (signed int)num2->slot_length; slot_idx++)
    {
        new_carry = (num1->slot[slot_idx] >= ((uint64_t)num2->slot[slot_idx] + carry)) ? 0 : 1;
        num1->slot[slot_idx] = (uint64_t)num1->slot[slot_idx] + ((uint64_t)new_carry << BIT_PER_SLOT) - (num2->slot[slot_idx] + carry);
        carry = new_carry;
    }
    if (carry == 1)
    {
        for (; slot_idx != (signed int)num1->slot_length; slot_idx++)
        {
            new_carry = (num1->slot[slot_idx] >= carry) ? 0 : 1;
            num1->slot[slot_idx] = (uint64_t)num1->slot[slot_idx] + ((uint64_t)new_carry << BIT_PER_SLOT) - (carry);
            carry = new_carry;
        }
    }
    slot_idx = num1->slot_length - 1;
    while (num1->slot[slot_idx] == 0 && slot_idx >= 0) {slot_idx--;}
    if (slot_idx == -1)
    {
        /* result = 0 */
        num1->slot_length = 1;
        num1->bit_length = 1;
    }
    else
    {
        num1->slot_length = slot_idx + 1;
        num1->bit_length = MUL_SLOT(num1->slot_length - 1) + hbidx_32(num1->slot[num1->slot_length - 1]);
    }
    return 0;
}

/* judge if a int is zero */
inline int big_int_is_zero(big_int_t *num)
{
    return (num->slot_length == 1 && num->slot[0] == 0) ? 1 : 0;
}

int big_int_add_to(big_int_t *num1, big_int_t *num2)
{
    int ret;
    int sign;
    big_int_t *temp = NULL;
    if (num1->sign == BIG_NUMBER_POSITIVE)
    {
        if (num2->sign == BIG_NUMBER_POSITIVE)
        {
            /* '+' + '+' = '+' */
            sign = BIG_NUMBER_POSITIVE;
            ret = big_int_add_to_raw(num1, num2);
        }
        else
        {
            /* '+' + '-' = '+' - ABS('-') */
            if (big_int_compare_raw(num1, num2) > 0)
            {
                sign = BIG_NUMBER_POSITIVE;
                ret =  big_int_sub_to_raw(num1, num2);
            }
            else
            {
                sign = BIG_NUMBER_NEGATIVE;
                temp = big_int_assign(num2);
                if (temp == NULL) return -1;
                ret =  big_int_sub_to_raw(temp, num1);
                __big_int_clean_slots(num1->slot, num1->slot_length);
                __big_int_mem_pool_free(num1->slot, num1->in_pool);
                num1->slot = temp->slot;
                num1->slot_length = temp->slot_length;
                num1->allocated_slot_length = temp->allocated_slot_length;
                num1->bit_length = temp->bit_length;
                num1->sign = temp->sign;
                num1->in_pool = temp->in_pool;
                free(temp); temp = NULL;
            }
        }
    }
    else
    {
        if (num2->sign == BIG_NUMBER_NEGATIVE)
        {
            /* '-' + '-' = '-' */
            sign = BIG_NUMBER_NEGATIVE;
            ret = big_int_add_to_raw(num1, num2);
        }
        else
        {
            /* '-' + '+' =  */
            if (big_int_compare_raw(num1, num2) > 0)
            {
                sign = BIG_NUMBER_NEGATIVE;
                ret = big_int_sub_to_raw(num1, num2);
            }
            else
            {
                sign = BIG_NUMBER_POSITIVE;
                temp = big_int_assign(num2);
                if (temp == NULL) return -1;
                ret =  big_int_sub_to_raw(temp, num1);
                __big_int_clean_slots(num1->slot, num1->slot_length);
                __big_int_mem_pool_free(num1->slot, num1->in_pool);
                num1->slot = temp->slot;
                num1->slot_length = temp->slot_length;
                num1->allocated_slot_length = temp->allocated_slot_length;
                num1->bit_length = temp->bit_length;
                num1->sign = temp->sign;
                num1->in_pool = temp->in_pool;
                free(temp); temp = NULL;
            }
        }
    }
    num1->sign = sign;
    /* Zero check */
    if (big_int_is_zero(num1)) num1->sign = BIG_NUMBER_POSITIVE;
    return ret;
}

int big_int_sub_to(big_int_t *num1, big_int_t *num2)
{
    int ret = 0;
    int sign;
    big_int_t *temp = NULL;

    if (num1->sign == BIG_NUMBER_POSITIVE)
    {
        if (num2->sign == BIG_NUMBER_POSITIVE)
        {
            /* '+' - '+' = */
            if (big_int_compare_raw(num1, num2) > 0)
            {
                sign = BIG_NUMBER_POSITIVE;
                ret = big_int_sub_to_raw(num1, num2);
            }
            else
            {
                sign = BIG_NUMBER_NEGATIVE;
                temp = big_int_assign(num2);
                if (temp == NULL) return -1;
                ret = big_int_sub_to_raw(temp, num1);
                __big_int_clean_slots(num1->slot, num1->slot_length);
                __big_int_mem_pool_free(num1->slot, num1->in_pool);
                num1->slot = temp->slot;
                num1->slot_length = temp->slot_length;
                num1->allocated_slot_length = temp->allocated_slot_length;
                num1->bit_length = temp->bit_length;
                num1->sign = temp->sign;
                num1->in_pool = temp->in_pool;
                free(temp); temp = NULL;
            }
        }
        else
        {
            /* '+' - '-' = */
            sign = BIG_NUMBER_POSITIVE;
            ret = big_int_add_to_raw(num1, num2);
        }
    }
    else
    {
        if (num2->sign == BIG_NUMBER_POSITIVE)
        {
            /* '-' - '+' = */
            sign = BIG_NUMBER_NEGATIVE;
            ret = big_int_add_to_raw(num1, num2);
        }
        else
        {
            /* '-' - '-' = */
            if (big_int_compare_raw(num1, num2) > 0)
            {
                sign = BIG_NUMBER_NEGATIVE;
                ret = big_int_add_to_raw(num1, num2);
            }
            else
            {
                sign = BIG_NUMBER_POSITIVE;
                temp = big_int_assign(num2);
                if (temp == NULL) return -1;
                ret = big_int_sub_to_raw(temp, num1);
                __big_int_clean_slots(num1->slot, num1->slot_length);
                __big_int_mem_pool_free(num1->slot, num1->in_pool);
                num1->slot = temp->slot;
                num1->slot_length = temp->slot_length;
                num1->allocated_slot_length = temp->allocated_slot_length;
                num1->bit_length = temp->bit_length;
                num1->sign = temp->sign;
                num1->in_pool = temp->in_pool;
                free(temp); temp = NULL;
            }
        }
    }
    num1->sign = sign;
    /* Zero check */
    if (big_int_is_zero(num1)) num1->sign = BIG_NUMBER_POSITIVE;
    return ret;
}

inline big_int_t *__big_int_mul_plain(big_int_t *num1, big_int_t *num2);
inline int __big_int_mul_karatsuba_split(big_int_t *num, unsigned int shift, big_int_t **high, big_int_t **low);
inline big_int_t *__big_int_mul_karatsuba(big_int_t *num1, big_int_t *num2);
inline big_int_t *__big_int_mul_without_check(big_int_t *num1, big_int_t *num2);

inline void __u64_add(uint64_t *carry, uint64_t *sum, uint64_t num1, uint64_t num2)
{
    uint64_t num1_high = num1 >> 32, num1_low = num1 & 0xFFFFFFFF;
    uint64_t num2_high = num2 >> 32, num2_low = num2 & 0xFFFFFFFF;
    uint64_t sum_low = num1_low + num2_low;
    uint64_t carry_low = (sum_low) >> 32;
    uint64_t sum_high = num1_high + num2_high + carry_low;
    uint64_t carry_high = (sum_high) >> 32;
    sum_low &= 0xFFFFFFFF;
    sum_high &= 0xFFFFFFFF;
    *carry = carry_high;
    *sum = sum_high << 32 | sum_low;
}

inline void __u64_shift_1(uint64_t *carry, uint64_t *result, uint64_t num)
{
    *carry = num >> 63;
    *result = num << 1;
}

/* Fast squaring algorithm implementation
 * described in <<Handbook of Applied Cryptography>> Chapter 14.2.4 */
inline big_int_t *__big_int_square_plain(big_int_t *x)
{
    int i, j;
    int idx;
    big_int_t *w = NULL;
    unsigned int *c_fix_arr = NULL;
    uint64_t uv, c;
    uint64_t carry, sum, tmp_carry, tmp_sum;
    uint64_t u, v;
    int t = x->slot_length;
    /* Create w for containing the result */
    w = __big_int_new_zero(MUL_SLOT(t << 1));
    if (w == NULL) goto fail;
    c_fix_arr = (unsigned int *)malloc(sizeof(unsigned int) * (t << 1));
    if (c_fix_arr == NULL) goto fail;
    for (idx = 0; idx != t << 1; idx++) c_fix_arr[idx] = 0;

    /* For i from 0 to (t - 1) */
    for (i = 0; i < t; i++)
    {
        /* Step 1 */
        /* (uv)b <- w[2i] + x[i]*x[i] */
        uv = ((uint64_t)w->slot[i << 1] | ((uint64_t)c_fix_arr[i << 1] << BIT_PER_SLOT)) + (uint64_t)x->slot[i] * x->slot[i];
        u = uv >> BIT_PER_SLOT;
        v = uv & 0xFFFFFFFF;
        /* w[2i] <- v */
        w->slot[i << 1] = v & 0xFFFFFFFF;
        c_fix_arr[i << 1] = 0;
        /* c <- u */
        c = u;
        /* Step 2 */
        for (j = i + 1; j < t; j++)
        {
            /* (uv)b <- w[i+j] + 2 * xj * xi + c */
            carry = sum = 0;
            __u64_shift_1(&carry, &sum, ((uint64_t)x->slot[j] * x->slot[i]));
            __u64_add(&tmp_carry, &tmp_sum, w->slot[i + j] | ((uint64_t)c_fix_arr[i + j] << BIT_PER_SLOT), sum);
            carry += tmp_carry;
            __u64_add(&tmp_carry, &sum, tmp_sum, c);
            carry += tmp_carry;
            u = (sum >> BIT_PER_SLOT) | (carry << BIT_PER_SLOT);
            v = sum & 0xFFFFFFFF;
            /* w[i+j] <- v */
            w->slot[i + j] = v & 0xFFFFFFFF;
            c_fix_arr[i + j] = 0;
            /* c <- u */
            c = u;
        }
        /* Step 3 */
        /* w[i+t] <- u */
        w->slot[i + t] = u & 0xFFFFFFFF;
        c_fix_arr[i + t] = u >> BIT_PER_SLOT;
    }
    i = w->slot_length - 1;
    while ((i >= 0) && (w->slot[i] == 0)) i--;
    if (i == -1)
    {
        /* Zero */
        w->bit_length = 1;
        w->slot_length = 1;
    }
    else
    {
        w->slot_length = i + 1;
        w->bit_length = MUL_SLOT(i) + hbidx_32(w->slot[i]);
    }
fail:
    if (c_fix_arr != NULL) free(c_fix_arr);
    return w;
}

inline big_int_t *__big_int_mul_plain(big_int_t *num1, big_int_t *num2)
{
    big_int_t *num_term, *num_final;
    int num1_slot_idx, num2_slot_idx, num_term_slot_idx;
    int num_term_slot_idx_max; /* the maximum value of term value on last calculation, for cleaning the term */
    unsigned int neccessary_bit;
    unsigned int carry;
    uint64_t tmp;

    /* squaring */
    if (num1 == num2) 
    {
        return __big_int_square_plain(num1);
    }

    /* create new int for containing then result */
    neccessary_bit = num1->bit_length + num2->bit_length;
    num_term = __big_int_new_zero(neccessary_bit);
    if (num_term == NULL) return NULL;
    num_final = big_int_new_from_int(0);
    if (num_final == NULL) 
    {
        big_int_destroy(num_term);
        return NULL;
    }
    num_term_slot_idx_max = 0;
    /* multiply Operation */
    for (num1_slot_idx = 0; num1_slot_idx != (signed int)num1->slot_length; num1_slot_idx++)
    {
        /* clear term */
        for (num_term_slot_idx = 0; num_term_slot_idx != num_term_slot_idx_max; num_term_slot_idx++)
            num_term->slot[num_term_slot_idx] = 0;

        if (num1->slot[num1_slot_idx] != 0)
        {
            carry = 0;
            num_term_slot_idx = num1_slot_idx;
            for (num2_slot_idx = 0; num2_slot_idx != (signed int)num2->slot_length; num2_slot_idx++, num_term_slot_idx++)
            {
                tmp = (uint64_t)num1->slot[num1_slot_idx] * num2->slot[num2_slot_idx] + carry;
                carry = tmp >> BIT_PER_SLOT;
                num_term->slot[num_term_slot_idx] = tmp & 0xFFFFFFFF;
            }
            num_term->slot_length = num_term_slot_idx;

            num_term->slot[num_term_slot_idx] = carry;
            num_term->slot_length += !(!carry); /* need one more slot if carry greater then zero */
            num_term->bit_length = MUL_SLOT(num_term->slot_length - 1) + hbidx_32(num_term->slot[num_term->slot_length - 1]);

            num_term_slot_idx_max = num_term->slot_length;
            /* Add term into final */
            if (big_int_add_to(num_final, num_term) != 0)
            {
                big_int_destroy(num_term);
                big_int_destroy(num_final);
                return NULL;
            }
        }
    }

    big_int_destroy(num_term);

    /* Reture result */
    return num_final;
}

inline int __big_int_mul_karatsuba_split(big_int_t *num, unsigned int shift, big_int_t **high, big_int_t **low)
{
    int ret = 0;
    unsigned int slot_idx, slot_idx_2;
    unsigned int high_bit_length, low_bit_length;
    big_int_t *new_low, *new_high;

	if (shift == 0) return -1;
    if ((signed int)num->bit_length - (signed int)shift <= 0)
    {
        high_bit_length = 0;
    }
    else
    {
        high_bit_length = num->bit_length - shift;
    }
    low_bit_length = shift;

    new_low = __big_int_new_zero(low_bit_length);
    if (high_bit_length == 0)
        new_high = big_int_new_from_int(0);
    else
        new_high = __big_int_new_zero(high_bit_length);
    if (new_low == NULL || new_high == NULL) goto fail;

    /* Low part */
    for (slot_idx = 0; slot_idx < shift / BIT_PER_SLOT; slot_idx++)
        new_low->slot[slot_idx] = num->slot[slot_idx];
    slot_idx--;
    while (new_low->slot[slot_idx] == 0 && slot_idx > 0) slot_idx--;
    slot_idx++;
    new_low->slot_length = slot_idx;
    new_low->bit_length = MUL_SLOT(new_low->slot_length - 1) + hbidx_32(new_low->slot[new_low->slot_length - 1]);
    if (new_low->bit_length == 0) new_low->bit_length = 1;

    /* High part */
    if (high_bit_length != 0)
    {
        slot_idx = shift / BIT_PER_SLOT;
        slot_idx_2 = 0;
        for (; slot_idx < num->slot_length; slot_idx++, slot_idx_2++)
            new_high->slot[slot_idx_2] = num->slot[slot_idx];
        slot_idx_2--;
        while (new_high->slot[slot_idx_2] == 0 && slot_idx_2 > 0) slot_idx_2--;
        slot_idx_2++;
        new_high->slot_length = slot_idx_2;
        new_high->bit_length = MUL_SLOT(new_high->slot_length - 1) + hbidx_32(new_high->slot[new_high->slot_length - 1]);
        if (new_high->bit_length == 0) new_high->bit_length = 1;
    }

    *high = new_high; *low = new_low;
    ret = 0;
fail:
    if (ret != 0)
    {
        if (new_low != NULL) big_int_destroy(*high);
        if (new_high != NULL) big_int_destroy(*low);
    }
    return ret;
}

/* Karatsuba multiplication 
 * Described in http://en.wikipedia.org/wiki/Karatsuba_algorithm 
 */
inline big_int_t *__big_int_mul_karatsuba(big_int_t *num1, big_int_t *num2)
{
    big_int_t *x, *y; /* x * y */
    big_int_t *x1 = NULL, *x0 = NULL, *y1 = NULL, *y0 = NULL;
    big_int_t *z0 = NULL, *z1 = NULL, *z2 = NULL;
    big_int_t *t0 = NULL, *t1 = NULL;
    big_int_t *result = NULL;
    int b;

    /* Make multiplier(y) not shorter than multiplicand(x) */
    if (num1->bit_length > num2->bit_length) {x = num2; y = num1;}
    else {x= num1; y= num2;}

    /* Multiplicand is too shorter, use plain multipliy instead */
    if ((x->bit_length << 1) < y->bit_length)
    {
        return __big_int_mul_plain(num1, num2);
    }

    /* Split high and low part */
    b = y->bit_length >> 1;
    b = (b | 31) + 1; /* Fill 8 lowest bits */
    /*b = (b | 15) + 1; *//* Fill 4 lowest bits */
    if (__big_int_mul_karatsuba_split(x, b, &x1, &x0) != 0) goto fail;
    if (num1 == num2)
    {
        y1 = x1;
        y0 = x0;
    }
    else if (__big_int_mul_karatsuba_split(y, b, &y1, &y0) != 0) goto fail;

    /* High part multiplication (z2 = x1 * y1) */
    z2 = __big_int_mul_without_check(x1, y1);

    /* Low part multiplication (z0 = x0 * y0) */
    z0 = __big_int_mul_without_check(x0, y0);

    /* (z1 = z2 + z0 - (x1 - x0)(y1 - y0)) */

    /* Step 1: t0 = x1 - x0 */
    if ((t0 = big_int_assign(x1)) == NULL) goto fail;
    if (big_int_sub_to(t0, x0) != 0) goto fail;

    /* Step 2 t1 = y1 - y0 */
    if (num1 == num2)
    {
        t1 = t0;
    }
    else
    {
        if ((t1 = big_int_assign(y1)) == NULL) goto fail;
        if (big_int_sub_to(t1, y0) != 0) goto fail;
    }

    /* Step3 z1 = z2 + z0 - t0 * t1 */

    if (big_int_mul_to(t0, t1) != 0) goto fail;

    if ((z1 = big_int_assign(z2)) == NULL) goto fail;
    if (big_int_add_to(z1, z0) != 0) goto fail;
    if (big_int_sub_to(z1, t0) != 0) goto fail;

    /* Z = (z2 << (b << 1)) + (z1 << b) * (z0) */
    big_int_left_shift(z2, b << 1);

    big_int_left_shift(z1, b);
    if ((result = big_int_assign(z2)) == NULL) goto fail;
    if (big_int_add_to(result, z1) != 0) 
    {
        big_int_destroy(result); result = NULL;
        goto fail;
    }
    if (big_int_add_to(result, z0) != 0) 
    {
        big_int_destroy(result); result = NULL;
        goto fail;
    }

fail:
    if (x0 != NULL) big_int_destroy(x0);
    if (x1 != NULL) big_int_destroy(x1);
    if (z0 != NULL) big_int_destroy(z0);
    if (z1 != NULL) big_int_destroy(z1);
    if (z2 != NULL) big_int_destroy(z2);
    if (t0 != NULL) big_int_destroy(t0);
    if (num1 != num2)
    {
        if (t1 != NULL) big_int_destroy(t1);
        if (y0 != NULL) big_int_destroy(y0);
        if (y1 != NULL) big_int_destroy(y1);
    }
    return result;
}

/* [Invocation graph] 
 *   |        |
 *   v        v
 * mul_to -> mul -> mul_without_check -> mul_plain
 *   ----------------------^          -> mul_karatsuba */

/* mul function invoked by mul_to without check (check has been done by mul_to) */
#define BIG_NUMBER_MUL_KARATSUBA_THRESHOLD 768
inline big_int_t *__big_int_mul_without_check(big_int_t *num1, big_int_t *num2)
{
    int sign = (num1->sign == num2->sign) ? BIG_NUMBER_POSITIVE : BIG_NUMBER_NEGATIVE;
    big_int_t *result = NULL;

    if (MIN(num1->bit_length, num2->bit_length) > BIG_NUMBER_MUL_KARATSUBA_THRESHOLD)
    {
        result = __big_int_mul_karatsuba(num1, num2);
    }
    else
    {
        result = __big_int_mul_plain(num1, num2);
    }
    if (result != NULL)
    {
        /* Sign */
        result->sign = sign;
        /* Zero check */
        if (big_int_is_zero(result)) num1->sign = BIG_NUMBER_POSITIVE;
    }
    return result;
}

/* Multiplication interface 1 */
big_int_t *big_int_mul(big_int_t *num1, big_int_t *num2)
{
    if (num1->slot_length == 1 && num1->slot[0] == 0)
    {
        /* 0 * X = 0 */
        return big_int_new_from_int(0);
    }
    else if (num1->slot_length == 1 && num1->slot[0] == 1)
    {
        /* 1 * X == X */
        return big_int_assign(num2);
    }
    else if (num2->slot_length == 1 && num2->slot[0] == 1)
    {
        /* X * 1 == X */
        return big_int_assign(num1);
    }
    else if (num2->slot_length == 1 && num2->slot[0] == 0)
    {
        /* X * 0 == 0 */
        return big_int_new_from_int(0);
    }

    return __big_int_mul_without_check(num1, num2);
}

/* Multiplication interface 2 */
int big_int_mul_to(big_int_t *num1, big_int_t *num2)
{
    unsigned int num1_slot_idx;
    big_int_t *num_final;

    if (num1->slot_length == 1 && num1->slot[0] == 0)
    {
        /* 0 * X = 0 */
        return 0;
    }
    else if ((num1 != num2) && (num1->slot_length == 1 && num1->slot[0] == 1 && num1->sign == BIG_NUMBER_POSITIVE))
    {
        /* 1 * X == X */
        if (num1->allocated_slot_length < num2->slot_length) /* No enough space */
        {
            __big_int_clean_slots(num1->slot, num1->slot_length);
            __big_int_mem_pool_free(num1->slot, num1->in_pool);
            num1->slot = (slot_t *)__big_int_mem_pool_malloc(sizeof(slot_t) * num2->allocated_slot_length, &num1->in_pool);
            if (num1->slot == NULL) return -1;
            num1->bit_length = 1;
            num1->allocated_slot_length = num2->allocated_slot_length;
            num1->slot_length = 1;
            if (num1->in_pool == 0)
            {
                for (num1_slot_idx = 0; num1_slot_idx < num1->allocated_slot_length; num1_slot_idx++) num1->slot[num1_slot_idx] = 0;
            }
            num1->slot[0] = 1;
        }
        for (num1_slot_idx = 0; num1_slot_idx != num2->slot_length; num1_slot_idx++) 
            num1->slot[num1_slot_idx] = num2->slot[num1_slot_idx];
        for (; num1_slot_idx < num1->slot_length; num1_slot_idx++) 
            num1->slot[num1_slot_idx] = 0;
        num1->bit_length = num2->bit_length;
        num1->slot_length = num2->slot_length;
        num1->sign = num2->sign;
        return 0;
    }
    else if (num2->slot_length == 1 && num2->slot[0] == 1 && num2->sign == BIG_NUMBER_POSITIVE)
    {
        /* X * 1 == X */
        return 0;
    }
    else if (num2->slot_length == 1 && num2->slot[0] == 0)
    {
        /* X * 0 == 0 */
        for (num1_slot_idx = 0; num1_slot_idx < num1->allocated_slot_length; num1_slot_idx++)
        {
            num1->slot[num1_slot_idx] = 0;
        }
        num1->bit_length = 1;
        num1->slot_length = 1;
        num1->sign = BIG_NUMBER_POSITIVE;
        return 0;
    }
    /* Calculate */
    num_final = __big_int_mul_without_check(num1, num2);
    if (num_final == NULL) return -1;
    /* Update final to num1 */
    __big_int_clean_slots(num1->slot, num1->slot_length);
    __big_int_mem_pool_free(num1->slot, num1->in_pool);
    num1->slot = num_final->slot;
    num1->allocated_slot_length = num_final->allocated_slot_length;
    num1->bit_length = num_final->bit_length;
    num1->slot_length = num_final->slot_length;
    num1->sign = num_final->sign;
    num1->in_pool = num_final->in_pool;
    free(num_final);

    return 0;
}

int big_int_left_shift(big_int_t *num, int bit_length)
{
    slot_t *dst_slot;
    int dst_in_pool;
    int slot_idx;
    unsigned int slot_delta, bit_delta, neccessary_bit, neccessary_slot;
    neccessary_bit = num->bit_length + bit_length;
    neccessary_slot = BIT_TO_SLOT(neccessary_bit);
    if (neccessary_slot >= num->allocated_slot_length) 
    {
        dst_slot = (slot_t *)__big_int_mem_pool_malloc(sizeof(slot_t) * (neccessary_slot + ALLOCATE_SLOT_SIZE), &dst_in_pool);
        if (dst_slot == NULL) return -1;
        num->allocated_slot_length = neccessary_slot + ALLOCATE_SLOT_SIZE;
        for (slot_idx = 0; slot_idx < (signed int)num->slot_length; slot_idx++) dst_slot[slot_idx] = num->slot[slot_idx];
        if (dst_in_pool == 0)
        {
            for (; slot_idx < (signed int)num->allocated_slot_length; slot_idx++) dst_slot[slot_idx] = 0;
        }
        __big_int_clean_slots(num->slot, num->slot_length);
        __big_int_mem_pool_free(num->slot, num->in_pool);
        num->slot = dst_slot;
        num->in_pool = dst_in_pool;
    }
    slot_delta = BIT_TO_SLOT_FLOOR(bit_length);
    bit_delta = SLOT_TAIL(bit_length);
    if (slot_delta > 0)
    {
        for (slot_idx = num->slot_length - 1; slot_idx >= 0; slot_idx--) 
            num->slot[slot_idx + slot_delta] = num->slot[slot_idx];
        /* blank space being created at lower slots */
        for (slot_idx = slot_delta - 1; slot_idx >= 0; slot_idx--) 
            num->slot[slot_idx] = 0;
        num->slot_length += slot_delta;
        num->bit_length += MUL_SLOT(slot_delta);
    }
    if (bit_delta > 0)
    {
        num->slot[num->slot_length] = 0;

        for (slot_idx = num->slot_length; slot_idx >= 1; slot_idx--)
        {
            num->slot[slot_idx] = ((num->slot[slot_idx] << bit_delta) & 0xFFFFFFFF) |\
                                  ((num->slot[slot_idx - 1] >> (32 - bit_delta)) & ((1 << bit_delta) - 1));
        }
        /* final slot */
        num->slot[slot_idx] = (num->slot[slot_idx] << bit_delta) & 0xFFFFFFFF;
        num->bit_length += bit_delta;

        if (num->slot[num->slot_length] != 0) num->slot_length++;
    }
    return 0;
}

int big_int_right_shift(big_int_t *num, int bit_length)
{
    int slot_idx;
    unsigned int slot_delta, bit_delta;
    slot_delta = BIT_TO_SLOT_FLOOR(bit_length);
    bit_delta = SLOT_TAIL(bit_length);
    if (slot_delta > 0)
    {
        for (slot_idx = 0; slot_idx < (signed int)num->slot_length - (signed int)slot_delta; slot_idx++)
        {
            num->slot[slot_idx] = num->slot[slot_idx + slot_delta];
        }
        /* blank space being created at higher slots */
        for (;slot_idx < (signed int)num->slot_length; slot_idx++)
        {
            num->slot[slot_idx] = 0;
        }
        num->slot_length -= slot_delta;
        num->bit_length -= MUL_SLOT(slot_delta);
        if (num->slot_length == 0) num->slot_length = 1;
    }
    if (bit_delta > 0)
    {
        for (slot_idx = 0; slot_idx != (signed int)num->slot_length; slot_idx++)
        {
            num->slot[slot_idx] = (num->slot[slot_idx] >> bit_delta) |\
                                  ((num->slot[slot_idx + 1] & ((1 << bit_delta) - 1)) << (BIT_PER_SLOT - bit_delta));
        }
    }
    if (num->slot[num->slot_length - 1] == 0 && num->slot_length > 0) num->slot_length--;
    num->bit_length = MUL_SLOT(num->slot_length - 1) + hbidx_32(num->slot[num->slot_length - 1]);
    return 0;
}

/* Barrett Reduction method, a faster algorithm to compute modulo 
 * with pre-computed value
 * Described in http://en.wikipedia.org/wiki/Barrett_reduction */

/* Calculate (2^(2n))/N = (1<<(2n))/N
 * n = N.bit_length */
big_int_t *big_int_barret_build(big_int_t *num_divisor)
{
    int ret;
    unsigned int n;
    big_int_t *num_barret;

	num_barret = big_int_new_from_int(1);
    if (num_barret == NULL) return NULL;
    n = num_divisor->bit_length;
    big_int_left_shift(num_barret, n << 1);
    ret = big_int_div_to(num_barret, num_divisor);
    if (ret != 0)
    {
        big_int_destroy(num_barret);
        return NULL;
    }
    return num_barret;
}

/* num1 >= num2 */
int big_int_mod_to_with_barret(big_int_t *num1, big_int_t *num2, big_int_t *barret)
{
    int slot_idx;
	unsigned int n;
    big_int_t *q;

    int ret = big_int_compare(num1, num2);
    if (ret == 0)
    {
        /* X % X = 0 */
        for (slot_idx = 0; slot_idx < (signed int)num1->slot_length; slot_idx++)
            num1->slot[slot_idx] = 0;
        num1->slot_length = 1;
        num1->bit_length = 1;
        return 0;
    }
    else if (ret == -1)
    {
        return 0;
    }

    /* Calculate q 
     * q = ((Z/2^(n-1))*((2^(2n))/N))/(2^(n-1)) 
     *   = ((Z>>(n-1))*(barret))>>(n+1)*/
    n = num2->bit_length;
    q = big_int_assign(num1);
    if (q == NULL) return -1;
    big_int_right_shift(q, n - 1);
    big_int_mul_to(q, barret);
    big_int_right_shift(q, n + 1);

    /* Z mod N = Z-q*N */
    big_int_mul_to(q, num2);
    big_int_sub_to(num1, q);

    /* Adjust result if reminder is greater than divisor */
    while (big_int_compare(num1, num2) >= 0)
    {
        big_int_sub_to(num1, num2);
    }

    big_int_destroy(q);
    return 0;
}

/* num1 >= num2 */
int big_int_mod_to(big_int_t *num1, big_int_t *num2)
{
	int ret;
    unsigned int slot_idx;
    int bit_delta;
	big_int_t *divisor;

    ret = big_int_compare(num1, num2);
    if (ret == 0)
    {
        /* X % X = 0 */
        for (slot_idx = 0; slot_idx < num1->slot_length; slot_idx++)
            num1->slot[slot_idx] = 0;
        num1->slot_length = 1;
        num1->bit_length = 1;
        return 0;
    }
    else if (ret == -1)
    {
        return 0;
    }
    divisor = big_int_assign(num2);
    if (divisor == NULL) return -1;
    bit_delta = num1->bit_length - divisor->bit_length;
    big_int_left_shift(divisor, bit_delta);
    while (big_int_compare(num1, divisor) == -1) 
    {
        big_int_right_shift(divisor, 1);
        bit_delta--;
    }
    for (;;)
    {
        big_int_sub_to(num1, divisor);
        /* TO BE OPTIMIZE: right shift to less */
        while (((big_int_compare(divisor, num1)) == 1) && bit_delta > 0) /* do loop while num1 < divisor */
        {
            big_int_right_shift(divisor, 1);
            bit_delta--;
        }
        if (bit_delta == 0 && big_int_compare(divisor, num1) > 0) break;
    }
    if (divisor != NULL) big_int_destroy(divisor);
    return 0;
}

int big_int_dec(big_int_t *num)
{
    unsigned int slot_idx;
    slot_idx = 0;
    while (slot_idx < num->slot_length)
    {
        if (num->slot[slot_idx] != 0)
        {
            num->slot[slot_idx]--;
            break;
        }
        else
        {
            num->slot[slot_idx] = 0xFFFFFFFF;
        }
        slot_idx++;
    }
    if (num->slot[num->slot_length - 1] == 0)
    {
        num->slot_length--;
        if (num->slot_length == 0) num->slot_length++;
    }
    return 0;
}

int big_int_add_to_u16(big_int_t *num, unsigned int value)
{
    int slot_idx;
    unsigned int carry = 0;
    uint64_t tmp;

    carry = value;
    for (slot_idx = 0; slot_idx != (signed int)num->slot_length; slot_idx++)
    {
        tmp = (uint64_t)num->slot[slot_idx] + carry;
        carry = (uint64_t)num->slot[slot_idx] >> BIT_PER_SLOT;
        num->slot[slot_idx] = tmp & 0xFFFFFFFF;
    }
    if (carry)
    {
        num->slot[num->slot_length] = 1;
        num->slot_length++;
        num->bit_length = MUL_SLOT(num->slot_length - 1) + 1;
    }
    else
    {
        __trim_tail(num->slot, &num->slot_length, &num->bit_length);
    }
    return 0;
}

int big_int_pow_to(big_int_t *num1, big_int_t *num2)
{
    int ret;
    int sign;
    big_int_t *count = NULL, *result = NULL, *temp = NULL;
    /* Sign */
    if (num1->sign == BIG_NUMBER_POSITIVE)
    {
        sign = BIG_NUMBER_POSITIVE;
    }
    else
    {
        if ((num2->slot[0] & 1) == 0) /* Even */
            sign = BIG_NUMBER_POSITIVE;
        else /* Odd */
            sign = BIG_NUMBER_NEGATIVE;
    }
    if (num1->slot_length == 1 && num1->slot[0] == 0) /* 0 ^ n = 0 */
    {
        result = big_int_new_from_int(0);
    }
    else if (num1->slot_length == 1 && num1->slot[0] == 1) /* 1 ^ n = 1 */
    {
        result = big_int_new_from_int(1);
    }
    else
    {
        result = big_int_new_from_int(1);
        count = big_int_assign(num2);
        if (result == NULL) {ret = -1; goto fail;}
        if (count == NULL) {ret = -1; goto fail;}
        if (num1->slot_length == 1 && num1->slot[0] == 2)
        {
            while (!(count->slot_length == 1 && count->slot[0] == 0))
            {
                if (count->slot[0] != 0)
                {
                    big_int_left_shift(result, count->slot[0]);
                    temp = big_int_new_from_int(count->slot[0]);
                    big_int_sub_to(count, temp);
                    big_int_destroy(temp); temp = NULL;
                    /*count->slot[0] = 0;*/
                }
                else
                {
                    big_int_left_shift(result, 1);
                    big_int_dec(count);
                }
            }
        }
        else
        {
            while (!(count->slot_length == 1 && count->slot[0] == 0))
            {
                big_int_mul_to(result, num1);
                big_int_dec(count);
            }
        }
    }
    __big_int_clean_slots(num1->slot, num1->slot_length);
    __big_int_mem_pool_free(num1->slot, num1->in_pool);
    num1->slot = result->slot;
    num1->allocated_slot_length = result->allocated_slot_length;
    num1->bit_length = result->bit_length;
    num1->slot_length = result->slot_length;
    num1->sign = sign;
    num1->in_pool = result->in_pool;
    free(result); result = NULL;
    /* Sign */
    ret = 0;
fail:
    if (count != NULL) big_int_destroy(count);
    if (result != NULL) big_int_destroy(result);
    if (temp != NULL) big_int_destroy(temp);
    return ret;
}

int big_int_div_to(big_int_t *num1, big_int_t *num2)
{
	int ret;
    int sign = (num1->sign == num2->sign) ? BIG_NUMBER_POSITIVE : BIG_NUMBER_NEGATIVE;
    unsigned int slot_idx;
    int bit_delta;
    big_int_t *quotient;
    big_int_t *divisor;

    ret = big_int_compare(num1, num2);
    /* FIXME: Divide by Zero */
    if (ret == 0)
    {
        /* X / X = 1 */
        for (slot_idx = 0; slot_idx < num1->slot_length; slot_idx++)
            num1->slot[slot_idx] = 0;
        num1->slot[0] = 1;
        num1->slot_length = 1;
        num1->bit_length = 1;
        return 0;
    }
    else if (ret == -1)
    {
        /* min / max = 0 */
        for (slot_idx = 0; slot_idx < num1->slot_length; slot_idx++)
            num1->slot[slot_idx] = 0;
        num1->slot_length = 1;
        num1->bit_length = 1;
        return 0;
    }
    quotient = __big_int_new_zero(num1->bit_length);
    divisor = big_int_assign(num2);
    if (quotient == NULL || divisor == NULL) goto fail;
    /* Locate divisor */
    bit_delta = num1->bit_length - divisor->bit_length;
    big_int_left_shift(divisor, bit_delta);
    while (big_int_compare(num1, divisor) == -1) 
    {
        big_int_right_shift(divisor, 1);
        bit_delta--;
    }
    quotient->bit_length = bit_delta;
    quotient->slot_length = BIT_TO_SLOT(bit_delta + 1);
    /* Divide */
    for (;;)
    {
        quotient->slot[bit_delta / BIT_PER_SLOT] |= 1 << (bit_delta % BIT_PER_SLOT);
        big_int_sub_to(num1, divisor);
        /* TO BE OPTIMIZE: right shift to less */
        while (((big_int_compare(divisor, num1)) == 1) && bit_delta > 0) /* do loop while num1 < divisor */
        {
            big_int_right_shift(divisor, 1);
            bit_delta--;
        }
        if (bit_delta == 0 && big_int_compare(divisor, num1) > 0) break;
    }
    __big_int_clean_slots(num1->slot, num1->slot_length);
    __big_int_mem_pool_free(num1->slot, num1->in_pool);
    num1->slot = quotient->slot;
    num1->slot_length = quotient->slot_length;
    num1->bit_length = quotient->bit_length;
    /* Sign */
    num1->sign = sign;
    num1->in_pool = quotient->in_pool;
    /* Zero check */
    if (big_int_is_zero(num1)) num1->sign = BIG_NUMBER_POSITIVE;
    free(quotient); quotient = NULL;
    ret = 0;
fail:
    if (quotient != NULL) big_int_destroy(quotient);
    if (divisor != NULL) big_int_destroy(divisor);
    return ret;
}

int big_int_pow_mod_to(big_int_t *num1, big_int_t *num2, big_int_t *num3)
{
    int ret;
    int sign;
    int slot_idx, bit_idx;
    int bit_count;
    big_int_t *result = NULL, *temp = NULL;

    /* Sign for pow part */
    if (num1->sign == BIG_NUMBER_POSITIVE)
    {
        sign = BIG_NUMBER_POSITIVE;
    }
    else
    {
        if ((num2->slot[0] & 1) == 0) /* Even */
            sign = BIG_NUMBER_POSITIVE;
        else /* Odd */
            sign = BIG_NUMBER_NEGATIVE;
    }
    /* Sign for mod part */
    sign = (sign == num2->sign) ? BIG_NUMBER_POSITIVE : BIG_NUMBER_NEGATIVE;


    if (num1->slot_length == 1 && num1->slot[0] == 0) /* 0 ^ n = 0 */
    {
        result = big_int_new_from_int(0);
    }
    else if (num1->slot_length == 1 && num1->slot[0] == 1) /* 1 ^ n = 1 */
    {
        result = big_int_new_from_int(1);
    }
    else
    {
        result = big_int_new_from_int(1);
        if (result == NULL) {ret = -1; goto fail;}
        bit_count = num2->bit_length;
        for (slot_idx = 0; slot_idx != (signed int)num2->slot_length; slot_idx++)
        {
            for (bit_idx = 0; bit_idx != BIT_PER_SLOT; bit_idx++)
            {
                if (num2->slot[slot_idx] & (1 << bit_idx))
                {
                    big_int_mul_to(result, num1);
                    big_int_mod_to(result, num3);
                }
                big_int_mul_to(num1, num1);
                big_int_mod_to(num1, num3);
                bit_count--;
                if (bit_count == 0) break;
            }
        }
    }
    __big_int_clean_slots(num1->slot, num1->slot_length);
    __big_int_mem_pool_free(num1->slot, num1->in_pool);
    num1->slot = result->slot;
    num1->allocated_slot_length = result->allocated_slot_length;
    num1->bit_length = result->bit_length;
    num1->slot_length = result->slot_length;
    num1->sign = result->sign;
    num1->in_pool = result->in_pool;
    free(result); result = NULL;
    /* Sign */
    num1->sign = sign;
    ret = 0;
fail:
    if (result != NULL) big_int_destroy(result);
    if (temp != NULL) big_int_destroy(temp);
    return ret;
}

int big_int_pow_mod_to_with_barret(big_int_t *num1, big_int_t *num2, big_int_t *num3, big_int_t *num3_barret)
{
    int ret;
    int sign;
    int slot_idx, bit_idx;
    int bit_count;
    big_int_t *result = NULL, *temp = NULL;

	/* Sign for pow part */
    if (num1->sign == BIG_NUMBER_POSITIVE)
    {
        sign = BIG_NUMBER_POSITIVE;
    }
    else
    {
        if ((num2->slot[0] & 1) == 0) /* Even */
            sign = BIG_NUMBER_POSITIVE;
        else /* Odd */
            sign = BIG_NUMBER_NEGATIVE;
    }
    /* Sign for mod part */
    sign = (sign == num2->sign) ? BIG_NUMBER_POSITIVE : BIG_NUMBER_NEGATIVE;


    if (num1->slot_length == 1 && num1->slot[0] == 0) /* 0 ^ n = 0 */
    {
        result = big_int_new_from_int(0);
    }
    else if (num1->slot_length == 1 && num1->slot[0] == 1) /* 1 ^ n = 1 */
    {
        result = big_int_new_from_int(1);
    }
    else
    {
        result = big_int_new_from_int(1);
        if (result == NULL) {ret = -1; goto fail;}
        bit_count = num2->bit_length;
        for (slot_idx = 0; slot_idx != (signed int)num2->slot_length; slot_idx++)
        {
            for (bit_idx = 0; bit_idx != BIT_PER_SLOT; bit_idx++)
            {
                if (num2->slot[slot_idx] & (1 << bit_idx))
                {
                    big_int_mul_to(result, num1);
                    big_int_mod_to_with_barret(result, num3, num3_barret);
                }
                big_int_mul_to(num1, num1);
                big_int_mod_to_with_barret(num1, num3, num3_barret);
                bit_count--;
                if (bit_count == 0) break;
            }
        }
    }
    __big_int_clean_slots(num1->slot, num1->slot_length);
    __big_int_mem_pool_free(num1->slot, num1->in_pool);
    num1->slot = result->slot;
    num1->allocated_slot_length = result->allocated_slot_length;
    num1->bit_length = result->bit_length;
    num1->slot_length = result->slot_length;
    num1->sign = result->sign;
    num1->in_pool = result->in_pool;
    free(result); result = NULL;
    /* Sign */
    num1->sign = sign;
    ret = 0;
fail:
    if (result != NULL) big_int_destroy(result);
    if (temp != NULL) big_int_destroy(temp);
    return ret;
}

