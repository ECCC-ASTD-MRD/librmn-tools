// Hopefully useful functions for C and FORTRAN
// Copyright (C) 2021  Recherche en Prevision Numerique
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

// O3 useful for gcc to get faster code
#if defined(__GNUC__) && ! defined(__INTEL_COMPILER_UPDATE) &&  ! defined(__INTEL_LLVM_COMPILER) && ! defined(__clang__) && ! defined(__PGI)
#pragma GCC optimize "O3"
#endif

// build a 1 bit per element bitmap (big endian style)
// array   [IN] : source array (32 bit elements)
// bmp    [OUT] : pointer to rmn_bitmap struct
// special [IN] : "special" value
// mmask   [IN] : mask applied to "special" value
// n       [IN] : number of elements in array
// return value : pointer to rmn_bitmap struct if successful, NULL if ther was an error
rmn_bitmap *bitmask_be_01(void *array, rmn_bitmap *bmp, uint32_t special, uint32_t mmask, int n){
  uint32_t i, i0, t, result, n31 = n & 0x1F ;
  rmn_bitmap *bitmap = bmp ;
  uint32_t *bits ;
  uint32_t *src = (uint32_t *) array ;
  size_t bmp_size ; // needed size for bitmap

  mmask = (~mmask) ;
  // allocate bitmap if bmp was NULL
  if(bmp == NULL) {
    bmp_size = ((n + 31)/32)*sizeof(uint32_t)+sizeof(rmn_bitmap) ;
    bitmap = (rmn_bitmap *) malloc(bmp_size) ;
    bitmap->size = (n + 31)/32 ;               // number of 32 bit elements in bitmap array
  }else{
    if(n > bitmap->size * 32) return NULL ;    // not enough room in array bm
  }
  bits = (uint32_t *) bitmap->bm ;
  bitmap->elem = n ;                           // array bm will contain n useful bits
  bitmap->rle = 0 ;                            // not RLE encoded

  for(i0 = 0 ; i0 < n-31 ; i0 += 32){          // slices of 32 elements
    result = 0 ;
    for(i=0 ; i<32 ; i++){                     // encode a 32 element slice
      t   = ((src[i] & mmask) == special) ? 1 : 0 ;   // 1 if special value, 0 if not
      t <<= (31 - i) ;                         // shift to insertion point (big endian style)
      result |= t ;                            // inject into accumulator
    }
    src += 32 ;                                // next slice from source array
    *bits = result ;                           // store 32 bits into bitmap
    bits++ ;                                   // bump bitnap pointer
  }
  result = 0 ;
  for(i=0 ; i<n31 ; i++){                      // last, shorter slice
    t   = ((src[i] & mmask) == special) ? 1 : 0 ;   // 1 if special value, 0 if not
    t <<= (31 - i) ;                           // shift to insertion point (big endian style)
    result |= t ;
  }
  if(n31 > 0){                                 // do not store if there were no leftovers
    *bits = result ;
  }
  return bitmap ;
}

// restore array from a 1 bit per element bitmap (big endian style)
// array  [OUT] : destination array (32 bit elements)
// bmp     [IN] : pointer to rmn_bitmap struct
// special [IN] : value plugged into array where is a 1 in the bitmap
// n       [IN] : number of elements in array
// return value : number of values scanned in bit map, negative value if there was an error
int bitmask_restore_be_01(void *array, rmn_bitmap *bmp, uint32_t special, int n){
  uint32_t i, i0, n31 = n & 0x1F ;
  int32_t token ;
  int32_t *bitmap ;
  uint32_t *dst = (uint32_t *) array ;

  if(array == NULL || bmp == NULL) return -1 ; // bad addresses
  if(n > bmp->elem) return -2 ;                // not enough data in bitmap
  if(bmp->rle) return -3 ;                     // RLE encoded bitmap, OOPS

  bitmap = bmp->bm ;
  for(i0 = 0 ; i0 < n-31 ; i0 += 32){          // slices of 32 elements
    token = *bitmap ;
    bitmap++ ;
    for(i=0 ; i<32 ; i++){                     // conditionally plug a 32 element slice
      dst[i] = (token < 0) ? special : dst[i] ;
      token <<= 1 ;
    }
    dst += 32 ;
  }
  token = *bitmap ;
  for(i=0 ; i<n31 ; i++){                      // last, shorter slice
    dst[i] = (token < 0) ? special : dst[i] ;
    token <<= 1 ;
  }
  return n ;
}

