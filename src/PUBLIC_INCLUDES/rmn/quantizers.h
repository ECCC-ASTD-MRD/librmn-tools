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

#if ! defined(FP_QUANTIZE_LIN)

#define FP_QUANTIZE_LIN 0
#define FP_QUANTIZE_LOG 1
#define FP_FAKE_INT     2

#include <stdint.h>
#include <rmn/data_properties.h>

float fp2q_quantum(float maxabs, float maxerr, int32_t nbits);

// linear quantizer for float values
// z    [IN] : 32 bit float
// ovdq [IN] : inverse of discretization quantum (32 bit float, ideally a power of 2)
// return quantized value (32 bit integer) (including proper rounding)
#define FP2QLIN(Z, OVDQ) ( ((Z) * (OVDQ)) + (((Z) < 0) ? -.5f : .5f) )
int32_t fp2q_lin(float *z, int *q, int n, float dq, int32_t offset);

// restore float from linear quantized value (inverse of FP2QLIN)
// q  [IN] : quantized value (32 bit integer)
// dq [IN] : float discretization quantum (ideally a power of 2)
// return restored float value
// dq MUST BE the inverse of ovdq (FP2QLIN)
#define Q2FPLIN(D, DQ) ((D) * (DQ))
void q2fp_lin(float *z, int *q, int n, int32_t e_base, int32_t offset);

int32_t fp2q_log1_(float z, int32_t e_base, int32_t mbits, uint32_t round);
int32_t fp2q_log(float *z, int32_t *q, int n, float vref, int32_t mbits);

float q2fp_log1_(int32_t q, int32_t e_base, int32_t mbits);
void q2fp_log(float *z, int32_t *q, int n, int32_t e_base);

int32_t fp2q_n(float *z, int32_t *q, int n, block_properties *bp, float max_err, int32_t nbits, float max_sig, int32_t *offset, int32_t mode);

int32_t q2fp_n(float *z, int32_t *q, int n, int32_t e_base, int32_t offset, int32_t mode);

void fp2fsi_n(float *z, int32_t *q, int n, int32_t nbits);

void fsi2fp_n(float *z, int32_t *q, int n, int32_t nbits);


#endif
