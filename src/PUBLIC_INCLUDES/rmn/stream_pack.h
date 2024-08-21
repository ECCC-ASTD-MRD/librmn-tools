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
// BITS_xxxxxxxx macros  : base macros
// BITSTREAM_xxx macros  : direct individual use of stream32 struct (convenient, but slower)
// EZSTREAM_xxxx macros  : get stream32 info , multiple insert/extract, save stream32 info (as fast as BITS_xxx)
//
#if ! defined(STREAM32_VERSION)

#define STREAM32_VERSION 1
#define STREAM32_MARKER 0xDEADBEEF

#include <stdint.h>
#include <rmn/ct_assert.h>

// accumulator if filled from MSB toward LSB
// the topmost "insert" or "xtract" bits are useful data
// accumulator MUST BE a 64 bit (preferably UNSIGNED) variable
// the following MUST BE TRUE :  0 < nbits <= 32
// the macros always shift accumulator right as UNSIGNED 64 BITS
//
// macro arguments
// S32      stream32 struct                        (BITSTREAM_xxx EZSTREAM_ macros)
// ACCUM    64 bit unsigned accumulator            (BITS_xxxx macros)
// INSERT   inserted bit count for insertion       (BITS_xxxx macros)
// XTRACT   available bit count for extraction     (BITS_xxxx macros)
// STREAM   pointer into packed stream             (BITS_xxxx macros)
// W32      32 bit signed or unsigned variable     (all macros)
// NBITS    number of useful bits in W32           (all macros)

// declaration macros necessary to use the EZSTREAM_xxxx insertion / extraction macros
// declare variables for insertion
#define EZSTREAM_DCL_IN  uint64_t AcC_In  ; uint32_t InSeRt, *StReAm_In ;
// declare variables for extraction
#define EZSTREAM_DCL_OUT uint64_t AcC_OuT ; uint32_t XtRaCt, *StReAm_OuT ;

// get accumulator/count/pointer for insertion from stream32 structure
#define EZSTREAM_GET_IN(S32)  AcC_In  = (S32).acc_i ; InSeRt = (S32).insert ; StReAm_In  = (S32).in ;
// get accumulator/count/pointer for extraction from stream32 structure
#define EZSTREAM_GET_OUT(S32) AcC_OuT = (S32).acc_x ; XtRaCt = (S32).xtract ; StReAm_OuT = (S32).out ;

// if there are 32 or more valid bits in accumulator, push 32 bits back into stream
#define EZSTREAM_ADJUST_OUT if(XtRaCt >= 32) { AcC_OuT <<= 32 ; StReAm_OuT-- ; XtRaCt -= 32 ; } ;

// save accumulator/count/pointer for insertion into stream32 structure
#define EZSTREAM_SAVE_IN(S32)  (S32).acc_i = AcC_In  ; (S32).insert = InSeRt ; (S32).in  = StReAm_In ;
// save accumulator/count/pointer for extraction into stream32 structure
#define EZSTREAM_SAVE_OUT(S32) EZSTREAM_ADJUST_OUT ; (S32).acc_x = AcC_OuT ; (S32).xtract = XtRaCt ; (S32).out = StReAm_OuT ;

// access to stream32 in/out/limit(size) as indexes into a unit32_t array
#define EZSTREAM_IN(S32)     ((S32).in    - (S32).first)
#define EZSTREAM_OUT(S32)    ((S32).out   - (S32).first)
#define BITSTREAM_SIZE(S32)  ((S32).limit - (S32).first)
#define BITSTREAM_VALID(S32) ((S32).marker == STREAM32_MARKER && (S32).version == STREAM32_VERSION)

// =============== insertion macros ===============

// initialize stream for insertion, accumulator and inserted bits count are set to 0
#define BITS_PUT_INIT(ACCUM, INSERT) { ACCUM = 0 ; INSERT = 0 ; }
#define BITSTREAM_PUT_INIT(S32) BITS_PUT_INIT( (S32).acc_i, (S32).insert )
#define EZSTREAM_PUT_INIT BITS_PUT_INIT(AcC_In, InSeRt)

// insert the lower NBITS bits from W32 into ACCUM, update INSERT, ACCUM
// insert BELOW the topmost INSERT bits in accumulator
// (unsafe as it assumes that NBITS bits can be inserted into acumulator)
#define BITS_PUT_FAST(ACCUM, INSERT, W32, NBITS) \
        { uint64_t t=(W32) ; t <<= (64-(NBITS)) ; t >>= INSERT ; INSERT += (NBITS) ; ACCUM |= t ; }
