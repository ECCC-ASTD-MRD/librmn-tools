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

// 32 bit float <--> 32 bit integer  (linear quantization)

#if ! defined(FP_2_INT)

#define FP_2_INT 1

#include <stdint.h>
#include <rmn/move_blocks.h>

// linear quantization for float values
// Z    [IN] : 32 bit float
// OVDQ [IN] : inverse of discretization quantum (32 bit float, ideally a power of 2)
// compute quantized value (32 bit integer), including rounding (toward infinity)
#define FP2QLIN(Z, OVDQ) ( ((Z) * (OVDQ)) + (((Z) < 0) ? -.5f : .5f) )

// functions mostly for internal use
int32_t fp_to_qlin_1(float f, float ovdq);
int32_t fp_to_qlin_n(float *z, int32_t *q, int n, float dq, int32_t offset);
// users should call this function
int32_t fp_to_qlin(float *f, int32_t *q, int n, float max_err, int32_t nbits, int32_t *offset, block_properties *bp);

// restore float value from linear quantized value (inverse of FP2QLIN)
// D  [IN] : quantized value (32 bit integer)
// DQ [IN] : float discretization quantum (ideally a power of 2)
// return restored float value
// DQ MUST BE the inverse of OVDQ (FP2QLIN)
// if an offset is needed, use (D+offset) as D
#define Q2FPLIN(D, DQ) ((D) * (DQ))

// functions mostly for internal use
float qlin_to_fp_1(int32_t d, float dq, int32_t offset);
void qflin_to_fp_n(float *z, int32_t *q, int n, float dq, int32_t offset);
// users should call this function (using e_base, offset from fp_to_qlin)
void qflin_to_fp(float *z, int32_t *q, int n, int32_t e_base, int32_t offset);

float fp_to_from_qlin(float *f, int n, float max_err, int32_t nbits, int32_t *offset);

#endif
