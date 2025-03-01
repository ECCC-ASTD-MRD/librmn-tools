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
// these macros fill the bit stream Big Endian style, from the Most significant bits 
//
#include <stdint.h>

// bit stream macros and functions
#include <rmn/bitstream.h>

// ================================ bit insertion/extraction macros into/from bitstream ===============================
// macro arguments description
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
// accumulator SHOULD be zeroed before starting to insert
// ===============================================================================================
// for stream insertion, accum will be (s).acc_i, stream pointer will be (s).in

// undefine everything in case of multiple inclusion

#undef PACK_ENDIAN
#define PACK_ENDIAN 0xBE

// initialize big endian (BE) stylestream for insertion, accumulator and inserted bits count are set to 0
#undef INSERT_BEGIN
#define INSERT_BEGIN(accum, insert) { accum = 0 ; insert = 0 ; }
#undef STREAM_INSERT_BEGIN
#define STREAM_INSERT_BEGIN(s) INSERT_BEGIN((s).acc_i, (s).insert)

// insert the lower nbits bits from w32 into accumulator, update insert, accum
// (unsafe as it assumes that nbits bits can be inserted into acumulator)
#undef INSERT_NBITS
#define INSERT_NBITS(accum, insert, w32, nbits) \
        { uint64_t t=(w32) ; t <<= (64-(nbits)) ; t >>= insert ; insert += (nbits) ; accum |= t ; }
#undef STREAM_INSERT_NBITS
#define STREAM_INSERT_NBITS(s, w32, nbits) INSERT_NBITS((s).acc_i, (s).insert, w32, nbits)

// check that 32 bits can be safely inserted into accumulator
// if not possible, store upper 32 bits of accum into stream, update accum, insert, stream pointer
#undef INSERT_CHECK
#define INSERT_CHECK(accum, insert, streamptr) \
        { *(streamptr) = (uint64_t) accum >> 32 ; if(insert > 32) { insert -= 32 ; (streamptr)++ ; accum <<= 32 ; } ; }
#undef STREAM_INSERT_CHECK
#define STREAM_INSERT_CHECK(s) INSERT_CHECK((s).acc_i, (s).insert, (s).in)

// push data to stream without fully updating control info (stream, insert)
#undef PUSH
#define PUSH(accum, insert, streamptr) \
        { INSERT_CHECK(accum, insert, streamptr) ; if(insert > 0) { *(streamptr) = (uint64_t) accum >> 32 ; } }
#undef STREAM_PUSH
#define STREAM_PUSH(s) PUSH((s).acc_i, (s).insert, (s).in)

// store any residual data from accum into stream, update accum, insert, stream
#undef INSERT_FINALIZE
#define INSERT_FINALIZE(accum, insert, streamptr) \
        { INSERT_CHECK(accum, insert, streamptr) ; if(insert > 0) { *(streamptr) = (uint64_t) accum >> 32 ; (streamptr)++ ; insert = 0 ; } }
#undef STREAM_INSERT_FINALIZE
#define STREAM_INSERT_FINALIZE(s) INSERT_FINALIZE((s).acc_i, (s).insert, (s).in)

// safely put lower nbits from w32 into accumulator, update accum, xtract, stream pointer
#undef PUT_NBITS
#define PUT_NBITS(accum, insert, w32, nbits, streamptr) \
        { INSERT_CHECK(accum, insert, streamptr) ; INSERT_NBITS(accum, insert, w32, nbits) ; }
#undef STREAM_PUT_NBITS
#define STREAM_PUT_NBITS(s, w32, nbits) PUT_NBITS((s).acc_i, (s).insert, w32, nbits, (s).in)

// alignment calls should be preceded with INSERT_CHECK/STREAM_INSERT_CHECK
// align insertion point to a 32 bit boundary (an appropriate number of 0 bits will be inserted into accumulator)
#undef INSERT_ALIGN32
#define INSERT_ALIGN32(accum, insert) if(insert != 0){ uint32_t tbits=64-insert ; accum>>=tbits ; accum<<=tbits ; tbits &= 31 ; insert += tbits ; }
#undef STREAM_INSERT_ALIGN32
#define STREAM_INSERT_ALIGN32(s) INSERT_ALIGN32((s).acc_i, (s).insert)

// align insertion point to a 16 bit boundary (an appropriate number of 0 bits will be inserted into accumulator)
#undef INSERT_ALIGN16
#define INSERT_ALIGN16(accum, insert) if(insert != 0){ uint32_t tbits=64-insert ; accum>>=tbits ; accum<<=tbits ; tbits &= 15 ; insert += tbits ; }
#undef STREAM_INSERT_ALIGN16
#define STREAM_INSERT_ALIGN16(s) INSERT_ALIGN16((s).acc_i, (s).insert)

// align insertion point to a 8 bit boundary (an appropriate number of 0 bits will be inserted into accumulator)
#undef INSERT_ALIGN8
#define INSERT_ALIGN8(accum, insert) if(insert != 0){ uint32_t tbits=64-insert ; accum>>=tbits ; accum<<=tbits ; tbits &= 7 ; insert += tbits ; }
#undef STREAM_INSERT_ALIGN8
#define STREAM_INSERT_ALIGN8(s) INSERT_ALIGN8((s).acc_i, (s).insert)

