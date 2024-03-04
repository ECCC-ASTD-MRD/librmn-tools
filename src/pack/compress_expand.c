// Hopefully useful functions for C and FORTRAN
// Copyright (C) 2023-2024  Recherche en Prevision Numerique
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
#undef __SSE2__
#undef __AVX512F__
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

// src     [IN] : source array (32 bit items)
// dst    [OUT] : store-compressed destination array  (32 bit items)
// be_mask [IN] : bit array used for store-compress (BIG-ENDIAN style)
// n       [IN] : number of items
// return : next storage address for array dst
// where there is a 0 in the mask, nothing is added into array d
// where there is a 1 in the mask, the corresponding item in array s is appended to array d
// returned pointer - original dst = number of items stored into array dst
// the mask is interpreted Big Endian style, bit 0 is the MSB, bit 31 is the LSB
#if defined(__x86_64__) && defined(__SSE2__)
// SSE2 version of stream_compress_32
uint32_t *CompressStore_sse_be(void *src, void *dst, void *be_mask, int n){
  uint32_t *map = (uint32_t *) be_mask, mask ;
  int i ;

  for(i=0 ; i<n-31 ; i+=32, src += 32){                       // chunks of 32
    mask = *map++ ;
    dst = CompressStore_32_sse_be(src, dst, mask) ;
  }
  mask = *map ;
  dst = CompressStore_0_31_c_be(src, dst, mask, n - i) ;      // leftovers (0 -> 31)
  return dst ;
}
#endif

// pure C code version (non SIMD)
uint32_t *CompressStore_c_be(void *src, void *dst, void *be_mask, int n){
  uint32_t *map = (uint32_t *) be_mask, mask ;
  uint32_t *w32 = (uint32_t *) src ;
  int i ;
  for(i=0 ; i<n-31 ; i+=32, w32 += 32){                       // chunks of 32
    mask = *map++ ;
    dst = CompressStore_32_c_be(w32, dst, mask) ;
  }
  mask = *map ;
  dst = CompressStore_0_31_c_be(w32, dst, mask, n - i) ;      // leftovers (0 -> 31)
  return dst ;
}

// generic version, switches between the optimized versions
uint32_t *CompressStore_be_(void *src, void *dst, void *be_mask, int n){
#if defined(__x86_64__) && defined(__SSE2__)
  return CompressStore_sse_be(src, dst, be_mask, n) ;
#else
  return CompressStore_c_be(src, dst, be_mask, n) ;
#endif
}

// the AVX512 version does not use the compressed store with mask instruction
// the alternative is just plain C code
// src    [IN] : "full" array
// dst [OUT] : non masked elements from "full" array
// le_mask        [IN] : 32 bits mask, if bit[I] is 1, src[I] is stored, if 0, it is skipped
// the function returns the next address to be stored into for the "dst" array
// the mask is interpreted Little Endian style, bit[0] is the LSB, bit[31] is the MSB
// this version processes 32 elements from "src"
// this version uses a store rather than a compressed store for performance reasons
// and can store up to 32 extra (irrelevant) values after the useful end of dst
static void *CompressStore_32_le(void *src, void *dst, uint32_t le_mask){
#if defined(__AVX512F__)
  dst = CompressStore_32_avx512_le(src, dst, le_mask) ;
#elif defined(__SSE2__)
  dst = CompressStore_32_sse_le(src, dst, le_mask) ;
#else
  dst = CompressStore_32_c_le(src, dst, le_mask) ;
#endif
return dst ;
}
static void *CompressStore_32_be(void *src, void *dst, uint32_t le_mask){
#if defined(__SSE2__)
  dst = CompressStore_32_sse_be(src, dst, le_mask) ;
#else
  dst = CompressStore_32_c_be(src, dst, le_mask) ;
#endif
return dst ;
}

