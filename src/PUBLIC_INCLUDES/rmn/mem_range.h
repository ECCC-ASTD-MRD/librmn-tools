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

// macros to help manipulate "memory address ranges" with elements of varied types

#if ! defined(RANGE_TYPEDEF)

// memory address (pointer to anything)
#define PTR_VOID(what) ((void *)(what))

// memory address (pointer to unsigned bytes)
#define PTR(what) ((uint8_t *)(what))

// cast a pointer into a pointer to a specified type
#define PTR_CAST(PTR, KIND) ((KIND *)(PTR))

// apply byte offset to base address, result is a typeless pointer
#define SET_PTR_OFFSET(BASE, OFFSET) ( (void *)( PTR(BASE) + (OFFSET) ) )

// difference between addresses (in BYTES), 0 if any argument is NULL
#define PTR_OFFSET(BASE, ADDR2)  ( ( (ADDR2) && (BASE) )  ? ( PTR(ADDR2) - PTR(BASE) ) : 0 )

// difference between addresses (in "element" units) (both arguments MUST be pointers to the same type), 0 if any argument is NULL
#define PTR_ELEMENTS(BASE, ADDR2) ( ( (ADDR2) && (BASE) ) ? ( (ADDR2) - (BASE) ) : 0 )

// the type for an address range for data type xxx will be xxx_range
// e.g. for a float it would be float_range as in : float_range some_name
// the xxx_range struct contains 2 pointers, bot and top
// bot points to the beginning of the memory arena, top points 1 element past the last element in the arena
// type KIND_range and static constant KIND_range_null are defined

// declare a range struct with elements of type KIND with a name, e.g. RANGE(some_type) some_name ;
#define RANGE(KIND) KIND##_range
#define NO_RANGE {NULL, NULL}
#define RANGE_NULL(KIND) ((RANGE(KIND))NO_RANGE)
#define RANGE_TYPEDEF(KIND) typedef struct{ KIND *bot, *top ; } RANGE(KIND) ; static const RANGE(KIND) KIND##_range_null = RANGE_NULL(KIND) ;
#define RANGE_KIND(KIND,BOT,TOP) ( (RANGE(KIND)) { (void *)(BOT) , (void *)(TOP) } )

#define RANGE_CAST(R,KIND) (RANGE(KIND)){(KIND *)(R.bot), (KIND *)(R.top) }

// some predefined/generic address ranges, using signed and unsigned integers
RANGE_TYPEDEF(int8_t) ;
RANGE_TYPEDEF(uint8_t) ;                 // uint8_t_range
typedef RANGE(uint8_t) RANGE(byte) ;     // byte range

RANGE_TYPEDEF(int16_t) ;
RANGE_TYPEDEF(uint16_t) ;
typedef RANGE(uint16_t) RANGE(hword) ;   // halfword range

RANGE_TYPEDEF(int32_t) ;
RANGE_TYPEDEF(uint32_t) ;
typedef RANGE(uint32_t) RANGE(word) ;    // word range

RANGE_TYPEDEF(int64_t) ;
RANGE_TYPEDEF(uint64_t) ;
typedef RANGE(uint64_t) RANGE(dword) ;   // doubleword range

RANGE_TYPEDEF(void) ;                    // typeless
typedef void_range RANGE(address) ;      // generic address range, synonym of void_range

// get address of the first (bottom) element in the range
#define RANGE_BOT(R) ( PTR((R).bot) )

// get address of last element still within the range, NULL is top or bot is NULL
#define RANGE_LIMIT(R) ( ((R).top && (R).bot) ? ( PTR((R).top) - sizeof((R).bot[0]) ) : NULL )

// get address of element just above the range
#define RANGE_TOP(R) ( PTR((R).top) )

// range validity check
#define VALID_RANGE(R)   ( ((R).bot != NULL) && ((R).top != NULL) && (RANGE_TOP(R) >  RANGE_BOT(R)) )
#define INVALID_RANGE(R) ( ((R).bot == NULL) || ((R).top == NULL) || (RANGE_TOP(R) <= RANGE_BOT(R)) )

// offset of address in range in bytes, 0 if any argument is NULL
#define RANGE_OFFSET(R,ADR) ( ( (ADR) && RANGE_BOT(R) ) ? (PTR(ADR) - RANGE_BOT(R)) : 0 )

// space available above address in range (in bytes), 0 if any argument is NULL
#define RANGE_AVAIL(R,ADR)  ( ( RANGE_TOP(R) && (ADR) ) ? (RANGE_TOP(R) - PTR((ADR)+1)) : 0 )

// get number of bytes in a range, 0 if any argument is NULL
#define RANGE_BYTES(R) ( PTR_OFFSET( (R).bot ,  (R).top ) )

// get number of elements in a range (element type tells element size), 0 if any argument is NULL
#define RANGE_ELEMENTS(R) PTR_ELEMENTS((R).bot , (R).top)

// is address range BOT -> TOP entirely within range R0 (accounts for element size of TOP)
#define IN_RANGE(R0, BOT, TOP) ( (PTR(BOT) >= RANGE_BOT(R0)) && (PTR((TOP)+1) <= RANGE_TOP(R0)) )

// is range R2 entirely within range R0 (a subrange of R0)
#define IS_SUB_RANGE(R0, R2) ( (RANGE_BOT(R0) <= RANGE_BOT(R2)) && (RANGE_top(R0) >= RANGE_TOP(R2)) )

// get the size of a range element in bytes, 0 if (R).bot is NULL
#define ELEMENT_SIZE(R) ( (R).bot ? (sizeof((R).bot[0]) / sizeof(uint8_t)) : 0 )

// set address of (bottom) in a range
#define SET_RANGE_BOT(R, BOT)  { (R).bot = (void *)(BOT) ; }

// set address of (top) in a range , NULL if TOP or R.bot is NULL
#define SET_RANGE_TOP(R, TOP)  { (R).top = (void *)( ( PTR(TOP) && (R).bot ) ? (PTR(TOP)) : NULL ) ; }

// set top and bottom of range
#define SET_RANGE(R, BOT, TOP) { SET_RANGE_BOT(R, (BOT)) ; SET_RANGE_TOP(R, (TOP)) ; }

// set top address of range to base address + space to accomodate N elements, NULL if (R).bot is NULL
#define SET_RANGE_ELEMENTS(R, N)  { (R).top = (R).bot ? ((R).bot + N) : NULL ; }

// set top address of range to base address + size bytes, NULL if (R).bot is NULL
#define SET_RANGE_BYTES(R, BYTES) { (R).top = (R).bot ? SET_PTR_OFFSET( (R).bot, (BYTES) ) : NULL ; }

// set bottom address and top addresses of a range (accomodate up to BYTES bytes), { NULL, NULL } if BOT is NULL
#define SET_BYTE_RANGE(R, BOT, BYTES) { SET_RANGE_BOT(R, BOT) ; SET_RANGE_BYTES(R, BYTES) ; }

#endif
