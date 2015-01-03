/*
   Big Integer Library - Fibonacci Array
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


#include "big_int.h"
#include "big_int_fibonacci.h"

#define MATRIX_2_MUL_EXP(r1,r2,a1,a2,b1,b2,a3,a4,b3,b4) \
    do {\
        t1 = big_int_mul(a[a1][a2], b[b1][b2]); \
        t2 = big_int_mul(a[a3][a4], b[b3][b4]); \
        big_int_add_to(t1, t2); \
        big_int_destroy(t2); t2 = NULL; \
        result[r1][r2] = t1; t1 = NULL; \
    } while (0)

static int matrix_2_mul(big_int_t *result[2][2], big_int_t *a[2][2], big_int_t *b[2][2])
{
    big_int_t *t1, *t2;

    MATRIX_2_MUL_EXP(0, 0,  0, 0, 0, 0, 0, 1, 1, 0);
    MATRIX_2_MUL_EXP(0, 1,  0, 0, 0, 1, 0, 1, 1, 1);
    MATRIX_2_MUL_EXP(1, 0,  1, 0, 0, 0, 1, 1, 1, 0);
    MATRIX_2_MUL_EXP(1, 1,  1, 0, 0, 1, 1, 1, 1, 1);

    return 0;
}

#define MATRIX_2_MOVE(dst,src) \
    do {\
        dst[0][0] = src[0][0]; src[0][0] = NULL; \
        dst[0][1] = src[0][1]; src[0][1] = NULL; \
        dst[1][0] = src[1][0]; src[1][0] = NULL; \
        dst[1][1] = src[1][1]; src[1][1] = NULL; \
    } while (0)

#define MATRIX_2_DESTROY(m) \
    do {\
        big_int_destroy(m[0][0]); m[0][0] = NULL; \
        big_int_destroy(m[0][1]); m[0][1] = NULL; \
        big_int_destroy(m[1][0]); m[1][0] = NULL; \
        big_int_destroy(m[1][1]); m[1][1] = NULL; \
    } while (0)

big_int_t *fibonacci(unsigned int n)
{
    big_int_t *a[2][2], *r[2][2], *t[2][2];
    big_int_t *final;

    /* First 3 items are 0, 1, and 1 */
    switch (n)
    {
        case 0: return big_int_new_from_int(0);
        case 1:
        case 2: return big_int_new_from_int(1);
        default: n -= 2;
    }

    /* Initialize Matrix
     * 
     * a = | 1 1 |
     *     | 1 0 |
     *
     * r = | 1 0 |
     *     | 0 1 |
     */
    a[0][0] = big_int_new_from_int(1);
    a[0][1] = big_int_new_from_int(1);
    a[1][0] = big_int_new_from_int(1);
    a[1][1] = big_int_new_from_int(0);

    r[0][0] = big_int_new_from_int(1);
    r[0][1] = big_int_new_from_int(0);
    r[1][0] = big_int_new_from_int(0);
    r[1][1] = big_int_new_from_int(1);

    while (n != 0)
    {
        if ((n & 1) == 1) 
        {
            MATRIX_2_MOVE(t, r);
            matrix_2_mul(r, t, a);
            MATRIX_2_DESTROY(t);
        }
        MATRIX_2_MOVE(t, a);
        matrix_2_mul(a, t, t);
        MATRIX_2_DESTROY(t);
        n >>= 1;
    }
    final = big_int_assign(r[0][0]);
    big_int_add_to(final, r[0][1]);
    MATRIX_2_DESTROY(a);
    MATRIX_2_DESTROY(r);

    return final;
}



