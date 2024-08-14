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
#include <stdio.h>

#include <rmn/simd_compare.h>

#define VL 8

// increment count[j] when z[i] < ref[j] (0 <= j < 6, 0 <= i < n)
// z        [IN] : array of NON NEGATIVE SIGNED values
// ref      [IN] : 6 reference values
// count [INOUT] : 6 counts to be incremented
// n        [IN] : number of values
// often slower version (performance is compiler and platform dependent)
void v_less_than_c(int32_t *z, int32_t ref[6], int32_t count[6], int32_t n){
  v_less_than_c_6(z, ref, count, n) ;
}
void v_less_than_c_6(int32_t *z, int32_t ref[6], int32_t count[6], int32_t n){
  int32_t vc0[VL], vc1[VL], vc2[VL], vc3[VL], vc4[VL], vc5[VL]  ;     // vector counts
  int32_t kr0[VL], kr1[VL], kr2[VL], kr3[VL], kr4[VL], kr5[VL]  ;     // vector copies of reference values
  int32_t t[VL] ;
  int i, nvl ;

  for(i=0 ; i<VL ; i++) {                           // ref scalar -> vector
    kr0[i] = ref[0] ;
    kr1[i] = ref[1] ;
    kr2[i] = ref[2] ;
    kr3[i] = ref[3] ;
    kr4[i] = ref[4] ;
    kr5[i] = ref[5] ;
  }

  nvl = (n & (VL-1)) ;
  for(i=0   ; i<VL ; i++) { t[i] = z[i] ; }         // first slice modulo(n, VL)
  if(nvl == 0) nvl = VL ;                           // modulo is 0, full slice
  for(i=nvl ; i<VL ; i++) { t[i] = 0x7FFFFFFF ; }   // pad to full vector length
  for(i=0 ; i<VL ; i++) {
    vc0[i] = ((t[i] - kr0[i]) >> 31) ;              // -1 if smaller that reference value
    vc1[i] = ((t[i] - kr1[i]) >> 31) ;
    vc2[i] = ((t[i] - kr2[i]) >> 31) ;
    vc3[i] = ((t[i] - kr3[i]) >> 31) ;
    vc4[i] = ((t[i] - kr4[i]) >> 31) ;
    vc5[i] = ((t[i] - kr5[i]) >> 31) ;
  }
  z += nvl ;                                        // next slice
  n -= nvl ;

  while(n > VL - 1){                                // subsequent slices
    for(i=0 ; i<VL ; i++) {
      vc0[i] = vc0[i] + ((z[i] - kr0[i]) >> 31) ;   // -1 if smaller that reference value
      vc1[i] = vc1[i] + ((z[i] - kr1[i]) >> 31) ;   // 0 otherwise
      vc2[i] = vc2[i] + ((z[i] - kr2[i]) >> 31) ;
      vc3[i] = vc3[i] + ((z[i] - kr3[i]) >> 31) ;
      vc4[i] = vc4[i] + ((z[i] - kr4[i]) >> 31) ;
      vc5[i] = vc5[i] + ((z[i] - kr5[i]) >> 31) ;
    }
    z += VL ;                                       // next slice
    n -= VL ;
  }
  // vector counts are negative, substract from count[]
  for(i=0 ; i<VL ; i++) {   // fold vector counts into count[]
    count[0] -= vc0[i] ;
    count[1] -= vc1[i] ;
    count[2] -= vc2[i] ;
    count[3] -= vc3[i] ;
    count[4] -= vc4[i] ;
    count[5] -= vc5[i] ;
  }
  return;
}
void v_less_than_c_4(int32_t *z, int32_t ref[4], int32_t count[4], int32_t n){
  int32_t vc0[VL], vc1[VL], vc2[VL], vc3[VL]  ;     // vector counts
  int32_t kr0[VL], kr1[VL], kr2[VL], kr3[VL]  ;     // vector copies of reference values
  int32_t t[VL] ;
  int i, nvl ;

  for(i=0 ; i<VL ; i++) {                           // ref scalar -> vector
    kr0[i] = ref[0] ;
    kr1[i] = ref[1] ;
    kr2[i] = ref[2] ;
    kr3[i] = ref[3] ;
  }

  nvl = (n & (VL-1)) ;
  for(i=0   ; i<VL ; i++) { t[i] = z[i] ; }         // first slice modulo(n, VL)
  if(nvl == 0) nvl = VL ;                           // modulo is 0, full slice
  for(i=nvl ; i<VL ; i++) { t[i] = 0x7FFFFFFF ; }   // pad to full vector length
  for(i=0 ; i<VL ; i++) {
    vc0[i] = ((t[i] - kr0[i]) >> 31) ;              // -1 if smaller that reference value
    vc1[i] = ((t[i] - kr1[i]) >> 31) ;
    vc2[i] = ((t[i] - kr2[i]) >> 31) ;
    vc3[i] = ((t[i] - kr3[i]) >> 31) ;
  }
  z += nvl ;                                        // next slice
  n -= nvl ;

  while(n > VL - 1){                                // subsequent slices
    for(i=0 ; i<VL ; i++) {
      vc0[i] = vc0[i] + ((z[i] - kr0[i]) >> 31) ;   // -1 if smaller that reference value
      vc1[i] = vc1[i] + ((z[i] - kr1[i]) >> 31) ;   // 0 otherwise
      vc2[i] = vc2[i] + ((z[i] - kr2[i]) >> 31) ;
      vc3[i] = vc3[i] + ((z[i] - kr3[i]) >> 31) ;
    }
    z += VL ;                                       // next slice
    n -= VL ;
  }
  // vector counts are negative, substract from count[]
  for(i=0 ; i<VL ; i++) {   // fold vector counts into count[]
    count[0] -= vc0[i] ;
    count[1] -= vc1[i] ;
    count[2] -= vc2[i] ;
    count[3] -= vc3[i] ;
  }
  return;
}

