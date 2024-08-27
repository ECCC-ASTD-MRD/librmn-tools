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

// #if defined(NO_SIMD)
// #warning "simd_functions : NO_SIMD IS DEFINED"
// #else
// #warning "simd_functions : NO_SIMD IS NOT DEFINED"
// #endif

#undef QUIET_SIMD_
#include <with_simd.h>

// #if defined(WITH_SIMD)
// #warning "simd_functions : using Intel SIMD intrinsics, use -DWITHOUT_SIMD to use pure C code"
// #else
// #warning "simd_functions : NOT using Intel SIMD intrinsics"
// #endif

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
#define MASK4I(V4,N) V4 = _mm_memmask_si128(N)

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
#define MASK8I(V8,N) V8 = _mm256_memmask_si256(N)

#endif

#endif

#if defined(__AVX2__) && defined(WITH_SIMD)

typedef __m256  V8F ;
typedef __m256i V8I ;
typedef __m128  V4F ;
typedef __m128i V4I ;

#define F4ADDI _mm_add_epi32
#define F8ADDI _mm256_add_epi32
#define F4MAXI _mm_max_epi32
#define F8MAXI _mm256_max_epi32
#define F4MINI _mm_min_epi32
#define F8MINI _mm256_min_epi32
#define F4MINU _mm_min_epu32
#define F8MINU _mm256_min_epu32

#else

typedef struct{ union{ float   f[8] ; int32_t  i[8] ; uint32_t u[8] ; } ; } V8F ;
typedef struct{ union{ float   f[8] ; int32_t  i[8] ; uint32_t u[8] ; } ; } V8I ;
// typedef V8F V8I
// typedef struct{ union{ int32_t v[8] ; uint32_t u[8] ; float    f[8] ; } ; } V8I ;
typedef struct{ union{ float   f[4] ; int32_t  i[4] ; uint32_t u[4] ; } ; } V4F ;
typedef v4F V4I
// typedef struct{ union{ int32_t v[4] ; uint32_t u[4] ; float    f[4] ; } ; } V4I ;

static inline V8F _mm256_set1_ps(float value){
  int i ;
  V8F R ;
  for(i=0 ; i<8 ; i++) R.f[i] = value ;
  return R ;
}

static inline V4F _mm_set1_ps(float value){
  int i ;
  V4F R ;
  for(i=0 ; i<4 ; i++) R.f[i] = value ;
  return R ;
}

static inline V8I _mm256_set1_epi32(int32_t value){
  int i ;
  V8I R ;
  for(i=0 ; i<8 ; i++) R.i[i] = value ;
  return R ;
}

static inline V4I _mm_set1_epi32(int32_t value){
  int i ;
  V4I R ;
  for(i=0 ; i<4 ; i++) R.i[i] = value ;
  return R ;
}

static inline void _mm256_storeu_si256(V8I *mem, V8I V){
  int i ;
  for(i=0 ; i<8 ; i++) mem->i[i] = V.i[i] ;
}

static inline void _mm_storeu_si128(V4I *mem, V4I V){
  int i ;
  for(i=0 ; i<4 ; i++) mem->i[i] = V.i[i] ;
}

static inline V8I _mm256_loadu_si256(V8I *mem){
  int i ;
  V8I R ;
  for(i=0 ; i<8 ; i++) R.i[i] = mem->i[i] ;
  return R ;
}

static inline V4I _mm_loadu_si128(V4I *mem){
  int i ;
  V4I R ;
  for(i=0 ; i<4 ; i++) R.i[i] = mem->i[i] ;
  return R ;
}

static inline V8F _mm256_loadu_ps(float *mem){
  int i ;
  V8F R ;
  for(i=0 ; i<8 ; i++) R.f[i] = mem[i] ;
  return R ;
}

static inline V4F _mm_loadu_ps(float *mem){
  int i ;
  V4F R ;
  for(i=0 ; i<4 ; i++) R.f[i] = mem[i] ;
  return R ;
}

static inline V8I _mm256_xor_si256( V8I A,  V8I B){
  int i ;
  V8I R ;
  for(i=0 ; i<8 ; i++) R.i[i] = A.i[i] ^ B.i[i] ;
  return R ;
}

