/* 
 * Copyright (C) 2025  Recherche en Prevision Numerique
 *                     Environnement Canada
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation,
 * version 2.1 of the License.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 */
#define USE_SIMD_INTRINSICS
// #define EMULATE_SIMD
// #define ALIAS_SIMD_INTRINSICS

#include <rmn/simd_functions.h>

void count_gt(int count[4], int *values, int ref4[4], int n);
void count_le(int count[4], int *values, int ref4[4], int n);

void count_lt(int count[4], int *values, int ref4[4], int n);
void count_ge(int count[4], int *values, int ref4[4], int n);

void count_eq(int count[4], int *values, int ref4[4], int n);
void count_ne(int count[4], int *values, int ref4[4], int n);
