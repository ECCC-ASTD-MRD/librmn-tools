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

#if ! defined (TILE_ENCODERS_INCLUDED)
#define TILE_ENCODERS_INCLUDED

// #include <rmn/ct_assert.h>
// #include <rmn/misc_operators.h>
// packing macros
// #include <rmn/bi_endian_pack.h>
// bit stream macros and functions
// #include <rmn/bit_stream.h>
#include <rmn/move_blocks.h>
#include <rmn/bitstream.h>

// ======================================= encoded tile layout =======================================
// (last revised 2025/03/22)
//
//
// <- always -> <-----------------   as needed        -------------------->
// +-----------+-----------+-----------------+          +-----------------+
// |   header  |  options  |     value 0     | ........ |     value n     |
// +-----------+-----------+-----------------+          +-----------------+
// <-8/12 bits->
// options : 5 bit bbbbb field, 2 bit ee field, 1-32 bit offset field
// the number of values in the encoded block must come from an EXTERNAL source
//
// 8 bits header part 1 (nbits <= 16, except for constant blocks, SS == 00)
// SSMEnnnn [ee][bbbbb][o.....o]  (o.....o uses bbbbb+1 bits)
// 000bbbbb c.....c               (c.....c uses bbbbb+1 bits)
//
// A 000bbbbb      constant block, ZIGZAG(value), 1 -> 32 bits/value, bbbbb == number of bits - 1
// X 0010xxxx      reserved for future use
// L 0011SSME      preamble of 12 bit long header      (> 16 bits / value) 0011SSME nnnn
//   001100xx      NOT USED, constant blocks shall use the A type header (can be reserved for future use)
// B 01MEnnnn      all values >= 0  ( 1->16 bits / value, nnnn == number of bits - 1)
// C 10MEnnnn      all values <= 0  ( 1->16 bits / value, ABS(value), nnnn == number of bits - 1)
// D 11MEnnnn      mixed signs      (1->16 bits / value, nnnn == number of bits - 1)
//
// the first 2/3/4 bits indicate the header type
// 000x  A type header (8 bits)   000bbbbb c.....c
// 0010  X type header (8+ bits)  0010xxxx  (reserved)
// 0011  L type header (8+4 bits) 0011SSME nnnn [ee] [bbbbb] [o.....o]  (SS == 00 is reserved)
// 01xx  B type header (8 bits)   01MEnnnn [ee] [bbbbb] [o.....o]
// 10xx  C type header (8 bits)   10MEnnnn [ee] [bbbbb] [o.....o]
// 11xx  D type header (8 bits)   11MEnnnn [ee] [bbbbb] [o.....o]
//
// A-D full header length : 8 + [E == 1 ? 2 : 0] + [M == 1 ? 5 + (bbbbb+1) : 0] bits
// only B, C, D headers may have E == 1 or M == 1
// only C, D, E headers may have E == 1 or M == 1
// X 0010xxxx       reserved for future use
//
// 8+4 bits header (nbits > 16)
//
// L full header length : 12 + [E == 1 ? 2 : 0] + [M == 1 ? 5 + (bbbbb+1) : 0] bits
//   0011SSME nnnn  17->32 bits/value, nnnn == number of bits - 17  (2 mandatory pieces)
//                  0011SSME nnnn [ee] [bbbbb] [o.....o] (2 - 5 pieces)
//
// SS : 00 constant block
//      01 all values >= 0
//      10 all values <= 0 (ABS(value) is stored)
//      11 mixed signs, zigzag (sign in LSB) type encoding
// M  : 0 no offset
//      1 a ZIGZAG encoded value will be present, a 5 bits bbbbb nb of bits for offset field width follows
//      bbbbb == number of bits - 1 used for offset
// E  : 0 no special block encoding of data values
//      1 special short/long block encoding is used, a 2 bit ee field is present
//        short value : 0 vv...vv  (nshort+1 bits)
//        zero value  : 0          (special case if nshort == 0)
//        long value  : 1 vv...vv  (nbits+1 bits)
//      e.g. 0011SS1Ennnnbbbbb[offset]     offset is used (offset length is bbbbb bits)
//           0011SS11nnnnbbbbbee[offset]   offset, short/long encoding (offset length is bbbbb bits)
//           0011SS01nnnnee                no offset, short/long encoding
//
// ee : 00   short value = 0, encoded as 0 (nbits/8 maybe ?)
//      01   short value : nbits/2,          encoded as 0 followed by nbits/2 bits
//      10   short value : nbits/2 + 1 bits, encoded as 0 followed by nbits/2+1 bits
//      11   short value : nbits/2 + 2 bits, encoded as 0 followed by nbits/2+2 bits
//           long values, encoded as 1 followed by nbits bits
//
// N.B.  some SSME combinations are not valid (e.g. 0010 and 0011)
//       M and E make no sense if SS == 0 (constant blocks)

int encode_tile(bitstream *s, int32_t *tile, int32_t nval, block_properties *bp);
int decode_tile(bitstream *s, int32_t *tile, int32_t nval);

// ======================================= encoded block layout =======================================
//         a BLOCK is subdivided for encoding into TILES
//         (usual basic tile size = 8 x 8)
//
//         the last tile along a dimension may be shorter or longer than the basic size
//         basic_size/2  <= last_tile_size < (basic_size + basic_size/2)
//        <-- basic_size -->                                  < last_tile_size >
//        +----------------+----------------------------------+----------------+    ^            ^
//        |                |                                  |                |    |            |
//        |  tile(0,ntj)   |                                  |  tile(nti,ntj) | last_tile_size  |
//        |                |                                  |                |    |            |
//        +----------------+----------------------------------+----------------+    v            |
//        |                |                                  |                |                 |
//        |                |                                  |                |                 |
//        |                |                                  |                |            block size
//        |                |                                  |                |                (nj)
//        |                |                                  |                |                 |
//        |                |                                  |                |                 |
//        |                |                                  |                |                 |
//        +----------------+----------------------------------+----------------+    ^            |
//        |                |                                  |                |    |            |
//        |  tile(0,0)     |                                  |  tile(nti,0)   | basic_size      |
//        |                |                                  |                |    |            |
//        +----------------+----------------------------------+----------------+    v            v
//        <------- 8 ------>                                  < last_tile_size >
//        <----------------------- block size (ni) ---------------------------->
//
//  nti = (ni + basic_size/2) / basic_size
//  ntj = (ni + basic_size/2) / basic_size
//  basic_size MUST BE EVEN
// the last tile size along i and j may be different

int encode_block(bitstream *s_in, int32_t *block, int lnis, int ni, int nj, int basic_size);
int decode_block(bitstream *s_in, int32_t *block, int lnid, int ni, int nj, int basic_size);

void print_encode_stats(int reset);

#endif
