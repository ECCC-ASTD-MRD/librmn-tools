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
// bit stream macros and functions
#include <rmn/bit_stream.h>

// encoded tile layout (tentative) :
//
// ============================================= LAYOUT 2 (new) =============================================
// (revised 2024/08/21)
//
// <- always -> <-----------        as needed            -------------->
// +-----------+-----------------+          +-----------------+--------+
// |   header  |     token 1     | ........ |     token n     |  fill  |
// +-----------+-----------------+          +-----------------+--------+
// <--- NH --->                                                <- 0-7 -> (bits)
// <---------------------------- TSIZE  -------------------------------> (bytes)
//
// header :
//
// <----------------- always --------> <----------------------- as needed  --------------------------->
// +-----+-----+---------------+------+--------+---------+---------------+----------+-----------------+
// |  S0 |  N0 |     ndata     | MODE | ENCODE |  NBI0   |      NBY0     |   NBMI   |     OFFSET      |
// +-----+-----+---------------+------+--------+---------+---------------+----------+-----------------+
//  < 3 > < 2 > <- 6/8/16/32 -> <  2 > <-- 3 -> <-- 5 --> <- 6/8/16/32 -> <------- NBMI + 5 ----------> (bits)
// <--------------------------------------------- NH -------------------------------------------------> (bits)
//
// S0     : tile size indicator ( 3 bits )
//     000 -> 011  : 2/3/4/5 bytes (2 + S0)
//     100         : TSIZE = NBY0 + 6 bytes (NBY0 uses 6 bits)
//     101         : TSIZE = NBY0 + 6 bytes (NBY0 uses 8 bits)
//     110         : TSIZE = NBY0 + 6 bytes (NBY0 uses 16 bits)
//     111         : TSIZE = NBY0 + 6 bytes (NBY0 uses 32 bits)
// N0     : number of bits used for ndata ( 2 bits )
//     00 -  6 bits for ndata
//     01 -  8 bits for ndata
//     10 - 16 bits for ndata
//     11 - 32 bits for ndata
// ndata  : number of data points - 1 ( 6/8/16/32 bits )
// MODE   : data properties ( 2 bits )
//     00 - unsigned data (NO NEGATIVE VALUES)
//     01 - signed data (2s complement or zigzag)
//     10 - zero tile (coding and nbits 0 absent)
//     11 - 2s complement offset is used (data  >= 0 after offset removal)
// ENCODE : encoding mode ( 3 bits )
//    000 - NBI0 + 1 bits per value (unsigned or 2s complement signed)
//    001 - short/long encoding, NSHORT == (NBI0+1)/2 - 1 bits
//    010 - short/long encoding, NSHORT == (NBI0+1)/2     bits
//    011 - short/long encoding, NSHORT == (NBI0+1)/2 + 1 bits
//    100 - short/long encoding, NSHORT == 0             bits
//    101 - constant valued tile (implies MODE == 11, OFFSET is value)
//    110 - no short/long encoding, all values < 0, -value stored in NBI0 + 1 bits
//    111 - alternative encoding (reserved for future encoding schemes)
// NBI0   : base number of bits per encoded value = NBI0 + 1 ( 5 bits )
//          (not used for constant valued tiles)
// NBY0   : number of bytes - 6 used by tile ( 6/8/16/32 bits )
// NBMI   : number of bits - 1 used to store 2s complement offset
// OFFSET : minimum data value (2s complement) ( NBMI + 1 bits )
// TSIZE  : number of bytes used by tile
// NH     : number of bits used by tile header
//
// short / long encoding :
//    a "short" value is a non negative value that can be represented using NSHORT bits or less
//    any other value is considered as a "long" value
//    short values : 0 followed by NSHORT bits ( token length = NSHORT + 1 bits)
//    long values  : 1 followed by NBI0 bits ( token length = NBI0 + 1 bits)
//    if values are signed, zigzag format is used for short/long data value encoding
//
// zero tile and constant tile only use the header
//
// zero tile     : 3 + 2 + 2 + 6/8/16/32 bits, MODE = 10
//                 (13/15/23/39 useful bits) -> (2/2/3/5 bytes used)
// constant tile : 3 + 2 + 2 + 6/8/16/32 bits, MODE = 11, ENCODE = 101, NBMI and OFFSET used
//                 (16/18/26/42 + 5 + NBMI+1 useful bits) (roun up to next multiple of 8 bits used)
//
// ============================================= LAYOUT 1 (old) =============================================
// (before 2024/08/06)
//
//             <------ optional fields -------------->
// +-----------+--------+----------+-----------------+-----------------+          +-----------------+
// |   header  | bshort |   nbmi   |     minimum     |     token 1     | ........ |     token n     |
// +-----------+--------+----------+-----------------+-----------------+          +-----------------+
// <- 16 bits ->        <------ nbmi + 5 bits ------->
//             <-5 bits->
//
// 16 bit header description :
//          nbts         number of bits per token - 1 (0-31)
//          npij         number of values -1 in tile (0-63)
//          min0 = 0     no minimum value removed
//          min0 = 1     minimum value (offset) removed from tile values
//          encd = 00    each value  : nbts+1 bits
//          encd = 01    short value : first bit=0 , bshort bits
//                       other value : first bit=1 , nbts+1 bits
//          encd = 10    zero value  : first bit=0
//                       other value : first bit=1 , nbts+1 bits
//          encd = 11    all values are identical (possibly 0) (nbts+1 bits) (0 bits if sign == 00)
//
//          sign = 00    every value is 0
//          sign = 01    every value is NON negative (short ZigZag without sign)
//          sign = 10    every value is negative (short ZigZag without sign)
//          sign = 11    mixed value signs (full ZigZag)
//
// the bshort part is only used when encd = 1 to indicate the number of nits for "short" values
// the nbmi/minimum part is only used when min0 = 0 in header (minimum value subtracted)
//
// zero tile : sign == 00, encd = 11, nbts = 0 (don't really care), min0 = 0 (don't really care)
// +-----------+
// |   header  |
// +-----------+
// <- 16 bits ->
//
// constant tile : sign = 11, encd = 11, min0 = 0 (don't really care)
// +-----------+-----------------+
// |   header  |     token 1     |
// +-----------+-----------------+
// <- 16 bits ->
//             <- nbts + 1 bits ->
//
// full ZigZag description          (used when all values have mixed signs)
//        value >= 0 :   value << 1
//        value  < 0 : ~(value << 1)
// short ZigZag description         (used when all values have the same sign)
//        value >= 0 :   value
//        value  < 0 :  ~value
// values are normally stored in full ZigZag form, sign is LSB
// if all values are negative or all values are non negative,
// the sign bit is omitted and value or ~value is stored
//
// header for an encoded tile (16 bits)
typedef struct{          // 2 D tile
  uint16_t nbts: 5,      // number of bits per token - 1
           sign: 2,      // 00 all == 0, 01 all >= 0, 10 all < 0, 11 ZigZag
           encd: 2,      // encoding ( 00: none, 01: 0//short , 1//full, 10: 0 , 1//full, 11: constant tile
           npij: 6,      // dimension (npij = n - 1) (0 <= npij <= 64)
//            npti: 3,      // first dimension (npti = ni - 1) (0 <= ni <= 8)
//            nptj: 3,      // second dimension (nptj = nj - 1) (0 <= nj <= 8)
           min0: 1;      // 1 : minimum value is used as offset, 0 : minimum not used
}tile_head ;             // header with bit fields
CT_ASSERT_(2 == sizeof(tile_head))

