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

#if ! defined(QUANTIZERS_DEFINED)
#define QUANTIZERS_DEFINED

#include <stdint.h>

int32_t fp2q_exp(float maxabs, float maxerr, int32_t nbits);

// linear quantizer for float values
// z      [IN] : 32 bit float
//ovd     [IN] : inverse of discretization quantum (32 bit float, ideally a power of 2)
// return quantized value (32 bit integer)
static inline int32_t fp2q_lin_(float z, float ovd){
  int32_t t = (z * ovd) + ((z < 0) ? -.5f : .5f) ;
  return t ;
}

void fp2q_lin(float *z, int *q, int n, float ovd, int32_t offset);

void q2fp_lin(float *z, int *q, int n, float d, int32_t offset);

// linear de_quantizer (inverse of fp2q_lin_)
// q [IN] : quantized value (32 bit integer)
// d [IN] : float discretization quantum
// return restored float value
static inline float q2fp_lin_(int32_t q, float d){
  float t = q * d ;
  return t ;
}

#endif
