// Hopefully useful code for C
// Copyright (C) 2022  Recherche en Prevision Numerique
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

#include <rmn/misc_operators.h>
#include <rmn/bi_endian_pack.h>

// header for an encoding tile
typedef struct{
  uint16_t nbts: 5,
           sign: 2,      // 00 all == 0, 01 all >= 0, 10 all < 0, 11 ZigZag
           encd: 2,      // encoding ( 00: none, 01: 0,short/1,full, 10: reserved, 11: constant block
           npti: 3,      // first dimension (npti = ni - 1)
           nptj: 3,      // second dimension (nptj = nj - 1)
           resv: 1;      // reserved
}tile_head ;             // header with bit fields

typedef struct{          // 8 x 8 encoding tile header (16 bits)
  union{
    tile_head h ;
    uint16_t s ;         // allows to grab it as one piece
  } ;
} tile_header ;

int32_t encode_ints(void *f, int ni, int nj, bitstream *stream);
int32_t encode_64_ints(void *f, bitstream *stream);
int32_t decode_ints(void *f, int *ni, int *nj, bitstream *stream);
int32_t decode_64_ints(void *f, bitstream *stream, uint16_t h16);
