//  LIBRMN - Library of hopefully useful routines for C and FORTRAN
//  Copyright (C) 2022  Recherche en Prevision Numerique
//                      Environnement Canada
// 
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation,
//  version 2.1 of the License.
// 
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Library General Public License for more details.
//

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <float.h>

#include <rmn/data_info.h>

uint32_t INT32_maxa(limits_w32 l32){
  int32_t maxs = l32.i.maxs ;
  int32_t mins = l32.i.mins ;
  maxs = (maxs < 0) ? -maxs : maxs ;          // ABS(maxs)
  mins = (mins < 0) ? -mins : mins ;          // ABS(mins)
  return (maxs > mins) ? maxs : mins ;        // MAX(ABS(maxs), ABS(mins))
}

// replace "missing" values in 32 bit array
// f    [INOUT]  array of 32 bit values
// np      [IN]  number of data items
// spval   [IN]  pointer to 32 bit " missing value" pattern
// mmask   [IN]  missing mask (bits having the value 1 will be ignored for missing values)
// pad     [IN]  pointer to 32 bit value to be used as "missing" replacement
int W32_replace_missing(void * restrict f, int np, void *spval, uint32_t mmask, void *pad){
  uint32_t *fu = (uint32_t *) f, missf, *fill = (uint32_t *) pad, *miss = (uint32_t *) spval ;
  int i ;

  if(f == NULL || spval == NULL || mmask == 0xFFFFFFFFu || pad == NULL) return -1 ;  // nothing can be done, error
  mmask = ~mmask ;
  missf = mmask & *miss ;                            // masked "missing" value
  for(i=0 ; i<np ; i++){
    fu[i] = ( (fu[i] & mmask) == missf ) ? *fill : fu[i] ;
  }
  return np ;
}

// get unsigned integer 32 extrema
// f     [IN]  32 bit unsigned integer array
// np    [IN]  number of data items
// l    [OUT]  extrema
limits_w32 UINT32_extrema(void * restrict f, int np){
  uint32_t *fu = (uint32_t *) f ;
  int i ;
  uint32_t maxa, mina, min0, tu, t0 ;
  limits_w32 l ;

  maxa = mina = fu[0] ;
  min0 = 0xFFFFFFFFu ;
  for(i=0 ; i<np ; i++){
    tu = fu[i] ;
    t0 = (tu == 0) ? 0xFFFFFFFFu : tu ;
    mina = (tu < mina) ? tu : mina ;
    min0 = (t0 < min0) ? t0 : min0 ;
    maxa = (tu > maxa) ? tu : maxa ;
  }
  l.i.maxs = maxa ;  // signed makes no sense for unsigned integers, using unsigned value
  l.i.mins = mina ;  // signed makes no sense for unsigned integers, using unsigned value
  l.i.mina = mina ;
  l.i.min0 = min0 ;
  l.i.maxa = maxa ;
  return l ;
}

// get integer 32 extrema (signed and absolute value)
// f     [IN]  32 bit signed integer array
// np    [IN]  number of data items
// l    [OUT]  extrema
limits_w32 INT32_extrema(void * restrict f, int np){
  int32_t *fs = (int32_t *) f ;
  int i ;
  uint32_t maxa, mina, min0, tu, t0 ;
  int32_t maxs, mins, ts ;
  limits_w32 l ;

  maxa = mina = (fs[0] < 0) ? -fs[0] : fs[0] ;
  maxs = mins = fs[0] ;
  min0 = 0x7FFFFFFF ;
  for(i=0 ; i<np ; i++){
    ts = fs[i] ;
    tu = (ts < 0) ? -ts : ts ;
    t0 = (tu == 0) ? 0x7FFFFFFF : tu ;
    mina = (tu < mina) ? tu : mina ;
    min0 = (t0 < min0) ? t0 : min0 ;
    maxs = (ts > maxs) ? ts : maxs ;
    mins = (ts < mins) ? ts : mins ;
  }
  l.i.maxs = maxs ;
  l.i.mins = mins ;
  l.i.mina = mina ;
  l.i.min0 = min0 ;
  l.i.maxa = INT32_maxa(l) ;
  return l ;
}

