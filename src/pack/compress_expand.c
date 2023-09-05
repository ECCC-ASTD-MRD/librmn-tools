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
// N.B. the SSE2 versions are 2-3 x faster than the plain C versions (compiler dependent)
//
#include <stdio.h>

#include <rmn/compress_expand.h>
#include <rmn/bits.h>

// SSE2 version of stream_replace_32
#if defined(__x86_64__) && defined(__SSE2__)
static void stream_replace_32_sse(uint32_t *s, uint32_t *d, uint32_t *map, int n){
  int j ;
  uint32_t mask ;

  for(j=0 ; j<n-31 ; j+=32){                            // chunks of 32
    mask = *map++ ;
    s = sse_expand_replace_32(s, d, mask) ; d += 32 ;
  }
  mask = *map++ ;
  expand_replace_n(s, d, mask, n - j) ;             // leftovers (0 -> 31)
}
#endif

// pure C code version
// s   [IN] : store-compressed source array (32 bit items)
// d  [OUT] : load-expanded destination array  (32 bit items)
// map [IN] : bitmap used to load-expand (BIG-ENDIAN style)
// n   [IN] : number of items
// where there is a 0 in the mask, nothing is done to array d
// where there is a 1 in the mask, the next value from s is stored into array d
//       (at the corresponding position in the map)
// values from s replace some values in d
#if ! defined(__SSE2__)
static void stream_replace_32(uint32_t *s, uint32_t *d, uint32_t *map, int n){
  int j ;
  uint32_t mask ;
  for(j=0 ; j<n-31 ; j+=32){                        // chunks of 32
    mask = *map++ ;
    s = expand_replace_32(s, d, mask) ; d += 32 ;
  }
  mask = *map++ ;
  expand_replace_n(s, d, mask, n - j) ;             // leftovers (0 -> 31)
}
#endif

// SSE2 version of stream_expand_32
#if defined(__x86_64__) && defined(__SSE2__)
void stream_expand_32_sse(void *s_, void *d_, void *map_, int n, void *fill_){
  uint32_t *s = (uint32_t *) s_ ;
  uint32_t *d = (uint32_t *) d_ ;
  uint32_t *map = (uint32_t *) map_ ;
  uint32_t *pfill = (uint32_t *) fill_ ;
  int j ;
  uint32_t mask, fill ;

  if(fill_ == NULL){
    stream_replace_32_sse(s, d, map, n) ;
    return ;
  }
  fill = *pfill ;
  for(j=0 ; j<n-31 ; j+=32){                            // chunks of 32
    mask = *map++ ;
    s = sse_expand_fill_32(s, d, mask, fill) ; d += 32 ;
  }
  mask = *map++ ;
  expand_fill_n(s, d, mask, fill, n - j) ;              // leftovers (0 -> 31)
}
#endif

// pure C code version
// s   [IN] : store-compressed source array (32 bit items)
// d  [OUT] : load-expanded destination array  (32 bit items)
// map [IN] : bitmap used to load-expand (BIG-ENDIAN style)
// n   [IN] : number of items
// where there is a 0 in the mask, value "fill" is stored into array d
// where there is a 1 in the mask, the next value from s is stored into array d
//       (at the corresponding position in the map)
// all elements of d receive a new value ("fill" of a value from s)
void stream_expand_32(void *s_, void *d_, void *map_, int n, void *fill_){
#if defined(__x86_64__) && defined(__SSE2__)
  stream_expand_32_sse(s_, d_, map_, n, fill_) ;
#else
  uint32_t *s = (uint32_t *) s_ ;
  uint32_t *d = (uint32_t *) d_ ;
  uint32_t *map = (uint32_t *) map_ ;
  uint32_t *pfill = (uint32_t *) fill_ ;
  int j ;
  uint32_t mask, fill ;
  if(fill_ == NULL){
    stream_replace_32(s, d, map, n) ;
    return ;
  }
  fill = *pfill ;
  for(j=0 ; j<n-31 ; j+=32){                            // chunks of 32
    mask = *map++ ;
    s = expand_fill_32(s, d, mask, fill) ; d += 32 ;
  }
  mask = *map++ ;
  expand_fill_n(s, d, mask, fill, n - j) ;              // leftovers (0 -> 31)
#endif
}

// SSE2 version of stream_compress_32
#if defined(__x86_64__) && defined(__SSE2__)
uint32_t *stream_compress_32_sse(void *s_, void *d_, void *map_, int n){
  uint32_t *s = (uint32_t *) s_ ;
  uint32_t *d = (uint32_t *) d_ ;
  uint32_t *map = (uint32_t *) map_ ;
  int j ;
  uint32_t mask ;

  for(j=0 ; j<n-31 ; j+=32){                            // chunks of 32
    mask = *map++ ;
    d = sse_compress_32(s, d, mask) ; s += 32 ;
  }
  mask = *map ;
  d = c_compress_n(s, d, mask, n - j) ;                 // leftovers (0 -> 31)
  return d ;
}
#endif

// pure C code version
// s   [IN] : source array (32 bit items)
// d  [OUT] : store-compressed destination array  (32 bit items)
// map [IN] : bitmap used to store-compress (BIG-ENDIAN style)
// n   [IN] : number of items
// return : next storage address for array d
// where there is a 0 in the mask, nothing is added into array d
// where there is a 1 in the mask, the corresponding item in array s is appended to array d
// return pointer - original d = number of items stored into array d
uint32_t *stream_compress_32(void *s_, void *d_, void *map_, int n){
#if defined(__x86_64__) && defined(__SSE2__)
  return stream_compress_32_sse(s_, d_, map_, n) ;
#else
  uint32_t *s = (uint32_t *) s_ ;
  uint32_t *d = (uint32_t *) d_ ;
  uint32_t *map = (uint32_t *) map_ ;
  int j ;
  uint32_t mask ;
  for(j=0 ; j<n-31 ; j+=32){                            // chunks of 32
    mask = *map++ ;
    d = c_compress_32(s, d, mask) ; s += 32 ;
  }
  mask = *map ;
  d = c_compress_n(s, d, mask, n - j) ;                 // leftovers (0 -> 31)
  return d ;
#endif
}
