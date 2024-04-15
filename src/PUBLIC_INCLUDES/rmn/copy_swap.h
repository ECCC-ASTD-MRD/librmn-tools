// Hopefully useful functions for C and FORTRAN
// Copyright (C) 2024  Recherche en Prevision Numerique
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

static uint32_t word32 = 1 ;
static uint8_t *le = (uint8_t *) &word32 ;

void FetchShuffleStore(void *src, void *dst, uint8_t *indx, int nbytes);
int Copy_items_r2l(void *src, uint32_t srclen, void *dst, uint32_t dstlen, uint32_t ns);
int Copy_items_l2r(void *src, uint32_t srclen, void *dst, uint32_t dstlen, uint32_t ns);
