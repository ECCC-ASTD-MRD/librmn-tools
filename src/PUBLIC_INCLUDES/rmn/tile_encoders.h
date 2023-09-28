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

#include <rmn/ct_assert.h>
#include <rmn/misc_operators.h>
#include <rmn/bi_endian_pack.h>

// encoded tile layout :
//                   min0 == 0
// +-----------+-----------------+          +-----------------+
// |   header  |     token 1     | ........ |     token n     |
// +-----------+-----------------+          +-----------------+
// <- 16 bits ->
//
//                   min0 == 1
// +-----------+----------+-----------------+-----------------+          +-----------------+
// |   header  |   nbmi   |     minimum     |     token 1     | ........ |     token n     |
// +-----------+----------+-----------------+-----------------+          +-----------------+
// <- 16 bits -x- 5 bits -x- nbmi + 1 bits ->
//
// token encoding : 
//          encd = 00    each value  : nbts+1 bits
//          encd = 01    small value : 0bit , (nbts+1)/2 bits
//                       other value : 1bit , nbts+1 bits
//          encd = 10    zero value  : 0bit
//                       other value : 1bit , nbts+1 bits
//          encd = 11    all values are identical (possibly 0) (nbts+1 or 0 bits)
//
//          sign = 00    every value is 0
//          sign = 01    every value is NON negative (short ZigZag without sign)
//          sign = 10    every value is negative (short ZigZag without sign)
//          sign = 11    mixed value signs (full ZigZag)
//
// zero tile (sign == 00, encd = 11, nbts = 0, min0 = 0)
// +-----------+
// |   header  |
// +-----------+
// <- 16 bits ->
//
// constant tile (sign = 11, encd = 11, min0 = 0)
// +-----------+-----------------+
// |   header  |     token 1     |
// +-----------+-----------------+
// <- 16 bits -x- nbts + 1 bits ->
//
// ZigZag description
//        value >= 0 :    value << 1
//        value  < 0 : ~(value << 1)
// values are normally stored in full ZigZag form, sign is LSB
// if all values are negative or all values are non negative, 
// the sign bit is omitted and value or ~value is stored
//
// header for an encoded tile (16 bits)
// for a 1D tile, npti and nptj are to be interpreted as ni = 1 + npti + 8 * nptj, nj = 1
// for a 2D tile, ni = npti + 1, nj = nptj + 1
// if both npti == 7 and nptj == 7, it does not matter, we have a full 64 value block
// TODO: npti, nptj or npij ?
typedef struct{
  uint16_t nbts: 5,      // number of bits per token - 1
           sign: 2,      // 00 all == 0, 01 all >= 0, 10 all < 0, 11 ZigZag
           encd: 2,      // encoding ( 00: none, 01: 0//short , 1//full, 10: 0 , 1//full, 11: constant tile
           npti: 3,      // first dimension (npti = ni - 1) (0 <= ni <= 8)
           nptj: 3,      // second dimension (nptj = nj - 1) (0 <= nj <= 8)
           min0: 1;      // 1 : minimum value is used as offset, 0 : minimum not used
}tile_head ;             // header with bit fields
CT_ASSERT(2 == sizeof(tile_head))

typedef struct{          // 1-8 x 1-8 encoded tile header (16 bits)
  union{
    tile_head h ;
    uint16_t s ;         // allows to grab everything as one piece
  } ;
} tile_header ;
CT_ASSERT(2 == sizeof(tile_header))

// tile properties for encoding (includes tile header)
typedef struct{
      uint32_t min ;                // u32[0]
      uint16_t bshort:8, nshort:8 ; // u16[2]
      tile_head h ;                 // u16[3]
} tile_parms ;
CT_ASSERT(8 == sizeof(tile_parms))

typedef union{
    tile_parms t ;
    uint64_t u64 ;         // allows to grab everything as one piece
    uint32_t u32[2] ;      // u32[0] : min
    uint16_t u16[4] ;      // u16[2] : grab bshort and nshort, u16[3] : grab tile header
} tile_properties ;
CT_ASSERT(8 == sizeof(tile_properties))

// uint64_t encode_tile_scheme(uint64_t p64);
void tile_population(int32_t *tile, int n, int32_t pop[4], int32_t ref[4]);
uint64_t encode_tile_properties(void *field, int ni, int lni, int nj, uint32_t tile[64]);
void print_tile_properties(uint64_t p64);

int32_t encode_tile(void *field, int ni, int lni, int nj, bitstream *stream, uint32_t tile[64]);
int32_t encode_contiguous(uint64_t tp64, bitstream *stream, uint32_t tile[64]);
int32_t encode_as_tiles(void *field, int ni, int lni, int nj, bitstream *stream);

int32_t decode_tile(void *field, int *ni, int lni, int *nj, bitstream *stream);
int32_t decode_as_tiles(void *field, int ni, int lni, int nj, bitstream *stream);

#endif
