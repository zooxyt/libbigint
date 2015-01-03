libbigint -- Big Integer Library
================================

An Integer Library which supports arbitrary-precision calculations and 
easy to intergrate with other projects.


Features
--------
1. Arbitrary-precision arithemetic operations
2. Bitwise operations
3. Optimized multiple, division and pow-mod etc.
4. Prime number related operations.
5. Fast Fibonacci Array operations


Hardware Requirements
---------------------
The library is designed for working on 32-bit CPU or above.


Software Requirements
---------------------
The source code of the library is written in C99 completely and 
should works with most modern C compilers on 32-bit Operating Systems.


Building
--------
Python2 is required to generate the Makefile. 
(Python3 is not supported yet)

Use the following command to generate the Makefile:
```
$ ./configure
```

And the following commands to build the project:
```
$ make # for building the test program
$ make static # for building the static library
$ make shared # for building the shared (dynamic) library
```

LICENSE
-------
BSD 3

