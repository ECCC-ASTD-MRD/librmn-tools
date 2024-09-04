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

#include <stdio.h>
#include <rmn/ct_assert.h>

// #include <with_simd.h>
// plain C code version using arrays for SIMD types
typedef struct{ union{ double d[4]; int64_t l[4]; float f[8]; int32_t i[8]; uint32_t u[8]; int16_t hi[16]; uint16_t hu[16]; int8_t c[32]; uint8_t cu[32]; } ; } __v256x ;
CT_ASSERT(sizeof(__v256x) == 32, "ERROR: sizeof(__v256x) MUST BE 32")
typedef struct{ union{ double d[2]; int64_t l[2]; float f[4]; int32_t i[4]; uint32_t u[4]; int16_t hi[ 8]; uint16_t hu[ 8]; int8_t c[16]; uint8_t cu[16]; } ; } __v128x ;
CT_ASSERT(sizeof(__v128x) == 16, "ERROR: sizeof(__v128x) MUST BE 16")

#if defined(USE_INTEL_INTRINSICS)

#define WITH_SIMD
#undef ALIAS_INTEL_INTRINSICS

#if defined(__x86_64__)
#if defined(__AVX2__)
#include <immintrin.h>
#elif defined(__SSE2__)
#include <emmintrin.h>
#endif
#endif    // __x86_64__

#if defined(VERBOSE_SIMD)
#warning "simd_functions : using Intel SIMD intrinsics"
#endif

#else     // USE_INTEL_INTRINSICS

#if defined(VERBOSE_SIMD)
#warning "simd_functions : NOT using Intel SIMD intrinsics"
#endif

#endif    // USE_INTEL_INTRINSICS

#define STATIC static inline
#define SIMD_LOOP(N, OPER) { int i ;  for(i=0 ; i<N ; i++) { OPER ; } ; }
#define SIMD_FN(SCOPE, KIND, N, FN, OPER) SCOPE KIND FN { KIND R ; SIMD_LOOP(N, OPER) ; return R ; }
#define VOID_FN(SCOPE, N, FN, OPER)       SCOPE void FN {          SIMD_LOOP(N, OPER) ;            }

// #if defined(__x86_64__) && defined(USE_INTEL_INTRINSICS)
#if defined(USE_INTEL_INTRINSICS)
// #define F4ADDI _mm_add_epi32
// #define F8ADDI _mm256_add_epi32
// #define F4MAXI _mm_max_epi32
// #define F8MAXI _mm256_max_epi32
// #define F4MINI _mm_min_epi32
// #define F8MINI _mm256_min_epi32
// #define F4MINU _mm_min_epu32
// #define F8MINU _mm256_min_epu32

// casts (true casts with native SIMD intrinsics)
#define __V256   (__m256)
#define __V256f  (__m256)
#define __V256i  (__m256i)
#define __V256d  (__m256d)
#define __V128   (__m128)
#define __V128f  (__m128)
#define __V128i  (__m128i)
#define __V128d  (__m128d)

typedef __m256  __v256f ;
typedef __m256i __v256i ;
typedef __m256d __v256d ;
typedef __m128  __v128f ;
typedef __m128i __v128i ;
typedef __m128d __v128d ;

#endif    // defined(__x86_64__) && defined(USE_INTEL_INTRINSICS)

#if ! defined(USE_INTEL_INTRINSICS)
// casts (fake casts with simulated SIMD intrinsics, all typedefs are the same)
#define __V256
#define __V256f
#define __V256i
#define __V256d
#define __V128
#define __V128f
#define __V128i
#define __V128d

typedef __v256x __m256  ;
typedef __v256x __m256i ;
typedef __v256x __m256d ;
typedef __v128x __m128  ;
typedef __v128x __m128i ;
typedef __v128x __m128d ;

typedef __v256x __v256f ;
typedef __v256x __v256i ;
typedef __v256x __v256d ;
typedef __v128x __v128f ;
typedef __v128x __v128i ;
typedef __v128x __v128d ;

#endif

#if defined(ALIAS_INTEL_INTRINSICS)

#define _mm256_set1_ps     set1_v8f
#define _mm_set1_ps        set1_v4f
#define _mm256_set1_pd     set1_v4d
#define _mm_set1_pd        set1_v2d
#define _mm256_set1_epi64x set1_v4l
#define _mm_set1_epi64x    set1_v2l
#define _mm256_set1_epi32  set1_v8i
#define _mm_set1_epi32     set1_v4i

