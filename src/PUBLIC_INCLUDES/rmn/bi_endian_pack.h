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
// set of macros and functions to manage insertion/extraction into/from a bit stream
//
// N.B. the accumulator MUST be a 64 bit variable
// initialize stream for insertion
//   [L|B]E64_INSERT_BEGIN(accumulator, counter)
//   [L|B]E64_STREAM_INSERT_BEGIN(stream)
// insert the lower nbits bits from uint32 into accumulator, update counter, accumulator
//   [L|B]E64_INSERT_NBITS(accumulator, counter, uint32, nbits)
//   [L|B]E64_STREAM_INSERT_NBITS(stream, uint32, nbits)
// check that 32 bits can be safely inserted into accumulator
// if not possible, store lower 32 bits of accumulator into stream, update accumulator, counter, stream
//   [L|B]E64_INSERT_CHECK(accumulator, counter, stream)
//   [L|B]E64_STREAM_INSERT_CHECK(stream)
// push data to stream without fully updating (accumulator, stream, counter)
//   [L|B]E64_PUSH(accumulator, counter, stream)
// store any residual data from accumulator into stream, update accumulator, counter, stream
//   [L|B]E64_INSERT_FINAL(accumulator, counter, stream)
//   [L|B]E64_STREAM_INSERT_FINAL(stream)
// combined INSERT_CHECK and INSERT_NBITS, update accumulator, counter, stream
//   [L|B]E64_PUT_NBITS(accumulator, counter, uint32, nbits, stream)
//   [L|B]E64_STREAM_PUT_NBITS(stream, uint32, nbits)
//
// N.B. : if uint32 and accumulator are signed variables, the extract will produce a "signed" result
//        if uint32 and accumulator are unsigned variables, the extract will produce an "unsigned" result
// initialize stream for extraction
//   [L|B]E64_XTRACT_BEGIN(accumulator, xtract, stream)
//   [L|B]E64_STREAM_XTRACT_BEGIN(stream)
// take a peek at nbits bits from accumulator into uint32
//   [L|B]E64_PEEK_NBITS(accumulator, xtract, uint32, nbits)
//   [L|B]E64_STREAM_PEEK_NBITS(stream, uint32, nbits)
//   [L|B]E64_STREAM_PEEK_NBITS_S(stream, uint32, nbits)
// extract nbits bits into uint32 from accumulator, update xtract, accumulator
//   [L|B]E64_XTRACT_NBITS(accumulator, xtract, uint32, nbits)
//   [L|B]E64_STREAM_XTRACT_NBITS(stream, uint32, nbits)
// check that 32 bits can be safely extracted from accumulator
// if not possible, get extra 32 bits into accumulator from stream, update accumulator, xtract, stream
//   [L|B]E64_XTRACT_CHECK(accumulator, xtract, stream)
//   [L|B]E64_STREAM_XTRACT_CHECK(stream)
// finalize extraction, update accumulator, xtract
//   [L|B]E64_XTRACT_FINAL(accumulator, xtract)
//   [L|B]E64_STREAM_XTRACT_FINAL(stream)
// combined XTRACT_CHECK and XTRACT_NBITS, update accumulator, xtract, stream
//   [L|B]E64_GET_NBITS(accumulator, xtract, uint32, nbits, stream)
//   [L|B]E64_STREAM_GET_NBITS(stream, uint32, nbits)
//

// MAKE_SIGNED_32 is defined in this file and acts as double include detection
#if ! defined(MAKE_SIGNED_32)

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

// compile time assertions
#include <rmn/ct_assert.h>

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

// word (32 bit) stream descriptor
// number of words in buffer : in - out (available for extraction)
// space available in buffer : limit - in (available for insertion)
// if alloc == 1, a call to realloc is O.K., limit must be adjusted
// initial state : in = out = 0
typedef struct{
  uint32_t *buf ;     // pointer to start of stream data storage
  uint32_t limit ;    // buffer size
  uint32_t in ;       // insertion position
  uint32_t out ;      // extraction position
  uint32_t full:  1 , // the whole struct was allocated with malloc (realloc is possible)
           alloc: 1 , // buffer allocated with malloc (realloc is possible)
           user:  1 , // buffer was user supplied (realloc is NOT possible)
           spare: 5 , // spare bits
           valid:24 ; // signature marker
} wordstream ;
CT_ASSERT(sizeof(wordstream) == 24) ;   // 3 64 bit elements

static wordstream null_wordstream = { .buf = NULL, .limit = 0, .in = 0, .out = 0,
                                      .full = 0, .alloc = 0, .user = 0, .spare = 0, .valid = 0 } ;

// bit stream descriptor. both insert / extract may be positive
// in insertion only mode, xtract would be -1
// in extraction only mode, insert would be -1
// for now, a bit stream is unidirectional (either insert or extract mode)
typedef struct{
  uint64_t  acc_i ;   // 64 bit unsigned bit accumulator for insertion
  uint64_t  acc_x ;   // 64 bit unsigned bit accumulator for extraction
  int32_t   insert ;  // # of bits used in accumulator (0 <= insert <= 64)
  int32_t   xtract ;  // # of bits extractable from accumulator (0 <= xtract <= 64)
  uint32_t *first ;   // pointer to start of stream data storage
  uint32_t *in ;      // pointer into packed stream (insert mode)
  uint32_t *out ;     // pointer into packed stream (extract mode)
  uint32_t *limit ;   // pointer to end of stream data storage (1 byte beyond stream buffer end)
  uint64_t full:  1 , // the whole struct was allocated with malloc
           alloc: 1 , // buffer allocated with malloc
           user:  1 , // buffer was user supplied
           endian:2 , // 01 : Big Endian stream, 10 : Little Endian stream, 00/11 : invalid
           spare:27 , // spare bits
           valid:32 ; // signature marker
//   uint32_t buf[] ;    // flexible array (meaningful only if full == 1)
} bitstream ;
CT_ASSERT(sizeof(bitstream) == 64) ;   // 8 64 bit elements

// all fields set to 0, makes for a fast initialization xxx = null_bitstream
static bitstream null_bitstream = { .acc_i = 0, .acc_x = 0 , .insert = 0 , .xtract = 0, 
                                    .first = NULL, .in = NULL, .out = NULL, .limit = NULL,
                                    .full = 0, .alloc = 0, .user = 0, .endian = 0, .spare = 0, .valid = 0 } ;

// convert the nbits rightmost bits to a 2s complement signed number
// for this macro to produce meaningful results, w32 MUST BE int32_t (32 bit signed int)
#define MAKE_SIGNED_32(w32, nbits) { w32 <<= (32 - (nbits)) ; w32 >>= (32 - (nbits)) ; }
// for this macro to produce meaningful results, w64 MUST BE int64_t (64 bit signed int)
#define MAKE_SIGNED_64(w64, nbits) { w64 <<= (64 - (nbits)) ; w64 >>= (64 - (nbits)) ; }

