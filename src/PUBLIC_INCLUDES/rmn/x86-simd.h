// Hopefully useful code for C
// Copyright (C) 2023  Recherche en Prevision Numerique
//
// This code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
// Author:
//     M. Valin,   Recherche en Prevision Numerique, 2023
//
// inspired by / borrowed from code by Paul Mineiro
// original license from said code below
/*=====================================================================*
 *                   Copyright (C) 2011 Paul Mineiro                   *
 * All rights reserved.                                                *
 *                                                                     *
 * Redistribution and use in source and binary forms, with             *
 * or without modification, are permitted provided that the            *
 * following conditions are met:                                       *
 *                                                                     *
 *     * Redistributions of source code must retain the                *
 *     above copyright notice, this list of conditions and             *
 *     the following disclaimer.                                       *
 *                                                                     *
 *     * Redistributions in binary form must reproduce the             *
 *     above copyright notice, this list of conditions and             *
 *     the following disclaimer in the documentation and/or            *
 *     other materials provided with the distribution.                 *
 *                                                                     *
 *     * Neither the name of Paul Mineiro nor the names                *
 *     of other contributors may be used to endorse or promote         *
 *     products derived from this software without specific            *
 *     prior written permission.                                       *
 *                                                                     *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND              *
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,         *
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES               *
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE             *
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER               *
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,                 *
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES            *
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE           *
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR                *
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF          *
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT           *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY              *
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE             *
 * POSSIBILITY OF SUCH DAMAGE.                                         *
 *                                                                     *
 * Contact: Paul Mineiro <paul@mineiro.com>                            *
 *=====================================================================*/
//
// AVX2 SIMD primitives, extending original SSE2 code
//

#if ! defined(v2dil1)

#if defined(__AVX2__)
#include <immintrin.h>
#include <stdint.h>

// 128 bit vectors
typedef __m128  v4sf;      // 128 bit vector, 32 bit floats
typedef __m128i v4si;      // 128 bit vector, 32 bit integers

typedef union{v4si v128 ; uint32_t m[4] ; } v4mi ;      // 128 bit vector / memory (integers)
typedef union{v4sf v128 ; float    m[4] ; } v4mf ;      // 128 bit vector / memory (floats)

// 256 bit vectors
typedef __m256  v8sf;      // 256 bit vector, 32 bit floats
typedef __m256i v8si;      // 256 bit vector, 32 bit integers

typedef union{v8si v256 ; v4si v128[2] ; } v48i ;      // 256 bit vector / 128 bit vector
typedef union{v8sf v256 ; v4sf v128[2] ; } v48f ;

typedef union{v8si v256 ; v4si v128[2] ; uint32_t m[8] ; } v48mi ;      // 256 bit vector / 128 bit vector / memory
typedef union{v8sf v256 ; v4sf v128[2] ; float    m[8] ; } v48mf ;

// int32_t <-> float conversions, 128 bit vectors
#define v4si_to_v4sf _mm_cvtepi32_ps
#define v4sf_to_v4si _mm_cvttps_epi32
// int32_t <-> float conversions, 256 bit vectors
#define v8si_to_v8sf _mm256_cvtepi32_ps
#define v8sf_to_v8si _mm256_cvttps_epi32

// create 1 64 bit item from 2 32 bit items
#define v2sil2(x,y) ( (((uint64_t) (y)) << 32) | (x) )
// create 1 64 bit item from 2 copies of a single 32 bit item
#define v2sil1(x) v2sil2(x,x)

// fill 128 bit register with a constant value
// single integer value
#define v2dil1(x) ((v4si) { (x), (x) })                                     // initialize with 2 identical 64 bit items
#define v4sil1(x) v2dil1( v2sil1(x) )                                       // initialize with 4 identical 32 bit items
// 4 integer values
#define v4sil4(a, b, c, d) ((v4si) { v2sil2(a,b) , v2sil2(c,d) } )          // initialize with 4 32 bit items
// 1 float value
#define v4sfl1(x) ((v4sf) { (x), (x), (x), (x) })                           // initialize with 4 identical float items
// 4 float values
#define v4sfl4(a,b,c,d) ((v4sf) { (a), (b), (c), (d) })                     // initialize with 4 float items

