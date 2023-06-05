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
// Lesser General Public License for more details.
//
// Author:
//     M. Valin,   Recherche en Prevision Numerique, 2022
//
// set of macros and functions to manage insertion/extraction into/from a bit stream
//
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#if ! defined(MAKE_SIGNED_32)

#if ! defined(STATIC)
#define STATIC static
#define STATIC_DEFINED_HERE
#endif

// bit stream descriptor. ONLY ONE of insert / extract should be positive
// in insertion mode, xtract should be -1
// in extraction mode, insert should be -1
// for now, a bit stream is unidirectional (either insert or extract mode)
typedef struct{
//   uint64_t  accum ;   // 64 bit unsigned bit accumulator
  uint64_t  acc_i ;   // 64 bit unsigned bit accumulator for insertion
  uint64_t  acc_x ;   // 64 bit unsigned bit accumulator for extraction
  int32_t   insert ;  // # of bits used in accumulator (0 <= insert <= 64)
  int32_t   xtract ;  // # of bits extractable from accumulator (0 <= xtract <= 64)
  uint32_t *first ;   // pointer to start of stream data storage
//   uint32_t *stream ;  // pointer into packed stream (both insert and extract mode)
  uint32_t *in ;      // pointer into packed stream (insert mode)
  uint32_t *out ;     // pointer into packed stream (extract mode)
  uint32_t *limit ;   // pointer to end of stream data storage (1 byte beyond stream buffer end)
  uint64_t alloc:1 ,  // structure and buffer allocated together with malloc (buf[] is usable)
           dyn:1 ,    // buffer allocated with malloc (buf[] is NOT usable)
           resv:63 ;  // reserved
  uint32_t buf[] ;    // flexible array
} bitstream ;
// alternative struct 
// typedef struct{
//   uint64_t  accum ;   // 64 bit unsigned bit accumulator
//   int32_t   insert ;  // # of bits used in accumulator (0 <= insert <= 64)
//   int32_t   xtract ;  // # of bits extractable from accumulator (0 <= xtract <= 64)
//   uint32_t *first ;   // pointer to start of bit stream storage
//   uint32_t *in ;      // pointer into packed stream (insert mode)
//   uint32_t *out ;     // pointer into packed stream (extract mode)
//   uint32_t *limit ;   // pointer to end of bit stream storage (1 location beyond stream buffer end)
//   uint64_t alloc:1 ,  // structure and buffer allocated together with malloc (buf[] is usable)
//            dyn:1 ,    // buffer allocated with malloc (buf[] is NOT usable)
//            resv:63 ;  // reserved
//   uint32_t buf[] ;    // elastic array
// } bit_stream ;        // linear (non circular) bit stream storage

// duplicate a bit stream descriptor
static void StreamDup(bitstream *sdst, bitstream *ssrc){
  memcpy(sdst, ssrc, sizeof(bitstream)) ;
}

// allocate a bit stream capable of storintg up to size 32 bit items
// static bit_stream *bit_stream_allocate(int size){
//   bit_stream *t = (bit_stream *) malloc(size*sizeof(uint32_t) + sizeof(bit_stream)) ;
//   t->resv = 0 ;
//   t->alloc = 1 ;
//   t->dyn = 0 ;
//   t->accum = 0 ;
//   t->insert = 0 ;
//   t->xtract = 0 ;
//   t->first = t->in = t->out = &(t->buf[0]) ;
//   t->limit = t->first + size ;
//   return t ;
// }

// initialize a bit stream s for storing up to size 32 bit items
// if buf is not NULL, use caller supplied space to store the bit stream
// otherwise allocate space
// static void bit_stream_init(bit_stream *s, int size, void *buf){
//   s->resv = 0 ;
//   s->alloc = 0 ;
//   s->accum = 0 ;
//   s->insert = 0 ;
//   s->xtract = 0 ;
//   if(buf != NULL){
//     s->first = (uint32_t *)buf ;  // buffer space supplied by caller
//     s->dyn = 0 ;
//   }else{
//     s->first = (uint32_t *)malloc(size*sizeof(uint32_t)) ;  // allocate buffer space
//     s->dyn = 1 ;                  // automatically allocated buffer space
//   }
//   s->in = s->out = s->first ;
//   s->limit = s->first + size ;
// }

