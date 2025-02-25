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
// return[l] and return[l+4] : accum[l|l+4] + number of values == ref4[l] (ymm register)
static __v256i count8_eq(__v256i accum, __v128i ref4, __v256i value){
  __v256i ref44 ;
  ref44 = inserti_128(accum, ref4, 0) ;             // phony operation to avoid a warning
  ref44 = inserti_128(ref44, ref4, 1) ;             // [0] [1] [2] [3] [0] [1] [2] [3]
  __v256i vd0 = value ;                             // [0] [1] [2] [3] [4] [5] [6] [7]
  __v256i vd1 = shuffle_v8i(value, 0b00111001 ) ;   // [1] [2] [3] [0] [5] [6] [7] [4]
  __v256i vd2 = shuffle_v8i(value, 0b01001110 ) ;   // [2] [3] [0] [1] [6] [7] [4] [5]
  __v256i vd3 = shuffle_v8i(value, 0b10010011 ) ;   // [3] [0] [1] [2] [7] [4] [5] [6]

  vd0 = cmpeq_v8i(vd0, ref44) ;                     // compare to reference
  vd1 = cmpeq_v8i(vd1, ref44) ;
  vd2 = cmpeq_v8i(vd2, ref44) ;
  vd3 = cmpeq_v8i(vd3, ref44) ;

  accum = sub_v8i(accum, vd0) ;                     // add counts to accumulator
  accum = sub_v8i(accum, vd1) ;
  accum = sub_v8i(accum, vd2) ;
  accum = sub_v8i(accum, vd3) ;
  return accum ;
}

// 8 values in ymm register (accum and value)
// 4 values in xmm register (ref4)
// return[l] and return[l+4] : accum[l|l+4] + number of values > ref4[l] (ymm register)
static __v256i count8_gt(__v256i accum, __v128i ref4, __v256i value){
  __v256i ref44 ;
  ref44 = inserti_128(accum, ref4, 0) ;             // phony operation to avoid a warning
  ref44 = inserti_128(ref44, ref4, 1) ;             // [0] [1] [2] [3] [0] [1] [2] [3]
  __v256i vd0 = value ;                             // [0] [1] [2] [3] [4] [5] [6] [7]
  __v256i vd1 = shuffle_v8i(value, 0b00111001 ) ;   // [1] [2] [3] [0] [5] [6] [7] [4]
  __v256i vd2 = shuffle_v8i(value, 0b01001110 ) ;   // [2] [3] [0] [1] [6] [7] [4] [5]
  __v256i vd3 = shuffle_v8i(value, 0b10010011 ) ;   // [3] [0] [1] [2] [7] [4] [5] [6]

  vd0 = cmpgt_v8i(vd0, ref44) ;                     // compare to reference
  vd1 = cmpgt_v8i(vd1, ref44) ;
  vd2 = cmpgt_v8i(vd2, ref44) ;
  vd3 = cmpgt_v8i(vd3, ref44) ;

  accum = sub_v8i(accum, vd0) ;                     // add counts to accumulator
  accum = sub_v8i(accum, vd1) ;
  accum = sub_v8i(accum, vd2) ;
  accum = sub_v8i(accum, vd3) ;
  return accum ;
}

// 8 values in ymm register (accum and value)
// 4 values in xmm register (ref4)
// return[l] and return[l+4] : accum[l|l+4] + number of values < ref4[l] (ymm register)
static __v256i count8_lt(__v256i accum, __v128i ref4, __v256i value){
  __v256i ref44 ;
  ref44 = inserti_128(accum, ref4, 0) ;             // phony operation to avoid a warning
  ref44 = inserti_128(ref44, ref4, 1) ;             // [0] [1] [2] [3] [0] [1] [2] [3]
  __v256i vd0 = value ;                             // [0] [1] [2] [3] [4] [5] [6] [7]
  __v256i vd1 = shuffle_v8i(value, 0b00111001 ) ;   // [1] [2] [3] [0] [5] [6] [7] [4]
  __v256i vd2 = shuffle_v8i(value, 0b01001110 ) ;   // [2] [3] [0] [1] [6] [7] [4] [5]
  __v256i vd3 = shuffle_v8i(value, 0b10010011 ) ;   // [3] [0] [1] [2] [7] [4] [5] [6]

  vd0 = cmpgt_v8i(ref44, vd0) ;                     // compare to reference
  vd1 = cmpgt_v8i(ref44, vd1) ;
  vd2 = cmpgt_v8i(ref44, vd2) ;
  vd3 = cmpgt_v8i(ref44, vd3) ;

  accum = sub_v8i(accum, vd0) ;                     // add counts to accumulator
  accum = sub_v8i(accum, vd1) ;
  accum = sub_v8i(accum, vd2) ;
  accum = sub_v8i(accum, vd3) ;
  return accum ;
}

