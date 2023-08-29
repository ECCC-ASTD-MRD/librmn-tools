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
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <rmn/bitmaps.h>
#include <rmn/bits.h>

// create bit maps / restore from bit maps
//   comparing floats /ints / unsigned ints to a special value (greater than, less than, masked equality)
// RLE encode / decode these bit maps

// O3 useful for gcc to get faster code
#if defined(__GNUC__) && ! defined(__INTEL_COMPILER_UPDATE) &&  ! defined(__INTEL_LLVM_COMPILER) && ! defined(__clang__) && ! defined(__PGI)
#pragma GCC optimize "O3"
#endif

// add potential guard bits at end of bitmap
// now using 4 extra bits at beginning (encoding mode) + 1 extra bit at end (inverted guard)
#define EXTRA_BITS 32

// create a bit map capable of containing nelem elements
// (can also be used to create a  pointer to an encoded (RLE) bitmap using a cast)
// nelem [IN] : max number of elements in bitmap
rmn_bitmap *bitmap_create(int nelem){
  size_t bmp_size = (nelem + EXTRA_BITS + 31)/32 ;     // number of uint32_t elements needed
  rmn_bitmap *bitmap = (rmn_bitmap *) malloc( bmp_size * sizeof(uint32_t) + sizeof(rmn_bitmap) ) ;
  bitmap->size = bmp_size ;                            // capacity of bitmap
  bitmap->elem = 0 ;                                   // number of used bits in bitmap
  bitmap->nrle = 0 ;                                   // number of used bits if RLE encoded
  bitmap->ones = 0 ;                                   // number of bits set in bit map
  return bitmap ;
}

// duplicate a bit map (possibly RLE encoded)
// bmp_src  [IN] : pointer to a valid rmn_bitmap structure to be duplicated
// bmp_dst [OUT] : pointer to a valid rmn_bitmap structure to receive the duplicate
//                 (if NULL, a new bit map will be created)
// return : pointer to duplicata, NULL if error
rmn_bitmap *bitmap_dup(rmn_bitmap *bmp_dst, rmn_bitmap *bmp_src){
  int i, to_copy ;
  if(bmp_src == NULL) return NULL ;
  if(bmp_dst == NULL) bmp_dst = bitmap_create(32 * bmp_src->size - EXTRA_BITS) ;
  if(bmp_dst->size < bmp_src->size) return NULL ;
  bmp_dst->elem = bmp_src->elem ;
  bmp_dst->nrle = bmp_src->nrle ;
  bmp_dst->ones = bmp_src->ones ;
  to_copy = (bmp_src->elem > bmp_src->nrle) ? bmp_src->elem : bmp_src->nrle ; // max(elem, nrle)
  to_copy = (to_copy + 31) / 32 ;                                             // in 32 bit word units
  for(i=0 ; i<to_copy ; i++) bmp_dst->data[i] = bmp_src->data[i] ;
  return bmp_dst ;
}

// build a 1 bit per element bitmap (big endian style) (array can be of ANY 32 bit type)
// 1 if (value & ~mask) == (special & ~mmask)
// array   [IN] : source array (32 bit elements)
// bmp    [OUT] : pointer to rmn_bitmap struct (if NULL, a new rmn_bitmap struct will be allocated)
// special [IN] : "special" value
// mmask   [IN] : mask applied to "special" value
// n       [IN] : number of elements in array
// return value : pointer to rmn_bitmap struct if successful, NULL if ther was an error
rmn_bitmap *bitmap_be_eq_01(void *array, rmn_bitmap *bmp, uint32_t special, uint32_t mmask, int n){
  uint32_t t, result, n31, ones = 0, all1 = 0, zero = 0 ;
  int32_t i, i0 ;
  rmn_bitmap *bitmap = bmp ;
  uint32_t *bits ;
  uint32_t *src = (uint32_t *) array ;

  mmask = (~mmask) ;
  special &= mmask ;
  // allocate bitmap if bmp was NULL
  if(bmp == NULL) {
    bitmap = bitmap_create(n) ;                // create an empty bitmap with the needed size
  }else{
    if(n > bitmap->size * 32) return NULL ;    // not enough room in bitmap data array data
  }
  bits = (uint32_t *) bitmap->data ;           // bitmap data array
  bitmap->elem = n ;                           // array data will contain n useful bits
  bitmap->nrle = 0 ;                           // not RLE encoded

  for(i0 = 0 ; i0 < n-31 ; i0 += 32){          // full slices of 32 elements
    result = 0 ;
    for(i=0 ; i<32 ; i++){                     // encode a 32 element slice
      t   = ((src[i] & mmask) == special) ? 1 : 0 ;   // 1 if special value, 0 if not
      t <<= (31 - i) ;                         // shift to insertion point (big endian style)
      result |= t ;                            // inject into accumulator
    }
    ones += popcnt_32(result) ;                // count bits set to 1
    all1 += (~result == 0) ? 1 : 0 ;
    zero += (result == 0) ? 1 : 0 ;
    src += 32 ;                                // next slice from source array
    *bits = result ;                           // store 32 bits into bitmap
    bits++ ;                                   // bump bitmap pointer
  }
  result = 0 ;
  n31 = n & 0x1F ;                             // n modulo 32 (number of leftovers)
  for(i = 0 ; i < n31 ; i++){                  // last, shorter slice (0 -> 31 elements)
    t   = ((src[i] & mmask) == special) ? 1 : 0 ;   // 1 if special value, 0 if not
    t <<= (31 - i) ;                           // shift to insertion point (big endian style)
    result |= t ;
  }
  if(n31 > 0){                                 // only store if there were leftovers
    ones += popcnt_32(result) ;                // count bits set to 1
    *bits = result ;
  }
  bitmap->ones = ones ;                        // number of bits set to 1
  bitmap->all1 = all1 ;                        // number of words set to 0xFFFFFFFF
  bitmap->zero = zero ;                        // number of words set to 0
  return bitmap ;
}

