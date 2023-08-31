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
  uint32_t bits ;    // number of bits per element in pixmap
  uint32_t elem ;    // number of bits used in pixmap
  uint32_t nrle ;    // number of bits used if rle encoded, 0 if not
  uint32_t pop1 ;    // number of bits set to 1
  uint32_t zero ;    // number of 32 bit words with the value 0
  uint32_t all1 ;    // number of 32 bit words with all bits set
  int32_t  data[] ;
} rmn_pixmap ;       // uncompressed or RLE encoded pixmap

// 12/3 full encoding, appropriate for long (>48) sequences
// 0 means use 8/3 encoding, appropriate for shorter (>4, <49) sequences
#define RLE_123_0   8
#define RLE_123_1   4
// use full encoding for 0s or 1s, 12/3 or 8/3, medium or long (>4 sequences)
// 0 means simple encoding, appropriate for very short(<5) sequences
#define RLE_FULL_0  2
#define RLE_FULL_1  1
// by default, use full 8/3 encoding for 0s and simple encoding for 1s
#define RLE_DEFAULT RLE_FULL_0
// full (complex) encoding for both 0s and 1s
#define RLE_FULL_01 (RLE_FULL_0 | RLE_FULL_1)

#define OP_EQ           0
#define OP_SIGNED_GT    1
#define OP_SIGNED_LT   -1
#define OP_UNSIGNED_GT  2
#define OP_UNSIGNED_LT -2

rmn_pixmap *pixmap_create(int nelem, int nbits);
rmn_pixmap *pixmap_dup(rmn_pixmap *bmp_dst, rmn_pixmap *bmp_src);
// pixmap from data
rmn_pixmap *pixmap_be_eq_01(void *array, rmn_pixmap *bmp, uint32_t special, uint32_t mmask, int nelem);
rmn_pixmap *pixmap_be_int_01(void *array, rmn_pixmap *bmp, int32_t special, int32_t mmask, int n, int oper);
rmn_pixmap *pixmap_be_fp_01(float *array, rmn_pixmap *bmp, float special, int32_t mmask, int n, int oper);
// data from pixmap (potentially RLE encoded)
int pixmap_restore_be_01(void *array, rmn_pixmap *bmp, uint32_t plug, int nelem);

// RLE encoder hints
int pixmap_encode_hint_01(rmn_pixmap *bmp);
// RLE encoder for pixmap
rmn_pixmap *pixmap_encode_be_01(rmn_pixmap *bmp, rmn_pixmap *rle_stream, int rle_mode);
// decode RLE encoded pixmap
rmn_pixmap *pixmap_decode_be_01(rmn_pixmap *bmp, rmn_pixmap *rle_stream);

#endif

