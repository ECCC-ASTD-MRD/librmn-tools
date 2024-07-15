//
// Copyright (C) 2022-2024  Environnement Canada
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
//     M. Valin,   Recherche en Prevision Numerique, 2022-2024
//
// set of macros and functions to manage insertion/extraction into/from a bit stream
//
// N.B. the accumulator MUST be a 64 bit variable
// initialize stream for insertion
//   [L|B]E64_INSERT_BEGIN(accumulator, counter)
// insert the lower nbits bits from uint32 into accumulator, update counter, accumulator
//   [L|B]E64_INSERT_NBITS(accumulator, counter, uint32, nbits)
//   [L|B]E64_FAST_INSERT_NBITS(accumulator, counter, uint32, nbits)
// check that 32 bits can be safely inserted into accumulator
// if not possible, store lower 32 bits of accumulator into stream, update accumulator, counter, stream
//   [L|B]E64_INSERT_CHECK(accumulator, counter, stream)
// push data to stream without fully updating (accumulator, stream, counter)
//   [L|B]E64_PUSH(accumulator, counter, stream)
// store any residual data from accumulator into stream, update accumulator, counter, stream
//   [L|B]E64_INSERT_FINAL(accumulator, counter, stream)
// combined INSERT_CHECK and INSERT_NBITS, update accumulator, counter, stream
//   [L|B]E64_PUT_NBITS(accumulator, counter, uint32, nbits, stream)
//   [L|B]E64_FAST_PUT_NBITS(accumulator, counter, uint32, nbits, stream)
//
// N.B. : if uint32 and accumulator are signed variables, the extract will produce a "signed" result
//        if uint32 and accumulator are unsigned variables, the extract will produce an "unsigned" result
// initialize stream for extraction
//   [L|B]E64_XTRACT_BEGIN(accumulator, xtract, stream)
// take a peek at nbits bits from accumulator into uint32
//   [L|B]E64_PEEK_NBITS(accumulator, xtract, uint32, nbits)
// extract nbits bits into uint32 from accumulator, update xtract, accumulator
//   [L|B]E64_XTRACT_NBITS(accumulator, xtract, uint32, nbits)
// check that 32 bits can be safely extracted from accumulator
// if not possible, get extra 32 bits into accumulator from stream, update accumulator, xtract, stream
//   [L|B]E64_XTRACT_CHECK(accumulator, xtract, stream)
// finalize extraction, update accumulator, xtract
//   [L|B]E64_XTRACT_FINAL(accumulator, xtract)
// combined XTRACT_CHECK and XTRACT_NBITS, update accumulator, xtract, stream
//   [L|B]E64_GET_NBITS(accumulator, xtract, uint32, nbits, stream)
//
//
#include <stdint.h>

#undef LE64_INSERT_BEGIN
#undef LE64_INSERT_NBITS
#undef LE64_FAST_INSERT_NBITS
#undef LE64_INSERT_CHECK
#undef LE64_PUSH
#undef LE64_INSERT_FINAL
#undef LE64_PUT_NBITS
#undef LE64_FAST_PUT_NBITS
#undef LE64_INSERT_ALIGN

#undef LE64_XTRACT_BEGIN
#undef LE64_PEEK_NBITS
#undef LE64_XTRACT_NBITS
#undef LE64_XTRACT_CHECK
#undef LE64_XTRACT_FINAL
#undef LE64_GET_NBITS
#undef LE64_XTRACT_ALIGN

#undef BE64_INSERT_BEGIN
#undef BE64_INSERT_NBITS
#undef BE64_FAST_INSERT_NBITS
#undef BE64_INSERT_CHECK
#undef BE64_PUSH
#undef BE64_INSERT_FINAL
#undef BE64_PUT_NBITS
#undef BE64_FAST_PUT_NBITS
#undef BE64_INSERT_ALIGN

#undef BE64_XTRACT_BEGIN
#undef BE64_PEEK_NBITS
#undef BE64_XTRACT_NBITS
#undef BE64_XTRACT_CHECK
#undef BE64_XTRACT_FINAL
#undef BE64_GET_NBITS
#undef BE64_XTRACT_ALIGN

#if defined(FILL_FROM_TOP)
#undef FILL_FROM_BOTTOM
#else
#define FILL_FROM_BOTTOM
#endif