#if defined(__AVX2__)
#include <immintrin.h>

// build a 256 bit mask (8 x 32 bits) to keep n (0-8) elements in masked operations
// mask is built as a 64 bit mask (8x8bit), then expanded to 256 bits (8x32bit)
static inline __m256i _mm256_memmask_si256(int n){
  __m128i vm ;
  uint64_t i64 = ~0lu ;                                 // all 1s
  i64 = (n&7) ? (i64 >> ( 8 * (8 - (n&7)) )) : i64 ;    // shift right to eliminate unneeded elements
  vm = _mm_set1_epi64x(i64) ;                           // load into 128 bit register
  return _mm256_cvtepi8_epi32(vm) ;                     // convert from 8 bit to 32 bit mask (8 elements)
}

// increment count[j] when z[i] < ref[j] (0 <= j < 6, 0 <= i < n)
// z        [IN] : array of NON NEGATIVE SIGNED values
// ref      [IN] : 6 reference values
// count [INOUT] : 6 counts to be incremented
// n        [IN] : number of values
// usually faster, AVX2 version (performance is compiler and platform dependent)
void v_less_than_simd(int32_t *z, int32_t ref[6], int32_t count[6], int32_t n){
  v_less_than_simd_6(z, ref, count, n) ;
}
void v_less_than_simd_6(int32_t *z, int32_t ref[6], int32_t count[6], int32_t n){
  int nvl, t, xtra ;
  __m256i vz, vr0, vr1, vr2, vr3, vr4, vr5, vc0, vc1, vc2, vc3, vc4, vc5, vm ;

  nvl = (n & (VL-1)) ;
  if(nvl == 0) nvl = VL ;
  xtra = 8 - nvl ;                         // xtra points will be counted twice
  vm = _mm256_memmask_si256(nvl) ;
  vr0 = _mm256_broadcastd_epi32( _mm_set1_epi32(ref[0]) ) ;
  vr1 = _mm256_broadcastd_epi32( _mm_set1_epi32(ref[1]) ) ;
  vr2 = _mm256_broadcastd_epi32( _mm_set1_epi32(ref[2]) ) ;
  vr3 = _mm256_broadcastd_epi32( _mm_set1_epi32(ref[3]) ) ;
  vr4 = _mm256_broadcastd_epi32( _mm_set1_epi32(ref[4]) ) ;
  vr5 = _mm256_broadcastd_epi32( _mm_set1_epi32(ref[5]) ) ;
  // first slice (may partially overlap the next slice)
  vz = _mm256_maskload_epi32(z, vm) ;      // set overlap values to 0 (will be counted twice)
  vc0 = _mm256_cmpgt_epi32(vr0, vz) ;
  vc1 = _mm256_cmpgt_epi32(vr1, vz) ;
  vc2 = _mm256_cmpgt_epi32(vr2, vz) ;
  vc3 = _mm256_cmpgt_epi32(vr3, vz) ;
  vc4 = _mm256_cmpgt_epi32(vr4, vz) ;
  vc5 = _mm256_cmpgt_epi32(vr5, vz) ;
  z += nvl ;
  n -= nvl ;
  while(n > VL - 1){
    vz = _mm256_loadu_si256((__m256i *)z) ;
    vc0 = _mm256_add_epi32( vc0 , _mm256_cmpgt_epi32(vr0, vz) ) ;
    vc1 = _mm256_add_epi32( vc1 , _mm256_cmpgt_epi32(vr1, vz) ) ;
    vc2 = _mm256_add_epi32( vc2 , _mm256_cmpgt_epi32(vr2, vz) ) ;
    vc3 = _mm256_add_epi32( vc3 , _mm256_cmpgt_epi32(vr3, vz) ) ;
    vc4 = _mm256_add_epi32( vc4 , _mm256_cmpgt_epi32(vr4, vz) ) ;
    vc5 = _mm256_add_epi32( vc5 , _mm256_cmpgt_epi32(vr5, vz) ) ;
    z += VL ;
    n -= VL ;
  }
  // fold 256 to 128
  __m128i vi0 = _mm_add_epi32( _mm256_extracti128_si256(vc0, 0) , _mm256_extracti128_si256(vc0, 1) ) ;
  __m128i vi1 = _mm_add_epi32( _mm256_extracti128_si256(vc1, 0) , _mm256_extracti128_si256(vc1, 1) ) ;
  __m128i vi2 = _mm_add_epi32( _mm256_extracti128_si256(vc2, 0) , _mm256_extracti128_si256(vc2, 1) ) ;
  __m128i vi3 = _mm_add_epi32( _mm256_extracti128_si256(vc3, 0) , _mm256_extracti128_si256(vc3, 1) ) ;
  __m128i vi4 = _mm_add_epi32( _mm256_extracti128_si256(vc4, 0) , _mm256_extracti128_si256(vc4, 1) ) ;
  __m128i vi5 = _mm_add_epi32( _mm256_extracti128_si256(vc5, 0) , _mm256_extracti128_si256(vc5, 1) ) ;
  // fold 128 to 32
  vi0 = _mm_add_epi32( _mm_bsrli_si128(vi0, 8), vi0) ; vi0 = _mm_add_epi32( _mm_bsrli_si128(vi0, 4), vi0) ;
  vi1 = _mm_add_epi32( _mm_bsrli_si128(vi1, 8), vi1) ; vi1 = _mm_add_epi32( _mm_bsrli_si128(vi1, 4), vi1) ;
  vi2 = _mm_add_epi32( _mm_bsrli_si128(vi2, 8), vi2) ; vi2 = _mm_add_epi32( _mm_bsrli_si128(vi2, 4), vi2) ;
  vi3 = _mm_add_epi32( _mm_bsrli_si128(vi3, 8), vi3) ; vi3 = _mm_add_epi32( _mm_bsrli_si128(vi3, 4), vi3) ;
  vi4 = _mm_add_epi32( _mm_bsrli_si128(vi4, 8), vi4) ; vi4 = _mm_add_epi32( _mm_bsrli_si128(vi4, 4), vi4) ;
  vi5 = _mm_add_epi32( _mm_bsrli_si128(vi5, 8), vi5) ; vi5 = _mm_add_epi32( _mm_bsrli_si128(vi5, 4), vi5) ;

  _mm_storeu_si32(&t , vi0) ; count[0] -= t ; count[0] -= xtra ;  // xtra points have been counted twice
  _mm_storeu_si32(&t , vi1) ; count[1] -= t ; count[1] -= xtra ;
  _mm_storeu_si32(&t , vi2) ; count[2] -= t ; count[2] -= xtra ;
  _mm_storeu_si32(&t , vi3) ; count[3] -= t ; count[3] -= xtra ;
  _mm_storeu_si32(&t , vi4) ; count[4] -= t ; count[4] -= xtra ;
  _mm_storeu_si32(&t , vi5) ; count[5] -= t ; count[5] -= xtra ;
}
void v_less_than_simd_4(int32_t *z, int32_t ref[4], int32_t count[4], int32_t n){
  int nvl, t, xtra ;
  __m256i vz, vr0, vr1, vr2, vr3, vc0, vc1, vc2, vc3, vm ;

  nvl = (n & (VL-1)) ;
  if(nvl == 0) nvl = VL ;
  xtra = 8 - nvl ;                         // xtra points will be counted twice
  vm = _mm256_memmask_si256(nvl) ;
  vr0 = _mm256_broadcastd_epi32( _mm_set1_epi32(ref[0]) ) ;
  vr1 = _mm256_broadcastd_epi32( _mm_set1_epi32(ref[1]) ) ;
  vr2 = _mm256_broadcastd_epi32( _mm_set1_epi32(ref[2]) ) ;
  vr3 = _mm256_broadcastd_epi32( _mm_set1_epi32(ref[3]) ) ;
  // first slice (may partially overlap the next slice)
  vz = _mm256_maskload_epi32(z, vm) ;      // set overlap values to 0 (will be counted twice)
  vc0 = _mm256_cmpgt_epi32(vr0, vz) ;
  vc1 = _mm256_cmpgt_epi32(vr1, vz) ;
  vc2 = _mm256_cmpgt_epi32(vr2, vz) ;
  vc3 = _mm256_cmpgt_epi32(vr3, vz) ;
  z += nvl ;
  n -= nvl ;
  while(n > VL - 1){
    vz = _mm256_loadu_si256((__m256i *)z) ;
    vc0 = _mm256_add_epi32( vc0 , _mm256_cmpgt_epi32(vr0, vz) ) ;
    vc1 = _mm256_add_epi32( vc1 , _mm256_cmpgt_epi32(vr1, vz) ) ;
    vc2 = _mm256_add_epi32( vc2 , _mm256_cmpgt_epi32(vr2, vz) ) ;
    vc3 = _mm256_add_epi32( vc3 , _mm256_cmpgt_epi32(vr3, vz) ) ;
    z += VL ;
    n -= VL ;
  }
  // fold 256 to 128
  __m128i vi0 = _mm_add_epi32( _mm256_extracti128_si256(vc0, 0) , _mm256_extracti128_si256(vc0, 1) ) ;
  __m128i vi1 = _mm_add_epi32( _mm256_extracti128_si256(vc1, 0) , _mm256_extracti128_si256(vc1, 1) ) ;
  __m128i vi2 = _mm_add_epi32( _mm256_extracti128_si256(vc2, 0) , _mm256_extracti128_si256(vc2, 1) ) ;
  __m128i vi3 = _mm_add_epi32( _mm256_extracti128_si256(vc3, 0) , _mm256_extracti128_si256(vc3, 1) ) ;
  // fold 128 to 32
  vi0 = _mm_add_epi32( _mm_bsrli_si128(vi0, 8), vi0) ; vi0 = _mm_add_epi32( _mm_bsrli_si128(vi0, 4), vi0) ;
  vi1 = _mm_add_epi32( _mm_bsrli_si128(vi1, 8), vi1) ; vi1 = _mm_add_epi32( _mm_bsrli_si128(vi1, 4), vi1) ;
  vi2 = _mm_add_epi32( _mm_bsrli_si128(vi2, 8), vi2) ; vi2 = _mm_add_epi32( _mm_bsrli_si128(vi2, 4), vi2) ;
  vi3 = _mm_add_epi32( _mm_bsrli_si128(vi3, 8), vi3) ; vi3 = _mm_add_epi32( _mm_bsrli_si128(vi3, 4), vi3) ;

  _mm_storeu_si32(&t , vi0) ; count[0] -= t ; count[0] -= xtra ;  // xtra points have been counted twice
  _mm_storeu_si32(&t , vi1) ; count[1] -= t ; count[1] -= xtra ;
  _mm_storeu_si32(&t , vi2) ; count[2] -= t ; count[2] -= xtra ;
  _mm_storeu_si32(&t , vi3) ; count[3] -= t ; count[3] -= xtra ;
}
#endif
void v_less_than_6(int32_t *z, int32_t ref[6], int32_t count[6], int32_t n){
#if defined(__AVX2__)
  v_less_than_simd_6(z, ref, count, n) ;
#else
  v_less_than_c_6(z, ref, count, n) ;
#endif
}
void v_less_than(int32_t *z, int32_t ref[6], int32_t count[6], int32_t n){
  v_less_than_6(z, ref, count, n) ;
}
void v_less_than_4(int32_t *z, int32_t ref[4], int32_t count[4], int32_t n){
#if defined(__AVX2__)
  v_less_than_simd_4(z, ref, count, n) ;
#else
  v_less_than_c_4(z, ref, count, n) ;
#endif
}
#if defined(__AVX512F__)
#define VL2 32
#else
#define VL2 16
#endif
// get minimum, maximum, minimum absolute values from signed array z
// z        [IN] : array of SIGNED values
// n        [IN] : number of values
// mins    [OUT] : smallest value
// maxs    [OUT] : largest value
// min0    [OUT] : smallest non zero absolute value
void v_minmax(int32_t *z, int32_t n, int32_t *mins, int32_t *maxs, uint32_t *min0){
  int i, nvl ;
  int32_t vmins[VL2], vmaxs[VL2] ;
  uint32_t vmina[VL2] ;

  nvl = (n & (VL2-1)) ;
  if(nvl == 0) nvl = VL2 ;
  if(n > VL2-1){
    for(i=0 ; i<VL2 ; i++){
      vmins[i] = vmaxs[i] = z[i] ;
      vmina[i] = (z[i] < 0) ? -z[i] : z[i] ;
    }
    z += nvl ; n -= nvl ;
    while(n > VL2-1){
      for(i=0 ; i<VL2 ; i++){
        uint32_t temp = (z[i] > 0) ? z[i] : -z[i] ;
        temp = (temp == 0) ? vmina[i] : temp ;
        vmins[i] = (z[i] < vmins[i]) ? z[i] : vmins[i] ;
        vmaxs[i] = (z[i] > vmaxs[i]) ? z[i] : vmaxs[i] ;
        vmina[i] = (temp < vmina[i]) ? temp : vmina[i] ;
      }
      z += VL2 ; n -= VL2 ;
    }
    for(i=1 ; i<VL2 ; i++){
      vmins[0] = (vmins[i] < vmins[0]) ? vmins[i] : vmins[0] ;
      vmaxs[0] = (vmaxs[i] > vmaxs[0]) ? vmaxs[i] : vmaxs[0] ;
      vmina[0] = (vmina[i] < vmina[0]) ? vmina[i] : vmina[0] ;
    }
    *mins = vmins[0] ;
    *maxs = vmaxs[0] ;
    *min0 = vmina[0] ;
  }else{
  }
}
