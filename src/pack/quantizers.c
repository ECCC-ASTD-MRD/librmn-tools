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

#include <stdio.h>
#include <stdlib.h>

#include <rmn/quantizers.h>
#include <rmn/ieee_functions.h>
#include <rmn/move_blocks.h>

// ======================= linear quantization =======================

// constant absolute max error quantizer/de-quantizer

// compute the discretization quantum exponent from largest value, nbits , max error
// maxabs [IN] : largest absolute value in array (set to 0.0f to ignore it)
// maxerr [IN] : largest absolute error desired
// nbits  [IN] : max number of bits to use
// return the discretization quantum
float fp2q_quantum(float maxabs, float maxerr, int32_t nbits){
  int32_t err_exp, min_exp ;
  if(nbits < 0) goto fail ;
  // the discretization quantum exponent is determined by the larger of 2 values
  // - the first power of 2 <= 2.0 * max error
  // - the first power of 2 <= largest absolute value / 2.0 ** nbits
  nbits = (nbits == 0) ? 24 : nbits ;     // if nbits is 0, set to 24
  nbits = (nbits < 25) ? nbits : 24 ;     // nbits should be <= 24
  err_exp = fp32_exp(maxerr) ;            // exponent from max desired absolute error
  min_exp = fp32_exp(maxabs) - nbits ;    // smallest acceptable value for err_exp
  err_exp = (min_exp > err_exp) ? min_exp : err_exp ;

  // quantum is twice the max absolute error (add 1 to exponent of error)
  if(err_exp > -127) return fp32_pow2(err_exp + 1) ;  // err_exp <= -127 is too small a value

fail:
  return 0.0f ;
}

// linear quantizer for float values
// z      [IN] : 32 bit float float array
// q     [OUT] : 32 bit integer array, result of quantification
// n      [IN] : number of values
// dq     [IN] : discretization quantum (float, will be truncated down to a power of 2)
// offset [IN] : discretization offset (removed from quantized values)
int32_t fp2q_lin(float *z, int *q, int n, float dq, int32_t offset){
  int32_t e_base = fp32_exp(dq) ;
  dq = fp32_pow2(-e_base) ;     // 1.0 / dq
  int i ;
  for(i=0 ; i<n ; i++) q[i] = fp2q_lin_( z[i], dq ) - offset ;
  return e_base ;
}

// linear de_quantizer (inverse of fp2q_lin_1)
// z     [OUT] : restored 32 bit float float array
// q      [IN] : 32 bit integer array, from linear quantification
// n      [IN] : number of values
// e_base [IN] : discretization quantum exponent (from fp2q_lin)
// offset [IN] : discretization offset (to be added to quantized values)
void q2fp_lin(float *z, int *q, int n, int32_t e_base, int32_t offset){
  int i ;
  float d = fp32_pow2(e_base) ;
  for(i=0 ; i<n ; i++) z[i] = q2fp_lin_( q[i] + offset, d) ;
}

// ======================= pseudo log quantization =======================

// constant relative max error quantizer/de-quantizer
// transforms the float into a "piecewise linear" log represented as an integer
// actual log for pure powers of 2, linear between 2 pure powers
// the mbits least significant bits are the upper mbits of the mantissa
// the most significant bits reflect the IEEE exponent

// z      [IN] : 32 bit float float
// n      [IN] : number of values
// e_base [IN] : power of 2 <= smallest significant value
// mbits  [IN] : number of mantissa bits to keep
// round  [IN] : normally 0 if mbits == 23, 1 << (22 -mbits) if mbits < 23
// return integer result of quantization (sign, reduced exponent, reduced mantissa)
// for values < 2.0**e_base, a "denormalized" style result is produced
// N.B. some aggressive optimization by compilers may result in an attempt to combine
//      the two multipliers into a single one, mult1*mult2 with potentially disastrous results
int32_t fp2q_log1_(float z, int32_t e_base, int32_t mbits, uint32_t round){
  union{ int32_t i ; float f ; } iuf ;
  int32_t q, sign ;

  float mult2 = 1.0f ;                    // "neutral" multiplier 2
  if(e_base > 0){                         // single multiplier would be too large for float format
    int delta = e_base + 1 ;
    e_base -= delta ;                     // adjust e_base
    mult2 = fp32_pow2(-delta) ;           // multiplier 2
  }
  float mult = fp32_pow2(-(127+e_base)) ; // multiplier 1

  iuf.f = z ;
  sign = iuf.i >> 31 ;                    // capture sign
  iuf.i &= 0x7FFFFFFFu ;                  // take absolute value
  iuf.i += round ;                        // apply rounding (this may increase exponent)
  iuf.f *= mult2 ;                        // apply multipliers
  iuf.f *= mult ;                         // |z| < 2.0**e_base will produce a "denorm"
  q = iuf.i >> (23 - mbits) ;             // eliminate unwanted bits from mantissa
  q = (q ^ sign) - sign ;                 // restore sign (2's complement formula)
  return q ;
}