//                           only keep lower NBITS bits from t ; update count      ; insert
#define BITSTREAM_PUT_FAST(S32, W32, NBITS) BITS_PUT_FAST((S32).acc_i, (S32).insert, W32, NBITS)
#define EZSTREAM_PUT_FAST(W32, NBITS) BITS_PUT_FAST(AcC_In, InSeRt,  W32, NBITS)

// check that 32 bits can be safely inserted into ACCUM
// if not possible, store upper 32 bits of ACCUM into stream, update ACCUM, INSERT, stream
#define BITS_PUT_CHECK(ACCUM, INSERT, STREAM) \
        { *(STREAM) = (uint64_t) ACCUM >> 32 ; if(INSERT > 32) { INSERT -= 32 ; (STREAM)++ ; ACCUM <<= 32 ; } ; }
//       always store upper 32 bits of ACCUM ; if necessary  :   update count ; bump STREAM ; remove stored bits
#define BITSTREAM_PUT_CHECK(S32) BITS_PUT_CHECK((S32).acc_i, (S32).insert, (S32).in)
#define EZSTREAM_PUT_CHECK BITS_PUT_CHECK(AcC_In, InSeRt, StReAm_In)

// combined CHECK and INSERT, update ACCUM, INSERT, STREAM
#define BITS_PUT_SAFE(ACCUM, INSERT, W32, NBITS, STREAM) \
        { BITS_PUT_CHECK(ACCUM, INSERT, STREAM) ; BITS_PUT_FAST(ACCUM, INSERT, W32, NBITS) ; }
#define BITSTREAM_PUT_SAFE(S32, W32, NBITS) BITS_PUT_SAFE((S32).acc_i, (S32).insert, W32, NBITS, (S32).in)
#define EZSTREAM_PUT_SAFE(W32, NBITS) BITS_PUT_SAFE(AcC_In, InSeRt, W32, NBITS, StReAm_In)

// push all accumulator data into stream without fully updating control info (STREAM, INSERT)
#define BITS_PUSH(ACCUM, INSERT, STREAM) \
        { BITS_PUT_CHECK(ACCUM, INSERT, STREAM) ; if(INSERT > 0) { *(STREAM) = (uint64_t) ACCUM >> 32 ; } }
//                                              if there is residual data in accumulator store it
#define BITSTREAM_PUSH(S32) BITS_PUSH((S32).acc_i, (S32).insert, (S32).in)
#define EZSTREAM_PUSH BITS_PUSH(AcC_In, InSeRt, StReAm_In)

// flush any residual data from ACCUM into stream, update ACCUM, INSERT, STREAM
#define BITS_PUT_FLUSH(ACCUM, INSERT, STREAM) \
        { BITS_PUT_CHECK(ACCUM, INSERT, STREAM) ; if(INSERT > 0) { *(STREAM) = (uint64_t) ACCUM >> 32 ; (STREAM)++ ; INSERT = 0 ; ACCUM = 0 ;} }
//                                      if there is residual data in accumulator store it, bump STREAM ptr, mark ACCUM as empty
#define BITSTREAM_PUT_FLUSH(S32) BITS_PUT_FLUSH((S32).acc_i, (S32).insert, (S32).in)
#define EZSTREAM_PUT_FLUSH BITS_PUT_FLUSH(AcC_In, InSeRt, StReAm_In)

// align insertion point to a 32 bit boundary
//  make INSERT a 32 bit multiple              free bits             modulo 31
#define BITS_PUT_ALIGN(ACCUM, INSERT) { uint32_t tbits = 64 - INSERT ; tbits &= 31 ; INSERT += tbits ; }
#define BITSTREAM_PUT_ALIGN(S32) BITS_PUT_ALIGN((S32).acc_i, (S32).insert)
#define EZSTREAM_PUT_ALIGN BITS_PUT_ALIGN(AcC_In, InSeRt)

// =============== extraction macros ===============
// N.B. : if W32 is a signed variable, the extract will produce a "signed" result
//        if W32 is an unsigned variable, the extract will produce an "unsigned" result
//
// initialize stream for extraction, fill 32 bits
//                                                                               fill topmost bits  bump STREAM   32 useful bits
#define BITS_GET_INIT(ACCUM, XTRACT, STREAM) { uint32_t t = *(STREAM) ; ACCUM = t ; ACCUM <<= 32 ; (STREAM)++ ; XTRACT = 32 ; }
#define BITSTREAM_GET_INIT(S32) BITS_GET_INIT( (S32).acc_x, (S32).xtract, (S32).out)
#define EZSTREAM_GET_INIT BITS_GET_INIT(AcC_OuT, XtRaCt, StReAm_OuT)

