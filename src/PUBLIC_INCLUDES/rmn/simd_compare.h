//
// Copyright (C) 2024  Environnement Canada
//
// This is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation,
// version 2.1 of the License.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details .
//
// Author:
//     M. Valin,   Recherche en Prevision Numerique, 2024
//
#include <stdint.h>

void v_less_than_c(int32_t *z, int32_t ref[6], int32_t count[6], int n) ;
void v_less_than_c_6(int32_t *z, int32_t ref[6], int32_t count[6], int n) ;
void v_less_than_c_4(int32_t *z, int32_t ref[4], int32_t count[4], int n) ;

void v_less_than_simd(int32_t *z, int32_t ref[6], int32_t count[6], int n) ;
void v_less_than_simd_6(int32_t *z, int32_t ref[6], int32_t count[6], int n) ;
void v_less_than_simd_4(int32_t *z, int32_t ref[4], int32_t count[4], int n) ;

void v_less_than(int32_t *z, int32_t ref[6], int32_t count[6], int n) ;
void v_less_than_6(int32_t *z, int32_t ref[6], int32_t count[6], int n) ;
void v_less_than_4(int32_t *z, int32_t ref[4], int32_t count[4], int n) ;

void v_minmax(int32_t *z, int32_t n, int32_t *mins, int32_t *maxs, uint32_t *min0) ;
void v_minmax_simd(int32_t *z, int32_t n, int32_t *mins, int32_t *maxs, uint32_t *min0) ;
void v_minmax_c(int32_t *z, int32_t n, int32_t *mins, int32_t *maxs, uint32_t *min0) ;