// stream insert/extract mode (0 or 3 would mean both insert and extract)
// extract mode
#define BIT_XTRACT        1
// insert mode
#define BIT_INSERT        2
// full initialization mode
#define BIT_FULL_INIT     8

// endianness
#define STREAM_BE 1
#define STREAM_LE 2
#define STREAM_ENDIANNESS(s) (s).endian
#define STREAM_IS_BIG_ENDIAN(s) ( (s).endian == STREAM_BE )
#define STREAM_IS_LITTLE_ENDIAN(s) ( (s).endian == STREAM_LE )

// =========================== utility functions and macros ======================
// is stream valid ?
// s [IN] : pointer to a bit stream struct
static int StreamIsValid(bitstream *s){
  if(s->valid != 0xCAFEFADEu)                  return 0 ;    // incorrect marker
  if(s->first == NULL)                         return 0 ;    // no buffer
  if(s->limit == NULL)                         return 0 ;    // invalid limit
  if(s->limit <= s->first)                     return 0 ;    // invalid limit
  if(s->in < s->first  || s->in > s->limit)    return 0 ;    // in outside limits
  if(s->out < s->first || s->out > s->limit)   return 0 ;    // out outside limits
  if(s->endian == 0 || s->endian == 3 )        return 0 ;    // invalid endianness
  return 1 ;                                                 // valid stream
}

// get stream endianness, return 0 if invalid endianness
// s [IN] : pointer to a bit stream struct
static int StreamEndianness(bitstream *stream){
  int endian = STREAM_ENDIANNESS( (*stream) ) ;
  if((endian == 0) || (endian == (STREAM_BE | STREAM_LE)) ) return 0 ;
  return endian ;
}

// size of stream buffer (in bytes)
#define STREAM_BUFFER_BYTES(s) ( ((s).limit - (s).first) * sizeof(uint32_t) )

// address of stream buffer
#define STREAM_BUFFER_ADDRESS(s) (s).first

// bits used in accumulator (trustable if stream in insertion mode)
#define STREAM_ACCUM_BITS_USED(s) ((s).insert)
// bits used in stream (trustable if stream in insertion mode)
#define STREAM_BITS_USED(s) ((s).insert + ((s).in - (s).out) * 8l * sizeof(uint32_t))
// bits left to fill in stream (trustable if stream in insertion mode)
#define STREAM_BITS_EMPTY(s) ( ((s).limit - (s).in) * 8l * sizeof(uint32_t) - (s).insert)

// bits available in accumulator (trustable if stream in extract mode)
#define STREAM_ACCUM_BITS_AVAIL(s) ((s).xtract)
// bits available in stream (trustable if stream in extract mode)
#define STREAM_BITS_AVAIL(s) ((s).xtract + ((s).in - (s).out) * 8l * sizeof(uint32_t) )

// number of bits available for extraction
STATIC inline size_t StreamAvailableBits(bitstream *p){
//   if(p->xtract < 0) return -1 ;             // extraction not allowed
  int32_t in_xtract = (p->xtract < 0) ? 0 : p->xtract ;
  return (p->in - p->out)*32 + ((p->insert < 0) ? 0 :p->insert ) + in_xtract ;  // stream + accumulators contents
}
STATIC inline size_t StreamStrictAvailableBits(bitstream *p){
//   if(p->xtract < 0) return -1 ;             // extraction not allowed
  int32_t in_xtract = (p->xtract < 0) ? 0 : p->xtract ;
  return (p->in - p->out)*32 + in_xtract ;              // stream + extract accumulator contents
}

// number of bits available for insertion
STATIC inline size_t StreamAvailableSpace(bitstream *p){
  if(p->insert < 0) return -1 ;   // insertion not allowd
  return (p->limit - p->in)*32 - p->insert ;
}
// true if stream is in read (extract) mode
// possibly false for a NEWLY INITIALIZED stream
#define STREAM_XTRACT_MODE(s) ((s).xtract >= 0)
// true if stream is in write (insert) mode
// possibly false for a NEWLY INITIALIZED (empty) stream
#define STREAM_INSERT_MODE(s) ((s).insert >= 0)

// get stream mode as a string
STATIC inline char *StreamMode(bitstream p){
  if( STREAM_INSERT_MODE(p) && STREAM_XTRACT_MODE(p)) return("RW") ;
  if( STREAM_XTRACT_MODE(p) ) return "R" ;
  if( STREAM_INSERT_MODE(p) ) return "W" ;
  return("Unknown") ;
}
// get stream mode as a code
STATIC inline int StreamModeCode(bitstream p){
  uint32_t mode = 0 ;
//   if(p.insert >= 0) mode |= BIT_INSERT ;
//   if(p.xtract >= 0) mode |= BIT_XTRACT ;
//   if( p.insert >= 0 && p.xtract >= 0) return 0 ;       // both insert and extract operations allowed
  if( STREAM_XTRACT_MODE(p) ) mode |= BIT_XTRACT ;
  if( STREAM_INSERT_MODE(p) ) mode |= BIT_INSERT ;
  return mode ? mode : -1 ;                               // return -1 if neither extract nor insert is set
}
//

// =======================  stream state save/restore  =======================
//
// bit stream state for save/restore operations
typedef struct{
  uint64_t  acc_i ;   // 64 bit unsigned bit accumulator for insertion
  uint64_t  acc_x ;   // 64 bit unsigned bit accumulator for extraction
  uint32_t *first ;   // pointer to start of stream data storage (used for consistency check)
  int32_t   in ;      // insertion offset (-1 if invalid) (bitstream.in - first)
  int32_t   out ;     // extraction offset (-1 if invalid) (bitstream.out - first)
  int32_t   insert ;  // # of bits used in accumulator (-1 <= insert <= 64) (-1 if invalid)
  int32_t   xtract ;  // # of bits extractable from accumulator (-1 <= xtract <= 64) (-1 if invalid)
} bitstream_state ;
CT_ASSERT(sizeof(bitstream_state) == 40) ;
//
// save the current bit stream state in a bitstream_state structure
// stream  [IN] : pointer to a bit stream struct
// state  [OUT] : pointer to a bit stream state struct
static int StreamSaveState(bitstream *stream, bitstream_state *state){
  if(! StreamIsValid(stream)) goto error ;
  state->acc_i  = stream->acc_i ;
  state->acc_x  = stream->acc_x ;
  state->first  = stream->first ;
  state->in     = stream->in - stream->first ;
  state->out    = stream->out - stream->first ;
  state->insert = stream->insert ;
  state->xtract = stream->xtract ;
  return 0 ;

error:
  state->in     = -1 ;     // make sure returned state is invalid
  state->out    = -1 ;
  state->insert = -1 ;
  state->xtract = -1 ;
  state->first  = NULL ;
fprintf(stderr, "error in save state\n");
  return 1 ;
}

