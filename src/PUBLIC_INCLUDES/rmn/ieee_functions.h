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

typedef struct{
  int16_t s ;
  int16_t e ;
  int32_t m ;
}fp32_sem ;

int fp32_mant(float z){
  union{ int i ; float f ; } u ;
  u.f = z ;
  return (u.i & 0x7FFFFF) ;
}

int fp32_sign(float z){
  union{ int i ; float f ; } u ;
  u.f = z ;
  return (u.i >> 31) ;
}

int fp32_exp(float z){
  union{ int i ; float f ; } u ;
  u.f = z ;
  return ((u.i >> 23) & 0xFF) -127 ;
}

fp32_sem fp32_to_sem(float z){
  fp32_sem r ;
  r.s = fp32_sign(z) ;
  r.e = fp32_exp(z) ;
  r.m = fp32_mant(z) ;
  return r ;
}

float fp32_from_i3(int s, int e, int m){
  union{ int i ; float f ; } r ;
  r.i = s ; r.i <<= 31 ;
  e = ((e + 127) & 0xFF) ;
  r.i |= (e << 23) ;
  m &= 0x7FFFFF ;
  r.i |= m ;
  return r.f ;
}

float fp32_from_sem(fp32_sem sem){
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
