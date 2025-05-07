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

#include <rmn/quantizers.h>
#include <rmn/ieee_functions.h>

// ======================= linear quantizers =======================

// compute the discretization quantum exponent from largest value, nbits , max error
// maxabs [IN] : largest absolute value in array
// maxerr [IN] : largest absolute error desired
// nbits  [IN] : max number of bits to use
// return the unbiased power of 2 for the discretization quantum
int32_t fp2q_exp(float maxabs, float maxerr, int32_t nbits){
  int32_t err_exp, min_exp ;
  // the discretization quantum exponent is determined by the larger of 2 values
  // - the first power of 2 <= 2.0 * max error
  // - the first power of 2 <= largest absolute value / 2.0 ** nbits
  nbits = (nbits == 0) ? 24 : nbits ;             // if nbits is 0, set to 24
  nbits = (nbits < 25) ? nbits : 24 ;             // nbits should be <= 24
  err_exp = fp32_exp(maxerr) ;            // exponent from max desired absolute error
  min_exp = fp32_exp(maxabs) - nbits ;    // smallest acceptable value for err_exp
  err_exp = (min_exp > err_exp) ? min_exp : err_exp ;

  return err_exp + 1 ;
}

// linear quantizer for float values
// z   [IN] : 32 bit float float array
// q  [OUT] : 32 bit integer array, result of linear quantification
// n   [IN] : number of values
//ovd  [IN] : inverse of discretization quantum (32 bit float, ideally a power of 2)
// offset [IN] : discretization offset (removed from quantized values)
void fp2q_lin(float *z, int *q, int n, float ovd, int32_t offset){
  int i ;
  for(i=0 ; i<n ; i++) q[i] = fp2q_lin_( z[i], ovd ) - offset ;
}

// linear de_quantizer (inverse of fp2q_lin_1)
// z     [OUT] : 32 bit float float array
// q      [IN] : 32 bit integer array, from linear quantification
// n      [IN] : number of values
// d      [IN] : discretization quantum (32 bit float, ideally a power of 2)
// offset [IN] : discretization offset (to be added to quantized values)
void q2fp_lin(float *z, int *q, int n, float d, int32_t offset){
  int i ;
  for(i=0 ; i<n ; i++) z[i] = q2fp_lin_( q[i] + offset, d) ;
}