// build a 1 bit per element bitmap (big endian style) (from signed or unsigned 32 bit integer array)
// 1 if (value & ~mask)  < (special & ~mmask)   (oper < 0)
//                    or
// 1 if (value & ~mask)  > (special & ~mmask)   (oper > 0 )
// array   [IN] : source array (32 bit elements) (assumed to be signed integers)
// bmp    [OUT] : pointer to rmn_bitmap struct (if NULL, a new rmn_bitmap struct will be allocated)
// special [IN] : "special" value
// mmask   [IN] : mask applied to "special" value (only used for equality comparisons)
// n       [IN] : number of elements in array
// oper    [IN] : comparison type ( 0 : == , -1 : < signed, 1 : > signed, -2 < unsigned, 2 : > unsigned)
// return value : pointer to rmn_bitmap struct if successful, NULL if there was an error
// where bits in mmask are 1, the corresponding bits in array are ignored
// e.g. mmask = 7 will ignore the lower (least significant) 3 bits
rmn_bitmap *bitmap_be_int_01(void *array, rmn_bitmap *bmp, int32_t special, int32_t mmask, int n, int oper){
  uint32_t t, t_gt, t_lt, t_eq, result_lt, result_gt, result_eq, result, n31 ;
  rmn_bitmap *bitmap = bmp ;
  uint32_t *data, ones = 0, all1 = 0, zero = 0 ;
  int32_t i, i0, *src = (int32_t *) array, special0, msb = 0 ;

  mmask = (~mmask) ;
  special0 = special & mmask ;   // mmask is only used for == comparisons (oper == 0)
  if(oper == OP_UNSIGNED_LT || oper == OP_UNSIGNED_GT) {
    msb = (1 << 31) ;   // the MSB will have to be complemented (^msb) for unsigned comparisons
    special ^= msb ;    // using signed comparison operators
  }
  // allocate bitmap if bmp is NULL
  if(bmp == NULL) {
    bitmap = bitmap_create(n) ;                  // create an empty bitmap with the appropriate size
  }else{
    if(n > bitmap->size * 32) return NULL ;      // not enough room in bitmap data array data
  }
  data = (uint32_t *) bitmap->data ;             // bitmap data array
  bitmap->elem = n ;                             // array data will contain n useful bits
  bitmap->nrle = 0 ;                             // not RLE encoded

  for(i0 = 0 ; i0 < n-31 ; i0 += 32){            // full slices of 32 elements
    result = 0 ;
    if(oper < 0){
      for(i=0 ; i<32 ; i++){                     // encode a 32 element slice
        t   = ((src[i] ^ msb)  < special) ? 1 : 0 ;     // 1 if < special value, 0 if not
        t <<= (31 - i) ;                         // shift to insertion point (big endian style)
        result |= t ;                            // inject into accumulator
      }
    }else if(oper == 0){
      for(i=0 ; i<32 ; i++){                     // encode a 32 element slice
        t   = ((src[i] & mmask) == special0) ? 1 : 0 ;  // 1 if == special0, 0 if not (mmask is used)
        t <<= (31 - i) ;                         // shift to insertion point (big endian style)
        result |= t ;                            // inject into accumulator
      }
    }else{
      for(i=0 ; i<32 ; i++){                     // encode a 32 element slice
        t   = ((src[i] ^ msb)  > special) ? 1 : 0 ;     // 1 if > special value, 0 if not
        t <<= (31 - i) ;                         // shift to insertion point (big endian style)
        result |= t ;                            // inject into accumulator
      }
    }
    ones += popcnt_32(result) ;
    all1 += (~result == 0) ? 1 : 0 ;
    zero += (result == 0) ? 1 : 0 ;
    src += 32 ;                                  // next slice from source array
    *data = result ;                             // store 32 bits into bitmap
    data++ ;                                     // bump bitmap pointer
  }
  result_gt = result_lt= result_eq = 0 ;
  n31 = n & 0x1F ;                               // n modulo 32 (number of leftovers)
  for(i = 0 ; i < n31 ; i++){                    // last, shorter slice (0 -> 31 elements)
    t_gt   = ((src[i] ^ msb)  > special) ? 1 : 0 ;   // 1 if > special value, 0 if not
    t_lt   = ((src[i] ^ msb)  < special) ? 1 : 0 ;   // 1 if < special value, 0 if not
    t_eq   = ((src[i] & mmask) == special) ? 1 : 0 ;   // 1 if < special value, 0 if not
    t_gt <<= (31 - i) ;                          // shift to insertion point (big endian style)
    t_lt <<= (31 - i) ;                          // shift to insertion point (big endian style)
    t_eq <<= (31 - i) ;                          // shift to insertion point (big endian style)
    result_gt |= t_gt ;                          // inject into accumulator
    result_lt |= t_lt ;                          // inject into accumulator
    result_eq |= t_eq ;                          // inject into accumulator
  }
  if(n31 > 0){                                   // only store if there were leftovers
    result = (oper < 0) ? result_lt : result_gt ;// < or >
    result = (oper == 0) ? result_eq : result ;  // ==
    ones += popcnt_32(result) ;
    *data = result ;
  }
  bitmap->ones = ones ;                          // number of bits set to 1
  bitmap->all1 = all1 ;                          // number of words set to 0xFFFFFFFF
  bitmap->zero = zero ;                          // number of words set to 0
  return bitmap ;
}

