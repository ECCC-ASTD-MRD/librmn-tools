//
// Copyright (C) 2024  Environnement Canada
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
//     M. Valin,   Recherche en Prevision Numerique, 2024
//
// set of macros to manage insertion into/extraction from a 32 bit word based bit stream
//
#if ! defined(BITS_PUT_START)

#include <stdint.h>
#include <rmn/ct_assert.h>

// accumulator if filled from MSB toward LSB
// the topmost "insert" or "xtract" bits are useful data
// accumulator MUST BE a 64 bit (preferably UNSIGNED) variable
// the following MUST BE TRUE :  0 < nbits <= 32
// the macros always shift accumulator right as UNSIGNED 64 BITS

// declaration macros necessary to use the EZSTREAM insertion / extraction macors below
// declare variables for insertion
#define EZSTREAM_DCL_IN  uint64_t AcC_In  ; uint32_t InSeRt, *StReAm_In ;
// declare variables for extraction
#define EZSTREAM_DCL_OUT uint64_t AcC_OuT ; uint32_t XtRaCt, *StReAm_OuT ;

// get accumulator/count/pointer for insertion from stream32 structure
#define EZSTREAM_GET_IN(s)  AcC_In  = (s).acc_i ; InSeRt = (s).insert ; StReAm_In  = (s).in ;
// get accumulator/count/pointer for extraction from stream32 structure
#define EZSTREAM_GET_OUT(s) AcC_OuT = (s).acc_x ; XtRaCt = (s).xtract ; StReAm_OuT = (s).out ;

// if there are 32 or more valid bits in accumulator, push 32 bits back into stream
#define EZSTREAM_ADJUST_OUT if(XtRaCt >= 32) { AcC_OuT <<= 32 ; StReAm_OuT-- ; XtRaCt -= 32 ; } ;

// save accumulator/count/pointer for insertion into stream32 structure
#define EZSTREAM_SAVE_IN(s)  (s).acc_i = AcC_In  ; (s).insert = InSeRt ; (s).in  = StReAm_In ;
// save accumulator/count/pointer for extraction into stream32 structure
#define EZSTREAM_SAVE_OUT(s) EZSTREAM_ADJUST_OUT ; (s).acc_x = AcC_OuT ; (s).xtract = XtRaCt ; (s).out = StReAm_OuT ;

// access to stream32 in/out/limit(size) as indexes into a unit32_t array
#define EZSTREAM_IN(s)    ((s).in    - (s).first)
#define EZSTREAM_OUT(s)   ((s).out   - (s).first)
#define BITSTREAM_SIZE(s) ((s).limit - (s).first)

// =============== insertion macros ===============

// initialize stream for insertion, accumulator and inserted bits count are set to 0
#define BITS_PUT_START(accum, insert) { accum = 0 ; insert = 0 ; }
#define BITSTREAM_PUT_START(s) BITS_PUT_START( (s).acc_i, (s).insert )
#define EZSTREAM_PUT_START BITS_PUT_START(AcC_In, InSeRt)

// insert the lower nbits bits from w32 into accum, update insert, accum
// insert BELOW the topmost insert bits in accumulator
// (unsafe as it assumes that nbits bits can be inserted into acumulator)
#define BITS_PUT_FAST(accum, insert, w32, nbits) \
        { uint64_t t=(w32) ; t <<= (64-(nbits)) ; t >>= insert ; insert += (nbits) ; accum |= t ; }
//                           only keep lower nbits bits from t ; update count      ; insert
#define BITSTREAM_PUT_FAST(s, w32, nbits) BITS_PUT_FAST((s).acc_i, (s).insert, w32, nbits)
#define EZSTREAM_PUT_FAST(w32, nbits) BITS_PUT_FAST(AcC_In, InSeRt,  w32, nbits)

// check that 32 bits can be safely inserted into accum
// if not possible, store upper 32 bits of accum into stream, update accum, insert, stream
#define BITS_PUT_CHECK(accum, insert, stream) \
        { *(stream) = (uint64_t) accum >> 32 ; if(insert > 32) { insert -= 32 ; (stream)++ ; accum <<= 32 ; } ; }