// free memory space associated with bit stream s
// set stream to null contents
// return stream address if it was not entirely allocated with malloc
// return NULL if it was
// static bit_stream *bit_stream_free(bit_stream *s){
//   s->accum = 0 ;
//   s->insert = 0 ;
//   s->xtract = 0 ;
//   if(s->dyn) {        // dynamic buffer space in "non dynamic" stream
//     free(s->first) ;
//     s->dyn = 0 ;
//     s->limit = s->first = NULL ;
//   }
//   s->in = s->out = s->first ;  // bit stream is empty
//   if(s->alloc) {      // stream was allocated with malloc
//     free(s) ;
//     s = NULL ;
//   }
//   return s ;
// }

// for this macro to produce meaningful results, w32 MUST BE int32_t (32 bit signed int)
#define MAKE_SIGNED_32(w32, nbits) { w32 <<= (32 - (nbits)) ; w32 >>= (32 - (nbits)) ; }
// for this macro to produce meaningful results, w64 MUST BE int64_t (64 bit signed int)
#define MAKE_SIGNED_64(w64, nbits) { w64 <<= (64 - (nbits)) ; w64 >>= (64 - (nbits)) ; }

// stream insert/extract mode (0 would mean insert or extract)
// extract only mode
#define BIT_XTRACT_MODE  1
// insert only mode
#define BIT_INSERT_MODE  2

//
// macro arguments description
// accum  [INOUT] : 64 bit accumulator (normally acc_i or acc_x)
// insert [INOUT] : # of bits used in accumulator (0 <= insert <= 64)
// xtract [INOUT] : # of bits extractable from accumulator (0 <= xtract <= 64)
// stream [INOUT] : pointer to next position in bit stream (in or out component of bitstream struct)
// w32    [IN]    : 32 bit integer containing data to be inserted (expression allowed)
//        [OUT]   : 32 bit integer receiving extracted data (MUST be a variable)
// nbits  [IN]    : number of bits to insert / extract in w32 (<= 32 bits)
//
// N.B. : if w32 and accum are signed variables, extraction will produce a "signed" result
//        if w32 and accum are unsigned variables, extraction will produce an "unsigned" result
//
// little endian style (right to left) bit stream packing
// insertion at top, most significant part
// the useful bits are at the bottom (least significant part) of accum
//
// initialize stream for insertion
#define LE64_INSERT_BEGIN(accum, insert) { accum = 0 ; insert = 0 ; }
#define LE64_STREAM_INSERT_BEGIN(s) { LE64_INSERT_BEGIN(s.acc_i, s.insert) }
// insert the lower nbits bits from w32 into accum, update insert, accum
#define LE64_INSERT_NBITS(accum, insert, w32, nbits) \
        { uint32_t mask = ~0 ; mask >>= (32-(nbits)) ; uint64_t w64 = (w32) & mask ; accum |= (w64 << insert) ; insert += (nbits) ; }
#define LE64_STREAM_INSERT_NBITS(s, w32, nbits) LE64_INSERT_NBITS(s.acc_i, s.insert, w32, nbits)
// check that 32 bits can be safely inserted into accum
// if not possible, store lower 32 bits of accum into stream, update accum, insert, stream
#define LE64_INSERT_CHECK(accum, insert, stream) \
        { if(insert > 32) { *stream = accum ; stream++ ; insert -= 32 ; accum >>= 32 ; } ; }
#define LE64_STREAM_INSERT_CHECK(s) LE64_INSERT_CHECK(s.acc_i, s.insert, s.stream)
// push data to stream without updating control info
#define LE64_PUSH(accum, insert, stream) \
        { LE64_INSERT_CHECK(accum, insert, stream) ; { if(insert > 0) { *stream = accum ;} ; } }
// store any residual data from accum into stream, update accum, insert, stream
#define LE64_INSERT_FINAL(accum, insert, stream) \
        { LE64_INSERT_CHECK(accum, insert, stream) ; { if(insert > 0) { *stream = accum ; stream++ ;} ; } }