// z     [IN] : float values to be quantized
// q    [OUT] : quantized values
// n     [IN] : number of values
// vsig  [IN] : |values| < |vsig| will start losing significant bits in mantissa and may become 0
// mbits [IN] : number of mantissa bits to keep
// return the exponent base to be used for restoring floats from quantized values (passed to q2fp_log)
// N.B. some aggressive optimization by compilers may result in an attempt to combine
//      the two multipliers into a single one, mult1*mult2 with potentially disastrous results
// the biased IEEE exponent of a float value equal to vsig will be brought down to 0 by multiplying
// that value with appropriate mulipliers.
// any value having an IEEE exponent smaller than vsig will end as a "denormalized" float
// if the absolute value is less than vsig / 2**mbits, the result will be 0
int32_t fp2q_log(float *z, int32_t *q, int n, float vsig, int32_t mbits){
  union{ int32_t i ; float f ; } iuf ;
  int32_t sign, i, round ;
  int32_t e_base = fp32_exp(vsig) ;       // unbiased exponent from vsig
  int32_t e_ret = e_base ;

  // rounding term
  if(mbits < 23){                         // less than full mantissa
    round = (1 << (22 - mbits)) ;         // add 1 below last mantissa bit kept
  }else{
    mbits = 23 ;                          // full mantissa (23 bits)
    round = 0 ;
  }
  // multiplier(s)
  float mult2 = 1.0f ;                    // "neutral" multiplier 2
  if(e_base > 0){                         // single multiplier would be too large for float format
    int delta = e_base + 1 ;
    e_base -= delta ;                     // adjust e_base
    mult2 = fp32_pow2(-delta) ;           // multiplier 2
  }
  float mult = fp32_pow2(-(127+e_base)) ; // multiplier 1
  // discretization (quantization) loop
  // values having an IEEE exponent equal to the IEEE exponent of vsig will end up
  // with an IEEE exponent of 0 (denorm format)
  // values < vsig / 2.0 ** mbits will end up as 0
  for(i=0 ; i<n ; i++){
    iuf.f = z[i] ;
    sign = iuf.i >> 31 ;                    // capture sign (-1 or 0)
    iuf.i &= 0x7FFFFFFFu ;                  // take absolute value
    iuf.i += round ;                        // apply rounding (this may increase exponent)
    iuf.f *= mult2 ;                        // apply multipliers
    iuf.f *= mult ;                         // |z| < 2.0**e_base will produce a "denorm"
    q[i] = iuf.i >> (23 - mbits) ;          // eliminate unwanted bits from mantissa
    q[i] = (q[i] ^ sign) - sign ;           // restore sign (2's complement formula)
  }
  return e_ret ;
}