static inline V4I _mm_xor_si128( V4I A,  V4I B){
  int i ;
  V4I R ;
  for(i=0 ; i<4 ; i++) R.i[i] = A.i[i] ^ B.i[i] ;
  return R ;
}

V8F _mm256_blendv_ps(V8F A, V8F B, V8F MASK){
  int i ;
  V8F R ;
  for(i=0 ; i<8 ; i++) R.i[i] = (MASK.i[i] < 0) ? B.i[i] : A.i[i] ;
  return R ;
}

static inline V8I _mm256_cmpeq_epi32(V8I A, V8I B){
  int i ;
  V8I R ;
  for(i=0 ; i<8 ; i++) R.i[i] = (A.i[i] == B.i[i]) ? -1 : 0 ;
  return R ;
}

static inline V4I _mm_cmpeq_epi32(V4I A, V4I B){
  int i ;
  V4I R ;
  for(i=0 ; i<4 ; i++) R.i[i] = (A.i[i] == B.i[i]) ? -1 : 0 ;
  return R ;
}

static inline V4I _mm256_extracti128_si256(V8I A, int upper){
  int i, offset = upper << 2 ;
  V4I R ;
  for(i=0 ; i<4 ; i++) R.i[i] = A.i[i+offset] ;
  return R ;
}

static inline V8I _mm256_max_epi32( V8I A,  V8I B){
  int i ;
  V8I R ;
  for(i=0 ; i<8 ; i++) R.i[i] = (A.i[i] > B.i[i]) ? A.i[i] : B.i[i] ;
  return R ;
}

static inline V4I _mm_max_epi32( V4I A,  V4I B){
  int i ;
  V4I R ;
  for(i=0 ; i<4 ; i++) R.v[i] = (A.i[i] > B.i[i]) ? A.i[i] : B.i[i] ;
  return R ;
}

static inline V8I _mm256_min_epi32( V8I A,  V8I B){
  int i ;
  V8I R ;
  for(i=0 ; i<8 ; i++) R.v[i] = (A.i[i] < B.i[i]) ? A.i[i] : B.i[i] ;
  return R ;
}

static inline V4I _mm_min_epi32( V4I A,  V4I B){
  int i ;
  V4I R ;
  for(i=0 ; i<4 ; i++) R.v[i] = (A.i[i] < B.i[i]) ? A.i[i] : B.i[i] ;
  return R ;
}

static inline V8I _mm256_min_epu32( V8I A,  V8I B){
  int i ;
  V8I R ;
  for(i=0 ; i<8 ; i++) R.u[i] = (A.u[i] < B.u[i]) ? A.u[i] : B.u[i] ;
  return R ;
}

static inline V4I _mm_min_epu32( V4I A,  V4I B){
  int i ;
  V4I R ;
  for(i=0 ; i<4 ; i++) R.u[i] = (A.u[i] < B.u[i]) ? A.u[i] : B.u[i] ;
  return R ;
}

static inline V8I _mm256_and_si256( V8I A,  V8I B){
  int i ;
  V8I R ;
  for(i=0 ; i<8 ; i++) R.i[i] = A.i[i] & B.i[i] ;
  return R ;
}

static inline V4I _mm_and_si128( V4I A,  V4I B){
  int i ;
  V4I R ;
  for(i=0 ; i<4 ; i++) R.i[i] = A.i[i] & B.i[i] ;
  return R ;
}

static inline V8I _mm256_add_epi32( V8I A,  V8I B){
  int i ;
  V8I R ;
  for(i=0 ; i<8 ; i++) R.i[i] = A.i[i] + B.i[i] ;
  return R ;
}

static inline V4I _mm_add_epi32( V4I A,  V4I B){
  int i ;
  V4I R ;
  for(i=0 ; i<4 ; i++) R.i[i] = A.i[i] + B.i[i] ;
  return R ;
}

static inline V8I _mm256_abs_epi32( V8I A){
  int i ;
  V8I R ;
  for(i=0 ; i<8 ; i++) R.i[i] = (A.i[i] < 0) ? -A.i[i] : A.i[i] ;
  return R ;
}

static inline V4I _mm_abs_epi32( V4I A){
  int i ;
  V4I R ;
  for(i=0 ; i<4 ; i++) R.i[i] = (A.i[i] < 0) ? -A.i[i] : A.i[i] ;
  return R ;
}

