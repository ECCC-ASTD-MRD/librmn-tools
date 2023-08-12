/* Hopefully useful functions for C and FORTRAN
 * Copyright (C) 2021-2023  Recherche en Prevision Numerique
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation,
 * version 2.1 of the License.
 *
 * This is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <float.h>

#include <rmn/misc_operators.h>
// #include <rmn/tools_types.h>
#include <rmn/test_helpers.h>
#include <rmn/timers.h>

// the double include is deliberate
#include <rmn/ieee_quantize.h>
#include <rmn/ieee_quantize.h>

#define NPT  8
#define NPTS 37
#define NPTST 4096
#define N    35
#define NEXP 4

float max_abs_err(float *f1, float *f2, int n){
  float maxerr = 0.0f ;
  int i ;
  for(i=0 ; i<n ; i++) maxerr = MAX(maxerr, ABS(f1[i] - f2[i])) ;
  return maxerr ;
}

float avg_abs_err(float *f1, float *f2, int n){
  float maxerr = 0.0f ;
  int i ;
  for(i=0 ; i<n ; i++) maxerr += ABS(f1[i] - f2[i]) ;
//   for(i=0 ; i<n ; i++) maxerr += (f1[i] - f2[i]) ;
  return maxerr/n ;
}

int main(int argc, char **argv){
  FloatInt x1, x2, x3, y, z0, z, t0, t1, t2, fi0, fo0 ;
  uint32_t e, e0, m ;
  int i, j, nbits, nbits0 ;
  float r ;
  int denorm , first_denorm = 1;
  float    Big = FLT_MAX ;
//   float    Big = 66000.1 ;
  uint32_t Inf = 0X7F800000 ;
  uint32_t NaN = 0X7F800001 ;
  float zmax = 65432.898 ;
  float scale = 1.0f ;
//   float zmax = 8190.99 ;
//   float zmax = 7.0E+4 ;
  float fi[NPTST], fo[NPTST], FO[NPTST] ;
  uint32_t *uo = (uint32_t *) fo ;
  uint32_t *ui = (uint32_t *) fi ;
  uint32_t *tu ;
  int32_t qu[NPTST] ;
  float *fu = (float *) qu ;
  uint64_t h64 ;
  int32_t q[NPTST] ;
  qhead h ;
  float fz0[NPT] ;
//   float fp32 ;
//   qhead he ;
//   uint16_t fp16 ;
  uint16_t vfp16[NPTS] ;
  uint32_t limit16 = ((127+14) << 23) | 0x7FFFFF ; // largest representable FP16
//   float baseval = 8388607.0f ;
//   float baseval = 4194303.0f ;
//   float baseval = 2097151.0f ;
//   float baseval = 524287.0f ;
//   float baseval = 262143.0f ;
//   float baseval = 131070.10f ;
//   float baseval = 65534.1f ;
  float baseval = 64.01f ;
//   float baseval = 1.001f ;
//   float baseval = 1.0f / 32768.0f ;
//   float basefac = 1.0f ;
  float basefac = -32768.0f ;
//   int nbits_test = -1 ;
  int nbits_test = 13 ;
//   float quantum = 0.01f ;
  float quantum = 0.0f ;
  TIME_LOOP_DATA ;
  ieee32_u hieee ;
  int pos_neg = 0 ;
// #define WITH_TIMINGS
  start_of_test(argv[0]);
quantum = 0.1f ;

// ============================ NOT IN PLACE TESTS (type 2) ============================
  for(i=0 ; i<NPTS ; i++) fi[i] = baseval + basefac * (0.00001f + (i * 1.0f) / NPTS) ;
//   for(i=0 ; i<NPTS ; i++) fi[i] = baseval ;   // this MUST work too (constant array)
  if(pos_neg) for(i=0 ; i<NPTS ; i+=2) fi[i] = -fi[i] ;   // alternate signs, positive even, negative odd

  fprintf(stderr, "\n=============== NOT IN PLACE (type 2) ==============\n") ;
  fprintf(stderr, " in[0:1:last] = %12.6g, %12.6g, %12.6g\n", fi[0], fi[1], fi[NPTS-1]) ;
  for(i=0 ; i<NPTS ; i++) fprintf(stderr, " %5.2f", (fi[i] < 0.0f) ? fi[i] + baseval : fi[i] - baseval) ; fprintf(stderr, "\n") ;
  for(i=0 ; i<NPTS ; i++) qu[i] = -999999 ;
  h64 = IEEE32_linear_quantize_2(fi, NPTS, quantum > 0 ? 0 : nbits_test, quantum*.5f, qu) ;
  hieee.u = h64 ;
  for(i=0 ; i<NPTS ; i++) fprintf(stderr, " %5d", qu[i]) ; fprintf(stderr, "\n") ;
  IEEE32_linear_unquantize_2(qu, h64, NPTS, fo) ;
  for(i=0 ; i<NPTS ; i++) FO[i] = (fo[i] < 0) ? fo[i] + baseval : fo[i] - baseval ;
  for(i=0 ; i<NPTS ; i++) fprintf(stderr, " %5.2f", fo[i]) ; fprintf(stderr, "\n") ;
  for(i=0 ; i<NPTS ; i++) fprintf(stderr, " %5.2f", FO[i]) ; fprintf(stderr, "\n") ;
  fprintf(stderr, "<2> out[0:1:last] = %12.6g, %12.6g, %12.6g, avg(max) error = %12.6g(%12.6g), desired errmax = %g, nbits = %d\n",
          fo[0], fo[1], fo[NPTS-1], avg_abs_err(fi, fo, NPTS), max_abs_err(fi, fo, NPTS-1), quantum*.5f, hieee.x.nbts) ;
  for(i=0 ; i<NPTS ; i++) fprintf(stderr, " %5.2f", ABS(fo[i]-fi[i])) ; fprintf(stderr, "\n") ;
// return 0 ;
#if defined(WITH_TIMINGS)
  fprintf(stderr, "\n=============== TIMINGS (type 2) ==============\n") ;
  for(i=0 ; i<NPTST ; i++) fi[i] = i + .0001f ;
  for(i=0 ; i<NPTS ; i+=2) fi[i] = -fi[i] ;   // alternate signs, positive even, negative odd
  TIME_LOOP_EZ(1000, NPTST, h64 = IEEE32_linear_quantize_2(fi, NPTST, 16, .1f, qu)) ;
  fprintf(stderr, "IEEE32_linear_quantize_2    : %s\n",timer_msg);
  TIME_LOOP_EZ(1000, NPTST/2, h64 = IEEE32_linear_quantize_2(fi, NPTST/2, 16, .1f, qu)) ;
  fprintf(stderr, "IEEE32_linear_quantize_2    : %s\n",timer_msg);
  TIME_LOOP_EZ(1000, NPTST/4, h64 = IEEE32_linear_quantize_2(fi, NPTST/4, 16, .1f, qu)) ;
  fprintf(stderr, "IEEE32_linear_quantize_2    : %s\n",timer_msg);
  TIME_LOOP_EZ(1000, NPTST/32, h64 = IEEE32_linear_quantize_2(fi, NPTST/32, 16, .1f, qu)) ;
  fprintf(stderr, "IEEE32_linear_quantize_2    : %s\n",timer_msg);

  h64 = IEEE32_linear_quantize_2(fi, NPTST, 16, .1f, qu) ;
  TIME_LOOP_EZ(1000, NPTST, IEEE32_linear_unquantize_2(qu, h64, NPTST, fo) ;) ;
  fprintf(stderr, "IEEE32_linear_unquantize_2  : %s\n",timer_msg);
  h64 = IEEE32_linear_quantize_2(fi, NPTST/2, 16, .1f, qu) ;
  TIME_LOOP_EZ(1000, NPTST/2, IEEE32_linear_unquantize_2(qu, h64, NPTST/2, fo) ;) ;
  fprintf(stderr, "IEEE32_linear_unquantize_2  : %s\n",timer_msg);
  h64 = IEEE32_linear_quantize_2(fi, NPTST/4, 16, .1f, qu) ;
  TIME_LOOP_EZ(1000, NPTST/4, IEEE32_linear_unquantize_2(qu, h64, NPTST/4, fo) ;) ;
  fprintf(stderr, "IEEE32_linear_unquantize_2  : %s\n",timer_msg);
  h64 = IEEE32_linear_quantize_2(fi, NPTST/8, 16, .1f, qu) ;
  TIME_LOOP_EZ(1000, NPTST/8, IEEE32_linear_unquantize_2(qu, h64, NPTST/8, fo) ;) ;
  fprintf(stderr, "IEEE32_linear_unquantize_2  : %s\n",timer_msg);
#endif
// return 0 ;
// ============================ NOT IN PLACE TESTS (type 1) ============================
  for(i=0 ; i<NPTS ; i++) fi[i] = baseval + basefac * (0.00001f + (i * 1.0f) / NPTS) ;
//   for(i=0 ; i<NPTS ; i++) fi[i] = baseval ;   // this MUST work too (constant array)
  if(pos_neg) for(i=0 ; i<NPTS ; i+=2) fi[i] = -fi[i] ;   // alternate signs, positive even, negative odd

  fprintf(stderr, "\n=============== NOT IN PLACE (type 1) ==============\n") ;
  fprintf(stderr, " in[0:1:last] = %12.6g, %12.6g, %12.6g\n", fi[0], fi[1], fi[NPTS-1]) ;
  for(i=0 ; i<NPTS ; i++) fprintf(stderr, " %5.2f", (fi[i] < 0.0f) ? fi[i] + baseval : fi[i] - baseval) ; fprintf(stderr, "\n") ;
  for(i=0 ; i<NPTS ; i++) qu[i] = -999999 ;
//   fi[NPTS/2] = 0.0f ;
  h64 = IEEE32_linear_quantize_1(fi, NPTS, quantum > 0 ? 0 : nbits_test, quantum*.5f, qu) ;
  hieee.u = h64 ;
  for(i=0 ; i<NPTS ; i++) fprintf(stderr, " %5d", qu[i]) ; fprintf(stderr, "\n") ;
  IEEE32_linear_unquantize_1(qu, h64, NPTS, fo) ;
  for(i=0 ; i<NPTS ; i++) FO[i] = (fo[i] < 0) ? fo[i] + baseval : fo[i] - baseval ;
  for(i=0 ; i<NPTS ; i++) fprintf(stderr, " %5.2f", fo[i]) ; fprintf(stderr, "\n") ;
  for(i=0 ; i<NPTS ; i++) fprintf(stderr, " %5.2f", FO[i]) ; fprintf(stderr, "\n") ;
//   for(i=0 ; i<NPTS ; i++) fprintf(stderr, " %5.2f", (fo[i] < 0) ? fo[i] + baseval : fo[i] - baseval) ; fprintf(stderr, "\n") ;
  fprintf(stderr, "<1> out[0:1:last] = %12.6g, %12.6g, %12.6g, avg(max) error = %12.6g(%12.6g), desired errmax = %g, nbits = %d\n",
          fo[0], fo[1], fo[NPTS-1], avg_abs_err(fi, fo, NPTS), max_abs_err(fi, fo, NPTS-1), quantum*.5f, hieee.x.nbts) ;
  for(i=0 ; i<NPTS ; i++) fprintf(stderr, " %5.2f", ABS(fo[i]-fi[i])) ; fprintf(stderr, "\n") ;
// return 0 ;
// ============================ TIMINGS (type 1) ============================
#if defined(WITH_TIMINGS)
  fprintf(stderr, "\n=============== TIMINGS (type 1) ==============\n") ;
  for(i=0 ; i<NPTST ; i++) fi[i] = i + .0001f ;
  for(i=0 ; i<NPTS ; i+=2) fi[i] = -fi[i] ;   // alternate signs, positive even, negative odd
  TIME_LOOP_EZ(1000, NPTST, h64 = IEEE32_linear_quantize_1(fi, NPTST, 16, .1f, qu)) ;
  fprintf(stderr, "IEEE32_linear_quantize_1    : %s\n",timer_msg);
  TIME_LOOP_EZ(1000, NPTST/2, h64 = IEEE32_linear_quantize_1(fi, NPTST/2, 16, .1f, qu)) ;
  fprintf(stderr, "IEEE32_linear_quantize_1    : %s\n",timer_msg);
  TIME_LOOP_EZ(1000, NPTST/4, h64 = IEEE32_linear_quantize_1(fi, NPTST/4, 16, .1f, qu)) ;
  fprintf(stderr, "IEEE32_linear_quantize_1    : %s\n",timer_msg);
  TIME_LOOP_EZ(1000, NPTST/32, h64 = IEEE32_linear_quantize_1(fi, NPTST/32, 16, .1f, qu)) ;
  fprintf(stderr, "IEEE32_linear_quantize_1    : %s\n",timer_msg);

  h64 = IEEE32_linear_quantize_1(fi, NPTST, 16, .1f, qu) ;
  TIME_LOOP_EZ(1000, NPTST, IEEE32_linear_unquantize_1(qu, h64, NPTST, fo) ;) ;
  fprintf(stderr, "IEEE32_linear_unquantize_1  : %s\n",timer_msg);
  h64 = IEEE32_linear_quantize_1(fi, NPTST/2, 16, .1f, qu) ;
  TIME_LOOP_EZ(1000, NPTST/2, IEEE32_linear_unquantize_1(qu, h64, NPTST/2, fo) ;) ;
  fprintf(stderr, "IEEE32_linear_unquantize_1  : %s\n",timer_msg);
  h64 = IEEE32_linear_quantize_1(fi, NPTST/4, 16, .1f, qu) ;
  TIME_LOOP_EZ(1000, NPTST/4, IEEE32_linear_unquantize_1(qu, h64, NPTST/4, fo) ;) ;
  fprintf(stderr, "IEEE32_linear_unquantize_1  : %s\n",timer_msg);
  h64 = IEEE32_linear_quantize_1(fi, NPTST/8, 16, .1f, qu) ;
  TIME_LOOP_EZ(1000, NPTST/8, IEEE32_linear_unquantize_1(qu, h64, NPTST/8, fo) ;) ;
  fprintf(stderr, "IEEE32_linear_unquantize_1  : %s\n",timer_msg);
// return 0 ;
#endif
// return 0 ;
// ============================ NOT IN PLACE TESTS (type 0) ============================
  for(i=0 ; i<NPTS ; i++) fi[i] = baseval + basefac * (0.00001f + (i * 1.0f) / NPTS) ;
//   for(i=0 ; i<NPTS ; i++) fi[i] = baseval ;   // this MUST work too (constant array)
  if(pos_neg) for(i=0 ; i<NPTS ; i+=2) fi[i] = -fi[i] ;   // alternate signs, positive even, negative odd

  fprintf(stderr, "\n=============== NOT IN PLACE (type 0) ==============\n") ;
//   for(i=0 ; i<NPTS ; i++) fprintf(stderr, "%8.8x ", ui[i]) ; fprintf(stderr, "\n");
//   for(i=0 ; i<NPTS ; i++) fo[i] = fi[i] ;
  fprintf(stderr, " in[0:1] = %g, %g\n", fi[0], fi[1]) ;
  for(i=0 ; i<NPTS ; i++) fprintf(stderr, " %5.2f", (fi[i] < 0.0f) ? fi[i] + baseval : fi[i] - baseval) ; fprintf(stderr, "\n") ;
  for(i=0 ; i<NPTS ; i++) qu[i] = -1 ;
  h64 = IEEE32_linear_quantize_0(fi, NPTS, quantum > 0 ? 0 : nbits_test, quantum*0.5f, qu) ;
hieee.u = h64 ;
  for(i=0 ; i<NPTS ; i++) fprintf(stderr, " %5d", qu[i]) ; fprintf(stderr, "\n") ;
  for(i=0 ; i<NPTS ; i++) fo[i] = 999999.0f ;
  IEEE32_linear_unquantize_0(qu, h64, NPTS, fo) ;
//   for(i=0 ; i<NPTS ; i++) fprintf(stderr, "%5.2f", fi[i]) ; fprintf(stderr, "\n") ;
//   for(i=0 ; i<NPTS ; i++) fprintf(stderr, "%5.2f", fo[i]) ; fprintf(stderr, "\n") ;
  fprintf(stderr, "<0> out[0:1:last] = %12.6g, %12.6g, %12.6g, avg(max) error = %12.6g(%12.6g), desired errmax = %g, nbits = %d\n",
          fo[0], fo[1], fo[NPTS-1], avg_abs_err(fi, fo, NPTS), max_abs_err(fi, fo, NPTS-1), quantum*.5f, hieee.x.nbts) ;
//   for(i=0 ; i<NPTS ; i++) fprintf(stderr, " %5.2f", (fo[i] < 0) ? fo[i] + baseval : fo[i] - baseval) ; fprintf(stderr, "\n") ;
//   fprintf(stderr, " out[0:1] = %g, %g\n", fo[0], fo[1]) ;
  for(i=0 ; i<NPTS ; i++) fprintf(stderr, " %5.2f", ABS(fo[i]-fi[i])) ; fprintf(stderr, "\n") ;
return 0 ;
// ============================ IN PLACE TESTS (type 0) ============================
  fprintf(stderr, "\n=============== IN PLACE (type 0) ==============\n") ;
  for(i=0 ; i<NPTS ; i++) fo[i] = fi[i] ;
  fprintf(stderr, " in[0:1] = %g, %g\n", fo[0], fo[1]) ;
  for(i=0 ; i<NPTS ; i++) fprintf(stderr, " %5.2f", (fo[i] < 0) ? fo[i] + baseval : fo[i] - baseval) ; fprintf(stderr, "\n") ;
  h64 = IEEE32_linear_quantize_0(fo, NPTS, quantum > 0 ? 0 : nbits_test, quantum*0.5f, fo) ;;                   // quantize in-place
  for(i=0 ; i<NPTS ; i++) fprintf(stderr, " %5d", uo[i]) ; fprintf(stderr, "\n") ;
  IEEE32_linear_unquantize_0(fo, h64, NPTS, fo) ;                            // restore in-place
  fprintf(stderr, ">0< out[0:1:last] = %12.6g, %12.6g, %12.6g, avg(max) error = %12.6g(%12.6g), desired errmax = %g, nbits = %d\n",
          fo[0], fo[1], fo[NPTS-1], avg_abs_err(fi, fo, NPTS), max_abs_err(fi, fo, NPTS-1), quantum*.5f, hieee.x.nbts) ;
//   for(i=0 ; i<NPTS ; i++) fprintf(stderr, " %5.2f", (fo[i] < 0) ? fo[i] + baseval : fo[i] - baseval) ; fprintf(stderr, "\n") ;
//   fprintf(stderr, " out[0:1] = %g, %g\n", fo[0], fo[1]) ;
  for(i=0 ; i<NPTS ; i++) fprintf(stderr, " %5.2f", ABS(fo[i]-fi[i])) ; fprintf(stderr, "\n") ;

// ============================ TIMINGS (type 0) ============================
#if defined(WITH_TIMINGS)
  fprintf(stderr, "\n=============== TIMINGS (type 0) ==============\n") ;
  for(i=0 ; i<NPTST ; i++) fi[i] = i + .0001f ;
  for(i=0 ; i<NPTS ; i+=2) fi[i] = -fi[i] ;   // alternate signs, positive even, negative odd
  TIME_LOOP_EZ(1000, NPTST, h64 = IEEE32_linear_quantize_0(fi, NPTST, 16, .1f, qu)) ;
  fprintf(stderr, "IEEE32_linear_quantize_0    : %s\n",timer_msg);
  TIME_LOOP_EZ(1000, NPTST/2, h64 = IEEE32_linear_quantize_0(fi, NPTST/2, 16, .1f, qu)) ;
  fprintf(stderr, "IEEE32_linear_quantize_0    : %s\n",timer_msg);
  TIME_LOOP_EZ(1000, NPTST/4, h64 = IEEE32_linear_quantize_0(fi, NPTST/4, 16, .1f, qu)) ;
  fprintf(stderr, "IEEE32_linear_quantize_0    : %s\n",timer_msg);
  TIME_LOOP_EZ(1000, NPTST/32, h64 = IEEE32_linear_quantize_0(fi, NPTST/32, 16, .1f, qu)) ;
  fprintf(stderr, "IEEE32_linear_quantize_0    : %s\n",timer_msg);

  h64 = IEEE32_linear_quantize_0(fi, NPTST, 16, .1f, qu) ;
  TIME_LOOP_EZ(1000, NPTST, IEEE32_linear_unquantize_0(qu, h64, NPTST, fo) ;) ;
  fprintf(stderr, "IEEE32_linear_unquantize_0  : %s\n",timer_msg);
  h64 = IEEE32_linear_quantize_0(fi, NPTST/2, 16, .1f, qu) ;
  TIME_LOOP_EZ(1000, NPTST/2, IEEE32_linear_unquantize_0(qu, h64, NPTST/2, fo) ;) ;
  fprintf(stderr, "IEEE32_linear_unquantize_0  : %s\n",timer_msg);
  h64 = IEEE32_linear_quantize_0(fi, NPTST/4, 16, .1f, qu) ;
  TIME_LOOP_EZ(1000, NPTST/4, IEEE32_linear_unquantize_0(qu, h64, NPTST/4, fo) ;) ;
  fprintf(stderr, "IEEE32_linear_unquantize_0  : %s\n",timer_msg);
  h64 = IEEE32_linear_quantize_0(fi, NPTST/8, 16, .1f, qu) ;
  TIME_LOOP_EZ(1000, NPTST/8, IEEE32_linear_unquantize_0(qu, h64, NPTST/8, fo) ;) ;
  fprintf(stderr, "IEEE32_linear_unquantize_0  : %s\n",timer_msg);
#endif

return 0 ;

  fprintf(stderr, "limit16 = %8.8x, %8d, %8.8x\n", limit16, limit16 >> 23, limit16 & 0x7FFFFF);
  fi[0] = 1.0 ;
  for(i=1 ; i < NPTS ; i++) { fi[i] = -fi[i-1] * 2.0f ; }
//   fi[17] = 0.000060975552 ;
  fi[16] = 65519.9980468749999999f ;  // largest value that does not decode to infinity
  fi[16] = 65519.998046875f ;      // generates infinity after decoding
  fi[18] = 0.00006104 ;
  fi[19] = 65536 - 56 ;
  for(i = 20 ; i < NPTS ; i++) { fi[i] = fi[i-1] + 1.0f ; }
  fp32_to_fp16_scaled(fi, vfp16, NPTS, scale) ;
  fp16_to_fp32_scaled(fo, vfp16, NPTS, NULL, scale) ;
//   fp16_to_fp32_scaled(fo, vfp16, NPTS, (void *) &Big, scale) ;
//   fp16_to_fp32(fo, vfp16, NPTS, (void *) &Big) ;
//   fp16_to_fp32_scaled(fo, vfp16, NPTS, &NaN, scale) ;
//   fp16_to_fp32_scaled(fo, vfp16, NPTS, &Inf, scale) ;
  for(i=0 ; i < NPTS ; i++){
    x1.f = fi[i] ;
//     fp16 = vfp16[i] ;
//     fp32 = fo[i] ;
    fprintf(stderr, "fp32 = %12g (%12g) (%8.8x), fp16 = %8.8x (%2d,%4.4x)\n", 
            fi[i], fo[i], x1.u, vfp16[i], vfp16[i]>>10, vfp16[i] & 0x3FF) ;
  }
// return 0 ;
//   for(i=0 ; i<NPT ; i++) fz0[i] = i - (NPT-1)/2.0f ;
//   fprintf(stderr,"fz0 :");
//   for(i=0 ; i<NPT ; i++) fprintf(stderr," %f",fz0[i]) ; fprintf(stderr,"\n\n");
//   for(j=1 ; j<=NPT ; j++){
//     quantize_setup(fz0, j, &he);
//     fprintf(stderr,"[%2d] max = %10f, min = %10f, mina = %10f, rng = %10f, rnga = %10f\n", 
//             j, he.fmax, he.fmin, he.amin, he.rng, he.rnga);
//   }
// return 0 ;
  IEEE32_clip(fi, 0, 16) ;
  fi[0] = zmax ;
  for(i=1 ; i<NPTS ; i++) fi[i] = fi[i-1] * .499 ;
//   fi[5] = -fi[5] ;
//   fi[3] = -fi[0] ;
  for(i=0 ; i<NPTS ; i++) fi[i] = -fi[i] ;
  for(i=1 ; i<NPTS ; i++) q[i] = 0 ;
  quantize_setup(fi, N, &h);
  fprintf(stderr,"fi  : max = %f, min = %f, mina = %f, rng = %f, rnga = %f\n", 
          h.fmax, h.fmin, h.amin, h.rng, h.rnga);
  nbits0 = 16 ;
  nbits = nbits0 ;
//   if(h.fmin * h.fmax < 0) nbits-- ;  // positive and negative numbers, need to reserve a bit for the sign
//   e0 = IEEE32_quantize( fi, &zmax,  q, N, NEXP, nbits, &h) ;
  e0 = IEEE32_quantize_v4( fi, q, N, NEXP, nbits, &h) ;
  fprintf(stderr,"nexp = %d, nbits = %d, e0 = %d %d, min = %d, max = %d, span = %d, limit = %8.8x, sbit = %d, neg = %d\n",
          h.nexp, h.nbits, h.e0, e0, h.min, h.max, h.max-h.min, h.limit, h.sbit, h.negative) ;
//   IEEE32_unquantize( fo, q, N, NEXP, e0, 16, &h) ;
  IEEE32_unquantize( fo, q, N, &h) ;
  for(i = 0 ; i < N ; i++) {
    fi0.f = fi[i] ; fo0.f = fo[i] ;
    r = fo[i]/fi[i] ; r = r - 1.0 ; r = (r>0) ? r : -r ;
    denorm = (ABS(q[i]) >> (nbits0-NEXP)) == 0 ;
    if(denorm && first_denorm) {
      fprintf(stderr,"\n") ;
      first_denorm = 0 ;
    }
    fprintf(stderr,"z0 = %8.8x %12g, coded = %8.8x, z1 = %8.8x %12g, r = %12f %10.0f %12.0f, d = %9f\n",
                   fi0.u, fi[i],     q[i],          fo0.u, fo[i],    r, 1.0/r, zmax / (fi[i] - fo[i]), fo[i] - fi[i]);
    if(q[i] == 0) break ;
  }
return 0 ;
#if 0
fprintf(stderr,"====================================\n");
  t0.i = 15 << 23 ;
  t1.i = (254 - 15) << 23 ;
first_denorm = 1;
  for(i = 0 ; i < 35 ; i++) {
    e0 = IEEE32_quantize( &z0.f, &zmax,  &y.i, 1, NEXP, 16, h) ;
    denorm = (y.i >> 11) == 0 ;
    if(denorm && first_denorm) {
      fprintf(stderr,"\n") ;
      first_denorm = 0 ;
    }
    IEEE32_unquantize( &x3.f, &y.i, 1, NEXP, e0, 16, h) ;
    r = x3.f / z0.f ;
    fprintf(stderr,"z0 = %8.8x %12g, x1 = %8.8x %12g, coded = %8.8x, z1 = %8.8x %12g, r = %f\n", 
                    z0.i, z0.f,      x1.i, x1.f,      y.i,       x3.i, x3.f,      r) ;
    if(y.i == 0) break ;
    z0.f *= .33 ;
  }

  return 0 ;
#endif
}
