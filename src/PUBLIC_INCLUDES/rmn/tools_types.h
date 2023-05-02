// Hopefully useful code for C
// Copyright (C) 2022  Recherche en Prevision Numerique
//
// This code is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation,
// version 2.1 of the License.
//
// This code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
#if ! defined(TOOLS_TYPES)
#define TOOLS_TYPES

#include <stdint.h>
#include <stddef.h>

// union covering most popular data types, length in memory is 64 bits
typedef union{      // 8/16/32/64 signed/unsigned integer, float, double, pointer to previous
  uint8_t  u8   ;   // 8/16/32/64 unsigned integer
  uint16_t u16  ;
  uint32_t u32  ;
  uint32_t u    ;
  uint64_t u64  ;
  uint64_t ul   ;
  int8_t   i8   ;   // 8/16/32/64 signed integer
  int16_t  i16  ;
  int32_t  i32  ;
  int32_t  i    ;
  int64_t  i64  ;
  int64_t  l    ;
  float    f    ;   // 32 bit float
  double   d    ;   // 64 bit float
  void    *p    ;   // generic memory pointer
  void    *u8p  ;   // pointer to 8 bit unsigned integer
  void    *i8p  ;   // pointer to 8 bit signed integer
  void    *u16p ;   // pointer to 16 bit unsigned integer
  void    *i16p ;   // pointer to 16 bit signed integer
  void    *u32p ;   // pointer to 32 bit unsigned integer
  void    *i32p ;   // pointer to 32 bit signed integer
  void    *u64p ;   // pointer to 64 bit unsigned integer
  void    *i64p ;   // pointer to 64 bit signed integer
  void    *fp   ;   // pointer to 32 bit float
  void    *dp   ;   // pointer to 32 bit double
} AnyType ;

typedef union{     // float | (un)signed 32 bit integer, length in memory is 32 bits
  uint8_t  u8   ;   // 8/16/32 unsigned integer
  uint16_t u16  ;
  uint32_t u32  ;
  uint32_t u    ;
  int8_t   i8   ;   // 8/16/32 signed integer
  int16_t  i16  ;
  int32_t  i32  ;
  int32_t  i    ;
  float    f    ;   // 32 bit float
} FloatInt ;

typedef union{     // float | (un)signed 32 bit integer, length in memory is 32 bits
  uint8_t  u8   ;   // 8/16/32 unsigned integer
  uint16_t u16  ;
  uint32_t u32  ;
  uint32_t u    ;
  int8_t   i8   ;   // 8/16/32 signed integer
  int16_t  i16  ;
  int32_t  i32  ;
  int32_t  i    ;
  float    f    ;   // 32 bit float
} AnyType32 ;

typedef struct{    // pair of signed 32 bit integers
  int32_t t[2] ;
} IntPair ;

typedef struct{    // pair of floats
  float t[2] ;
} FloatPair ;

// some properties of a float array, 32 bits total
typedef struct{
  uint32_t emax:8 ,  // largest float (absolute value) exponent
           emin:8 ,  // smallest non zero float (absolute value) exponent
           allp:1 ,  // 1 if all non negative numbers
           allm:1 ,  // 1 if all negative numbers
           zero:1 ,  // 1 if zero values detected
           mima:1 ,  // 1 if same exponent and no zero
           errf:1 ,  // error flag
           n8x8:1 ,  // full 8x8 block
           xtra:2 ,  // reserved for future use
           npti:4 ,  // nuber of points in row (1-8), 0 means unknown
           nptj:4 ;  // number of rows (1-8), 0 means unknown
} ieee_prop_p ;

typedef union{    // the union allows to transfer the whole contents in one shot
  ieee_prop_p p ;
  uint32_t u ;
} ieee_prop ;

static ieee_prop ieee_prop_0 = { 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 } ;

// some properties of a float array, 64 bits total
typedef struct{
  uint64_t emax:8 ,  // largest float (absolute value) exponent
           emin:8 ,  // smallest non zero float (absolute value) exponent
           bias:16 ,
           allp:1 ,  // 1 if all non negative numbers (or unsigned if integers)
           allm:1 ,  // 1 if all negative numbers
           zero:1 ,  // 1 if zero values detected
           mima:1 ,  // 1 if same exponent and no zero
           errf:1 ,  // error flag
           n_08:1 ,  // full 8x8 block
           n_64:1 ,  // full 64x64 block
           nbit:5 ,  // number of significant bits
           ieee:1 ,  // IEEE Float (1) or integer (0)
           xtra:3 ,  // reserved for future use
           npti:8 ,  // nuber of points in row (1-63), 0 means unknown or full
           nptj:8 ;  // number of rows (1-63), 0 means unknown or full
} ieee_prop_64_p ;

typedef union{    // the union allows to transfer the whole contents in one shot
  ieee_prop_64_p p ;
  uint64_t u ;
} ieee_prop_64 ;

static ieee_prop_64 ieee_prop_64_0 = { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 } ;

#endif