#define _mm256_cvtepi8_epi32 cvt_v8c_v8i
#define _mm_cvtepi8_epi32    cvt_v4c_v4i
#endif
#if defined(USE_INTEL_INTRINSICS)

#define  set1_v8f _mm256_set1_ps
#define  set1_v4f _mm_set1_ps
#define  set1_v4d _mm256_set1_pd
#define  set1_v2d _mm_set1_pd
#define  set1_v4l _mm256_set1_epi64x
#define  set1_v2l _mm_set1_epi64x
#define  set1_v8i _mm256_set1_epi32
#define  set1_v4i _mm_set1_epi32

#define cvt_v8c_v8i _mm256_cvtepi8_epi32
#define cvt_v4c_v4i _mm_cvtepi8_epi32
#else
SIMD_FN(STATIC, __m256,  8, set1_v8f(float    f32) , R.f[i] = f32)
SIMD_FN(STATIC, __m128,  4, set1_v4f(float    f32) , R.f[i] = f32)
SIMD_FN(STATIC, __m256,  4, set1_v4d(double   f64) , R.d[i] = f64)
SIMD_FN(STATIC, __m128,  2, set1_v2d(double   f64) , R.d[i] = f64)
SIMD_FN(STATIC, __m256i, 4, set1_v4l(uint64_t i64) , R.l[i] = i64 )
SIMD_FN(STATIC, __m128i, 2, set1_v2l(uint64_t i64) , R.l[i] = i64 )
SIMD_FN(STATIC, __m256i, 8, set1_v8i(int32_t  i32) , R.l[i] = i32 )
SIMD_FN(STATIC, __m128i, 4, set1_v4i(int32_t  i32) , R.l[i] = i32 )

SIMD_FN(STATIC, __m256i, 8, _mm256_cvtepi8_epi32(__m128i A) , R.i[i] = A.c[i] )     // convert signed 8 bit to 32 bit (8 values)
SIMD_FN(STATIC, __m128i, 4, _mm_cvtepi8_epi32(__m128i A)    , R.i[i] = A.c[i] )     // convert signed 8 bit to 32 bit (4 values)
#endif

#if defined(ALIAS_INTEL_INTRINSICS)
#endif
#if defined(USE_INTEL_INTRINSICS)
#else
#endif

#if ! defined(USE_INTEL_INTRINSICS)
// cast float to integer
SIMD_FN(STATIC, __m256i, 8, m256_ps_i(__m256 A) , R.i[i] = A.i[i] )
// cast integer to float
SIMD_FN(STATIC, __m256i, 8, m256_i_ps(__m256 A) , R.i[i] = A.i[i] )

#if defined(ALIAS_INTEL_INTRINSICS)
#define _mm256_loadu_si256  loadu_v256
#define _mm_loadu_si128     loadu_v128
#define _mm256_loadu_ps     loadu_v8f
#define _mm_loadu_ps        loadu_v4f
#define _mm256_storeu_si256 storeu_v256
#define _mm_storeu_si128    storeu_v128

#endif
#if defined(USE_INTEL_INTRINSICS)
#else
#endif
SIMD_FN(STATIC, __m256i, 8, _mm256_loadu_si256(__m256i *mem),   R.i[i] = mem->i[i] )
SIMD_FN(STATIC, __m128i, 4, _mm_loadu_si128(__m128i *mem),      R.i[i] = mem->i[i] )

SIMD_FN(STATIC, __m256i, 8, _mm256_loadu_ps(float *mem),   R.i[i] = mem[i] )
SIMD_FN(STATIC, __m128i, 4, _mm_loadu_ps(float *mem),      R.i[i] = mem[i] )

VOID_FN(STATIC, 8, _mm256_storeu_si256(__m256i *mem, __m256i V), mem->i[i] = V.i[i] )
VOID_FN(STATIC, 4, _mm_storeu_si128(__m128i *mem, __m128 V),     mem->i[i] = V.i[i] )

VOID_FN(STATIC, 8, _mm256_storeu_ps(float *mem, __m256i V), mem[i] = V.f[i] )
VOID_FN(STATIC, 4, _mm_storeu_ps(float *mem, __m128 V),     mem[i] = V.f[i] )