typedef struct{          // 1-8 x 1-8 encoded tile header (16 bits)
  union{
    tile_head h ;
    uint16_t s ;         // allows to grab everything as one piece
  } ;
} tile_header ;
CT_ASSERT_(2 == sizeof(tile_header))

// tile properties for encoding (includes tile header)
typedef struct{
      uint32_t min ;                // u32[0]
      uint16_t bshort:8, nshort:8 ; // u16[2]
      tile_head h ;                 // u16[3]
} tile_parms ;
CT_ASSERT_(8 == sizeof(tile_parms))

typedef union{
    tile_parms t ;
    uint64_t u64 ;         // allows to grab everything as one piece
    uint32_t u32[2] ;      // u32[0] : min
    uint16_t u16[4] ;      // u16[2] : grab bshort and nshort, u16[3] : grab tile header
} tile_properties ;
CT_ASSERT_(8 == sizeof(tile_properties))

// uint64_t encode_tile_scheme(uint64_t p64);
void tile_population(void *tile, int n, int32_t pop[4], void *ref);
uint64_t encode_tile_properties(void *field, int ni, int lni, int nj, uint32_t tile[64]);
void print_tile_properties(uint64_t p64);

int32_t encode_tile(void *field, int ni, int lni, int nj, bitstream *stream, uint32_t tile[64]);
int32_t encode_contiguous(uint64_t tp64, bitstream *stream, uint32_t tile[64]);
int32_t encode_as_tiles(void *field, int ni, int lni, int nj, bitstream *stream);

// int32_t decode_tile(void *field, int *ni, int lni, int *nj, bitstream *stream);
int32_t decode_tile(void *field, int ni, int lni, int nj, int *nptsij, bitstream *stream);
int32_t decode_as_tiles(void *field, int ni, int lni, int nj, bitstream *stream);

int32_t AecEncodeUnsigned(void *source, int32_t source_length, void *dest, int32_t dest_length, int bits_per_sample);
int32_t AecDecodeUnsigned(void *source, int32_t source_length, void *dest, int32_t dest_length, int bits_per_sample);

#endif
