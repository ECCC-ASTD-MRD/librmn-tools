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

float fp_fudge1(float f, int nbits){
  union{ uint32_t u ; float f ; } uf ;
  uf.f = f ;
  int32_t mask = -1 ;
  mask <<= (23 - nbits) ;
  uf.u &= mask ;                // get rid of lower 23 - nbits bits
  uf.u |= (1 << (22-nbits)) ;   // set bit below upper nbits bits of mantissa to 1
  return uf.f ;
}

float fp_fudge2(float f, int nbits, int lsbs){
  union{ uint32_t u ; float f ; } uf ;
  uf.f = f ;
  int32_t mask = -1 ;
  mask <<= (23 - nbits) ;
  uf.u &= mask ;                // get rid of lower 23 - nbits bits
  uf.u |= lsbs ;                // set some LSBs of mantissa
  return uf.f ;
}

#define ABS(x) (((x) < 0) ? (-(x)) : (x))

void verify_log(float r[NJ][NI], int32_t q[NJ][NI], int nij, int32_t e_base, float Z[NJ][NI]){
//   int32_t e_base0 = (e_base >> 8) - 127 ;
  int32_t e_base0 = (e_base & 0xFF) - 127 ;
//   int32_t mbits = e_base >> 8 ;
  int i, j, n = 0, erri, errj, zero = 0, qi ;
  float errmax = 0.0f, vmax = 999999.0f, verr = 999999.0f, errpt = 0.0f, vrest = 999999.0f ;
  float vref = fp32_pow2(e_base0 + 0) ;
// fprintf(stderr,"verify_log : vref = %f, e_base0 = %d, nij = %d\n", vref, e_base0, nij) ;
  q2fp_log((float *)r, (int32_t *)q, nij, e_base) ;
//   q2fp_n((void *)r, (void *)q, n, e_base, 0, FP_QUANTIZE_LOG) ;
  for(j=0 ; j<NJ ; j++){
    for(i=0 ; i<NI ; i++){
      float err ;
      float absz = (Z[j][i] < 0) ? -Z[j][i] : Z[j][i] ;
      if(Z[j][i] != 0 && r[j][i] == 0) zero++ ;
      if(absz < vref) continue ;
      n++ ;
      err = 0.0 ;
//       if(Z[j][i] != 0 && r[j][i] != 0) err = (Z[j][i] - r[j][i]) / Z[j][i] ;
      if(Z[j][i] != 0) err = (Z[j][i] - r[j][i]) / Z[j][i] ;      // relative error
      err = (err <0) ? (-err) : err ;
      if(err > errmax){
        errmax = err ;
        vmax = Z[j][i] ;
        verr = r[j][i] - Z[j][i] ;
        vrest = r[j][i] ;
        erri = i ;
        errj = j ;
        errpt = r[j][i] ;
        qi = q[j][i] ;
      }
    }
  }
  fprintf(stderr,"errmax = %f (1 part in %5.0f), vmax = %f, verr = %f, vref = %f, n = %d/%d, r[%d,%d]=%f, %d zeros\n\n", errmax, 1.0f/errmax, vmax, verr, vref, n, NI*NJ, erri, errj, errpt, zero) ;
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
  int32_t e_base, e_base0 ;
  char *msg = "generic error" ;
fprintf(stderr, "_MM_GET_ROUNDING_MODE = %8.8x, %8.8x, %8.8x, %8.8x, %8.8x\n", _MM_GET_ROUNDING_MODE(),_MM_ROUND_NEAREST, _MM_ROUND_DOWN, _MM_ROUND_UP, _MM_ROUND_TOWARD_ZERO );

  int32_t csrmode ;
  csrmode = fp32_disallow_denorm() ;
  fprintf(stderr, "csr = %8.8x, ", csrmode) ;
  csrmode = fp32_allow_denorm() ;
  fprintf(stderr, "csr = %8.8x, ", csrmode) ;
  csrmode = get_cpu_csr() ;
  fprintf(stderr, "csr = %8.8x\n", csrmode) ;

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
  nij = 0 ;
  for(j=0 ; j<NJ ; j++){
    for(i=0 ; i<NI ; i++){
      z[j][i] = (i - (NI-1)*.5f) + (j - (NJ-1)*.5f) + .5 ;
//       Z[j][i] = z[j][i] * ((z[j][i] < 0) ? (-z[j][i]) : z[j][i] ) ;
      Z[j][i] = z[j][i] * z[j][i] * z[j][i] * 1.9f ;
      r[j][i] = 999999.0f ;
      nij++ ;
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
  status = q2fp_n((void *)r, (void *)q, nij, e_base, offset, mode) ;
  msg = "status from q2fp_n not 0" ;
  if(status != 0) goto fail ;

  nij = 0 ;
  errmax = 0.0f ;
  target = 0.5f*quantum ;
  for(j=0 ; j<NJ ; j++){
    for(i=0 ; i<NI ; i++){
      err = z[j][i] - r[j][i] ;
      err = (err < 0) ? (-err) : err ;
      errmax = (err > errmax) ? err : errmax ;
      nij++ ;
    }
  }
  fprintf(stderr, "%d values : min = %f, max = %f, errmax = %f, target = %f, status = %d\n", nij, FLOAT_MIN_VALUE(bp), FLOAT_MAX_VALUE(bp), errmax, target, status) ;
  msg = "errmax > target" ;
  if(errmax > target) goto fail ;

  fprintf(stderr, "============================== linear timingss ==============================\n") ;

  niter = 100 ;
  TIME_LOOP_EZ(niter, nij, e_base = fp2q_lin((void *)z, (void *)q, nij, quantum, offset)) ;
  fprintf(stderr,"quantize : %s\n", timer_msg) ;

  TIME_LOOP_EZ(niter, nij, q2fp_lin((void *)z, (void *)q, nij, e_base, offset)) ;
  fprintf(stderr,"restore  : %s\n", timer_msg) ;

  fprintf(stderr, "SUCCESS\n");

  fprintf(stderr, "============================== pseudo log quantizers ==============================\n") ;

  int32_t mbits = 11, mbits0, e_range ;
  uint32_t round = (1 << (22 - mbits)) ;
  float vref = 32.0f ;

  int32_t uq ;
  float ur ;

  nij = analyze_data32_block((void *)Z, NI, NI, NJ, &bp) ;
  adjust_block_properties(&bp, float_data) ;
  print_float_props(bp);

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

  fprintf(stderr, "vref = %f, mbits = %d\n\n", vref, mbits) ;

  float vref0 = vref*4.0f, vref1, vref2 ;

  vref2 = fp_fudge2(vref, mbits, 0x1) ; ;
  uq = fp2q_log1_(vref2, e_base, mbits, round) ; ur = q2fp_log1_(uq, e_base, mbits) ;
  fprintf(stderr, "%8.8x = fp2q_log1_(%12.8f) [%12.8f] [%8.0f]\n", uq, vref2, ur, ABS(vref2/(ur-vref2)) ) ;

  vref1 = fp_fudge1(vref0, mbits) ;           // fudge for worst case error
  uq = fp2q_log1_(vref1, e_base, mbits, round) ; ur = q2fp_log1_(uq, e_base, mbits) ;
  fprintf(stderr, "%8.8x = fp2q_log1_(%12.8f) [%12.8f] [%8.0f]\n", uq, vref1, ur, ABS(vref1/(ur-vref1)) ) ;
  vref2 = fp_fudge2(vref0, mbits, 0x1) ;      // fudge for smallest error
  uq = fp2q_log1_(vref2, e_base, mbits, round) ; ur = q2fp_log1_(uq, e_base, mbits) ;
  fprintf(stderr, "%8.8x = fp2q_log1_(%12.8f) [%12.8f] [%8.0f]\n", uq, vref2, ur, ABS(vref2/(ur-vref2)) ) ;
  for(i=mbits+3 ; i>0 ; i-=3){
    fprintf(stderr, "\n") ;
    vref0 *= .125f ;
    vref1 = fp_fudge1(vref0, mbits) ;         // fudge for worst case error
    uq = fp2q_log1_(vref1, e_base, mbits, round) ; ur = q2fp_log1_(uq, e_base, mbits) ;
    fprintf(stderr, "%8.8x = fp2q_log1_(%12.8f) [%12.8f] [%8.0f]\n", uq, vref1, ur, ABS(vref1/(ur-vref1)) ) ;
    vref2 = fp_fudge2(vref0, mbits, 0x1) ;    // fudge for smallest error
    uq = fp2q_log1_(vref2, e_base, mbits, round) ; ur = q2fp_log1_(uq, e_base, mbits) ;
    fprintf(stderr, "%8.8x = fp2q_log1_(%12.8f) [%12.8f] [%8.0f]\n", uq, vref2, ur, ABS(vref2/(ur-vref2)) ) ;
  }
  fprintf(stderr, "\n") ;

  vref2 = FLOAT_MIN_ABS(bp) ;
  uq = fp2q_log1_(vref2, e_base, mbits, round) ; ur = q2fp_log1_(uq, e_base, mbits) ;
  fprintf(stderr, "%8.8x = fp2q_log1_(%12.8f) [%12.8f] [%8.0f]\n", uq, vref2, ur, ABS(vref2/(ur-vref2)) ) ;

  vref2 = +78.5001f ;
  uq = fp2q_log1_(vref2, e_base, mbits, round) ; ur = q2fp_log1_(uq, e_base, mbits) ;
  fprintf(stderr, "%8.8x = fp2q_log1_(%12.8f) [%12.8f] [%8.0f]\n", uq, vref2, ur, ABS(vref2/(ur-vref2)) ) ;

  vref2 = -78.5001f ;
  uq = fp2q_log1_(vref2, e_base, mbits, round) ; ur = q2fp_log1_(uq, e_base, mbits) ;
  fprintf(stderr, "%8.8x = fp2q_log1_(%12.8f) [%12.8f] [%8.0f]\n", uq, vref2, ur, ABS(vref2/(ur-vref2)) ) ;

  vref2 = FLOAT_MAX_VALUE(bp) ;
  uq = fp2q_log1_(vref2, e_base, mbits, round) ; ur = q2fp_log1_(uq, e_base, mbits) ;
  fprintf(stderr, "%8.8x = fp2q_log1_(%12.8f) [%12.8f] [%8.0f]\n", uq, vref2, ur, ABS(vref2/(ur-vref2)) ) ;

  vref2 = FLOAT_MIN_VALUE(bp) ;
  uq = fp2q_log1_(vref2, e_base, mbits, round) ; ur = q2fp_log1_(uq, e_base, mbits) ;
  fprintf(stderr, "%8.8x = fp2q_log1_(%12.8f) [%12.8f] [%8.0f]\n", uq, vref2, ur, ABS(vref2/(ur-vref2)) ) ;

  fprintf(stderr, "\n") ;

// testing generic function in LOG mode (verify_log uses generic function to restore)

  e_base0 = fp2q_n((float *)Z, (int32_t *)q, nij, NULL, 0.00003f, mbits,  vref, NULL, FP_QUANTIZE_LOG) ;
  verify_log((void *)r, (void *)q, nij, e_base0, (void *)Z) ;

  e_base0 = fp2q_n((float *)Z, (int32_t *)q, nij, NULL, 0.0f,     mbits,  vref, NULL, FP_QUANTIZE_LOG) ;
  verify_log((void *)r, (void *)q, nij, e_base0, (void *)Z) ;

  e_base0 = fp2q_n((float *)Z, (int32_t *)q, nij, NULL, 0.0005f, 0,       vref, NULL, FP_QUANTIZE_LOG) ;
  verify_log((void *)r, (void *)q, nij, e_base0, (void *)Z) ;

  e_base0 = fp2q_n((float *)Z, (int32_t *)q, nij, NULL, 0.0005f, mbits-1, vref/2, NULL, FP_QUANTIZE_LOG) ;
  verify_log((void *)r, (void *)q, nij, e_base0, (void *)Z) ;

  e_base0 = fp2q_n((float *)Z, (int32_t *)q, nij, NULL, 0.00003f, mbits, -vref, NULL, FP_QUANTIZE_LOG) ;
  verify_log((void *)r, (void *)q, nij, e_base0, (void *)Z) ;

  e_base0 = fp2q_n((float *)Z, (int32_t *)q, nij, NULL, 0.00003f, mbits,  vref, NULL, FP_QUANTIZE_LOG) ;
  block_properties bpi ;
  nij = analyze_data32_block((void *)q, NI, NI, NJ, &bpi) ;
  adjust_block_properties(&bpi, int_data) ;
  print_int_props(bpi);

// return 0 ;
  fprintf(stderr, "============================== pseudo log timingss ==============================\n") ;

  niter = 100 ;
  TIME_LOOP_EZ(niter, nij, e_base0 = fp2q_log((float *)Z, (int32_t *)q, nij, vref, mbits)) ;
  fprintf(stderr,"quantize : %s\n", timer_msg) ;

  TIME_LOOP_EZ(niter, nij, q2fp_log((float *)r, (int32_t *)q, nij, e_base0)) ;
  fprintf(stderr,"restore  : %s\n", timer_msg) ;

  fprintf(stderr, "SUCCESS\n");

  goto end ;
}
