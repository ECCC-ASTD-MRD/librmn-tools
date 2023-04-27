// Hopefully useful code for C (memory block movers)
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

#include <stdint.h>

#include "with_simd.h"
#include <rmn/misc_operators.h>

// SIMD does not seem to be useful any more for these funtions
#undef WITH_SIMD

#if ! defined(__INTEL_COMPILER_UPDATE) &&  ! defined(__PGI)
#pragma GCC optimize "tree-vectorize"
#endif

// special case for rows shorter than 8 elements
// insert a contiguous block (ni x nj) of 32 bit words into f from blk
// ni    : row size (row storage size in blk)
// lni   : row storage size in f
// nj    : number of rows
int put_word_block_07(void *restrict f, void *restrict blk, int ni, int lni, int nj){
  uint32_t *restrict d = (uint32_t *) f ;
  uint32_t *restrict s = (uint32_t *) blk ;
  int i, ni7 ;
#if defined(__x86_64__) && defined(__AVX2__) && defined(WITH_SIMD)
  __m256i vm = _mm256_memmask_si256(ni) ;  // mask for load and store operations
#endif

  if(ni > 7) return 1 ;
  ni7 = (ni & 7) ;
  while(nj--){
#if defined(__x86_64__) && defined(__AVX2__) && defined(WITH_SIMD)
    _mm256_maskstore_epi32 ((int *)d, vm, _mm256_maskload_epi32 ((int const *) s, vm) ) ;
#else
    for(i=0 ; i < ni7 ; i++) d[i] = s[i] ;
#endif
    s += ni ; d += lni ;
  }
  return 0 ;
}

// special case for rows shorter than 8 elements
// extract a contiguous block (ni x nj) of 32 bit words from f into blk
// ni    : row size (row storage size in blk)
// lni   : row storage size in f
// nj    : number of rows
int get_word_block_07(void *restrict f, void *restrict blk, int ni, int lni, int nj){
  uint32_t *restrict s = (uint32_t *) f ;
  uint32_t *restrict d = (uint32_t *) blk ;
  int i, ni7 ;
#if defined(__x86_64__) && defined(__AVX2__) && defined(WITH_SIMD)
  __m256i vm = _mm256_memmask_si256(ni) ;  // mask for load and store operations
#endif

  if(ni > 7) return 1 ;
  ni7 = (ni & 7) ;
  while(nj--){
#if defined(__x86_64__) && defined(__AVX2__) && defined(WITH_SIMD)
    _mm256_maskstore_epi32 ((int *)d, vm, _mm256_maskload_epi32 ((int const *) s, vm) ) ;
#else
    for(i=0 ; i < ni7 ; i++) d[i] = s[i] ;
#endif
    s += lni ; d += ni ;
  }
  return 0 ;
}

// insert a contiguous block (ni x nj) of 32 bit words into f from blk
// ni    : row size (row storage size in blk)
// lni   : row storage size in f
// nj    : number of rows
int put_word_block(void *restrict f, void *restrict blk, int ni, int lni, int nj){
  uint32_t *restrict d = (uint32_t *) f ;
  uint32_t *restrict s = (uint32_t *) blk ;
  int i0, i, ni7 ;

  if(ni  <  8) return put_word_block_07(f, blk, ni, lni, nj) ;
//   if(ni ==  8) return put_word_block_08(f, blk, ni, lni, nj) ;
//   if(ni == 32) return put_word_block_32(f, blk, ni, lni, nj) ;
//   if(ni == 64) return put_word_block_64(f, blk, ni, lni, nj) ;

  ni7 = (ni & 7) ;
  ni7 = ni7 ? ni7 : 8 ;
  while(nj--){
#if defined(__x86_64__) && defined(__AVX2__) && defined(WITH_SIMD)
      _mm256_storeu_si256((__m256i *)(d), _mm256_loadu_si256((__m256i *)(s))) ;
    for(i0 = ni7 ; i0 < ni-7 ; i0 += 8 ){
      _mm256_storeu_si256((__m256i *)(d+i0), _mm256_loadu_si256((__m256i *)(s+i0))) ;
    }
#else
    for(i=0 ; i<8 ; i++) d[i] = s[i] ;     // first and second chunk may overlap if ni not a multiple of 8
    for(i0 = ni7 ; i0 < ni-7 ; i0 += 8 ){
      for(i=0 ; i<8 ; i++) d[i0+i] = s[i0+i] ;
    }
#endif
    s += ni ; d += lni ;
  }
  return 0 ;
}

// extract a contiguous block (ni x nj) of 32 bit words from f into blk
// ni    : row size (row storage size in blk)
// lni   : row storage size in f
// nj    : number of rows
int get_word_block(void *restrict f, void *restrict blk, int ni, int lni, int nj){
  uint32_t *restrict s = (uint32_t *) f ;
  uint32_t *restrict d = (uint32_t *) blk ;
  int i0, i, ni7 ;

  if(ni  <  8) return get_word_block_07(f, blk, ni, lni, nj) ;
//   if(ni ==  8) return get_word_block_08(f, blk, ni, lni, nj) ;
//   if(ni == 32) return get_word_block_32(f, blk, ni, lni, nj) ;
//   if(ni == 64) return get_word_block_64(f, blk, ni, lni, nj) ;

  ni7 = (ni & 7) ;
  ni7 = ni7 ? ni7 : 8 ;
  while(nj--){
#if defined(__x86_64__) && defined(__AVX2__) && defined(WITH_SIMD)
      _mm256_storeu_si256((__m256i *)(d), _mm256_loadu_si256((__m256i *)(s))) ;
    for(i0 = ni7 ; i0 < ni-7 ; i0 += 8 ){
      _mm256_storeu_si256((__m256i *)(d+i0), _mm256_loadu_si256((__m256i *)(s+i0))) ;
    }
#else
    for(i=0 ; i<8 ; i++) d[i] = s[i] ;     // first and second chunk may overlap if ni not a multiple of 8
    for(i0 = ni7 ; i0 < ni-7 ; i0 += 8 ){
      for(i=0 ; i<8 ; i++) d[i0+i] = s[i0+i] ;
    }
#endif
    s += lni ; d += ni ;
  }
  return 0 ;
}
