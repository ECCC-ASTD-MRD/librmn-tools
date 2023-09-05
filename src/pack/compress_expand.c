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
// N.B. the SSE2 versions are 3-5 x faster than the plain C versions (compiler dependent)
//
#include <stdio.h>

#include <rmn/compress_expand.h>
#include <rmn/bits.h>
#include <immintrin.h>

static struct exptab{
  int8_t stab[16] ;
} exptab[16] = {
  { -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 0000
  { -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1,     0,  1,  2,  3 } ,  // 0001
  { -1, -1, -1, -1,    -1, -1, -1, -1,     0,  1,  2,  3,    -1, -1, -1, -1 } ,  // 0010
  { -1, -1, -1, -1,    -1, -1, -1, -1,     0,  1,  2,  3,     4,  5,  6,  7 } ,  // 0011
  { -1, -1, -1, -1,     0,  1,  2,  3,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 0100
  { -1, -1, -1, -1,     0,  1,  2,  3,    -1, -1, -1, -1,     4,  5,  6,  7 } ,  // 0101
  { -1, -1, -1, -1,     0,  1,  2,  3,     4,  5,  6,  7,    -1, -1, -1, -1 } ,  // 0110
  { -1, -1, -1, -1,     0,  1,  2,  3,     4,  5,  6,  7,     8,  9, 10, 11 } ,  // 0111
  {  0,  1,  2,  3,    -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 1000
  {  0,  1,  2,  3,    -1, -1, -1, -1,    -1, -1, -1, -1,     4,  5,  6,  7 } ,  // 1001
  {  0,  1,  2,  3,    -1, -1, -1, -1,     4,  5,  6,  7,    -1, -1, -1, -1 } ,  // 1010
  {  0,  1,  2,  3,    -1, -1, -1, -1,     4,  5,  6,  7,     8,  9, 10, 11 } ,  // 1011
  {  0,  1,  2,  3,     4,  5,  6,  7,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 1100
  {  0,  1,  2,  3,     4,  5,  6,  7,    -1, -1, -1, -1,     8,  9, 10, 11 } ,  // 1101
  {  0,  1,  2,  3,     4,  5,  6,  7,     8,  9, 10, 11,    -1, -1, -1, -1 } ,  // 1110
  {  0,  1,  2,  3,     4,  5,  6,  7,     8,  9, 10, 11,    12, 13, 14, 15 } ,  // 1111
             } ;

// SSE2 version of stream_replace_32
static void stream_replace_32_sse(uint32_t *s, uint32_t *d, uint32_t *masks, int npts){
  int i, j ;
  uint32_t mask, mask0, pop0, m31 ;
  __m128i vt0, vs0, vf0, vb0 ;

  for(j=0 ; j<npts-31 ; j+=32){                            // chunks of 32
    mask = *masks++ ;
    for(i=0 ; i<2 ; i++){                                  // explicit unroll by 4
      mask0 = mask >> 28 ; mask <<= 4 ; pop0 = popcnt_32(mask0) ;
      vt0 = _mm_loadu_si128((__m128i *)s) ; s += pop0 ;
      vs0 = _mm_loadu_si128((__m128i *)(exptab + mask0)) ;
      vf0 = _mm_loadu_si128((__m128i *)d) ;
      vb0 = _mm_srai_epi32(vs0, 31) ;                      // 1s where old, 0s where new
      vt0 = _mm_shuffle_epi8(vt0, vs0) ;
      vt0 = _mm_blendv_epi8(vt0, vf0, vb0) ;
      _mm_storeu_si128((__m128i *)d, vt0) ; d += 4 ;
      mask0 = mask >> 28 ; mask <<= 4 ; pop0 = popcnt_32(mask0) ;
      vt0 = _mm_loadu_si128((__m128i *)s) ; s += pop0 ;
      vs0 = _mm_loadu_si128((__m128i *)(exptab + mask0)) ;
      vf0 = _mm_loadu_si128((__m128i *)d) ;
      vb0 = _mm_srai_epi32(vs0, 31) ;                      // 1s where old, 0s where new
      vt0 = _mm_shuffle_epi8(vt0, vs0) ;
      vt0 = _mm_blendv_epi8(vt0, vf0, vb0) ;
      _mm_storeu_si128((__m128i *)d, vt0) ; d += 4 ;
      mask0 = mask >> 28 ; mask <<= 4 ; pop0 = popcnt_32(mask0) ;
      vt0 = _mm_loadu_si128((__m128i *)s) ; s += pop0 ;
      vs0 = _mm_loadu_si128((__m128i *)(exptab + mask0)) ;
      vf0 = _mm_loadu_si128((__m128i *)d) ;
      vb0 = _mm_srai_epi32(vs0, 31) ;                      // 1s where old, 0s where new
      vt0 = _mm_shuffle_epi8(vt0, vs0) ;
      vt0 = _mm_blendv_epi8(vt0, vf0, vb0) ;
      _mm_storeu_si128((__m128i *)d, vt0) ; d += 4 ;
      mask0 = mask >> 28 ; mask <<= 4 ; pop0 = popcnt_32(mask0) ;
      vt0 = _mm_loadu_si128((__m128i *)s) ; s += pop0 ;
      vs0 = _mm_loadu_si128((__m128i *)(exptab + mask0)) ;
      vf0 = _mm_loadu_si128((__m128i *)d) ;
      vb0 = _mm_srai_epi32(vs0, 31) ;                      // 1s where old, 0s where new
      vt0 = _mm_shuffle_epi8(vt0, vs0) ;
      vt0 = _mm_blendv_epi8(vt0, vf0, vb0) ;
      _mm_storeu_si128((__m128i *)d, vt0) ; d += 4 ;
    }
  }
  mask = *masks++ ;
  for(    ; j<npts ; j++){                                 // leftovers (0 -> 31)
    m31 = mask >> 31 ;
    *d = m31 ? *s : *d ;
    d++ ;
    s += m31 ;
    mask <<= 1 ;
  }
}

static void stream_replace_32(uint32_t *s, uint32_t *d, uint32_t *masks, int npts){
  int i, j ;
  uint32_t mask, m31 ;

  for(j=0 ; j<npts-31 ; j+=32){
    mask = *masks++ ;
    for(i=0 ; i<32 ; i++){
      m31 = mask >> 31 ;
      *d = m31 ? *s : *d ;
      d++ ;
      s += m31 ;
      mask <<= 1 ;
    }
  }
  mask = *masks++ ;
  for(    ; j<npts ; j++){
    m31 = mask >> 31 ;
    *d = m31 ? *s : *d ;
    d++ ;
    s += m31 ;
    mask <<= 1 ;
  }
}

// SSE2 version of stream_expand_32
void stream_expand_32_sse(uint32_t *s, uint32_t *d, uint32_t *masks, int npts, uint32_t *pfill){
  int i, j ;
  uint32_t mask, fill, mask0, pop0, m31 ;
  __m128i vt0, vs0, vf0, vb0 ;


  if(pfill == NULL){
    stream_replace_32_sse(s, d, masks, npts) ;
    return ;
  }
  fill = *pfill ;
  vf0 = _mm_set1_epi32(fill) ;
  for(j=0 ; j<npts-31 ; j+=32){                            // chunks of 32
    mask = *masks++ ;
    for(i=0 ; i<2 ; i++){                                  // explicit unroll by 4
      mask0 = mask >> 28 ; mask <<= 4 ; pop0 = popcnt_32(mask0) ;
      vt0 = _mm_loadu_si128((__m128i *)s) ; s += pop0 ;
      vs0 = _mm_loadu_si128((__m128i *)(exptab + mask0)) ;
      vb0 = _mm_srai_epi32(vs0, 31) ;                      // 1s where fill, 0s where new
      vt0 = _mm_shuffle_epi8(vt0, vs0) ;
      vt0 = _mm_blendv_epi8(vt0, vf0, vb0) ;
      _mm_storeu_si128((__m128i *)d, vt0) ; d += 4 ;
      mask0 = mask >> 28 ; mask <<= 4 ; pop0 = popcnt_32(mask0) ;
      vt0 = _mm_loadu_si128((__m128i *)s) ; s += pop0 ;
      vs0 = _mm_loadu_si128((__m128i *)(exptab + mask0)) ;
      vb0 = _mm_srai_epi32(vs0, 31) ;                      // 1s where fill, 0s where new
      vt0 = _mm_shuffle_epi8(vt0, vs0) ;
      vt0 = _mm_blendv_epi8(vt0, vf0, vb0) ;
      _mm_storeu_si128((__m128i *)d, vt0) ; d += 4 ;
      mask0 = mask >> 28 ; mask <<= 4 ; pop0 = popcnt_32(mask0) ;
      vt0 = _mm_loadu_si128((__m128i *)s) ; s += pop0 ;
      vs0 = _mm_loadu_si128((__m128i *)(exptab + mask0)) ;
      vb0 = _mm_srai_epi32(vs0, 31) ;                      // 1s where fill, 0s where new
      vt0 = _mm_shuffle_epi8(vt0, vs0) ;
      vt0 = _mm_blendv_epi8(vt0, vf0, vb0) ;
      _mm_storeu_si128((__m128i *)d, vt0) ; d += 4 ;
      mask0 = mask >> 28 ; mask <<= 4 ; pop0 = popcnt_32(mask0) ;
      vt0 = _mm_loadu_si128((__m128i *)s) ; s += pop0 ;
      vs0 = _mm_loadu_si128((__m128i *)(exptab + mask0)) ;
      vb0 = _mm_srai_epi32(vs0, 31) ;                      // 1s where fill, 0s where new
      vt0 = _mm_shuffle_epi8(vt0, vs0) ;
      vt0 = _mm_blendv_epi8(vt0, vf0, vb0) ;
      _mm_storeu_si128((__m128i *)d, vt0) ; d += 4 ;
    }
  }
  mask = *masks++ ;
  for(    ; j<npts ; j++){                                 // leftovers (0 -> 31)
    m31 = mask >> 31 ;
    *d = m31 ? *s : fill ;
    d++ ;
    s += m31 ;
    mask <<= 1 ;
  }
}

void stream_expand_32(uint32_t *s, uint32_t *d, uint32_t *masks, int npts, uint32_t *pfill){
  int i, j ;
  uint32_t mask, m31, fill ;

  if(pfill == NULL){
    stream_replace_32(s, d, masks, npts) ;
    return ;
  }
  fill = *pfill ;
  for(j=0 ; j<npts-31 ; j+=32){
    mask = *masks++ ;
    for(i=0 ; i<32 ; i++){
      m31 = mask >> 31 ;
      *d = m31 ? *s : fill ;
      d++ ;
      s += m31 ;
      mask <<= 1 ;
    }
  }
  mask = *masks++ ;
  for(    ; j<npts ; j++){
    m31 = mask >> 31 ;
    *d = m31 ? *s : fill ;
    d++ ;
    s += m31 ;
    mask <<= 1 ;
  }
}

static struct cmptab{
  int8_t stab[16] ;
} cmptab[16] = {
  { -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 0000
  { 12, 13, 14, 15,    -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 0001
  {  8,  9, 10, 11,    -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 0010
  {  8,  9, 10, 11,    12, 13, 14, 15,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 0011
  {  4,  5,  6,  7,    -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 0100
  {  4,  5,  6,  7,    12, 13, 14, 15,    -1, -1, -1, -1 ,   -1, -1, -1, -1 } ,  // 0101
  {  4,  5,  6,  7,     8,  9, 10, 11,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 0110
  {  4,  5,  6,  7,     8,  9, 10, 11,    12, 13, 14, 15,    -1, -1, -1, -1 } ,  // 0111
  {  0,  1,  2,  3,    -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 1000
  {  0,  1,  2,  3,    12, 13, 14, 15,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 1001
  {  0,  1,  2,  3,     8,  9, 10, 11,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 1010
  {  0,  1,  2,  3,     8,  9, 10, 11,    12, 13, 14, 15,    -1, -1, -1, -1 } ,  // 1011
  {  0,  1,  2,  3,     4,  5,  6,  7,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 1100
  {  0,  1,  2,  3,     4,  5,  6,  7,    12, 13, 14, 15,    -1, -1, -1, -1 } ,  // 1101
  {  0,  1,  2,  3,     4,  5,  6,  7,     8,  9, 10, 11,    -1, -1, -1, -1 } ,  // 1110
  {  0,  1,  2,  3,     4,  5,  6,  7,     8,  9, 10, 11,    12, 13, 14, 15 } ,  // 1111
             } ;

// SSE2 version of stream_compress_32
uint32_t *stream_compress_32_sse(uint32_t *s, uint32_t *d, uint32_t *masks, int npts){
  int i, j ;
  uint32_t mask, mask0, pop0 ;
  __m128i vt0, vs0 ;

  for(j=0 ; j<npts-31 ; j+=32){                            // chunks of 32
    mask = *masks++ ;
    for(i=0 ; i<2 ; i++){                                  // explicit unroll by 4
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
      mask0 = mask >> 28 ; mask <<= 4 ;
      vt0 = _mm_loadu_si128((__m128i *)s) ;  s += 4 ;
      vs0 = _mm_loadu_si128((__m128i *)(cmptab + mask0)) ;
      vt0 = _mm_shuffle_epi8(vt0, vs0) ;
      pop0 = popcnt_32(mask0) ;
      _mm_storeu_si128((__m128i *)d, vt0) ; d += pop0 ;
    }
  }
  mask = *masks ;
  for(    ; j<npts ; j++){                                 // leftovers (0 -> 31)
    *d = *s++ ;
    d += (mask >> 31) ;
    mask <<= 1 ;
  }
  return d ;
}

// plain C version
uint32_t *stream_compress_32(uint32_t *s, uint32_t *d, uint32_t *masks, int npts){
  int i, j ;
  uint32_t mask ;

  for(j=0 ; j<npts-31 ; j+=32){                            // chunks of 32
    mask = *masks++ ;
    for(i=0 ; i<32 ; i++){
      *d = *s++ ;
      d += (mask >> 31) ;
      mask <<= 1 ;
    }
  }
  mask = *masks ;
  for(    ; j<npts ; j++){                                 // leftovers (0 -> 31)
    *d = *s++ ;
    d += (mask >> 31) ;
    mask <<= 1 ;
  }
  return d ;
}