// build a 1 bit per element bitmap (big endian style) (from 32 bit float array)
// 1 if (value & ~mask)  < (special & ~mmask)   (oper < 0)
//                    or
// 1 if (value & ~mask)  > (special & ~mmask)   (oper > 0 )
// array   [IN] : source array (32 bit floats)
// bmp    [OUT] : pointer to rmn_bitmap struct (if NULL, a new rmn_bitmap struct will be allocated)
// special [IN] : "special" value
// mmask   [IN] : mask applied to "special" value (only used for equality comparisons)
// n       [IN] : number of elements in array
// oper    [IN] : comparison type ( 0 : == , -1 : < signed, 1 : > signed, -2 < unsigned, 2 : > unsigned)
// return value : pointer to rmn_bitmap struct if successful, NULL if there was an error
// where bits in mmask are 1, the corresponding bits in array are ignored
// e.g. mmask = 7 will ignore the lower (least significant) 3 bits
rmn_bitmap *bitmap_be_fp_01(float *array, rmn_bitmap *bmp, float special, int32_t mmask, int n, int oper){
  rmn_bitmap *bitmap = bmp ;
  uint32_t t, t_gt, t_lt, result_lt, result_gt, result_eq, result, n31, *data, ones = 0, all1 = 0, zero = 0 ;
  int32_t i, i0 ;
  float *src = (float *) array ;

  mmask = (~mmask) ;
  // allocate bitmap if bmp is NULL
  if(bmp == NULL) {
    bitmap = bitmap_create(n) ;                  // create an empty bitmap with the appropriate size
  }else{
    if(n > bitmap->size * 32) return NULL ;      // not enough room in bitmap data array data
  }
  data = (uint32_t *) bitmap->data ;             // bitmap data array
  bitmap->elem = n ;                             // array data will contain n useful bits
  bitmap->nrle = 0 ;                             // not RLE encoded

  for(i0 = 0 ; i0 < n-31 ; i0 += 32){            // full slices of 32 elements
    if(oper < 0){
      result = 0 ;
      for(i=0 ; i<32 ; i++){                     // encode a 32 element slice
        t   = ((src[i])  < special) ? 1 : 0 ;    // 1 if < special value, 0 if not
        t <<= (31 - i) ;                         // shift to insertion point (big endian style)
        result |= t ;                            // inject into accumulator
      }
    }else{
      result = 0 ;
      for(i=0 ; i<32 ; i++){                     // encode a 32 element slice
        t   = ((src[i])  > special) ? 1 : 0 ;    // 1 if > special value, 0 if not
        t <<= (31 - i) ;                         // shift to insertion point (big endian style)
        result |= t ;                            // inject into accumulator
      }
    }
    ones += popcnt_32(result) ;
    all1 += (~result == 0) ? 1 : 0 ;
    zero += (result == 0) ? 1 : 0 ;
    src += 32 ;                                  // next slice from source array
    *data = result ;                             // store 32 bits into bitmap
    data++ ;                                     // bump bitmap pointer
  }
  result_gt = result_lt = 0 ;
  n31 = n & 0x1F ;                               // n modulo 32 (number of leftovers)
  for(i = 0 ; i < n31 ; i++){                    // last, shorter slice (0 -> 31 elements)
    t_gt   = ((src[i])  > special) ? 1 : 0 ;     // 1 if > special value, 0 if not
    t_lt   = ((src[i])  < special) ? 1 : 0 ;     // 1 if < special value, 0 if not
    t_gt <<= (31 - i) ;                          // shift to insertion point (big endian style)
    t_lt <<= (31 - i) ;                          // shift to insertion point (big endian style)
    result_gt |= t_gt ;                          // inject into accumulator
    result_lt |= t_lt ;                          // inject into accumulator
  }
  if(n31 > 0){                                   // only store if there were leftovers
    result = (oper < 0) ? result_lt : result_gt ;// < or >
    ones += popcnt_32(result) ;
    *data = result ;
  }
  bitmap->ones = ones ;                          // number of bits set to 1
  bitmap->all1 = all1 ;                          // number of words set to 0xFFFFFFFF
  bitmap->zero = zero ;                          // number of words set to 0
  return bitmap ;
}

