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
#include <rmn/move_blocks.h>

#if defined(SYNTAX_TEST_ON)
#define VERBOSE_SIMD
#define NO_SIMD__

typedef struct{ union{ float   f[8] ; int32_t  i[8] ; uint32_t u[8] ; } ; } V8f ;
// typedef struct{ union{ float   f[8] ; int32_t  i[8] ; uint32_t u[8] ; } ; } V8i ;
typedef V8f V8i ;


V8f _mm256_blendv_ps_(V8f A, V8f B, V8f MASK){
  int i ;
  V8f R ;
  for(i=0 ; i<8 ; i++) R.i[i] = (MASK.i[i] < 0) ? B.i[i] : A.i[i] ;
  return R ;
}
static inline V8i _mm256_xor_si256_( V8i A,  V8i B){
  int i ;
  V8i R ;
  for(i=0 ; i<8 ; i++) R.i[i] = A.i[i] ^ B.i[i] ;
  return R ;
}
static inline V8f _mm256_castsi256_ps_( V8i A){
  int i ;
  V8f R ;
  for(i=0 ; i<8 ; i++) R.i[i] = A.i[i] ;
  return R ;
}

#define BLEND256(A,B,MASK)      (V8I)_mm256_blendv_ps_((V8F)A, (V8F)B, (V8F)MASK)

// syntax test
static void c_and_prop(/*void *s_, void *d_, uint32_t ni, block_properties *bp*/){
//   int32_t *si = (int32_t *) s_ , *s = si ;
//   int32_t *di = (int32_t *) d_ , *d = di ;
//   int n7 = ni & 7 ;
  V8i v0, v1, v2 ; // __m256i vv0, vv1, vv2 ;
  V8f vf, vx, vy, vm ; // __m256 vvf, vvx, vvy, vvm ;
  v0 = _mm256_xor_si256_(v0, v0) ;
  v0 = _mm256_xor_si256_(v1, v2) ;
  v0 = (V8i)_mm256_blendv_ps_((V8f)v0, (V8f)v1, (V8f)v2) ;
  v0 = (V8i)_mm256_blendv_ps_(v0, v1, v2) ;
//   vv0 = (__m256i) _mm256_blendv_ps((__m256) vv0, vvy, vvm) ;
  vf = _mm256_blendv_ps_(vx, vy, vm) ;
  vf = _mm256_blendv_ps_(_mm256_castsi256_ps_(v0), _mm256_castsi256_ps_(v1), _mm256_castsi256_ps_(v2)) ;
}
#endif

#define NITER 100
#define WITH_TIMING

#if defined(WITH_TIMING)
#include <rmn/timers.h>
#else
#define TIME_LOOP_DATA ;
#define TIME_LOOP_EZ(niter, npts, code) ;
  char *timer_msg = "" ;
  float NaNoSeC = 0.0f ;
#endif

#define NI  127
#define NJ  131
#define LNI 129

int main(int argc, char **argv){
  uint32_t z[NJ*2][LNI], r[NJ*2][LNI] ;
  int i, j, ni, nj, errors ;
  block_properties bp ;
  float t0 ;
  TIME_LOOP_DATA ;

  if(cycles_overhead == 0) cycles_overhead = 1;
  if(argc >= 0){
    start_of_test(argv[0]);
  }
  for(j=0 ; j<NJ ; j++){
    for(i=0 ; i<LNI ; i++){
      z[j][i] = (i << 8) + j ;
    }
  }
//   fprintf(stderr, "original array \n") ;
//   for(j=nj-1 ; j>=0 ; j--){
//     for(i=0 ; i<ni ; i++){
//       fprintf(stderr, "%4.4x ", z[j][i]) ;
//     }
//     fprintf(stderr, "\n") ;
//   }
//   fprintf(stderr, "\n") ;

  ni = 127 ; nj = 125 ;
  {
    uint32_t blk[nj][ni] ;
    for(j=0 ; j<1 ; j++){
      for(i=0 ; i<ni ; i++){
        blk[j][i] = 0xFFFF ;
      }
    }
    get_word_block(z, blk, ni, LNI, nj) ;
//     gather_word32_block(z, blk, ni, LNI, nj) ;
//     gather_int32_block((int32_t *)z, blk, ni, LNI, nj, &bp) ;
    errors = 0 ;
    for(j=0 ; j<nj ; j++){
      for(i=0 ; i<ni ; i++){
        if(blk[j][i] != z[j][i]) {
          if(errors == 0) fprintf(stderr, "(%d,%d) expected %4.4x, got %4.4x\n", i, j, z[j][i], blk[j][i]) ;
          errors++ ;
        }
      }
    }
    fprintf(stderr, "get block errors = %d [%dx%d]\n", errors, ni, nj) ;

    put_word_block(r, blk, ni, LNI, nj) ;
//     scatter_word32_block(r, blk, ni, LNI, nj) ;
    errors = 0 ;
    for(j=0 ; j<nj ; j++){
      for(i=0 ; i<ni ; i++){
        if(blk[j][i] != r[j][i]) {
          if(errors == 0) fprintf(stderr, "(%d,%d) expected %4.4x, got %4.4x\n", i, j, z[j][i], blk[j][i]) ;
          errors++ ;
        }
      }
    }
    fprintf(stderr, "put block errors = %d [%dx%d]\n", errors, ni, nj) ;

    TIME_LOOP_EZ(NITER, ni*nj, get_word_block(z, blk, ni, LNI, nj) ) ;
    if(timer_min == timer_max) timer_avg = timer_max ;
    t0 = timer_min * NaNoSeC / (ni*nj) ;
    fprintf(stderr, "get block     : %4.2f ns/word\n", t0) ;

//     TIME_LOOP_EZ(NITER, ni*nj, gather_int32_block((int32_t *)z, blk, ni, LNI, nj, &bp) ) ;
//     if(timer_min == timer_max) timer_avg = timer_max ;
//     t0 = timer_min * NaNoSeC / (ni*nj) ;
//     fprintf(stderr, "gather + prop : %4.2f ns/word\n", t0) ;

    TIME_LOOP_EZ(NITER, ni*nj, put_word_block(z, blk, ni, LNI, nj) ) ;
    t0 = timer_min * NaNoSeC / (ni*nj) ;
    fprintf(stderr, "put block     : %4.2f ns/word\n", t0) ;

//     TIME_LOOP_EZ(NITER, ni*nj, scatter_word32_block(z, blk, ni, LNI, nj) ) ;
//     t0 = timer_min * NaNoSeC / (ni*nj) ;
//     fprintf(stderr, "scatter block : %4.2f ns/word\n", t0) ;

    if(errors > 0){
      for(j=nj-1 ; j>=0 ; j--){
        for(i=0 ; i<ni ; i++){
          fprintf(stderr, "%4.4x ", blk[j][i]) ;
        }
        fprintf(stderr, "\n") ;
      }
    }
  }
  return 0 ;
}
