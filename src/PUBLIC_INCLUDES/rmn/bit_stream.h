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
// made of 32 bit words
//

#if !defined(STREAM_ENDIANNESS)

#include <rmn/ct_assert.h>
// big and little endian macros
#include <rmn/bit_pack_macros.h>

#if ! defined(STATIC)
#define STATIC static
#define STATIC_DEFINED_HERE
#endif

static int stream_debug_mode = 0 ;

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

// stream insert/extract mode (0 or 3 would mean both insert and extract)
// extract mode
#define BIT_XTRACT        1
// insert mode
#define BIT_INSERT        2
// full initialization mode
#define BIT_FULL_INIT     8
// set endianness
#define SET_BIG_ENDIAN      16
#define SET_LITTLE_ENDIAN   32

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
STATIC inline ssize_t StreamAvailableSpace(bitstream *p){
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
  int32_t mode = 0 ;
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
  if(stream_debug_mode > 1) fprintf(stderr, "error (%s) in restore state\n", msg);
  return 1 ;
}
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
  if(mode & SET_BIG_ENDIAN   ) p->endian = STREAM_BE ;
  if(mode & SET_LITTLE_ENDIAN) p->endian = STREAM_LE ;
//   if(p->endian == 0) p->endian = STREAM_BE ;   //  default to BIG endian if not already defined
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


#if defined(STATIC_DEFINED_HERE)
#undef STATIC
#undef STATIC_DEFINED_HERE
#endif

#endif