// src     [IN] : "source" array
// nw32    [IN] : number of elements in "source" array
// dst    [OUT] : non masked elements from "source" array
// be_mask [IN] : array of masks (Big Endian style)       bit 0 is MSB
// le_mask [IN] : array of masks (Little Endian style)    bit 0 is LSB
//                32 bits elements, if bit[I] is 1, src[I] is stored, if 0, it is skipped
// the function returns the next address to be stored into for the "dst" array
void *CompressStore_be(void *src, void *dst, void *be_map, int nw32){
  int i0 ;
  uint32_t *w32 = (uint32_t *) src ;
  uint32_t *be_mask = (uint32_t *) be_map ;
  for(i0 = 0 ; i0 < nw32 - 31 ; i0 += 32){
    dst = CompressStore_32_be(w32 + i0, dst, *be_mask) ;
    be_mask++ ;
  }
  dst = CompressStore_0_31_c_be(w32 + i0, dst, *be_mask, nw32 - i0) ;
  return dst ;
}
void *CompressStore_le(void *src, void *dst, void *le_map, int nw32){
  int i0 ;
  uint32_t *w32 = (uint32_t *) src ;
  uint32_t *le_mask = (uint32_t *) le_map ;
  for(i0 = 0 ; i0 < nw32 - 31 ; i0 += 32){
    dst = CompressStore_32_le(w32 + i0, dst, *le_mask) ;
    le_mask++ ;
  }
  dst = CompressStore_0_31_c_le(w32 + i0, dst, *le_mask, nw32 - i0) ;
  return dst ;
}
#if 0
// le_map   [OUT] : 1s where src[i] != reference value, 0s otherwise
static int32_t CompressStoreSpecialValue(void *src, void *dst, void *le_map, int nw32, const void *value){
  int32_t nval = 0, i, i0, j ;
  uint32_t *w32 = (uint32_t *) src ;
  uint32_t *vref = (uint32_t *) value ;
  uint32_t *mask = (uint32_t *) le_map ;
  uint32_t *dest = (uint32_t *) dst ;
  uint32_t mask0, bits ;
  uint32_t ref = *vref ;

#if defined(__AVX512F__)
  __m512i vr0 = _mm512_set1_epi32(ref) ;
// fprintf(stderr, "AVX512 code : 2 subslices of 16 values\n");
#elif defined(__AVX2__)
  __m256i vr0 = _mm256_set1_epi32(ref) ;
// fprintf(stderr, "AVX2 code : 4 subslices of 8 values\n");
#else
// fprintf(stderr, "plain C code : 1 slice of 32 values\n");
#endif

  for(i0 = 0 ; i0 < nw32 - 31 ; i0 += 32){   // 32 values per slice
#if defined(__AVX512F__)
//   AVX512 code : 2 subslices of 16 values
  __m512i vd0, vd1 ;
  __mmask16 mk0, mk1 ;
  uint32_t pop0, pop1 ;
  vd0 = _mm512_loadu_epi32(w32 + i0) ;
  vd1 = _mm512_loadu_epi32(w32 + i0 + 16) ;
  mk0 = _mm512_mask_cmp_epu32_mask (0xFFFF, vr0, vd0, _MM_CMPINT_NE) ;
  _mm512_mask_compressstoreu_epi32 (dest, mk0, vd0) ;          // store non special values
  pop0 = _mm_popcnt_u32(mk0) ;
  dest += pop0 ; nval += pop0 ;
  mk1 = _mm512_mask_cmp_epu32_mask (0xFFFF, vr0, vd1, _MM_CMPINT_NE) ;
  _mm512_mask_compressstoreu_epi32 (dest, mk1, vd1) ;          // store non special values
  pop1 = _mm_popcnt_u32(mk1) ;
  dest += pop1 ; nval += pop1 ;
  mask0 = mk1 ;
  mask0 = (mask0 << 16) | mk0 ;
#elif defined(__AVX2__)
//   AVX2 code : 4 subslices of 8 values, probably better to use the plain C code, because of the compressed store
  __m256i vd0, vd1, vd2, vd3, vs0, vc0, vc1, vc2, vc3 ;
  uint32_t pop ;
  vd0 = _mm256_loadu_si256((__m256i *)(w32 + i0     )) ;
  vd1 = _mm256_loadu_si256((__m256i *)(w32 + i0 +  8)) ;
  vd2 = _mm256_loadu_si256((__m256i *)(w32 + i0 + 16)) ;
  vd3 = _mm256_loadu_si256((__m256i *)(w32 + i0 + 24)) ;
  vc3 = _mm256_cmpeq_epi32(vd3, vr0) ; mask0  = _mm256_movemask_ps((__m256) vc3) ; mask0 <<= 8 ;
  vc2 = _mm256_cmpeq_epi32(vd2, vr0) ; mask0 |= _mm256_movemask_ps((__m256) vc2) ; mask0 <<= 8 ;
  vc1 = _mm256_cmpeq_epi32(vd1, vr0) ; mask0 |= _mm256_movemask_ps((__m256) vc1) ; mask0 <<= 8 ;
  vc0 = _mm256_cmpeq_epi32(vd0, vr0) ; mask0 |= _mm256_movemask_ps((__m256) vc0) ; mask0 = (~mask0) ;
  pop = _mm_popcnt_u32(mask0) ;
  nval += pop ;
  bits = mask0 ;
  for(j=0 ; j<32 ; j++, bits >>= 1){   // "compressed store"
    *dest = w32[i0 + j] ;              // always store value
    dest += (bits & 1) ;               // but increment pointer only if the store is "useful"
  }
#else
//   plain C code
  bits = 1 ; mask0 = 0 ;
  for(i = 0 ; i < 32 ; i++){
    int onoff = (w32[i+i0] != ref) ? 1 : 0 ;
    mask0 |= (onoff << i) ;   // insert onoff at the correct bit position
    *dest = w32[i+i0] ;       // always store
    dest += onoff ;           // increment only if the store is useful
    nval += onoff ;
  }
#endif

  *mask = mask0 ; mask++ ;
  }
  bits = 1 ; mask0 = 0 ;
  for(i = i0 ; i < nw32 ; i++){
    if(w32[i] != ref){
      mask0 |= bits ;
      *dest = w32[i] ; dest++ ;
      nval++ ;
    }
    bits <<= 1 ;
  }
  *mask = mask0 ;
  return nval ;
}
#endif