// 4 values in xmm register
// return[l] : accum[l] + number of values == ref4[l] (xmm register)
static __v128i count4_eq(__v128i accum, __v128i ref4, __v128i values){
  __v128i vd0 = values ;                           // [0] [1] [2] [3]
  __v128i vd1 = shuffle_v4i(values, 0b00111001 ) ; // [1] [2] [3] [0]
  __v128i vd2 = shuffle_v4i(values, 0b01001110 ) ; // [2] [3] [0] [1]
  __v128i vd3 = shuffle_v4i(values, 0b10010011 ) ; // [3] [0] [1] [2]

  vd0 = cmpeq_v4i(vd0, ref4) ;                     // compare to reference
  vd1 = cmpeq_v4i(vd1, ref4) ;
  vd2 = cmpeq_v4i(vd2, ref4) ;
  vd3 = cmpeq_v4i(vd3, ref4) ;

  accum = sub_v4i(accum, vd0) ;                    // add counts to accumulator
  accum = sub_v4i(accum, vd1) ;
  accum = sub_v4i(accum, vd2) ;
  accum = sub_v4i(accum, vd3) ;
  return accum ;
}

// 4 values in xmm register
// return[l] : accum[l] + number of values > ref4[l] (xmm register)
static __v128i count4_gt(__v128i accum, __v128i ref4, __v128i values){
  __v128i vd0 = values ;                           // [0] [1] [2] [3]
  __v128i vd1 = shuffle_v4i(values, 0b00111001 ) ; // [1] [2] [3] [0]
  __v128i vd2 = shuffle_v4i(values, 0b01001110 ) ; // [2] [3] [0] [1]
  __v128i vd3 = shuffle_v4i(values, 0b10010011 ) ; // [3] [0] [1] [2]

  vd0 = cmpgt_v4i(vd0, ref4) ;                     // compare to reference
  vd1 = cmpgt_v4i(vd1, ref4) ;
  vd2 = cmpgt_v4i(vd2, ref4) ;
  vd3 = cmpgt_v4i(vd3, ref4) ;

  accum = sub_v4i(accum, vd0) ;                    // add counts to accumulator
  accum = sub_v4i(accum, vd1) ;
  accum = sub_v4i(accum, vd2) ;
  accum = sub_v4i(accum, vd3) ;
  return accum ;
}

// 4 values in xmm register
// return[l] : accum[l] + number of values < ref4[l] (xmm register)
static __v128i count4_lt(__v128i accum, __v128i ref4, __v128i values){
  __v128i vd0 = values ;                           // [0] [1] [2] [3]
  __v128i vd1 = shuffle_v4i(values, 0b00111001 ) ; // [1] [2] [3] [0]
  __v128i vd2 = shuffle_v4i(values, 0b01001110 ) ; // [2] [3] [0] [1]
  __v128i vd3 = shuffle_v4i(values, 0b10010011 ) ; // [3] [0] [1] [2]

  vd0 = cmpgt_v4i(ref4, vd0) ;                     // compare to reference
  vd1 = cmpgt_v4i(ref4, vd1) ;
  vd2 = cmpgt_v4i(ref4, vd2) ;
  vd3 = cmpgt_v4i(ref4, vd3) ;

  accum = sub_v4i(accum, vd0) ;                    // add counts to accumulator
  accum = sub_v4i(accum, vd1) ;
  accum = sub_v4i(accum, vd2) ;
  accum = sub_v4i(accum, vd3) ;
  return accum ;
}

// 1, 2, or 3 values
// 4 values in xmm register (accum, ref4)
// values : pointer to array of values to test
// n      : number of values (modulo 3 is taken, n is assumed to be 0/1/2/3)
// return[l] : accum[l] + number of values == ref4[l] (xmm register)
static __v128i count123_eq(__v128i accum, __v128i ref4, int *values, int n){
  __v128i vd0 ;
  n &= 3 ;
  if(n > 1){   // 2 or 3
    vd0   = set1_v4i(*values) ; values ++ ;
    vd0   = cmpeq_v4i(vd0, ref4) ;
    accum = sub_v4i(accum, vd0) ;
    vd0   = set1_v4i(*values) ; values ++ ;
    vd0   = cmpeq_v4i(vd0, ref4) ;
    accum = sub_v4i(accum, vd0) ;
    n -= 2 ;
  }
  if(n > 0){   // 1
    vd0   = set1_v4i(*values) ;
    vd0   = cmpeq_v4i(vd0, ref4) ;
    accum = sub_v4i(accum, vd0) ;
  }
  return accum ;
}