#define LE64_STREAM_INSERT_FINAL(s) LE64_INSERT_FINAL(s.acc_i, s.insert, s.stream)
// combined INSERT_CHECK and INSERT_NBITS, update accum, insert, stream
#define LE64_PUT_NBITS(accum, insert, w32, nbits, stream) \
        { LE64_INSERT_CHECK(accum, insert, stream) ; LE64_INSERT_NBITS(accum, insert, w32, nbits) ; }
#define LE64_STREAM_PUT_NBITS(s, w32, nbits) LE64_PUT_NBITS(s.acc_i, s.insert, w32, nbits, s.stream)

// N.B. : if w32 and accum are signed variables, the extract will produce a "signed" result
//        if w32 and accum are unsigned variables, the extract will produce an "unsigned" result
// initialize stream for extraction
#define LE64_XTRACT_BEGIN(accum, xtract, stream) { accum = *(stream) ; (stream)++ ; xtract = 32 ; }
#define LE64_STREAM_XTRACT_BEGIN(s) { LE64_XTRACT_BEGIN(s.acc_x, s.xtract, s.stream) ; }
// take a peek at nbits bits from accum into w32
#define LE64_PEEK_NBITS(accum, xtract, w32, nbits) { w32 = (accum << (64-nbits)) >> (64-nbits) ; }
#define LE64_STREAM_PEEK_NBITS(s, w32, nbits) LE64_PEEK_NBITS(s.acc_x, s.xtract, w32, nbits)
#define LE64_STREAM_PEEK_NBITS_S(s, w32, nbits) LE64_PEEK_NBITS((int64_t) s.acc_x, s.xtract, w32, nbits)
// extract nbits bits into w32 from accum, update xtract, accum
#define LE64_XTRACT_NBITS(accum, xtract, w32, nbits) \
        { w32 = (accum << (64-nbits)) >> (64-nbits) ; accum >>= nbits ; xtract -= (nbits) ;}
#define LE64_STREAM_XTRACT_NBITS(s, w32, nbits) LE64_XTRACT_NBITS(s.acc_x, s.xtract, w32, nbits)
// check that 32 bits can be safely extracted from accum
// if not possible, get extra 32 bits into accum from stresm, update accum, xtract, stream
#define LE64_XTRACT_CHECK(accum, xtract, stream) \
        { if(xtract < 32) { uint64_t w64 = *(stream) ; accum |= (w64 << xtract) ; (stream)++ ; xtract += 32 ; } ; }
#define LE64_STREAM_XTRACT_CHECK(s) LE64_XTRACT_CHECK(s.acc_x, s.xtract, s.stream)
// finalize extraction, update accum, xtract
#define LE64_XTRACT_FINAL(accum, xtract) \
        { accum = 0 ; xtract = 0 ; }
#define LE64_STREAM_XTRACT_FINAL(s) LE64_XTRACT_FINAL(s.acc_x, s.xtract)
// combined XTRACT_CHECK and XTRACT_NBITS, update accum, xtract, stream
#define LE64_GET_NBITS(accum, xtract, w32, nbits, stream) \
        { LE64_XTRACT_CHECK(accum, xtract, stream) ; LE64_XTRACT_NBITS(accum, xtract, w32, nbits) ; }
#define LE64_STREAM_GET_NBITS(s, w32, nbits) LE64_GET_NBITS(s.acc_x, s.xtract, w32, nbits, s.stream)
//
// big endian style (left to right) bit stream packing
// insertion at bottom, least significant part
//
// initialize stream for insertion
#define BE64_INSERT_BEGIN(accum, insert) { accum = 0 ; insert = 0 ; }
#define BE64_STREAM_INSERT_BEGIN(s) { BE64_INSERT_BEGIN(s.acc_i, s.insert) ; }
// insert the lower nbits bits from w32 into accum, update insert, accum
#define BE64_INSERT_NBITS(accum, insert, w32, nbits) \
        {  uint32_t mask = ~0 ; mask >>= (32-(nbits)) ; accum <<= (nbits) ; insert += (nbits) ; accum |= ((w32) & mask) ; }
#define BE64_STREAM_INSERT_NBITS(s, w32, nbits) BE64_INSERT_NBITS(s.acc_i, s.insert, w32, nbits)
// check that 32 bits can be safely inserted into accum
// if not possible, store lower 32 bits of accum into stream, update accum, insert, stream
#define BE64_INSERT_CHECK(accum, insert, stream) \
        { if(insert > 32) { insert -= 32 ; *(stream) = accum >> insert ; (stream)++ ; } ; }