SIMD_FN(STATIC, __m256i, 8, _mm256_abs_epi32(__m256i A) , R.i[i] = (A.i[i] < 0) ? -A.i[i] : A.i[i] )
SIMD_FN(STATIC, __m128i, 4, _mm_abs_epi32(__m128i A)    , R.i[i] = (A.i[i] < 0) ? -A.i[i] : A.i[i] )

SIMD_FN(STATIC, __m256i, 8, _mm256_max_epi32(__m256i A, __m256i B) , R.i[i] = (A.i[i] > B.i[i]) ? A.i[i] : B.i[i] )
SIMD_FN(STATIC, __m128i, 4, _mm_max_epi32(__m128i A, __m128i B)    , R.i[i] = (A.i[i] > B.i[i]) ? A.i[i] : B.i[i] )

SIMD_FN(STATIC, __m256i, 8, _mm256_min_epi32(__m256i A, __m256i B) , R.i[i] = (A.i[i] < B.i[i]) ? A.i[i] : B.i[i] )
SIMD_FN(STATIC, __m128i, 4, _mm_min_epi32(__m128i A, __m128i B)    , R.i[i] = (A.i[i] < B.i[i]) ? A.i[i] : B.i[i] )

SIMD_FN(STATIC, __m256i, 8, _mm256_max_epu32(__m256i A, __m256i B) , R.u[i] = (A.u[i] > B.u[i]) ? A.u[i] : B.u[i] )
SIMD_FN(STATIC, __m128i, 4, _mm_max_epu32(__m128i A, __m128i B)    , R.u[i] = (A.u[i] > B.u[i]) ? A.u[i] : B.u[i] )

SIMD_FN(STATIC, __m256i, 8, _mm256_min_epu32(__m256i A, __m256i B) , R.u[i] = (A.u[i] < B.u[i]) ? A.u[i] : B.u[i] )
SIMD_FN(STATIC, __m128i, 4, _mm_min_epu32(__m128i A, __m128i B)    , R.u[i] = (A.u[i] < B.u[i]) ? A.u[i] : B.u[i] )

SIMD_FN(STATIC, __m256i, 8, _mm256_add_epi32(__m256i A, __m256i B) , R.i[i] = A.i[i] + B.i[i] )
SIMD_FN(STATIC, __m128i, 4, _mm_add_epi32(__m128i A, __m128i B)    , R.i[i] = A.i[i] + B.i[i] )

SIMD_FN(STATIC, __m256i, 8, _mm256_sub_epi32(__m256i A, __m256i B) , R.i[i] = A.i[i] - B.i[i] )
SIMD_FN(STATIC, __m128i, 4, _mm_sub_epi32(__m128i A, __m128i B)    , R.i[i] = A.i[i] - B.i[i] )

SIMD_FN(STATIC, __m256i, 8, _mm256_xor_si256( __m256i A,  __m256i B), R.i[i] = A.i[i] ^ B.i[i])
SIMD_FN(STATIC, __m128i, 4, _mm_xor_si128( __m128i A,  __m128i B),    R.i[i] = A.i[i] ^ B.i[i])

SIMD_FN(STATIC, __m256i, 8, _mm256_and_si256( __m256i A,  __m256i B), R.i[i] = A.i[i] & B.i[i])
SIMD_FN(STATIC, __m128i, 4, _mm_and_si128( __m128i A,  __m128i B),    R.i[i] = A.i[i] & B.i[i])

SIMD_FN(STATIC, __m256i, 8, _mm256_or_si256( __m256i A,  __m256i B), R.i[i] = A.i[i] | B.i[i])
SIMD_FN(STATIC, __m128i, 4, _mm_or_si128( __m128i A,  __m128i B),    R.i[i] = A.i[i] | B.i[i])

SIMD_FN(STATIC, __m256, 8, _mm256_blendv_ps(__m256 A, __m256 B, __m256 MASK), R.i[i] = ((MASK.i[i] >> 31) & (B.i[i] ^ A.i[i])) ^  A.i[i] )
SIMD_FN(STATIC, __m128, 4, _mm_blendv_ps(__m128 A, __m128 B, __m128 MASK),    R.i[i] = ((MASK.i[i] >> 31) & (B.i[i] ^ A.i[i])) ^  A.i[i] )