// ================================ bit insertion/extraction macros into/from bitstream ===============================
// macro arguments description
// accum  [INOUT] : 64 bit accumulator (normally acc_i or acc_x)
// insert [INOUT] : # of bits used in accumulator (0 <= insert <= 64)
// xtract [INOUT] : # of bits extractable from accumulator (0 <= xtract <= 64)
// w32    [IN]    : 32 bit integer containing data to be inserted (expression allowed)
//        [OUT]   : 32 bit integer receiving extracted data (MUST be a variable)
// nbits  [IN]    : number of bits to insert / extract in w32 (<= 32 bits)
//
// N.B. : if w32 and accum are "signed" variables, extraction will produce a "signed" result
//        if w32 and accum are "unsigned" variables, extraction will produce an "unsigned" result
// the EZ and STREAM macros use implicit accum/xtract/xtract/stream arguments
//
// ===============================================================================================
// little endian (LE) style (right to left) bit stream packing
// insertion into accumulator of token shifted left
// extraction from the bottom (least significant part) of accumulator then shift accumulator right
// accumulator MUST be zeroed before starting to insert (CRITICAL)
// ===============================================================================================
// initialize little endian (LE) style stream for insertion, accumulator and inserted bits count are set to 0
#define LE64_INSERT_BEGIN(accum, insert) { accum = 0 ; insert = 0 ; }
#if defined (FILL_FROM_BOTTOM)
// insert the lower nbits bits from w32 into accum, update insert, accum (safer/slower version using a mask)
// (unsafe as it assumes that nbits bits can be inserted into acumulator)
#define LE64_INSERT_NBITS(accum, insert, w32, nbits) \
        { uint32_t mask = ~0 ; mask >>= (32-(nbits)) ; uint64_t w64 = (w32) & mask ; accum |= (w64 << insert) ; insert += (nbits) ; }
// fast insert the lower nbits bits from w32 into accum, update insert, accum (unsafe/faster version without a mask)
// (unsafe as it assumes that nbits bits can be inserted into acumulator)
#define LE64_FAST_INSERT_NBITS(accum, insert, w32, nbits) \
        { uint64_t w64 = (w32) ; accum |= (w64 << insert) ; insert += (nbits) ; }
// check that 32 bits can be safely inserted into accum
// if not possible, store lower 32 bits of accum into stream, update accum, insert, stream
// accum MUST be treated as "unsigned"
#define LE64_INSERT_CHECK(accum, insert, stream) \
        { if(insert > 32) { *stream = accum ; stream++ ; insert -= 32 ; accum = (uint64_t) accum >> 32 ; } ; }
// push data to stream without fully updating control info (stream, insert)
#define LE64_PUSH(accum, insert, stream) \
        { LE64_INSERT_CHECK(accum, insert, stream) ; { if(insert > 0) { *stream = accum ; } ; } }
// store any residual data from accum into stream, update accum, insert, stream
#define LE64_INSERT_FINAL(accum, insert, stream) \
        { LE64_INSERT_CHECK(accum, insert, stream) ; { if(insert > 0) { *stream = accum ; stream++ ; insert = 0 ; } ; } }
// combined INSERT_CHECK and INSERT_NBITS, update accum, insert, stream (safe version)
#define LE64_PUT_NBITS(accum, insert, w32, nbits, stream) \
        { LE64_INSERT_CHECK(accum, insert, stream) ; LE64_INSERT_NBITS(accum, insert, w32, nbits) ; }
// fast combined INSERT_CHECK and INSERT_NBITS, update accum, insert, stream (unsafe version)
#define LE64_FAST_PUT_NBITS(accum, insert, w32, nbits, stream) \
        { LE64_INSERT_CHECK(accum, insert, stream) ; LE64_FAST_INSERT_NBITS(accum, insert, w32, nbits) ; }
// align insertion point to a 32 bit boundary
#define LE64_INSERT_ALIGN(insert) { uint32_t tbits = 64 - insert ;  tbits &= 31 ; insert += tbits ; }
// #define LE64_EZ_INSERT_ALIGN        { uint32_t tbits = 64 - StReAm_insert ; tbits &= 31 ; StReAm_insert += tbits ; }
// #define LE64_STREAM_INSERT_ALIGN(s) { uint32_t tbits = 64 - StReAm_insert ; tbits &= 31 ; (s).insert += tbits ; }
#endif  // FILL_FROM_BOTTOM