#define BE64_STREAM_INSERT_CHECK(s) BE64_INSERT_CHECK(s.acc_i, s.insert, s.stream)
// push data to stream without updating control info
#define BE64_PUSH(accum, insert, stream) \
        { BE64_INSERT_CHECK(accum, insert, stream) ; if(insert > 0) { *stream = accum << (32 - insert) ; } }
// store any residual data from accum into stream, update accum, insert, stream
#define BE64_INSERT_FINAL(accum, insert, stream) \
        { BE64_INSERT_CHECK(accum, insert, stream) ; if(insert > 0) { *stream = accum << (32 - insert) ; stream++ ; } }
#define BE64_STREAM_INSERT_FINAL(s) BE64_INSERT_FINAL(s.acc_i, s.insert, s.stream)
// combined INSERT_CHECK and INSERT_NBITS, update accum, insert, stream
#define BE64_PUT_NBITS(accum, insert, w32, nbits, stream) \
        { BE64_INSERT_CHECK(accum, insert, stream) ; BE64_INSERT_NBITS(accum, insert, w32, nbits) ; }
#define BE64_STREAM_PUT_NBITS(s, w32, nbits) BE64_PUT_NBITS(s.acc_i, s.insert, w32, nbits, s.stream)

// N.B. : if w32 and accum are signed variables, the extract will produce a "signed" result
//        if w32 and accum are unsigned variables, the extract will produce an "unsigned" result
// initialize stream for extraction
#define BE64_XTRACT_BEGIN(accum, xtract, stream) { accum = *(stream) ; accum <<= 32 ; (stream)++ ; xtract = 32 ; }
#define BE64_STREAM_XTRACT_BEGIN(s) { BE64_XTRACT_BEGIN(s.acc_x, s.xtract, s.stream) ; }
// take a peek at nbits bits from accum into w32
#define BE64_PEEK_NBITS(accum, xtract, w32, nbits) { w32 = accum >> (64 - (nbits)) ; }
#define BE64_STREAM_PEEK_NBITS(s, w32, nbits) BE64_PEEK_NBITS(s.acc_x, s.xtract, w32, nbits)
#define BE64_STREAM_PEEK_NBITS_S(s, w32, nbits) BE64_PEEK_NBITS((int64_t) s.acc_x, s.xtract, w32, nbits)
// extract nbits bits into w32 from accum, update xtract, accum
#define BE64_XTRACT_NBITS(accum, xtract, w32, nbits) \
        { w32 = accum >> (64 - (nbits)) ; accum <<= (nbits) ; xtract -= (nbits) ; }
#define BE64_STREAM_XTRACT_NBITS(s, w32, nbits) BE64_XTRACT_NBITS(s.acc_x, s.xtract, w32, nbits)
// check that 32 bits can be safely extracted from accum
// if not possible, get extra 32 bits into accum from stresm, update accum, xtract, stream
#define BE64_XTRACT_CHECK(accum, xtract, stream) \
        { if(xtract < 32) { accum >>= (32-xtract) ; accum |= *(stream) ; accum <<= (32-xtract) ; xtract += 32 ; (stream)++ ; } ; }
#define BE64_STREAM_XTRACT_CHECK(s) BE64_XTRACT_CHECK(s.acc_x, s.xtract, s.stream)
// finalize extraction, update accum, xtract
#define BE64_XTRACT_FINAL(accum, xtract) { accum = 0 ; xtract = 0 ; }
#define BE64_STREAM_XTRACT_FINAL(s) BE64_XTRACT_FINAL(s.acc_x, s.xtract)
// combined XTRACT_CHECK and XTRACT_NBITS, update accum, xtract, stream
#define BE64_GET_NBITS(accum, xtract, w32, nbits, stream) \
        { BE64_XTRACT_CHECK(accum, xtract, stream) ; BE64_XTRACT_NBITS(accum, xtract, w32, nbits) ; }
#define BE64_STREAM_GET_NBITS(s, w32, nbits) BE64_GET_NBITS(s.acc_x, s.xtract, w32, nbits, s.stream)