#define EMIT1_1 { fprintf(stderr,"1") ; kount++ ; accum |= (1 << shift) ; shift-- ; if(shift<0){ *str = accum ; str++ ; accum = 0 ; shift = 31 ; } }
#define EMIT1_0 { fprintf(stderr,"0") ; kount++ ;                         shift-- ; if(shift<0){ *str = accum ; str++ ; accum = 0 ; shift = 31 ; } }
// #define EMIT1 { kount++ ; shift-- ; if(shift<0){ *str = accum ; str++ ; accum = 0 ; shift = 31 ; } }
// #define EMIT1_1 { accum |= (1 << shift) ; EMIT1 }
// #define EMIT1_0 {                       ; EMIT1 }
#define NG 12
// 1s are assumed to be occurring much less frequently than 0s
// encoding for a stream of 0s (starts with 0, ends before second 1)
// 0 -+---------+- 1 -+---------+- start of stream of 1s
//    ^         |     ^         |
//    +--- 0 ---+     +--- 0 ---+  (single 0, optional set of NG 0s, separator, optional single 0s)
// 1      NG       0       1       (count value)
//
// encoding for a stream of 1s ( starts with 1, ends before next 0)
// 1 -+---------+-  start of stream of 0s
//    ^         |
//    +--- 1 ---+   (single 1, optional single 1s)
// 1       1       (count value)
//
// 01 1111        : 1 x 0, 4 x 1
// 0100 1111      : 3 x 0, 4 x 1
// 000100 1111    : 1+16+2 x 0 , 4 x 1
// 1 01 111       : 1 x 1, 1 x 0, 3 x 1
// 1 010 1111     : 1 x 1, 2 x 0, 4 x 1
//
rmn_rle_bitmap *bitmap_encode_be_01(rmn_bitmap *bmp, rmn_rle_bitmap *rle_stream){
  int totavail, nb, ntot, bit_type, left, last_type ;
  uint32_t *bitmap, scan0, scan1 ;
  rmn_rle_bitmap *stream = rle_stream ;
  int kount = 0 ;
  uint32_t accum, *str ;
  int shift ;

  if(bmp == NULL) return NULL ;
  if(stream == NULL) stream = (rmn_rle_bitmap *) malloc(sizeof(rmn_rle_bitmap) + sizeof(uint32_t)*bmp->size) ;
  if(stream == NULL) return NULL ;                   // allocation failed

  totavail = bmp->elem ;
  bitmap = bmp->bm ;
  scan0 = *bitmap ; bitmap++ ;                       // accumulator to scan for 0s
  scan1 = ~scan0 ;                                   // accumulator to scan for 1s
  left = 32 ;
  ntot = 0 ;
  bit_type = 0 ;                                     // start by counting 0s
  accum = 0 ;
  str = stream->rle ;
  shift = 31 ;

count_1_or_0 :
  nb = lzcnt_32( (bit_type ? scan1 : scan0) ) ;      // get number of leading zeroes
  ntot     += nb ;
  left     -= nb ;
  totavail -= nb ;
// fprintf(stderr, "nb=%2d ",nb);
  if(left == 0){                                     // nothing left in scan0/scan1
    if(totavail <= 0) goto done ;                    // nothing left in bitmap
    scan0 = *bitmap ; bitmap++ ;                     // get next 32 bits from bitmap
    scan1 = ~scan0 ;
    left = 32 ;                                      // 32 bits in scan0/scan1
    goto count_1_or_0 ;                              // keep counting for this bit type (0 / 1)
  }else{
    if(nb > 0){                                      // do not shoft if count is 0
      scan0 = ~((~scan0) << nb) ;                    // shift left, introducing 1s from the right
      scan1 = ~((~scan1) << nb) ;                    // shift left, introducing 1s from the right
    }
      // we have ntot bits of type bit_type
// fprintf(stderr, "  %d x %d\n", ntot, bit_type) ;
    if(ntot > 0){
      last_type = bit_type ;
      if(bit_type == 0){                             // encode 0s
        EMIT1_0 ; ntot-- ;                           // lead 0
        while(ntot >=NG) { ntot -= NG ; EMIT1_0 ; }  // groups of 0s
        EMIT1_1 ;                                    // separator
        while(ntot > 0) { ntot-- ; EMIT1_0 ; }       // single 0s
      }else{                                         // encode 1s
        while(ntot > 0) { ntot-- ; EMIT1_1 ; }       // single 1s
      }
    }
    bit_type = 1 - bit_type ;                        // invert bit type to scan
    ntot = 0 ;                                       // reset bit counter
    if(totavail > 0) goto count_1_or_0 ;
  }
done:
  if(ntot > 0) {
//     fprintf(stderr, "> %d x %d\n", ntot, bit_type) ;
    last_type = bit_type ;
    if(bit_type == 0){                             // encode 0s
      EMIT1_0 ; ntot-- ;                           // lead 0
      while(ntot >= NG) { ntot -=NG ; EMIT1_0 ; }  // groups of 8 0s
      EMIT1_1 ;                                    // separator
      while(ntot > 0) { ntot-- ; EMIT1_0 ; }       // single 0s
    }else{                                         // encode 1s
      while(ntot > 0) { ntot-- ; EMIT1_1 ; }       // single 1s
    }
  }
  // add a guard bit
  if(last_type == 0){
    EMIT1_1 ;
  }else{
    EMIT1_0 ;
  }
  if(shift < 31) *str = accum ; str++ ; // flush accumulator leftovers
//   bmp->rle = kount - 1 ;   // true RLE bit count
fprintf(stderr, "\nkount = %d (%d), shift = %d, last_type = %d\n", kount, bmp->elem, shift, last_type);
int i ;
for(i=0 ; i<str-stream->rle ; i++) fprintf(stderr, "%8.8x ", stream->rle[i]) ;
fprintf(stderr, "\n");
  return stream ;
  // copy RLE stream back into bitmap ?
}

rmn_bitmap *bitmap_decode_be_01(rmn_bitmap *bmp, rmn_rle_bitmap *rle_stream){
  if(bmp == NULL || rle_stream == NULL) return NULL ;

  return bmp ;
}