#if defined(__x86_64__) && defined(__AVX2__)
#include <immintrin.h>
#endif
// restore array from a 1 bit per element bitmap (big endian style)
// array  [OUT] : destination array (32 bit elements)
// bmp     [IN] : pointer to rmn_bitmap struct
// plug    [IN] : value plugged into array where is a 1 in the bitmap
// n       [IN] : size of array
// return value : number of values restored from bit map, negative value if there was an error
// NOTE : some compilers generate suboptimal code with the plain C version, a faster AVX2 version is supplied
int bitmap_restore_be_01(void *array, rmn_bitmap *bmp, uint32_t plug, int n){
  int32_t i, i0 ;
  uint32_t *bitmap, token, *dst = (uint32_t *)array, n31 = n & 0x1F ;
#if defined(__x86_64__) && defined(__AVX2__)
  uint32_t masks[8] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 } ;
  __m256i vzr, vto, vl0, vl1, vl2, vl3, vm0, vm1, vm2, vm3, vb0, vb1, vb2, vb3, vsp ;
  vm3 = _mm256_loadu_si256((__m256i *)masks) ;
  vm2 = _mm256_slli_epi32(vm3,  8) ;
  vm1 = _mm256_slli_epi32(vm3, 16) ;
  vm0 = _mm256_slli_epi32(vm3, 24) ;
  vzr = _mm256_xor_si256(vm3, vm3) ;
  vsp = (__m256i) _mm256_broadcast_ss((float const *) &plug) ;