// get IEEE 32 extrema (signed and absolute value)
// f       [IN]  32 bit integer array
// np      [IN]  number of data items
// spval   [IN]  pointer to 32 bit " missing value" pattern
// mmask   [IN]  missing mask (bits having the value 1 will be ignored for missing values)
// pad     [IN]  pointer to 32 bit value to be used as "missing" replacement
// return an extrema limits_f struct
//
// values with bit pattern (~mmask) & *spval wil be treated as "missing" and ignored
// if mmask == 0 or spval == NULL, the "missing" check is inactivated
limits_w32 INT32_extrema_missing(void * restrict f, int np, void *spval, uint32_t mmask, void *pad){
  int32_t *fs = (int32_t *) f ;
  int i ;
  uint32_t maxa, mina, min0, tu, t0 ;
  int32_t maxs, mins, ts ;
  limits_w32 l ;

  maxa = mina = (fs[0] < 0) ? -fs[0] : fs[0] ;
  maxs = mins = fs[0] ;
  min0 = 0x7FFFFFFF ;
  for(i=0 ; i<np ; i++){
    ts = fs[i] ;
    tu = (fs[i] < 0) ? -fs[i] : fs[i] ;
    t0 = (tu == 0) ? 0x7FFFFFFF : tu ;
    mina = (tu < mina) ? tu : mina ;
    min0 = (t0 < min0) ? t0 : min0 ;
    maxs = (ts > maxs) ? ts : maxs ;
    mins = (ts < mins) ? ts : mins ;
  }
  l.i.maxs = maxs ;
  l.i.mins = mins ;
  l.i.mina = mina ;
  l.i.min0 = min0 ;
  l.i.maxa = INT32_maxa(l) ;
  return l ;
}

uint32_t IEEE32_maxa(limits_w32 l32){
  uint32_t maxs = l32.i.maxs & 0x7FFFFFFF ;          // ABS(maxs)
  uint32_t mins = l32.i.mins & 0x7FFFFFFF ;          // ABS(mins)
  return (maxs > mins) ? maxs : mins ;               // MAX(ABS(maxs), ABS(mins))
}

// get IEEE 32 extrema (signed and absolute value)
// f     [IN]  32 bit IEEE float array
// np    [IN]  number of data items
// return an extrema limits_f struct
// N.B. this code only works for IEEE754 32 bit floats, all processing is done in INTEGER mode
limits_w32 IEEE32_extrema(void * restrict f, int np){
  uint32_t *fu = (uint32_t *) f ;
  int32_t *fs = (int32_t *) f ;
  int i ;
  uint32_t mina, min0, t0, tu ;
  int32_t maxs, mins, ts ;
  limits_w32 l32 ;

  mina = fu[0] & 0x7FFFFFFF ;
  ts = (fs[0] & 0x7FFFFFFF) ^ (fs[0] >> 31) ;
  maxs = mins = ts ;
  min0 = 0x7FFFFFFF ;
  for(i=0 ; i<np ; i++){
    tu = fu[i] & 0x7FFFFFFF ;
    t0 = (tu == 0) ? 0x7FFFFFFF : tu ;
    ts = (fs[i] & 0x7FFFFFFF) ^ (fs[i] >> 31) ;
    mina = (tu < mina) ? tu : mina ;
    min0 = (t0 < min0) ? t0 : min0 ;
    maxs = (ts > maxs) ? ts : maxs ;
    mins = (ts < mins) ? ts : mins ;
  }
  l32.i.maxs = (maxs & 0x7FFFFFFF) ^ (maxs >> 31) ;  // maximum float value (signed)
  l32.i.mins = (mins & 0x7FFFFFFF) ^ (mins >> 31) ;  // minimum float value (signed)
  l32.i.mina = mina ;                                // smallest float absolute value
  l32.i.min0 = min0 ;                                // smallest float non zero absolute value
//   maxs = l32.i.maxs & 0x7FFFFFFF ;                   // ABS(maximum float value)
//   mins = l32.i.mins & 0x7FFFFFFF ;                   // ABS(minimum float value)
//   l32.i.maxa = (maxs > mins) ? maxs : mins ;         // largest float absolute value
  l32.i.maxa = IEEE32_maxa(l32) ;                    // largest float absolute value
  return l32 ;
}