// 1, 2, or 3 values
// 4 values in xmm register (accum, ref4)
// values : pointer to array of values to test
// n      : number of values (modulo 3 is taken, n is assumed to be 0/1/2/3)
// return[l] : accum[l] + number of values > ref4[l] (xmm register)
static __v128i count123_gt(__v128i accum, __v128i ref4, int *values, int n){
  __v128i vd0 ;
  n &= 3 ;
  if(n > 1){   // 2 or 3
    vd0   = set1_v4i(*values) ; values ++ ;
    vd0   = cmpgt_v4i(vd0, ref4) ;
    accum = sub_v4i(accum, vd0) ;
    vd0   = set1_v4i(*values) ; values ++ ;
    vd0   = cmpgt_v4i(vd0, ref4) ;
    accum = sub_v4i(accum, vd0) ;
    n -= 2 ;
  }
  if(n > 0){   // 1
    vd0 = set1_v4i(*values) ;
    vd0 = cmpgt_v4i(vd0, ref4) ;
    accum = sub_v4i(accum, vd0) ;
  }
  return accum ;
}

// 1, 2, or 3 values
// 4 values in xmm register (accum, ref4)
// values : pointer to array of values to test
// n      : number of values (modulo 3 is taken, n is assumed to be 0/1/2/3)
// return[l] : accum[l] + number of values < ref4[l] (xmm register)
static __v128i count123_lt(__v128i accum, __v128i ref4, int *values, int n){
  __v128i vd0 ;
  n &= 3 ;
  if(n > 1){   // 2 or 3
    vd0   = set1_v4i(*values) ; values ++ ;
    vd0   = cmpgt_v4i(ref4, vd0) ;
    accum = sub_v4i(accum, vd0) ;
    vd0   = set1_v4i(*values) ; values ++ ;
    vd0   = cmpgt_v4i(ref4, vd0) ;
    accum = sub_v4i(accum, vd0) ;
    n -= 2 ;
  }
  if(n > 0){   // 1
    vd0   = set1_v4i(*values) ;
    vd0   = cmpgt_v4i(ref4, vd0) ;
    accum = sub_v4i(accum, vd0) ;
  }
  return accum ;
}

// return[k] : number of values == ref4[k]
// values [IN] : input array with values to test
// ref4   [IN] : 4 reference values to test against values[l]
// n      [IN] : dimension of array values
static __v128i count_eq_v4i(int *values, int ref4[4], int n){
  __v128i accu4, vref ;
  __v256i accu8, data ;
//   accu8 = _mm256_xor_si256(accu8, accu8) ;
  accu8 = set1_v8i(0) ;
  vref  = loadu_v128((void *) &ref4[0]) ;
  while(n > 7){                // blocks of 8 values
    data  = loadu_v256((void *) values) ;
    accu8 = count8_eq(accu8, vref, data) ;
    values += 8 ;
    n -= 8 ;
  }
  // fold sums to 128 bits
  accu4 = add_v4i( _mm256_extracti128_si256(accu8,0) , _mm256_extracti128_si256(accu8,1) ) ;
  if(n > 3){
    accu4 = count4_eq(accu4, vref, loadu_v128((void *) values)) ;  // block of 4 values
    values += 4 ;
    n -= 4 ;
  }
  if(n > 0){ accu4 = count123_eq(accu4, vref, values, n) ; }            // 1, 2, or 3 values
  return accu4 ;
}

// return[k] : number of values > ref4[k]
// values [IN] : input array with values to test
// ref4   [IN] : 4 reference values to test against values[l]
// n      [IN] : dimension of array values
static __v128i count_gt_v4i(int *values, int ref4[4], int n){
  __v128i accu4, vref ;
  __v256i accu8, data ;
//   accu8 = _mm256_xor_si256(accu8, accu8) ;
  accu8 = set1_v8i(0) ;
  vref  = loadu_v128((void *) &ref4[0]) ;
  while(n > 7){                // blocks of 8 values
    data = loadu_v256((void *) values) ;
    accu8 = count8_gt(accu8, vref, data) ;
    values += 8 ;
    n -= 8 ;
  }
  // fold sums to 128 bits
  accu4 = add_v4i( _mm256_extracti128_si256(accu8,0) , _mm256_extracti128_si256(accu8,1) ) ;
  if(n > 3){
    accu4 = count4_gt(accu4, vref, loadu_v128((void *) values)) ;  // block of 4 values
    values += 4 ;
    n -= 4 ;
  }
  if(n > 0){ accu4 = count123_gt(accu4, vref, values, n) ; }            // 1, 2, or 3 values
  return accu4 ;
}

