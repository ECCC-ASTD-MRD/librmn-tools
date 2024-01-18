//
// Copyright (C) 2022  Environnement Canada
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
//     M. Valin,   Recherche en Prevision Numerique, 2022-2023
//
// set of macros and functions to manage insertion/extraction into/from a bit stream or a word stream

// MAKE_SIGNED_32 is defined in this file and acts as double include detection
#if ! defined(MAKE_SIGNED_32)

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

// compile time assertions
#include <rmn/ct_assert.h>
// word stream macros and functions
#include <rmn/word_stream.h>
// bit stream macros and functions
#include <rmn/bit_stream.h>
//
// EZ macros use an implicit stream (see EZ macros preamble/postamble further down)
// initialize stream for insertion
//   [L|B]E64_STREAM_INSERT_BEGIN(stream)
//   [L|B]E64_EZ_INSERT_BEGIN
// insert the lower nbits bits from uint32 into accumulator, update counter, accumulator
//   [L|B]E64_STREAM_INSERT_NBITS(stream, uint32, nbits)
//   [L|B]E64_EZ_INSERT_NBITS(uint32, nbits)
// check that 32 bits can be safely inserted into accumulator
// if not possible, store lower 32 bits of accumulator into stream, update accumulator, counter, stream
//   [L|B]E64_STREAM_INSERT_CHECK(stream)
//   [L|B]E64_EZ_INSERT_CHECK
// push data to stream without fully updating accumulator, stream, counter
//   [L|B]E64_STREAM_PUSH(stream)
//   [L|B]E64_EZ_PUSH
// store any residual data from accumulator into stream, update accumulator, counter, stream
//   [L|B]E64_STREAM_INSERT_FINAL(stream)
//   [L|B]E64_EZ_INSERT_FINAL
// combined INSERT_CHECK and INSERT_NBITS, update accumulator, counter, stream
//   [L|B]E64_STREAM_PUT_NBITS(stream, uint32, nbits)
//   [L|B]E64_EZ_PUT_NBITS(uint32, nbits)
// align insertion point to a 32 bit boundary
//   [L|B]E64_STREAM_INSERT_ALIGN(stream)
//   [L|B]E64_EZ_INSERT_ALIGN
//
// N.B. : if uint32 and accumulator are signed variables, the extract will produce a "signed" result
//        if uint32 and accumulator are unsigned variables, the extract will produce an "unsigned" result
// initialize stream for extraction
//   [L|B]E64_STREAM_XTRACT_BEGIN(stream)
//   [L|B]E64_EZ_XTRACT_BEGIN
// take a peek at nbits bits from accumulator into uint32
//   [L|B]E64_STREAM_PEEK_NBITS(stream, uint32, nbits)
//   [L|B]E64_STREAM_PEEK_NBITS_S(stream, uint32, nbits)
//   [L|B]E64_EZ_PEEK_NBITS(uint32, nbits)
//   [L|B]E64_EZ_PEEK_NBITS_S(uint32, nbits)
// extract nbits bits into uint32 from accumulator, update xtract, accumulator
//   [L|B]E64_STREAM_XTRACT_NBITS(stream, uint32, nbits)
//   [L|B]E64_EZ_XTRACT_NBITS(uint32, nbits)
// check that 32 bits can be safely extracted from accumulator
// if not possible, get extra 32 bits into accumulator from stream, update accumulator, xtract, stream
//   [L|B]E64_STREAM_XTRACT_CHECK(stream)
//   [L|B]E64_EZ_XTRACT_CHECK
// finalize extraction, update accumulator, xtract
//   [L|B]E64_STREAM_XTRACT_FINAL(stream)
//   [L|B]E64_EZ_XTRACT_FINAL
// combined XTRACT_CHECK and XTRACT_NBITS, update accumulator, xtract, stream
//   [L|B]E64_STREAM_GET_NBITS(stream, uint32, nbits)
//   [L|B]E64_EZ_GET_NBITS(uint32, nbits)
// align extraction point to a 32 bit boundary
//   [L|B]E64_STREAM_XTRACT_ALIGN(stream)
//   [L|B]E64_EZ_XTRACT_ALIGN

#if ! defined(STATIC)
#define STATIC static
#define STATIC_DEFINED_HERE
#endif

CT_ASSERT(sizeof(uint64_t) == 8) ;     // this better be true !
CT_ASSERT(sizeof(void *) == 8) ;       // enforce 64 bit pointers

