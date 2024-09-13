//
// Copyright (C) 2024  Environnement Canada
//
// This is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation,
// version 2.1 of the License.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details .
//
// Author:
//     M. Valin,   Recherche en Prevision Numerique, 2024
//
// test the memory block movers
//
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <rmn/test_helpers.h>
// #define NO_SIMD
#define VERBOSE_SIMD
#define ALIAS_INTEL_SIMD_INTRINSICS
#define USE_INTEL_SIMD_INTRINSICS_
#include <rmn/simd_functions.h>

static uint8_t cm[32], ca[32], cb[32] ;
static uint32_t ia[8] = { 0x01,  0x02, 0x03,  0x04,  0x05,  0x06, 0x07,  0x08 } ;
static uint32_t ib[8] = { 0x11,  0x12, 0x13,  0x14,  0x15,  0x16, 0x17,  0x18 } ;
static uint32_t im[8] = {    0, 1<<31,    0, 1<<31,     0, 1<<31,    0, 1<<31 } ;
static uint32_t i0[8] = {    0,     0,    0,     0,     0,     0,    0,     0 } ;
static uint32_t ra[8] ;

int main(int argc, char **argv){
  __v128i v4ia, v4ib, v4ca, v4cb, v4cm, v4cr, v400, v411, v4ra, v4ma ;
  __v256i v8ia, v8ib, v8ca, v8cb, v8cm, v8cr, v800, v811, v8ra, v8ma ;
  __v128  v4fa, v4da ;
  __v256  v8fa, v8da ;
  int i ;

  if(argc >= 0){
    start_of_test(argv[0]);
  }

  fprintf(stderr, "- constants\n");
  v400 = _mm_xor_si128(v400, v400)    ; v411 = _mm_cmpeq_epi32(v400, v400) ;
  _mm_print_epu32("v400", v400) ; _mm_print_epu32("v411", v411) ;
  v800 = _mm256_xor_si256(v800, v800) ; v811 = _mm256_cmpeq_epi32(v800, v800) ;
  _mm256_print_epu32("v800", v800) ; _mm256_print_epu32("v811", v811) ;

  fprintf(stderr, " set1_v4l(8) set1_v2l(8/64/8/64)\n");
  v8ia = set1_v4l(0x0807060504030201lu) ; print_v32c("v8ia", v8ia) ;
  v4ia = set1_v2l(0x0807060504030201lu) ; print_v16c("v4ia", v4ia) ;
  v4ia = set1_v2l(0x0807060504030201lu) ; print_v2l("v4ia", v4ia) ;
  v4ib = set1_v2l(0xF8F7F6F5F4F3F2F1lu) ; print_v16c("v4ib", v4ib) ;
  v4ib = set1_v2l(0xF8F7F6F5F4F3F2F1lu) ; _mm_print_epu64("v4ib", v4ib) ;

  fprintf(stderr, "- cvt_v8c_v8i cvt_v4c_v4i cvt_v4c_v4i\n");
  v8ia = cvt_v8c_v8i(v4ia) ; print_v8i("v8ia", v8ia) ;
  v4ia = cvt_v4c_v4i(v4ia) ; print_v4i("v4ia", v4ia) ;
  v4ib = cvt_v4c_v4i(v4ib) ; print_v4i("v4ib", v4ib) ;

  fprintf(stderr, "- set1_v4f set1_v2d set1_v8f set1_v4d\n");
  v4fa = set1_v4f(-1.23456f) ;   _mm_print_ps("v4fa", v4fa) ;
  v4da = set1_v2d(-2.3456789) ;  print_v2d("v4da", v4da) ;
  v8fa = set1_v8f(1.23456f) ;    print_v8f("v8fa", v8fa) ;
  v8da = set1_v4d(2.3456789) ;   _mm256_print_pd("v8da", v8da) ;

  fprintf(stderr, "- maskload_v4i(3) maskload_v8i(5/3) maskstore_v4i(3) maskstore_v8i(5/3) maskstore_v4i(3/2)\n");
  v4ra = maskload_v4i((int *)ia, mask_v4i(3)) ; print_v4i("v4ra", v4ra) ;
  v8ra = maskload_v8i((int *)ia, mask_v8i(5)) ; print_v8i("v8ra", v8ra) ;
  v8ra = maskload_v8i((int *)ia, mask_v8i(3)) ; print_v8i("v8ra", v8ra) ;
  storeu_v128((__m128i *)ra, v4ia) ; v4ra = loadu_v128((__m128i *)ra) ; _mm_print_epu32("v4ra0", v4ra) ;
  maskstore_v4i((int *)ra, mask_v4i(3), v411) ; v4ra = loadu_v128((__m128i *)ra) ; _mm_print_epu32("v4ra1", v4ra) ;
  maskstore_v4i((int *)ra, mask_v4i(2), v400) ; v4ra = loadu_v128((__m128i *)ra) ; _mm_print_epu32("v4ra2", v4ra) ;
  storeu_v256((__m256i *)ra, v8ia) ; v8ra = loadu_v256((__m256i *)ra) ; _mm256_print_epu32("v8ra0", v8ra) ;
  maskstore_v8i((int *)ra, mask_v8i(5), v811) ; v8ra = loadu_v256((__m256i *)ra) ; _mm256_print_epu32("v8ra1", v8ra) ;
  maskstore_v8i((int *)ra, mask_v8i(3), v800) ; v8ra = loadu_v256((__m256i *)ra) ; _mm256_print_epu32("v8ra2", v8ra) ;

  fprintf(stderr, "- insert_128 (v400 -> v811 upper)\n");
  _mm256_print_epu32("v811", v811) ; _mm_print_epu32("v400", v400) ;
  v8ra = inserti_128(v811, v400, 1) ;
  _mm256_print_epu32("v8ra", v8ra) ;

  fprintf(stderr, "- insert_128 (v400 -> v811 lower)\n");
  _mm256_print_epu32("v811", v811) ; _mm_print_epu32("v400", v400) ;
  v8ra = inserti_128(v811, v400, 0) ;
  _mm256_print_epu32("v8ra", v8ra) ;

  return 0 ;

//   for(i=0 ; i<16 ; i++) { ca[i] = i ; cb[i] = ca[i] + 0x10 ; cm[i] = (i & 1) ? 0xFF : 0x00 ; }
  for(i=0 ; i<32 ; i++) { ca[i] = i ; cb[i] = ca[i] + 0x40 ; cm[i] = (i & 1) ? 0xFF : 0x00 ; }
  v4ca = loadu_v128( (__m128i *) ca) ; v4cb = loadu_v128( (__m128i *) cb) ; v4cm = loadu_v128( (__m128i *) cm) ;
  fprintf(stderr, "- _mm_blendv_epi8\n");
  v4cr = _mm_blendv_epi8(v4ca, v4cb, v4cm) ;
  _mm_print_epu8("v4ca", v4ca) ; _mm_print_epu8("v4cb", v4cb) ; _mm_print_epu8("v4cm", v4cm) ; _mm_print_epu8("v4cr", v4cr) ;
  v4ca = loadu_v128( (__m128i *) ca) ;    v4cb = loadu_v128( (__m128i *) cb) ;    v4cm = loadu_v128( (__m128i *) cm) ;
  fprintf(stderr, "- _mm_blendv_ps\n");
  v4cr = __V128i _mm_blendv_ps(__V128 v4ca, __V128 v4cb, __V128 v4cm) ;
  _mm_print_epu32("v4ca", v4ca) ; _mm_print_epu32("v4cb", v4cb) ; _mm_print_epu32("v4cm", v4cm) ; _mm_print_epu32("v4cr", v4cr) ;
  fprintf(stderr, "- _mm_blendv_epi32\n");
  v4cm = loadu_v128( (__m128i *) im) ;
  v4cr = _mm_blendv_epi32(v4ca, v4cb, v4cm) ;
  _mm_print_epu32("v4ca", v4ca) ; _mm_print_epu32("v4cb", v4cb) ; _mm_print_epu32("v4cm", v4cm) ; _mm_print_epu32("v4cr", v4cr) ;

  v8ca = loadu_v256( (__m256i *) ia) ; v8cb = loadu_v256( (__m256i *) ib) ; v8cm = loadu_v256( (__m256i *) im) ;
  fprintf(stderr, "- _mm256_blendv_ps\n");
  v8cr = __V256i _mm256_blendv_ps(__V256 v8ca, __V256 v8cb, __V256 v8cm) ;
  _mm256_print_epu32("v8ca", v8ca) ; _mm256_print_epu32("v8cb", v8cb) ; _mm256_print_epu32("v8cm", v8cm) ; _mm256_print_epu32("v8cr", v8cr) ;
  fprintf(stderr, "- _mm256_blendv_epi32\n");
  v8cr = _mm256_blendv_epi32(v8ca, v8cb, v8cm) ;
  _mm256_print_epu32("v8ca", v8ca) ; _mm256_print_epu32("v8cb", v8cb) ; _mm256_print_epu32("v8cm", v8cm) ; _mm256_print_epu32("v8cr", v8cr) ;

  fprintf(stderr, " _mm256_cmpeq_epi32 (v800 == v8cm)\n");
  _mm256_print_epu32("v8cm", v8cm) ;
  v8ra = _mm256_cmpeq_epi32(v800, v8cm) ; _mm256_print_epu32("v8ra", v8ra) ;

  fprintf(stderr, "- _mm_cmpeq_epi32 (v400 == v4cm)\n");
  _mm_print_epu32("v4cm", v4cm) ;
  v4ra = _mm_cmpeq_epi32(v400, v4cm) ; _mm_print_epu32("v4ra", v4ra) ;

  fprintf(stderr, "- _mm256_add_epi32 (v8ca + v8cb)\n");
  v8ra = _mm256_add_epi32(v8ca, v8cb) ; _mm256_print_epu32("v8ra", v8ra) ;

  fprintf(stderr, "- _mm_sub_epi32 (v4cb - v4ca)\n");
  v4ra = _mm_sub_epi32(v4cb, v4ca) ; _mm_print_epu32("v4ra", v4ra) ;
  v4ra = _mm_add_epi32(v4ra, v4ca) ; _mm_print_epu32("v4ra", v4ra) ;

  fprintf(stderr, " max_v4u (v4cb , v4ca)\n");
  v4ra = max_v4u(v4ca, v4cb) ;
  _mm_print_epu32("v4ra", v4ra) ;

  fprintf(stderr, "- min_v4i (v4cb , v4ca)\n");
  v4ra = min_v4i(v4ca, v4cb) ;
  _mm_print_epu32("v4ra", v4ra) ;

  fprintf(stderr, "- abs_v8i (v811)\n");
  v8ra = abs_v8i(v811) ; _mm256_print_epu32("v8ra", v8ra) ;
  fprintf(stderr, "- abs_v4i(-v4cr)\n");
  v4ra = _mm_sub_epi32(v400, v4cr) ; _mm_print_epu32("v4cr", v4cr) ; _mm_print_epu32("    ", v4ra) ;
  v4ra = abs_v4i(v4ra) ; _mm_print_epu32("v4ra", v4ra) ;  _mm_print_epu32("O.K.", _mm_cmpeq_epi32(v4ra, v4cr)) ;

  fprintf(stderr, "- _mm256_castsi128_si256 (v400 -> v811)\n");
  v8ra = v811 ;
  _mm256_print_epu32("v8ra", v8ra) ;
  v8ra = _mm256_castsi128_si256(v400) ;
  _mm256_print_epu32("v8ra", v8ra) ;
  fprintf(stderr, "- _mm256_castsi256_si128 (v8ra -> v4ra)\n");
  _mm_print_epu32("v4ra", v4ra) ;
  v4ra = _mm256_castsi256_si128(v8ra) ;
  _mm_print_epu32("v4ra", v4ra) ;

  return 0 ;
}