// true if stream is in read (extract) mode
// possibly false for a NEWLY INITIALIZED stream
#define STREAM_READ_MODE(s) (s.xtract >= 0)
// true if stream is in write (insert) mode
// possibly false for a NEWLY INITIALIZED (empty) stream
#define STREAM_WRITE_MODE(s) (s.insert >= 0)

// get stream mode as a string
STATIC inline char *StreamMode(bitstream p){
  if( p.insert >= 0 && p.xtract >= 0) return("ReadWrite") ;
  if( STREAM_READ_MODE(p) ) return "Read" ;
  if( STREAM_WRITE_MODE(p) ) return "Write" ;
  return("Unknown") ;
}
STATIC inline int StreamModeCode(bitstream p){
  if( p.insert >= 0 && p.xtract >= 0) return 0 ;
  if( STREAM_READ_MODE(p) ) return BIT_XTRACT_MODE ;
  if( STREAM_WRITE_MODE(p) ) return BIT_INSERT_MODE ;
  return -1 ;
}

// bits used in accumulator (stream being filled)
#define ACCUM_BITS_FILLED(s) (s.insert)
// bits used in stream (stream being filled)
#define STREAM_BITS_FILLED(s) (s.insert + (s.in - s.out) * 8l * sizeof(uint32_t))
// bits left to fill in stream (stream being filled)
#define STREAM_BITS_EMPTY(s) ( (s.limit - s.in) * 8l * sizeof(uint32_t) - s.insert)

// bits available in accumulator (stream in extract mode)
#define ACCUM_BITS_AVAIL(s) (s.xtract)
// bits available in stream (stream in extract mode)
#define STREAM_BITS_AVAIL(s) (s.xtract + (s.in - s.out) * 8l * sizeof(uint32_t) )

// size of stream buffer (in bits)
#define STREAM_BUFFER_SIZE(s) ( (s.limit - s.first) * 8l * sizeof(uint32_t) )

// initialize a LittleEndian stream
// mode should be BIT_INSERT_MODE or BIT_XTRACT_MODE or 0(both insert and extract)
// size is in bytes
// N.B. even if mode is not one of the above, 
//      it is still possible to insert/extract into/from bit stream
STATIC inline void  LeStreamInit(bitstream *p, uint32_t *buffer, size_t size, int mode){
  p->acc_i  = 0 ;         // insertion accumulator is empty
  p->acc_x  = 0 ;         // extraction accumulator is empty
  p->insert = 0 ;         // insertion point at Least Significant Bit
  p->xtract = 0 ;         // extraction point at Least Significant Bit
  if(mode == BIT_INSERT_MODE) p->xtract = -1 ;  // deactivate extract mode (insert only mode)
  if(mode == BIT_XTRACT_MODE) p->insert = -1 ;  // deactivate insert mode (extract only mode)
  p->first  = buffer ;    // stream storage
  p->limit  = buffer + size / sizeof(uint32_t) ;
  p->in     = buffer ;    // stream is empty and starts at beginning of buffer
  p->out    = buffer ;    // stream is full and starts at beginning of buffer
}

// initialize a BigEndian stream
// mode should be BIT_INSERT_MODE or BIT_XTRACT_MODE or 0(both insert and extract)
// size is in bytes
STATIC inline void  BeStreamInit(bitstream *p, uint32_t *buffer, size_t size, int mode){
  p->acc_i  = 0 ;         // insertion accumulator is empty
  p->acc_x  = 0 ;         // extraction accumulator is empty
  p->insert = 0 ;         // insertion point at Most Significant Bit
  p->xtract = 0 ;         // extraction point at Most Significant Bit
  if(mode == BIT_INSERT_MODE) p->xtract = -1 ;  // deactivate extract mode
  if(mode == BIT_XTRACT_MODE) p->insert = -1 ;  // deactivate insert mode
  p->first  = buffer ;    // stream storage
  p->limit   = buffer + size / sizeof(uint32_t) ;
  p->in     = buffer ;    // stream is empty and starts at beginning of buffer
  p->out    = buffer ;    // stream is full and starts at beginning of buffer
}

STATIC inline uint32_t RMask(uint32_t nbits){
  uint32_t mask = ~0 ;
  return  ( mask >> (32 - nbits)) ;
}