#if defined(FILL_FROM_TOP)
// insert the lower nbits bits from w32 into accum, update insert, accum
// (unsafe as it assumes that nbits bits can be inserted into acumulator)
#define LE64_INSERT_NBITS(accum, insert, w32, nbits) \
        { uint64_t t=(w32) ; t<<=(64-(nbits)) ; (uint64_t)accum>>=(nbits) ; accum|=t ; insert+=(nbits) ; }
#define LE64_FAST_INSERT_NBITS(accum, insert, w32, nbits)  LE64_INSERT_NBITS(accum, insert, w32, nbits)
// check that 32 bits can be safely inserted into accum
// if not possible, store lower 32 bits of accum into stream, update accum, insert, stream
// accum MUST be treated as "unsigned"
#define LE64_INSERT_CHECK(accum, insert, stream) \
        { *stream=((uint64_t)accum>>(64-insert)) ; if(insert > 32) { stream++ ; insert -= 32 ; } ; }
// push data to stream without fully updating control info (stream, insert)
#define LE64_PUSH(accum, insert, stream) \
        { LE64_INSERT_CHECK(accum, insert, stream) ; { if(insert > 0) { *stream = accum ; } ; } }
// store any residual data from accum into stream, update accum, insert, stream
#define LE64_INSERT_FINAL(accum, insert, stream) \
        { LE64_INSERT_CHECK(accum, insert, stream) ; { if(insert > 0) { *stream = ((uint64_t)accum>>(64-insert)) ; stream++ ; insert = 0 ; } ; } }
// combined INSERT_CHECK and INSERT_NBITS, update accum, insert, stream (safe version)
#define LE64_PUT_NBITS(accum, insert, w32, nbits, stream) \
        { LE64_INSERT_NBITS(accum, insert, w32, nbits) ; LE64_INSERT_CHECK(accum, insert, stream) ; }
#define LE64_FAST_PUT_NBITS(accum, insert, w32, nbits, stream) LE64_PUT_NBITS(accum, insert, w32, nbits, stream)
// align insertion point to a 32 bit boundary
#define LE64_INSERT_ALIGN(insert) { uint32_t tbits = 64 - insert ;  tbits &= 31 ; insert += tbits ; }
// #define LE64_EZ_INSERT_ALIGN        { uint32_t tbits = 64 - StReAm_insert ; tbits &= 31 ; StReAm_insert += tbits ; }
// #define LE64_STREAM_INSERT_ALIGN(s) { uint32_t tbits = 64 - StReAm_insert ; tbits &= 31 ; (s).insert += tbits ; }
#endif  // FILL_FROM_TOP
//
// N.B. : if w32 and accum are signed variables, the extract will produce a "signed" result
//        if w32 and accum are unsigned variables, the extract will produce an "unsigned" result
// initialize stream for extraction, load first 32 bits from stream into accum, set available bits count to 32
#define LE64_XTRACT_BEGIN(accum, xtract, stream) { accum = (uint32_t) *(stream) ; (stream)++ ; xtract = 32 ; }
// take a peek at nbits bits from accum into w32 (unsafe, assumes that nbits bits are available)
#define LE64_PEEK_NBITS(accum, xtract, w32, nbits) { w32 = (accum << (64-nbits)) >> (64-nbits) ; }
// extract nbits bits into w32 from accum, update xtract, accum (unsafe, assumes that nbits bits are available)
#define LE64_XTRACT_NBITS(accum, xtract, w32, nbits) \
        { w32 = (accum << (64-nbits)) >> (64-nbits) ; accum = (uint64_t) accum >> nbits ; xtract -= (nbits) ;}
// check that 32 bits can be safely extracted from accum (accum contains at least 32 available bits)
// if not possible, get extra 32 bits into accum from stream, update accum, xtract, stream
#define LE64_XTRACT_CHECK(accum, xtract, stream) \
        { if(xtract < 32) { uint64_t w64 = *(stream) ; accum |= (w64 << xtract) ; (stream)++ ; xtract += 32 ; } ; }