// restore a bit stream state from the state saved with StreamSaveState
// stream [OUT] : pointer to a bit stream struct
// state   [IN] : pointer to a bit stream state struct
// mode    [IN} : 0, BIT_XTRACT, BIT_INSERT (which mode state(s) to restore)
//                0 restore BOTH insert and extract states
static int StreamRestoreState(bitstream *stream, bitstream_state *state, int mode){
  char *msg ;
  msg = "invalid stream" ;
  if(! StreamIsValid(stream)) goto error ;                            // invalid stream
  if(mode == 0) mode = BIT_XTRACT | BIT_INSERT ;
  msg = "first mismatch" ;
  if(state->first != stream->first) goto error ;                      // state does not belong to this stream
  if(mode & BIT_XTRACT){                                              // restore extract state (out)
    msg = "out too large" ;
    if(state->out > (stream->limit - stream->first) ) goto error ;    // potential buffer overrrun
    msg = "stream not in extract mode" ;
    if(stream->xtract < 0) goto error ;                               // stream not in extract mode
    msg = "no extract state" ;
    if(state->xtract < 0) goto error ;                                // extract state not valid
    stream->out = stream->first + state->out ;                        // restore extraction pointer
    stream->acc_x  = state->acc_x ;                                   // restore accumulator and bit count
    stream->xtract = state->xtract ;
// fprintf(stderr, "restored extract state\n");
  }
  if(mode & BIT_INSERT){                                              // restore insert state (in)
  msg = "in too large" ;
    if(state->in > (stream->limit - stream->first) ) goto error ;     // potential buffer overrrun
    msg = "stream not in insert mode" ;
    if(stream->insert < 0) goto error ;                               // stream not in insert mode
    msg = "no insert state" ;
    if(state->insert < 0) goto error ;                                // insert state not valid
    stream->in = stream->first + state->in ;                          // restore insertion pointer
    stream->acc_i  = state->acc_i ;                                   // restore accumulator and bit count
    stream->insert = state->insert ;
// fprintf(stderr, "restored insert state\n");
  }
  return 0 ;                                                          // no error
error:
// fprintf(stderr, "error (%s) in restore state\n", msg);
  return 1 ;
}

// ========================== EZ macros preamble/postamble ===========================
// transfer stream field to/from local variable
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
// ================================ insertion/extraction macros ===============================
// macro arguments description
// accum  [INOUT] : 64 bit accumulator (normally acc_i or acc_x)
// insert [INOUT] : # of bits used in accumulator (0 <= insert <= 64)
// xtract [INOUT] : # of bits extractable from accumulator (0 <= xtract <= 64)
// stream [INOUT] : pointer to next position in bit stream (in or out component of bitstream struct)
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
#define LE64_INSERT_BEGIN(accum, insert) { accum = 0 ; insert = 0 ; }
#define LE64_EZ_INSERT_BEGIN        { LE64_INSERT_BEGIN(StReAm_acc_i, StReAm_insert) }
#define LE64_STREAM_INSERT_BEGIN(s) { LE64_INSERT_BEGIN((s).acc_i,    (s).insert) }
// insert the lower nbits bits from w32 into accum, update insert, accum
#define LE64_INSERT_NBITS(accum, insert, w32, nbits) \
        { uint32_t mask = ~0 ; mask >>= (32-(nbits)) ; uint64_t w64 = (w32) & mask ; accum |= (w64 << insert) ; insert += (nbits) ; }
#define LE64_EZ_INSERT_NBITS(w32, nbits)        LE64_INSERT_NBITS(StReAm_acc_i, StReAm_insert, w32, nbits)
#define LE64_STREAM_INSERT_NBITS(s, w32, nbits) LE64_INSERT_NBITS((s).acc_i,    (s).insert,    w32, nbits)
// check that 32 bits can be safely inserted into accum
// if not possible, store lower 32 bits of accum into stream, update accum, insert, stream
#define LE64_INSERT_CHECK(accum, insert, stream) \
        { if(insert > 32) { *stream = accum ; stream++ ; insert -= 32 ; accum >>= 32 ; } ; }
#define LE64_EZ_INSERT_CHECK        LE64_INSERT_CHECK(StReAm_acc_i, StReAm_insert, StReAm_in)
#define LE64_STREAM_INSERT_CHECK(s) LE64_INSERT_CHECK((s).acc_i,    (s).insert,    (s).in)
// push data to stream without fully updating control info (stream, insert)
#define LE64_PUSH(accum, insert, stream) \
        { LE64_INSERT_CHECK(accum, insert, stream) ; { if(insert > 0) { *stream = accum ; } ; } }
#define LE64_EZ_PUSH     LE64_PUSH(StReAm_acc_i, StReAm_insert, StReAm_in)
#define LE64_STREAM_PUSH LE64_PUSH((s).acc_i,    (s).insert,    (s).in)
// store any residual data from accum into stream, update accum, insert, stream
#define LE64_INSERT_FINAL(accum, insert, stream) \
        { LE64_INSERT_CHECK(accum, insert, stream) ; { if(insert > 0) { *stream = accum ; stream++ ; insert = 0 ; } ; } }
#define LE64_EZ_INSERT_FINAL        LE64_INSERT_FINAL(StReAm_acc_i, StReAm_insert, StReAm_in)
#define LE64_STREAM_INSERT_FINAL(s) LE64_INSERT_FINAL((s).acc_i,    (s).insert,    (s).in)
// combined INSERT_CHECK and INSERT_NBITS, update accum, insert, stream
#define LE64_PUT_NBITS(accum, insert, w32, nbits, stream) \
        { LE64_INSERT_CHECK(accum, insert, stream) ; LE64_INSERT_NBITS(accum, insert, w32, nbits) ; }