STATIC inline uint32_t LMask(uint32_t nbits){
  uint32_t mask = ~0 ;
  return  ( mask << (32 - nbits)) ;
}

// number of bits available for extraction
STATIC inline int32_t StreamAvailableBits(bitstream *p){
  if(p->xtract < 0) return -1 ;             // extraction not allowd
  return (p->in - p->out)*32 + p->insert + p->xtract ;  // stream + accumulators contents
}
STATIC inline int32_t StreamStrictAvailableBits(bitstream *p){
  if(p->xtract < 0) return -1 ;             // extraction not allowd
  return (p->in - p->out)*32 + p->xtract ;              // stream + extract accumulator contents
}

// number of bits available for insertion
STATIC inline int32_t StreamAvailableSpace(bitstream *p){
  if(p->insert < 0) return -1 ;   // insertion not allowd
  return (p->limit - p->in)*32 - p->insert ;
}

// flush stream being written into if any data left in insertion accumulator
STATIC inline void  LeStreamFlush(bitstream *p){
  if(p->insert > 0) LE64_INSERT_FINAL(p->acc_i, p->insert, p->in) ;
  p->acc_i = 0 ;
}

// push any data left in insertion accumulator into stream witout updating control info
STATIC inline void  LeStreamPush(bitstream *p){
  if(p->insert > 0) LE64_PUSH(p->acc_i, p->insert, p->in) ;
}

// flush stream being written into if any data left in insertion accumulator
STATIC inline void  BeStreamFlush(bitstream *p){
  if(p->insert > 0) BE64_INSERT_FINAL(p->acc_i, p->insert, p->in) ;
  p->acc_i = 0 ;
}

// push any data left in insertion accumulator into stream witout updating control info
STATIC inline void  BeStreamPush(bitstream *p){
  if(p->insert > 0)  BE64_PUSH(p->acc_i, p->insert, p->in) ;
}

// rewind Little Endian stream to read it from the beginning (make sure at least 32 bits are extractable)
STATIC inline void  LeStreamRewind(bitstream *p){
//   if(p->insert > 0) LeStreamFlush(p) ;   // something left in insert accumulator ?
  if(p->insert > 0) LeStreamPush(p) ;   // something left in insert accumulator ?
  p->acc_x = *(p->first) ;       // fill extraction accumulator from stream, extraction position at LSB
  p->xtract = 32 ;               // 32 bits are available, do not touch insert control info
  p->out = p->first + 1 ;        // point to next stream item
}

// rewind Big Endian stream to read it from the beginning (make sure at least 32 bits are extractable)
STATIC inline void  BeStreamRewind(bitstream *p){
//   if(p->insert > 0) BeStreamFlush(p) ;   // something left in insert accumulator ?
  if(p->insert > 0) BeStreamPush(p) ;   // something left in insert accumulator ?
  p->acc_x = *(p->first) ;       // fill extraction accumulator from stream
  p->acc_x <<= 32 ;              // extraction position at MSB
  p->xtract = 32 ;               // 32 bits are available, do not touch insert control info
  p->out = p->first + 1 ;        // point to next stream item
}

// take a peek at future extraction
STATIC inline uint32_t LeStreamPeek(bitstream *p, int nbits){
  uint32_t w32 ;
  LE64_PEEK_NBITS(p->acc_x, p->xtract, w32, nbits) ;
  return w32 ;
}

// take a peek at future extraction
STATIC inline int32_t LeStreamPeekSigned(bitstream *p, int nbits){
  int32_t w32 ;
  LE64_PEEK_NBITS((int64_t) p->acc_x, p->xtract, w32, nbits) ;
  return w32 ;
}

// take a peek at future extraction
STATIC inline uint32_t BeStreamPeek(bitstream *p, int nbits){
  uint32_t w32 ;
  BE64_PEEK_NBITS(p->acc_x, p->xtract, w32, nbits) ;
  return w32 ;
}

// take a peek at future extraction
STATIC inline int32_t BeStreamPeekSigned(bitstream *p, int nbits){
  int32_t w32 ;
  BE64_PEEK_NBITS((int64_t) p->acc_x, p->xtract, w32, nbits) ;
  return w32 ;
}

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
