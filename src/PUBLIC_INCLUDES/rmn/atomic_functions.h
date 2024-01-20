//
// Copyright (C) 2024  Environnement Canada
//
// This is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation,
// version 2.1 of the License.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details .
//
// Author:
//     M. Valin,   Recherche en Prevision Numerique, 2024
//
// set of macros and functions to manage atomic operations in memory
//

#if !defined(UNTIL_EQ)

#include <stdint.h>

#if !defined(EXTERN)
#define EXTERN static inline
#endif

#define UNTIL_LT -2
#define UNTIL_LE -1
#define UNTIL_EQ  0
#define UNTIL_GE  1
#define UNTIL_GT  2

// spin until *what COND value (cond = < <= == >= >)
// volatile is CRITICAL to prevent repeated memory load elimination
// return number of iterations in spin wait
// if call is made without using iteration count, optimizers act accordingly
static uint64_t spin_until(int32_t volatile *what, int32_t value, int32_t cond){
  uint64_t i = 0 ;
  if(cond == UNTIL_EQ){
    while(*what != value) i++ ;   // until *what == value
  }
  if(cond == UNTIL_GE){
    while(*what < value) i++ ;   // until *what >= value
  }
  if(cond == UNTIL_GT){
    while(*what <= value) i++ ;   // until *what > value
  }
  if(cond == UNTIL_LE){
    while(*what > value) i++ ;   // until *what <= value
  }
  if(cond == UNTIL_LT){
    while(*what >= value) i++ ;   // until *what < value
  }
  return i ;
}

// atomic operation *what oper n  (oper = add/and/etc...)
// return 1 if result is 0, 0 otherwise
#define OPER_AND_TEST(name, oper, kind) EXTERN int name(kind *what, kind n)\
        {uint8_t c;\
        __asm__ __volatile__("lock;\n" oper  "sete %1   ;\n" : "=m" (*what), "=qr" (c) : "ir" (n), "m" (*what) : "memory") ;\
        return c ;}

// atomic_add_and_test_64/32/16/8( *what, n)  add n to *what
OPER_AND_TEST(atomic_add_and_test_64, "add %2,%0 ;", int64_t)
OPER_AND_TEST(atomic_add_and_test_32, "add %2,%0 ;", int32_t)
OPER_AND_TEST(atomic_add_and_test_16, "add %2,%0 ;", int16_t)
OPER_AND_TEST(atomic_add_and_test_8,  "add %2,%0 ;", int8_t )

// atomic_and_and_test_64/32/16/8( *what, n)  and n to *what
OPER_AND_TEST(atomic_and_and_test_64, "and %2,%0 ;", int64_t)
OPER_AND_TEST(atomic_and_and_test_32, "and %2,%0 ;", int32_t)
OPER_AND_TEST(atomic_and_and_test_16, "and %2,%0 ;", int16_t)
OPER_AND_TEST(atomic_and_and_test_8,  "and %2,%0 ;", int8_t)

// atomic_fetch_and_add_64/32/16/8( *ptr, value) get *ptr, add value to *ptr, return original value
#define FETCH_AND_ADD(name, kind) EXTERN kind name(kind *ptr, kind value){ kind previous; \
        asm volatile ("lock; xadd %1, %2;" : "=r"(previous) : "0"(value), "m"(*ptr): "memory"); \
        return previous; }

FETCH_AND_ADD(atomic_fetch_and_add_64, int64_t)
FETCH_AND_ADD(atomic_fetch_and_add_32, int32_t)
FETCH_AND_ADD(atomic_fetch_and_add_16, int16_t)
FETCH_AND_ADD(atomic_fetch_and_add_8 , int8_t)

// atomic_compare_and_swap_64/32/16/8( *thevalue, expected, newvalue) if(*thevalue == expected) *thevalue = newvalue
// return original value of *thevalue
#define COMPARE_AND_SWAP(name, kind) EXTERN int name(kind *thevalue, kind expected, kind newvalue) {kind prev; \
        asm volatile("lock;cmpxchg %1, %2;" : "=a"(prev) : "q"(newvalue), "m"(*thevalue), "a"(expected) : "memory"); \
        return prev == expected;}

COMPARE_AND_SWAP(atomic_compare_and_swap_64, uint64_t)
COMPARE_AND_SWAP(atomic_compare_and_swap_32, uint32_t)
COMPARE_AND_SWAP(atomic_compare_and_swap_16, uint16_t)
COMPARE_AND_SWAP(atomic_compare_and_swap_8 , uint8_t)

#endif