#define LE64_EZ_PUT_NBITS(w32, nbits)        LE64_PUT_NBITS(StReAm_acc_i, StReAm_insert, w32, nbits, StReAm_in)
#define LE64_STREAM_PUT_NBITS(s, w32, nbits) LE64_PUT_NBITS((s).acc_i,    (s).insert,    w32, nbits, (s).in)
// align insertion point to a 32 bit boundary
#define LE64_INSERT_ALIGN(insert) { uint32_t tbits = 64 - insert ;  tbits &= 31 ; insert += tbits ; }
#define LE64_EZ_INSERT_ALIGN        LE64_INSERT_ALIGN(StReAm_insert)
#define LE64_STREAM_INSERT_ALIGN(s) LE64_INSERT_ALIGN((s).insert)
// #define LE64_EZ_INSERT_ALIGN        { uint32_t tbits = 64 - StReAm_insert ; tbits &= 31 ; StReAm_insert += tbits ; }
// #define LE64_STREAM_INSERT_ALIGN(s) { uint32_t tbits = 64 - StReAm_insert ; tbits &= 31 ; (s).insert += tbits ; }
//
// N.B. : if w32 and accum are signed variables, the extract will produce a "signed" result
//        if w32 and accum are unsigned variables, the extract will produce an "unsigned" result
// initialize stream for extraction
#define LE64_XTRACT_BEGIN(accum, xtract, stream) { accum = *(stream) ; (stream)++ ; xtract = 32 ; }
#define LE64_EZ_XTRACT_BEGIN { LE64_XTRACT_BEGIN(StReAm_acc_x, StReAm_xtract, StReAm_out) ; }
#define LE64_STREAM_XTRACT_BEGIN(s) { LE64_XTRACT_BEGIN((s).acc_x, (s).xtract, (s).out) ; }
// take a peek at nbits bits from accum into w32
#define LE64_PEEK_NBITS(accum, xtract, w32, nbits) { w32 = (accum << (64-nbits)) >> (64-nbits) ; }
#define LE64_EZ_PEEK_NBITS(w32, nbits)          LE64_PEEK_NBITS(StReAm_acc_x, StReAm_xtract, w32, nbits)
#define LE64_STREAM_PEEK_NBITS(s, w32, nbits)   LE64_PEEK_NBITS((s).acc_x,    (s).xtract,    w32, nbits)
#define LE64_EZ_PEEK_NBITS_S(w32, nbits)        LE64_PEEK_NBITS((int64_t) StReAm_acc_x, StReAm_xtract, w32, nbits)
#define LE64_STREAM_PEEK_NBITS_S(s, w32, nbits) LE64_PEEK_NBITS((int64_t) (s).acc_x,    (s).xtract,    w32, nbits)
// extract nbits bits into w32 from accum, update xtract, accum
#define LE64_XTRACT_NBITS(accum, xtract, w32, nbits) \
        { w32 = (accum << (64-nbits)) >> (64-nbits) ; accum >>= nbits ; xtract -= (nbits) ;}
#define LE64_EZ_XTRACT_NBITS(w32, nbits)        LE64_XTRACT_NBITS(StReAm_acc_x, StReAm_xtract, w32, nbits)
#define LE64_STREAM_XTRACT_NBITS(s, w32, nbits) LE64_XTRACT_NBITS((s).acc_x,    (s).xtract,    w32, nbits)
// check that 32 bits can be safely extracted from accum
// if not possible, get extra 32 bits into accum from stresm, update accum, xtract, stream
#define LE64_XTRACT_CHECK(accum, xtract, stream) \
        { if(xtract < 32) { uint64_t w64 = *(stream) ; accum |= (w64 << xtract) ; (stream)++ ; xtract += 32 ; } ; }
#define LE64_EZ_XTRACT_CHECK        LE64_XTRACT_CHECK(StReAm_acc_x, StReAm_xtract, StReAm_out)
#define LE64_STREAM_XTRACT_CHECK(s) LE64_XTRACT_CHECK((s).acc_x,    (s).xtract,    (s).out)
// finalize extraction, update accum, xtract
#define LE64_XTRACT_FINAL(accum, xtract) \
        { accum = 0 ; xtract = 0 ; }
#define LE64_EZ_XTRACT_FINAL        LE64_XTRACT_FINAL(StReAm_acc_x, StReAm_xtract)
#define LE64_STREAM_XTRACT_FINAL(s) LE64_XTRACT_FINAL((s).acc_x,    (s).xtract)
// combined XTRACT_CHECK and XTRACT_NBITS, update accum, xtract, stream
#define LE64_GET_NBITS(accum, xtract, w32, nbits, stream) \
        { LE64_XTRACT_CHECK(accum, xtract, stream) ; LE64_XTRACT_NBITS(accum, xtract, w32, nbits) ; }
#define LE64_EZ_GET_NBITS(w32, nbits)        LE64_GET_NBITS(StReAm_acc_x, StReAm_xtract, w32, nbits, StReAm_out)
#define LE64_STREAM_GET_NBITS(s, w32, nbits) LE64_GET_NBITS((s).acc_x,    (s).xtract,    w32, nbits, (s).out)
// align extraction point to a 32 bit boundary
#define LE64_XTRACT_ALIGN(accum, xtract) { uint32_t tbits = xtract ; tbits &= 31 ; accum >>= tbits ; xtract -= tbits ; }
#define LE64_EZ_XTRACT_ALIGN     LE64_XTRACT_ALIGN(StReAm_acc_x, StReAm_xtract)
#define LE64_STREAM_XTRACT_ALIGN LE64_XTRACT_ALIGN((s).acc_x,    (s).xtract)
// #define LE64_EZ_XTRACT_ALIGN { uint32_t tbits = StReAm_xtract ; tbits &= 31 ; StReAm_acc_x >>= tbits ; StReAm_xtract -= tbits ; }
// #define LE64_STREAM_XTRACT_ALIGN { uint32_t tbits = (s).xtract ; tbits &= 31 ; (s).acc_x >>= tbits ; (s).xtract -= tbits ; }
//
// ===============================================================================================
// big endian (BE) style (left to right) bit stream packing
// insertion at bottom (least significant part) of accumulator after accumulator shifted left
// extraction from the top (most significant part) of accumulator then shift accumulator left
// ===============================================================================================
// initialize stream for insertion
#define BE64_INSERT_BEGIN(accum, insert) { accum = 0 ; insert = 0 ; }
#define BE64_EZ_INSERT_BEGIN        { BE64_INSERT_BEGIN(StReAm_acc_i, StReAm_insert) ; }
#define BE64_STREAM_INSERT_BEGIN(s) { BE64_INSERT_BEGIN((s).acc_i,    (s).insert) ; }
// insert the lower nbits bits from w32 into accum, update insert, accum
#define BE64_INSERT_NBITS(accum, insert, w32, nbits) \
        {  uint32_t mask = ~0 ; mask >>= (32-(nbits)) ; accum <<= (nbits) ; insert += (nbits) ; accum |= ((w32) & mask) ; }
#define BE64_EZ_INSERT_NBITS(w32, nbits)        BE64_INSERT_NBITS(StReAm_acc_i, StReAm_insert, w32, nbits)
#define BE64_STREAM_INSERT_NBITS(s, w32, nbits) BE64_INSERT_NBITS((s).acc_i,    (s).insert,    w32, nbits)
// check that 32 bits can be safely inserted into accum
// if not possible, store lower 32 bits of accum into stream, update accum, insert, stream
#define BE64_INSERT_CHECK(accum, insert, stream) \
        { if(insert > 32) { insert -= 32 ; *(stream) = accum >> insert ; (stream)++ ; } ; }
