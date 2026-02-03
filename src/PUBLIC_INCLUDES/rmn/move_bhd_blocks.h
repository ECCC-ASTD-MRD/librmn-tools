// Hopefully useful code for C (memory block movers)
// Copyright (C) 2026  Recherche en Prevision Numerique
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
//     M. Valin,   Recherche en Prevision Numerique, 2026
//

#include <stdint.h>

void move_u8_to_u32(uint32_t *w, uint8_t *bhd, int lni, int ni, int nj);  // unsigned 8 -> 32
void move_u32_to_u8(uint8_t *bhd, uint32_t *w, int lni, int ni, int nj);  // unsigned 32 -> 8
void move_i8_to_i32(int32_t *w, int8_t *bhd, int lni, int ni, int nj);  // signed 8 -> 32
void move_i32_to_i8(int8_t *bhd, int32_t *w, int lni, int ni, int nj);  // signed 32 -> 8
void move_u16_to_u32(uint32_t *w, uint16_t *bhd, int lni, int ni, int nj);  // unsigned 16 -> 32
void move_u32_to_u16(uint16_t *bhd, uint32_t *w, int lni, int ni, int nj);  // unsigned 32 -> 16
void move_i16_to_i32(int32_t *w, int16_t *bhd, int lni, int ni, int nj);  // signed 16 -> 32
void move_i32_to_i16(int16_t *bhd, int32_t *w, int lni, int ni, int nj);  // signed 32 -> 16
void move_u64_to_u32(uint32_t *w, uint64_t *bhd, int lni, int ni, int nj);  // unsigned 64 -> 32
void move_u32_to_u64(uint64_t *bhd, uint32_t *w, int lni, int ni, int nj);  // unsigned 32 -> 64
void move_i64_to_i32(int32_t *w, int64_t *bhd, int lni, int ni, int nj);  // signed 64 -> 32
void move_i32_to_i64(int64_t *bhd, int32_t *w, int lni, int ni, int nj);  // signed 32 -> 64
void move_d64_to_f32(float *fp, double *dp, int lni, int ni, int nj);
void move_f32_to_d64(double *dp, float *fp, int lni, int ni, int nj);
