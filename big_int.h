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

#ifndef _BIG_INT_H_
#define _BIG_INT_H_

#include <stdio.h>
#include <stdint.h>

#include "big_int_compatible.h"

typedef uint32_t slot_t;

#define BIG_NUMBER_POSITIVE 0
#define BIG_NUMBER_NEGATIVE 1
typedef struct big_int
{
    slot_t *slot; /* one slot can contains 32 bits information */
    size_t bit_length; /* data length in binary */
    size_t slot_length; /* data slot used acturelly */
    size_t allocated_slot_length; /* data slot allocated */
    int sign; /* sign of integer, (zero is always marks as positive) */
    int in_pool; /* allocated in memory pool */
} big_int_t;

/* general */
big_int_t *big_int_new_from_int(unsigned int value);
big_int_t *big_int_new_from_int_with_sign(int sign, unsigned int value);
big_int_t *big_int_new_random(size_t bit_length);
int big_int_print(const big_int_t *big_int);
int big_int_destroy(big_int_t *big_int);
big_int_t *big_int_assign(big_int_t *num);
int big_int_assign_to(big_int_t *num_dst, big_int_t *num_src);
/* logic */
int big_int_compare(big_int_t *num1, big_int_t *num2);
inline int big_int_is_zero(big_int_t *num);
/* arithmetic */
int big_int_add_to(big_int_t *num1, big_int_t *num2);
big_int_t *big_int_mul(big_int_t *num1, big_int_t *num2);
int big_int_mul_to(big_int_t *num1, big_int_t *num2);
int big_int_sub_to(big_int_t *num1, big_int_t *num2); /* num1 >= num2 */
int big_int_div_to(big_int_t *num1, big_int_t *num2);
int big_int_mod_to(big_int_t *num1, big_int_t *num2); /* num1 >= num2 */
int big_int_dec(big_int_t *num);
int big_int_add_to_u16(big_int_t *num, unsigned int value);
int big_int_pow_to(big_int_t *num1, big_int_t *num2);
int big_int_pow_mod_to(big_int_t *num1, big_int_t *num2, big_int_t *num3);
/* bitwise */
int big_int_left_shift(big_int_t *num, int bit_length);
int big_int_right_shift(big_int_t *num, int bit_length);
/* special */
big_int_t *big_int_barret_build(big_int_t *num_divisor);
int big_int_mod_to_with_barret(big_int_t *num1, big_int_t *num2, big_int_t *barret);
int big_int_pow_mod_to_with_barret(big_int_t *num1, big_int_t *num2, big_int_t *num3, big_int_t *num3_barret);
/* memory pool */
int big_int_mem_pool_initialize(size_t size);
int big_int_mem_pool_uninitialize(void);

/* debug */
int __big_int_mul_karatsuba_split(big_int_t *num, unsigned int shift, big_int_t **high, big_int_t **low);
big_int_t *__big_int_mul_karatsuba(big_int_t *num1, big_int_t *num2);
big_int_t *__big_int_mul_plain(big_int_t *num1, big_int_t *num2);
big_int_t *big_int_new_from_str(char *value);
inline big_int_t *__big_int_square_plain(big_int_t *x);

#endif 