// fill 256 bit register with a constant value
// single integer value
#define v4dil1(x) ((v8si) { (x), (x), (x), (x) })                           // initialize with 4 identical 64 bit integers
#define v8sil1(x) v4dil1( v2sil1(x) )                                       // initialize with 8 identical 32 bit integers
// 8 integer values
#define v8sil8(a, b, c, d, e, f, g, h) ((v8si) { v2sil2(a,b) , v2sil2(c,d) , v2sil2(e,f) , v2sil2(g,h) } )  // initialize with 8 32 bit integers
// single float value
#define v8sfl1(x) ((v8sf) { (x), (x), (x), (x), (x), (x), (x), (x) })       // initialize with 8 identical floats
// 8 float values
#define v8sfl8(a,b,c,d,e,f,g,h) ((v8sf) { (a), (b), (c), (d), (e), (f), (g), (h) })  // initialize with 8 floats

// set vector to zero (force integer mode) (will have to cast result to (v4sf/v8sf) if zeroing floats)
#define v4zero(x) ((v4si) x ^ (v4si) x)    // value XOR value = 0
#define v8zero(x) ((v8si) x ^ (v8si) x)

#define MAX128(r, a, b, KIND) if (KIND == 1) { r = _mm_max_epi32(a, b) ; } ; \
                              if (KIND == 0) { r = _mm_max_epu32(a, b) ; } ;
#define MIN128(r, a, b, KIND) if (KIND == 1) { r = _mm_min_epi32(a, b) ; } ; \
                              if (KIND == 0) { r = _mm_min_epu32(a, b) ; } ;

// 4 2 bit constant values packed into an 8 bit immediate value
#define IMM8(a, b, c, d) (a + (b*4) + (c*16) + (d*64))
// reduce 4 elements to one value using operator OP
#define REDUCE4(v, r, OP) { v4si x = (v4si)v OP _mm_shuffle_epi32((v4si)v , IMM8(2, 3, 0, 1)) ; r = _mm_cvtsi128_si32(x OP _mm_shuffle_epi32(x , IMM8(1, 0, 3, 2))) ; }
// reduce 8 elements to one value using operator OP
#define REDUCE8(v, r, OP) { v48i t ; t.v256 = (v8si)v ; t.v128[0] = t.v128[0] & t.v128[1] ; REDUCE4(t.v128[0], r, OP) ; }

// some useful REDUCTION elementary functions
static inline int32_t v128_maxsi(v4si v){
  int32_t r ;
  v4si x = _mm_max_epi32(v, _mm_shuffle_epi32((v4si)v , IMM8(2, 3, 0, 1))) ;
  x      = _mm_max_epi32(v, _mm_shuffle_epi32((v4si)v , IMM8(1, 0, 3, 2))) ;
  r      = _mm_cvtsi128_si32(x) ;
  return r ;
}

static inline uint32_t v128_maxsu(v4si v){
  int32_t r ;
  v4si x = _mm_max_epu32(v, _mm_shuffle_epi32((v4si)v , IMM8(2, 3, 0, 1))) ;
  x      = _mm_max_epu32(v, _mm_shuffle_epi32((v4si)v , IMM8(1, 0, 3, 2))) ;
  r      = _mm_cvtsi128_si32(x) ;
  return r ;
}

static inline float v128_maxsf(v4sf v){
  float r ;
  v4sf x = _mm_max_ps(v, _mm_shuffle_ps((v4sf)v , (v4sf)v , IMM8(2, 3, 0, 1))) ;
  x      = _mm_max_ps(v, _mm_shuffle_ps((v4sf)v , (v4sf)v , IMM8(1, 0, 3, 2))) ;
  r      = _mm_cvtss_f32(x) ;
  return r ;
}

// get lower part of a 256 bit register
static inline v4si v256_lo(v8si v){
  v48i t = {.v256 = v } ;
  return t.v128[0] ;
}

// get upper part of a 256 bit register
static inline v4si v256_hi(v8si v){
  v48i t = {.v256 = v } ;
  return t.v128[1] ;
}

// 256 bit to 128 bit REDUCTION functions
// logical AND of lower 4 integers and upper 4 integers from a 256 bit vector
static inline v4si v256fold_andi(v8si v){
  v48i t = {.v256 = v } ;
  return t.v128[0] & t.v128[1] ;
}

// logical OR of lower 4 integers and upper 4 integers from a 256 bit vector
static inline v4si v256fold_ori(v8si v){
  v48i t = {.v256 = v } ;
  return t.v128[0] | t.v128[1] ;
}

// integer SUM of lower 4 integers and upper 4 integers from a 256 bit vector
static inline v4si v256fold_sumi(v8si v){
  v48i t = {.v256 = v } ;
  return t.v128[0] + t.v128[1] ;
}

// float SUM of lower 4 floats and upper 4 floats from a 256 bit vector
static inline v4sf v256fold_sumf(v8sf v){
  v48f t = {.v256 = v } ;
  return t.v128[0] + t.v128[1] ;
}