// rewind a bit stream to read it from the beginning (potentially force valid read mode)
#define  STREAM_REWIND(s, force_read) { \
  if(s.insert > 0) STREAM_PUSH(s) ; if(force_read) s.xtract = 0 ;     \
  if(s.xtract >= 0){ s.acc_x  = 0 ; s.out = s.first ; }               }

// ===============================================================================================
// for stream extraction, accum will be (s).acc_x, stream pointer will be (s).out

// initialize stream for extraction
// extraction from the top (most significant) part of accumulator then shift accumulator left (EXTRACT_FROM_TOP)
#undef XTRACT_BEGIN
#define XTRACT_BEGIN(accum, xtract, streamptr) { uint32_t t = *(streamptr) ; accum = t ; accum <<= 32 ; (streamptr)++ ; xtract = 32 ; }
#undef STREAM_XTRACT_BEGIN
#define STREAM_XTRACT_BEGIN(s) { XTRACT_BEGIN((s).acc_x, (s).xtract, (s).out) ; }

// take a peek at the next nbits bits from accumulator into w32 (unsafe, assumes that nbits bits are available)
#undef PEEK_NBITS
#define PEEK_NBITS(accum, xtract, w32, nbits) { w32 = (uint64_t)accum >> (64 - (nbits)) ; }
#undef STREAM_PEEK_NBITS
#define STREAM_PEEK_NBITS(s, w32, nbits)   PEEK_NBITS((s).acc_x, (s).xtract, w32, nbits)

// skip the next nbits bits from accumulator (unsafe, assumes that nbits bits are available)
#undef SKIP_NBITS
#define SKIP_NBITS(accum, xtract, nbits) { accum <<= (nbits) ; xtract -= (nbits) ; }
#undef STREAM_SKIP_NBITS
#define STREAM_SKIP_NBITS(s, nbits) SKIP_NBITS((s).acc_x, (s).xtract, nbits)

// extract nbits bits into w32 from accumulator, update xtract, accum (unsafe, assumes that nbits bits are available)
#undef XTRACT_NBITS
#define XTRACT_NBITS(accum, xtract, w32, nbits) \
        { PEEK_NBITS(accum, xtract, w32, nbits) ; SKIP_NBITS(accum, xtract, nbits) ; }
#undef STREAM_XTRACT_NBITS
#define STREAM_XTRACT_NBITS(s, w32, nbits) XTRACT_NBITS((s).acc_x, (s).xtract, w32, nbits)

// check that 32 bits can be safely extracted from accum
// if not possible, get extra 32 bits into accum from stresm, update accum, xtract, stream
#undef XTRACT_CHECK
#define XTRACT_CHECK(accum, xtract, streamptr) \
        { if(xtract < 32) { accum = (uint64_t) accum >> (32-xtract) ; accum |= *(streamptr) ; accum <<= (32-xtract) ; xtract += 32 ; (streamptr)++ ; } ; }
#undef STREAM_XTRACT_CHECK
#define STREAM_XTRACT_CHECK(s) XTRACT_CHECK((s).acc_x, (s).xtract, (s).out)

// finalize extraction, update accum, xtract
#undef XTRACT_FINAL
#define XTRACT_FINAL(accum, xtract) { accum = 0 ; xtract = 0 ; }
#undef STREAM_XTRACT_FINAL
#define STREAM_XTRACT_FINAL(s) XTRACT_FINAL((s).acc_x, (s).xtract)

// safely get nbits into w32, update accum, xtract, stream pointer
#undef GET_NBITS
#define GET_NBITS(accum, xtract, w32, nbits, streamptr) \
        { XTRACT_CHECK(accum, xtract, streamptr) ; XTRACT_NBITS(accum, xtract, w32, nbits) ; }
#undef STREAM_GET_NBITS
#define STREAM_GET_NBITS(s, w32, nbits) GET_NBITS((s).acc_x, (s).xtract, w32, nbits, (s).out)

// align extraction point to a 32 bit boundary
#undef XTRACT_ALIGN32
#define XTRACT_ALIGN32(accum, xtract) { uint32_t tbits = xtract ; tbits &= 31 ; accum <<= tbits ; xtract -= tbits ; }
#undef STREAM_XTRACT_ALIGN32
#define STREAM_XTRACT_ALIGN32(s) XTRACT_ALIGN32((s).acc_x, (s).xtract)

// align extraction point to a 16 bit boundary
#undef XTRACT_ALIGN16
#define XTRACT_ALIGN16(accum, xtract) { uint32_t tbits = xtract ; tbits &= 15 ; accum <<= tbits ; xtract -= tbits ; }
#undef STREAM_XTRACT_ALIGN16
#define STREAM_XTRACT_ALIGN16(s) XTRACT_ALIGN16((s).acc_x, (s).xtract)

// align extraction point to a 8 bit boundary
#undef XTRACT_ALIGN8
#define XTRACT_ALIGN8(accum, xtract) { uint32_t tbits = xtract ; tbits &= 7 ; accum <<= tbits ; xtract -= tbits ; }
#undef STREAM_XTRACT_ALIGN8
#define STREAM_XTRACT_ALIGN8(s) XTRACT_ALIGN8((s).acc_x, (s).xtract)