SIMD_FN(STATIC, __m256i, 32, _mm256_blendv_epi8(__m256i A, __m256i B, __m256i MASK), R.cu[i] = ((MASK.c[i] >> 7) & (B.cu[i] ^ A.cu[i])) ^  A.cu[i] )
SIMD_FN(STATIC, __m128i, 16, _mm_blendv_epi8(__m128i A, __m128i B, __m128i MASK),    R.cu[i] = ((MASK.c[i] >> 7) & (B.cu[i] ^ A.cu[i])) ^  A.cu[i] )

SIMD_FN(STATIC, __m256i, 8, _mm256_cmpeq_epi32(__m256i A, __m256i B), R.i[i] = (A.i[i] == B.i[i]) ? -1 : 0)
SIMD_FN(STATIC, __m128i, 4, _mm_cmpeq_epi32(__m128i A, __m128i B),    R.i[i] = (A.i[i] == B.i[i]) ? -1 : 0)

SIMD_FN(STATIC, __m128i, 4, _mm256_extracti128_si256(__m256i A, int upper) , R.i[i] = A.i[i + (upper ? 4 : 0)] )
// SIMD_FN(STATIC, __m256i, 4, _mm256_inserti128_si256(__m256i A, __m128i B, int upper) , R.i[i + (upper ? 4 : 0)] = B.i[i] ; R.i[i + (upper ? 0 : 4)] = A.i[i] )
SIMD_FN(STATIC, __m256i, 4, _mm256_inserti128_si256(__m256i A, __m128i B, int upper) , R.i[i] = upper ? B.i[i] : A.i[i] ; R.i[i+4] = upper ? A.i[i] : B.i[i] )

SIMD_FN(STATIC, __m128i, 4, _mm256_castsi256_si128(__m256i A) , R.i[i] = A.i[i] )
SIMD_FN(STATIC, __m256i, 4, _mm256_castsi128_si256(__m128 A) ,  R.i[i] = A.i[i] )

#endif

// integer blend, defined using float blend with type cast
SIMD_FN(STATIC, __m256i, 8, _mm256_blendv_epi32(__m256i A, __m256i B, __m256i MASK), R = __V256i _mm256_blendv_ps(__V256 A, __V256 B, __V256 MASK) )
SIMD_FN(STATIC, __m128i, 4, _mm_blendv_epi32(__m128i A, __m128i B, __m128i MASK),    R = __V128i _mm_blendv_ps(__V128 A, __V128 B, __V128 MASK) )

typedef struct{ union{ double d[4]; int64_t l[4]; float f[8]; int32_t i[8]; uint32_t u[8]; int16_t hi[16]; uint16_t hu[16]; int8_t c[32]; uint8_t cu[32]; } ; } VECTOR_256 ;
typedef struct{ union{ double d[2]; int64_t l[2]; float f[4]; int32_t i[4]; uint32_t u[4]; int16_t hi[ 8]; uint16_t hu[ 8]; int8_t c[16]; uint8_t cu[16]; } ; } VECTOR_128 ;

// print 128 bit vectors
static void _mm_print_pd(char *msg, __m128i vm){
  int i ; VECTOR_128 v ;
  _mm_storeu_si128((__m128i *) &v, vm) ;
  fprintf(stderr, "%s : ", msg) ;
  for(i=0 ; i<2 ; i++) fprintf(stderr, "%23.16E ", v.d[i]);
  fprintf(stderr, "\n") ;
}
static void _mm_print_ps(char *msg, __m128i vm){
  int i ; VECTOR_128 v ;
  _mm_storeu_si128((__m128i *) &v, vm) ;
  fprintf(stderr, "%s : ", msg) ;
  for(i=0 ; i<4 ; i++) fprintf(stderr, "%11.4E ", v.f[i]);
  fprintf(stderr, "\n") ;
}
static void _mm_print_epu64(char *msg, __m128i vm){
  int i ; VECTOR_128 v ;
  _mm_storeu_si128((__m128i *) &v, vm) ;
  fprintf(stderr, "%s : ", msg) ;
  for(i=0 ; i<2 ; i++) fprintf(stderr, "%23.16lx ", v.l[i]);
  fprintf(stderr, "\n") ;
}
static void _mm_print_epu32(char *msg, __m128i vm){
  int i ; VECTOR_128 v ;
  _mm_storeu_si128((__m128i *) &v, vm) ;
  fprintf(stderr, "%s : ", msg) ;
  for(i=0 ; i<4 ; i++) fprintf(stderr, "%11.8x ", v.u[i]);
  fprintf(stderr, "\n") ;
}
static void _mm_print_epu16(char *msg, __m128i vm){
  int i ; VECTOR_128 v ;
  _mm_storeu_si128((__m128i *) &v, vm) ;
  fprintf(stderr, "%s : ", msg) ;
  for(i=0 ; i<8 ; i++) fprintf(stderr, "%5.4x ", v.hu[i]);
  fprintf(stderr, "\n") ;
}
static void _mm_print_epu8(char *msg, __m128i vm){
  int i ; VECTOR_128 v ;
  _mm_storeu_si128((__m128i *) &v, vm) ;
  fprintf(stderr, "%s : ", msg) ;
  for(i=0 ; i<16 ; i++) fprintf(stderr, "%2.2x ", v.cu[i]);
  fprintf(stderr, "\n") ;
}