//       always store upper 32 bits of accum ; if necessary  :   update count ; bump stream ; remove stored bits
#define BITSTREAM_PUT_CHECK(s) BITS_PUT_CHECK((s).acc_i, (s).insert, (s).in)
#define EZSTREAM_PUT_CHECK BITS_PUT_CHECK(AcC_In, InSeRt, StReAm_In)

// combined CHECK and INSERT, update accum, insert, stream
#define BITS_PUT_SAFE(accum, insert, w32, nbits, stream) \
        { BITS_PUT_CHECK(accum, insert, stream) ; BITS_PUT_FAST(accum, insert, w32, nbits) ; }
#define BITSTREAM_PUT_SAFE(s, w32, nbits) BITS_PUT_SAFE((s).acc_i, (s).insert, w32, nbits, (s).in)
#define EZSTREAM_PUT_SAFE(w32, nbits) BITS_PUT_SAFE(AcC_In, InSeRt, w32, nbits, StReAm_In)

// push all accumulator data into stream without fully updating control info (stream, insert)
#define BITS_PUSH(accum, insert, stream) \
        { BITS_PUT_CHECK(accum, insert, stream) ; if(insert > 0) { *stream = (uint64_t) accum >> 32 ; } }
//                                              if there is residual data in accumulator store it
#define BITSTREAM_PUSH(s) BITS_PUSH((s).acc_i, (s).insert, (s).in)
#define EZSTREAM_PUSH BITS_PUSH(AcC_In, InSeRt, StReAm_In)

// store any residual data from accum into stream, update insert, stream
#define BITS_PUT_END(accum, insert, stream) \
        { BITS_PUT_CHECK(accum, insert, stream) ; if(insert > 0) { *stream = (uint64_t) accum >> 32 ; stream++ ; insert = 0 ; accum = 0 ;} }
//                                      if there is residual data in accumulator store it, bump stream ptr, mark accum as empty
#define BITSTREAM_PUT_END(s) BITS_PUT_END((s).acc_i, (s).insert, (s).in)
#define EZSTREAM_PUT_END BITS_PUT_END(AcC_In, InSeRt, StReAm_In)

// align insertion point to a 32 bit boundary
//  make insert a 32 bit multiple              free bits             modulo 31
#define BITS_PUT_ALIGN(accum, insert) { uint32_t tbits = 64 - insert ; tbits &= 31 ; insert += tbits ; }
#define BITSTREAM_PUT_ALIGN(s) BITS_PUT_ALIGN((s).acc_i, (s).insert)
#define EZSTREAM_PUT_ALIGN BITS_PUT_ALIGN(AcC_In, InSeRt)

// =============== extraction macros ===============
// N.B. : if w32 is a signed variable, the extract will produce a "signed" result
//        if w32 is an unsigned variable, the extract will produce an "unsigned" result
//
// initialize stream for extraction, fill 32 bits
//                                                                               fill topmost bits  bup stream   32 useful bits
#define BITS_GET_START(accum, xtract, stream) { uint32_t t = *(stream) ; accum = t ; accum <<= 32 ; (stream)++ ; xtract = 32 ; }
#define BITSTREAM_GET_START(s) BITS_GET_START( (s).acc_x, (s).xtract, (s).out)
#define EZSTREAM_GET_START BITS_GET_START(AcC_OuT, XtRaCt, StReAm_OuT)

// take a peek at the next nbits bits from accum into w32 (unsafe, assumes that nbits bits are available)
// if w32 is signed, the result will be signed, if w32 is unsigned, the result will be unsigned
#define BITS_PEEK(accum, xtract, w32, nbits) { w32 = (uint64_t) accum >> 32 ; w32 = w32 >> (32 - (nbits)) ; }
#define BITSTREAM_PEEK(w32, nbits) BITS_PEEK((s).acc_x, (s).xtract, w32, nbits)
#define EZSTREAM_PEEK(w32, nbits) BITS_PEEK(AcC_OuT, XtRaCt,  w32, nbits)

// skip the next nbits bits from accum (unsafe, assumes that nbits bits are available)
#define BITS_SKIP(accum, xtract, nbits) { accum <<= (nbits) ; xtract -= (nbits) ; }
#define BITSTREAM_SKIP(s, nbits) BITS_SKIP((s).acc_x, (s).xtract, nbits)
#define EZSTREAM_SKIP(nbits) BITS_SKIP(AcC_OuT, XtRaCt, nbits)

