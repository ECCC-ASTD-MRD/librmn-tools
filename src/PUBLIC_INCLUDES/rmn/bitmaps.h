// Hopefully useful functions for C and FORTRAN
// Copyright (C) 2021  Recherche en Prevision Numerique
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
#if ! defined(RMN_BITMAPS)
#define RMN_BITMAPS

#include <stdint.h>
#include <rmn/misc_pack.h>

typedef struct{
  uint32_t size ;    // size of bm in uint32_t units
  uint32_t elem ;    // number of bits used in bitmap
  uint32_t rle  ;    // number of bits if rle encoded, 0 if not
  int32_t bm[] ;
} rmn_bitmap ;       // uncompressed bitmap

typedef struct{
  uint32_t size ;    // size of rle in uint32_t units
  uint32_t elem ;    // number of bits used in rle
  uint32_t rle[] ;
} rmn_rle_bitmap ;   // pseudo run length encoded bitmap

rmn_bitmap *bitmask_be_01(void *array, rmn_bitmap *bmp, uint32_t special, uint32_t mmask, int n);
int bitmask_restore_be_01(void *array, rmn_bitmap *bmp, uint32_t special, int n);
rmn_rle_bitmap *bitmap_encode_be_01(rmn_bitmap *bmp, rmn_rle_bitmap *rle_stream);

#endif

