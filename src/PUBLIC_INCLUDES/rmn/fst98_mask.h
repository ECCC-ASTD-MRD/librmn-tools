// Hopefully useful functions for C and FORTRAN
// Copyright (C) 2026  Environnement Canada
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
#if ! defined(RLE_MICRO_0)

// 101n   n = rep count - 1
#define RLE_MICRO_0 0xA
// 110n
#define RLE_MICRO_1 0xD
#define RLE_MICRO_OFFSET 0

// 1001nnnn   nnnnnnnn = rep count - 3
#define RLE_SMALL_0 0x90
// 1110nnnn
#define RLE_SMALL_1 0xE0
#define RLE_SMALL_OFFSET 3

// 1000nnnnnnnn   nnnnnnnn = rep count - 19
#define RLE_LARGE_0 0x800
// 1111nnnnnnnn
#define RLE_LARGE_1 0xF00
#define RLE_LARGE_OFFSET 19

#define RLE_MAX_COUNT (RLE_LARGE_OFFSET + 255)

#include <stdint.h>
// use Big Endian stream encoding
#include <rmn/be_stream.h>

typedef struct{
  uint32_t code ;    // encoded repeat count and 0x00 / 0xFF byte marker
  uint16_t left ;    // leftover repeat count
  uint8_t  bits ;    // number of bits needed for code
  uint8_t  byte ;    // repeated byte (0x00 / 0xFF)
}fst_rle_code ;

fst_rle_code fst_rle_token(uint8_t byte, uint32_t count);
int32_t fst_rle_encode(uint8_t byte, uint32_t count, bitstream *stream);

#endif
