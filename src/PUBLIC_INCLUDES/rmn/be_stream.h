//
// Copyright (C) 2025  Environnement Canada
//
// This is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation,
// version 2.1 of the License.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details .
//
// Author:
//     M. Valin,   Recherche en Prevision Numerique, 2025
//
// set of macros and functions to manage insertion/extraction into/from a bit stream
// these macros work with the bit stream Big Endian style, from the Most significant bits 
//
#include <stdint.h>

// undefine everything in case of multiple inclusion

#undef PACK_ENDIAN
// #define PACK_ENDIAN 0xBE
#define PACK_ENDIAN 'B'

// bit stream macros and functions
#include <rmn/bitstream.h>

// ================================ bit insertion/extraction macros into/from bitstream ===============================
// arguments description for base macros
// accum  [INOUT] : 64 bit accumulator (normally acc_i or acc_x)
// insert [INOUT] : # of bits already inserted in accumulator (0 <= insert <= 64)
// xtract [INOUT] : # of bits available for extraction from accumulator (0 <= xtract <= 64)
// w32    [IN]    : 32 bit integer containing data to be inserted (expression allowed)
//        [OUT]   : 32 bit integer receiving extracted data (MUST be a variable)
// nbits  [IN]    : number of bits to insert / extract in w32 (<= 32 bits)
//
// N.B. : if w32 is a "signed" variable, extraction will produce a "signed" result
// the STREAM macros use implicit accum/xtract/xtract/stream arguments
//
// ===============================================================================================
// big endian (BE) style (left to right) bit stream packing
// insert token below bits already inserted into accumulator
// extract token from the top (most significant part) of accumulator then shift accumulator left
// accumulator MUST be zeroed before starting to insert
// ===============================================================================================

// insert the lower nbits bits from w32 into accumulator
// this macro is unsafe, it assumes that nbits bits can be inserted into acumulator
#undef INSERT_NBITS
#define INSERT_NBITS(accum, insert, w32, nbits) \
        { uint64_t t=(w32) ; t <<= (64-(nbits)) ; t >>= insert ; insert += (nbits) ; accum |= t ; }

// insert 1 into accumulator
// this macro is unsafe, it assumes that nbits bits can be inserted into acumulator
#undef INSERT_1
#define INSERT_1(accum, insert) { uint64_t t=1 ; t <<= (63-insert) ; insert ++ ; accum |= t ; }

// insert 0 into accumulator
// this macro is unsafe, it assumes that nbits bits can be inserted into acumulator
#undef INSERT_0
#define INSERT_0(accum, insert) { insert ++ ; }

// insert a stream of 0s into accumulator (equivalent of SKIP_NBITS in insert mode)
// no action is needed for accum, argument is kept for compatoibility with LE version
// this macro is unsafe, it assumes that nbits bits can be inserted into acumulator
#undef INSERT_PAD
#define INSERT_PAD(accum, insert, nbits) { insert+=nbits ; }

// check that 32 bits can be safely inserted into accumulator
// if not possible, store upper 32 bits of accum into stream, update accum, insert, stream pointer
#undef INSERT_CHECK
#define INSERT_CHECK(accum, insert, streamptr) \
        { *(streamptr) = (uint64_t) accum >> 32 ; if(insert > 32) { insert -= 32 ; (streamptr)++ ; accum <<= 32 ; } ; }

// push data to stream without fully updating control info (stream, insert)
#undef INSERT_PUSH
#define INSERT_PUSH(accum, insert, streamptr) \
        { INSERT_CHECK(accum, insert, streamptr) ; if(insert > 0) { *(streamptr) = (uint64_t) accum >> 32 ; } }

// store any residual data from accum into stream, update accum, insert, stream
#undef INSERT_FINALIZE
#define INSERT_FINALIZE(accum, insert, streamptr) \
        { INSERT_CHECK(accum, insert, streamptr) ; if(insert > 0) { *(streamptr) = (uint64_t) accum >> 32 ; (streamptr)++ ; insert = 0; accum = 0 ; } }

