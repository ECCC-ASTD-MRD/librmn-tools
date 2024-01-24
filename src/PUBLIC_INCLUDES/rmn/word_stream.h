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
// set of macros and functions to manage insertion/extraction into/from stream of 32/64 bit words
//

#if ! defined(W32_SIZEOF)

#include <stdint.h>
#include <stdlib.h>
#include <rmn/ct_assert.h>

// same as sizeof() but value is in 32 bit units
#define W32_SIZEOF(item) (sizeof(item) >> 2)
CT_ASSERT(W32_SIZEOF(uint32_t) == 1) ;   // 1 x 32 bit item

// word (32 bit) stream descriptor
// number of words in buffer : in - out (available for extraction)
// space available in buffer : limit - in (available for insertion)
// if alloc == 1, a call to realloc is O.K., limit must be adjusted, in and out are unchanged
// initial state : in = out = 0
// the fields from the following struct should only be accessed through the appropriate macros
// WS32_IN, etc ...
typedef struct{
  uint32_t *buf ;     // pointer to start of stream data storage
  uint32_t limit ;    // buffer size (in 32 bit units)
  uint32_t in ;       // insertion index (0 initially)
  uint32_t out ;      // extraction index (0 initially)
  uint32_t valid:29 , // signature marker
           spare: 2 , // spare bits
           alloc: 1 ; // buffer allocated with malloc (realloc is possible)
} wordstream ;
CT_ASSERT(sizeof(wordstream) == 24) ;   // 3 64 bit elements (1 x 64 bits + 4x 32 bits)

static wordstream null_wordstream = { .buf = NULL, .limit = 0, .in = 0, .out = 0, .valid = 0, .alloc = 0 } ;

//
// ================================ word insertion/extraction macros into/from wordstream ===============================
//
// 29 bit signature in wordstream struct
#define WS32_MARKER    0x1234BEE5
//
// macro arguments description
// stream [INOUT] : word stream (type wordstream)
// wout     [OUT] : 32 bit word
// word      [IN] : 32 bit word
// nwds      [IN] : 32 bit unsigned integer
#define WS32_IN(stream)           ((stream).in)
#define WS32_OUT(stream)          ((stream).out)
#define WS32_SIZE(stream)         ((stream).limit)
#define WS32_BUFFER(stream)       ((stream).buf)

// stream status (valid/empty/full)
#define WS32_EMPTY(stream)        ((WS32_IN(stream)   - WS32_OUT(stream)) == 0 )
#define WS32_FULL(stream)         ((WS32_SIZE(stream) - WS32_IN(stream)) == 0 )
// number of available words for extraction
#define WS32_FILLED(stream)       (WS32_IN(stream)    - WS32_OUT(stream))
// number of emptyu words in buffer
#define WS32_TOFILL(stream)       (WS32_SIZE(stream)  - WS32_IN(stream))
// set all fields to 0
#define WS32_INIT(stream)         (stream) = null_wordstream ; // nullify stream
// marker validity check
#define WS32_MARKER_VALID(stream) ((stream).valid == WS32_MARKER)
// full validity check
#define WS32_VALID(stream) ( (WS32_MARKER_VALID(stream))                      && \
                                   (WS32_BUFFER(stream) != NULL)                    && \
                                   (WS32_OUT(stream)    <= WS32_IN(stream))   && \
                                   (WS32_IN(stream)     <  WS32_SIZE(stream)) && \
                                   (WS32_OUT(stream)    <  WS32_SIZE(stream)) )

#define WS32_CAN_REALLOC     1
// create a word stream
// stream [INOUT] : pointer to a wordstream struct
// mem       [IN] : memory address (if NULL, use malloc)
// nwds      [IN] : number of 32 bit words that the stream has to be able to contain
// in        [IN] : if mem is not NULL, set insertion index at position in (ignored otherwise)
// options   [IN] : 0 = no options possible options : WS32_CAN_REALLOC
// return the address of the wordstream buffer (NULL in case of error)
static uint32_t *ws32_create(wordstream *stream, void *mem, uint32_t nwds, uint32_t in, uint32_t options){
  if(WS32_MARKER_VALID(*stream)) return NULL ;        // existing stream
  WS32_INIT(*stream) ;                                // initialize to null values
  WS32_BUFFER(*stream) = (mem == NULL) ? (uint32_t *) malloc(sizeof(uint32_t) * nwds) : mem ;
  if(WS32_BUFFER(*stream) != NULL) {
    stream->valid = WS32_MARKER ;                     // set stream marker
    stream->alloc = (mem == NULL) ? 1 : 0 ;                 // malleoc(ed) if mem == NULL
    if(options & WS32_CAN_REALLOC) stream->alloc = 1 ;       // user supplied but still realloc(atable)
    if(mem != NULL) stream->in = (in < nwds) ? in : nwds ;  // stream already filled up to in
    stream->limit = nwds ;                                  // set buffer size
  }
  return WS32_BUFFER(*stream) ;                       // return address of buffer
}

// is this stream valid and have consistent indexes
// stream [IN] : pointer to a wordstream struct
static int ws32_is_valid(wordstream *stream){
  return WS32_VALID(*stream) ;
}

