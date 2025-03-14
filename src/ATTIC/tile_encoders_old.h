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

// encoded tile layout (tentative) :
//
// ======================================= LAYOUT 3 (latest, more compact) =======================================
// (revised 2025/02/18)
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
// SSMEnnnn[ee][bbbb][offset]
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
// 000x  A type header (8 bits)   000bbbbb[constant_value]
// 0010  X type header (8+ bits)  0010xxxx  (reserved)
// 0011  L type header (8+5 bits) 0011SSME nnnnn [ee] [bbbbb] [offset]  (SS == 00 is reserved)
// 01xx  B type header (8 bits)   01MEnnnn [ee] [bbbb] [offset]
// 10xx  C type header (8 bits)   10MEnnnn [ee] [bbbb] [offset]
// 11xx  D type header (8 bits)   11MEnnnn [ee] [bbbb] [offset]
//
// A-D full header length : 8 + [E == 1 ? 2 : 0] + [M == 1 ? 5 + (bbbbb+1) : 0] bits
// only B, C, D headers may have E == 1 or M == 1
// only C, D, E headers may have E == 1 or M == 1
// X 0010xxxx       reserved for future use
//
// 8+4 bits header (nbits > 16)
//
// L full header length : 12 + [E == 1 ? 2 : 0] + [M == 1 ? 5 + (bbbbb+1) : 0] bits
//   0011SSME nnnn  17->32 bits/value, nnnn == number of bits - 16  (2 mandatory pieces)
//                  0011SSME nnnn[ee][bbbbb][offset] (2 - 5 pieces)
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

void print_encode_stats(int reset);

#if 0

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

int32_t encode_a_tile(void *field, int ni, int lni, int nj, bitstream *stream, uint32_t tile[64]);
int32_t encode_contiguous(uint64_t tp64, bitstream *stream, uint32_t tile[64]);
int32_t encode_as_tiles(void *field, int ni, int lni, int nj, bitstream *stream);

// int32_t decode_tile(void *field, int *ni, int lni, int *nj, bitstream *stream);
int32_t decode_a_tile(void *field, int ni, int lni, int nj, int *nptsij, bitstream *stream);
int32_t decode_as_tiles(void *field, int ni, int lni, int nj, bitstream *stream);

int32_t AecEncodeUnsigned(void *source, int32_t source_length, void *dest, int32_t dest_length, int bits_per_sample);
int32_t AecDecodeUnsigned(void *source, int32_t source_length, void *dest, int32_t dest_length, int bits_per_sample);

#endif   // if 0


#endif