// simple data buffer structure with capacity and fill level
typedef struct{
  int32_t size ;      // max data size in 32 bit units (max 16G - 4 bytes)
  int32_t next ;      // next insertion point in buf (0 in empty bucket)
  uint32_t buf[] ;    // data buffer
} bitbucket ;

// convert the nbits rightmost bits to a 2s complement signed number
// for this macro to produce meaningful results, w32 MUST BE int32_t (32 bit signed int)
#define MAKE_SIGNED_32(w32, nbits) { w32 <<= (32 - (nbits)) ; w32 >>= (32 - (nbits)) ; }
// for this macro to produce meaningful results, w64 MUST BE int64_t (64 bit signed int)
#define MAKE_SIGNED_64(w64, nbits) { w64 <<= (64 - (nbits)) ; w64 >>= (64 - (nbits)) ; }

// ========================== EZ macros preamble/postamble ===========================
// transfer bitstream field to/from local variable
// stream field xx <-> local variable RtReAm_xx

// EZ variables for extraction
// simple declaration of extraction variables
#define EZ_DCL_XTRACT_VARS \
  uint64_t StReAm_acc_x ; \
  int32_t  StReAm_xtract ; \
  uint32_t *StReAm_out ;
// declare extraction variables and set them to value from stream
#define EZ_NEW_XTRACT_VARS(s) \
  uint64_t StReAm_acc_x  = (s).acc_x ; \
  int32_t  StReAm_xtract = (s).xtract ; \
  uint32_t *StReAm_out   = (s).out ;
// get extraction variable values from stream
#define EZ_GET_XTRACT_VARS(s) \
  StReAm_acc_x  = (s).acc_x ; \
  StReAm_xtract = (s).xtract ; \
  StReAm_out    = (s).out ;
// update stream with current extraction variable values
#define EZ_SET_XTRACT_VARS(s) \
  (s).acc_x  = StReAm_acc_x ; \
  (s).xtract = StReAm_xtract ; \
  (s).out    = StReAm_out ;

// EZ variables for insertion
// simple declaration of insertion variables
#define EZ_DCL_INSERT_VARS \
  uint64_t StReAm_acc_i ; \
  int32_t  StReAm_insert ; \
  uint32_t *StReAm_in ;
// declare insertion variables and set them to value from stream
#define EZ_NEW_INSERT_VARS(s)\
  uint64_t StReAm_acc_i  = (s).acc_i ; \
  int32_t  StReAm_insert = (s).insert ; \
  uint32_t *StReAm_in    = (s).in ;
// get insertion variable values from stream
#define EZ_GET_INSERT_VARS(s)\
  StReAm_acc_i  = (s).acc_i ; \
  StReAm_insert = (s).insert ; \
  StReAm_in     = (s).in ;
// update stream with current insertion variable values
#define EZ_SET_INSERT_VARS(s) \
  (s).acc_i  = StReAm_acc_i ; \
  (s).insert = StReAm_insert ; \
  (s).in     = StReAm_in ;
