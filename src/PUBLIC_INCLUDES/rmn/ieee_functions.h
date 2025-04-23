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

#if ! defined(IEEE_FUNCTIONS_DEFINED)
#define IEEE_FUNCTIONS_DEFINED

#include <stdint.h>

// sign, exponent, mantissa from 32 bit float
typedef struct{
  int16_t  s ;
  int16_t  e ;
  uint32_t m ;
}fp32_sem ;

// get sign of float value z
static inline int32_t fp32_sign(float z){
  union{ int32_t i ; uint32_t u ; float f ; } r ;
  r.f = z ;
  return (r.i >> 31) ;  // will return 0 or -1
}

// get unbiased exponent from z
static inline int32_t fp32_exp(float z){
  union{ int32_t i ; uint32_t u ; float f ; } r ;
  r.f = z ;
  return ((r.i >> 23) & 0xFF) -127 ;
}

// get mantissa from float value z
static inline uint32_t fp32_mant(float z){
  union{ int32_t i ; uint32_t u ; float f ; } r ;
  r.f = z ;
  return (r.u & 0x7FFFFF) ;
}

// is z a NaN (not a number) ?
static inline int fp32_isnan(float z){
  union{ int32_t i ; uint32_t u ; float f ; } r ;
  r.f = z ;
  if((r.i & 0x7FFFFF) == 0) return 0 ;       // infinity or valid float
  return (((r.i >> 23) & 0xFF) == 0xFF) ;    // mantissa != 0 and exponent == 0xFF
}

// is z equal to infinity ?
static inline int fp32_isinf(float z){
  union{ int32_t i ; uint32_t u ; float f ; } r ;
  r.f = z ;
  if((r.i & 0x7FFFFF) != 0) return 0 ;       // NaN or valid float
  return (((r.i >> 23) & 0xFF) == 0xFF) ;    // mantissa == 0 and exponent == 0xFF
}

// generate a "quiet" or a "signaling" NaN
// signaling LSB == 1 : generate a "signaling" NaN
// signaling LSB == 0 : generate a "quiet" NaN
static inline float fp32_nan(int signaling){
  union{ int32_t i ; uint32_t u ; float f ; } r ;
  r.u = 0X7F800001 | ((signaling & 1) << 22) ;
  return r.f ;
}

// truncate |z| to the power of 2 <= |z|, keep sign intact
static inline float fp32_pow2_trunc(float z){
  union{ int32_t i ; uint32_t u ; float f ; } r ;
  r.f = z ;
  r.u &= 0xFF800000u ;   // get rid of mantissa
  return r.f ;
}

// round |z| to the nearest power of 2, keep sign intact
static inline float fp32_pow2_round(float z){
  union{ int32_t i ; uint32_t u ; float f ; } r ;
  r.f = z ;
  r.u += 0x00400000 ;   // round up in absolute value
  r.u &= 0xFF800000 ;   // get rid of mantissa
  return r.f ;
}

// return +2.0 to the power p
// if p is too large or too small, return "quiet" NaN
// p == -127 will return 0.0f
static inline float fp32_pow2(int p){
  union{ int32_t i ; uint32_t u ; float f ; } r ;
  r.u = (p <=127 && p >= -127) ? ((p + 127) << 23) : 0X7F800001u ;
  return r.f ;
}

// re-create 32 bit float from 3 integer values (sign, unbiased exponent, mantissa)
static inline float fp32_from_i3(int32_t s, int32_t e, uint32_t m){
  union{ int32_t i ; uint32_t u ; float f ; } r ;
  if(e > 128 || e < -127) return fp32_nan(0) ;  // bad exponent (e + 127 outside of 0->255)
  if(m & 0xF8000000u)     return fp32_nan(0) ;  // bad mantissa ( > 0x7FFFFFu) ;
  r.i = s ; r.i <<= 31 ;      // MSB is sign, from LSB of s
  e = (e + 127) ;             // add bias to exponent
  r.i |= (e << 23) ;          // install exponent
  r.i |= m ;                  // install mantissa
  return r.f ;
}

// split 32 bit float z into sign, exponent (unbiased), and mantissa
static inline fp32_sem fp32_to_sem(float z){
  fp32_sem r ;
  r.s = fp32_sign(z) ;
  r.e = fp32_exp(z) ;
  r.m = fp32_mant(z) ;
  return r ;
}

// re-create 32 bit float from sign/exponent/mantissa struct
static inline float fp32_from_sem(fp32_sem sem){
  return fp32_from_i3(sem.s, sem.e, sem.m) ;
}

// transform a float into a fake signed integer (comparison order preserving)
static inline int32_t fp32_as_int(float f){
  union{ int32_t i ; uint32_t u ; float f ; } r ;
  r.f = f ;
  return (r.i & 0x7FFFFFFF) ^ (r.i >> 31) ;
}

// restore float from fake integer representing float
static inline float int_as_fp32(int32_t fake){
  union{ int32_t i ; uint32_t u ; float f ; } r ;
  r.i = ((fake >> 31) ^ fake) | (fake & 0x80000000) ;
  return r.f ;
}

#endif
