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

#include <rmn/ct_assert.h>

// same as sizeof() but value is in 32 bit units
#define W32_SIZEOF(what) (sizeof(what) >> 2)
// same as sizeof() but value is in 64 bit units
// #define W64_SIZEOF(what) (sizeof(what) >> 3)

// word (32 bit) stream descriptor
// number of words in buffer : in - out (available for extraction)
// space available in buffer : limit - in (available for insertion)
// if alloc == 1, a call to realloc is O.K., limit must be adjusted, in and out are unchanged
// initial state : in = out = 0
// the fields from the following struct should only be accessed through the appropriate macros
// W32_STREAM_IN, etc ...
typedef struct{
  uint32_t *buf ;     // pointer to start of stream data storage
  uint32_t limit ;    // buffer size
  uint32_t in ;       // insertion position (0 initially)
  uint32_t out ;      // extraction position (0 initially)
  uint32_t valid:29 , // signature marker
           spare: 2 , // spare bits
           alloc: 1 ; // buffer allocated with malloc (realloc is possible)
} wordstream ;
CT_ASSERT(sizeof(wordstream) == 24) ;   // 3 64 bit elements (1 x 64 bits + 4x 32 bits)

static wordstream null_wordstream = { .buf = NULL, .limit = 0, .in = 0, .out = 0,
                                      .valid = 0, .alloc = 0 } ;
//
// ================================ word insertion/extraction macros into/from wordstream ===============================
//
#define W32_STREAM_MARKER    0x1234BEE5
//
// macro arguments description
// stream [INOUT] : word stream (type wordstream)
// wout     [OUT] : 32 bit word
// word      [IN] : 32 bit word
// nwds      [IN] : 32 bit unsigned integer
#define W32_STREAM_IN(stream)           ((stream).in)
#define W32_STREAM_OUT(stream)          ((stream).out)
#define W32_STREAM_SIZE(stream)         ((stream).limit)
#define W32_STREAM_BUFFER(stream)       ((stream).buf)

// stream status (valid/empty/full)
#define W32_STREAM_VALID(stream)        ((W32_STREAM_BUFFER(stream) != NULL) && ((stream).valid == W32_STREAM_MARKER))
#define W32_STREAM_EMPTY(stream)        ((W32_STREAM_IN(stream)   - W32_STREAM_OUT(stream)) == 0 )
#define W32_STREAM_FULL(stream)         ((W32_STREAM_SIZE(stream) - W32_STREAM_IN(stream)) == 0 )
// number of available words for extraction
#define W32_STREAM_FILLED(stream)       (W32_STREAM_IN(stream)    - W32_STREAM_OUT(stream))
// number of emptyu words in buffer
#define W32_STREAM_TOFILL(stream)       (W32_STREAM_SIZE(stream)  - W32_STREAM_IN(stream))
// est all fields to 0
#define W32_STREAM_INIT(stream)         (stream) = null_wordstream ; // nullify stream

// create a word stream
// stream [INOUT] : pointer to a wordstream struct
// mem       [IN] : memory address (if NULL, use malloc)
// nwds      [IN] : number of 32 bit words that the stream has to be able to contain
// in        [IN] : if mem is not NULL, set insertion index at position in (ignored otherwise)
// return the address of the wordstream buffer (NULL in case of error)
static uint32_t *w32_stream_create(wordstream *stream, void *mem, uint32_t nwds, uint32_t in){
  if(W32_STREAM_VALID(*stream)) return NULL ;   // existing valid stream
  W32_STREAM_INIT(*stream) ;
  W32_STREAM_BUFFER(*stream) = (mem == NULL) ? (uint32_t *) malloc(sizeof(uint32_t) * nwds) : mem ;
  if(W32_STREAM_BUFFER(*stream) != NULL) {
    stream->valid = W32_STREAM_MARKER ;                     // mark stream as valid
    stream->alloc = (mem == NULL) ? 1 : 0 ;                 // malleoc(ed) if mem == NULL
    if(mem != NULL) stream->in = (in < nwds) ? in : nwds ;  // stream already filled up to in
  }
  return W32_STREAM_BUFFER(*stream) ;
}