#define BE64_EZ_INSERT_CHECK        BE64_INSERT_CHECK(StReAm_acc_i, StReAm_insert, StReAm_in)
#define BE64_STREAM_INSERT_CHECK(s) BE64_INSERT_CHECK((s).acc_i,    (s).insert,    (s).in)
// push data to stream without fully updating control info (stream, insert)
#define BE64_PUSH(accum, insert, stream) \
        { BE64_INSERT_CHECK(accum, insert, stream) ; if(insert > 0) { *stream = accum << (32 - insert) ; } }
#define BE64_EZ_PUSH        BE64_PUSH(StReAm_acc_i, StReAm_insert, StReAm_in)
#define BE64_STREAM_PUSH(s) BE64_PUSH((s).acc_i,    (s).insert,    (s).in)
// store any residual data from accum into stream, update accum, insert, stream
#define BE64_INSERT_FINAL(accum, insert, stream) \
        { BE64_INSERT_CHECK(accum, insert, stream) ; if(insert > 0) { *stream = accum << (32 - insert) ; stream++ ; insert = 0 ; } }
#define BE64_EZ_INSERT_FINAL        BE64_INSERT_FINAL(StReAm_acc_i, StReAm_insert, StReAm_in)
#define BE64_STREAM_INSERT_FINAL(s) BE64_INSERT_FINAL((s).acc_i,    (s).insert,    (s).in)
// combined INSERT_CHECK and INSERT_NBITS, update accum, insert, stream
#define BE64_PUT_NBITS(accum, insert, w32, nbits, stream) \
        { BE64_INSERT_CHECK(accum, insert, stream) ; BE64_INSERT_NBITS(accum, insert, w32, nbits) ; }
#define BE64_EZ_PUT_NBITS(w32, nbits)        BE64_PUT_NBITS(StReAm_acc_i, StReAm_insert, w32, nbits, StReAm_in)
#define BE64_STREAM_PUT_NBITS(s, w32, nbits) BE64_PUT_NBITS((s).acc_i,    (s).insert,    w32, nbits, (s).in)
// align insertion point to a 32 bit boundary
#define BE64_INSERT_ALIGN(accum, insert) { uint32_t tbits = 64 - insert ; tbits &= 31 ; insert += tbits ; accum <<= tbits ; }
#define BE64_EZ_INSERT_ALIGN        BE64_INSERT_ALIGN(StReAm_acc_i, StReAm_insert)
#define BE64_STREAM_INSERT_ALIGN(s) BE64_INSERT_ALIGN((s).acc_i,    (s).insert)
// #define BE64_EZ_INSERT_ALIGN        { uint32_t tbits = 64 - StReAm_insert ; tbits &= 31 ; StReAm_insert += tbits ; StReAm_acc_i <<= tbits ; }
// #define BE64_STREAM_INSERT_ALIGN(s) { uint32_t tbits = 64 - StReAm_insert ; tbits &= 31 ; (s).insert += tbits ; (s).acc_i <<= tbits ; }
//
// N.B. : if w32 and accum are signed variables, the extract will produce a "signed" result
//        if w32 and accum are unsigned variables, the extract will produce an "unsigned" result
// initialize stream for extraction
#define BE64_XTRACT_BEGIN(accum, xtract, stream) { uint32_t t = *(stream) ; accum = t ; accum <<= 32 ; (stream)++ ; xtract = 32 ; }
#define BE64_EZ_XTRACT_BEGIN        { BE64_XTRACT_BEGIN(StReAm_acc_x, StReAm_xtract, StReAm_out) ; }
#define BE64_STREAM_XTRACT_BEGIN(s) { BE64_XTRACT_BEGIN((s).acc_x,    (s).xtract,    (s).out) ; }
// take a peek at nbits bits from accum into w32
#define BE64_PEEK_NBITS(accum, xtract, w32, nbits) { w32 = accum >> (64 - (nbits)) ; }
#define BE64_EZ_PEEK_NBITS(w32, nbits)          BE64_PEEK_NBITS(StReAm_acc_x, StReAm_xtract, w32, nbits)
#define BE64_STREAM_PEEK_NBITS(s, w32, nbits)   BE64_PEEK_NBITS((s).acc_x,    (s).xtract,    w32, nbits)
#define BE64_EZ_PEEK_NBITS_S(w32, nbits)        BE64_PEEK_NBITS((int64_t) StReAm_acc_x, StReAm_xtract, w32, nbits)
#define BE64_STREAM_PEEK_NBITS_S(s, w32, nbits) BE64_PEEK_NBITS((int64_t) (s).acc_x,    (s).xtract,    w32, nbits)
// extract nbits bits into w32 from accum, update xtract, accum
#define BE64_XTRACT_NBITS(accum, xtract, w32, nbits) \
        { w32 = accum >> (64 - (nbits)) ; accum <<= (nbits) ; xtract -= (nbits) ; }
#define BE64_EZ_XTRACT_NBITS(w32, nbits)        BE64_XTRACT_NBITS(StReAm_acc_x, StReAm_xtract, w32, nbits)
#define BE64_STREAM_XTRACT_NBITS(s, w32, nbits) BE64_XTRACT_NBITS((s).acc_x,    (s).xtract,    w32, nbits)
// check that 32 bits can be safely extracted from accum
// if not possible, get extra 32 bits into accum from stresm, update accum, xtract, stream
#define BE64_XTRACT_CHECK(accum, xtract, stream) \
        { if(xtract < 32) { accum >>= (32-xtract) ; accum |= *(stream) ; accum <<= (32-xtract) ; xtract += 32 ; (stream)++ ; } ; }
#define BE64_EZ_XTRACT_CHECK        BE64_XTRACT_CHECK(StReAm_acc_x, StReAm_xtract, StReAm_out)
#define BE64_STREAM_XTRACT_CHECK(s) BE64_XTRACT_CHECK((s).acc_x,    (s).xtract,    (s).out)
// finalize extraction, update accum, xtract
#define BE64_XTRACT_FINAL(accum, xtract) { accum = 0 ; xtract = 0 ; }
#define BE64_EZ_XTRACT_FINAL(s)     BE64_XTRACT_FINAL(StReAm_acc_x, StReAm_xtract)
#define BE64_STREAM_XTRACT_FINAL(s) BE64_XTRACT_FINAL((s).acc_x,    (s).xtract)
// combined XTRACT_CHECK and XTRACT_NBITS, update accum, xtract, stream
#define BE64_GET_NBITS(accum, xtract, w32, nbits, stream) \
        { BE64_XTRACT_CHECK(accum, xtract, stream) ; BE64_XTRACT_NBITS(accum, xtract, w32, nbits) ; }
