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
#define ALIAS_INTEL_SIMD_INTRINSICS
#define USE_INTEL_SIMD_INTRINSICS_
#include <rmn/simd_functions.h>

#include <rmn/move_blocks.h>

// move a block (ni x nj) of 32 bit words from src to dst
// src   : array the data comes from
// lnis  : row storage size of src
// dst   : array to receive data
// lnid  : row storage size of dst
// ni    : row size (row storage size of blk)
// nj    : number of rows
// return the number of words moved
int move_word32_block(void *restrict src, int lnis, void *restrict dst, int lnid, int ni, int nj){
  uint32_t *restrict d = (uint32_t *) dst ;
  uint32_t *restrict s = (uint32_t *) src ;

  if(ni*nj == 0) return 0 ;

  if(ni < 8){
    while(nj--){
      switch(ni & 7){   // switch on row length
        //       copy value
        case 7 : d[6] = s[6] ;
        case 6 : d[5] = s[5] ;
        case 5 : d[4] = s[4] ;
        case 4 : d[3] = s[3] ;
        case 3 : d[2] = s[2] ;
        case 2 : d[1] = s[1] ;
        case 1 : d[0] = s[0] ;
        case 0 : d += lnid ; s += lnis ;   // pointers to next row
      }
    }
  }else{
    __v256i vdata ;
    int i, ni7, n ;
    ni7 = (ni & 7) ;               // modulo(ni , 8)
    while(nj--){
      uint32_t *s0, *d0 ;
      n = ni ; s0 = s ; d0 = d ;
      if(ni7){                                   // first slice with less thatn 8 elements
        vdata = loadu_v256((__v256i *)s0) ;      // load data from source array
        storeu_v256((__v256i *)d0, vdata) ;      // store into destination array (CONTIGUOUS)
        n -= ni7 ; s0 += ni7 ; d0 += ni7 ;       // bump count and pointers
      }
      while(n > 7){                              // following slices with 8 elements
        vdata = loadu_v256((__v256i *)s0) ;      // load data from source array
        storeu_v256((__v256i *)d0, vdata) ;      // store into destination array (CONTIGUOUS)
        n -= 8 ; s0 += 8 ; d0 += 8 ;
      }
      s += lnis ; d += lnid ;                    // pointers to next row
    }
  }
  return ni * nj ;
}

int gather_word32_block(void *restrict array, void *restrict blk, int ni, int lni, int nj){
  uint32_t *restrict d = (uint32_t *) blk ;
  uint32_t *restrict s = (uint32_t *) array ;

  if(ni*nj == 0) return 0 ;

  if(ni < 8){
    while(nj--){
      switch(ni & 7){   // switch on row length
        //       copy value
        case 7 : d[6] = s[6] ;
        case 6 : d[5] = s[5] ;
        case 5 : d[4] = s[4] ;
        case 4 : d[3] = s[3] ;
        case 3 : d[2] = s[2] ;
        case 2 : d[1] = s[1] ;
        case 1 : d[0] = s[0] ;
        case 0 : d += ni ; s += lni ;   // pointers to next row
      }
    }
  }else{
    __v256i vdata ;
    int i, ni7, n ;
    ni7 = (ni & 7) ;               // modulo(ni , 8)
    while(nj--){
      uint32_t *s0, *d0 ;
      n = ni ; s0 = s ; d0 = d ;
      if(ni7){                                   // first slice with less thatn 8 elements
        vdata = loadu_v256((__v256i *)s0) ;      // load data from source array
        storeu_v256((__v256i *)d0, vdata) ;      // store into destination array (CONTIGUOUS)
        n -= ni7 ; s0 += ni7 ; d0 += ni7 ;       // bump count and pointers
      }
      while(n > 7){                              // following slices with 8 elements
        vdata = loadu_v256((__v256i *)s0) ;      // load data from source array
        storeu_v256((__v256i *)d0, vdata) ;      // store into destination array (CONTIGUOUS)
        n -= 8 ; s0 += 8 ; d0 += 8 ;
      }
      s += lni ; d += ni ;                       // pointers to next row
    }
  }

  return ni * nj ;
}