#define W32_STREAM_REREAD(stream)       (stream).out = 0
#define W32_STREAM_REWRITE(stream)      (stream).in  = 0
// set all indexes to start of buffer (reread + rewrite)
#define W32_STREAM_RESET(stream)        (stream).in  = (stream).out = 0

#define W32_STREAM_INSERT1(stream, word) { (stream).in = (word) ; (stream).in++ ; }
// insert multiple words into a word stream
// stream [INOUT] : pointer to a wordstream struct
// words     [IN] : pointer to data to be inserted (32 bit words)
// nwords    [IN] : number of 32 bit elements to insert into word stream
// return number of words inserted (-1 in case of error)
static int w32_stream_insert(wordstream *stream, void *words, uint32_t nwords){
  uint32_t *data = (uint32_t *) words ;
  int status = -1 ;
  if(stream->valid == W32_STREAM_MARKER) {         // check that stream is valid
    if(W32_STREAM_TOFILL(*stream) <= nwords){      // is enough space available in stream
      status = nwords ;
      while(nwords > 0){
        W32_STREAM_INSERT1(*stream, *data) ;
        nwords-- ; data++ ; W32_STREAM_IN(*stream)++ ;
      }
    }
  }
  return status ;
}
#define W32_STREAM_NEXT_VALUE(stream)  W32_STREAM_BUFFER(stream)[W32_STREAM_OUT(stream)]
// get next available 32 bit word, do not advance out index
// stream [IN] : pointer to a wordstream struct
// return zero if word stream is empty
static uint32_t w32_stream_peek(wordstream *stream){
  return (W32_STREAM_EMPTY(*stream)) ? 0 : W32_STREAM_NEXT_VALUE(*stream) ;
}
#define W32_STREAM_XTRACT1(stream, wout) { (wout) = (stream).out ; (stream).out++ ; }
// extract multiple words from a word stream
// stream [INOUT] : pointer to a wordstream struct
// words     [IN] : pointer to memory area to receive data (32 bit words)
// nwords    [IN] : number of 32 bit elements to extract from word stream
// return number of words extracted (-1 in case of error)
static int w32_stream_xtract(wordstream *stream, void *words, uint32_t nwords){
  uint32_t *data = (uint32_t *) words ;
  int status = -1 ;
  if(stream->valid == W32_STREAM_MARKER) {         // check that stream is valid
    if(W32_STREAM_FILLED(*stream) >= nwords){      // is enough data available in stream
      status = nwords ;
      while(nwords > 0){
        W32_STREAM_XTRACT1(*stream, *data) ;
        nwords-- ; data++ ; W32_STREAM_OUT(*stream)++ ;
      }
    }
  }
  return status ;
}
// resize the buffer of a wordstream
// stream [INOUT] : pointer to a valid wordstream struct
// return 0 if resize successful (or not needed)
// return -1 in case of resize error
static int w32_stream_resize(wordstream *stream, uint32_t size){
  int status = -1 ;
  if(stream->alloc == 1){       // check that buffer was "malloc(ed)" and can be "realloc(ed)"
    if(size > W32_STREAM_SIZE(*stream)){   // requested size > actual size, reallocate a bigger buffer
      size_t newsize = size * sizeof(uint32_t) ;
      void *ptr = realloc(stream->buf, newsize) ;
      if(ptr != NULL){
        status = 0 ;
        W32_STREAM_BUFFER(*stream) = (uint32_t *) ptr ;
        W32_STREAM_SIZE(*stream)   = size ;
      }
    }else{
      status = 0 ;              // size not increased, no need to reallocate
    }
  }
  return status ;
}

#endif
