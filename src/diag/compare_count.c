/* 
 * Copyright (C) 2025  Recherche en Prevision Numerique
 *                     Environnement Canada
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation,
 * version 2.1 of the License.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 */
#include <rmn/compare_count.h>

#if defined(__AVX2__) && defined(WITH_SIMD)
// 8 values in ymm register (accum and value)
// 4 values in xmm register (ref4)
// return[l] and output[l+4] added count against same ref[l] value
static __m256i _mm256i_count8_lt(__m256i accum, __m128i ref4, __m256i value){
  __m256i ref44 ;
  ref44 = _mm256_inserti128_si256(ref44, ref4, 0) ;
  ref44 = _mm256_inserti128_si256(ref44, ref4, 1) ;
  __m256i vd0 = value ;
  __m256i vd1 = _mm256_shuffle_epi32(value, 0b00111001 ) ;
  __m256i vd2 = _mm256_shuffle_epi32(value, 0b01001110 ) ;
  __m256i vd3 = _mm256_shuffle_epi32(value, 0b10010011 ) ;

  vd0 = _mm256_cmpgt_epi32(ref44, vd0) ;
  vd1 = _mm256_cmpgt_epi32(ref44, vd1) ;
  vd2 = _mm256_cmpgt_epi32(ref44, vd2) ;
  vd3 = _mm256_cmpgt_epi32(ref44, vd3) ;

  accum = _mm256_sub_epi32(accum, vd0) ;
  accum = _mm256_sub_epi32(accum, vd1) ;
  accum = _mm256_sub_epi32(accum, vd2) ;
  accum = _mm256_sub_epi32(accum, vd3) ;
  return accum ;
}

// 4 values in xmm register
// return[l] added count against ref4[l]
static __m128i _mm_count4_lt(__m128i accum, __m128i ref4, __m128i values){
  __m128i vd0 = values ;
  __m128i vd1 = _mm_shuffle_epi32(values, 0b00111001 ) ;
  __m128i vd2 = _mm_shuffle_epi32(values, 0b01001110 ) ;
  __m128i vd3 = _mm_shuffle_epi32(values, 0b10010011 ) ;

  vd0 = _mm_cmpgt_epi32(ref4, vd0) ;
  vd1 = _mm_cmpgt_epi32(ref4, vd1) ;
  vd2 = _mm_cmpgt_epi32(ref4, vd2) ;
  vd3 = _mm_cmpgt_epi32(ref4, vd3) ;

  accum = _mm_sub_epi32(accum, vd0) ;
  accum = _mm_sub_epi32(accum, vd1) ;
  accum = _mm_sub_epi32(accum, vd2) ;
  accum = _mm_sub_epi32(accum, vd3) ;
  return accum ;
}

// 1, 2, or 3 values
// 4 values in xmm register (accum, ref4)
// values : pointer to array of values to test
// n      : number of values (modulo 3 is takes, n is assumed to be 0/1/2/3)
// return[l] added count against ref4[l]
static __m128i _mm_count123_lt(__m128i accum, __m128i ref4, int *values, int n){
  __m128i vd0 ;
  n &= 3 ;
  if(n > 0){
    vd0 = _mm_set1_epi32(*values) ; values ++ ;
    vd0 = _mm_cmpgt_epi32(ref4, vd0) ;
    accum = _mm_sub_epi32(accum, vd0) ;
    if(n > 1){
      vd0 = _mm_set1_epi32(*values) ; values ++ ;
      vd0 = _mm_cmpgt_epi32(ref4, vd0) ;
      accum = _mm_sub_epi32(accum, vd0) ;
    }
    if(n > 2){
      vd0 = _mm_set1_epi32(*values) ; values ++ ;
      vd0 = _mm_cmpgt_epi32(ref4, vd0) ;
      accum = _mm_sub_epi32(accum, vd0) ;
    }
  }
  return accum ;
}

// return vector [k] == number of values where value[l] < ref4[k]
// values [IN] : input array with values to test
// ref4   [IN] : 4 reference values to test against values[l]
// n      [IN] : dimension of array values
__m128i _mm_count_lt(int *values, int ref4[4], int n){
  __m128i accu4, vref ;
  __m256i accu8, data ;
  accu8 = _mm256_xor_si256(accu8, accu8) ;
  vref  = _mm_loadu_si128((void *) &ref4[0]) ;
  while(n > 7){                // blocks of 8 values
    data = _mm256_loadu_si256((void *) values) ;
    accu8 = _mm256i_count8_lt(accu8, vref, data) ;
    values += 8 ;
    n -= 8 ;
  }
  // fold sums to 128 bits
  accu4 = _mm_add_epi32( _mm256_extracti128_si256(accu8,0) , _mm256_extracti128_si256(accu8,1) ) ;
  if(n > 3){
    accu4 = _mm_count4_lt(accu4, vref, _mm_loadu_si128((void *) values)) ;  // block of 4 values
    values += 4 ;
    n -= 4 ;
  }
  if(n > 0){ accu4 = _mm_count123_lt(accu4, vref, values, n) ; }            // 1, 2, or 3 values
  return accu4 ;
}
#endif

// count[k] == number of values where value[l] < ref4[k]
// count [OUT] : 4 output counts
// values [IN] : input array with values to test
// ref4   [IN] : 4 reference values to test against values[l]
// n      [IN] : dimension of array values
void count_lt(int count[4], int *values, int ref4[4], int n){
#if defined(__AVX2__) && defined(WITH_SIMD)
  _mm_storeu_si128((void *)count, _mm_count_lt(values, ref4, n)) ;
#else
  int i, j ;
  for(j=0 ; j<4 ; j++) count[j] = 0 ;

  for(i=0 ; i<n ; i++){
    int v = values[i] ;
    for(j=0 ; j<4 ; j++){
      count[j] = count[j] - ((v-ref4[j]) >> 31) ; // add 1 or 0
    }
  }
#endif
}
