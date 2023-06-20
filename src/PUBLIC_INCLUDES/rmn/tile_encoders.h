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

#if ! defined (TILE_ENCODERS_H)
#define TILE_ENCODERS_H

#include <rmn/misc_operators.h>
#include <rmn/bi_endian_pack.h>

// header for an encoded tile (16 bits)
typedef struct{
  uint16_t nbts: 5,
           sign: 2,      // 00 all == 0, 01 all >= 0, 10 all < 0, 11 ZigZag
           encd: 2,      // encoding ( 00: none, 01: 0,short/1,full, 10: reserved, 11: constant block
           npti: 3,      // first dimension (npti = ni - 1) (1 <= ni <= 8)
           nptj: 3,      // second dimension (nptj = nj - 1) (1 <= nj <= 8)
           min0: 1;      // 1 : minimum value is used as offset, 0 : minimum not used
}tile_head ;             // header with bit fields

typedef struct{          // 1-8 x 1-8 encoded tile header (16 bits)
  union{
    tile_head h ;
    uint16_t s ;         // allows to grab all as one piece
  } ;
} tile_header ;

// tile properties for encoding (includes tile header)
typedef struct{
      uint32_t min ;                // u32[0]
      uint16_t nzero:8, nshort:8 ;  // u16[2]
      tile_head h ;                 // u16[3]
} tile_parms ;

typedef union{
    tile_parms t ;
    uint64_t u64 ;         // allows to grab all as one piece
    uint32_t u32[2] ;      // u32[0] : min
    uint16_t u16[4] ;      // u16[2] : grab nzero and nshort, u16[3] : grab tile head
} tile_properties ;

uint64_t encode_tile_properties_(void *f, int ni, int lni, int nj, uint32_t tile[64]);
uint64_t encode_tile_properties(void *f, int ni, int lni, int nj, uint32_t tile[64]);
void print_tile_properties(uint64_t p64);

int32_t encode_tile_(uint32_t tile[64], bitstream *s, uint64_t tp64);
int32_t encode_tile(void *f, int ni, int lni, int nj, bitstream *s, uint32_t tile[64]);
int32_t encode_tile_8x8(void *f, int lni, bitstream *stream, uint32_t tile[64]);

int32_t decode_tile(void *f, int *ni, int lni, int *nj, bitstream *stream);
int32_t decode_tile_8x8(void *f, int lni, bitstream *stream, uint16_t h16);

#endif