// print 256 bit vectors
static void _mm256_print_pd(char *msg, __m256i vm){
  int i ; VECTOR_256 v ;
  _mm256_storeu_si256((__m256i *) &v, vm) ;
  fprintf(stderr, "%s : ", msg) ;
  for(i=0 ; i<4 ; i++) fprintf(stderr, "%23.16E ", v.d[i]);
  fprintf(stderr, "\n") ;
}
static void _mm256_print_ps(char *msg, __m256i vm){
  int i ; VECTOR_256 v ;
  _mm256_storeu_si256((__m256i *) &v, vm) ;
  fprintf(stderr, "%s : ", msg) ;
  for(i=0 ; i<8 ; i++) fprintf(stderr, "%11.4E ", v.f[i]);
  fprintf(stderr, "\n") ;
}
static void _mm256_print_epu64(char *msg, __m256i vm){
  int i ; VECTOR_256 v ;
  _mm256_storeu_si256((__m256i *) &v, vm) ;
  fprintf(stderr, "%s : ", msg) ;
  for(i=0 ; i<4 ; i++) fprintf(stderr, "%23.16lx ", v.l[i]);
  fprintf(stderr, "\n") ;
}
static void _mm256_print_epu32(char *msg, __m256i vm){
  int i ; VECTOR_256 v ;
  _mm256_storeu_si256((__m256i *) &v, vm) ;
  fprintf(stderr, "%s : ", msg) ;
  for(i=0 ; i<8 ; i++) fprintf(stderr, "%11.8x ", v.u[i]);
  fprintf(stderr, "\n") ;
}
static void _mm256_print_epu16(char *msg, __m256i vm){
  int i ; VECTOR_256 v ;
  _mm256_storeu_si256((__m256i *) &v, vm) ;
  fprintf(stderr, "%s : ", msg) ;
  for(i=0 ; i<16 ; i++) fprintf(stderr, "%5.4x ", v.hu[i]);
  fprintf(stderr, "\n") ;
}
static void _mm256_print_epu8(char *msg, __m256i vm){
  int i ; VECTOR_256 v ;
  _mm256_storeu_si256((__m256i *) &v, vm) ;
  fprintf(stderr, "%s : ", msg) ;
  for(i=0 ; i<32 ; i++) fprintf(stderr, "%2.2x ", v.cu[i]);
  fprintf(stderr, "\n") ;
}

// build a 128 bit mask (4 x 32 bits) to keep n (0-4) elements in masked operations
// mask is built as a 32 bit mask (4x8bit), then expanded to 128 bits (4x32bit)
static inline __m128i _mm_memmask_epi32(int n){
  __m128i vm ;
  uint32_t i32 = ~0u ;                           // all 1s
  i32 = (n&3) ? i32 >> ( 8 * (4 - (n&3)) ) : i32 ;     // shift right to eliminate unneeded elements
  vm = _mm_set1_epi32(i32) ;                     // load into 128 bit register
  return _mm_cvtepi8_epi32(vm) ;                 // convert from 8 bit to 32 bit mask (4 elements)
}

// build a 256 bit mask (8 x 32 bits) to keep n (0-8) elements in masked operations
// mask is built as a 64 bit mask (8x8bit), then expanded to 256 bits (8x32bit)
static inline __m256i _mm256_memmask_epi32(int n){
  __m128i vm ;
  uint64_t i64 = ~0lu ;                           // all 1s
  i64 = (n&7) ? i64 >> ( 8 * (8 - (n&7)) ) : i64 ;  // shift right to eliminate unneeded elements
  vm = _mm_set1_epi64x(i64) ;                     // load into 128 bit register
  return _mm256_cvtepi8_epi32(vm) ;               // convert from 8 bit to 32 bit mask (8 elements)
}

