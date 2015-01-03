/*
   Big Integer Library - Testing Program
   Copyright (c) 2013-2015 Cheryl Natsu 
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

#include <stdio.h>
#include <stdlib.h>

#include "argsparse.h"

#include "big_int.h"
#include "big_int_rand.h"
#include "big_int_prime.h"
#include "big_int_fibonacci.h"


static int show_version(void)
{
    const char *info = ""
        "Big Integer Library Demo Program\n"
        "Copyright (c) 2013-2015 Cheryl Natsu\n";
    puts(info);
    return 0;
}

static int show_help(void)
{
    const char *info = ""
        "Usage : bigint <testname> <arguments>\n"
        "\n"
        "Arithmetic:\n"
        "add <num1> <num2>\n"
        "sub <num1> <num2>\n"
        "mul <num1> <num2>\n"
        "div <num1> <num2>\n"
        "mod <num1> <num2>\n"
        "\n"
        "Public-Key Cryptography:\n"
        "random    <length:bit>     Random Number generate\n"
        "prime     <length:bit>     Big prime number generate\n"
        "dh        <length:bit>     Diffie–Hellman key exchange\n"
        "\n"
        "Others:\n"
        "fib       <n:int>          nth item in fibonacci array\n"
        "";
    show_version();
    puts(info);
    return 0;
}


/* Add */
int add_op(char *s_num1, char *s_num2)
{
    big_int_t *num1 = big_int_new_from_str(s_num1);
    big_int_t *num2 = big_int_new_from_str(s_num2);

    big_int_add_to(num1, num2);
    big_int_print(num1);printf("\n");fflush(stdout);
    big_int_destroy(num1);
    big_int_destroy(num2);

    return 0;
}


/* Substract */
int sub_op(char *s_num1, char *s_num2)
{
    big_int_t *num1 = big_int_new_from_str(s_num1);
    big_int_t *num2 = big_int_new_from_str(s_num2);

    big_int_sub_to(num1, num2);
    big_int_print(num1);printf("\n");fflush(stdout);
    big_int_destroy(num1);
    big_int_destroy(num2);

    return 0;
}


/* Multiple */
int mul_op(char *s_num1, char *s_num2)
{
    big_int_t *num1 = big_int_new_from_str(s_num1);
    big_int_t *num2 = big_int_new_from_str(s_num2);

    big_int_mul_to(num1, num2);
    big_int_print(num1);printf("\n");fflush(stdout);
    big_int_destroy(num1);
    big_int_destroy(num2);

    return 0;
}


/* Divide */
int div_op(char *s_num1, char *s_num2)
{
    big_int_t *num1 = big_int_new_from_str(s_num1);
    big_int_t *num2 = big_int_new_from_str(s_num2);

    big_int_div_to(num1, num2);
    big_int_print(num1);printf("\n");fflush(stdout);
    big_int_destroy(num1);
    big_int_destroy(num2);

    return 0;
}


/* Modulo */
int mod_op(char *s_num1, char *s_num2)
{
    big_int_t *num1 = big_int_new_from_str(s_num1);
    big_int_t *num2 = big_int_new_from_str(s_num2);

    big_int_mod_to(num1, num2);
    big_int_print(num1);printf("\n");fflush(stdout);
    big_int_destroy(num1);
    big_int_destroy(num2);

    return 0;
}


/* Random number generator */
int random_generate(size_t length)
{
    big_int_t *num = NULL;

    num = big_int_new_random(length);
    big_int_print(num);printf("\n");fflush(stdout);
    big_int_destroy(num);

    return 0;
}


/* Prime number generator */
int prime_generate(size_t length)
{
    big_int_t *num = NULL;

    num = big_int_new_prime(length);
    big_int_print(num);printf("\n");fflush(stdout);
    big_int_destroy(num);

    return 0;
}


/* A simple demonstration of Diffie–Hellman key exchange
 * http://en.wikipedia.org/wiki/Diffie%E2%80%93Hellman_key_exchange */
int dh(size_t length)
{

    big_int_t *p, *g;
    big_int_t *private_alice, *private_bob;
    big_int_t *public_alice, *public_bob;
    big_int_t *password_alice = NULL, *password_bob = NULL;

    p = big_int_new_prime(128*3);
    g = big_int_new_from_int(2);
    printf("p="); printf("0x"); big_int_print(p); printf("\n");
    printf("g="); printf("0x"); big_int_print(g); printf("\n");

    private_alice = big_int_new_prime(length);
    private_bob = big_int_new_prime(length);

    printf("private_alice="); printf("0x"); big_int_print(private_alice); printf("\n");
    printf("private_bob="); printf("0x"); big_int_print(private_bob); printf("\n");

    fflush(stdout);

    public_alice = big_int_assign(g);
    big_int_pow_mod_to(public_alice, private_alice, p);

    public_bob = big_int_assign(g);
    big_int_pow_mod_to(public_bob, private_bob, p);

    printf("public_alice="); printf("0x"); big_int_print(public_alice); printf("\n");
    printf("public_bob="); printf("0x"); big_int_print(public_bob); printf("\n");

    password_alice = big_int_assign(public_bob);
    big_int_pow_mod_to(password_alice, private_alice, p);

    password_bob = big_int_assign(public_alice);
    big_int_pow_mod_to(password_bob, private_bob, p);

    printf("password_alice = "); printf("0x"); big_int_print(password_alice); printf("\n");
    printf("password_bob   = "); printf("0x"); big_int_print(password_bob); printf("\n");
    fflush(stdout);

    big_int_destroy(p);
    big_int_destroy(g);
    big_int_destroy(private_alice);
    big_int_destroy(private_bob);

    big_int_destroy(public_alice);
    big_int_destroy(public_bob);
    if (password_alice != NULL) big_int_destroy(password_alice);
    if (password_bob != NULL) big_int_destroy(password_bob);

    return 0;
}