// insert a contiguous block (ni x nj) of 32 bit words into array from blk
// array : array to receive extracted data
// blk   : memory block to get data from
// ni    : row size (row storage size of blk)
// lni   : row storage size of array
// nj    : number of rows
// blk   : ni * nj block to scatter
int scatter_word32_block(void *restrict array, void *restrict blk, int ni, int lni, int nj){
  uint32_t *restrict s = (uint32_t *) blk ;
  uint32_t *restrict d = (uint32_t *) array ;

  if(ni*nj == 0) return 0 ;

  if(ni < 8){
    while(nj--){
      switch(ni & 7){   // switch on row length
        //       copy value
        case 7 : d[6] = s[6] ;
        case 6 : d[5] = s[5] ;
        case 5 : d[4] = s[4] ;
        case 4 : d[3] = s[3] ;
        case 3 : d[2] = s[2] ;
        case 2 : d[1] = s[1] ;
        case 1 : d[0] = s[0] ;
        case 0 : d += lni ; s += ni ;   // pointers to next row
      }
    }
  }else{
    __v256i vdata ;
    int i, ni7, n ;
    ni7 = (ni & 7) ;               // modulo(ni , 8)
    while(nj--){
      uint32_t *s0, *d0 ;
      n = ni ; s0 = s ; d0 = d ;
      if(ni7){                                   // first slice with less thatn 8 elements
        vdata = loadu_v256((__v256i *)s0) ;      // load data from source array
        storeu_v256((__v256i *)d0, vdata) ;      // store into destination array (CONTIGUOUS)
        n -= ni7 ; s0 += ni7 ; d0 += ni7 ;       // bump count and pointers
      }
      while(n > 7){                              // following slices with 8 elements
        vdata = loadu_v256((__v256i *)s0) ;      // load data from source array
        storeu_v256((__v256i *)d0, vdata) ;      // store into destination array (CONTIGUOUS)
        n -= 8 ; s0 += 8 ; d0 += 8 ;
      }
      s += ni ; d += lni ;                       // pointers to next row
    }
  }

  return 0 ;
}

// fold 8 value vectors for min /max / min_abs into scalars and store into bp
static void fold_properties(__v256i vmaxs, __v256i vmins, __v256i vminu, block_properties *bp){
  int32_t ti[8], tu[8], i ;

  storeu_v256( (__v256i *)tu , vminu ) ;
  for(i=0 ; i<8 ; i++) tu[0] = (tu[i] < tu[0]) ? tu[i] : tu[0] ;
  bp->minu.u = tu[0] ;
  storeu_v256( (__v256i *)ti , vmins ) ;
  for(i=0 ; i<8 ; i++) ti[0] = (ti[i] < ti[0]) ? ti[i] : ti[0] ;
  bp->mins.i = ti[0] ;
  storeu_v256( (__v256i *)ti , vmaxs ) ;
  for(i=0 ; i<8 ; i++) ti[0] = (ti[i] > ti[0]) ? ti[i] : ti[0] ;
  bp->maxs.i = ti[0] ;
}

// restore integer representing flot to original float bit pattern
static int32_t unfake_float(int32_t fake){
  return ((fake >> 31) ^ fake) | (fake & 0x80000000) ;
}

#define MIN(OLD,NEW) OLD = (NEW < OLD) ? NEW : OLD
#define MAX(OLD,NEW) OLD = (NEW > OLD) ? NEW : OLD