// restore float from quantized value (fp2q_log_)
// q      [IN] : quantized value
// e_base [IN] : exponent offset to be applied (does not include IEEE bias)
// mbits  [IN] : number of mantissa bits kept
// return restored float value
// this will only work if e_base < 0
// N.B. some aggressive optimization by compilers may result in an attempt to combine
//      the two multipliers into a single one, mult1*mult2 with potentially disastrous results
float q2fp_log1_(int32_t q, int32_t e_base, int32_t mbits){
  union{ int32_t i ; float f ; } iuf ;

  float mult2 = 1.0f ;                    // "neutral" multiplier 2
  if(e_base > 0){                         // exponent would be too large for single multiplier
    int delta = e_base + 1 ;
    e_base -= delta ;                     // adjust e_base
    mult2 = fp32_pow2(delta) ;            // multiplier 2
  }
  float mult = fp32_pow2((127+e_base)) ;  // multiplier 1

  int32_t sign = q >> 31 ;                // capture sign
  iuf.i = (q ^ sign) - sign ;             // take absolute value (2's complement formula)
  iuf.i <<= (23 - mbits) ;                // mantissa in bits 0->22, exp in bits 23->30, sign in bit 31
  iuf.f *= mult2 ;                        // allpy multipliers
  iuf.f *= mult ;
  iuf.i |= (sign << 31) ;                 // restore sign
  return iuf.f ;
}

// restore floats from quantized values (inverse of fp2q_log_)
// z     [OUT] : restored values
// q      [IN] : quantized values
// n      [IN] : number of values
// e_base [IN] : exponent offset to be applied (from fp2q_logn_)
// mbits  [IN] : number of mantissa bits kept
// N.B. some aggressive optimization by compilers may result in an attempt to combine
//      the two multipliers into a single one, mult1*mult2 with potentially disastrous results
void q2fp_log(float *z, int32_t *q, int n, int32_t e_base, int32_t mbits){
  union{ int32_t i ; float f ; } iuf ;
  float mult2 = 1.0f ;                    // "neutral" multiplier 2

  if(e_base > 0){                         // exponent would be too large for single multiplier
    int32_t delta = e_base + 1 ;
    e_base -= delta ;                     // adjust e_base
    mult2 = fp32_pow2(delta) ;            // multiplier 2
  }
  float mult = fp32_pow2((127+e_base)) ;  // multiplier 1

  int i ;
  for(i=0 ; i<n ; i++){
    int32_t sign = q[i] >> 31 ;                // capture sign
    iuf.i = (q[i] ^ sign) - sign ;             // take absolute value (2's complement formula)
    iuf.i <<= (23 - mbits) ;                // mantissa in bits 0->22, exp in bits 23->30, sign in bit 31
    iuf.f *= mult2 ;                        // apply multipliers
    iuf.f *= mult ;
    iuf.i |= (sign << 31) ;                 // restore sign
    z[i] = iuf.f ;
  }
}

// =======================  generic functions =======================

