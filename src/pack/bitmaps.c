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
// O3 useful for gcc to get faster code
#if defined(__GNUC__)
#if ! defined(__INTEL_COMPILER_UPDATE)
#if ! defined(__PGI)
#pragma GCC optimize "O3,tree-vectorize"
#endif
#endif
#endif

// build a 1 bit per element bitmap (big endian style)
// array   [IN] : source array (32 bit elements)
// bmp    [OUT] : array that will receive bitmap
// special [IN] : "special" value
// mmask   [IN] : mask applied to "special" value
// n       [IN] : number of elements in array
void *bitmask_be_01(void *array, void *bmp, uint32_t special, uint32_t mmask, int n){
  uint32_t i, i0, t, result, n31 = n & 0x1F ;
  uint32_t *bitmap = (uint32_t *) bmp ;
  uint32_t *src = (uint32_t *) array ;
  size_t bmp_size = (n+7)/8+sizeof(uint32_t) ; // needed size for bitmap
  // allocate bitmap if bmp was NULL
  if(bmp == NULL) bitmap = (uint32_t *) malloc(bmp_size) ;

  for(i0 = 0 ; i0 < n-31 ; i0 += 32){          // slices of 32 elements
    result = 0 ;
    for(i=0 ; i<32 ; i++){                     // encode a 32 element slice
      t   = ((src[i] & mmask) == special) ? 1 : 0 ;
      t <<= (31 - i) ;                         // shift to insertion point (big endian style)
      result |= t ;                            // inject into accumulator
    }
    src += 32 ;                                // next slice from source array
    *bitmap = result ;                         // store 32 bits into bitmap
    bitmap++ ;                                 // bump bitnap pointer
  }
  result = 0 ;
  for(i=0 ; i<n31 ; i++){                      // last, shorter slice
    t   = ((src[i] & mmask) == special) ? 1 : 0 ;
    t <<= (31 - i) ;
    result |= t ;
  }
  if(n31 > 0){
    *bitmap = result ;
  }
  return bitmap ;
}

// restore array from a 1 bit per element bitmap (big endian style)
// array  [OUT] : destination array (32 bit elements)
// bmp     [IN] : array that contains the bitmap
// special [IN] : value plugged into array where is a 1 in the bitmap
// n       [IN] : number of elements in array
void bitmask_restore_be_01(void *array, void *bmp, uint32_t special, int n){
  uint32_t i, i0, n31 = n & 0x1F ;
  int32_t token ;
  int32_t *bitmap = (int32_t *) bmp ;
  uint32_t *dst = (uint32_t *) array ;
  for(i0 = 0 ; i0 < n-31 ; i0 += 32){          // slices of 32 elements
    token = *bitmap ;
    bitmap++ ;
    for(i=0 ; i<32 ; i++){                     // plug a 32 element slice
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
}
