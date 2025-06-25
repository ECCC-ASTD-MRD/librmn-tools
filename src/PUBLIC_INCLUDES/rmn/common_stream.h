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
// these macros are common to Big Endian and Little Endian streams
//
// undefine everything in case of multiple inclusion
//
// ===============================================================================================
// same code for Big and Little Endian modes, avoids duplication
// initialize for insertion, accumulator and inserted bits count are set to 0
#undef INSERT_BEGIN
#define INSERT_BEGIN(accum, insert) { accum = 0 ; insert = 0 ; }

// safely put lower nbits from w32 into accumulator, update accum, xtract, stream pointer
#undef PUT_NBITS
#define PUT_NBITS(accum, insert, w32, nbits, streamptr) \
        { INSERT_CHECK(accum, insert, streamptr) ; FAST_PUT_NBITS(accum, insert, w32, nbits) ; }

// safely put 1 into accumulator, update accum, xtract, stream pointer
#undef PUT_1
#define PUT_1(accum, insert, streamptr) \
        { INSERT_CHECK(accum, insert, streamptr) ; FAST_PUT_1(accum, insert) ; }

// safely put 0 into accumulator, update accum, xtract, stream pointer
#undef PUT_0
#define PUT_0(accum, insert, streamptr) \
        { INSERT_CHECK(accum, insert, streamptr) ; FAST_PUT_0(accum, insert) ; }

// extract nbits bits into w32 from accumulator, update xtract, accum (unsafe, assumes that nbits bits are available)
#undef FAST_GET_NBITS
#define FAST_GET_NBITS(accum, xtract, w32, nbits) \
        { FAST_PEEK_NBITS(accum, xtract, w32, nbits) ; FAST_SKIP_NBITS(accum, xtract, nbits) ; }

// extract 1 bit into w32 from accumulator, update xtract, accum (unsafe, assumes that 1 bit is available)
#undef FAST_GET_1
#define FAST_GET_1(accum, xtract, w32) \
        { FAST_PEEK_1(accum, xtract, w32) ; FAST_SKIP_1(accum, xtract) ; }

// safely get nbits into w32, update accum, xtract, stream pointer
#undef GET_NBITS
#define GET_NBITS(accum, xtract, w32, nbits, streamptr) \
        { XTRACT_CHECK(accum, xtract, streamptr) ; FAST_GET_NBITS(accum, xtract, w32, nbits) ; }

// safely get nbits into w32, update accum, xtract, stream pointer
#undef GET_1
#define GET_1(accum, xtract, w32, streamptr) \
        { XTRACT_CHECK(accum, xtract, streamptr) ; FAST_GET_1(accum, xtract, w32) ; }

// ===============================================================================================
// for stream insertion, accum will be (s).acc_i, stream pointer will be (s).in

// initialize a stream for insertion, accumulator and inserted bits count are set to 0
#undef STREAM_INSERT_BEGIN
#define STREAM_INSERT_BEGIN(s) { INSERT_BEGIN((s).acc_i, (s).insert) }

#undef STREAM_FAST_PUT_NBITS
#define STREAM_FAST_PUT_NBITS(s, w32, nbits) { FAST_PUT_NBITS((s).acc_i, (s).insert, w32, nbits) }

#undef STREAM_FAST_PUT_1
#define STREAM_FAST_PUT_1(s) { FAST_PUT_1((s).acc_i, (s).insert) }

#undef STREAM_FAST_PUT_0
#define STREAM_FAST_PUT_0(s) { FAST_PUT_0((s).acc_i, (s).insert) }

#undef STREAM_FAST_PUT_PAD0
#define STREAM_FAST_PUT_PAD0(s, nbits) { FAST_PUT_PAD0((s).acc_i, (s).insert), nbits }

#undef STREAM_INSERT_CHECK
#define STREAM_INSERT_CHECK(s) { INSERT_CHECK((s).acc_i, (s).insert, (s).in) }

#undef STREAM_INSERT_PUSH
#define STREAM_INSERT_PUSH(s) { INSERT_PUSH((s).acc_i, (s).insert, (s).in) }

#undef STREAM_INSERT_FINALIZE
#define STREAM_INSERT_FINALIZE(s) { INSERT_FINALIZE((s).acc_i, (s).insert, (s).in) }

#undef STREAM_PUT_NBITS
#define STREAM_PUT_NBITS(s, w32, nbits) { PUT_NBITS((s).acc_i, (s).insert, w32, nbits, (s).in) }

#undef STREAM_PUT_1
#define STREAM_PUT_1(s) { PUT_1((s).acc_i, (s).insert, (s).in) }

#undef STREAM_PUT_0
#define STREAM_PUT_0(s) { PUT_0((s).acc_i, (s).insert, (s).in) }

#undef STREAM_INSERT_ALIGN32
#define STREAM_INSERT_ALIGN32(s) { INSERT_ALIGN32((s).acc_i, (s).insert) }
#undef STREAM_INSERT_ALIGN16
#define STREAM_INSERT_ALIGN16(s) { INSERT_ALIGN16((s).acc_i, (s).insert) }
#undef STREAM_INSERT_ALIGN8
#define STREAM_INSERT_ALIGN8(s)  { INSERT_ALIGN8((s).acc_i, (s).insert) }