// get IEEE 32 extrema (signed and absolute value)
// f       [IN]  32 bit IEEE float array
// np      [IN]  number of data items
// spval   [IN]  pointer to 32 bit " missing value" pattern
// mmask   [IN]  missing mask (bits having the value 1 will be ignored for missing values)
// pad     [IN]  pointer to 32 bit value to be used as "missing" replacement
// return an extrema limits_f struct
//
// values with bit pattern (~mmask) & *spval wil be treated as "missing" and ignored
// if mmask == 0 or spval == NULL, the "missing" check is inactivated
// N.B. this code only works for IEEE754 32 bit floats, all processing is done in INTEGER mode
limits_w32 IEEE32_extrema_missing(void * restrict f, int np, void *spval, uint32_t mmask, void *pad){
  uint32_t *fu = (uint32_t *) f ;
  int i ;
  uint32_t mina, min0, tu, t0, good, missf, *miss = (uint32_t *)spval, *fill = (uint32_t *)pad  ;
  int32_t maxs, mins, ts ;
  limits_w32 l32 ;

  mmask = ~mmask ;
  if(spval == NULL || mmask == 0) return IEEE32_extrema(f, np) ;   // "missing" option not used

  mins = mina = min0 = 0x7FFFFFFFu ;                 // largest positive value
  maxs = 1 << 31 ;                                   // largest negative value
  missf = mmask & *miss ;                            // masked "missing" value
  if(fill == NULL){                                  // no explicit replacement value
    good = fu[0] ;                                   // in case no non "missing" value found
    for(i=0 ; i<np ; i++){                           // find first non "missing" value
      good = fu[i] ;
      if( (good & mmask) != missf) break ;           // non "missing" value found
    }
  }else{
    good = *fill ;                                   // we have an explicit replacement value
  }
  for(i=0 ; i<np ; i++){
    tu = fu[i] ;
    tu = (tu & mmask) == missf ? good : tu ;         // replace "missing" value with "good" value
    t0 = (tu == 0) ? 0x7FFFFFFF : tu ;               // if tu == 0, set t0 to largest value to compute min0
    ts = (tu & 0x7FFFFFFF) ^ ((int32_t)tu >> 31) ;   // fake signed integer from float
    tu = tu & 0x7FFFFFFF ;                           // ABS(tu)
    mina = (tu < mina) ? tu : mina ;                 // smallest absolute value
    min0 = (t0 < min0) ? t0 : min0 ;                 // smallest non zero absolute value
    maxs = (ts > maxs) ? ts : maxs ;                 // maximum signed value
    mins = (ts < mins) ? ts : mins ;                 // minimum signed value
  }
  l32.i.maxs = (maxs & 0x7FFFFFFF) ^ (maxs >> 31) ;  // restore float from fake signed integer
  l32.i.mins = (mins & 0x7FFFFFFF) ^ (mins >> 31) ;  // restore float from fake signed integer
  l32.i.mina = mina ;                                // no restore needed, mina/min0 are >= 0
  l32.i.min0 = min0 ;
//   maxs = l32.i.maxs & 0x7FFFFFFF ;                   // ABS(maxs)
//   mins = l32.i.mins & 0x7FFFFFFF ;                   // ABS(mins)
//   l32.i.maxa = (maxs > mins) ? maxs : mins ;         // MAX(ABS(maxs), ABS(mins))
  l32.i.maxa = IEEE32_maxa(l32) ;                    // largest float absolute value
  return l32 ;                                     // return float part of the union
}

// if for some reason some vectorizers do a bad job in plain C, an explicit SIMD X86 AVX2 version is supplied
#if defined(__AVX2__) && defined(__x86_64__)
#include <immintrin.h>

#define LOWER128(v256)         _mm256_extractf128_si256((__m256i) v256, 0)
#define UPPER128(v256)         _mm256_extractf128_si256((__m256i) v256, 1)
#define REDUCE256(op, v)       op(LOWER128(v), UPPER128(v))
#define REDUCE0123(op, v128)   op(v128, _mm_shuffle_epi32 ((__m128i) v128, 0b00001110))
#define REDUCE01(op, v128)     op(v128, _mm_shuffle_epi32 ((__m128i) v128, 0b00000001))
#define REDUCE(op, v256, v128) v128 = REDUCE256(op, v256) ; v128 = REDUCE0123(op, v128) ; v128 = REDUCE01(op, v128) ;