#endif

  if(array == NULL || bmp == NULL) return -1 ; // bad addresses
  if(n < bmp->elem) return -2 ;                // array is too small
  if(bmp->nrle > 0) {                          // RLE encoded bitmap, must decode in-place first
    bmp = bitmap_decode_be_01(bmp, bmp) ;
  }

  bitmap = (uint32_t *)bmp->data ;
  for(i0 = 0 ; i0 < n-31 ; i0 += 32){          // slices of 32 elements
    token = *bitmap ;
    if(token != 0){                            // check that the whole slice is not 0
#if defined(__x86_64__) && defined(__AVX2__)
      // fast but idiot simple SIMD (AVX2) version
      vto = (__m256i) _mm256_broadcast_ss((float const *) bitmap) ;
      vl0 = _mm256_loadu_si256((__m256i *)(dst+ 0)) ;    // load 32 elements from source data
      vl1 = _mm256_loadu_si256((__m256i *)(dst+ 8)) ;
      vl2 = _mm256_loadu_si256((__m256i *)(dst+16)) ;
      vl3 = _mm256_loadu_si256((__m256i *)(dst+24)) ;
      vb0 = _mm256_and_si256(vto, vm0) ;                 // token & bit mask
      vb1 = _mm256_and_si256(vto, vm1) ;
      vb2 = _mm256_and_si256(vto, vm2) ;
      vb3 = _mm256_and_si256(vto, vm3) ;
      vb0 = _mm256_cmpeq_epi32(vzr, vb0) ;               // compare to 0
      vb1 = _mm256_cmpeq_epi32(vzr, vb1) ;               // - will have all 0s where source data must be "plugged"
      vb2 = _mm256_cmpeq_epi32(vzr, vb2) ;               // - will have all 1s where source data must be kept
      vb3 = _mm256_cmpeq_epi32(vzr, vb3) ;
      vl0 = _mm256_blendv_epi8(vsp, vl0, vb0) ;          // special value where (token & bit mask) non zero
      vl1 = _mm256_blendv_epi8(vsp, vl1, vb1) ;          // original value where (token & bit mask) is zero
      vl2 = _mm256_blendv_epi8(vsp, vl2, vb2) ;
      vl3 = _mm256_blendv_epi8(vsp, vl3, vb3) ;
      _mm256_storeu_si256((__m256i *)(dst+ 0), vl0) ;    // store possibly modified data
      _mm256_storeu_si256((__m256i *)(dst+ 8), vl1) ;
      _mm256_storeu_si256((__m256i *)(dst+16), vl2) ;
      _mm256_storeu_si256((__m256i *)(dst+24), vl3) ;
#else
      for(i=0 ; i<32 ; i++){                     // conditionally plug a 32 element slice
        dst[i] = (token & (1 << (31-i))) ? plug : dst[i] ;
      }
#endif
    }
    bitmap++ ;
    dst += 32 ;
  }
  token = *bitmap ;
  for(i=0 ; i<n31 ; i++){                      // last, shorter slice (if n not a multiple of 32)
    dst[i] = (token & (1 << (31-i))) ? plug : dst[i] ;
  }
  return n ;
}
//
// 1s are assumed to be occurring much less frequently than 0s (reletively long sequences of 0s)
// full encoding for a stream of 0s (starts with 0, ends before third 1)
// 0 -+---------+- 1 -+---------+- 1 -+---------+- start of stream of 1s
//    ^         |     ^         |     ^         |
//    +--- 0 ---+     +--- 0 ---+     +--- 0 ---+  (0, set of 0s, 1, set of 0s, 1, set of 0s)
// 1      NG       0       3       0       1       (weight, 12 or 8)
//
// simple encoding for a stream of 1s ( starts with 1, ends before next 0)
// 1 -+---------+- start of stream of 0s
//    ^         |
//    +--- 1 ---+  (single 1, optional single 1s)
// 1       1       (weight)
//
// simple encoding for a stream of 0s ( starts with 0, ends before next 1)
// 0 -+---------+- start of stream of 0s
//    ^         |
//    +--- 0 ---+  (single 0, optional single 0s)
// 1       1       (weight)
//
// 01 1111        : 1 x 0, 4 x 1
// 0100 1111      : 3 x 0, 4 x 1
// 000100 1111    : 1+16+2 x 0 , 4 x 1
// 1 01 111       : 1 x 1, 1 x 0, 3 x 1
// 1 010 1111     : 1 x 1, 2 x 0, 4 x 1
//
// if 1s and 0s are occurring with similar probability in relatively long sequences
// full encoding for a stream of 1s (starts with 1, ends before third 0)
// 1 -+---------+- 0 -+---------+- 0 -+---------+- start of stream of 1s
//    ^         |     ^         |     ^         |
//    +--- 1 ---+     +--- 1 ---+     +--- 1 ---+  (single 1, set of 1s, 0, set of 1s, 0, set of 1s)
// 1      NG       0       3       0       1       (weight, 12 or 8)
//
// TODO : make sure that we are not overflowing the storage if the encoded stream
//        becomes longer than the original stream (EMIT macros)
// TODO :
//        maybe add 2 bits at the beginning to tell whether full or simple encoding is used
//        xy  00 would mean that no encoding is used, default would be 10 (full 0, simple 1)
//        x : 1 if full encoding is used for 0s, 0 if simple encoding is used
//        y : 1 if full encoding is used for 01s, 0 if simple encoding is used
//
// bmp         [IN] : pointer to bitmap to analyze
int bitmap_encode_hint_01(rmn_bitmap *bmp){
  int totavail, nb, ntot, bit_type, left, count[2], lseq[2], mode ;
  uint32_t *bitmap, scan0, scan1 ;

  if(bmp == NULL) return -1 ;
  totavail = bmp->elem ;                             // number of elements to encode
  totavail >>= 1 ;                                   // only analyze the first half of the bitmap for hints
  bitmap = (uint32_t *)bmp->data ;                   // bitmap to encode
  scan0 = *bitmap ; bitmap++ ;                       // accumulator to scan for 0s
  scan1 = ~scan0 ;                                   // accumulator to scan for 1s
  left = 32 ;
  ntot = 0 ;
  bit_type = 0 ;                                     // start by counting 0s
  count[0] = count[1] = lseq[0] = lseq[1] = 0 ;      // set counters to 0

count_1_or_0 :
  nb = lzcnt_32( (bit_type ? scan1 : scan0) ) ;      // get number of leading zeroes
  ntot     += nb ;
  left     -= nb ;
  totavail -= nb ;

  if(left == 0){                                     // nothing left in scan0/scan1
    if(totavail <= 0) goto done ;                    // nothing left to encode in bitmap
    scan0 = *bitmap ; bitmap++ ;                     // get next 32 bits from bitmap
    scan1 = ~scan0 ;                                 // scan1 is complement of scan0
    left = 32 ;                                      // 32 bits available in scan0/scan1
    goto count_1_or_0 ;                              // keep counting for this bit type (0 / 1)
  }else{                                             // there are bits left in scan0/scan1
    if(nb > 0){                                      // do not shift if count is 0
      scan0 = ~((~scan0) << nb) ;                    // shift left, introducing 1s from the right
      scan1 = ~((~scan1) << nb) ;                    // shift left, introducing 1s from the right
    }
    if(ntot > 0){                                    // (ntot can be 0)
      count[bit_type]++ ;                            // add one to the sequence count
      lseq[bit_type] += ntot ;                       // we have a sequence of ntot bits of type bit_type
    }
    bit_type = 1 - bit_type ;                        // invert bit type to scan
    ntot = 0 ;                                       // reset counter
    if(totavail > 0) goto count_1_or_0 ;             // start counting potential new sequence
  }
done:
  if(ntot > 0) {                                     // (ntot may be 0)
    count[bit_type]++ ;                              // add one to the sequence count
    lseq[bit_type] += ntot ;                         // we have a sequence of ntot bits of type bit_type
  }

  if(count[0] > 0) lseq[0] /= count[0] ;             // average length for sequences of 0s (assume at least 1 sequence)
  if(count[1] > 0) lseq[1] /= count[1] ;             // average length for sequences of 1s (assume at least 1 sequence)
  mode = 0 ;                                         // simple encoding for both 1s and 0s
  if(lseq[0] >  4) mode |= RLE_FULL_0 ;              // full encoding for 0s
  if(lseq[1] >  4) mode |= RLE_FULL_1 ;              // full encoding for 1s
  if(lseq[0] > 48) mode |= RLE_123_0 ;               // 12/3 encoding rather thatn 8/3 for 0s
  if(lseq[1] > 48) mode |= RLE_123_1 ;               // 12/3 encoding rather thatn 8/3 for 1s
  return mode ;                                      // return full vs simple, 12/3 vs 8/3 encoding hint
}

