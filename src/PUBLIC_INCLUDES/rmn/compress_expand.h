// Hopefully useful functions for C and FORTRAN
// Copyright (C) 2023  Recherche en Prevision Numerique
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
// uncomment following line to force plain C (non SIMD) code
// #undef __SSE2__
#undef __SSE2__
#undef __AVX512F__

#if ! defined(STORE_COMPRESS_LOAD_EXPAND)
#define STORE_COMPRESS_LOAD_EXPAND

#include <stdint.h>
#include <rmn/bits.h>

#if defined(__x86_64__) && defined(__SSE2__)
#include <immintrin.h>

// N.B. permutation tables are BYTE permutation tables,
//      using a vector index byte permutation instruction,
//      as there is no vector index word permutation instruction
static struct exptab{   // lookup permutation table used to perform a SIMD load-expand
  int8_t stab[16] ;     // elements to pick from s[0,1,2,3], x means don't care (set to 0)
} exptab[16] = {                                                                 // MASK s[]
  { -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 0000 [x,x,x,x]
  { -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1,     0,  1,  2,  3 } ,  // 0001 [x,x,x,0]
  { -1, -1, -1, -1,    -1, -1, -1, -1,     0,  1,  2,  3,    -1, -1, -1, -1 } ,  // 0010 [x,x,0,x]
  { -1, -1, -1, -1,    -1, -1, -1, -1,     0,  1,  2,  3,     4,  5,  6,  7 } ,  // 0011 [x,x,0,1]
  { -1, -1, -1, -1,     0,  1,  2,  3,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 0100 [x,0,x,x]
  { -1, -1, -1, -1,     0,  1,  2,  3,    -1, -1, -1, -1,     4,  5,  6,  7 } ,  // 0101 [x,0,x,1]
  { -1, -1, -1, -1,     0,  1,  2,  3,     4,  5,  6,  7,    -1, -1, -1, -1 } ,  // 0110 [x,0,1,x]
  { -1, -1, -1, -1,     0,  1,  2,  3,     4,  5,  6,  7,     8,  9, 10, 11 } ,  // 0111 [x,0,1,2]
  {  0,  1,  2,  3,    -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 1000 [0,x,x,x]
  {  0,  1,  2,  3,    -1, -1, -1, -1,    -1, -1, -1, -1,     4,  5,  6,  7 } ,  // 1001 [0,x,x,1]
  {  0,  1,  2,  3,    -1, -1, -1, -1,     4,  5,  6,  7,    -1, -1, -1, -1 } ,  // 1010 [0,x,1,x]
  {  0,  1,  2,  3,    -1, -1, -1, -1,     4,  5,  6,  7,     8,  9, 10, 11 } ,  // 1011 [0,x,1,2]
  {  0,  1,  2,  3,     4,  5,  6,  7,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 1100 [0,1,x,x]
  {  0,  1,  2,  3,     4,  5,  6,  7,    -1, -1, -1, -1,     8,  9, 10, 11 } ,  // 1101 [0,1,x,2]
  {  0,  1,  2,  3,     4,  5,  6,  7,     8,  9, 10, 11,    -1, -1, -1, -1 } ,  // 1110 [0,1,2,x]
  {  0,  1,  2,  3,     4,  5,  6,  7,     8,  9, 10, 11,    12, 13, 14, 15 } ,  // 1111 [0,1,2,3]
             } ;

static struct cmp_le{   // lookup permutation table used to perform a SIMD store-compress
  int8_t stab[16] ;     // elements from s[0,1,2,3] to store into d, x means fill or leave as is
} cmp_le[16] = {                                                                 // MASK
  { -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 0000 [x,x,x,x]
  { 12, 13, 14, 15,    -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 0001 [3,x,x,x]
  {  8,  9, 10, 11,    -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 0010 [2,x,x,x]
  {  8,  9, 10, 11,    12, 13, 14, 15,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 0011 [2,3,x,x]
  {  4,  5,  6,  7,    -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 0100 [1,x,x,x]
  {  4,  5,  6,  7,    12, 13, 14, 15,    -1, -1, -1, -1 ,   -1, -1, -1, -1 } ,  // 0101 [1,3,x,x]
  {  4,  5,  6,  7,     8,  9, 10, 11,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 0110 [1,2,x,x]
  {  4,  5,  6,  7,     8,  9, 10, 11,    12, 13, 14, 15,    -1, -1, -1, -1 } ,  // 0111 [1,2,3,x]
  {  0,  1,  2,  3,    -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 1000 [0,x,x,x]
  {  0,  1,  2,  3,    12, 13, 14, 15,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 1001 [0,3,x,x]
  {  0,  1,  2,  3,     8,  9, 10, 11,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 1010 [0,2,x,x]
  {  0,  1,  2,  3,     8,  9, 10, 11,    12, 13, 14, 15,    -1, -1, -1, -1 } ,  // 1011 [0,2,3,x]
  {  0,  1,  2,  3,     4,  5,  6,  7,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 1100 [0,1,x,x]
  {  0,  1,  2,  3,     4,  5,  6,  7,    12, 13, 14, 15,    -1, -1, -1, -1 } ,  // 1101 [0,1,3,x]
  {  0,  1,  2,  3,     4,  5,  6,  7,     8,  9, 10, 11,    -1, -1, -1, -1 } ,  // 1110 [0,1,2,x]
  {  0,  1,  2,  3,     4,  5,  6,  7,     8,  9, 10, 11,    12, 13, 14, 15 } ,  // 1111 [0,1,2,3]
             } ;

static struct cmp_be{   // lookup permutation table used to perform a SIMD store-compress
  int8_t stab[16] ;     // elements from s[0,1,2,3] to store into d, x means fill or leave as is
} cmp_be[16] = {                                                                 // MASK
  { -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 0000 [x,x,x,x]
  {  0,  1,  2,  3,    -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 0001 [0,x,x,x]
  {  4,  5,  6,  7,    -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 0010 [1,x,x,x]
  {  0,  1,  2,  3,     4,  5,  6,  7,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 0011 [0,1,x,x]
  {  8,  9, 10, 11,    -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 0100 [2,x,x,x]
  {  0,  1,  2,  3,     8,  9, 10, 11,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 0101 [0,2,x,x]
  {  4,  5,  6,  7,     8,  9, 10, 11,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 0110 [1,2,x,x]
  {  0,  1,  2,  3,     4,  5,  6,  7,     8,  9, 10, 11,    -1, -1, -1, -1 } ,  // 0111 [0,1,2,x]
  { 12, 13, 14, 15,    -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 1000 [3,x,x,x]
  {  0,  1,  2,  3,    12, 13, 14, 15,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 1001 [0,3,x,x]
  {  4,  5,  6,  7,    12, 13, 14, 15,    -1, -1, -1, -1 ,   -1, -1, -1, -1 } ,  // 1010 [1,3,x,x]
  {  0,  1,  2,  3,     8,  9, 10, 11,    12, 13, 14, 15,    -1, -1, -1, -1 } ,  // 1101 [0,2,3,x]
  {  8,  9, 10, 11,    12, 13, 14, 15,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 1100 [2,3,x,x]
  {  4,  5,  6,  7,     8,  9, 10, 11,    12, 13, 14, 15,    -1, -1, -1, -1 } ,  // 1110 [1,2,3,x]
  {  0,  1,  2,  3,     4,  5,  6,  7,    12, 13, 14, 15,    -1, -1, -1, -1 } ,  // 1011 [0,1,3,x]
  {  0,  1,  2,  3,     4,  5,  6,  7,     8,  9, 10, 11,    12, 13, 14, 15 } ,  // 1111 [0,1,2,3]
             } ;

static inline uint32_t *sse_expand_replace_32(uint32_t *s, uint32_t *d, uint32_t mask){
  int i ;
  __m128i vt0, vs0, vf0, vb0 ;
  uint32_t mask0, pop0 ;

  for(i=0 ; i<2 ; i++){                                            // explicit unroll by 4
      mask0 = mask >> 28 ; mask <<= 4 ; pop0 = popcnt_32(mask0) ;  // get 4 bit mask0 for this slice
      vt0 = _mm_loadu_si128((__m128i *)s) ; s += pop0 ;            // load items from source (compressed) array
//       vt0 = _mm_maskload_epi32((int const *)s, ~vs0) ;             // load using ~vs0 as a mask to avoid load beyond array
      vs0 = _mm_loadu_si128((__m128i *)(exptab + mask0)) ;         // load permutation for this mask0
      vf0 = _mm_loadu_si128((__m128i *)d) ;                        // load items from destination (expanded) array
      vb0 = _mm_srai_epi32(vs0, 31) ;                              // 1s where keep, 0s where new from source
      vt0 = _mm_shuffle_epi8(vt0, vs0) ;                           // shufffle to get source items in proper position
      vt0 = _mm_blendv_epi8(vt0, vf0, vb0) ;                       // blend keep/new value
      _mm_storeu_si128((__m128i *)d, vt0) ; d += 4 ;               // store into destination
      mask0 = mask >> 28 ; mask <<= 4 ; pop0 = popcnt_32(mask0) ;
      vt0 = _mm_loadu_si128((__m128i *)s) ; s += pop0 ;
      vs0 = _mm_loadu_si128((__m128i *)(exptab + mask0)) ;
      vf0 = _mm_loadu_si128((__m128i *)d) ;
      vb0 = _mm_srai_epi32(vs0, 31) ;
      vt0 = _mm_shuffle_epi8(vt0, vs0) ;
      vt0 = _mm_blendv_epi8(vt0, vf0, vb0) ;
      _mm_storeu_si128((__m128i *)d, vt0) ; d += 4 ;
      mask0 = mask >> 28 ; mask <<= 4 ; pop0 = popcnt_32(mask0) ;
      vt0 = _mm_loadu_si128((__m128i *)s) ; s += pop0 ;
      vs0 = _mm_loadu_si128((__m128i *)(exptab + mask0)) ;
      vf0 = _mm_loadu_si128((__m128i *)d) ;
      vb0 = _mm_srai_epi32(vs0, 31) ;
      vt0 = _mm_shuffle_epi8(vt0, vs0) ;
      vt0 = _mm_blendv_epi8(vt0, vf0, vb0) ;
      _mm_storeu_si128((__m128i *)d, vt0) ; d += 4 ;
      mask0 = mask >> 28 ; mask <<= 4 ; pop0 = popcnt_32(mask0) ;
      vt0 = _mm_loadu_si128((__m128i *)s) ; s += pop0 ;
      vs0 = _mm_loadu_si128((__m128i *)(exptab + mask0)) ;
      vf0 = _mm_loadu_si128((__m128i *)d) ;
      vb0 = _mm_srai_epi32(vs0, 31) ;
      vt0 = _mm_shuffle_epi8(vt0, vs0) ;
      vt0 = _mm_blendv_epi8(vt0, vf0, vb0) ;
      _mm_storeu_si128((__m128i *)d, vt0) ; d += 4 ;
  }
  return s ;
}
#endif

uint32_t *expand_replace_32(uint32_t *s, uint32_t *d, uint32_t mask){
  int i ;

  for(i=0 ; i<8 ; i++){    // explicit unroll by 4
    uint32_t m31 ;
    m31 = mask >> 31 ;
    *d = m31 ? *s : *d ;
    d++ ;
    s += m31 ;
    mask <<= 1 ;
    m31 = mask >> 31 ;
    *d = m31 ? *s : *d ;
    d++ ;
    s += m31 ;
    mask <<= 1 ;
    m31 = mask >> 31 ;
    *d = m31 ? *s : *d ;
    d++ ;
    s += m31 ;
    mask <<= 1 ;
    m31 = mask >> 31 ;
    *d = m31 ? *s : *d ;
    d++ ;
    s += m31 ;
    mask <<= 1 ;
  }
  return s ;
}

static uint32_t *expand_replace_n(uint32_t *s, uint32_t *d, uint32_t mask, int n){
  while(n--){
    uint32_t m31 ;
    m31 = mask >> 31 ;
    *d = m31 ? *s : *d ;
    d++ ;
    s += m31 ;
    mask <<= 1 ;
  }
  return s ;
}

#if defined(__x86_64__) && defined(__SSE2__)
static inline uint32_t * sse_expand_fill_32(uint32_t *s, uint32_t *d, uint32_t mask, uint32_t fill){
  int i ;
  __m128i vt0, vs0, vf0, vb0 ;

  vf0 = _mm_set1_epi32(fill) ;                                   // fill value
  for(i=0 ; i<2 ; i++){                                          // explicit unroll by 4
    uint32_t mask0, pop0 ;
    mask0 = mask >> 28 ; mask <<= 4 ; pop0 = popcnt_32(mask0) ;  // get 4 bit mask0 for this slice
    vt0 = _mm_loadu_si128((__m128i *)s) ; s += pop0 ;            // load items from source (compressed) array
//     vt0 = _mm_maskload_epi32((int const *)s, ~vs0) ;             // load using ~vs0 as a mask to avoid load beyond array
    vs0 = _mm_loadu_si128((__m128i *)(exptab + mask0)) ;         // load permutation for this mask0
    vb0 = _mm_srai_epi32(vs0, 31) ;                              // 1s where fill, 0s where new from source
    vt0 = _mm_shuffle_epi8(vt0, vs0) ;                           // shufffle to get source items in proper position
    vt0 = _mm_blendv_epi8(vt0, vf0, vb0) ;                       // blend in fill value
    _mm_storeu_si128((__m128i *)d, vt0) ; d += 4 ;               // store into destination
    mask0 = mask >> 28 ; mask <<= 4 ; pop0 = popcnt_32(mask0) ;
    vt0 = _mm_loadu_si128((__m128i *)s) ; s += pop0 ;
    vs0 = _mm_loadu_si128((__m128i *)(exptab + mask0)) ;
    vb0 = _mm_srai_epi32(vs0, 31) ;
    vt0 = _mm_shuffle_epi8(vt0, vs0) ;
    vt0 = _mm_blendv_epi8(vt0, vf0, vb0) ;
    _mm_storeu_si128((__m128i *)d, vt0) ; d += 4 ;
    mask0 = mask >> 28 ; mask <<= 4 ; pop0 = popcnt_32(mask0) ;
    vt0 = _mm_loadu_si128((__m128i *)s) ; s += pop0 ;
    vs0 = _mm_loadu_si128((__m128i *)(exptab + mask0)) ;
    vb0 = _mm_srai_epi32(vs0, 31) ;
    vt0 = _mm_shuffle_epi8(vt0, vs0) ;
    vt0 = _mm_blendv_epi8(vt0, vf0, vb0) ;
    _mm_storeu_si128((__m128i *)d, vt0) ; d += 4 ;
    mask0 = mask >> 28 ; mask <<= 4 ; pop0 = popcnt_32(mask0) ;
    vt0 = _mm_loadu_si128((__m128i *)s) ; s += pop0 ;
    vs0 = _mm_loadu_si128((__m128i *)(exptab + mask0)) ;
    vb0 = _mm_srai_epi32(vs0, 31) ;
    vt0 = _mm_shuffle_epi8(vt0, vs0) ;
    vt0 = _mm_blendv_epi8(vt0, vf0, vb0) ;
    _mm_storeu_si128((__m128i *)d, vt0) ; d += 4 ;
  }
  return s ;
}
#endif

static inline uint32_t *expand_fill_32(uint32_t *s, uint32_t *d, uint32_t mask, uint32_t fill){
  int i ;
  for(i=0 ; i<8 ; i++){    // explicit unroll by 4
    uint32_t m31 ;
    m31 = mask >> 31 ;
    *d = m31 ? *s : fill ;
    d++ ;
    s += m31 ;
    mask <<= 1 ;
    m31 = mask >> 31 ;
    *d = m31 ? *s : fill ;
    d++ ;
    s += m31 ;
    mask <<= 1 ;
    m31 = mask >> 31 ;
    *d = m31 ? *s : fill ;
    d++ ;
    s += m31 ;
    mask <<= 1 ;
    m31 = mask >> 31 ;
    *d = m31 ? *s : fill ;
    d++ ;
    s += m31 ;
    mask <<= 1 ;
  }
  return s ;
}

static inline uint32_t * expand_fill_n(uint32_t *s, uint32_t *d, uint32_t mask, uint32_t fill, int n){
  while(n--){
    uint32_t m31 = mask >> 31 ;
    *d = m31 ? *s : fill ;
    d++ ;
    s += m31 ;
    mask <<= 1 ;
  }
  return s ;
}

#if defined(__x86_64__) && defined(__SSE2__)
// store-compress 32 items according to mask using SSE2 instructions
static inline uint32_t *CompressStore_32_sse_be(uint32_t *s, uint32_t *d, uint32_t mask){
  uint32_t pop0, mask0, i ;
  __m128i vt0, vs0 ;

  for(i=0 ; i<2 ; i++){                                    // explicit unroll by 4
    mask0 = mask >> 28 ; mask <<= 4 ;
    vt0 = _mm_loadu_si128((__m128i *)s) ;  s += 4 ;        // load items from source array
    vs0 = _mm_loadu_si128((__m128i *)(cmp_le + mask0)) ;   // load permutation for this mask0
    vt0 = _mm_shuffle_epi8(vt0, vs0) ;                     // shufffle to get source items in proper position
    pop0 = popcnt_32(mask0) ;                              // number of useful items that will be stored
    _mm_storeu_si128((__m128i *)d, vt0) ; d += pop0 ;      // store into destination
//     _mm_maskmoveu_si128(vt0, ~vs0, (char *)d) ;            // mask store, using ~vs0 as mask
    mask0 = mask >> 28 ; mask <<= 4 ;
    vt0 = _mm_loadu_si128((__m128i *)s) ;  s += 4 ;
    vs0 = _mm_loadu_si128((__m128i *)(cmp_le + mask0)) ;
    vt0 = _mm_shuffle_epi8(vt0, vs0) ;
    pop0 = popcnt_32(mask0) ;
    _mm_storeu_si128((__m128i *)d, vt0) ; d += pop0 ;
    mask0 = mask >> 28 ; mask <<= 4 ;
    vt0 = _mm_loadu_si128((__m128i *)s) ;  s += 4 ;
    vs0 = _mm_loadu_si128((__m128i *)(cmp_le + mask0)) ;
    vt0 = _mm_shuffle_epi8(vt0, vs0) ;
    pop0 = popcnt_32(mask0) ;
    _mm_storeu_si128((__m128i *)d, vt0) ; d += pop0 ;
    mask0 = mask >> 28 ; mask <<= 4 ;
    vt0 = _mm_loadu_si128((__m128i *)s) ;  s += 4 ;
    vs0 = _mm_loadu_si128((__m128i *)(cmp_le + mask0)) ;
    vt0 = _mm_shuffle_epi8(vt0, vs0) ;
    pop0 = popcnt_32(mask0) ;
    _mm_storeu_si128((__m128i *)d, vt0) ; d += pop0 ;
  }
  return d ;
}
static inline uint32_t *CompressStore_32_sse_le(uint32_t *s, uint32_t *d, uint32_t le_mask){
  uint32_t pop0, mask0, i ;
  __m128i vt0, vs0 ;

  for(i=0 ; i<2 ; i++){                                    // explicit unroll by 4, 16 values per loop iteration
    mask0 = le_mask & 0xFF ; le_mask >>= 4 ;
    vt0 = _mm_loadu_si128((__m128i *)s) ;  s += 4 ;        // load items from source array
    vs0 = _mm_loadu_si128((__m128i *)(cmp_le + mask0)) ;   // load permutation for this mask0
    vt0 = _mm_shuffle_epi8(vt0, vs0) ;                     // shufffle to get source items in proper position
    pop0 = popcnt_32(mask0) ;                              // number of useful items that will be stored
    _mm_storeu_si128((__m128i *)d, vt0) ; d += pop0 ;      // store into destination
    mask0 = le_mask & 0xFF ; le_mask >>= 4 ;
    vt0 = _mm_loadu_si128((__m128i *)s) ;  s += 4 ;
    vs0 = _mm_loadu_si128((__m128i *)(cmp_le + mask0)) ;
    vt0 = _mm_shuffle_epi8(vt0, vs0) ;
    pop0 = popcnt_32(mask0) ;
    _mm_storeu_si128((__m128i *)d, vt0) ; d += pop0 ;
    mask0 = le_mask & 0xFF ; le_mask >>= 4 ;
    vt0 = _mm_loadu_si128((__m128i *)s) ;  s += 4 ;
    vs0 = _mm_loadu_si128((__m128i *)(cmp_le + mask0)) ;
    vt0 = _mm_shuffle_epi8(vt0, vs0) ;
    pop0 = popcnt_32(mask0) ;
    _mm_storeu_si128((__m128i *)d, vt0) ; d += pop0 ;
    mask0 = le_mask & 0xFF ; le_mask >>= 4 ;
    vt0 = _mm_loadu_si128((__m128i *)s) ;  s += 4 ;
    vs0 = _mm_loadu_si128((__m128i *)(cmp_le + mask0)) ;
    vt0 = _mm_shuffle_epi8(vt0, vs0) ;
    pop0 = popcnt_32(mask0) ;
    _mm_storeu_si128((__m128i *)d, vt0) ; d += pop0 ;
  }
  return d ;
}
#endif

#if defined(__AVX512F__)
// store-compress 32 items according to mask (AVX512 code)
// mask is little endian style, src[0] is controlled by the mask LSB
static void *CompressStore_32_avx512_le(void *src, void *dst, uint32_t le_mask){
  uint32_t *w32 = (uint32_t *) src ;
  uint32_t *dest = (uint32_t *) dst ;
  __m512i vd0 = _mm512_loadu_epi32(w32) ;
  __m512i vd1 = _mm512_loadu_epi32(w32+16) ;
  vd0 = _mm512_maskz_compress_epi32(le_mask & 0xFFFF, vd0) ; // in register compress
  _mm512_storeu_epi32(dest, vd0) ;
//   _mm512_mask_compressstoreu_epi32 (dest, le_mask & 0xFFFF, vd0) ;
  dest += _mm_popcnt_u32(le_mask & 0xFFFF) ;                 // bits 0-15
  vd1 = _mm512_maskz_compress_epi32(le_mask >> 16,  vd1) ;   // in register compress
  _mm512_storeu_epi32(dest, vd1) ;
//   _mm512_mask_compressstoreu_epi32 (dest, le_mask >> 16,  vd1) ;
  dest += _mm_popcnt_u32(le_mask >> 16) ;                    // bits 16-31
  return dest ;
}
#endif

// store-compress 32 items according to mask (plain C code)
// mask is big endian style, src[0] is controlled by the mask MSB
static inline uint32_t *CompressStore_32_c_be(uint32_t *src, uint32_t *dst, uint32_t be_mask){
  int i ;
uint32_t *dst0 = dst ;
fprintf(stderr, "32_c_be mask = %8.8x %8.8x\n", be_mask, *src) ;
  for(i=0 ; i<8 ; i++){    // explicit unroll by 4
    *dst = *src++ ;
    dst += (be_mask >> 31) ;
fprintf(stderr, "%1d ", be_mask >> 31) ;
    be_mask <<= 1 ;
    *dst = *src++ ;
    dst += (be_mask >> 31) ;
fprintf(stderr, "%1d ", be_mask >> 31) ;
    be_mask <<= 1 ;
    *dst = *src++ ;
    dst += (be_mask >> 31) ;
fprintf(stderr, "%1d ", be_mask >> 31) ;
    be_mask <<= 1 ;
    *dst = *src++ ;
    dst += (be_mask >> 31) ;
fprintf(stderr, "%1d ", be_mask >> 31) ;
    be_mask <<= 1 ;
  }
fprintf(stderr, " %ld %8.8x\n", dst-dst0, *src);
  return dst ;
}

// store-compress 32 items according to mask (plain C code)
// mask is little endian style, src[0] is controlled by the mask LSB
static inline uint32_t *CompressStore_32_c_le(uint32_t *src, uint32_t *dst, uint32_t le_mask){
  int i ;
uint32_t *dst0 = dst ;
fprintf(stderr, "32_c_le mask = %8.8x %8.8x\n", le_mask, *src) ;
  for(i=0 ; i<8 ; i++){    // explicit unroll by 4
    *dst = *src++ ;
    dst += (le_mask & 1) ;
fprintf(stderr, "%1d ", le_mask & 1) ;
    le_mask >>= 1 ;
    *dst = *src++ ;
    dst += (le_mask & 1) ;
fprintf(stderr, "%1d ", le_mask & 1) ;
    le_mask >>= 1 ;
    *dst = *src++ ;
    dst += (le_mask & 1) ;
fprintf(stderr, "%1d ", le_mask & 1) ;
    le_mask >>= 1 ;
    *dst = *src++ ;
    dst += (le_mask & 1) ;
fprintf(stderr, "%1d ", le_mask & 1) ;
    le_mask >>= 1 ;
  }
fprintf(stderr, " %ld %8.8x\n", dst-dst0, *src);
  return dst ;
}

// store-compress n ( 0 <= n < 32) items according to mask
// mask is big endian style, src[0] is controlled by the mask MSB
static inline uint32_t *CompressStore_0_31_c_be(uint32_t *src, uint32_t *dst, uint32_t be_mask, int nw32){
  if(nw32 > 31 || nw32 < 0) return NULL ;
  while(nw32--){
    *dst = *src++ ;
    dst += (be_mask >> 31) ;
    be_mask <<= 1 ;
  }
  return dst ;
}

// src    [IN] : "full" array
// dst [OUT] : non masked elements from "full" array
// le_mask     [IN] : 32 bits mask, if bit[I] is 1, src[I] is stored, if 0, it is skipped
// the function returns the next address to be stored into for the "dst" array
// NULL in case of error nw32 > 31 or nw32 < 0
// the mask is interpreted Little Endian style, src[0] is controlled by the mask LSB
// this version processes 0 - 31 elements from "src"
static void *CompressStore_0_31_c_le(uint32_t *src, uint32_t *dst, uint32_t le_mask, int nw32){
//   uint32_t *w32 = (uint32_t *) src ;
//   uint32_t *dest = (uint32_t *) dst ;
//   int i ;
  if(nw32 > 31 || nw32 < 0) return NULL ;
  while(nw32--){
    *dst = *src++ ;
    dst += (le_mask & 1) ;
    le_mask >>=1 ;
  }
  return dst ;
}

void *CompressStore_be(void *src, void *dst, void *le_map, int nw32);
void *CompressStore_le(void *src, void *dst, void *le_map, int nw32);

uint32_t *CompressStore_sse_be(void *s_, void *d_, void *map_, int n);
uint32_t *CompressStore_c_be(void *s_, void *d_, void *map_, int n);

void stream_expand_32(void *s_, void *d_, void *map_, int n, void *fill_);
void stream_expand_32_sse(void *s_, void *d_, void *map_, int n, void *fill_);


#endif