#define BE64_EZ_GET_NBITS(w32, nbits)        BE64_GET_NBITS(StReAm_acc_x, StReAm_xtract, w32, nbits, StReAm_out)
#define BE64_STREAM_GET_NBITS(s, w32, nbits) BE64_GET_NBITS((s).acc_x,    (s).xtract,    w32, nbits, (s).out)
// align extraction point to a 32 bit boundary
#define BE64_XTRACT_ALIGN(accum, xtract) { uint32_t tbits = xtract ; tbits &= 31 ; accum <<= tbits ; xtract -= tbits ; }
#define BE64_EZ_XTRACT_ALIGN     BE64_XTRACT_ALIGN(StReAm_acc_x, StReAm_xtract)
#define BE64_STREAM_XTRACT_ALIGN BE64_XTRACT_ALIGN((s).acc_x,    (s).xtract)
// #define BE64_EZ_XTRACT_ALIGN     { uint32_t tbits = StReAm_xtract ; tbits &= 31 ; StReAm_acc_x <<= tbits ; StReAm_xtract -= tbits ; }
// #define BE64_STREAM_XTRACT_ALIGN { uint32_t tbits = (s).xtract ; tbits &= 31 ; (s).acc_x <<= tbits ; (s).xtract -= tbits ; }
//
#if 0
// these macros have been moved to bi_endian_pack.c
// ===============================================================================================
// declare state variables (unsigned)
#define STREAM_DCL_STATE_VARS(acc, ind, ptr) uint64_t acc ; int32_t ind  ; uint32_t *ptr
// declare state variables (signed)
#define STREAM_DCL_STATE_VARS_S(acc, ind, ptr) int64_t acc ; int32_t ind  ; uint32_t *ptr

// get/set stream insertion state
#define STREAM_GET_INSERT_STATE(s, acc, ind, ptr) acc = (s).acc_i ; ind = (s).insert ; ptr = (s).in
#define STREAM_SET_INSERT_STATE(s, acc, ind, ptr) (s).acc_i = acc ; (s).insert = ind ; (s).in = ptr

// get/set stream extraction state (unsigned)
#define STREAM_GET_XTRACT_STATE(s, acc, ind, ptr) acc = (s).acc_x ; ind = (s).xtract ; ptr = (s).out
#define STREAM_SET_XTRACT_STATE(s, acc, ind, ptr) (s).acc_x = acc ; (s).xtract = ind ; (s).out = ptr
// get stream extraction state (signed)
#define STREAM_GET_XTRACT_STATE_S(s, acc, ind, ptr) acc = (int64_t)(s).acc_x ; ind = (s).xtract ; ptr = (s).out
#endif
// =======================  stream initialization  =======================
//
// generic bit stream (re)initializer
// p    [OUT] : pointer to an existing bitstream structure (structure will be updated)
// mem   [IN] : pointer to memory (if NULL, allocate memory for bit stream)
// size  [IN] : size of the memory area (user supplied or auto allocated)
// mode  [IN] : combination of BIT_INSERT, BIT_XTRACT, BIT_FULL_INIT
// if mode == 0, both insertion and extraction operations are allowed
// size is in bytes
STATIC inline void  StreamInit(bitstream *p, void *mem, size_t size, int mode){
  uint32_t *buf = (uint32_t *) mem ;
  if(mode & BIT_FULL_INIT){
    *p = null_bitstream ;    // perform a full (re)initialization, nullify all fields
  }
  if((mode & (BIT_INSERT | BIT_XTRACT)) == 0) mode = mode | BIT_INSERT | BIT_XTRACT ;  // neither insert nor extract set, set both

  if( (p->first != NULL) && (p->in != NULL) && (p->out != NULL) && (p->limit != NULL) && (p->valid == 0xCAFEFADEu) ){
    buf = p->first ;        // existing and valid stream, already has a buffer, set buf to first
  }else{                    // not an existing stream, perform a full initialization
    if(buf == NULL){
      p->user   = 0 ;                          // not user supplied space
      p->alloc  = 1 ;                          // auto allocated space (can be freed if resizing)
      buf    = (uint32_t *) malloc(size) ;     // allocate space to accomodate up to size bytes
    }else{
      p->user   = 1 ;                          // user supplied space
      p->alloc  = 0 ;                          // not auto allocated space
    }
    p->full   = 0 ;                            // malloc not for both struct and buffer
    p->spare  = 0 ;
    p->valid   = 0xCAFEFADEu ;                 // mark bit stream as valid
    p->first  = buf ;                          // stream storage buffer
    p->limit  = buf + size/sizeof(uint32_t) ;  // potential truncation to 32 bit alignment
  }

  p->in     = buf ;                            // stream is empty and insertion starts at beginning of buffer
  p->out    = buf ;                            // stream is filled and extraction starts at beginning of buffer
  p->acc_i  = 0 ;                              // insertion accumulator is empty
  p->acc_x  = 0 ;                              // extraction accumulator is empty
  p->insert = 0 ;                              // insertion point at first free bit
  p->xtract = 0 ;                              // extraction point at first available bit
  if((mode & BIT_XTRACT) == 0) p->xtract = -1 ;  // deactivate extract mode (insert only mode)
  if((mode & BIT_INSERT) == 0) p->insert = -1 ;  // deactivate insert mode  (extract only mode)
}

// initialize a LittleEndian stream
STATIC inline void  LeStreamInit(bitstream *p, uint32_t *buffer, size_t size, int mode){
  StreamInit(p, buffer, size, mode) ;   // call generic stream init
  p->endian = STREAM_LE ;
}

// initialize a BigEndian stream
STATIC inline void  BeStreamInit(bitstream *p, uint32_t *buffer, size_t size, int mode){
  StreamInit(p, buffer, size, mode) ;   // call generic stream init
  p->endian = STREAM_BE ;
}

// =======================  stream creation (new stream)  =======================
//
// allocate a new bit stream struct and initialize it
// size [IN] : see StreamInit
// mode [IN] : see StreamInit
// return a pointer to the created struct
// p->full will be set, p->alloc will be 0
// p->alloc can end up as 1 if a resize is performed at a later time
static bitstream *StreamCreate(size_t size, int mode){
  char *buf ;
  bitstream *p = (bitstream *) malloc(size + sizeof(bitstream)) ;  // allocate size + overhead
fprintf(stderr, "StreamCreate : size = %ld, mode = %d\n", size*8, mode) ;
  buf = (char *) p ;
  buf += sizeof(bitstream) ;
  StreamInit(p, buf, size, mode | BIT_FULL_INIT) ;                 // fully initialize bit stream structure
  p->full = 1 ;                                                    // mark whole struct as allocated
  return p ;
}