// EMIT macros are unsafe, they should check that str does not overflow str_limit
//
// #define EMIT1_1 { fprintf(stderr,"1") ; kount++ ; accum |= (1 << shift) ; shift-- ; if(shift<0){ *str = accum ; str++ ; accum = 0 ; shift = 31 ; } }
// #define EMIT1_0 { fprintf(stderr,"0") ; kount++ ;                         shift-- ; if(shift<0){ *str = accum ; str++ ; accum = 0 ; shift = 31 ; } }
#define EMIT1   { kount++ ; shift-- ; if(shift<0){ *str = accum ; str++ ; accum = 0 ; shift = 31 ; } }
#define EMIT1_1 { accum |= (1 << shift) ; EMIT1 }
#define EMIT1_0 {                       ; EMIT1 }
// #define NG 12

// bmp         [IN] : pointer to bitmap to encode
// rle_stream [OUT] : pointer to encoded bitmap (if NULL, it will be allocated internally)
// mode        [IN] : see bitmap_encode_hint_01 above.(-1 means default)
//                    use RLE_FULL_0, RLE_FULL_1, RLE_123_0, RLE_123_1 macros
// return pointer to encoded stream, NULL in case of error
// for now only full encoding for 0s and simple encoding for 1s is implemented
//
rmn_bitmap *bitmap_encode_be_01(rmn_bitmap *bmp, rmn_bitmap *rle_stream, int mode){
  int totavail, nb, ntot, bit_type, left, last_type ;
  uint32_t *bitmap, scan0, scan1 ;
  rmn_bitmap *stream = rle_stream ;
  int kount = 0 ;
  uint32_t accum, *str, *str_limit ;
  int shift, inplace ;
  int full_0, full_1, ng0, ng1 ;

  if(mode == -1) mode = RLE_FULL_0 | RLE_123_0 ;  // default : simple encoding for 1s, full 12/3 encoding for 0s
  if(mode & RLE_FULL_0){
    int full_12_0, full_8_0 ;
    full_0 = 1 ;
    full_12_0 = (mode & RLE_123_0) != 0 ;
    full_8_0  = (mode & RLE_123_0) == 0 ;
    ng0 = full_12_0 ? 12 : 8 ;
  }else{
    full_0 = ng0 = 0 ;
  }
  if(mode & RLE_FULL_1){
    int full_12_1, full_8_1 ;
    full_1 = 1 ;
    full_12_1 = (mode & RLE_123_1) != 0 ;
    full_8_1  = (mode & RLE_123_1) == 0 ;
    ng1 = full_12_1 ? 12 : 8 ;
  }else{
    full_1 = ng1 = 0 ;
  }
// fprintf(stderr, "ng0 = %d, ng1 = %d\n", ng0, ng1) ;
  if(bmp == NULL) return NULL ;
  inplace = (void *)bmp == (void *)rle_stream ;
  if(inplace) stream = NULL ;                        // inplace encoding
  if(stream == NULL)                                 // need to allocate an encoded stream
    stream = (rmn_bitmap *) bitmap_create(bmp->elem) ;
  if(stream == NULL) return NULL ;                   // error : allocation failed

  totavail = bmp->elem ;                             // number of elements to encode
  bitmap = (uint32_t *)bmp->data ;                   // bitmap to encode
  scan0 = *bitmap ; bitmap++ ;                       // accumulator to scan for 0s
  scan1 = ~scan0 ;                                   // accumulator to scan for 1s
  left = 32 ;
  ntot = 0 ;
  bit_type = 0 ;                                     // start by counting 0s
//   accum = 0 ;
//   shift = 31 ;
  accum = mode << 28 ;
  shift = 27 ;
  str = (uint32_t *)stream->data ;
  str_limit = ((uint32_t *)stream->data) + stream->size ;

count_1_or_0 :
  nb = lzcnt_32( (bit_type ? scan1 : scan0) ) ;      // get number of leading zeroes
  ntot     += nb ;
  left     -= nb ;
  totavail -= nb ;

  if(left == 0){                                     // nothing left in scan0/scan1
    if(totavail <= 0) goto done ;                    // nothing left to encode in bitmap
    scan0 = *bitmap ; bitmap++ ;                     // get next 32 bits from bitmap
    scan1 = ~scan0 ;
    left = 32 ;                                      // 32 bits in scan0/scan1
    goto count_1_or_0 ;                              // keep counting for this bit type (0 / 1)
  }else{
    if(nb > 0){                                      // do not shift if count is 0
      scan0 = ~((~scan0) << nb) ;                    // shift left, introducing 1s from the right
      scan1 = ~((~scan1) << nb) ;                    // shift left, introducing 1s from the right
    }
    // we have ntot bits of type bit_type (ntot may be 0)
    if(ntot > 0){
      last_type = bit_type ;
      if(bit_type == 0){                             // encode 0s (full encoding)
        EMIT1_0 ; ntot-- ;                           // lead 0
        if(full_0){
          while(ntot >= ng0) { ntot -= ng0 ; EMIT1_0 ; } // groups of NG 0s
          EMIT1_1 ;                                    // separator
          while(ntot >= 3) { ntot -= 3 ; EMIT1_0 ; }   // groups of 3 0s
          EMIT1_1 ;                                    // separator
        }
        while(ntot > 0) { ntot-- ; EMIT1_0 ; }       // single 0s
      }else{                                         // encode 1s (simple encoding)
        EMIT1_1 ; ntot-- ;
        if(full_1){
        }
        while(ntot > 0) { ntot-- ; EMIT1_1 ; }       // single 1s
      }
    }
    bit_type = 1 - bit_type ;                        // invert bit type to scan
    ntot = 0 ;                                       // reset bit counter
    if(totavail > 0) goto count_1_or_0 ;             // start counting for new sequence
  }
done:
  if(ntot > 0) {
    last_type = bit_type ;
    if(bit_type == 0){                               // encode 0s (full encoding)
      EMIT1_0 ; ntot-- ;                             // lead 0
      if(full_0){
        while(ntot >= ng0) { ntot -=ng0 ; EMIT1_0 ; }    // groups of NG 0s
        EMIT1_1 ;                                      // separator
        while(ntot >= 3) { ntot -=3 ; EMIT1_0 ; }      // groups of 3 0s
        EMIT1_1 ;                                      // separator
      }
      while(ntot > 0) { ntot-- ; EMIT1_0 ; }         // single 0s
    }else{                                           // encode 1s (simple encoding)
      EMIT1_1 ; ntot-- ;
      if(full_1){
      }
      while(ntot > 0) { ntot-- ; EMIT1_1 ; }         // single 1s
    }
  }
  // add a guard bit
  if(last_type == 0){
    EMIT1_1 ;
  }else{
    EMIT1_0 ;
  }
  if(shift < 31) *str = accum ; str++ ;            // flush accumulator leftovers
  if( (void *)bmp == (void *)rle_stream){          // copy RLE stream back into original bitmap
    int i ;
    for(i = 0 ; i < ((kount+31)/32) ; i++) 
      bmp->data[i] = stream->data[i] ;
    free(stream) ;
    stream = (rmn_bitmap *)bmp ;
  }else{
    stream->elem = bmp->elem ;                     // number of bits before encoding
  }
  stream->nrle = kount - 1 ;  // do not include the guard bit in bit count

  return stream ;

}

