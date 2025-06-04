// Hopefully useful code for C
// Copyright (C) 2025  Recherche en Prevision Numerique
//
// This code is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation,
// version 2.1 of the License.
//
// This code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// Author:
//     M. Valin,   Recherche en Prevision Numerique, 2025
//
#include <stdio.h>

#include <rmn/timers.h>
#include <rmn/test_helpers.h>
#include <rmn/quantizers.h>
#include <rmn/move_blocks.h>
#include <rmn/misc_operators.h>

#define NI 95
#define NJ 65

void verify_log(float r[NJ][NI], int32_t q[NJ][NI], int nij, int32_t e_base0, int32_t mbits, float Z[NJ][NI], float vref){
  int i, j, n = 0 ;
  float errmax = 0.0f, vmax = 999999.0f, verr = 999999.0f ;;

  q2fp_log((float *)r, (int32_t *)q, nij, e_base0, mbits) ;
  for(j=0 ; j<NJ ; j++){
    for(i=0 ; i<NI ; i++){
      float err ;
      float absz = (Z[j][i] < 0) ? -Z[j][i] : Z[j][i] ;
      if(Z[j][i] == 0 || absz <= vref) continue ;
      n++ ;
      if(Z[j][i] != 0 && absz > vref) err = (Z[j][i] - r[j][i]) / Z[j][i] ;
      err = (err <0) ? (-err) : err ;
      if(err > errmax){
        errmax = err ;
        vmax = Z[j][i] ;
        verr = r[j][i] - Z[j][i] ;
      }
    }
  }
  fprintf(stderr,"errmax = %f (1 part in %5.0f), vmax = %f, verr = %f, vref = %f, n = %d/%d \n\n", errmax, 1.0f/errmax, vmax, verr, vref, n, NI*NJ) ;
}

