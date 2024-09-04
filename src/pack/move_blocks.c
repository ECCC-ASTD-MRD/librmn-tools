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

#define NO_SIMD
#undef WITH_SIMD_

#define VERBOSE_SIMD
#define USE_SIMD_INTRINSICS
#include <rmn/simd_functions.h>
#undef WITH_SIMD_

#include <rmn/move_blocks.h>

#if 0
// lowest non zero absolute value
//                            -1 where V == 0    ABS value    bump zeros count  blend VMI0 where zero  unsigned minimum
#define MIN08(V,VMI0,V0,VZ) { V8I z=VEQ8(V,V0) ; V=ABS8I(V) ; VZ=ADD8I(VZ,z) ;  V=BLEND8(V,VMI0,z) ;   VMI0=MINU8(V,VMI0) ; }

static inline void copy_and_properties_1d(void *s_, void *d_, uint32_t ni, block_properties *bp){
  int32_t *si = (int32_t *) s_ , *s = si ;
  int32_t *di = (int32_t *) d_ , *d = di ;
  int n7 = ni & 7 ;
  V8I v, vmai, vmii, vmiu, v0, vzero, vmask ;
  V8F vf ;

  ZERO8I(v0) ;
//   vf = (V8F) _mm256_xor_si256(vf, (V8I) vf) ;
//   ZERO8( vf, V8F) ;
  ZERO8I(vzero) ;
  MASK8I(vmask, n7) ;
  SET8I(vmii, 0x7FFFFFFF) ;
  SET8I(vmai, 0x80000000) ;
  if(n7){
    MOVE8I(v,s+ 0,d+ 0) ; MIMA8I(v,vmii,vmai) ; MIN08(v,vmiu,v0,vzero) ;
    vzero = AND8I(vmask,vzero) ;
    ni -= n7 ;
    s += n7 ;
    d += n7 ;
  }
  while(ni > 32) {
    MOVE8I(v,s+ 0,d+ 0) ; MIMA8I(v,vmii,vmai) ; MIN08(v,vmiu,v0,vzero) ;
    MOVE8I(v,s+ 8,d+ 8) ; MIMA8I(v,vmii,vmai) ; MIN08(v,vmiu,v0,vzero) ;
    MOVE8I(v,s+16,d+16) ; MIMA8I(v,vmii,vmai) ; MIN08(v,vmiu,v0,vzero) ;
    MOVE8I(v,s+24,d+24) ; MIMA8I(v,vmii,vmai) ; MIN08(v,vmiu,v0,vzero) ;
    ni -= 32 ;
    s += 32 ;
    d += 32 ;
  };
  uint32_t n4 = ni / 8 ;
  s = s - (4 - n4) * 8 ;
  d = d - (4 - n4) * 8 ;
  switch(n4)
  {
    case 4 : MOVE8I(v,s+ 0,d+ 0) ; MIMA8I(v,vmii,vmai) ; MIN08(v,vmiu,v0,vzero) ;
    case 3 : MOVE8I(v,s+ 8,d+ 8) ; MIMA8I(v,vmii,vmai) ; MIN08(v,vmiu,v0,vzero) ;
    case 2 : MOVE8I(v,s+16,d+16) ; MIMA8I(v,vmii,vmai) ; MIN08(v,vmiu,v0,vzero) ;
    case 1 : MOVE8I(v,s+24,d+24) ; MIMA8I(v,vmii,vmai) ; MIN08(v,vmiu,v0,vzero) ;
    case 0 :
    default : ;
  }
  FOLD8_IS(F4MAXI, vmai,  &(bp->maxs.i) ) ;    // fold 8 element vector into a scalar
  FOLD8_IS(F4MINI, vmii,  &(bp->mins.i) ) ;
  FOLD8_IS(F4MINU, vmiu,  &(bp->min0.u) ) ;
  FOLD8_IS(F4ADDI, vzero, &(bp->zeros)) ;
}
#endif
// SIMD does not seem to be useful any more for these funtions
#undef WITH_SIMD_

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
  __m256i vm = _mm256_memmask_epi32(ni) ;  // mask for load and store operations
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
  __m256i vm = _mm256_memmask_epi32(ni) ;  // mask for load and store operations
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

// insert a contiguous block (ni x nj) of 32 bit words into array from blk
// ni    : row size (row storage size in blk)
// lni   : row storage size in array
// nj    : number of rows
static int scatter_word_block_07(void *restrict array, void *restrict blk, int ni, int lni, int nj){
  uint32_t *restrict s = (uint32_t *) blk ;
  uint32_t *restrict d = (uint32_t *) array ;
  return 0 ;
}
int scatter_word_block(void *restrict array, void *restrict blk, int ni, int lni, int nj){
  uint32_t *restrict s = (uint32_t *) blk ;
  uint32_t *restrict d = (uint32_t *) array ;

  if(ni < 8){
    return scatter_word_block_07(array, blk, ni, lni, nj) ;
  }else{
    while(nj--){
    }
  }

  return 0 ;
}

// extract a block (ni x nj) of 32 bit integers from src and store it into blk
// ni    : row size (row storage size in blk)
// lni   : row storage size in src
// nj    : number of rows
static int gather_int32_block_07(int32_t *restrict src, void *restrict blk, int ni, int lni, int nj, block_properties *bp){
  int32_t *restrict s = (int32_t *) src ;
  int32_t *restrict d = (int32_t *) blk ;
  return 0 ;
}
int gather_int32_block(int32_t *restrict src, void *restrict blk, int ni, int lni, int nj, block_properties *bp){
  int32_t *restrict s = (int32_t *) src ;
  int32_t *restrict d = (int32_t *) blk ;
  int i0, i, ni7 ;

  if(ni  <  8) {
    return gather_int32_block_07(src, blk, ni, lni, nj, bp) ;
  }else{

    ni7 = (ni & 7) ;
    ni7 = ni7 ? ni7 : 8 ;
    while(nj--){
  // #if defined(__x86_64__) && defined(__AVX2__) && defined(WITH_SIMD_)
  //     copy_and_properties_1d(s, d, ni, bp) ;
  // #else
      for(i=0 ; i<8 ; i++) d[i] = s[i] ;     // first and second chunk may overlap if ni not a multiple of 8
      for(i0 = ni7 ; i0 < ni-7 ; i0 += 8 ){
        for(i=0 ; i<8 ; i++) d[i0+i] = s[i0+i] ;
      }
  // #endif
      s += lni ; d += ni ;
    }
  }
  return 0 ;
}

// extract a block (ni x nj) of 32 bit words from f and store it into blk
// ni    : row size (row storage size in blk)
// lni   : row storage size in f
// nj    : number of rows
int get_word_block(void *restrict f, void *restrict blk, int ni, int lni, int nj){
  uint32_t *restrict s = (uint32_t *) f ;
  uint32_t *restrict d = (uint32_t *) blk ;
  int i0, i, ni7 ;
  block_properties bp ;

  if(ni  <  8) return get_word_block_07(f, blk, ni, lni, nj) ;

  ni7 = (ni & 7) ;
  ni7 = ni7 ? ni7 : 8 ;
  while(nj--){
#if defined(__x86_64__) && defined(__AVX2__) && defined(WITH_SIMD)
//     copy_and_properties_1d(s, d, ni, &bp) ;
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