// ===============================================================================================
// for stream extraction, accum will be (s).acc_x, stream pointer will be (s).out
// if w32 is a "signed" variable, extraction will produce a "signed" result

#undef STREAM_XTRACT_BEGIN
#define STREAM_XTRACT_BEGIN(s) { XTRACT_BEGIN((s).acc_x, (s).xtract, (s).out) ; }

#undef STREAM_PEEK_NBITS
#define STREAM_PEEK_NBITS(s, w32, nbits) { STREAM_XTRACT_CHECK(s) ; FAST_PEEK_NBITS((s).acc_x, (s).xtract, w32, nbits) }

#undef STREAM_FAST_PEEK_NBITS
#define STREAM_FAST_PEEK_NBITS(s, w32, nbits) { FAST_PEEK_NBITS((s).acc_x, (s).xtract, w32, nbits) }

#undef STREAM_PEEK_1
#define STREAM_PEEK_1(s, w32) { STREAM_XTRACT_CHECK(s) ; FAST_PEEK_1((s).acc_x, (s).xtract, w32) }

#undef STREAM_FAST_PEEK_1
#define STREAM_FAST_PEEK_1(s, w32) { FAST_PEEK_1((s).acc_x, (s).xtract, w32) }

#undef STREAM_SKIP_NBITS
#define STREAM_SKIP_NBITS(s, nbits) { STREAM_XTRACT_CHECK(s) ; FAST_SKIP_NBITS((s).acc_x, (s).xtract, nbits) }

#undef STREAM_FAST_SKIP_NBITS
#define STREAM_FAST_SKIP_NBITS(s, nbits) { FAST_SKIP_NBITS((s).acc_x, (s).xtract, nbits) }

#undef STREAM_SKIP_1
#define STREAM_SKIP_1(s) { STREAM_XTRACT_CHECK(s) ; FAST_SKIP_1((s).acc_x, (s).xtract) }

#undef STREAM_FAST_SKIP_1
#define STREAM_FAST_SKIP_1(s) { FAST_SKIP_1((s).acc_x, (s).xtract) }

#undef STREAM_XTRACT_NBITS
#define STREAM_XTRACT_NBITS(s, w32, nbits) { FAST_GET_NBITS((s).acc_x, (s).xtract, w32, nbits) }

#undef STREAM_XTRACT_CHECK
#define STREAM_XTRACT_CHECK(s) { XTRACT_CHECK((s).acc_x, (s).xtract, (s).out) }

#undef STREAM_XTRACT_FINAL
#define STREAM_XTRACT_FINAL(s) { XTRACT_FINAL((s).acc_x, (s).xtract) }

#undef STREAM_GET_NBITS
#define STREAM_GET_NBITS(s, w32, nbits) { GET_NBITS((s).acc_x, (s).xtract, w32, nbits, (s).out) }

#undef STREAM_GET_1
#define STREAM_GET_1(s, w32) { GET_1((s).acc_x, (s).xtract, w32, (s).out) }

#undef STREAM_XTRACT_ALIGN32
#define STREAM_XTRACT_ALIGN32(s) { XTRACT_ALIGN32((s).acc_x, (s).xtract) }
#undef STREAM_XTRACT_ALIGN16
#define STREAM_XTRACT_ALIGN16(s) { XTRACT_ALIGN16((s).acc_x, (s).xtract) }
#undef STREAM_XTRACT_ALIGN8
#define STREAM_XTRACT_ALIGN8(s)  { XTRACT_ALIGN8((s).acc_x, (s).xtract) }

// ===============================================================================================
// TODO : add status argument to STREAM_REWIND, STREAM_REWRITE, STREAM_FLUSH ?
//
// rewind a bit stream to read it from the beginning (potentially force valid read mode)
#undef STREAM_REWIND
#define STREAM_REWIND(s, force_read) { \
  if((s).insert > 0) { STREAM_INSERT_PUSH(s) ; } if(force_read) { (s).xtract = 0 ; }  \
  if((s).xtract >= 0){ (s).acc_x  = 0 ; (s).out = (s).first ; (s).xtract = 0 ;} }

// rewind a bit stream to rewrite it from the beginning (potentially force valid write mode)
#undef STREAM_REWRITE
#define STREAM_REWRITE(s, force_write) { \
  if(force_write) { (s).insert = 0 ; }  \
  if((s).insert > 0) { STREAM_INSERT_PUSH(s) ; } \
  (s).acc_i  = 0 ; (s).in = (s).first ; }

// flush stream being written into if any data left in insertion accumulator
#undef STREAM_FLUSH
#define STREAM_FLUSH(s) { STREAM_INSERT_FINALIZE(s) }

// initialize stream and set it to the proper endian mode
#undef STREAM_INIT
#define STREAM_INIT(ps, mem, size, mode) { InitStream(ps, mem, size, mode) ; SET_STREAM_ENDIANNESS(*(ps)) ; }

// create stream and set it to the proper endian mode
#undef STREAM_CREATE
#define STREAM_CREATE(ps, mem, size, mode) { ps = CreateStream(mem, size, mode) ; SET_STREAM_ENDIANNESS(*(ps)) ; }

// destroy a stream
#undef STREAM_FREE
#define STREAM_FREE(ps, status) { ps = FreeStream(ps, &status) ; }
