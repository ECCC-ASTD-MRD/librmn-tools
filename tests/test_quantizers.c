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

#include <rmn/test_helpers.h>
#include <rmn/quantizers.h>
#include <rmn/move_blocks.h>
#include <rmn/misc_operators.h>

#define NI 95
#define NJ 65

int main(int argc, char **argv){
  float z[NJ][NI] ;
  float Z[NJ][NI] ;
  float r[NJ][NI] ;
  int32_t q[NJ][NI] ;
  int i, j, nij ;
  float ovd, errmax, err ;
  int32_t offset ;
  block_properties bp ;

  if(argc > 100) return 1 ;

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
  ovd = .25f ;
  fp2q_lin((void *)z, (void *)q, NI*NJ, ovd, offset);
  q2fp_lin((void *)r, (void *)q, NI*NJ, 1.0f/ovd, offset);

  errmax = 0.0f ;
  for(j=0 ; j<NJ ; j++){
    for(i=0 ; i<NI ; i++){
      err = z[j][i] - r[j][i] ;
      err = (err <0) ? (-err) : err ;
      errmax = (err > errmax) ? err : errmax ;
    }
  }
  fprintf(stderr, "min = %f, max = %f, errmax = %f, target = %f\n",FLOAT_MIN_VALUE(bp), FLOAT_MAX_VALUE(bp), errmax, 0.5f/ovd) ;

  fprintf(stderr, "============================== pseudo log quantizers ==============================\n") ;

  nij = analyze_data32_block((void *)Z, NI, NI, NJ, &bp) ;
  adjust_block_properties(&bp, float_data) ;
  print_float_props(bp);

  int32_t mbits = 11 ;
  uint32_t round = (1 << (22 - mbits)), e_range = 0 ;
  float qzero = 8.0f ;
  int32_t e_base = fp32_exp(FLOAT_MIN_ABS(bp)) ;
  e_range = fp32_exp(FLOAT_MAX_ABS(bp)) - e_base ;
  fprintf(stderr, "exponent range = %d, e_base = %d\n", e_range, e_base) ;
  if(fp32_exp(qzero) > e_base){
    e_base = fp32_exp(qzero) ;
    e_range = fp32_exp(FLOAT_MAX_ABS(bp)) - e_base ;
    fprintf(stderr, "revised exponent range = %d, e_base = %d\n", e_range, e_base) ;
  }
  int32_t e_bits = BitsNeeded_u32(e_range) ;
  fprintf(stderr, "%d bits needed for exponent, qzero = %f, %d bits for mantissa\n", e_bits, qzero, mbits) ;
  for(j=0 ; j<NJ ; j++){
    for(i=0 ; i<NI ; i++){
      r[j][i] = 999999.0f ;
    }
  }
  errmax = 0.0f ;
  float vmax = 999999.0f, verr = 999999.0f ;
  for(j=0 ; j<NJ ; j++){
    for(i=0 ; i<NI ; i++){
      q[j][i] = fp2q_log1_(Z[j][i], e_base, mbits, round);
      r[j][i] = q2fp_log1_(q[j][i], e_base, mbits) ;
      float absz = (Z[j][i] < 0) ? -Z[j][i] : Z[j][i] ;
      if(Z[j][i] != 0 && absz > qzero) err = (Z[j][i] - r[j][i]) / Z[j][i] ;
//       err = (Z[j][i] - r[j][i]) ;
      err = (err <0) ? (-err) : err ;
      if(err > errmax){
        errmax = err ;
        vmax = Z[j][i] ;
        verr = r[j][i] ;
      }
    }
  }
  fprintf(stderr,"errmax = %f (1 part in %5.0f), vmax = %f, verr = %f \n", errmax, 1.0f/errmax, vmax, verr) ;
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

  uint32_t uq ;

  uq = fp2q_log1_(qzero*.00001f, e_base, mbits, round) ;
  fprintf(stderr, "%8.8x = fp2q_log1_(%f) [%f]\n", uq, qzero*.00001f, q2fp_log1_(uq, e_base, mbits) ) ;

  uq = fp2q_log1_(qzero*.001f, e_base, mbits, round) ;
  fprintf(stderr, "%8.8x = fp2q_log1_(%f) [%f]\n", uq, qzero*.001f, q2fp_log1_(uq, e_base, mbits) ) ;

  uq = fp2q_log1_(qzero*.1f, e_base, mbits, round) ;
  fprintf(stderr, "%8.8x = fp2q_log1_(%f) [%f]\n", uq, qzero*.1f, q2fp_log1_(uq, e_base, mbits) ) ;

  uq = fp2q_log1_(FLOAT_MIN_ABS(bp), e_base, mbits, round) ;
  fprintf(stderr, "%8.8x = fp2q_log1_(%f) [%f]\n", uq, FLOAT_MIN_ABS(bp), q2fp_log1_(uq, e_base, mbits) ) ;

  uq = fp2q_log1_(+78.5001f, e_base, mbits, round) ;
  fprintf(stderr, "%8.8x = fp2q_log1_(%f) [%f]\n", uq, +78.5001f, q2fp_log1_(uq, e_base, mbits) ) ;

  uq = fp2q_log1_(-78.5001f, e_base, mbits, round) ;
  fprintf(stderr, "%8.8x = fp2q_log1_(%f) [%f]\n", uq, -78.5001f, q2fp_log1_(uq, e_base, mbits) ) ;

  uq = fp2q_log1_(FLOAT_MAX_ABS(bp), e_base, mbits, round) ;
  fprintf(stderr, "%8.8x = fp2q_log1_(%f) [%f]\n", uq, FLOAT_MAX_ABS(bp), q2fp_log1_(uq, e_base, mbits) ) ;

//   fprintf(stderr, "amin = %f, amax = %f, min = %f, max = %f, minq = %8.8x, maxq = %8.8x\n",
//           FLOAT_MIN_ABS(bp), FLOAT_MAX_ABS(bp), FLOAT_MIN_VALUE(bp), FLOAT_MAX_VALUE(bp), INT_MIN_VALUE(bpi), INT_MAX_VALUE(bpi)) ;
}