// return[k] : number of values < ref4[k]
// values [IN] : input array with values to test
// ref4   [IN] : 4 reference values to test against values[l]
// n      [IN] : dimension of array values
static __v128i count_lt_v4i(int *values, int ref4[4], int n){
  __v128i accu4, vref ;
  __v256i accu8, data ;
//   accu8 = _mm256_xor_si256(accu8, accu8) ;
  accu8 = set1_v8i(0) ;
  vref  = loadu_v128((void *) &ref4[0]) ;
  while(n > 7){                // blocks of 8 values
    data = loadu_v256((void *) values) ;
    accu8 = count8_lt(accu8, vref, data) ;
    values += 8 ;
    n -= 8 ;
  }
  // fold sums to 128 bits
  accu4 = add_v4i( _mm256_extracti128_si256(accu8,0) , _mm256_extracti128_si256(accu8,1) ) ;
  if(n > 3){
    accu4 = count4_lt(accu4, vref, loadu_v128((void *) values)) ;  // block of 4 values
    values += 4 ;
    n -= 4 ;
  }
  if(n > 0){ accu4 = count123_lt(accu4, vref, values, n) ; }            // 1, 2, or 3 values
  return accu4 ;
}
#endif

// count[k] == number of values where value[l] == ref4[k]
// count [OUT] : 4 output counts
// values [IN] : input array with values to test
// ref4   [IN] : 4 reference values to test against values[l]
// n      [IN] : dimension of array values
void count_eq(int count[4], int *values, int ref4[4], int n){
#if defined(__AVX2__) && defined(WITH_SIMD)
  storeu_v128((void *)count, count_eq_v4i(values, ref4, n)) ;
#else
  int i, j ;
  for(j=0 ; j<4 ; j++) count[j] = 0 ;

  for(i=0 ; i<n ; i++){
    int v = values[i] ;
    for(j=0 ; j<4 ; j++){
      count[j] = count[j] - ((ref4[j] == v) ? -1 : 0) ; // add 1 or 0
    }
  }
#endif
}

// count[k] == number of values where value[l] > ref4[k]
// count [OUT] : 4 output counts
// values [IN] : input array with values to test
// ref4   [IN] : 4 reference values to test against values[l]
// n      [IN] : dimension of array values
void count_gt(int count[4], int *values, int ref4[4], int n){
#if defined(__AVX2__) && defined(WITH_SIMD)
  storeu_v128((void *)count, count_gt_v4i(values, ref4, n)) ;
#else
  int i, j ;
  for(j=0 ; j<4 ; j++) count[j] = 0 ;

  for(i=0 ; i<n ; i++){
    int v = values[i] ;
    for(j=0 ; j<4 ; j++){
      count[j] = count[j] - ((ref4[j]-v) >> 31) ; // add 1 or 0
    }
  }
#endif
}

// count[k] == number of values where value[l] < ref4[k]
// count [OUT] : 4 output counts
// values [IN] : input array with values to test
// ref4   [IN] : 4 reference values to test against values[l]
// n      [IN] : dimension of array values
void count_lt(int count[4], int *values, int ref4[4], int n){
#if defined(__AVX2__) && defined(WITH_SIMD)
  storeu_v128((void *)count, count_lt_v4i(values, ref4, n)) ;
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

// count[k] == number of values where value[l] != ref4[k]
void count_ne(int count[4], int *values, int ref4[4], int n){
  int i ;
  count_eq(count, values, ref4, n) ;               // count for condition NOT TRUE
  for(i=0 ; i<4 ; i++) count[i] = n - count[i] ;   // invert the count
}

// count[k] == number of values where value[l] <= ref4[k]
void count_le(int count[4], int *values, int ref4[4], int n){
  int i ;
  count_gt(count, values, ref4, n) ;               // count for condition NOT TRUE
  for(i=0 ; i<4 ; i++) count[i] = n - count[i] ;   // invert the count
}

// count[k] == number of values where value[l] >= ref4[k]
void count_ge(int count[4], int *values, int ref4[4], int n){
  int i ;
  count_lt(count, values, ref4, n) ;               // count for condition NOT TRUE
  for(i=0 ; i<4 ; i++) count[i] = n - count[i] ;   // invert the count
}