// 128 bit to 32 bit REDUCTION functions

// logical AND of the 4 32 bit integers in a 128 bit vector
static inline int v128andi(v4si v){
  v = v & _mm_shuffle_epi32(v , 0b00001110) ;
  return _mm_cvtsi128_si32(v & _mm_shuffle_epi32(v , 0b00000001)) ;
}

// logical OR of the 4 32 bit integers in a 128 bit vector
static inline int v128ori(v4si v){
  v = v | _mm_shuffle_epi32(v , 0b00001110) ;
  return _mm_cvtsi128_si32(v | _mm_shuffle_epi32(v , 0b00000001)) ;
}

// integer SUM of the 4 32 bit integers in a 128 bit vector
static inline int v128sumi(v4si v){
  v = v + _mm_shuffle_epi32(v , 0b00001110) ;
  return _mm_cvtsi128_si32(v + _mm_shuffle_epi32(v , 0b00000001)) ;
}

// float sum of the 4 32 bit floats in a 128 bit vector
static inline float v128sumf(v4sf v){
  v = v + _mm_shuffle_ps(v , v, 0b00001110) ;
  v = v + _mm_shuffle_ps(v , v, 0b00000001) ;
  v4mf t = { .v128 = v } ;
  return t.m[0] ;
}

// 256 bit to 32 bit REDUCTION functions

// logical AND of the 8 32 bit integers in a 256 bit vector
// fold 8 integers to 4 then AND these 4 integers
static inline int32_t v256andi(v8si v){
  return v128andi(v256fold_andi (v)) ;
}

// logical OR of the 8 32 bit integers in a 256 bit vector
// fold 8 integers to 4 then OR these 4 integers
static inline int32_t v256ori(v8si v){
  return v128andi(v256fold_andi (v)) ;
}

// integer SUM of the 8 32 bit integers in a 256 bit vector
// fold 8 integers to 4 then SUM these 4 integers
static inline int32_t v256sumi(v8si v){
  return v128sumi(v256fold_sumi (v)) ;
}

// float sum of the 8 32 bit floats in a 256 bit vector
// fold 8 floats to 4 then sum these 4 floats
static inline float v256sumf(v8sf v){
  return v128sumf(v256fold_sumf (v)) ;
}

// comparison functions
static inline v4si v128gti(v4si a, v4si b){
  return a > b ;
}
static inline v4si v128lti(v4si a, v4si b){
  return a < b ;
}
static inline v4si v128eqi(v4si a, v4si b){
  return a == b ;
}
static inline v4sf v128eqf(v4sf a, v4sf b){
  return a == b ;
}

static inline v8si v256gti(v8si a, v8si b){
  return a > b ;
}
static inline v8si v256lti(v8si a, v8si b){
  return a < b ;
}
static inline v8si v256eqi(v8si a, v8si b){
  return a == b ;
}
static inline v8sf v256eqf(v8sf a, v8sf b){
  return a == b ;
}

// set to ZERO
static inline v4si v128zeroi(v4si v){   // 128 bit, integers
  return v ^ v ;
}
static inline v4sf v128zerof(v4sf v){   // 128 bit, floats
  return _mm_xor_ps(v, v) ;
}
static inline v8si v256zeroi(v8si v){   // 256 bit, integers
  return v ^ v ;
}
static inline v8sf v256zerof(v8sf v){   // 256 bit, floats
  return _mm256_xor_ps(v, v) ;
}

// set to ONES ( v == v ) not accepted by some sompilers
static inline v4si v128onesi(v4si v){   // 128 bit, integers
  return v128eqi(v, v) ;
//   return _mm_cmpeq_epi32(v, v) ;
}
static inline v4sf v128onesf(v4sf v){   // 128 bit, floats
  return v128eqf(v, v) ;
//   return _mm_cmp_ps(v, v, _CMP_EQ_OQ) ;
}
static inline v8si v256onesi(v8si v){   // 256 bit, integers
  return v256eqi(v, v) ;
//   return _mm256_cmpeq_epi32(v, v) ;
}
static inline v8sf v256onesf(v8sf v){   // 256 bit, floats
  return v256eqf(v, v) ;
//   return _mm256_cmp_ps(v, v, _CMP_EQ_OQ) ;
}

// logical NOT (not an AVX instructions)
// static inline v4si v128noti(v4si v){
static inline v4si _mm_not_si128(v4si v){
  return ~v ;
}
// static inline v8si v256noti(v8si v){
static inline v8si _mm256_not_si128(v8si v){
  return ~v ;
}

#endif     // __AVX2__

#endif     // v2dil1