// fully allocate and initialize a LittleEndian stream
static bitstream *LeStreamCreate(size_t size, int mode){
  bitstream *p = StreamCreate(size, mode) ;
  p->endian = STREAM_LE ;
  return p ;
}

// fully allocate and initialize a BigEndian stream
static bitstream *BeStreamCreate(size_t size, int mode){
  bitstream *p = StreamCreate(size, mode) ;
  p->endian = STREAM_BE ;
  return p ;
}

// =======================  stream size management =======================
//
// resize a stream
// p    [OUT] : pointer to a bitstream structure
// size  [IN] : size of the memory area (user supplied or auto allocated)
// funtion return : size of new buffer, 0 in case of error
// N.B. size MUST be larger than the original size
//      the contents of the old buffer will be copied to the new buffer
//      size is in bytes
STATIC inline size_t StreamResize(bitstream *p, void *mem, size_t size){
  uint32_t in, out ;
  size_t old_size ;

// fprintf(stderr, "new size = %ld, old size = %ld\n", size/sizeof(int32_t),  (p->limit - p->first)) ;
  if(size/sizeof(int32_t) <= (p->limit - p->first) ) return 0 ;         // size if too small
  if(mem == NULL) mem = malloc(size) ;                                  // allocate with malloc if mem is NULL
  if(mem == NULL) return 0 ;                                            // failed to allocate memory

  old_size = sizeof(int32_t) * (p->limit - p->first) ;                  // size of current buffer
  memmove(mem, p->first, old_size)  ;                                   // copy old (p->first) buffer into new (mem)
  in  = p->in - p->first ;                                              // relative position of in pointer
  out = p->out - p->first ;                                             // relative position of out pointer
  if(p->alloc) free(p->first) ;                                         // previous buffer was "malloced"
  p->alloc = (mem != NULL) ? 1 : 0 ;                                    // flag buffer as "malloced"
  p->first = (uint32_t *) mem ;                                         // updated first pointer
  p->in    = p->first + in ;                                            // updated in pointer
  p->out   = p->first + out ;                                           // updated out pointer
  p->limit = p->first + size / sizeof(int32_t) ;                        // updated limit pointer
  return size ;
}

// this function will be useful to make an already filled stream ready for extraction
// stream  [IN] : pointer to a bit stream struct
// pos     [IN] : number of valid bits for extraction from stream buffer
static int StreamSetFilledBits(bitstream *stream, size_t pos){
  if(! StreamIsValid(stream)) return 1 ;                    // invalid stream
  pos = (pos + 7) / 8 ;                                     // round up as bytes
  pos = (pos + sizeof(uint32_t) - 1) / sizeof(uint32_t) ;   // round up as 32 bit words
  if(pos > (stream->limit - stream->first) ) return 1 ;     // check for potential buffer overrrun
  stream->in = stream->first + pos ;                        // mark stream as filled up to stream->in
  return 0 ;
}

// mark stream as filled with size bytes
// size [IN] : number of bytes available for extraction (should be a multiple of 4)
STATIC inline int  StreamSetFilledBytes(bitstream *p, size_t size){
  return StreamSetFilledBits(p, size * 8) ;
}

// =======================  stream duplication  =======================
//
// duplicate a bit stream structure
// sdst [OUT] : pointer to a bit stream (destination)
// ssrc  [IN] : pointer to a bit stream (source)
static void StreamDup(bitstream *sdst, bitstream *ssrc){
//   memcpy(sdst, ssrc, sizeof(bitstream)) ;    // image copy of the bitstream struct
  *sdst = *ssrc ;
  sdst->full  = 0 ;                             // not fully allocated with malloc
  sdst->alloc = 0 ;                             // cannot allow a free, this is not the original copy
  sdst->user  = 1 ;                             // treat as if it were user allocated memory
}

// =======================  stream destuction  =======================
//
// free a bit stream structure (and allocated memory if possible)
// s [OUT] : pointer to a bit stream struct
// return : 0 if a free was performed, 1 if not
// N.B. s may no longer be valid at exit
static int StreamFree(bitstream *s){
  int status = 1 ;
  if(s->full){       // struct and buffer
    fprintf(stderr, "auto allocated bit stream (%p) freed\n", s) ;
    if(s->alloc) free(s->first) ;   // a resize operation has been performed
    free(s) ;
    return 0 ;
  }
  if(s->alloc){       // buffer was allocated with malloc
    fprintf(stderr, "auto allocated bit stream buffer (%p) freed\n", s->first) ;
    free(s->first) ;
    s->first = s->in = s->out = s->limit = NULL ;   // nullify data pointers
    s->user = s->full = s->alloc = 0 ;
    status = 0 ;
  }else{
    fprintf(stderr, "not owner of buffer, no free done (%p)\n", s->first);
  }
  s->insert = s->xtract = s->acc_i = s->acc_x = 0 ;  // mark accumulators as empty
  return status ;
}

// =======================  stream flush (insertion mode)  =======================
// flush stream being written into if any data left in insertion accumulator
STATIC inline void  LeStreamFlush(bitstream *p){
  if(p->insert > 0) LE64_INSERT_FINAL(p->acc_i, p->insert, p->in) ;
  p->acc_i = 0 ;
  p->insert = 0 ;
}

// flush stream being written into if any data left in insertion accumulator
STATIC inline void  BeStreamFlush(bitstream *p){
  if(p->insert > 0) BE64_INSERT_FINAL(p->acc_i, p->insert, p->in) ;
  p->acc_i = 0 ;
  p->insert = 0 ;
}

// flush stream being written into if any data left in insertion accumulator
STATIC inline void  StreamFlush(bitstream *p){
  if(STREAM_IS_BIG_ENDIAN(*p))    BeStreamFlush(p) ;
  if(STREAM_IS_LITTLE_ENDIAN(*p)) LeStreamFlush(p) ;
}

// =======================  stream push (insertion mode) =======================
// push any data left in insertion accumulator into stream witout updating control info
STATIC inline void  LeStreamPush(bitstream *p){
  if(p->insert > 0) LE64_PUSH(p->acc_i, p->insert, p->in) ;
}

// push any data left in insertion accumulator into stream witout updating control info
STATIC inline void  BeStreamPush(bitstream *p){
  if(p->insert > 0)  BE64_PUSH(p->acc_i, p->insert, p->in) ;
}

// push any data left in insertion accumulator into stream witout updating control info
STATIC inline void  StreamPush(bitstream *p){
  if(STREAM_IS_BIG_ENDIAN(*p))    BeStreamPush(p) ;
  if(STREAM_IS_LITTLE_ENDIAN(*p)) LeStreamPush(p) ;
}

