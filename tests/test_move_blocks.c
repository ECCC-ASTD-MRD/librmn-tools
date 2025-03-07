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
#define NJ  128
#define LNI 129

int main(int argc, char **argv){
  uint32_t z[NJ*2][LNI], r[NJ*2][LNI] ;
  float f1[NJ*2][LNI] ;
  float f2[NJ*2][LNI] ;
  float f3[NJ*2][LNI] ;
  int i, j, ni, nj, errors ;
  block_properties bp ;
  float t0 ;
  TIME_LOOP_DATA ;

  start_of_test("basic test of block move functions") ;

  if(cycles_overhead == 0) cycles_overhead = 1;
  if(argc >= 0){
    start_of_test(argv[0]);
  }
  for(j=0 ; j<NJ ; j++){
    for(i=0 ; i<LNI ; i++){
//       z[j][i] = (i << 8) + j ;
//       z[j][i] |= (i << 31) ;
      z[j][i] = (i - NI/2) + (j - NJ/2) ;
      f2[j][i] = -(i*j+1) ;
      f3[j][i] = (i*j) ;
      if(z[j][i] == 0) z[j][i] = 1 ;
//       f1[j][i] = 1.0f * (i - ni/2) * (j - ni/2) + .5f ;
      f1[j][i] = (j * LNI) + i ;
      f1[j][i] = -f1[j][i] ;
    }
  }
  f1[0][0] = 0.5f ;
//   f1[0][1] = 2.5f ;
  errors = 0 ;
  for(j=0 ; j<NJ ; j++){
    for(i=0 ; i<LNI ; i++){
      int32_t fake = fake_int(f1[j][i]) ;
      float r = unfake_float(fake) ;
      if(r != f1[j][i]) {
        fprintf(stderr, "ERROR: expecting %f1, got %f1, fake = %8.8x\n", f1[j][i], r, fake) ;
        exit(1) ;
      }
    }
  }
  fprintf(stderr, "SUCCESS : float -> fake int -> float test\n") ;

  ni = 127 ; nj = 125 ;
  {
    uint32_t blk[nj][ni] ;

    move_w32_block(&f1[0][0],   LNI, blk, ni, 2*ni/3, 2*nj/3, &bp) ;  // same operation, different syntax
    move_w32_block((float *)f1, LNI, blk, ni, 2*ni/3, 2*nj/3, &bp) ;
    if(argc == 512) {
      void *ff = &f1[0][0] ;
      move_w32_block(&f1[0][0], LNI, blk, ni, 2*ni/3, 2*nj/3, &bp) ;
      move_w32_block(&z[0][0], LNI, blk, ni, ni,     nj,     &bp) ;
      move_w32_block(ff,  LNI, blk, ni, 2*ni/3, 2*nj/3, &bp) ;
    }
    print_float_props(bp) ;

    move_w32_block((float *)f2, LNI, blk, ni, 2*ni/3, 2*nj/3, &bp) ;
    print_float_props(bp) ;

    move_w32_block((float *)f3, LNI, blk, ni, 2*ni/3, 2*nj/3, &bp) ;
    print_float_props(bp) ;

    for(j=0 ; j<1 ; j++){
      for(i=0 ; i<ni ; i++){
        blk[j][i] = 0xFFFF ;
      }
    }
    move_w32_block((uint32_t *) &z[0][0], LNI, blk, ni, ni,     nj,     &bp) ;
    print_int_props(bp) ;

    move_w32_block((int32_t *)  &z[0][0], LNI, blk, ni, ni,     nj,     &bp) ;
    print_int_props(bp) ;

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

//     put_word_block(r, blk, ni, LNI, nj) ;
//     scatter_word32_block(r, blk, ni, LNI, nj) ;
//     move_word32_block(blk, ni, r, LNI, ni, nj, raw_data, NULL) ;
    move_w32_block(&blk[0][0], ni, r, LNI, ni, nj, NULL) ;
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

//     TIME_LOOP_EZ(NITER, ni*nj, get_word_block(z, blk, ni, LNI, nj) ) ;
//     if(timer_min == timer_max) timer_avg = timer_max ;
//     t0 = timer_min * NaNoSeC / (ni*nj) ;
//     fprintf(stderr, "get block      : %4.2f ns/word\n", t0) ;

//     TIME_LOOP_EZ(NITER, ni*nj, gather_int32_block((int32_t *)z, blk, ni, LNI, nj, &bp) ) ;
    TIME_LOOP_EZ(NITER, ni*nj, move_w32_block((int32_t *)z, LNI, blk, ni, ni, nj, &bp) ) ;
    if(timer_min == timer_max) timer_avg = timer_max ;
    t0 = timer_min * NaNoSeC / (ni*nj) ;
    fprintf(stderr, "move int + prop : %4.2f ns/word\n", t0) ;

    TIME_LOOP_EZ(NITER, ni*nj, move_w32_block((int32_t *)z, LNI, z, LNI, ni, nj, &bp) ) ;
    if(timer_min == timer_max) timer_avg = timer_max ;
    t0 = timer_min * NaNoSeC / (ni*nj) ;
    fprintf(stderr, "nomove int prop : %4.2f ns/word\n", t0) ;

    TIME_LOOP_EZ(NITER, ni*nj, move_w32_block((float *)z, LNI, blk, ni, ni, nj, &bp) ) ;
    if(timer_min == timer_max) timer_avg = timer_max ;
    t0 = timer_min * NaNoSeC / (ni*nj) ;
    fprintf(stderr, "move flt + prop : %4.2f ns/word\n", t0) ;

    TIME_LOOP_EZ(NITER, ni*nj, move_w32_block((float *)z, LNI, z, LNI, ni, nj, &bp) ) ;
    if(timer_min == timer_max) timer_avg = timer_max ;
    t0 = timer_min * NaNoSeC / (ni*nj) ;
    fprintf(stderr, "nomove flt prop : %4.2f ns/word\n", t0) ;

    TIME_LOOP_EZ(NITER, ni*nj, move_data32_block(z, LNI, blk, ni, ni, nj, NULL) ) ;
    if(timer_min == timer_max) timer_avg = timer_max ;
    t0 = timer_min * NaNoSeC / (ni*nj) ;
    fprintf(stderr, "move datanoprop : %4.2f ns/word\n", t0) ;

    TIME_LOOP_EZ(NITER, ni*nj, move_mem32_block(z, LNI, blk, ni, ni, nj) ) ;
    if(timer_min == timer_max) timer_avg = timer_max ;
    t0 = timer_min * NaNoSeC / (ni*nj) ;
    fprintf(stderr, "move mem32      : %4.2f ns/word\n", t0) ;

    TIME_LOOP_EZ(NITER, ni*nj, move_w32_block((void *)z, LNI, blk, ni, ni, nj, NULL) ) ;
    if(timer_min == timer_max) timer_avg = timer_max ;
    t0 = timer_min * NaNoSeC / (ni*nj) ;
    fprintf(stderr, "move in         : %4.2f ns/word\n", t0) ;

//     TIME_LOOP_EZ(NITER, ni*nj, put_word_block(z, blk, ni, LNI, nj) ) ;
//     t0 = timer_min * NaNoSeC / (ni*nj) ;
//     fprintf(stderr, "put block      : %4.2f ns/word\n", t0) ;

//     TIME_LOOP_EZ(NITER, ni*nj, scatter_word32_block(z, blk, ni, LNI, nj) ) ;
    TIME_LOOP_EZ(NITER, ni*nj, move_w32_block((void *)blk, ni, r, LNI, ni, nj, NULL) ) ;
    t0 = timer_min * NaNoSeC / (ni*nj) ;
    fprintf(stderr, "move back       : %4.2f ns/word\n", t0) ;

    if(errors > 0){
      for(j=nj-1 ; j>=0 ; j--){
        for(i=0 ; i<ni ; i++){
          fprintf(stderr, "%4.4x ", blk[j][i]) ;
        }
        fprintf(stderr, "\n") ;
      }
    }
  }

//   split_and_process(z, LNI, NI, NJ, int_data, 64, 64, NULL, NULL) ;  // demo mode 

  return 0 ;
}
