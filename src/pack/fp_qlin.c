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

// 32 bit float <--> 32 bit integer

#include <stdio.h>

#include <rmn/ieee_extras.h>
#include <rmn/fp_qlin.h>
#include <rmn/move_blocks.h>
#include <rmn/data_properties.h>

// constant absolute max error quantizer/de-quantizer

// compute the discretization quantum exponent from largest value, nbits , max error
// maxabs [IN] : largest absolute value in array (set to 0.0f to ignore it)
// abserr [IN] : largest absolute error desired
// nbits  [IN] : max number of bits to use (MUST BE >= 0)
// return the discretization quantum
float fp_to_q_quantum(float maxabs, float abserr, int32_t nbits){
  int32_t err_exp, min_exp ;
  if(nbits < 0) goto fail ;
  // the discretization quantum exponent is determined by the larger of 2 values
  // - the first power of 2 <= 2.0 * max error
  // - the first power of 2 <= largest absolute value / 2.0 ** nbits
  nbits = (nbits == 0) ? 24 : nbits ;     // if nbits is 0, set to 24
  nbits = (nbits < 25) ? nbits : 24 ;     // nbits should be <= 24
  err_exp = fp32_exp(abserr) ;            // exponent from max desired absolute error
  min_exp = fp32_exp(maxabs) - nbits ;    // smallest acceptable value for err_exp
  err_exp = (min_exp > err_exp) ? min_exp : err_exp ;

  // quantum is twice the max absolute error (add 1 to exponent of error)
  if(err_exp > -127) return fp32_pow2(err_exp + 1) ;  // err_exp <= -127 is too small a value

fail :
  return 0.0f ;
}

// ================================== quantize ==================================

// linear quantizer for float values
// f      [IN] : 32 bit float value
// dq     [IN] : discretization quantum (float, should be a power of 2)
// return the quantized value
int32_t fp_to_qlin_1(float f, float dq){
  float ovdq = fp32_pow2(-fp32_exp(dq)) ;        // 1.0 / dq
  return FP2QLIN(f, ovdq) ;                      // uses FP2QLIN from rmn/fp_qlin.h.h
}

// linear quantizer for an array of float values
// f      [IN] : 32 bit float array
// q     [OUT] : 32 bit integer array, result of quantification
// n      [IN] : number of values
// dq     [IN] : discretization quantum (float, will be truncated down to a power of 2)
// offset [IN] : discretization offset (subtracted from quantized values, often 0)
// return biased exponent of quantum
int32_t fp_to_qlin_n(float *f, int32_t *q, int n, float dq, int32_t offset){
  if(dq == 0.0f) return -1 ;                   // infinity
  float ovdq = fp32_pow2(-fp32_exp(dq)) ;        // 1.0 / dq
  int i ;
  // quantization loop, uses FP2QLIN from rmn/fp_qlin.h
  for(i=0 ; i<n ; i++) q[i] = FP2QLIN( f[i], ovdq ) - offset ;
  return (fp32_exp(dq) + 127) ;                  // add IEEE bias to exponent
}

