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

// ======================= linear quantizers =======================

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