// 32 bit pseudo log type float quantizer
// z         [IN] : float values to be quantized
// q        [OUT] : quantized values
// n         [IN] : number of values
// bp        [IN] : block properties fo z (min, max, ...)
// maxerr    [IN] : largest absolute error desired (linear quantizer)
//                  largest relative error desired (pseudo log quantizer)
//                  if max_err == 0.0f, it will be computed using nbits
// nbits     [IN] : number of bits for quantized value (linear quantizer)
//                  number of mantissa bist to keep (pseudo log quantizer)
//                  if nbits == 0 , it will be computed using maxerr
// max_sig   [IN] : smallest value considered significant (pseudo log quantizer only)
// offset [INOUT] : offset used in qunatification (linear quantizer only)
// mode      [IN] : 0 (FP_QUANTIZE_LIN) linear
//                  1 (FP_QUANTIZE_LOG) pseudo log
// for the pseudo log quantizer, all absolute values >= max_sig will have the precision requested via max_err
// all absolute values < max_sig will lose precision and eventually become zero
int32_t fp2q_n(float *z, int32_t *q, int n, block_properties *bp, float max_err, int32_t nbits, float max_sig, int32_t *offset, int32_t mode){
  float max_abs, min_abs, min_val, quantum ;
//   int32_t e_base ;
  int32_t result, e_min, e_max, e_sig, e_err ;
  block_properties bp0 ;
//   union{ uint32_t u ; float f ; } uf ;

  if(nbits == 0 && max_err == 0.0f) goto fail ;
  if(max_err < 0 || nbits < 0)      goto fail ;

  if(bp == NULL){ bp = &bp0 ;  bp0.kind = bad_data ; }
  if(! data_kind_valid(bp->kind)){                 // if the data properties are not valid
    analyze_data32_block((void *)z, n, n, 1, bp) ; // get data block properties
    adjust_block_properties(bp, float_data) ;      // adjust properties for float data
  }
  if(bp->kind != float_data) goto fail ;
  max_abs = FLOAT_MAX_ABS(*bp) ;

  switch(mode){
    case 0:        // linear quantizer, uses max_abs, max_err, offset, nbits
      quantum = fp2q_quantum(max_abs, max_err, nbits) ;    // compute quantum
      if(quantum == 0.0f) goto fail ;
      if(*offset == 0x7FFFFFFF){                   // flag to set offset to quantized minimum
        int32_t e_base = fp32_exp(quantum) ;
        float ovq = fp32_pow2(-e_base) ;           // 1.0 / quantum
        min_val = FLOAT_MIN_VALUE(*bp) ;
        *offset = fp2q_lin_(min_val, ovq) ;        // quantized value of minimum value in array
      }
      result = fp2q_lin((void *)z, (void *)q, n, quantum, *offset) ;
      break ;
    case 1:        // pseudo log quantizer, uses min_abs, max_abs, max_err, max_sig, nbits
      min_abs = FLOAT_MIN_ABS(*bp) ;
      e_err = fp32_exp_raw(max_err) ;              // raw (biased) exponent (MUST BE < 127)
fprintf(stderr, "fp2q_n : max_err = %f, nbits = %d, max_sig = %f, min_abs = %f\n", max_err, nbits, max_sig, min_abs) ;
      if(nbits == 0){
        nbits = 127 - e_err ;                      // need enough mantissa bits to satisfy error criteria
        if(nbits > 23) nbits = 23 ;
        if(nbits < 1) goto fail ;                  // max_err was >= 1.0
      }
      if(e_err == 0){                              // nbits != 0, max_err == unnormalized, assumed as 0
        e_err = (127 - nbits) ;                    // set max relative error to 2 ** -nbits
      }
      e_err = e_err - 127 ;                        // unbiased exponent
      if(nbits > (-e_err)) nbits = (-e_err) ;

      e_min = fp32_exp_raw(min_abs) ;              // raw (biased) exponent
      e_max = fp32_exp_raw(max_abs) ;              // raw (biased) exponent
      e_sig = fp32_exp_raw(max_sig) ;              // raw (biased) exponent
      if(e_sig < e_min) e_sig = e_min ;
      if(max_sig < 0){     // quantized value of max_sig will be 0
fprintf(stderr, "fp2q_n : e_sig = %d, nbits = %d, maxabs = %f", e_sig, nbits, max_abs) ;
        e_sig += (nbits -1) ;
        if(e_sig > e_max) e_sig = e_max -1 ;  // e_sig cannot be greater than e_max - 1
fprintf(stderr, ", e_sig = %d, e_max = %d\n", e_sig, e_max) ;
      }
      max_sig = fp32_pow2(e_sig - 127) ;
fprintf(stderr, "fp2q_n : max_err = %f, nbits = %d, max_sig = %f, min_abs = %f\n", fp32_pow2(-nbits), nbits, max_sig, min_abs) ;
      result = fp2q_log((void *)z, (void *)q, n, max_sig, nbits) ;
      break ;
    default:       // ERROR
      goto fail ;
  }
  return result ;

fail:
fprintf(stderr, "fp2q_n : FAILED\n") ;
fprintf(stderr, "fp2q_n : max_err = %f, nbits = %d, max_sig = %f, min_abs = %f\n", fp32_pow2(-nbits), nbits, max_sig, min_abs) ;
  return 0x7FFFFFFF ;  // huge value, ERROR
}

// 32 bit float de-quantizer
int32_t q2fp_n(float *z, int32_t *q, int n, int32_t e_base, int32_t mbits, int32_t offset, int32_t mode){
  switch(mode){
    case 0:        // linear de-quantizer, uses e_base, offset
      q2fp_lin((void *)z, (void *)q, n, e_base, offset) ;
      break ;
    case 1:        // pseudo log de-quantizer, uses e_base, mbits
      q2fp_log((void *)z, (void *)q, n, e_base, mbits) ;
      break ;
    default:
      return 1 ;
  }
  return 0 ;
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
