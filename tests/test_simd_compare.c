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
#include <stdio.h>
#include <stdlib.h>

#include <rmn/simd_compare.h>

#define WITH_TIMING

#define NITER 100
#if defined(WITH_TIMING)
#include <rmn/timers.h>
#else
#define TIME_LOOP_DATA ;
#define TIME_LOOP_EZ(niter, npts, code) ;
  char *timer_msg = "" ;
  float NaNoSeC = 0.0f ;
#endif

// multiple of 8 + 7
#define NPTS 4097
#define BITS 12
#define MASK 0xFFF

int main(int argc, char **argv){
  TIME_LOOP_DATA ;
  int32_t array[NPTS] ;
  int32_t count[16] ;
  int32_t ref[] = { 1, 4, 8, 16, 32, 64, 128 } ;
  int32_t bit[] = { 1, 2, 3,  4,  5,  6,   7 } ;
  int i, imax ;
  float xrand ;

  srandom( (uint64_t)(&count) & 0xFFFFFFFFu ) ;
  srand( (uint64_t)(&count) & 0xFFFFFFFFu ) ;
  if(argc > 1) srand(atoi(argv[1])) ;

  for(i=0 ; i<NPTS ; i++) {
    xrand = (random() & MASK) / 4096.0f ;
    array[i] = xrand * xrand * xrand * xrand * xrand * NPTS ;
  }
  for(i=0 ; i<8 ; i++) array[i] = 0 ;  // set potential extra points to 0 to be counted twice

  imax = 0 ;
  for(i=0 ; i<NPTS ; i++) imax = (array[i] > imax) ? array[i] : imax ;
  fprintf(stderr, "largest of %d values is %d\n", NPTS, imax) ;

  for(i=0 ; i<6 ; i++) count[i] = 0 ;
  v_less_than_simd_6(array, ref, count, NPTS) ;   // fast SIMD call
  for(i=0 ; i<6 ; i++){
    int short_c = count[i] ;
    int long_c = NPTS - short_c ;
    int bt = short_c * (bit[i] + 1) + 13 * long_c ;
    int full_c = BITS * NPTS ;
    if(bt > full_c) bt = full_c ;
    fprintf(stderr, "SIMD : %4d values smaller than %4d [%6d/%6d bits]\n", count[i], ref[i], bt, BITS * NPTS) ;
  }
  fprintf(stderr, "\n") ;

  for(i=0 ; i<6 ; i++) count[i] = 0 ;
  v_less_than_c_6(array, ref, count, NPTS) ;   // C call
  for(i=0 ; i<6 ; i++){
    int short_c = count[i] ;
    int long_c = NPTS - short_c ;
    int bt = short_c * (bit[i] + 1) + 13 * long_c ;
    int full_c = BITS * NPTS ;
    if(bt > full_c) bt = full_c ;
    fprintf(stderr, "C    : %4d values smaller than %4d [%6d/%6d bits]\n", count[i], ref[i], bt, BITS * NPTS) ;
  }
  fprintf(stderr, "\n") ;

#if defined(WITH_TIMING)
  float t0 ;
  if(cycles_overhead == 0) cycles_overhead = 1 ;

  for(i=0 ; i<6 ; i++) count[i] = 0 ;
  TIME_LOOP_EZ(NITER, NPTS, v_less_than(array, ref, count, NPTS) ; )
  t0 = timer_min * NaNoSeC / (NPTS) ;
  if(timer_max > timer_min) timer_max = timer_avg ;
  fprintf(stderr, "generic version : t(min) = %5.3f ns/pt, %ld ticks (%d pts)\n", t0, timer_min, NPTS) ;

  for(i=0 ; i<6 ; i++) count[i] = 0 ;
  TIME_LOOP_EZ(NITER, NPTS, v_less_than_6(array, ref, count, NPTS) ; )
  t0 = timer_min * NaNoSeC / (NPTS) ;
  if(timer_max > timer_min) timer_max = timer_avg ;
  fprintf(stderr, "generic version6: t(min) = %5.3f ns/pt, %ld ticks (%d pts)\n", t0, timer_min, NPTS) ;

  for(i=0 ; i<6 ; i++) count[i] = 0 ;
  TIME_LOOP_EZ(NITER, NPTS, v_less_than_4(array, ref, count, NPTS) ; )
  t0 = timer_min * NaNoSeC / (NPTS) ;
  if(timer_max > timer_min) timer_max = timer_avg ;
  fprintf(stderr, "generic version4: t(min) = %5.3f ns/pt, %ld ticks (%d pts)\n", t0, timer_min, NPTS) ;

  for(i=0 ; i<6 ; i++) count[i] = 0 ;
  TIME_LOOP_EZ(NITER, NPTS, v_less_than_c(array, ref, count, NPTS) ; )
  t0 = timer_min * NaNoSeC / (NPTS) ;
  if(timer_max > timer_min) timer_max = timer_avg ;
  fprintf(stderr, "C       version : t(min) = %5.3f ns/pt, %ld ticks (%d pts)\n", t0, timer_min, NPTS) ;

  for(i=0 ; i<6 ; i++) count[i] = 0 ;
  TIME_LOOP_EZ(NITER, NPTS, v_less_than_c_6(array, ref, count, NPTS) ; )
  t0 = timer_min * NaNoSeC / (NPTS) ;
  if(timer_max > timer_min) timer_max = timer_avg ;
  fprintf(stderr, "C6      version : t(min) = %5.3f ns/pt, %ld ticks (%d pts)\n", t0, timer_min, NPTS) ;

  for(i=0 ; i<6 ; i++) count[i] = 0 ;
  TIME_LOOP_EZ(NITER, NPTS, v_less_than_c_4(array, ref, count, NPTS) ; )
  t0 = timer_min * NaNoSeC / (NPTS) ;
  if(timer_max > timer_min) timer_max = timer_avg ;
  fprintf(stderr, "C4      version : t(min) = %5.3f ns/pt, %ld ticks (%d pts)\n", t0, timer_min, NPTS) ;

  for(i=0 ; i<6 ; i++) count[i] = 0 ;
  TIME_LOOP_EZ(NITER, NPTS, v_less_than_simd(array, ref, count, NPTS) ; )
  t0 = timer_min * NaNoSeC / (NPTS) ;
  if(timer_max > timer_min) timer_max = timer_avg ;
  fprintf(stderr, "SIMD    version : t(min) = %5.3f ns/pt, %ld ticks (%d pts)\n", t0, timer_min, NPTS) ;

  for(i=0 ; i<6 ; i++) count[i] = 0 ;
  TIME_LOOP_EZ(NITER, NPTS, v_less_than_simd_6(array, ref, count, NPTS) ; )
  t0 = timer_min * NaNoSeC / (NPTS) ;
  if(timer_max > timer_min) timer_max = timer_avg ;
  fprintf(stderr, "SIMD6   version : t(min) = %5.3f ns/pt, %ld ticks (%d pts)\n", t0, timer_min, NPTS) ;

  for(i=0 ; i<6 ; i++) count[i] = 0 ;
  TIME_LOOP_EZ(NITER, NPTS, v_less_than_simd_4(array, ref, count, NPTS) ; )
  t0 = timer_min * NaNoSeC / (NPTS) ;
  if(timer_max > timer_min) timer_max = timer_avg ;
  fprintf(stderr, "SIMD4   version : t(min) = %5.3f ns/pt, %ld ticks (%d pts)\n", t0, timer_min, NPTS) ;

  fprintf(stderr, "\n") ;
#endif

#define ABS(val)  (((val) < 0) ? (-(val)) : (val))

// short case (NPTS < vector length)
#undef NPTS
#define NPTS 7
  uint32_t mina = 0xFFFFFFFFu ;
  int32_t mins = 0x7FFFFFFF, maxs = -mins ;
  for(i=0 ; i<NPTS/2 ; i++) { array[i] = -(i + 5) ; array[i + NPTS/2] = (i + 5) ; }
  array[NPTS-1] = NPTS + 5 ;      // set max value
  array[0] = array[NPTS/2] = 0 ;  // eliminate smallest values as the > 0 min abs value
  v_minmax(array, NPTS, &mins, &maxs, &mina) ;
  fprintf(stderr, "minimum = %6d, maximum = %6d, abs minimum = %6d, %d values\n", mins, maxs, mina, NPTS) ;
  if(mina != (uint32_t)array[1 + NPTS/2] ) exit(1) ;
  if(mina != (uint32_t)ABS(array[1])) exit(1) ;
  if(mins != array[NPTS/2 - 1]) exit(1) ;
  if(maxs != array[NPTS-1]) exit(1) ;

// normal case (NPTS >= vector length)
#undef NPTS
#define NPTS 4097
  mina = 0xFFFFFFFFu ;
  mins = 0x7FFFFFFF ;
  maxs = -mins ;
  for(i=0 ; i<NPTS/2 ; i++) { array[i] = -(i + 15) ; array[i + NPTS/2] = (i + 15) ; }
  array[NPTS-1] = NPTS + 5 ;      // set max value
  array[0] = array[NPTS/2] = 0 ;  // eliminate smallest values as the > 0 min abs value
  v_minmax(array, NPTS, &mins, &maxs, &mina) ;
  fprintf(stderr, "minimum = %6d, maximum = %6d, abs minimum = %6d, %d values\n", mins, maxs, mina, NPTS) ;
  if(mina != (uint32_t)array[1 + NPTS/2] || mina != (uint32_t)ABS(array[1]) ) exit(1) ;
  if(mins != array[NPTS/2 - 1]) exit(1) ;
  if(maxs != array[NPTS-1]) exit(1) ;
  fprintf(stderr, "\n") ;

#if defined(WITH_TIMING)
  TIME_LOOP_EZ(NITER, NPTS,v_minmax(array, NPTS, &mins, &maxs, &mina) ; )
  t0 = timer_min * NaNoSeC / (NPTS) ;
  if(timer_max > timer_min) timer_max = timer_avg ;
  fprintf(stderr, "v_minmax        : t(min) = %5.3f ns/pt, %ld ticks (%d pts)\n", t0, timer_min, NPTS) ;

  fprintf(stderr, "\n") ;
#endif
}