// finalize extraction, update accum, xtract
#define LE64_XTRACT_FINAL(accum, xtract) \
        { accum = 0 ; xtract = 0 ; }
// combined XTRACT_CHECK and XTRACT_NBITS, update accum, xtract, stream
#define LE64_GET_NBITS(accum, xtract, w32, nbits, stream) \
        { LE64_XTRACT_CHECK(accum, xtract, stream) ; LE64_XTRACT_NBITS(accum, xtract, w32, nbits) ; }
// align extraction point to a 32 bit boundary
#define LE64_XTRACT_ALIGN(accum, xtract) { uint32_t tbits = xtract ; tbits &= 31 ; accum = (uint64_t) accum >> tbits ; xtract -= tbits ; }
// #define LE64_EZ_XTRACT_ALIGN { uint32_t tbits = StReAm_xtract ; tbits &= 31 ; StReAm_acc_x >>= tbits ; StReAm_xtract -= tbits ; }
// #define LE64_STREAM_XTRACT_ALIGN { uint32_t tbits = (s).xtract ; tbits &= 31 ; (s).acc_x >>= tbits ; (s).xtract -= tbits ; }
//
// ===============================================================================================
// big endian (BE) style (left to right) bit stream packing
// insertion at bottom (least significant part) of accumulator after accumulator shifted left
// extraction from the top (most significant part) of accumulator then shift accumulator left
// it is not critical to set the accumulator to zero before starting to insert
// ===============================================================================================
// initialize big endian (BE) stylestream for insertion, accumulator and inserted bits count are set to 0
#define BE64_INSERT_BEGIN(accum, insert) { accum = 0 ; insert = 0 ; }
#if defined (FILL_FROM_BOTTOM)
// insert the lower nbits bits from w32 into accum, update insert, accum (safer/slower version using a mask)
// (unsafe as it assumes that nbits bits can be inserted into acumulator)
#define BE64_INSERT_NBITS(accum, insert, w32, nbits) \
        {  uint32_t mask = ~0 ; mask >>= (32-(nbits)) ; accum <<= (nbits) ; insert += (nbits) ; accum |= ((w32) & mask) ; }
// fast insert the lower nbits bits from w32 into accum, update insert, accum (unsafe/faster version without a mask)
// (unsafe as it assumes that nbits bits can be inserted into acumulator)
#define BE64_FAST_INSERT_NBITS(accum, insert, w32, nbits) \
        { accum <<= (nbits) ; insert += (nbits) ; accum |= (w32) ; }
// check that 32 bits can be safely inserted into accum
// if not possible, store lower 32 bits of accum into stream, update accum, insert, stream
#define BE64_INSERT_CHECK(accum, insert, stream) \
        { if(insert > 32) { insert -= 32 ; *(stream) = accum >> insert ; (stream)++ ; } ; }
// push data to stream without fully updating control info (stream, insert)
#define BE64_PUSH(accum, insert, stream) \
        { BE64_INSERT_CHECK(accum, insert, stream) ; if(insert > 0) { *stream = accum << (32 - insert) ; } }
// store any residual data from accum into stream, update accum, insert, stream
#define BE64_INSERT_FINAL(accum, insert, stream) \
        { BE64_INSERT_CHECK(accum, insert, stream) ; if(insert > 0) { *stream = accum << (32 - insert) ; stream++ ; insert = 0 ; } }
// combined INSERT_CHECK and INSERT_NBITS, update accum, insert, stream
#define BE64_PUT_NBITS(accum, insert, w32, nbits, stream) \
        { BE64_INSERT_CHECK(accum, insert, stream) ; BE64_INSERT_NBITS(accum, insert, w32, nbits) ; }
// fast combined INSERT_CHECK and INSERT_NBITS, update accum, insert, stream (unsafe version)
#define BE64_FAST_PUT_NBITS(accum, insert, w32, nbits, stream) \
        { BE64_INSERT_CHECK(accum, insert, stream) ; BE64_FAST_INSERT_NBITS(accum, insert, w32, nbits) ; }
// align insertion point to a 32 bit boundary
#define BE64_INSERT_ALIGN(accum, insert) { uint32_t tbits = 64 - insert ; tbits &= 31 ; insert += tbits ; accum <<= tbits ; }
#endif  // FILL_FROM_BOTTOM