// get IEEE 32 extrema (signed and absolute value)
// f       [IN]  32 bit IEEE float array
// np      [IN]  number of data items
// spval   [IN]  pointer to 32 bit " missing value" pattern
// mmask   [IN]  missing mask (bits having the value 1 will be ignored for missing values)
// pad     [IN]  pointer to 32 bit value to be used as "missing" replacement
// return an extrema limits_f struct
//
// values with bit pattern (~mmask) & *spval wil be treated as "missing" and ignored
// if mmask == 0 or spval == NULL, the "missing" check is inactivated
limits_w32 IEEE32_extrema_missing_simd(void * restrict f, int np, void *spval, uint32_t mmask, void *pad){
  int i0, i, np7 ;
  uint32_t *fu = (uint32_t *) f, *fill = (uint32_t *)pad, *miss = (uint32_t *)spval ;
  limits_w32 l32 ;
  uint32_t tmp[8], good, missf ;
  __m256i vmina256, vmins256, vmaxs256, vmin0256 ;
  __m128i vmina128, vmins128, vmaxs128, vmin0128 ;
  __m256i vzero, vones, vsign, vmiss, vmask, vf256, vt256, vx256, vb256, vnext, vg256, vz256 ;

  mmask = ~mmask ;
  if(spval == NULL || mmask == 0) return IEEE32_extrema(f, np) ;

  missf = mmask & *miss ;
  if(fill == NULL){
    good = fu[0] ;
    for(i=0 ; i<np ; i++){
      good = fu[i] ;
      if( (good & mmask) != missf) break ;
    }
  }else{
    good = *fill ;
  }
  vg256 = _mm256_set1_epi32(good) ;            // value used to replace "missing" values

  vzero = _mm256_xor_si256(vzero, vzero) ;
  vones = _mm256_cmpeq_epi32(vzero, vzero) ;
  vsign = _mm256_srli_epi32(vones, 1) ;
  vmiss = _mm256_set1_epi32(mmask & *miss) ;   // missing & mmask
  vmask = _mm256_set1_epi32(mmask) ;           //mmask

  vmins256 = vsign ;                           // 0x7FFFFFFF +HUGE
  vmin0256 = vsign ;
  vmaxs256 = _mm256_slli_epi32(vones, 31) ;    // 0x80000000 -HUGE
  vmina256 = vsign ;                           // 0x7FFFFFFF +HUGE

  np7 = (np&7) ? (np&7) : 8 ;
  if(np < 8){                                                                // short case
    for(i=0 ; i<np7 && i<8 ; i++) tmp[i] = fu[i] ;                           // np7 values
    for(    ; i<8         ; i++) tmp[i] = *miss ;                            // fill with missing value
    vf256    = _mm256_loadu_si256((__m256i *)(tmp)) ;                        // first chunk
  }else{
    vf256    = _mm256_loadu_si256((__m256i *)(fu)) ;                         // first chunk
  }

  vb256    = _mm256_cmpeq_epi32(vmiss, _mm256_and_si256(vf256, vmask)) ;     // find special values
  vf256    = _mm256_blendv_epi8(vf256, vg256 , vb256) ;                      // replace special value with good
  for(i0 = np7 ; i0 < np-7 ; i0 += 8){
    vnext    = _mm256_loadu_si256((__m256i *)(fu+i0)) ;                      // prefetch next chunk
    vb256    = _mm256_cmpeq_epi32(vmiss, _mm256_and_si256(vnext, vmask)) ;   // find special values
    vnext    = _mm256_blendv_epi8(vnext, vg256 , vb256) ;                    // replace special value with good

    vt256    = _mm256_and_si256(vsign, vf256) ;                              // get rid of sign
    vb256    = _mm256_cmpeq_epi32(vt256, vzero) ;                            // find zero values
    vz256    = _mm256_blendv_epi8(vt256, vsign, vb256) ;                     // replace with vsign
    vx256    = _mm256_xor_si256(vt256, _mm256_srai_epi32(vf256,31)) ;        // fake integer representing float
    vf256    = vnext ;
    vmaxs256 = _mm256_max_epi32(vmaxs256 , vx256) ;
    vmins256 = _mm256_min_epi32(vmins256 , vx256) ;
    vmina256 = _mm256_min_epu32(vmina256 , vt256) ;
    vmin0256 = _mm256_min_epu32(vmin0256 , vz256) ;
  }
  vt256    = _mm256_and_si256(vsign, vf256) ;                                // last chunk (special values already plugged)
  vx256    = _mm256_xor_si256(vt256, _mm256_srai_epi32(vf256,31)) ;          // fake integer representing float
  vmaxs256 = _mm256_max_epi32(vmaxs256 , vx256) ;
  vmins256 = _mm256_min_epi32(vmins256 , vx256) ;
  vmina256 = _mm256_min_epu32(vmina256 , vt256) ;

  REDUCE(_mm_max_epi32, vmaxs256, vmaxs128) ;
  _mm_storeu_si32(&(l32.i.maxs), vmaxs128) ;
  l32.i.maxs = (l32.i.maxs & 0x7FFFFFFF) ^ (l32.i.maxs >> 31) ;    // restore float from fake integer
  REDUCE(_mm_min_epi32, vmins256, vmins128) ;
  _mm_storeu_si32(&(l32.i.mins), vmins128) ;
  l32.i.mins = (l32.i.mins & 0x7FFFFFFF) ^ (l32.i.mins >> 31) ;    // restore float from fake integer
  REDUCE(_mm_min_epu32, vmina256, vmina128) ;
  _mm_storeu_si32(&(l32.i.mina), vmina128) ;
  REDUCE(_mm_min_epu32, vmin0256, vmin0128) ;
  _mm_storeu_si32(&(l32.i.min0), vmin0128) ;
//   uint32_t maxs = l32.i.maxs & 0x7FFFFFFF ;
//   uint32_t mins = l32.i.mins & 0x7FFFFFFF ;
//   l32.i.maxa = (maxs > mins) ? maxs : mins ;
  l32.i.maxa = IEEE32_maxa(l32) ;                    // largest float absolute value

  return l32 ;
}
#else
// AVX2 not available, call the plain C version and hope for the best
limits_w32 IEEE32_extrema_missing_simd(void * restrict f, int np, void *missing, uint32_t mmask){
  return IEEE32_extrema_missing(f, np, missing, mmask) ;
}
#endif

