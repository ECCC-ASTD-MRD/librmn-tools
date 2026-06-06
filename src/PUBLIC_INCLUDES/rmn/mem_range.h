//
// Copyright (C) 2026  Environnement Canada
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
//     M. Valin,   Recherche en Prevision Numerique, 2026
//
#if ! defined(RANGE_TYPEDEF)

// memory address
#define PTR(what) ((uint8_t *)(what))

// apply byte offset to base address, result is a typeless pointer
#define PTR_OFFSET(BASE, OFFSET) ( (void *)( PTR(BASE) + (OFFSET) ) )

// difference between addresses (in BYTES)
#define PTR_DIFF(ADDR1, ADDR2) ( PTR(ADDR2) - PTR(ADDR1) )

// the type for an address range for data type xxx will be xxx_range
// e.g. for a float it would be float_range as in : float_range some_name
#define NULL_RANGE {NULL, 0L}
// a xxx_range struct contains 2 pointers, bot and top
// bot points to the beginning of the memory arena, top points 1 element past the last element in the arena
#define RANGE_TYPEDEF(KIND) typedef struct{ KIND *bot, *top ; } KIND##_range ; static const KIND##_range KIND##_range_null = NULL_RANGE ;

// some predefined address ranges
// unsigned integers
RANGE_TYPEDEF(uint8_t) ;    // creates type  uint8_t_range
RANGE_TYPEDEF(uint16_t) ;
RANGE_TYPEDEF(uint32_t) ;
RANGE_TYPEDEF(uint64_t) ;
// signed integers
RANGE_TYPEDEF(int8_t) ;
RANGE_TYPEDEF(int16_t) ;
RANGE_TYPEDEF(int32_t) ;
RANGE_TYPEDEF(int64_t) ;
// typeless
RANGE_TYPEDEF(void) ;
// generic range
typedef struct{
  void *bot ;     // start of the memory range
  void *top ;     // address of element just above the memory range
}address_range ;

// declare a range struct with elements of type KIND
#define RANGE(KIND) KIND##_range

// return base address of range
#define RANGE_BASE(R) ( PTR((R).bot) )

// return last address still in the range
#define RANGE_LIMIT(R) ( PTR((R).top) - 1 )

// return address just above last address in range
#define RANGE_TOP(R) ( PTR((R).top) )

// return number of bytes in a range
#define RANGE_SIZE(R) ( PTR_DIFF( (R).bot ,  (R).top ) )

// return number of elements in a range
#define RANGE_ELEMENTS(R) ((R).top - (R).bot)

// return the size of a range element in bytes
#define ELEMENT_SIZE(R) (sizeof((R).bot[0]) / sizeof(uint8_t))

// set base address of range
#define SET_RANGE_BASE(R, BASE)  { (R).bot = (void *)(BASE) ; }

// set top address of range to base address + size in bytes
#define SET_RANGE_SIZE(R, BYTES) { (R).top = PTR_OFFSET( (R).bot, (BYTES) ) ; }

// set base address and size in bytes of address range
#define SET_RANGE(R, BASE, BYTES) { SET_RANGE_BASE(R, BASE) ; SET_RANGE_SIZE(R, BYTES) ; }


#endif
