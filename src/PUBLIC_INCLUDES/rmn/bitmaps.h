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
#if ! defined(RMN_BITMAPS)
#define RMN_BITMAPS

#include <stdint.h>

// create bit maps / restore from bit maps
//   comparing floats /ints / unsigned ints to a special value (greater than, less than, masked equality)
// RLE encode / decode these bit maps

typedef struct{
  uint32_t size ;    // size of bm in uint32_t units
  uint32_t elem ;    // number of bits used in bitmap
  uint32_t nrle ;    // number of bits if rle encoded, 0 if not
  uint32_t ones ;    // number of bits set to 1
  int32_t  data[] ;
} rmn_bitmap ;       // uncompressed bitmap

#define RLE_FULL_0  2
#define RLE_FULL_1  1
// by default, use full encoding for 0s and simple encoding for 1s
#define RLE_DEFAULT RLE_FULL_0
// full (complex) encoding for both 0s and 1s
#define RLE_FULL_01 (RLE_FULL_0 | RLE_FULL_1)

#define OP_EQ           0
#define OP_SIGNED_GT    1
#define OP_SIGNED_LT   -1
#define OP_UNSIGNED_GT  2
#define OP_UNSIGNED_LT -2

rmn_bitmap *bitmap_create(int nelem);
rmn_bitmap *bitmap_dup(rmn_bitmap *bmp_dst, rmn_bitmap *bmp_src);
// bitmap from data
rmn_bitmap *bitmap_be_eq_01(void *array, rmn_bitmap *bmp, uint32_t special, uint32_t mmask, int nelem);
rmn_bitmap *bitmap_be_int_01(void *array, rmn_bitmap *bmp, int32_t special, int32_t mmask, int n, int oper);
rmn_bitmap *bitmap_be_fp_01(float *array, rmn_bitmap *bmp, float special, int32_t mmask, int n, int oper);
// data from bitmap (potentially RLE encoded)
int bitmap_restore_be_01(void *array, rmn_bitmap *bmp, uint32_t plug, int nelem);

// RLE encoder for bitmap
rmn_bitmap *bitmap_encode_be_01(rmn_bitmap *bmp, rmn_bitmap *rle_stream, int rle_mode);
// decode RLE encoded bitmap
rmn_bitmap *bitmap_decode_be_01(rmn_bitmap *bmp, rmn_bitmap *rle_stream);

#endif

