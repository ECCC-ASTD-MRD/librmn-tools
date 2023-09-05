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

static struct cmptab{   // lookup permutation table used to perform a SIMD store-compress
  int8_t stab[16] ;     // elements from s[0,1,2,3] to store into d, x means fill or leave as is
} cmptab[16] = {                                                                 // MASK
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

static inline uint32_t *sse_expand_replace_32(uint32_t *s, uint32_t *d, uint32_t mask){
  int i ;
  __m128i vt0, vs0, vf0, vb0 ;
  uint32_t mask0, pop0 ;

  for(i=0 ; i<2 ; i++){                                            // explicit unroll by 4
      mask0 = mask >> 28 ; mask <<= 4 ; pop0 = popcnt_32(mask0) ;  // get 4 bit mask0 for this slice
      vt0 = _mm_loadu_si128((__m128i *)s) ; s += pop0 ;            // load items from source (compressed) array
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
static inline uint32_t *sse_compress_32(uint32_t *s, uint32_t *d, uint32_t mask){
  uint32_t pop0, mask0, i ;
  __m128i vt0, vs0 ;

  for(i=0 ; i<2 ; i++){                                    // explicit unroll by 4
    mask0 = mask >> 28 ; mask <<= 4 ;
    vt0 = _mm_loadu_si128((__m128i *)s) ;  s += 4 ;        // load items from source array
    vs0 = _mm_loadu_si128((__m128i *)(cmptab + mask0)) ;   // load permutation for this mask0
    vt0 = _mm_shuffle_epi8(vt0, vs0) ;                     // shufffle to get source items in proper position
    pop0 = popcnt_32(mask0) ;                              // number of useful items that will be stored
    _mm_storeu_si128((__m128i *)d, vt0) ; d += pop0 ;      // store into destination
    mask0 = mask >> 28 ; mask <<= 4 ;
    vt0 = _mm_loadu_si128((__m128i *)s) ;  s += 4 ;
    vs0 = _mm_loadu_si128((__m128i *)(cmptab + mask0)) ;
    vt0 = _mm_shuffle_epi8(vt0, vs0) ;
    pop0 = popcnt_32(mask0) ;
    _mm_storeu_si128((__m128i *)d, vt0) ; d += pop0 ;
    mask0 = mask >> 28 ; mask <<= 4 ;
    vt0 = _mm_loadu_si128((__m128i *)s) ;  s += 4 ;
    vs0 = _mm_loadu_si128((__m128i *)(cmptab + mask0)) ;
    vt0 = _mm_shuffle_epi8(vt0, vs0) ;
    pop0 = popcnt_32(mask0) ;
    _mm_storeu_si128((__m128i *)d, vt0) ; d += pop0 ;
    mask0 = mask >> 28 ; mask <<= 4 ;
    vt0 = _mm_loadu_si128((__m128i *)s) ;  s += 4 ;
    vs0 = _mm_loadu_si128((__m128i *)(cmptab + mask0)) ;
    vt0 = _mm_shuffle_epi8(vt0, vs0) ;
    pop0 = popcnt_32(mask0) ;
    _mm_storeu_si128((__m128i *)d, vt0) ; d += pop0 ;
  }
  return d ;
}
#endif

// store-compress n ( 0 <= n < 32) items according to mask
static inline uint32_t *c_compress_n(uint32_t *s, uint32_t *d, uint32_t mask, int n){
  while(n--){
    *d = *s++ ;
    d += (mask >> 31) ;
    mask <<= 1 ;
  }
  return d ;
}

// store-compress 32 items according to mask (plain C code)
static inline uint32_t *c_compress_32(uint32_t *s, uint32_t *d, uint32_t mask){
  int i ;
  for(i=0 ; i<8 ; i++){    // explicit unroll by 4
    *d = *s++ ;
    d += (mask >> 31) ;
    mask <<= 1 ;
    *d = *s++ ;
    d += (mask >> 31) ;
    mask <<= 1 ;
    *d = *s++ ;
    d += (mask >> 31) ;
    mask <<= 1 ;
    *d = *s++ ;
    d += (mask >> 31) ;
    mask <<= 1 ;
  }
  return d ;
}

uint32_t *stream_compress_32(void *s_, void *d_, void *map_, int n);
uint32_t *stream_compress_32_sse(void *s_, void *d_, void *map_, int n);

void stream_expand_32(void *s_, void *d_, void *map_, int n, void *fill_);
void stream_expand_32_sse(void *s_, void *d_, void *map_, int n, void *fill_);

#endif
