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

// 32 bit float <--> 32 bit integer fake log

#include <stdint.h>

#include <rmn/extra_bf16.h>

#if ! defined(FP_2_FLOG)

#define FP_2_FLOG 2
#define FP_2_QLOG 3

void fp_to_flog(float * restrict z, int32_t * restrict q, int n, int32_t nbits);     // 32 bit IEEE floats
void flog_to_fp(float * restrict z, int32_t * restrict q, int n, int32_t nbits);

void e5m10_to_flog(_Float16 * restrict z, int16_t * restrict q, int n, int nbits);   // 16 bit IEEE floats
void flog_to_e5m10(_Float16 * restrict z, int16_t * restrict q, int n, int nbits);

void e8m7_to_flog(__bf16 * restrict z, int16_t * restrict q, int n, int nbits);      // 16 bit brain floats
void flog_to_e8m7(__bf16 * restrict z, int16_t * restrict q, int n, int nbits);

void fp_to_qlog(float * restrict z, int32_t * restrict q, int n, int32_t nbits, float minabs, float zval);
void qlog_to_fp(float * restrict z, int32_t * restrict q, int n, int32_t nbits, float minabs, float zval);

// COMPILE_TEST_CODE is expected to be NOT DEFINED
#if defined(COMPILE_TEST_CODE)
float fp_to_from_qlog(float *f, int n, int nbits, float minabs, float zval);
float fp_to_from_flog(float *f, int n, int nbits);
#endif

#endif