/* nth item in fibonacci array */
int fibonacci_nth(int idx)
{
    big_int_t *num;

    num = fibonacci(idx);
    big_int_print(num); printf("\n");
    big_int_destroy(num);

    return 0;
}


int main(int argc, const char *argv[])
{
    argsparse_t argsparse;
    char *s_length, *s_index, *s_num1, *s_num2;

    rand_initialize();
    big_int_mem_pool_initialize(4096 * 128);
    argsparse_init(&argsparse, (int)argc, (char **)argv);

    if (argsparse_available(&argsparse) == 0)
    { show_help(); goto done; }

    if (argsparse_match_str(&argsparse, "--version"))
    { show_version(); goto done; }
    else if (argsparse_match_str(&argsparse, "--help"))
    { show_help(); goto done; }
    else if (argsparse_match_str(&argsparse, "add"))
    { 
        argsparse_next(&argsparse);
        if (argsparse_available_count(&argsparse, 2) == 0)
        { show_help(); goto done; }
        else
        { 
            s_num1 = argsparse_fetch(&argsparse); 
            argsparse_next(&argsparse);
            s_num2 = argsparse_fetch(&argsparse); 
            add_op(s_num1, s_num2);
        }
    }
    else if (argsparse_match_str(&argsparse, "sub"))
    {
        argsparse_next(&argsparse);
        if (argsparse_available_count(&argsparse, 2) == 0)
        { show_help(); goto done; }
        else
        { 
            s_num1 = argsparse_fetch(&argsparse); 
            argsparse_next(&argsparse);
            s_num2 = argsparse_fetch(&argsparse); 
            sub_op(s_num1, s_num2);
        }
    }
    else if (argsparse_match_str(&argsparse, "mul"))
    {
        argsparse_next(&argsparse);
        if (argsparse_available_count(&argsparse, 2) == 0)
        { show_help(); goto done; }
        else
        { 
            s_num1 = argsparse_fetch(&argsparse); 
            argsparse_next(&argsparse);
            s_num2 = argsparse_fetch(&argsparse); 
            mul_op(s_num1, s_num2);
        }
    }
    else if (argsparse_match_str(&argsparse, "div"))
    {
        argsparse_next(&argsparse);
        if (argsparse_available_count(&argsparse, 2) == 0)
        { show_help(); goto done; }
        else
        { 
            s_num1 = argsparse_fetch(&argsparse); 
            argsparse_next(&argsparse);
            s_num2 = argsparse_fetch(&argsparse); 
            div_op(s_num1, s_num2);
        }
    }
    else if (argsparse_match_str(&argsparse, "mod"))
    {
        argsparse_next(&argsparse);
        if (argsparse_available_count(&argsparse, 2) == 0)
        { show_help(); goto done; }
        else
        { 
            s_num1 = argsparse_fetch(&argsparse); 
            argsparse_next(&argsparse);
            s_num2 = argsparse_fetch(&argsparse); 
            mod_op(s_num1, s_num2);
        }
    }
    else if (argsparse_match_str(&argsparse, "random"))
    {
        argsparse_next(&argsparse);
        if (argsparse_available(&argsparse) == 0)
        { show_help(); goto done; }
        else
        { s_length = argsparse_fetch(&argsparse); }
        random_generate(atoi(s_length));
    }
    else if (argsparse_match_str(&argsparse, "prime"))
    {
        argsparse_next(&argsparse);
        if (argsparse_available(&argsparse) == 0)
        { show_help(); goto done; }
        else
        { s_length = argsparse_fetch(&argsparse); }
        prime_generate(atoi(s_length));
    }
    else if (argsparse_match_str(&argsparse, "dh"))
    {
        argsparse_next(&argsparse);
        if (argsparse_available(&argsparse) == 0)
        { show_help(); goto done; }
        else
        { s_length = argsparse_fetch(&argsparse); }
        dh(atoi(s_length));
    }
    else if (argsparse_match_str(&argsparse, "fib"))
    { 
        argsparse_next(&argsparse);
        if (argsparse_available(&argsparse) == 0)
        { show_help(); goto done; }
        else
        { s_index = argsparse_fetch(&argsparse); }
        fibonacci_nth(atoi(s_index));
    }
    else
    { show_help(); goto done; }

done:
    big_int_mem_pool_uninitialize();
    rand_uninitialize();
    return 0;
}