// ======================= pseudo log quantizers =======================
#include <stdio.h>
// round is 0 if mbits == 23, 1 << (22 -mbits) if mbits < 23
// this will only work if e_base < 0
uint32_t fp2q_log1_(float z, int32_t e_base, int32_t mbits, uint32_t round){
  union{ uint32_t i ; float f ; } iuf ;
  int32_t mant ;
  int32_t e_z ;
  float mult2 = 1.0f ;

  iuf.f = z ;
  iuf.i += round ;                   // apply rounding (this may increase exponent)

  if(e_base > 0){
    int delta = e_base + 1 ;
    e_base -= delta ;
    mult2 = fp32_pow2(-delta) ;
  }
  float mult = fp32_pow2(-(127+e_base)) ;
  iuf.f *= mult2 ;
  iuf.f *= mult ;                    // |z| < 2.0**e_base will produce a "denorm"
  mant = iuf.i ;
  mant >>= (23 - mbits) ;            // eliminate unwanted bits from mantissa
  return mant ;
}
#if 0
uint32_t fp2q_log_(float z, int32_t e_base, float zero, int32_t mbits, uint32_t round){
  union{ uint32_t u ; float f ; } iuf ;
  uint32_t sign, mant ;
  int32_t e_z ;
//   int32_t e_zero = fp32_exp(zero) ;
//   e_base = (e_zero > e_base) ? e_zero : e_base ;
// fprintf(stderr, ">>>> fp2q_log_ z = %f\n", z);

  iuf.f = z ;
  sign = iuf.u >> 31 ;               // get sign
  iuf.u &= 0x7FFFFFFFu ;             // suppress sign
//   if(iuf.f < zero) return 0 ;
  iuf.u += round ;                   // apply rounding (this may increase exponent)

  float mult = fp32_pow2(-(127+e_base)) ;
// fprintf(stderr, "mult = %G\n", mult) ;
  iuf.f *= mult ;
  mant = iuf.u ;
//   mant = iuf.u & 0x7FFFFFu ;         // extract mantissa (lower 23 bits)
//   e_z  = (iuf.u >> 23) - 127 ;       // get IEEE exponent (without 127 bias)
// fprintf(stderr, "e_z = %d, e_base = %d, diff = %d, mant = %8.8x, mbits = %d\n", e_z, e_base, e_z-e_base, mant, mbits) ;
//   e_z -= e_base ;                    // subtract reference exponent
//   if(e_z < 0) return 0 ;
//   mant |= (e_z << 23) ;              // add exponent difference (never < 0) to mantissa
// fprintf(stderr, "mant = %8.8x\n", mant) ;

  mant >>= (23 - mbits) ;            // eliminate unwanted bits from mantissa
// fprintf(stderr, "mant = %8.8x, sign = %d\n", mant, sign) ;
  mant = (mant << 1) | sign ;        // add sign as LSB
// fprintf(stderr, "mant = %8.8x, sign = %d\n", mant, sign) ;
//   mant += 1 ;                        // add 1 to never produce 0 except for values that would be restored as 0
// fprintf(stderr, "mant = %8.8x\n", mant) ;
//   return (z < zero) ? 0 : mant ;     // encoded version
  return mant ;
}
#endif
// restore float from quantized value (fp2q_log_)
// q      [IN] : quantized value
// e_base [IN] : exponent offset to be applied (does not include IEEE bias)
// mbits  [IN] : number of mantissa bits kept
// return restored float value
// this will only work if e_base < 0
float q2fp_log1_(int32_t q, int32_t e_base, int32_t mbits){
  union{ int32_t i ; float f ; } iuf ;
  float mult2 = 1.0f ;
  if(e_base > 0){
    int delta = e_base + 1 ;
    e_base -= delta ;
    mult2 = fp32_pow2(delta) ;
  }
  float mult = fp32_pow2((127+e_base)) ;
  e_base += 127 ;
  iuf.i = q ;
  iuf.i <<= (23 - mbits) ;             // mantissa in bits 0->22, exp in bits 23->30, sign in bit 31
  iuf.f *= mult2 ;
  iuf.f *= mult ;
  return iuf.f ;
}
#if 0
float q2fp_log_(int32_t q, int32_t e_base, int32_t mbits){
  union{ int32_t i ; float f ; } iuf ;
  int32_t sign ;
  float mult = fp32_pow2((127+e_base)) ;
  e_base += 127 ;
  iuf.i = q ;
  sign  = iuf.i & 1 ;                            // LSB is sign
// fprintf(stderr, "<<<< q2fp_log_ q = %8.8x, e_base = %d, mbits = %d, mult = %G, sign = %d\n", q, e_base, mbits, mult, sign);
  iuf.i >>= 1 ;                                  // get rid of sign

  iuf.i <<= (23 - mbits) ;                       // mantissa in bits 0->22, exp in bits 23->30
  iuf.f *= mult ;
//   iuf.i += (e_base << 23) ;                      // add exponent offset

  iuf.f = (sign != 0) ? (-iuf.f) : iuf.f ;       // restore sign
  return (q == 0) ? 0.0f : iuf.f ;               // q == 0 restores as 0
}
#endif
// qzero [IN] : any z[i] < qzero will be treated as if it were 0
// mbits [IN] : number of mantissa bits to keep (significant bits)
// nexp  [IN} : any z[i] < zmax / (2**nexp) will be treated as if it were 0
// float fp2q_log(float *z, float zmax, int *q, int n, float qzero, int32_t mbits, int32_t nexp){
//   int32_t e_base, e_zero ;
// 
//   e_base  = fp32_exp(zmax) - nexp ;
//   e_zero = fp32_exp(qzero) ;
//   e_zero = (e_zero > e_base) ? e_zero : e_base ;
//   qzero = fp32_pow2(e_zero) ;             // any z[i] < qzero will be treated as 0
// 
//   return qzero ;
// }