#define ZERO8I(V)              V = _mm256_xor_si256(V, V)
#define ZERO8F(V)              V = (__m256)_mm256_xor_si256((__m256i)V, (__m256i)V)
#define ZERO8(V,T)             V = T _mm256_xor_si256((__m256i)V, (__m256i)V)

#define SET8I(V,VALUE)         V = _mm256_set1_epi32(VALUE)
#define SET4I(V,VALUE)         V = _mm_set1_epi32(VALUE)
#define SET8F(V,VALUE)         V = _mm256_set1_ps(VALUE)
#define SET4F(V,VALUE)         V = _mm_set1_ps(VALUE)

#define FETCH8I(V,A256)       _mm256_loadu_si256((__m256i *)(A256))
#define FETCH8F(V,A256)       _mm256_loadu_ps((float *)(A256))
#define FETCH8(V,T,A256)      T _mm256_loadu_si256((__m256i *)(A256))

#define STORE8(V,A256)        _mm256_storeu_si256((__m256i *)(A256), (__m256i)V)
#define MOVE8I(V,S256,D256)   V = FETCH8I(V, S256) ; STORE8(V, D256)
#define MOVE8F(V,S256,D256)   V = FETCH8F(V, S256) ; STORE8(V, D256)
// #define MOVE8(V,T,S256,D256)  V = (T)FETCH8I(V, S256) ; STORE8(V, D256)

#define MAXS8(V,VMAX)         _mm256_max_epi32(VMAX, V)
#define MINS8(V,VMIN)         _mm256_min_epi32(VMIN, V)
#define MINU8(V,VMI0)         _mm256_min_epu32(VMI0, V)
#define MIMA8I(V,VMIN,VMAX)   VMAX = MAXS8(VMAX, V) ; VMIN = MINS8(VMIN, V)

#define AND8I(A,B)            _mm256_and_si256((__m256i)A, (__m256i)B)
#define ADD8I(A,B)            _mm256_add_epi32((__m256i)A, (__m256i)B)
#define ABS8I(V)              _mm256_abs_epi32(V)
#define VEQ8(A,B)             _mm256_cmpeq_epi32((__m256i)A, (__m256i)B)
#define BLEND8(A,B,MASK)      (__m256i)_mm256_blendv_ps((__m256)A, (__m256)B, (__m256)MASK)

#define UPPER4(V8)            _mm256_extracti128_si256((__m256i)V8, 1)
#define LOWER4(V8)            _mm256_extracti128_si256((__m256i)V8, 0)

// fold 8 elements into 4 using 4 element function FN4
#define FOLD8_I4(FN4, V8, V4) V4 = FN4( (__m128i)UPPER4(V8) , (__m128i)LOWER4(V8) )
#define FOLD8_F4(FN4, V8, V4) V4 = FN4( (__m128)UPPER4(V8) , (__m128)LOWER4(V8) )

// fold 4 elements into 1 using 4 element function FN4
#define FOLD4_I1(FN4, V4)     V4 = FN4( (__m128i)_mm_bsrli_si128((__m128i)V4, 8), (__m128i)V4) ; V4 = FN4( (__m128i)_mm_bsrli_si128((__m128i)V4, 4), (__m128i)V4) ;
#define FOLD4_F1(FN4, V4)     V4 = FN4( (__m128)_mm_bsrli_si128((__m128i)V4, 8), (__m128)V4) ; V4 = FN4( (__m128)_mm_bsrli_si128((__m128i)V4, 4), (__m128)V4) ;

// fold 4 elements into 1 using 4 element function FN4 and store result
#define FOLD4_IS(FN4, V4, R)  FOLD4_I1(FN4, V4) ; _mm_storeu_si32(R , (__m128i)V4) ;
#define FOLD4_FS(FN4, V4, R)  FOLD4_F1(FN4, V4) ; _mm_storeu_si32(R , (__m128i)V4) ;

// fold 8 elements into 1 using 4 element function FN4 and store result
#define FOLD8_IS(FN4, V8, R) { __m128i v = FOLD8_I4(FN4, V8, v) ; FOLD4_IS(FN4, v, R) ; }
#define FOLD8_FS(FN4, V8, R) { __m128 v = FOLD8_F4(FN4, V8, v) ; FOLD4_FS(FN4, v, R) ; }