// linear quantizer for an array of float values
// f         [IN] : 32 bit float array
// q        [OUT] : 32 bit integer array, result of quantification
// n         [IN] : number of values
// max_err   [IN] : max absolute error desired (0 means use nbits only)
// nbits     [IN] : number of bits desired for quantized value (0 means use max_err only)
// offset [INOUT] : discretization offset (subtracted from quantized values, often 0)
//                  offset == 0x7FFFFFFF means use quantized minimum as offset
// return biased exponent of quantum, -1 in case of error
int32_t fp_to_qlin(float *f, int32_t *q, int n, float max_err, int32_t nbits, int32_t *offset, block_properties *bp){
  float dq ;
  if(max_err < 0.0f || nbits < 0) return -1 ;         // negative values not allowed
  block_properties lbp ;
  float max_abs, min_val ;

  lbp.kind = bad_data ;
  if(*offset == 0x7FFFFFFF){                          // automatic offset, need properties of f
    if(bp == NULL) { bp = &lbp ; }                    // use local copy
    if(! data_kind_valid(bp->kind)){                  // if the data properties are not valid
      analyze_data32_block((void *)f, n, n, 1, bp) ;  // get data properties
      adjust_block_properties(bp, float_data) ;       // adjust properties for float data
    }
  }

  if(max_err != 0.0f && nbits == 0){                  // max_err explicitely specified, nbits == 0
    dq = max_err * 2.0f ;
  }else{                                              // max_err == 0, nbits > 0
    if(max_err == 0.0f && nbits == 0) return -1 ;     // cannot both be 0
    if(bp == NULL) { bp = &lbp ; }                    // use local copy
    if(bp->kind != float_data){                       // no valid properties available
      analyze_data32_block((void *)f, n, n, 1, bp) ;  // get data properties
      adjust_block_properties(bp, float_data) ;       // adjust properties for float data
      max_abs = FLOAT_MAX_ABS(*bp) ;                  // float with largest absolute value
    }
    dq = fp_to_q_quantum(max_abs, max_err, nbits) ;   // compute quantum
    if(dq == 0.0f) return -1 ;
  }
  if(*offset == 0x7FFFFFFF){
    min_val = FLOAT_MIN_VALUE(*bp) ;                   // signed minimum float value from array f
    *offset = fp_to_qlin_1(min_val, dq) ;             // offset = quantized value of signed minimum
  }
  return fp_to_qlin_n(f, q, n, dq, *offset) ;         // perform linear quantization
}

// ================================== restore ==================================

// restore float value from linear quantized value (inverse of fp_to_qlin)
// q      [IN] : quantized value (32 bit integer)
// dq     [IN] : float discretization quantum (ideally a power of 2)
// offset [IN] : discretization offset (from fp_to_qlin)
// return restored float value
// dq MUST BE SAME VALUE that was used by fp_to_qlin/fp_to_qlin_n
float qlin_to_fp_1(int32_t q, float dq, int32_t offset){
  return Q2FPLIN( (q + offset), dq ) ;
}

// restore float values from linear quantized value (inverse of fp_to_qlin)
// f     [OUT] : restored float array (32 bit floats)
// q      [IN] : 32 bit integer array of quantized values
// n      [IN] : number of values
// dq     [IN] : discretization quantum (SAME VALUE used by fp_to_qlin/fp_to_qlin_n)
// offset [IN] : discretization offset (from fp_to_qlin)
void qflin_to_fp_n(float *f, int32_t *q, int n, float dq, int32_t offset){
  int i ;
  for(i=0 ; i<n ; i++) f[i] = Q2FPLIN( (q[i] + offset), dq ) ;
}
// f     [OUT] : restored float array (32 bit floats)
// q      [IN] : 32 bit integer array of quantized values
// n      [IN] : number of values
// e_base [IN] : IEEE exponent for discretization quantum (from fp_to_qlin)
// offset [IN] : discretization offset (from fp_to_qlin)
void qflin_to_fp(float *f, int32_t *q, int n, int32_t e_base, int32_t offset){
  float dq = fp32_pow2(e_base - 127) ;        // remove exponent bias
  qflin_to_fp_n(f, q, n, dq, offset) ;
}

// ================================== difference test ==================================

float fp_to_from_qlin(float *f, int n, float max_err, int32_t nbits, int32_t *offset){
  int32_t i, q[n] ;
  float t, maxdiff = 0.0f, r[n] ;

  int32_t e_base = fp_to_qlin(f, q, n, max_err, nbits, offset, NULL) ;
  qflin_to_fp(r, q, n, e_base, *offset) ;
  for(i=0 ; i<n ; i++){
    t = r[i] - f[i] ;
    t = (t < 0.0f) ? (-t) : t ;
    maxdiff = (t > maxdiff) ? t : maxdiff ;
  }
  return maxdiff ;
}