#define INSERT1          { ninserted++ ; insert-- ; if(insert < 0) { *bitmap = accum ; bitmap++ ; accum = 0 ; insert = 31 ; } }
#define INSERT1_1        { accum |= (1 << insert) ; INSERT1 }
#define INSERT1_0        { INSERT1 }
#define INSERTN_0(NG)    {  ninserted+=NG ; insert -= NG ; if(insert < 0) { *bitmap = accum ; bitmap++ ; accum = 0 ; insert += 32 ;} ; }
#define INSERT3_0        {  ninserted+=3 ; insert -= 3 ; if(insert < 0) { *bitmap = accum ; bitmap++ ; accum = 0 ; insert += 32 ;} ; }
#define INSERTN_1(NG)    { int n ; for(n=0 ; n<NG; n++) {INSERT1_1} ; }
#define INSERT3_1        { int n ; for(n=0 ; n<3 ; n++) {INSERT1_1} ; }
#define GET_1_RLE(X)   { ndecoded++ ; if(avail == 0) { encoded = *rle ; rle++ ; avail = 32 ; } ; X = (encoded >> 31) ; encoded <<= 1 ; avail--  ; totavail-- ; }
// for now only full encoding for 0s and simple encoding for 1s is implemented in the decoder
rmn_bitmap *bitmap_decode_be_01(rmn_bitmap *bmp, rmn_bitmap *rle_stream){
  uint32_t *bitmap, *rle, encoded, accum, bit ;
  uint32_t msb1 = 0x80000000u ;        // 1 in Most Significant Bit
  int totavail, avail, insert, ndecoded, ninserted, inplace = 0, i ;
  int full_0, full_1, ng0, ng1, full_12_0, full_12_1 ;

  if(rle_stream == NULL) return NULL ;   // no encoded stream to decode

  if(bmp == NULL) {                      // auto allocate bmp
    bmp = (rmn_bitmap *)bitmap_create(rle_stream->elem) ;
// fprintf(stderr, "decode_be_01 : creating bitmap for %d elements\n", rle_stream->elem) ;
  }
  if((void *)bmp == (void *)rle_stream){ // in-place decoding
// fprintf(stderr, "decode_be_01 : in-place decoding\n");
    rle_stream = (rmn_bitmap *) bitmap_create(bmp->elem) ;  // new RLE stream
    inplace = 1 ;                                               // flag it to be freed at exit
    rle_stream->nrle = bmp->nrle ;
    rle_stream->elem = bmp->elem ;
    for(i = 0 ; i < ((bmp->elem+1+31)/32) ; i++) rle_stream->data[i] = bmp->data[i] ;
    bmp->nrle = 0 ;                                             // mark bitmap as non encoded
  }

  bitmap = (uint32_t *)bmp->data ;       // insertion point in bitmap
  accum = 0 ;                            // insertion accumulator for bitmap
  insert = 31 ;                          // insertion point in bitmap
  rle = (uint32_t *)rle_stream->data ;
// fprintf(stderr, "%d bits to decode from %d RLE encoded bits\n", rle_stream->elem, rle_stream->nrle) ;
  totavail = 0 ;
  avail = 0 ;
  ndecoded = 0 ;
  ninserted = 0 ;

  // get 4 bit header that describes encoding
  GET_1_RLE(full_12_0) ; ng0 = full_12_0 ? 12 : 8 ;     // full 12/3 or 8/3 encoding for 0s
  GET_1_RLE(full_12_1) ; ng1 = full_12_1 ? 12 : 8 ;     // full 12/3 or 8/3 encoding for 1s
  GET_1_RLE(full_0)    ; ng0 = full_0 ? ng0 : 0 ;       // full or simple encoding for 0s
  GET_1_RLE(full_1)    ; ng1 = full_1 ? ng1 : 0 ;       // full or simple encoding for 1s
  ndecoded = 0 ;
  totavail = rle_stream->nrle ;          // total number of bits to decode

  GET_1_RLE(bit) ;                       // get first RLE encoded bit
  if(bit == 1) goto decode1 ;

decode0:
  INSERT1_0 ;                            // insert first 0
  GET_1_RLE(bit) ;
  if(full_0){                            // full encoding using ng0/3
    while(bit == 0) { INSERTN_0(ng0) ; GET_1_RLE(bit) ; }  // separator == 1 after groups of NG loop
    GET_1_RLE(bit) ;
    while(bit == 0) { INSERT3_0 ; GET_1_RLE(bit) ; }       // separator == 1 after groups of 3 loop
    GET_1_RLE(bit) ;
  }
  while(bit == 0) { INSERT1_0 ; GET_1_RLE(bit) ; }         // bit == 1 after singles loop
  if(totavail <= 0) goto end ;

decode1:
  INSERT1_1 ;                            // insert first 1
  GET_1_RLE(bit) ;
  if(full_1){
    while(bit == 1) { INSERTN_1(ng1) ; GET_1_RLE(bit) ; }  // separator == 0 after groups of NG loop
    GET_1_RLE(bit) ;
    while(bit == 1) { INSERT3_1 ; GET_1_RLE(bit) ; }       // separator == 1 after groups of 3 loop
    GET_1_RLE(bit) ;
  }
  while(bit == 1) { INSERT1_1 ; GET_1_RLE(bit) ; }         // bit == 0 after singles loop
  if(totavail > 0) goto decode0 ;

end:
  if(insert < 31) *bitmap = accum ;   // insert last token
  if(inplace) {
    free(rle_stream) ;      // free internally allocated rle stream
// fprintf(stderr, "RLE stream freed\n");
  }
// fprintf(stderr, "%d RLE bits decoded, %d bits inserted\n", ndecoded-1, ninserted) ;
  return bmp ;
}
