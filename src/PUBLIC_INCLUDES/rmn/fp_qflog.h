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

int32_t fp_to_qflog_1(float f, int nbits, float minabs, float zval);
void    fp_to_qflog_n(float *z, int32_t *q, int n, int32_t nbits, float minabs, float zval);
void    fp_to_qflog(float *z, int32_t *q, int n, int32_t nbits, float minabs, float zval);

float qflog_to_fp_1(int32_t i, int nbits, float minabs);
void  qflog_to_fp_n(float *z, int32_t *q, int n, int32_t nbits, float minabs);
void  qflog_to_fp(float *z, int32_t *q, int n, int32_t nbits, float minabs);

float fp_to_from_qflog(float *f, int n, int nbits, float minabs, float zval);