// extract a block (ni x nj) of 32 bit integers from src and store it into blk
// src   : float array to extract data from (NON CONTIGUOUS storage)
// blk   : array to put extracted data into (CONTIGUOUS storage)
// ni    : row size (row storage size in blk)
// lni   : row storage size in src
// nj    : number of rows
// bp    : block properties (min / max / min abs)
// return number of values processed
int move_float_block(float *restrict src, int lnis, void *restrict dst, int lnid, int ni, int nj, block_properties *bp){
  if(bp == NULL) return move_word32_block(src, lnis, dst, lnid, ni, nj) ;
  return ni * nj ;
}
int gather_float_block(float *restrict src, void *restrict blk, int ni, int lni, int nj, block_properties *bp){
  int32_t *restrict s = (int32_t *) src ;
  int32_t *restrict d = (int32_t *) blk ;

  if(ni*nj == 0) return 0 ;

  if(ni  <  8) {
    int32_t maxs = 0x80000000, mins = 0x7FFFFFFF, t ;
    uint32_t minu = 0x7FFFFFFF, ta ;
    while(nj--){
      switch(ni & 7){   // switch on row length
        //       copy value        absolute value        fake integer   signed min    signed max    abs value min
        case 7 : d[6] = t = s[6] ; ta = (t < 0) ? -t : t ; t=ta^(t>>31) ; MIN(mins,t) ; MAX(maxs,t) ; MIN(minu,ta) ;
        case 6 : d[5] = t = s[5] ; ta = (t < 0) ? -t : t ; t=ta^(t>>31) ; MIN(mins,t) ; MAX(maxs,t) ; MIN(minu,ta) ;
        case 5 : d[4] = t = s[4] ; ta = (t < 0) ? -t : t ; t=ta^(t>>31) ; MIN(mins,t) ; MAX(maxs,t) ; MIN(minu,ta) ;
        case 4 : d[3] = t = s[3] ; ta = (t < 0) ? -t : t ; t=ta^(t>>31) ; MIN(mins,t) ; MAX(maxs,t) ; MIN(minu,ta) ;
        case 3 : d[2] = t = s[2] ; ta = (t < 0) ? -t : t ; t=ta^(t>>31) ; MIN(mins,t) ; MAX(maxs,t) ; MIN(minu,ta) ;
        case 2 : d[1] = t = s[1] ; ta = (t < 0) ? -t : t ; t=ta^(t>>31) ; MIN(mins,t) ; MAX(maxs,t) ; MIN(minu,ta) ;
        case 1 : d[0] = t = s[0] ; ta = (t < 0) ? -t : t ; t=ta^(t>>31) ; MIN(mins,t) ; MAX(maxs,t) ; MIN(minu,ta) ;
        case 0 : d += ni ; s += lni ;   // pointers to next row
      }
    }
    bp->maxs.i = maxs ; bp->mins.i = mins ; bp->minu.u = minu ;
  }else{      // (ni  <  8)
    __v256i vmaxs, vmins, vminu, vdata, vmask, vsign, v1111, v0111, vfabs, vfake ;
    int32_t *s0, *d0 ;
    int i, ni7, n ;

    v1111 = ones_v256() ;
    v0111 = srli_v8i(v1111, 1)  ;  // 0x7FFFFFFF sign mask to get absolute value
    vmins = srli_v8i(v1111, 1)  ;  // 0x7FFFFFFF  huge positive
    vmaxs = slli_v8i(v1111, 31) ;  // 0x80000000  huge negative
    vminu = vmins ;                // 0x7FFFFFFF  huge positive
    ni7 = (ni & 7) ;               // modulo(ni , 8)
    while(nj--){                                 // loop over rows
      n = ni ; s0 = s ; d0 = d ;
      if(ni7){                                   // first slice with less thatn 8 elements
        vdata = loadu_v256((__v256i *)s0) ;      // load data from source array
        vfabs = and_v256(vdata, v0111) ;         // absolute value (suppress sign bit)
        vsign = srai_v8i(vdata, 31) ;            // 0 / -1 for sign
        vfake = xor_v256(vfabs, vsign) ;         // fake signed integer value representing float value
        vminu = min_v8u(vminu, vfabs) ;          // minimum absolute value
        vmaxs = max_v8i(vmaxs, vfake) ;          // maximum signed value
        vmins = min_v8i(vmins, vfake) ;          // minimum signed value
        storeu_v256((__v256i *)d0, vdata) ;      // store into destination array (CONTIGUOUS)
        n -= ni7 ; s0 += ni7 ; d0 += ni7 ;       // bump count and pointers
      }
      while(n > 7){                              // following slices with 8 elements
        vdata = loadu_v256((__v256i *)s0) ;      // load data from source array
        vfabs = and_v256(vdata, v0111) ;         // absolute value (suppress sign bit)
        vsign = srai_v8i(vdata, 31) ;            // 0 / -1 for sign
        vfake = xor_v256(vfabs, vsign) ;         // fake signed integer value representing float value
        vminu = min_v8u(vminu, vfabs) ;          // minimum absolute value
        vmaxs = max_v8i(vmaxs, vfake) ;          // maximum signed value
        vmins = min_v8i(vmins, vfake) ;          // minimum signed value
        storeu_v256((__v256i *)d0, vdata) ;      // store into destination array (CONTIGUOUS)
        n -= 8 ; s0 += 8 ; d0 += 8 ;
      }
      s += lni ; d += ni ;                       // pointers to next row
    }
    fold_properties(vmaxs, vmins, vminu, bp) ; // fold results into a single scalar
  }      // (ni  <  8)
  // translate signed values back into floats
  bp->maxs.i = unfake_float(bp->maxs.i) ;
  bp->mins.i = unfake_float(bp->mins.i) ;
  return ni * nj ;
}
int gather_float_block_(float *restrict src, void *restrict blk, int ni, int lni, int nj, block_properties *bp){
  return ni * nj ;
}