int main(int argc, char **argv){
  float z[NJ][NI] ;
  float Z[NJ][NI] ;
  float r[NJ][NI] ;
  int32_t q[NJ][NI] ;
  int i, j, nij ;
  float errmax, err, target, quantum ;
  int32_t offset, nbits, mode, status ;
  block_properties bp ;
  uint64_t freq ;
  double nano ;
  TIME_LOOP_DATA ;
  int niter = 5 ;
  int32_t e_base = 255, e_base0 = -255 ;
  char *msg = "generic error" ;

  goto test ;

end:
  fprintf(stderr, "SUCCESS\n") ;
  if(timer_max == 0) fprintf(stderr, "timer_max is zero\n") ;
  return 0 ;

fail:
  fprintf(stderr, "FAILED : %s\n", msg) ;
  return 1 ;

test:
  if(argc > 100) return 1 ;

  freq = cycles_counter_freq() ;
  nano = 1000000000 ;
  nano /= freq ;
  fprintf(stderr, "time counter tick = %4.2f ns\n", nano) ;

  start_of_test(argv[0]);
  fprintf(stderr, "============================== linear quantizers ==============================\n") ;

  for(j=0 ; j<NJ ; j++){
    for(i=0 ; i<NI ; i++){
      z[j][i] = (i - (NI-1)*.5f) + (j - (NJ-1)*.5f) + .5 ;
//       Z[j][i] = z[j][i] * ((z[j][i] < 0) ? (-z[j][i]) : z[j][i] ) ;
      Z[j][i] = z[j][i] * z[j][i] * z[j][i] * 1.9f ;
      r[j][i] = 999999.0f ;
    }
  }
  nij = analyze_data32_block((void *)z, NI, NI, NJ, &bp) ;
  adjust_block_properties(&bp, float_data) ;

  offset = 0 ;
  quantum = 4.0f ;
  nbits = 0 ;
  mode = FP_QUANTIZE_LIN ;
  nij = NI*NJ ;
//   e_base = fp2q_lin((void *)z, (void *)q, nij, quantum, offset);
  e_base = fp2q_n((void *)z, (void *)q, nij, NULL, quantum * .5f, nbits, 0.0f, &offset, mode) ;
  nbits = -1 ;
//   q2fp_lin((void *)r, (void *)q, nij, e_base, offset);
  status = q2fp_n((void *)r, (void *)q, nij, e_base, nbits, offset, mode) ;
  msg = "status from q2fp_n not 0" ;
  if(status != 0) goto fail ;

  errmax = 0.0f ;
  target = 0.5f*quantum ;
  for(j=0 ; j<NJ ; j++){
    for(i=0 ; i<NI ; i++){
      err = z[j][i] - r[j][i] ;
      err = (err <0) ? (-err) : err ;
      errmax = (err > errmax) ? err : errmax ;
    }
  }
  fprintf(stderr, "min = %f, max = %f, errmax = %f, target = %f, status = %d\n",FLOAT_MIN_VALUE(bp), FLOAT_MAX_VALUE(bp), errmax, target, status) ;
  msg = "errmax > target" ;
  if(errmax > target) goto fail ;

  fprintf(stderr, "============================== linear timingss ==============================\n") ;

//   TIME_LOOP_EZ(niter, nij, e_base = fp2q_lin((void *)z, (void *)q, nij, quantum, offset)) ;
//   fprintf(stderr,"quantize : %s\n", timer_msg) ;
// 
//   TIME_LOOP_EZ(niter, nij, q2fp_lin((void *)z, (void *)q, nij, e_base, offset)) ;
//   fprintf(stderr,"restore  : %s\n", timer_msg) ;

  fprintf(stderr, "============================== pseudo log quantizers ==============================\n") ;

  nij = analyze_data32_block((void *)Z, NI, NI, NJ, &bp) ;
  adjust_block_properties(&bp, float_data) ;
  print_float_props(bp);

  int32_t mbits = 11 ;
  uint32_t round = (1 << (22 - mbits)), e_range = 0 ;
  float vref = 32.0f ;
  e_base = 255 ; e_base0 = -255 ;

  e_base  = fp32_exp(FLOAT_MIN_ABS(bp)) ;
  e_range = fp32_exp(FLOAT_MAX_ABS(bp)) - e_base ;
  fprintf(stderr, "exponent range = %d, e_base = %d\n", e_range, e_base) ;
  if(fp32_exp(vref) > e_base){
    e_base = fp32_exp(vref) ;
    e_range = fp32_exp(FLOAT_MAX_ABS(bp)) - e_base ;
    fprintf(stderr, "revised exponent range = %d, e_base = %d\n", e_range, e_base) ;
  }

  int32_t e_bits = BitsNeeded_u32(e_range) ;
  fprintf(stderr, "%d bits needed for exponent, vref = %f, %d bits for mantissa\n\n", e_bits, vref, mbits) ;
  for(j=0 ; j<NJ ; j++){
    for(i=0 ; i<NI ; i++){
      r[j][i] = 999999.0f ;
    }
  }
  errmax = 0.0f ;
  float vmax = 999999.0f, verr = 999999.0f ;

//   e_base0 = fp2q_log((float *)Z, (int32_t *)q, nij, vref, mbits) ;
  e_base0 = fp2q_n((float *)Z, (int32_t *)q, nij, NULL, 0.00003f, mbits,  vref, NULL, FP_QUANTIZE_LOG) ;
  verify_log((void *)r, (void *)q, nij, e_base0, mbits, (void *)Z, vref) ;

  e_base0 = fp2q_n((float *)Z, (int32_t *)q, nij, NULL, 0.0f,     mbits,  vref, NULL, FP_QUANTIZE_LOG) ;
  verify_log((void *)r, (void *)q, nij, e_base0, mbits, (void *)Z, vref) ;

  e_base0 = fp2q_n((float *)Z, (int32_t *)q, nij, NULL, 0.0005f, 0,       vref, NULL, FP_QUANTIZE_LOG) ;
  verify_log((void *)r, (void *)q, nij, e_base0, mbits, (void *)Z, vref) ;

  e_base0 = fp2q_n((float *)Z, (int32_t *)q, nij, NULL, 0.00003f, mbits, -vref, NULL, FP_QUANTIZE_LOG) ;
  verify_log((void *)r, (void *)q, nij, e_base0, mbits, (void *)Z, 65536.0) ;

//   msg = "e_base0 != e_base" ;
//   if(e_base0 != e_base) goto fail ;

//   q2fp_log((float *)r, (int32_t *)q, nij, e_base0, mbits) ;
// 
//   for(j=0 ; j<NJ ; j++){
//     for(i=0 ; i<NI ; i++){
//       float absz = (Z[j][i] < 0) ? -Z[j][i] : Z[j][i] ;
//       if(Z[j][i] != 0 && absz > vref) err = (Z[j][i] - r[j][i]) / Z[j][i] ;
//       err = (err <0) ? (-err) : err ;
//       if(err > errmax){
//         errmax = err ;
//         vmax = Z[j][i] ;
//         verr = r[j][i] - Z[j][i] ;
//       }
//     }
//   }
//   fprintf(stderr,"errmax = %f (1 part in %5.0f), vmax = %f, verr = %f \n", errmax, 1.0f/errmax, vmax, verr) ;

  block_properties bpi ;
  nij = analyze_data32_block((void *)q, NI, NI, NJ, &bpi) ;
  adjust_block_properties(&bpi, int_data) ;
  print_int_props(bpi);

  for(j=NJ-1 ; j>NJ-9 ; j--){
    fprintf(stderr, "j = %3d :", j) ;
    for(i=NI-8 ; i<NI ; i++){
      fprintf(stderr, "%8.0f ", Z[j][i]);
    }
    fprintf(stderr, "\n") ;
  }
  fprintf(stderr, "\n") ;
  for(j=NJ-1 ; j>NJ-9 ; j--){
    fprintf(stderr, "j = %3d :", j) ;
    for(i=NI-8 ; i<NI ; i++){
      fprintf(stderr, "%8d ", q[j][i]);
    }
    fprintf(stderr, "\n") ;
  }

  int32_t uq ;
  float ur ;

  uq = fp2q_log1_(vref*.00001f, e_base, mbits, round) ; ur = q2fp_log1_(uq, e_base, mbits) ;
  fprintf(stderr, "%8.8x = fp2q_log1_(%f) [%f]\n", uq, vref*.00001f, ur ) ;

  uq = fp2q_log1_(vref*.001f, e_base, mbits, round) ; ur = q2fp_log1_(uq, e_base, mbits) ;
  fprintf(stderr, "%8.8x = fp2q_log1_(%f) [%f]\n", uq, vref*.001f, ur ) ;

  uq = fp2q_log1_(vref*.1f, e_base, mbits, round) ; ur = q2fp_log1_(uq, e_base, mbits) ;
  fprintf(stderr, "%8.8x = fp2q_log1_(%f) [%f]\n", uq, vref*.1f, ur ) ;

  uq = fp2q_log1_(FLOAT_MIN_ABS(bp), e_base, mbits, round) ; ur = q2fp_log1_(uq, e_base, mbits) ;
  fprintf(stderr, "%8.8x = fp2q_log1_(%f) [%f]\n", uq, FLOAT_MIN_ABS(bp), ur ) ;

  uq = fp2q_log1_(+78.5001f, e_base, mbits, round) ; ur = q2fp_log1_(uq, e_base, mbits) ;
  fprintf(stderr, "%8.8x = fp2q_log1_(%f) [%f]\n", uq, +78.5001f, ur ) ;

  uq = fp2q_log1_(-78.5001f, e_base, mbits, round) ; ur = q2fp_log1_(uq, e_base, mbits) ;
  fprintf(stderr, "%8.8x = fp2q_log1_(%f) [%f]\n", uq, -78.5001f, ur ) ;

  uq = fp2q_log1_(FLOAT_MAX_VALUE(bp), e_base, mbits, round) ; ur = q2fp_log1_(uq, e_base, mbits) ;
  fprintf(stderr, "%8.8x = fp2q_log1_(%f) [%f]\n", uq, FLOAT_MAX_VALUE(bp), ur ) ;

  uq = fp2q_log1_(FLOAT_MIN_VALUE(bp), e_base, mbits, round) ; ur = q2fp_log1_(uq, e_base, mbits) ;
  fprintf(stderr, "%8.8x = fp2q_log1_(%f) [%f]\n", uq, FLOAT_MIN_VALUE(bp), ur ) ;

  fprintf(stderr, "============================== pseudo log timingss ==============================\n") ;

  TIME_LOOP_EZ(niter, nij, e_base0 = fp2q_log((float *)Z, (int32_t *)q, nij, vref, mbits)) ;
  fprintf(stderr,"quantize : %s\n", timer_msg) ;

  TIME_LOOP_EZ(niter, nij, q2fp_log((float *)r, (int32_t *)q, nij, e_base0, mbits)) ;
  fprintf(stderr,"restore  : %s\n", timer_msg) ;

  goto end ;
}
