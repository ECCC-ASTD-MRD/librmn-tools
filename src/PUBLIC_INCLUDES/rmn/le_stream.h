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
// these macros fill the bit stream Little Endian style, from the Least significant bits 
//
#include <stdint.h>

// bit stream macros and functions
#include <rmn/bitstream.h>

// undefine everything in case of multiple inclusion

#undef PACK_ENDIAN
#define PACK_ENDIAN 0xEB

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
// little endian (LE) style (right to left) bit stream packing
// insert token into top (most significant part) of accumulator after accumulator has been shifted right
// extract token from the bottom (least significant part) of accumulator then shift accumulator right
// accumulator SHOULD be zeroed before starting to insert
// ===============================================================================================

// insert the lower nbits bits from w32 into accumulator
// these macros are unsafe, they assume that nbits bits can be inserted into acumulator
#undef INSERT_NBITS
#define INSERT_NBITS(accum, insert, w32, nbits) \
        { uint64_t t=(uint32_t)(w32) ; t<<=(64-(nbits)) ; accum=(uint64_t)accum>>(nbits) ; accum|=t ; insert+=(nbits) ; }

// check that 32 bits can be safely inserted into accum
// if not possible, store lower 32 useful bits of accum into stream, update accum, insert, stream pointer
#undef INSERT_CHECK
#define INSERT_CHECK(accum, insert, streamptr) \
        { if(insert > 32) { *(streamptr)=(((uint64_t)accum)>>(64-insert)) ; (streamptr)++ ; insert -= 32 ; } ; }

// push data into stream without fully updating control info (stream pointer, insert)
#undef INSERT_PUSH
#define INSERT_PUSH(accum, insert, streamptr) \
        { INSERT_CHECK(accum, insert, streamptr) ; { if(insert > 0) { *(streamptr)=(((uint64_t)accum)>>(64-insert)) ; } ; } }

// store any residual data from accum into stream, update insert, stream pointer
#undef INSERT_FINALIZE
#define INSERT_FINALIZE(accum, insert, streamptr) \
        { INSERT_CHECK(accum, insert, streamptr) ; { if(insert > 0) { *(streamptr) = ((uint64_t)accum>>(64-insert)) ; (streamptr)++ ; insert = 0; accum = 0 ; } ; } }

// alignment calls should be preceded or followed with INSERT_CHECK/STREAM_INSERT_CHECK
// align insertion point to a 32 bit boundary (accum MUST BE UPDATED) (an appropriate number of 0 bits will be inserted)
#undef INSERT_ALIGN32
#define INSERT_ALIGN32(accum, insert) if(insert != 0){ int tbits = 64 - insert ;  tbits &= 31 ; insert += tbits ; accum >>= tbits ; }

// align insertion point to a 16 bit boundary (accum MUST BE UPDATED) (an appropriate number of 0 bits will be inserted)
#undef INSERT_ALIGN16
#define INSERT_ALIGN16(accum, insert) if(insert != 0){ int tbits = 64 - insert ;  tbits &= 15 ; insert += tbits ; accum >>= tbits ; }

// align insertion point to a 8 bit boundary (accum MUST BE UPDATED) (an appropriate number of 0 bits will be inserted)
#undef INSERT_ALIGN8
#define INSERT_ALIGN8(accum, insert) if(insert != 0){ int tbits = 64 - insert ;  tbits &= 7 ; insert += tbits ; accum >>= tbits ; }

// ===============================================================================================
// if w32 is a "signed" variable, extraction will produce a "signed" result
//
// initialize stream for extraction, load first 32 bits from stream into accum, set available bits count to 32
#undef XTRACT_BEGIN
#define XTRACT_BEGIN(accum, xtract, streamptr) { accum = (uint32_t) *(streamptr) ; (streamptr)++ ; xtract = 32 ; }

// take a peek at the next nbits bits from accum into w32 (unsafe, assumes that nbits bits are available)
#undef PEEK_NBITS
#define PEEK_NBITS(accum, xtract, w32, nbits) { w32 = accum ; w32 = ( w32 << (32-(nbits)) ) >> (32-(nbits)) ; }

// skip the next nbits bits from accum (unsafe, assumes that nbits bits are available)
#undef SKIP_NBITS
#define SKIP_NBITS(accum, xtract, nbits) { accum = (uint64_t) accum >> (nbits) ; xtract -= (nbits) ; }

// check that 32 bits can be safely extracted from accum (accum contains at least 32 available bits)
// if not possible, get extra 32 bits into accum from stream, update accum, xtract, stream pointer
#undef XTRACT_CHECK
#define XTRACT_CHECK(accum, xtract, streamptr) \
        { if(xtract < 32) { uint64_t t = (uint32_t)(*(streamptr)) ; accum |= (t << xtract) ; (streamptr)++ ; xtract += 32 ; } ; }

// finalize extraction, update accum, xtract
#undef XTRACT_FINAL
#define XTRACT_FINAL(accum, xtract) { accum = 0 ; xtract = 0 ; }

// align extraction point to a 32 bit boundary
#undef XTRACT_ALIGN32
#define XTRACT_ALIGN32(accum, xtract) { uint32_t tbits = xtract ; tbits &= 31 ; accum = (uint64_t) accum >> tbits ; xtract -= tbits ; }

// align extraction point to a 16 bit boundary
#undef XTRACT_ALIGN16
#define XTRACT_ALIGN16(accum, xtract) { uint32_t tbits = xtract ; tbits &= 15 ; accum = (uint64_t) accum >> tbits ; xtract -= tbits ; }

// align extraction point to a 8 bit boundary
#undef XTRACT_ALIGN8
#define XTRACT_ALIGN8(accum, xtract) { uint32_t tbits = xtract ; tbits &= 7 ; accum = (uint64_t) accum >> tbits ; xtract -= tbits ; }

// ===============================================================================================
// common macros
#include <rmn/le_be_stream.h>