// extract a block (ni x nj) of 32 bit integers from src and store it into blk
// src   : integer array to extract data from (NON CONTIGUOUS storage)
// blk   : array to put extracted data into (CONTIGUOUS storage)
// ni    : row size (row storage size in blk)
// lni   : row storage size in src
// nj    : number of rows
// bp    : block properties (min / max / min abs)
// return number of values processed
int gather_int32_block(int32_t *restrict src, void *restrict blk, int ni, int lni, int nj, block_properties *bp){
  int32_t *restrict s = (int32_t *) src ;
  int32_t *restrict d = (int32_t *) blk ;

  if(ni*nj == 0) return 0 ;

  if(ni  <  8) {
    int32_t maxs = 0x80000000, mins = 0x7FFFFFFF, t ;
    uint32_t minu = 0x7FFFFFFF, ta ;
    while(nj--){
      switch(ni & 7){   // switch on row length
        //       copy value        absolute value        signed min    signed max    abs value min
        case 7 : d[6] = t = s[6] ; ta = t & 0x7FFFFFFF ; MIN(mins,t) ; MAX(maxs,t) ; MIN(minu,ta) ;
        case 6 : d[5] = t = s[5] ; ta = t & 0x7FFFFFFF ; MIN(mins,t) ; MAX(maxs,t) ; MIN(minu,ta) ;
        case 5 : d[4] = t = s[4] ; ta = t & 0x7FFFFFFF ; MIN(mins,t) ; MAX(maxs,t) ; MIN(minu,ta) ;
        case 4 : d[3] = t = s[3] ; ta = t & 0x7FFFFFFF ; MIN(mins,t) ; MAX(maxs,t) ; MIN(minu,ta) ;
        case 3 : d[2] = t = s[2] ; ta = t & 0x7FFFFFFF ; MIN(mins,t) ; MAX(maxs,t) ; MIN(minu,ta) ;
        case 2 : d[1] = t = s[1] ; ta = t & 0x7FFFFFFF ; MIN(mins,t) ; MAX(maxs,t) ; MIN(minu,ta) ;
        case 1 : d[0] = t = s[0] ; ta = t & 0x7FFFFFFF ; MIN(mins,t) ; MAX(maxs,t) ; MIN(minu,ta) ;
        case 0 : d += ni ; s += lni ;   // pointers to next row
      }
    }
    bp->maxs.i = maxs ; bp->mins.i = mins ; bp->minu.u = minu ;
  }else{      // (ni  <  8)
    __v256i vmaxs, vmins, vminu, vdata, vmask, v1111, viabs ;
    int32_t *s0, *d0 ;
    int i, ni7, n ;

    v1111 = ones_v256() ;
    vmins = srli_v8i(v1111, 1)  ;  // 0x7FFFFFFF  huge positive
    vmaxs = slli_v8i(v1111, 31) ;  // 0x80000000  huge negative
    vminu = vmins ;                // 0x7FFFFFFF  huge positive
    ni7 = (ni & 7) ;               // modulo(ni , 8)
    while(nj--){                                 // loop over rows
      n = ni ; s0 = s ; d0 = d ;
      if(ni7){                                   // first slice with less thatn 8 elements
        vdata = loadu_v256((__v256i *)s0) ;      // load data from source array
        viabs = abs_v8i(vdata) ;                 // absolute value
        vminu = min_v8u(vminu, viabs) ;          // minimum absolute value
        vmaxs = max_v8i(vmaxs, vdata) ;          // maximum signed value
        vmins = min_v8i(vmins, vdata) ;          // minimum signed value
        storeu_v256((__v256i *)d0, vdata) ;      // store into destination array (CONTIGUOUS)
        n -= ni7 ; s0 += ni7 ; d0 += ni7 ;       // bump count and pointers
      }
      while(n > 7){                              // following slices with 8 elements
        vdata = loadu_v256((__v256i *)s0) ;      // load data from source array
        viabs = abs_v8i(vdata) ;                 // absolute value
        vminu = min_v8u(vminu, viabs) ;          // minimum absolute value
        vmaxs = max_v8i(vmaxs, vdata) ;          // maximum signed value
        vmins = min_v8i(vmins, vdata) ;          // minimum signed value
        storeu_v256((__v256i *)d0, vdata) ;      // store into destination array (CONTIGUOUS)
        n -= 8 ; s0 += 8 ; d0 += 8 ;
      }
      s += lni ; d += ni ;                       // pointers to next row
    }
    fold_properties(vmaxs, vmins, vminu, bp) ; // fold results into a single scalar
  }      // (ni  <  8)

  return ni * nj ;
}
// int gather_int32_block(int32_t *restrict src, void *restrict blk, int ni, int lni, int nj, block_properties *bp){
//   int32_t *restrict s = (int32_t *) src ;
//   int32_t *restrict d = (int32_t *) blk ;
//   int i0, i, ni7 ;
// 
//   if(ni  <  8) {
//     return gather_int32_block_07(src, blk, ni, lni, nj, bp) ;
//   }else{
// 
//     ni7 = (ni & 7) ;
//     ni7 = ni7 ? ni7 : 8 ;
//     while(nj--){
//   // #if defined(__x86_64__) && defined(__AVX2__) && defined(WITH_SIMD_)
//   //     copy_and_properties_1d(s, d, ni, bp) ;
//   // #else
//       for(i=0 ; i<8 ; i++) d[i] = s[i] ;     // first and second chunk may overlap if ni not a multiple of 8
//       for(i0 = ni7 ; i0 < ni-7 ; i0 += 8 ){
//         for(i=0 ; i<8 ; i++) d[i0+i] = s[i0+i] ;
//       }
//   // #endif
//       s += lni ; d += ni ;
//     }
//   }
//   return ni * nj ;
// }