// alignment calls should be preceded or followed with INSERT_CHECK/STREAM_INSERT_CHECK
// align insertion point to a 32 bit boundary (an appropriate number of 0 bits will be inserted into accumulator)
#undef INSERT_ALIGN32
#define INSERT_ALIGN32(accum, insert) if(insert != 0){ uint32_t tbits=64-insert ; accum>>=tbits ; accum<<=tbits ; tbits &= 31 ; insert += tbits ; }

// align insertion point to a 16 bit boundary (an appropriate number of 0 bits will be inserted into accumulator)
#undef INSERT_ALIGN16
#define INSERT_ALIGN16(accum, insert) if(insert != 0){ uint32_t tbits=64-insert ; accum>>=tbits ; accum<<=tbits ; tbits &= 15 ; insert += tbits ; }

// align insertion point to a 8 bit boundary (an appropriate number of 0 bits will be inserted into accumulator)
#undef INSERT_ALIGN8
#define INSERT_ALIGN8(accum, insert) if(insert != 0){ uint32_t tbits=64-insert ; accum>>=tbits ; accum<<=tbits ; tbits &= 7 ; insert += tbits ; }

// ===============================================================================================
// if w32 is a "signed" variable, extraction will produce a "signed" result

// initialize stream for extraction
// extraction from the top (most significant) part of accumulator then shift accumulator left (EXTRACT_FROM_TOP)
#undef XTRACT_BEGIN
#define XTRACT_BEGIN(accum, xtract, streamptr) { uint32_t t = *(streamptr) ; accum = t ; accum <<= 32 ; (streamptr)++ ; xtract = 32 ; }

// take a peek at the next nbits bits from accumulator into w32 (unsafe, assumes that nbits bits are available)
#undef PEEK_NBITS
#define PEEK_NBITS(accum, xtract, w32, nbits) { w32 = (uint64_t)accum >> (64 - (nbits)) ; }

// take a peek at the next bit from accum into w32 (unsafe, assumes that 1 bit is available)
#undef PEEK_1
#define PEEK_1(accum, xtract, w32) { w32 = (accum >> 63) & 1 ; }

// skip the next nbits bits from accumulator (unsafe, assumes that nbits bits are available)
#undef SKIP_NBITS
#define SKIP_NBITS(accum, xtract, nbits) { accum <<= (nbits) ; xtract -= (nbits) ; }

// skip the next bit from accumulator (unsafe, assumes that 1 bit is available)
#undef SKIP_1
#define SKIP_1(accum, xtract) { accum <<= 1 ; xtract-- ; }

// check that 32 bits can be safely extracted from accum
// if not possible, get extra 32 bits into accum from stresm, update accum, xtract, stream
#undef XTRACT_CHECK
#define XTRACT_CHECK(accum, xtract, streamptr) \
        { if(xtract < 32) { accum = (uint64_t) accum >> (32-xtract) ; accum |= *(streamptr) ; accum <<= (32-xtract) ; xtract += 32 ; (streamptr)++ ; } ; }

// finalize extraction, update accum, xtract
#undef XTRACT_FINAL
#define XTRACT_FINAL(accum, xtract) { accum = 0 ; xtract = 0 ; }

// align extraction point to a 32 bit boundary
#undef XTRACT_ALIGN32
#define XTRACT_ALIGN32(accum, xtract) { uint32_t tbits = xtract ; tbits &= 31 ; accum <<= tbits ; xtract -= tbits ; }

// align extraction point to a 16 bit boundary
#undef XTRACT_ALIGN16
#define XTRACT_ALIGN16(accum, xtract) { uint32_t tbits = xtract ; tbits &= 15 ; accum <<= tbits ; xtract -= tbits ; }

// align extraction point to a 8 bit boundary
#undef XTRACT_ALIGN8
#define XTRACT_ALIGN8(accum, xtract) { uint32_t tbits = xtract ; tbits &= 7 ; accum <<= tbits ; xtract -= tbits ; }

// ===============================================================================================
// put  token2 (ltoken2 bits) to the right of shifted token1 (ltoken1 bits)  ltoken1 will be updated (left to right)
#undef CONCAT_TOKENS
#define CONCAT_TOKENS(token1, ltoken1, token2, ltoken2) { token1 = (token1 << ltoken2) | token2 ; ltoken1 += ltoken2 ; }

// ===============================================================================================
// common macros
#include <rmn/common_stream.h>
