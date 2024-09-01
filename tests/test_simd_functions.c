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
#define NO_SIMD_
#define VERBOSE_SIMD
#include <rmn/simd_functions.h>

int main(int argc, char **argv){
  __m128i v4ia, v4ib ;
  __m256i v8ia, v8ib ;
  __m128  v4fa ;
  __m256  v8fa ;
  int i ;

  if(argc >= 0){
    start_of_test(argv[0]);
  }

  v8ia = _mm256_set1_epi64x(0x0807060504030201lu) ; _mm256_print_epu8("v8ia", v8ia) ;
  v4ia = _mm_set1_epi64x(   0x0807060504030201lu) ; _mm_print_epu8("v4ia", v4ia) ;
  v4ia = _mm_set1_epi64x(   0x0807060504030201lu) ; _mm_print_epu64("v4ia", v4ia) ;
  v4ib = _mm_set1_epi64x(   0xF8F7F6F5F4F3F2F1lu) ; _mm_print_epu8("v4ib", v4ib) ;
  v4ib = _mm_set1_epi64x(   0xF8F7F6F5F4F3F2F1lu) ; _mm_print_epu64("v4ib", v4ib) ;
  v8ia = _mm256_cvtepi8_epi32(v4ia) ;               _mm256_print_epu32("v8ia", v8ia) ;
  v4ia = _mm_cvtepi8_epi32(v4ia) ;                  _mm_print_epu32("v4ia", v4ia) ;
  v4ib = _mm_cvtepi8_epi32(v4ib) ;                  _mm_print_epu32("v4ib", v4ib) ;

  v4fa = _mm_set1_ps(1.23456f) ;                    _mm_print_ps("v4fa", v4fa) ;
}