#if defined(__GNUC__)
// #if defined(__INTEL_COMPILER) || defined(__clang__) || defined(__PGIC__)
// #undef __GCC_IS_COMPILER__
// #else
#define __GCC_IS_COMPILER__
// #endif
#endif

// use Intel compatible intrinsics for SIMD instructions
#include <with_simd.h>

#define VL 8
#define VLMASK (VL - 1)

// float_info_simple is mostly used to check C compiler vectorization capabilities
// int float_info_simple(float *zz, int ni, int lni, int nj, float *maxval, float *minval, float *minabs){
//   int i, j ;
//   float zmax, zmin, abs, amin ; ;
// 
//   zmax = zmin = zz[0] ;
//   amin = FLT_MAX ;
//   if(lni == ni) { ni = ni * nj ; nj = 1 ; }
// 
//   for(j = 0 ; j < nj ; j++){                       // loop over rows
//     for(i = 0 ; i < ni ; i++){                     // loop for a row
//       zmax = (zz[i] > zmax) ? zz[i]  : zmax ;      // highest value
//       zmin = (zz[i] < zmin) ? zz[i]  : zmin ;      // lowest value
//       abs  = (zz[i] < 0.0f) ? -zz[i] : zz[i] ;     // absolute value
//       abs  = (abs == 0.0f)  ? amin   : abs ;       // set to amin if zero
//       amin = (abs < amin)   ? abs    : amin ;      // smallest NON ZERO absolute value
//     }
//     zz += lni ;
//   }
//   *maxval = zmax ;
//   *minval = zmin ;
//   *minabs = amin ;
//   return 0 ;
// }