#define WS32_REREAD(stream)       (stream).out = 0
#define WS32_REWRITE(stream)      (stream).in  = 0
// set all indexes to start of buffer (reread + rewrite)
#define WS32_RESET(stream)        (stream).in  = (stream).out = 0
// insert 1 word into a word stream (UNSAFE, no overrun check is made)
#define WS32_INSERT1(stream, word) { (stream).buf[(stream).in] = (word) ; (stream).in++ ; }
// insert multiple words into a word stream
// stream [INOUT] : pointer to a wordstream struct
// words     [IN] : pointer to data to be inserted (32 bit words)
// nwords    [IN] : number of 32 bit elements to insert into word stream
// return number of words inserted (-1 in case of error)
static int ws32_insert(wordstream *stream, void *words, uint32_t nwords){
  uint32_t *data = (uint32_t *) words ;
  int status = -1 ;
  if(WS32_MARKER_VALID(*stream)) {           // quick check that stream is valid
//     fprintf(stderr, "in = %d, out = %d, limit = %d\n", WS32_IN(*stream), WS32_OUT(*stream), WS32_SIZE(*stream));
    if(WS32_TOFILL(*stream) >= nwords){      // is enough space available in stream
      status = nwords ;
      while(nwords > 0){
        WS32_INSERT1(*stream, *data) ;
        nwords-- ; data++ ;
      }
    }
//     fprintf(stderr, "in = %d, out = %d, limit = %d\n", WS32_IN(*stream), WS32_OUT(*stream), WS32_SIZE(*stream));
  }
  return status ;
}
#define WS32_NEXT(stream)  WS32_BUFFER(stream)[WS32_OUT(stream)]
// get next available 32 bit word, do not advance out index
// stream [IN] : pointer to a wordstream struct
// return zero if word stream is empty
static uint32_t ws32_peek(wordstream *stream){
  return (WS32_EMPTY(*stream)) ? 0 : WS32_NEXT(*stream) ;
}
#define WS32_XTRACT1(stream, wout) { (wout) = (stream).buf[(stream).out] ; (stream).out++ ; }
// extract multiple words from a word stream
// stream [INOUT] : pointer to a wordstream struct
// words     [IN] : pointer to memory area to receive data (32 bit words)
// nwords    [IN] : number of 32 bit elements to extract from word stream
// return number of words extracted (-1 in case of error)
static int ws32_xtract(wordstream *stream, void *words, uint32_t nwords){
  uint32_t *data = (uint32_t *) words ;
  int status = -1 ;
  if(WS32_MARKER_VALID(*stream)) {         // check that stream is valid
//     fprintf(stderr, "in = %d, out = %d, limit = %d\n", WS32_IN(*stream), WS32_OUT(*stream), WS32_SIZE(*stream));
    if(WS32_FILLED(*stream) >= nwords){      // is enough data available in stream
      status = nwords ;
      while(nwords > 0){
        WS32_XTRACT1(*stream, *data) ;
        nwords-- ; data++ ;
      }
    }
//     fprintf(stderr, "in = %d, out = %d, limit = %d\n", WS32_IN(*stream), WS32_OUT(*stream), WS32_SIZE(*stream));
  }
  return status ;
}
// resize the buffer of a wordstream
// stream [INOUT] : pointer to a valid wordstream struct
// return 0 if resize successful (or not needed)
// return -1 in case of resize error
static int ws32_resize(wordstream *stream, uint32_t size){
  int status = -1 ;
  if(stream->alloc == 1){       // check that buffer was "malloc(ed)" and can be "realloc(ed)"
    if(size > WS32_SIZE(*stream)){   // requested size > actual size, reallocate a bigger buffer
      size_t newsize = size * sizeof(uint32_t) ;
      void *ptr = realloc(stream->buf, newsize) ;
      if(ptr != NULL){
        status = 0 ;
        WS32_BUFFER(*stream) = (uint32_t *) ptr ;
        WS32_SIZE(*stream)   = size ;
      }
    }else{
      status = 0 ;              // size not increased, no need to reallocate
    }
  }
  return status ;
}

// =======================  word stream state save/restore  =======================
// state of a word stream
typedef struct{
  uint32_t in ;       // insertion index (0 initially)
  uint32_t out ;      // extraction index (0 initially)
} wordstream_state ;
CT_ASSERT(sizeof(wordstream_state) == 8) ;   // 2 32 bit elements

#define WS32_STATE_IN(wordstate)           ((wordstate).in)
#define WS32_STATE_OUT(wordstate)          ((wordstate).out)
//
// save current state of a word stream
// stream  [IN] : pointer to a valid wordstream struct
// state  [OUT] : pointer to a wordstream_state struct
// return 0 if O.K., -1 if error
static int WStreamSaveState(wordstream *stream, wordstream_state *state){
  int status = -1 ;
  if(WS32_MARKER_VALID(*stream)){
    WS32_STATE_IN(*state)  = WS32_IN(*stream) ;
    WS32_STATE_OUT(*state) = WS32_OUT(*stream) ;
    status = 0 ;
  }
  return status ;
}

#define WS32_R    1
#define WS32_W    2
#define WS32_RW   3
// restore state of a word stream from a saved state
// stream [OUT] : pointer to a valid wordstream struct
// state   [IN] : pointer to a wordstream_state struct
// mode    [IN] : restore in, out, or both
// return 0 if O.K., -1 if error
static int WStreamRestoreState(wordstream *stream, wordstream_state *state, int mode){
  int status = -1 ;
  int new_in, new_out ;

  if(ws32_is_valid(stream)){                                  // check that stream is valid
    if(mode == 0) mode = WS32_RW ;                            // 0 means both read and write
    new_in  = (WS32_W & mode) ? state->in  : stream->in  ;    // new in index if write mode
    new_out = (WS32_R & mode) ? state->out : stream->out ;    // new out index if read mode
    if( (new_out <= new_in) && (new_in < stream->limit) ) {         // check that new stream indexes are consistent
      stream->in  = new_in ;                                        // update in and out indexes
      stream->out = new_out ;
      status = 0 ;
    }
  }
  return status ;
}

#endif