#if defined(FILL_FROM_TOP)
// insert the lower nbits bits from w32 into accum, update insert, accum
// (unsafe as it assumes that nbits bits can be inserted into acumulator)
#define BE64_INSERT_NBITS(accum, insert, w32, nbits) \
        { uint64_t t=(w32) ; t <<= (64-(nbits)) ; t >>= insert ; insert += (nbits) ; accum |= t ; }
#define BE64_FAST_INSERT_NBITS(accum, insert, w32, nbits) BE64_INSERT_NBITS(accum, insert, w32, nbits)
// check that 32 bits can be safely inserted into accum
// if not possible, store upper 32 bits of accum into stream, update accum, insert, stream
#define BE64_INSERT_CHECK(accum, insert, stream) \
        { *(stream) = (uint64_t) accum >> 32 ; if(insert > 32) { insert -= 32 ; (stream)++ ; accum <<= 32 ; } ; }
// push data to stream without fully updating control info (stream, insert)
#define BE64_PUSH(accum, insert, stream) \
        { BE64_INSERT_CHECK(accum, insert, stream) ; if(insert > 0) { *stream = (uint64_t) accum >> 32 ; } }
// store any residual data from accum into stream, update accum, insert, stream
#define BE64_INSERT_FINAL(accum, insert, stream) \
        { BE64_INSERT_CHECK(accum, insert, stream) ; if(insert > 0) { *stream = (uint64_t) accum >> 32 ; stream++ ; insert = 0 ; } }
// combined INSERT_CHECK and INSERT_NBITS, update accum, insert, stream
#define BE64_PUT_NBITS(accum, insert, w32, nbits, stream) \
        { BE64_INSERT_NBITS(accum, insert, w32, nbits) ; BE64_INSERT_CHECK(accum, insert, stream) ; }
#define BE64_FAST_PUT_NBITS(accum, insert, w32, nbits, stream) BE64_PUT_NBITS(accum, insert, w32, nbits, stream)
// align insertion point to a 32 bit boundary
#define BE64_INSERT_ALIGN(accum, insert) { uint32_t tbits = 64 - insert ; tbits &= 31 ; insert += tbits ; }
#endif  // FILL_FROM_TOP
//
// N.B. : if w32 and accum are signed variables, the extract will produce a "signed" result
//        if w32 and accum are unsigned variables, the extract will produce an "unsigned" result
// initialize stream for extraction
#define BE64_XTRACT_BEGIN(accum, xtract, stream) { uint32_t t = *(stream) ; accum = t ; accum <<= 32 ; (stream)++ ; xtract = 32 ; }
// take a peek at nbits bits from accum into w32 (unsafe, assumes that nbits bits are available)
#define BE64_PEEK_NBITS(accum, xtract, w32, nbits) { w32 = accum >> (64 - (nbits)) ; }
// extract nbits bits into w32 from accum, update xtract, accum (unsafe, assumes that nbits bits are available)
#define BE64_XTRACT_NBITS(accum, xtract, w32, nbits) \
        { w32 = accum >> (64 - (nbits)) ; accum <<= (nbits) ; xtract -= (nbits) ; }
// check that 32 bits can be safely extracted from accum
// if not possible, get extra 32 bits into accum from stresm, update accum, xtract, stream
#define BE64_XTRACT_CHECK(accum, xtract, stream) \
        { if(xtract < 32) { accum = (uint64_t) accum >> (32-xtract) ; accum |= *(stream) ; accum <<= (32-xtract) ; xtract += 32 ; (stream)++ ; } ; }
// finalize extraction, update accum, xtract
#define BE64_XTRACT_FINAL(accum, xtract) { accum = 0 ; xtract = 0 ; }
// combined XTRACT_CHECK and XTRACT_NBITS, update accum, xtract, stream
#define BE64_GET_NBITS(accum, xtract, w32, nbits, stream) \
        { BE64_XTRACT_CHECK(accum, xtract, stream) ; BE64_XTRACT_NBITS(accum, xtract, w32, nbits) ; }
// align extraction point to a 32 bit boundary
#define BE64_XTRACT_ALIGN(accum, xtract) { uint32_t tbits = xtract ; tbits &= 31 ; accum <<= tbits ; xtract -= tbits ; }
//