// Fortran layout(lni,nj) is assumed, i index varying first
// zz     [IN]  : 32 bit floating point array
// ni     [IN]  : number of useful points along i
// lni    [IN]  : actual row length ( >= ni )
// nj     [IN]  : number of rows (along j)
// maxval [OUT] : highest value in zz
// minval [OUT] : lowest value in zz
// minabs [OUT] : smallest NON ZERO absolute value in zz
// return value : 0 (to be consistent with float_info_missing
int float_info_no_missing(float *zz, int ni, int lni, int nj, float *maxval, float *minval, float *minabs){
  int i0, i, j ;
  float z[VL] ;
  float zmax[VL], zma ;
  float zmin[VL], zmi ;
  float abs[VL], zabs ;
  float amin[VL], ami ;
  int nfold, incr;

  if(lni == ni) { ni = ni * nj ; nj = 1 ; }

  if(ni < VL) {        // less than VL elements along i
    zma = zz[0] ;
    zmi = zma ;
    ami = FLT_MAX ;
    for(j = 0 ; j < nj ; j++){
      for(i = 0 ; i < ni ; i++){
        zma   = (zz[i] > zma)  ? zz[i]  : zma ;
        zmi   = (zz[i] < zmi)  ? zz[i]  : zmi ;
        zabs  = (zz[i] < 0.0f) ? -zz[i] : zz[i] ;
        zabs  = (zabs == 0.0f) ? ami    : zabs ;
        ami   = (zabs < ami)   ? zabs   : ami ;
      }
      zz += lni ;
    }
    *minabs = ami ;    // smallest NON ZERO absolute value
    *maxval = zma ;    // highest value
    *minval = zmi ;    // lowest value

  }else{
    for(i=0 ; i<VL ; i++){
      zmin[i] = FLT_MAX ;
      zmax[i] = -zmin[i] ;
      amin[i] = FLT_MAX ;
    }
    nfold = ni & VLMASK ;                                       // ni modulo VL

    for(j = 0 ; j < nj ; j++){                                  // loop over rows
      incr = (nfold > 0) ? nfold : VL ;                         // first increment is shorter if ni not a multiple of VL
      // there may be some overlap between pass 0 and pass 1
      // elements nfold -> VL -1 will be processed twice
      // but it does not matter  for min/max/abs/absmin
      // one SIMD pass is cheaper than a scalar loop
      for(i0 = 0 ; i0 < ni-VL+1 ; i0 += incr){                  // loop for a row
        for(i=0 ; i<VL ; i++){                                  // blocks of VL values
          z[i]      = zz[i0+i] ;
          zmax[i]   = (z[i] > zmax[i]) ? z[i]  : zmax[i] ;      // highest value of z
          zmin[i]   = (z[i] < zmin[i]) ? z[i]  : zmin[i] ;      // lowest value of z 
          abs[i]    = (z[i] < 0.0f)      ? -z[i]   : z[i] ;     // absolute value of z
          abs[i]    = (abs[i] == 0.0f)   ? amin[i] : abs[i] ;   // if zero, set absolute value to current minimum
          amin[i]   = (abs[i] < amin[i]) ? abs[i]  : amin[i] ;  // smallest NON ZERO absolute value
        }
        incr = VL ;
      }
      zz += lni ;
    }

    for(i=1 ; i < VL ; i++){                               // fold temporary vectors into their first element
      zmin[0] = (zmin[i] < zmin[0]) ? zmin[i] : zmin[0] ;  // fold lowest value into zmin[0]
      zmax[0] = (zmax[i] > zmax[0]) ? zmax[i] : zmax[0] ;  // fold highest value into zmax[0]
      amin[0] = (amin[i] < amin[0]) ? amin[i] : amin[0] ;  // fold smallest NON ZERO absolute value into amin[0]
    }
    *minabs = amin[0] ;    // smallest NON ZERO absolute value
    *maxval = zmax[0] ;    // highest value
    *minval = zmin[0] ;    // lowest value
  }

  return 0 ;         // number of values equal to special value
}

