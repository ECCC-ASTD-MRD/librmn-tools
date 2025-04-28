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