// =======================  stream rewind =======================
// rewind a Little Endian stream to read it from the beginning (potentially force valid read mode)
STATIC inline void  LeStreamRewind(bitstream *p, int force_read){
//   if(p->insert > 0) LeStreamFlush(p) ;   // something left in insert accumulator ?
  if(p->insert > 0) LeStreamPush(p) ;   // something left in insert accumulator ?
  if(force_read) p->xtract = 0 ;
  if(p->xtract >= 0){
    p->acc_x  = 0 ;
    p->out = p->first ;
//     LE64_XTRACT_BEGIN(p->acc_x, p->xtract, p->out) ; // prime the pump
  }
}

// rewind a Big Endian stream to read it from the beginning (potentially force valid read mode)
STATIC inline void  BeStreamRewind(bitstream *p, int force_read){
//   if(p->insert > 0) BeStreamFlush(p) ;   // something left in insert accumulator ?
  if(p->insert > 0) BeStreamPush(p) ;   // something left in insert accumulator ?
  if(force_read) p->xtract = 0 ;
  if(p->xtract >= 0){
    p->acc_x  = 0 ;
    p->out = p->first ;
//     BE64_XTRACT_BEGIN(p->acc_x, p->xtract, p->out) ; // prime the pump
  }
}

STATIC inline void StreamRewind(bitstream *p, int force_read){
  if(STREAM_IS_BIG_ENDIAN(*p))    BeStreamRewind(p, force_read) ;
  if(STREAM_IS_LITTLE_ENDIAN(*p)) LeStreamRewind(p, force_read) ;
}

// =======================  stream reset =======================
// reset both read and write pointers to beginning of stream (according to insert/xtract only flags)
// STATIC inline void  StreamRewrite(bitstream *p){
STATIC inline void  StreamReset(bitstream *p){
  if(p->insert >= 0){      // insertion allowed
    p->in     = p->first ;
    p->acc_i  = 0 ;
    p->insert = 0 ;
//     if(STREAM_IS_BIG_ENDIAN(*p))    BE64_STREAM_INSERT_BEGIN(*p) ;
//     if(STREAM_IS_LITTLE_ENDIAN(*p)) LE64_STREAM_INSERT_BEGIN(*p) ;
  }
  if(p->xtract >= 0){      // extraction allowed
    p->out    = p->first ;
    p->acc_x  = 0 ;
    p->xtract = 0 ;
//     if(STREAM_IS_BIG_ENDIAN(*p))    BE64_STREAM_XTRACT_BEGIN(*p) ;
//     if(STREAM_IS_LITTLE_ENDIAN(*p)) LE64_STREAM_XTRACT_BEGIN(*p) ;
  }
}

// =======================  stream peek (extraction mode) =======================
//
// take a peek at future extracted data
STATIC inline uint32_t LeStreamPeek(bitstream *p, int nbits){
  uint32_t w32 ;
  LE64_PEEK_NBITS(p->acc_x, p->xtract, w32, nbits) ;
  return w32 ;
}

// take a peek at future extracted data
STATIC inline uint32_t BeStreamPeek(bitstream *p, int nbits){
  uint32_t w32 ;
  BE64_PEEK_NBITS(p->acc_x, p->xtract, w32, nbits) ;
  return w32 ;
}

// take a peek at future extracted data (signed)
STATIC inline int32_t LeStreamPeekSigned(bitstream *p, int nbits){
  int32_t w32 ;
  LE64_PEEK_NBITS((int64_t) p->acc_x, p->xtract, w32, nbits) ;
  return w32 ;
}

// take a peek at future extracted data (signed)
STATIC inline int32_t BeStreamPeekSigned(bitstream *p, int nbits){
  int32_t w32 ;
  BE64_PEEK_NBITS((int64_t) p->acc_x, p->xtract, w32, nbits) ;
  return w32 ;
}

// =======================  stream data access  =======================
//
// copy stream data into array mem (from beginning up to in pointer and data in accumulator if any)
// the original stream control info remains untouched (up to 2 32 bit items may get added to its buffer)
// stream [IN] : pointer to bit stream struct
// mem   [OUT] : where to copy
// size   [IN] : size of mem array in bytes
// return original size of valid info from stream in bits (-1 in case of error)
static int64_t StreamDataCopy(bitstream *stream, void *mem, size_t size){
  size_t nbtot, nborig ;
  bitstream temp ;    // temporary struct used during the copy process

  if(! StreamIsValid(stream))         return -1 ;               // invalid stream
  temp = *stream ;                                              // copy stream struct to avoid altering original

  // precise number of used bits in stream buffer
  nborig = (temp.in - temp.first) * 8 * sizeof(uint32_t) + ((temp.insert > 0) ? temp.insert : 0) ;
  if(temp.insert > 0) {                                         // flush contents of accumulator into buffer
    StreamFlush(&temp) ;
  }
  nbtot = (temp.in - temp.first) * sizeof(uint32_t) ;           // size in bytes when nothing is left in acumulator
  if(nbtot == 0) return 0 ;                                     // there was no data in stream
//   nbtot = ((nbtot + 31) / 32) * 32 ;                            // round to a multiple of 32 bits
//   nbtot /= 8 ;                                                  // convert to bytes
  if(nbtot > size) return -1 ;                                  // insufficient space
  if(mem != memmove(mem, temp.first, nbtot)) return -1 ;        // error copying
  return nborig ;                                               // return unrounded result
}

// =======================  prototypes for bi_endian_pack.c =======================
// insert multiple values (unsigned)
int  LeStreamInsert(bitstream *p, uint32_t *w32, int nbits, int nw);
// insert multiple values from list (unsigned)
int  LeStreamInsertM(bitstream *p, uint32_t *w32, int *nbits, int *n);

// insert multiple values (unsigned)
int  BeStreamInsert(bitstream *p, uint32_t *w32, int nbits, int nw);
// insert multiple values from list (unsigned)
int  BeStreamInsertM(bitstream *p, uint32_t *w32, int *nbits, int *n);

// extract multiple values (unsigned)
int  LeStreamXtract(bitstream *p, uint32_t *w32, int nbits, int n);
// extract multiple values (signed)
int  LeStreamXtractSigned(bitstream *p, int32_t *w32, int nbits, int n);
// extract multiple values from list (unsigned)
int  LeStreamXtractM(bitstream *p, uint32_t *w32, int *nbits, int *n);

// extract multiple values (unsigned)
int  BeStreamXtract(bitstream *p, uint32_t *w32, int nbits, int n);
// extract multiple values (signed)
int  BeStreamXtractSigned(bitstream *p, int32_t *w32, int nbits, int n);
// extract multiple values from list (unsigned)
int  BeStreamXtractM(bitstream *p, uint32_t *w32, int *nbits, int *n);

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