#endif

#define ZERO8I(V)              V = _mm256_xor_si256(V, V)
#define ZERO8F(V)              V = (V8F)_mm256_xor_si256((V8I)V, (V8I)V)
#define ZERO8(V,T)             V = (T)_mm256_xor_si256((V8I)V, (V8I)V)

#define SET8I(V,VALUE)         V = _mm256_set1_epi32(VALUE)
#define SET4I(V,VALUE)         V = _mm_set1_epi32(VALUE)
#define SET8F(V,VALUE)         V = _mm256_set1_ps(VALUE)
#define SET4F(V,VALUE)         V = _mm_set1_ps(VALUE)

#define FETCH8I(V,A256)       _mm256_loadu_si256((V8I *)(A256))
#define FETCH8F(V,A256)       _mm256_loadu_ps((float *)(A256))
#define FETCH8(V,T,A256)      (T)_mm256_loadu_si256((V8I *)(A256))

#define STORE8(V,A256)        _mm256_storeu_si256((V8I *)(A256), (V8I)V)
#define MOVE8I(V,S256,D256)   V = FETCH8I(V, S256) ; STORE8(V, D256)
#define MOVE8F(V,S256,D256)   V = FETCH8F(V, S256) ; STORE8(V, D256)
// #define MOVE8(V,T,S256,D256)  V = (T)FETCH8I(V, S256) ; STORE8(V, D256)

#define MAXS8(V,VMAX)         _mm256_max_epi32(VMAX, V)
#define MINS8(V,VMIN)         _mm256_min_epi32(VMIN, V)
#define MINU8(V,VMI0)         _mm256_min_epu32(VMI0, V)
#define MIMA8I(V,VMIN,VMAX)   VMAX = MAXS8(VMAX, V) ; VMIN = MINS8(VMIN, V)

#define AND8I(A,B)            _mm256_and_si256((V8I)A, (V8I)B)
#define ADD8I(A,B)            _mm256_add_epi32((V8I)A, (V8I)B)
#define ABS8I(V)              _mm256_abs_epi32(V)
#define VEQ8(A,B)             _mm256_cmpeq_epi32((V8I)A, (V8I)B)
#define BLEND8(A,B,MASK)      (V8I)_mm256_blendv_ps((V8F)A, (V8F)B, (V8F)MASK)

#define UPPER4(V8)            _mm256_extracti128_si256((V8I)V8, 1)
#define LOWER4(V8)            _mm256_extracti128_si256((V8I)V8, 0)

// fold 8 elements into 4 using 4 element function FN4
#define FOLD8_I4(FN4, V8, V4) V4 = FN4( (V4I)UPPER4(V8) , (V4I)LOWER4(V8) )
#define FOLD8_F4(FN4, V8, V4) V4 = FN4( (V4F)UPPER4(V8) , (V4F)LOWER4(V8) )

// fold 4 elements into 1 using 4 element function FN4
#define FOLD4_I1(FN4, V4)     V4 = FN4( (V4I)_mm_bsrli_si128((V4I)V4, 8), (V4I)V4) ; V4 = FN4( (V4I)_mm_bsrli_si128((V4I)V4, 4), (V4I)V4) ;
#define FOLD4_F1(FN4, V4)     V4 = FN4( (V4F)_mm_bsrli_si128((V4I)V4, 8), (V4F)V4) ; V4 = FN4( (V4F)_mm_bsrli_si128((V4I)V4, 4), (V4F)V4) ;

// fold 4 elements into 1 using 4 element function FN4 and store result
#define FOLD4_IS(FN4, V4, R)  FOLD4_I1(FN4, V4) ; _mm_storeu_si32(R , (V4I)V4) ;
#define FOLD4_FS(FN4, V4, R)  FOLD4_F1(FN4, V4) ; _mm_storeu_si32(R , (V4I)V4) ;

// fold 8 elements into 1 using 4 element function FN4 and store result
#define FOLD8_IS(FN4, V8, R) { V4I v = FOLD8_I4(FN4, V8, v) ; FOLD4_IS(FN4, v, R) ; }
#define FOLD8_FS(FN4, V8, R) { V4F v = FOLD8_F4(FN4, V8, v) ; FOLD4_FS(FN4, v, R) ; }
