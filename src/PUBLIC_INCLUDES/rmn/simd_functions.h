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

#include <with_simd.h>

#if defined(__AVX2__) && defined(WITH_SIMD)

#define V8F __m256
#define V8I __m256i
#define V4F __m128
#define V4I __m128i

#define ZERO8I(V)              V = (V8I)_mm256_xor_si256((V8I)V, (V8I)V)
#define ZERO8F(V)              V = (V8F)_mm256_xor_si256((V8I)V, (V8I)V)

#define FETCH8I(V, A256)       _mm256_loadu_si256((V8I *)(A256))
#define FETCH8F(V, A256)       _mm256_loadu_ps((float *)(A256))
#define FETCH8(V, T, A256)     (T)_mm256_loadu_si256((V8I *)(A256))

#define STORE8(V, A256)        _mm256_storeu_si256((V8I *)(A256), (V8I)V)
#define MOVE8I(V, S256, D256)  V = FETCH8I(V, S256) ; STORE8(V, D256)
#define MOVE8F(V, S256, D256)  V = FETCH8F(V, S256) ; STORE8(V, D256)
#define MOVE8(V, T, S256, D256) V = (T)FETCH8I(V, S256) ; STORE8(V, D256)

#define MAXS8(V, VMAX)         _mm256_max_epi32((V8I)VMAX, (V8I)V)
#define MINS8(V, VMIN)         _mm256_min_epi32((V8I)VMIN, (V8I)V)
#define MINU8(V, VMI0)         _mm256_min_epu32((V8I)V, (V8I)VMI0)
#define MIMA8I(V, VMIN, VMAX) VMAX = MAXS8(VMAX, V) ; VMIN = MINS8(VMIN, V)

#define ADD8I(A, B)            _mm256_add_epi32((V8I)A, (V8I)B)
#define ABS8I(V)               _mm256_abs_epi32(V)
#define VEQ8(A, B)             _mm256_cmpeq_epi32((V8I)A, (V8I)B)
#define BLEND8(A, B, MASK)  (V8I)_mm256_blendv_ps((V8F)A, (V8F)B, (V8F)MASK)

#define MIN08(V,VMI0,V0,VZ) { V8I z=VEQ8(V,V0) ; V=ABS8I(V) ; VZ=ADD8I(VZ,z) ; V=BLEND8(V,VMI0,z) ; VMI0=MINU8(V,VMI0) ; }

#define UPPER4(V8)             _mm256_extracti128_si256((V8I)V8, 1)
#define LOWER4(V8)             _mm256_extracti128_si256((V8I)V8, 0)

#else

#endif

#if defined(__x86_64__) && defined(WITH_SIMD)

#if defined(__SSE2__)
// build a 128 bit mask (4 x 32 bits) to keep n (0-4) elements in masked operations
// mask is built as a 32 bit mask (4x8bit), then expanded to 128 bits (4x32bit)
static inline __m128i _mm_memmask_si128(int n){
  __m128i vm ;
  uint32_t i32 = ~0u ;                           // all 1s
  i32 = (n&3) ? i32 >> ( 8 * (4 - (n&3)) ) : i32 ;     // shift right to eliminate unneeded elements
  vm = _mm_set1_epi32(i32) ;                     // load into 128 bit register
  return _mm_cvtepi8_epi32(vm) ;                 // convert from 8 bit to 32 bit mask (4 elements)
}
#endif

#if defined(__AVX2__)
// build a 256 bit mask (8 x 32 bits) to keep n (0-8) elements in masked operations
// mask is built as a 64 bit mask (8x8bit), then expanded to 256 bits (8x32bit)
static inline __m256i _mm256_memmask_si256(int n){
  __m128i vm ;
  uint64_t i64 = ~0lu ;                           // all 1s
  i64 = (n&7) ? i64 >> ( 8 * (8 - (n&7)) ) : i64 ;  // shift right to eliminate unneeded elements
  vm = _mm_set1_epi64x(i64) ;                     // load into 128 bit register
  return _mm256_cvtepi8_epi32(vm) ;               // convert from 8 bit to 32 bit mask (8 elements)
}
#endif

#endif