// as some compilers generate code with poor performance, an alternative version
// using Intel instrinsics for SIMD instructions is provided
// it is activated by adding -DWITH_SIMD to the compiler options
// Fortran layout(lni,nj) is assumed, i index varying first
// zz     [IN]  : 32 bit floating point array
// ni     [IN]  : number of useful points along i
// lni    [IN]  : actual row length ( >= ni )
// nj     [IN]  : number of rows (along j)
// maxval [OUT] : highest value in zz
// minval [OUT] : lowest value in zz
// minabs [OUT] : smallest NON ZERO absolute value in zz
// spval  [IN]  : any (value & spmask) in zz that is equal to (spval & spmask) will be IGNORED
//                should spval be a NULL pointer, there is no such value
// spmask  [IN] : mask for spval
// return value : number of elements in zz that are equal to the special value
int float_info_missing(float *zz, int ni, int lni, int nj, float *maxval, float *minval, float *minabs, float *spval, uint32_t spmask){
  int i0, i, j ;
  uint32_t *iz ;
  float sz ;
  float zmax[VL], zma ;
  float zmin[VL], zmi ;
  float zabs ;
  float amin[VL], ami ;
  int count[VL], fixcount ;
  int nfold, incr, cnt ;
  uint32_t *ispval = (uint32_t *) spval ;
  uint32_t missing ;
#if defined(__AVX2__) && defined(__x86_64__) && defined(__GCC_IS_COMPILER__) && defined(WITH_SIMD)
  __m256i vcnt, vs1, vmissing, vmask, vti ;
  __m256  v, vt, vmax, vmin, vamin, vma, vmi, vsign, vs2, vzero ;
#endif

  // if no special value, call simpler, faster function
  if(spval == NULL) return float_info_no_missing(zz, ni, lni, nj, maxval, minval, minabs) ;
  spmask = (~spmask) ;            // complement spmask

  if(lni == ni) { ni = ni * nj ; nj = 1 ; }
  missing = (*ispval) & spmask ;  // special value & mask
  fixcount = 0 ;

  if(ni < VL) {        // less than VL elements along i
    zma = -FLT_MAX ;
    zmi = FLT_MAX ;
    ami = FLT_MAX ;
    cnt = 0 ;
    for(j = 0 ; j < nj ; j++){
      iz = (uint32_t *) zz ;
      for(i = 0 ; i < ni ; i++){
        sz    = ((iz[i]  & spmask) == missing) ? zma : zz[i] ;
        zma   = (sz > zma)  ? sz  : zma ;
        sz    = ((iz[i]  & spmask) == missing) ? zmi : zz[i] ;
        zmi   = (sz < zmi)  ? sz  : zmi ;
        sz    = ((iz[i]  & spmask) == missing) ? ami : zz[i] ;
        zabs  = (sz < 0.0f)    ? -sz  : sz ;
        zabs  = (zabs == 0.0f) ? ami  : zabs ;
        ami   = (zabs < ami)   ? zabs : ami ;
        cnt  += ((iz[i]  & spmask) == missing) ? 0 : 1 ;
      }
      zz += lni ;
    }
    *minabs = ami ;    // smallest NON ZERO absolute value (FLT_MAX if all values are missing)
    *maxval = zma ;    // highest value (-FLT_MAX if all values are missing)
    *minval = zmi ;    // lowest value (FLT_MAX if all values are missing)
    count[0] = cnt ;

  }else{
    for(i=0 ; i<VL ; i++){
      zmin[i] = FLT_MAX ;
      zmax[i] = -FLT_MAX ;
      amin[i] = FLT_MAX ;
      count[i] = 0 ;
    }
    nfold = ni & VLMASK ;                                       // ni modulo VL

#if defined(__AVX2__) && defined(__x86_64__) && defined(__GCC_IS_COMPILER__) && defined(WITH_SIMD)
    vmin  = _mm256_loadu_ps(zmin) ;
    vmax  = _mm256_loadu_ps(zmax) ;
    vamin = _mm256_loadu_ps(amin) ;
    vcnt  = _mm256_loadu_si256 ((__m256i *) count) ;
    vmissing = _mm256_set1_epi32(missing) ;                     // special value & mask
    vmask = _mm256_set1_epi32(spmask) ;                         // mask for special value
    vsign = _mm256_set1_ps(-0.0f) ;
    vzero = _mm256_xor_ps(vsign, vsign) ;                       // set to 0
#endif
    for(j = 0 ; j < nj ; j++){                                  // loop over rows
      incr = (nfold > 0) ? nfold : VL ;                         // first increment is shorter if ni not a multiple of VL
      iz = (uint32_t *) zz ;
      // there may be some overlap between pass 0 and pass 1
      // elements incr -> VL -1 will be processed twice
      // but it does not matter  for min/max/abs/absmin
      // one SIMD pass is cheaper than a scalar loop
      for(i = incr ; i < VL ; i++)                              // some missing values could be counted twice
        if((iz[i]  & spmask) == missing) {                                  // scan iz[incr:VL-1] to find and count them
          fixcount++ ;
        }
      for(i0 = 0 ; i0 < ni-VL+1 ; i0 += incr){                       // loop over a row
#if defined(__AVX2__) && defined(__x86_64__) && defined(__GCC_IS_COMPILER__) && defined(WITH_SIMD)
        v = _mm256_loadu_ps(zz + i0) ;                               // get values
        vti = _mm256_and_si256((__m256i) v, vmask) ;                  // apply special value mask
        vs1 = _mm256_cmpeq_epi32(vmissing, vti) ;                     // compare with masked missing value

        vma = _mm256_blendv_ps(v, vmax, (__m256) vs1) ;              // vmax if equal to missing value
        vmi = _mm256_blendv_ps(v, vmin, (__m256) vs1) ;              // vmin if equal to missing value
        vt = _mm256_blendv_ps(v, vamin, (__m256) vs1) ;              // vamin if equal to missing value

        vmax = _mm256_max_ps(vmax, vma) ;                            // max(vma, vmax)
        vmin = _mm256_min_ps(vmin, vmi) ;                            // min(vmi, vmin)

        vt = _mm256_andnot_ps(vsign, vt) ;                           // flush sign (absolute value)
        vs2 = _mm256_cmp_ps(vt, vzero, 0) ;                          // compare for equality
        vcnt  = _mm256_sub_epi32(vcnt, vs1) ;                        // count++ where equal to missing value
        vt = _mm256_blendv_ps(vt, vamin, (__m256) vs2) ;             // vamin if 0
        vamin = _mm256_min_ps(vamin, vt) ;                           // min of absolute value
#else
        float z[VL], z1[VL], z2[VL], abs[VL] ;
        uint32_t zi[VL] ;
        for(i=0 ; i<VL ; i++){                                  // blocks of VL values
          z[i]      = zz[i0+i] ;                                // next set of values
          zi[i]     = iz[i0+i] & spmask;

          // replace zi[i] == missing with (z[i] & missing_mask) == missing
          z1[i]     = (zi[i] == missing) ? zmax[i] : z[i] ;     // if missing, set to current highest value
          zmax[i]   = (z1[i] > zmax[i])  ? z1[i]   : zmax[i] ;  // highest value of z

          z2[i]     = (zi[i] == missing) ? zmin[i] : z[i] ;     // if missing, set to current lowest value
          zmin[i]   = (z2[i] < zmin[i])  ? z2[i]   : zmin[i] ;  // lowest value of z 

          abs[i]    = (zi[i] == missing) ? amin[i] : z[i] ;     // if missing, set to current minimum
          abs[i]    = (abs[i] < 0.0f)    ? -abs[i] : abs[i] ;   // absolute value of z
          abs[i]    = (abs[i] == 0.0f)   ? amin[i] : abs[i] ;   // if zero, set absolute valkue to current minimum
          amin[i]   = (abs[i] < amin[i]) ? abs[i]  : amin[i] ;  // smallest NON ZERO absolute value

          count[i] += ( (zi[i] == missing) ? 1 : 0 ) ;
        }
#endif
        incr = VL ;
      }
      zz += lni ;
    }

#if defined(__AVX2__) && defined(__x86_64__) && defined(__GCC_IS_COMPILER__) && defined(WITH_SIMD)
    _mm256_storeu_ps(zmin, vmin) ;                         // store SIMD registers
    _mm256_storeu_ps(zmax, vmax) ;
    _mm256_storeu_ps(amin, vamin) ;
    _mm256_storeu_si256((__m256i *) count, vcnt) ;
#endif
    for(i=1 ; i < VL ; i++){                               // fold temporary vectors into their first element
      zmin[0] = (zmin[i] < zmin[0]) ? zmin[i] : zmin[0] ;  // fold lowest value into zmin[0]
      zmax[0] = (zmax[i] > zmax[0]) ? zmax[i] : zmax[0] ;  // fold highest value into zmax[0]
      amin[0] = (amin[i] < amin[0]) ? amin[i] : amin[0] ;  // fold smallest NON ZERO absolute value into amin[0]
      count[0] += count[i] ;
    }
    *minabs = amin[0] ;    // smallest NON ZERO absolute value (FLT_MAX if all values are missing)
    *maxval = zmax[0] ;    // highest value (-FLT_MAX if all values are missing)
    *minval = zmin[0] ;    // lowest value (FLT_MAX if all values are missing)
  }
  return count[0] - fixcount ;         // number of values not equal to special value
}

// Fortran layout(lni,nj) is assumed, i index varying first
// zz     [IN]  : 32 bit floating point array
// ni     [IN]  : number of useful points along i
// lni    [IN]  : actual row length ( >= ni )
// nj     [IN]  : number of rows (along j)
// maxval [OUT] : highest value in zz
// minval [OUT] : lowest value in zz
// minabs [OUT] : smallest NON ZERO absolute value in zz
// spval  [IN]  : any value in zz that is equal to spval will be IGNORED
//                should spval be a NULL pointer, there is no such value
// return value : number of elements in zz that are equal to the special value
int float_info(float *zz, int ni, int lni, int nj, float *maxval, float *minval, float *minabs, float *spval, uint32_t spmask){
  if(spval == NULL){
    return float_info_no_missing(zz, ni, lni, nj, maxval, minval, minabs) ;
  }else{
    return float_info_missing(zz, ni, lni, nj, maxval, minval, minabs, spval, spmask) ;
  }
}

