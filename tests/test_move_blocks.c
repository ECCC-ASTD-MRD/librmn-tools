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

#define NI  128
#define NJ  128
#define LNI 129

int main(int argc, char **argv){
  uint32_t z[NJ*2][LNI], r[NJ*2][LNI] ;
  int i, j, ni, nj, errors ;
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

  ni = 128 ; nj = 127 ;
  {
    uint32_t blk[nj][ni] ;
    for(j=0 ; j<1 ; j++){
      for(i=0 ; i<ni ; i++){
        blk[j][i] = 0xFFFF ;
      }
    }
    get_word_block(z, blk, ni, LNI, nj) ;
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

    fprintf(stderr, "get block : %4.2f ns/word\n", t0) ;
    TIME_LOOP_EZ(NITER, ni*nj, put_word_block(z, blk, ni, LNI, nj) ) ;
    t0 = timer_min * NaNoSeC / (ni*nj) ;
    fprintf(stderr, "put block : %4.2f ns/word\n", t0) ;
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