// take a peek at the next NBITS bits from ACCUM into W32 (unsafe, assumes that NBITS bits are available)
// if W32 is signed, the result will be signed, if W32 is unsigned, the result will be unsigned
#define BITS_PEEK(ACCUM, W32, NBITS) { W32 = (uint64_t) ACCUM >> 32 ; W32 = W32 >> (32 - (NBITS)) ; }
#define BITSTREAM_PEEK(W32, NBITS) BITS_PEEK((S32).acc_x, W32, NBITS)
#define EZSTREAM_PEEK(W32, NBITS) BITS_PEEK(AcC_OuT,  W32, NBITS)

// fast skip of the next NBITS bits (<= 32) from ACCUM (unsafe, assumes that NBITS bits are available)
// if NBITS > XTRACT, it is an unchecked error
// may leave less than 32 available bits in accumulator
#define BITS_SKIP(ACCUM, XTRACT, NBITS) { ACCUM <<= (NBITS) ; XTRACT -= (NBITS) ; }
#define BITSTREAM_SKIP(S32, NBITS) BITS_SKIP((S32).acc_x, (S32).xtract, NBITS)
#define EZSTREAM_SKIP(NBITS) BITS_SKIP(AcC_OuT, XtRaCt, NBITS)

// safely skip NBITS bits in stream (does not assume that NBITS bits are available)
// no limit on NBITS other than stream contents
// may leave less than 32 available bits in accumulator
#define BITS_SKIP_SAFE(ACCUM, XTRACT, NBITS, STREAM) \
        { if((NBITS) <= XTRACT){ \
            ACCUM <<= (NBITS) ; XTRACT -= (NBITS) ; \
          }else { \
            int nb = (NBITS) - XTRACT ; STREAM += (nb / 32) ; nb &= 31 ; \
            ACCUM = *(STREAM) ; ACCUM <<= (32 + nb) ; (STREAM)++ ; XTRACT = 32 - nb ; \
          } ; \
        }
#define BITSTREAM_SKIP_SAFE(S32, NBITS) BITS_SKIP_SAFE((S32).acc_x, (S32).xtract, NBITS, (S32).out)
#define EZSTREAM_SKIP_SAFE(NBITS) BITS_SKIP_SAFE(AcC_OuT, XtRaCt, NBITS, StReAm_OuT)

// extract NBITS bits into W32 from ACCUM, update XTRACT, ACCUM (unsafe, assumes that NBITS bits are available)
#define BITS_GET_FAST(ACCUM, XTRACT, W32, NBITS) \
        { BITS_PEEK(ACCUM, W32, NBITS) ; BITS_SKIP(ACCUM, XTRACT, NBITS) ; }
#define BITSTREAM_GET_FAST(W32, NBITS) BITS_GET_FAST((S32).acc_x, (S32).xtract, W32, NBITS)
#define EZSTREAM_GET_FAST(W32, NBITS) BITS_GET_FAST(AcC_OuT, XtRaCt, W32, NBITS)

// check that 32 bits can be safely extracted from ACCUM
// if it is not true, get an extra 32 bits into ACCUM from stresm, update ACCUM, XTRACT, STREAM
#define BITS_GET_CHECK(ACCUM, XTRACT, STREAM) \
        { if(XTRACT < 32) { ACCUM = (uint64_t) ACCUM >> (32-XTRACT) ; ACCUM |= *(STREAM) ; ACCUM <<= (32-XTRACT) ; XTRACT += 32 ; (STREAM)++ ; } ; }
#define BITSTREAM_GET_CHECK(S32) BITS_GET_CHECK((S32).acc_x, (S32).xtract, (S32).out)
#define EZSTREAM_GET_CHECK BITS_GET_CHECK(AcC_OuT, XtRaCt, StReAm_OuT)

// combined CHECK and EXTRACT, update ACCUM, XTRACT, STREAM
#define BITS_GET_SAFE(ACCUM, XTRACT, W32, NBITS, STREAM) \
        { BITS_GET_CHECK(ACCUM, XTRACT, STREAM) ; BITS_GET_FAST(ACCUM, XTRACT, W32, NBITS) ; }