//
// ================================ bit insertion/extraction macros into/from bitstream ===============================
// macro arguments description
// insert [INOUT] : # of bits used in accumulator (0 <= insert <= 64)
// xtract [INOUT] : # of bits extractable from accumulator (0 <= xtract <= 64)
// s      [INOUT] : bit stream
// w32    [IN]    : 32 bit integer containing data to be inserted (expression allowed)
//        [OUT]   : 32 bit integer receiving extracted data (MUST be a variable)
// nbits  [IN]    : number of bits to insert / extract in w32 (<= 32 bits)
//
// N.B. : if w32 and accum are "signed" variables, extraction will produce a "signed" result
//        if w32 and accum are "unsigned" variables, extraction will produce an "unsigned" result
// the EZ and STREAM macros use implicit accum/xtract/xtract/stream arguments
// EZ macros     : StReAm_acc_i/StReAm_acc_x, StReAm_insert/StReAm_xtract, StReAm_in/StReAm_out variables
// STREAM macros : acc_i/acc_x, insert/xtract, in/out fields from bitstream struct
// ===============================================================================================
// little endian (LE) style (right to left) bit stream packing
// insertion at top (most significant part) of accum
// extraction from the bottom (least significant part) of accum
// ===============================================================================================
// initialize stream for insertion
#define LE64_EZ_INSERT_BEGIN        { LE64_INSERT_BEGIN(StReAm_acc_i, StReAm_insert) }
#define LE64_STREAM_INSERT_BEGIN(s) { LE64_INSERT_BEGIN((s).acc_i,    (s).insert) }
// insert the lower nbits bits from w32 into accum, update insert, accum
#define LE64_EZ_INSERT_NBITS(w32, nbits)        LE64_INSERT_NBITS(StReAm_acc_i, StReAm_insert, w32, nbits)
#define LE64_STREAM_INSERT_NBITS(s, w32, nbits) LE64_INSERT_NBITS((s).acc_i,    (s).insert,    w32, nbits)
// check that 32 bits can be safely inserted into accum
// if not possible, store lower 32 bits of accum into stream, update accum, insert, stream
#define LE64_EZ_INSERT_CHECK        LE64_INSERT_CHECK(StReAm_acc_i, StReAm_insert, StReAm_in)
#define LE64_STREAM_INSERT_CHECK(s) LE64_INSERT_CHECK((s).acc_i,    (s).insert,    (s).in)
// push data to stream without fully updating control info (stream, insert)
#define LE64_EZ_PUSH     LE64_PUSH(StReAm_acc_i, StReAm_insert, StReAm_in)
#define LE64_STREAM_PUSH(s) LE64_PUSH((s).acc_i,    (s).insert,    (s).in)
// store any residual data from accum into stream, update accum, insert, stream
#define LE64_EZ_INSERT_FINAL        LE64_INSERT_FINAL(StReAm_acc_i, StReAm_insert, StReAm_in)
#define LE64_STREAM_INSERT_FINAL(s) LE64_INSERT_FINAL((s).acc_i,    (s).insert,    (s).in)
// combined INSERT_CHECK and INSERT_NBITS, update accum, insert, stream
#define LE64_EZ_PUT_NBITS(w32, nbits)        LE64_PUT_NBITS(StReAm_acc_i, StReAm_insert, w32, nbits, StReAm_in)
#define LE64_STREAM_PUT_NBITS(s, w32, nbits) LE64_PUT_NBITS((s).acc_i,    (s).insert,    w32, nbits, (s).in)
// align insertion point to a 32 bit boundary
#define LE64_EZ_INSERT_ALIGN        LE64_INSERT_ALIGN(StReAm_insert)
#define LE64_STREAM_INSERT_ALIGN(s) LE64_INSERT_ALIGN((s).insert)
// #define LE64_EZ_INSERT_ALIGN        { uint32_t tbits = 64 - StReAm_insert ; tbits &= 31 ; StReAm_insert += tbits ; }
// #define LE64_STREAM_INSERT_ALIGN(s) { uint32_t tbits = 64 - StReAm_insert ; tbits &= 31 ; (s).insert += tbits ; }
//
// N.B. : if w32 and accum are signed variables, the extract will produce a "signed" result
//        if w32 and accum are unsigned variables, the extract will produce an "unsigned" result
// initialize stream for extraction
#define LE64_EZ_XTRACT_BEGIN { LE64_XTRACT_BEGIN(StReAm_acc_x, StReAm_xtract, StReAm_out) ; }
#define LE64_STREAM_XTRACT_BEGIN(s) { LE64_XTRACT_BEGIN((s).acc_x, (s).xtract, (s).out) ; }
// take a peek at nbits bits from accum into w32
#define LE64_EZ_PEEK_NBITS(w32, nbits)          LE64_PEEK_NBITS(StReAm_acc_x, StReAm_xtract, w32, nbits)
#define LE64_STREAM_PEEK_NBITS(s, w32, nbits)   LE64_PEEK_NBITS((s).acc_x,    (s).xtract,    w32, nbits)
#define LE64_EZ_PEEK_NBITS_S(w32, nbits)        LE64_PEEK_NBITS((int64_t) StReAm_acc_x, StReAm_xtract, w32, nbits)
#define LE64_STREAM_PEEK_NBITS_S(s, w32, nbits) LE64_PEEK_NBITS((int64_t) (s).acc_x,    (s).xtract,    w32, nbits)
// extract nbits bits into w32 from accum, update xtract, accum
#define LE64_EZ_XTRACT_NBITS(w32, nbits)        LE64_XTRACT_NBITS(StReAm_acc_x, StReAm_xtract, w32, nbits)
#define LE64_STREAM_XTRACT_NBITS(s, w32, nbits) LE64_XTRACT_NBITS((s).acc_x,    (s).xtract,    w32, nbits)
// check that 32 bits can be safely extracted from accum
// if not possible, get extra 32 bits into accum from stresm, update accum, xtract, stream
#define LE64_EZ_XTRACT_CHECK        LE64_XTRACT_CHECK(StReAm_acc_x, StReAm_xtract, StReAm_out)
#define LE64_STREAM_XTRACT_CHECK(s) LE64_XTRACT_CHECK((s).acc_x,    (s).xtract,    (s).out)
// finalize extraction, update accum, xtract
#define LE64_EZ_XTRACT_FINAL        LE64_XTRACT_FINAL(StReAm_acc_x, StReAm_xtract)
#define LE64_STREAM_XTRACT_FINAL(s) LE64_XTRACT_FINAL((s).acc_x,    (s).xtract)
// combined XTRACT_CHECK and XTRACT_NBITS, update accum, xtract, stream
#define LE64_EZ_GET_NBITS(w32, nbits)        LE64_GET_NBITS(StReAm_acc_x, StReAm_xtract, w32, nbits, StReAm_out)
#define LE64_STREAM_GET_NBITS(s, w32, nbits) LE64_GET_NBITS((s).acc_x,    (s).xtract,    w32, nbits, (s).out)
// align extraction point to a 32 bit boundary
#define LE64_EZ_XTRACT_ALIGN        LE64_XTRACT_ALIGN(StReAm_acc_x, StReAm_xtract)
#define LE64_STREAM_XTRACT_ALIGN(s) LE64_XTRACT_ALIGN((s).acc_x,    (s).xtract)
// #define LE64_EZ_XTRACT_ALIGN { uint32_t tbits = StReAm_xtract ; tbits &= 31 ; StReAm_acc_x >>= tbits ; StReAm_xtract -= tbits ; }
// #define LE64_STREAM_XTRACT_ALIGN { uint32_t tbits = (s).xtract ; tbits &= 31 ; (s).acc_x >>= tbits ; (s).xtract -= tbits ; }
//
// ===============================================================================================
// big endian (BE) style (left to right) bit stream packing
// insertion at bottom (least significant part) of accumulator after accumulator shifted left
// extraction from the top (most significant part) of accumulator then shift accumulator left
// ===============================================================================================
// initialize stream for insertion
#define BE64_EZ_INSERT_BEGIN        { BE64_INSERT_BEGIN(StReAm_acc_i, StReAm_insert) ; }
#define BE64_STREAM_INSERT_BEGIN(s) { BE64_INSERT_BEGIN((s).acc_i,    (s).insert) ; }
// insert the lower nbits bits from w32 into accum, update insert, accum
#define BE64_EZ_INSERT_NBITS(w32, nbits)        BE64_INSERT_NBITS(StReAm_acc_i, StReAm_insert, w32, nbits)
#define BE64_STREAM_INSERT_NBITS(s, w32, nbits) BE64_INSERT_NBITS((s).acc_i,    (s).insert,    w32, nbits)
// check that 32 bits can be safely inserted into accum
// if not possible, store lower 32 bits of accum into stream, update accum, insert, stream
#define BE64_EZ_INSERT_CHECK        BE64_INSERT_CHECK(StReAm_acc_i, StReAm_insert, StReAm_in)
#define BE64_STREAM_INSERT_CHECK(s) BE64_INSERT_CHECK((s).acc_i,    (s).insert,    (s).in)
// push data to stream without fully updating control info (stream, insert)
#define BE64_EZ_PUSH        BE64_PUSH(StReAm_acc_i, StReAm_insert, StReAm_in)
#define BE64_STREAM_PUSH(s) BE64_PUSH((s).acc_i,    (s).insert,    (s).in)
// store any residual data from accum into stream, update accum, insert, stream
#define BE64_EZ_INSERT_FINAL        BE64_INSERT_FINAL(StReAm_acc_i, StReAm_insert, StReAm_in)
#define BE64_STREAM_INSERT_FINAL(s) BE64_INSERT_FINAL((s).acc_i,    (s).insert,    (s).in)
// combined INSERT_CHECK and INSERT_NBITS, update accum, insert, stream
#define BE64_EZ_PUT_NBITS(w32, nbits)        BE64_PUT_NBITS(StReAm_acc_i, StReAm_insert, w32, nbits, StReAm_in)
#define BE64_STREAM_PUT_NBITS(s, w32, nbits) BE64_PUT_NBITS((s).acc_i,    (s).insert,    w32, nbits, (s).in)
// align insertion point to a 32 bit boundary
#define BE64_EZ_INSERT_ALIGN        BE64_INSERT_ALIGN(StReAm_acc_i, StReAm_insert)
#define BE64_STREAM_INSERT_ALIGN(s) BE64_INSERT_ALIGN((s).acc_i,    (s).insert)
// #define BE64_EZ_INSERT_ALIGN        { uint32_t tbits = 64 - StReAm_insert ; tbits &= 31 ; StReAm_insert += tbits ; StReAm_acc_i <<= tbits ; }
// #define BE64_STREAM_INSERT_ALIGN(s) { uint32_t tbits = 64 - StReAm_insert ; tbits &= 31 ; (s).insert += tbits ; (s).acc_i <<= tbits ; }
//
// N.B. : if w32 and accum are signed variables, the extract will produce a "signed" result
//        if w32 and accum are unsigned variables, the extract will produce an "unsigned" result
// initialize stream for extraction
#define BE64_EZ_XTRACT_BEGIN        { BE64_XTRACT_BEGIN(StReAm_acc_x, StReAm_xtract, StReAm_out) ; }
#define BE64_STREAM_XTRACT_BEGIN(s) { BE64_XTRACT_BEGIN((s).acc_x,    (s).xtract,    (s).out) ; }
// take a peek at nbits bits from accum into w32
#define BE64_EZ_PEEK_NBITS(w32, nbits)          BE64_PEEK_NBITS(StReAm_acc_x, StReAm_xtract, w32, nbits)
#define BE64_STREAM_PEEK_NBITS(s, w32, nbits)   BE64_PEEK_NBITS((s).acc_x,    (s).xtract,    w32, nbits)
#define BE64_EZ_PEEK_NBITS_S(w32, nbits)        BE64_PEEK_NBITS((int64_t) StReAm_acc_x, StReAm_xtract, w32, nbits)
#define BE64_STREAM_PEEK_NBITS_S(s, w32, nbits) BE64_PEEK_NBITS((int64_t) (s).acc_x,    (s).xtract,    w32, nbits)
// extract nbits bits into w32 from accum, update xtract, accum
#define BE64_EZ_XTRACT_NBITS(w32, nbits)        BE64_XTRACT_NBITS(StReAm_acc_x, StReAm_xtract, w32, nbits)
#define BE64_STREAM_XTRACT_NBITS(s, w32, nbits) BE64_XTRACT_NBITS((s).acc_x,    (s).xtract,    w32, nbits)
// check that 32 bits can be safely extracted from accum
// if not possible, get extra 32 bits into accum from stresm, update accum, xtract, stream
#define BE64_EZ_XTRACT_CHECK        BE64_XTRACT_CHECK(StReAm_acc_x, StReAm_xtract, StReAm_out)
#define BE64_STREAM_XTRACT_CHECK(s) BE64_XTRACT_CHECK((s).acc_x,    (s).xtract,    (s).out)
// finalize extraction, update accum, xtract
#define BE64_EZ_XTRACT_FINAL(s)     BE64_XTRACT_FINAL(StReAm_acc_x, StReAm_xtract)
#define BE64_STREAM_XTRACT_FINAL(s) BE64_XTRACT_FINAL((s).acc_x,    (s).xtract)
// combined XTRACT_CHECK and XTRACT_NBITS, update accum, xtract, stream
#define BE64_EZ_GET_NBITS(w32, nbits)        BE64_GET_NBITS(StReAm_acc_x, StReAm_xtract, w32, nbits, StReAm_out)
#define BE64_STREAM_GET_NBITS(s, w32, nbits) BE64_GET_NBITS((s).acc_x,    (s).xtract,    w32, nbits, (s).out)
// align extraction point to a 32 bit boundary
#define BE64_EZ_XTRACT_ALIGN        BE64_XTRACT_ALIGN(StReAm_acc_x, StReAm_xtract)
#define BE64_STREAM_XTRACT_ALIGN(s) BE64_XTRACT_ALIGN((s).acc_x,    (s).xtract)
// #define BE64_EZ_XTRACT_ALIGN     { uint32_t tbits = StReAm_xtract ; tbits &= 31 ; StReAm_acc_x <<= tbits ; StReAm_xtract -= tbits ; }
// #define BE64_STREAM_XTRACT_ALIGN { uint32_t tbits = (s).xtract ; tbits &= 31 ; (s).acc_x <<= tbits ; (s).xtract -= tbits ; }
//
// STATIC inline void  LeStreamReset(bitstream *p, uint32_t *buffer){
//   p->accum = 0 ;
//   p->insert = 0 ;
//   p->xtract = 0 ;
//   p->stream = buffer ;
//   p->first  = buffer ;
// }
// 
// STATIC inline void  BeStreamReset(bitstream *p, uint32_t *buffer){
//   p->accum = 0 ;
//   p->insert = 0 ;
//   p->xtract = 0 ;
//   p->stream = buffer ;
//   p->first  = buffer ;
// }

#if defined(STATIC_DEFINED_HERE)
#undef STATIC
#undef STATIC_DEFINED_HERE
#endif

#endif
