// Hopefully useful functions for C and FORTRAN
// Copyright (C) 2023  Recherche en Prevision Numerique
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

#include <stdint.h>

uint32_t *stream_compress_32_sse(uint32_t *s, uint32_t *d, uint32_t *masks, int npts);
uint32_t *stream_compress_32_simd_2(uint32_t *s, uint32_t *d, uint32_t *masks, int npts);
uint32_t *stream_compress_32_simd(uint32_t *s, uint32_t *d, uint32_t *masks, int npts);
uint32_t *stream_compress_32(uint32_t *s, uint32_t *d, uint32_t *masks, int npts);
void stream_expand_32(uint32_t *s, uint32_t *d, uint32_t *masks, int npts, uint32_t *fill);
void stream_expand_32_sse(uint32_t *s, uint32_t *d, uint32_t *masks, int npts, uint32_t *pfill);
// void stream_expand_32_0(uint32_t *s, uint32_t *d, uint32_t *masks, int npts);
// void stream_replace_32(uint32_t *s, uint32_t *d, uint32_t *masks, int npts);