#if 0
// lowest non zero absolute value
//                            -1 where V == 0    ABS value    bump zeros count  blend VMI0 where zero  unsigned minimum
#define MIN08(V,VMI0,V0,VZ) { V8I z=VEQ8(V,V0) ; V=ABS8I(V) ; VZ=ADD8I(VZ,z) ;  V=BLEND8(V,VMI0,z) ;   VMI0=MINU8(V,VMI0) ; }
#endif

// SIMD does not seem to be useful any more for these funtions
#undef WITH_SIMD

#if ! defined(__INTEL_COMPILER_UPDATE) &&  ! defined(__PGI)
// give an explicit hint to the gcc optimizer
#pragma GCC optimize "tree-vectorize"
#endif

// special case for rows shorter than 8 elements
// insert a contiguous block (ni x nj) of 32 bit words into f from blk
// ni    : row size (row storage size in blk)
// lni   : row storage size in f
// nj    : number of rows
static int put_word_block_07(void *restrict f, void *restrict blk, int ni, int lni, int nj){
  uint32_t *restrict d = (uint32_t *) f ;
  uint32_t *restrict s = (uint32_t *) blk ;
  int i, ni7 ;
#if defined(__x86_64__) && defined(__AVX2__) && defined(WITH_SIMD)
  __m256i vm = _mm256_memmask_epi32(ni) ;  // mask for load and store operations
#endif

  if(ni > 7 || ni < 0) return -1 ;

  ni7 = (ni & 7) ;
  while(nj--){
#if defined(__x86_64__) && defined(__AVX2__) && defined(WITH_SIMD)
    _mm256_maskstore_epi32 ((int *)d, vm, _mm256_maskload_epi32 ((int const *) s, vm) ) ;
#else
    for(i=0 ; i < ni7 ; i++) d[i] = s[i] ;
#endif
    s += ni ; d += lni ;
  }
  return ni * nj ;
}

// special case for rows shorter than 8 elements
// extract a contiguous block (ni x nj) of 32 bit words from f into blk
// ni    : row size (row storage size in blk)
// lni   : row storage size in f
// nj    : number of rows
static int get_word_block_07(void *restrict f, void *restrict blk, int ni, int lni, int nj){
  uint32_t *restrict s = (uint32_t *) f ;
  uint32_t *restrict d = (uint32_t *) blk ;
  int i, ni7 ;
#if defined(__x86_64__) && defined(__AVX2__) && defined(WITH_SIMD)
  __m256i vm = _mm256_memmask_epi32(ni) ;  // mask for load and store operations
#endif

  if(ni > 7 || ni < 0) return -1 ;

  ni7 = (ni & 7) ;
  while(nj--){
#if defined(__x86_64__) && defined(__AVX2__) && defined(WITH_SIMD)
    _mm256_maskstore_epi32 ((int *)d, vm, _mm256_maskload_epi32 ((int const *) s, vm) ) ;
#else
    for(i=0 ; i < ni7 ; i++) d[i] = s[i] ;
#endif
    s += lni ; d += ni ;
  }
  return ni * nj ;
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
