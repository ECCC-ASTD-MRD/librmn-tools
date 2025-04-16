//
// Copyright (C) 2025  Environnement Canada
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
//     M. Valin,   Recherche en Prevision Numerique, 2025
//

#if ! defined(IEEE_FUNCTIONS)
#define IEEE_FUNCTIONS

#include <stdint.h>
#include <rmn/data_kind.h>

// sign, exponent, mantissa from 32 bit float
typedef struct{
  int16_t s ;
  int16_t e ;
  int32_t m ;
}fp32_sem ;

// get mantissa from float value z
static inline int fp32_mant(float z){
  union{ int i ; float f ; } u ;
  u.f = z ;
  return (u.i & 0x7FFFFF) ;
}

// get sign of z
static inline int fp32_sign(float z){
  union{ int i ; float f ; } u ;
  u.f = z ;
  return (u.i >> 31) ;
}

// get unbiased exponent from z
static inline int fp32_exp(float z){
  union{ int i ; float f ; } u ;
  u.f = z ;
  return ((u.i >> 23) & 0xFF) -127 ;
}

// split 32 bit float z into sign, exponent (unbiased), and mantissa
static inline fp32_sem fp32_to_sem(float z){
  fp32_sem r ;
  r.s = fp32_sign(z) ;
  r.e = fp32_exp(z) ;
  r.m = fp32_mant(z) ;
  return r ;
}

// is z a NaN (not a number) ?
static inline int fp32_isnan(float z){
  union{ int i ; float f ; } u ;
  u.f = z ;
  if((u.i & 0x7FFFFF) == 0) return 0 ;       // infinity
  return (((u.i >> 23) & 0xFF) == 0xFF) ;    // mantissa != 0 and exponent == 0xFF
}

// is z equal to infinity ?
static inline int fp32_isinf(float z){
  union{ int i ; float f ; } u ;
  u.f = z ;
  if((u.i & 0x7FFFFF) != 0) return 0 ;       // NaN or valid float
  return (((u.i >> 23) & 0xFF) == 0xFF) ;    // mantissa == 0 and exponent == 0xFF
}

// generate a "quiet" or a "signaling" NaN
static inline float fp32_nan(int signaling){
  union{ int i ; float f ; } r ;
  r.i = 0X7F800001 | (signaling << 22) ;
  return r.f ;
}

// truncate z to the power of 2 <= z
static inline float fp32_pow2_trunc(float z){
  union{ int i ; float f ; } r ;
  r.f = z ;
  r.i &= 0xFF800000 ;   // get rid of mantissa
  return r.f ;
}

// round z to the nearest power of 2
static inline float fp32_pow2_round(float z){
  union{ int i ; float f ; } r ;
  r.f = z ;
  r.i += 0x00400000 ;   // round up in value
  r.i &= 0xFF800000 ;   // get rid of mantissa
  return r.f ;
}

// return 2.0 to the power p
static inline float fp32_pow2(int p){
  union{ int i ; float f ; } r ;
  r.i = (p <=127 && p >= -127) ? ((p +127) << 23) : 0X7F800001 ;
  return r.f ;
}

// re-create 32 bit float from 3 values (sign, unbiased exponent, mantissa)
static inline float fp32_from_i3(int s, int e, int m){
  union{ int i ; float f ; } r ;
  r.i = s ; r.i <<= 31 ;      // MSB is sign
  e = ((e + 127) & 0xFF) ;    // add bias to exponent
  r.i |= (e << 23) ;          // install exponent
  m &= 0x7FFFFF ;             // only use lower 23 bits of mantissa
  r.i |= m ;                  // install mantissa
  return r.f ;
}

// re-create 32 bit float from sign/exponent/mantissa struct
static inline float fp32_from_sem(fp32_sem sem){
  return fp32_from_i3(sem.s, sem.e, sem.m) ;
}

// transform a float into a fake signed integer (comparison order preserving)
static inline int32_t fp32_as_int(float f){
  iuf32_t iuf ;
  iuf.f = f ;
  return (iuf.i & 0x7FFFFFFF) ^ (iuf.i >> 31) ;
}

// restore float from fake integer representing float
static inline float int_as_fp32(int32_t fake){
  iuf32_t iuf ;
  iuf.i = ((fake >> 31) ^ fake) | (fake & 0x80000000) ;
  return iuf.f ;
}

#endif