#define BITSTREAM_GET_SAFE(S32, W32, NBITS) BITS_GET_SAFE((S32).acc_x, (S32).xtract, W32, NBITS, (S32).out)
#define EZSTREAM_GET_SAFE(W32, NBITS) BITS_GET_SAFE(AcC_OuT, XtRaCt, W32, NBITS, StReAm_OuT)

// finalize extraction, push unused full 32 bit words back into stream, update ACCUM, XTRACT, STREAM
#define BITS_GET_FINALIZE(ACCUM, XTRACT, STREAM) if(XTRACT > 32) { ACCUM <<= 32 ; XTRACT -= 32 ; (STREAM)-- ; }
#define BITSTREAM_GET_FINALIZE(S32) BITS_GET_FINALIZE((S32).acc_x, (S32).xtract, (S32).out)
#define EZSTREAM_GET_FINALIZE BITS_GET_FINALIZE(AcC_OuT, XtRaCt, StReAm_OuT)

// align extraction point to a 32 bit boundary
//  make XTRACT a 32 bit multiple                available bits   modulo 32
#define BITS_GET_ALIGN(ACCUM, XTRACT) { uint32_t tbits = XTRACT ; tbits &= 31 ; ACCUM <<= tbits ; XTRACT -= tbits ; }
#define BITSTREAM_GET_ALIGN(S32) BITS_GET_ALIGN((S32).acc_x, (S32).xtract)
#define EZSTREAM_GET_ALIGN BITS_GET_ALIGN(AcC_OuT, XtRaCt)

// word (32 bit) stream descriptor
// number of words in buffer : in - out (available for extraction)
// space available in buffer : limit - in (available for insertion)
// if alloc == 1, a call to realloc is O.K., limit, in and out must be adjusted
// the 2 most popular trios [acc_i, in, insert] and [xtract, out, acc_x] are adjacent in memory
// the fields from the following struct should only be accessed through the appropriate macros
typedef struct{
  uint32_t version: 8,  // version    (to be kept in the first 32 bits of the struct)
           spare:  22,  // spare bits
           ualloc:  1,  // user allocated buffer, may be free(d)/realloc(ed)
           alloc:   1;  // whole struct allocated with malloc (realloc is possible)
  uint32_t marker ;     // signature marker
  uint32_t *first ;     // pointer to start of stream data storage
  uint32_t *limit ;     // buffer size (in 32 bit units)
  uint64_t acc_i ;      // accumulator for insertion
  uint32_t *in ;        // insertion index (0 initially)
  uint32_t insert ;     // number of inserted bits in acc_i
  uint32_t xtract ;     // umber of available bits in acc_x
  uint32_t *out ;       // extraction index (0 initially)
  uint64_t acc_x ;      // accumulator for extraction
//   uint32_t data[] ;    // dynamic array, only used if alloc is true
} stream32 ;
CT_ASSERT(sizeof(stream32) == 64, "wordstream size MUST be 56 bytes")    // 8 x 64 bits (6 x 64 bits + 4 x 32 bits)

// old style behavior interface (less safe, no indication of pak dimansion)
uint32_t pack_w32(void *unp, void *pak, int nbits, int n) ;
uint32_t unpack_u32(void *unp, void *pak, int nbits, int n) ;
uint32_t unpack_i32(void *unp, void *pak, int nbits, int n) ;

// preferred stream32 based interface
stream32 *stream32_create(stream32 *s_old, void *buf, uint32_t size) ;
stream32 *stream32_resize(stream32 *s_old, uint32_t size) ;
void *stream32_rewind(stream32 *s) ;
void *stream32_rewrite(stream32 *s) ;
int stream32_free(stream32 *s) ;

// stream32 pack/unpack options
#define GET_NO_INIT     0x00000001
#define GET_NO_FINALIZE 0x00000002
#define PUT_NO_INIT     0x00000004
#define PUT_NO_FLUSH    0x00000008

uint32_t stream32_pack(stream32 *s, void *unp, int nbits, int n, uint32_t options) ;
uint32_t stream32_unpack_u32(stream32 *s,void *unp,  int nbits, int n, uint32_t options) ;
uint32_t stream32_unpack_i32(stream32 *s,void *unp,  int nbits, int n, uint32_t options) ;

uint32_t stream32_vpack(stream32 *s, void *unp, int *nbits, int n, uint32_t options) ;
uint32_t stream32_vunpack_u32(stream32 *s,void *unp,  int *nbits, int n, uint32_t options) ;
uint32_t stream32_vunpack_u32(stream32 *s,void *unp,  int *nbits, int n, uint32_t options) ;

#endif