// extract nbits bits into w32 from accum, update xtract, accum (unsafe, assumes that nbits bits are available)
#define BITS_GET_FAST(accum, xtract, w32, nbits) \
        { BITS_PEEK(accum, xtract, w32, nbits) ; BITS_SKIP(accum, xtract, nbits) ; }
#define BITSTREAM_GET_FAST(w32, nbits) BITS_GET_FAST((s).acc_x, (s).xtract, w32, nbits)
#define EZSTREAM_GET_FAST(w32, nbits) BITS_GET_FAST(AcC_OuT, XtRaCt, w32, nbits)

// check that 32 bits can be safely extracted from accum
// if not possible, get extra 32 bits into accum from stresm, update accum, xtract, stream
#define BITS_GET_CHECK(accum, xtract, stream) \
        { if(xtract < 32) { accum = (uint64_t) accum >> (32-xtract) ; accum |= *(stream) ; accum <<= (32-xtract) ; xtract += 32 ; (stream)++ ; } ; }
#define BITSTREAM_GET_CHECK(s) BITS_GET_CHECK((s).acc_x, (s).xtract, (s).out)
#define EZSTREAM_GET_CHECK BITS_GET_CHECK(AcC_OuT, XtRaCt, StReAm_OuT)

// combined CHECK and EXTRACT, update accum, xtract, stream
#define BITS_GET_SAFE(accum, xtract, w32, nbits, stream) \
        { BITS_GET_CHECK(accum, xtract, stream) ; BITS_GET_FAST(accum, xtract, w32, nbits) ; }
#define BITSTREAM_GET_SAFE(s, w32, nbits) BITS_GET_SAFE((s).acc_x, (s).xtract, w32, nbits, (s).out)
#define EZSTREAM_GET_SAFE(w32, nbits) BITS_GET_SAFE(AcC_OuT, XtRaCt, w32, nbits, StReAm_OuT)

// finalize extraction, update accum, xtract
#define BITS_GET_END(accum, xtract) { accum = 0 ; xtract = 0 ; }
#define BITSTREAM_GET_END(s) BITS_GET_END((s).acc_x, (s).xtract)
#define EZSTREAM_GET_END BITS_GET_END(AcC_OuT, XtRaCt)

// align extraction point to a 32 bit boundary
//  make xtract a 32 bit multiple                available bits   modulo 32
#define BITS_GET_ALIGN(accum, xtract) { uint32_t tbits = xtract ; tbits &= 31 ; accum <<= tbits ; xtract -= tbits ; }
#define BITSTREAM_GET_ALIGN(s) BITS_GET_ALIGN((s).acc_x, (s).xtract)
#define EZSTREAM_GET_ALIGN BITS_GET_ALIGN(AcC_OuT, XtRaCt)

// word (32 bit) stream descriptor
// number of words in buffer : in - out (available for extraction)
// space available in buffer : limit - in (available for insertion)
// if alloc == 1, a call to realloc is O.K., limit, in and out must be adjusted
// the 2 most popular trios [acc_i, in, insert] and [xtract, out, acc_x] are adjacent in memory
// the fields from the following struct should only be accessed through the appropriate macros
typedef struct{
  uint32_t *first ;    // pointer to start of stream data storage
  uint32_t *limit ;    // buffer size (in 32 bit units)
  uint64_t acc_i ;     // accumulator for insertion
  uint32_t *in ;       // insertion index (0 initially)
  uint32_t insert ;    // number of inserted bits in acc_i
  uint32_t xtract ;    // umber of available bits in acc_x
  uint32_t *out ;      // extraction index (0 initially)
  uint64_t acc_x ;     // accumulator for extraction
  uint64_t valid:32 ,  // signature marker
           spare:31 ,  // spare bits
           alloc: 1 ;  // buffer allocated with malloc (realloc is possible)
  uint32_t data[] ;    // dynamic array, only used if alloc is true
} stream32 ;
CT_ASSERT(sizeof(stream32) == 64, "wordstream size MUST be 56 bytes")    // 8 64 bit elements (7 x 64 bits + 2 x 32 bits)

#endif
