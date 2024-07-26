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
#include <stdlib.h>

#include <rmn/stream_pack.h>

// =================== explicit array interface ===================
#define USE_STREAM32

#if defined(USE_STREAM32)
static stream32 make_stream(void *pak, int nbits, int n){
  stream32 s ;
  s.in = s.out = s.first = pak ;
  s.limit   = s.first + (n * nbits + 31) / 32 ;
  s.marker  = STREAM32_MARKER ;
  s.version = STREAM32_VERSION ;
  return s ;
}
#endif

// stream packer, pack the lower nbits bits of unp into pak
// unp   [IN] : pointer to array of 32 bit words to pack
// pak  [OUT] : packed bit stream
// nbits [IN] : number of rightmost bits to keep in unp
// n     [IN] : number of 32 bit elements in unp array
// return number of 32 bit words used
#if defined(USE_STREAM32)
uint32_t pack_w32(void *unp, void *pak, int nbits, int n){
  stream32 s = make_stream(pak, nbits, n) ;
  return stream32_pack(&s, unp, nbits, n, 0) ;
}
#else
uint32_t pack_w32(void *unp, void *pak, int nbits, int n){
  uint64_t accum ;
  uint32_t *unp_u = (uint32_t *) unp, *pak_u = (uint32_t *) pak, count ;
  uint32_t *pak_0 = pak_u ;
  BITS_PUT_INIT(accum, count) ;
  if(nbits < 9){
    while(n>3) {
      BITS_PUT_FAST(accum, count, *unp_u, nbits) ; unp_u++ ;
      BITS_PUT_FAST(accum, count, *unp_u, nbits) ; unp_u++ ;
      BITS_PUT_FAST(accum, count, *unp_u, nbits) ; unp_u++ ;
      BITS_PUT_FAST(accum, count, *unp_u, nbits) ; unp_u++ ;
      BITS_PUT_CHECK(accum, count, pak_u) ; n -= 4 ;
    }
  }
  if(nbits < 17){
    while(n>1) {
      BITS_PUT_FAST(accum, count, *unp_u, nbits) ; unp_u++ ;
      BITS_PUT_FAST(accum, count, *unp_u, nbits) ; unp_u++ ;
      BITS_PUT_CHECK(accum, count, pak_u) ; n -= 2 ;
    }
  }
  while(n--){ BITS_PUT_SAFE(accum, count, *unp_u, nbits, pak_u) ; unp_u++ ; }
  BITS_PUT_FLUSH(accum, count, pak_u) ;
  return pak_u - pak_0 ;
}
#endif

// unsigned unpacker, unpack into the lower nbits bits of unp
// unp  [OUT] : pointer to array of 32 bit words to receive unsigned unpacked data
// pak   [IN] : packed bit stream
// nbits [IN] : number of bits of packed items
// n     [IN] : number of 32 bit elements in unp array
// return index to the current extraction position
#if defined(USE_STREAM32)
uint32_t unpack_u32(void *unp, void *pak, int nbits, int n){
  stream32 s = make_stream(pak, nbits, n) ;
  return stream32_unpack_u32(&s, unp, nbits, n, 0) ;
}
#else
uint32_t unpack_u32(void *unp, void *pak, int nbits, int n){
  uint64_t accum ;
  uint32_t *unp_u = (uint32_t *) unp, *pak_u = (uint32_t *) pak, count ;
  uint32_t *pak_0 = pak_u ;

  BITS_GET_INIT(accum, count, pak_u) ;
  if(nbits < 9){
    while(n>3) {         // 8 bits or less, safe to get 4 elements back to back
      BITS_GET_CHECK(accum, count, pak_u) ; n -= 4 ;
      BITS_GET_FAST(accum, count, *unp_u, nbits) ; unp_u++ ;
      BITS_GET_FAST(accum, count, *unp_u, nbits) ; unp_u++ ;
      BITS_GET_FAST(accum, count, *unp_u, nbits) ; unp_u++ ;
      BITS_GET_FAST(accum, count, *unp_u, nbits) ; unp_u++ ;
    }
  }
  if(nbits < 17){        // 16 bits or less, safe to get 2 elements back to back
    while(n>1) {
      BITS_GET_CHECK(accum, count, pak_u) ; n -= 2 ;
      BITS_GET_FAST(accum, count, *unp_u, nbits) ; unp_u++ ;
      BITS_GET_FAST(accum, count, *unp_u, nbits) ; unp_u++ ;
    }
  }
  while(n--) { BITS_GET_SAFE(accum, count, *unp_u, nbits, pak_u) ; unp_u++ ; } ;
  if(count >= 32) pak_u-- ;
  return pak_u - pak_0 ;
}
#endif

// signed unpacker, unpack into unp
// unp  [OUT] : pointer to array of 32 bit words to receive signed unpacked data
// pak   [IN] : packed bit stream
// nbits [IN] : number of bits of packed items
// n     [IN] : number of 32 bit elements in unp array
// return index to the current extraction position
#if defined(USE_STREAM32)
uint32_t unpack_i32(void *unp, void *pak, int nbits, int n){
  stream32 s = make_stream(pak, nbits, n) ;
  return stream32_unpack_i32(&s, unp, nbits, n, 0) ;
}
#else
uint32_t unpack_i32(void *unp, void *pak, int nbits, int n){
  uint64_t accum ;
  int32_t *unp_s = (int32_t *) unp ;
  uint32_t *pak_u = (uint32_t *) pak, count ;
  uint32_t *pak_0 = pak_u ;

  BITS_GET_INIT(accum, count, pak_u) ;
  if(nbits < 9){         // 8 bits or less, safe to get 4 elements back to back
    while(n>3) {
      BITS_GET_CHECK(accum, count, pak_u) ; n -= 4 ;
      BITS_GET_FAST(accum, count, *unp_s, nbits) ; unp_s++ ;
      BITS_GET_FAST(accum, count, *unp_s, nbits) ; unp_s++ ;
      BITS_GET_FAST(accum, count, *unp_s, nbits) ; unp_s++ ;
      BITS_GET_FAST(accum, count, *unp_s, nbits) ; unp_s++ ;
    }
  }
  if(nbits < 17){        // 16 bits or less, safe to get 2 elements back to back
    while(n>1) {
      BITS_GET_CHECK(accum, count, pak_u) ; n -= 2 ;
      BITS_GET_FAST(accum, count, *unp_s, nbits) ; unp_s++ ;
      BITS_GET_FAST(accum, count, *unp_s, nbits) ; unp_s++ ;
    }
  }
  while(n--) { BITS_GET_SAFE(accum, count, *unp_s, nbits, pak_u) ; unp_s++ ; } ;
  if(count >= 32) pak_u-- ;
  return pak_u - pak_0 ;
}
#endif

// =================== stream32 interface ===================
#include <stdio.h>

// create stream32
// s    [IN] : pointer to an existing stream32 struct
//             if s is NULL, buf is ignored, s and buf are allocated internally
// buf  [IN] : buffer address for stream32 (if NULL, it will be allocated internally)
// size [IN] : size of buffer in 32 bit units (
// return pointer to stream32 struct (NULL in case of error)
stream32 *stream32_create(stream32 *s, void *buf, uint32_t size){

  if(s == NULL){        // new stream2 struct, ignore buf
    s = (stream32 *) malloc( sizeof(stream32) +  (size * sizeof(uint32_t)) ) ;  // struct + buffer
    if(s == NULL) return NULL ;
    s->first = (uint32_t *)(s + 1) ;   // buffer is located just after the struct itself
    s->alloc = 1 ;                     // whole struct + buffer may be free(d) or realloc(ed)
    s->ualloc = 0 ;                    // buffer may not be free(d) or realloc(ed)
  }else{                // existing stream2 struct
    s->first = (buf != NULL) ? buf : (uint32_t *) malloc( size * sizeof(uint32_t) ) ;
    if(s->first == NULL) return NULL ;
    s->alloc = 0 ;                     // whole struct not to be free(d) or realloc(ed)
    s->ualloc = (buf == NULL) ;        // buffer may be free(d) or realloc(ed)
  }
  s->in = s->out = s->first ;
  s->limit   = s->first + size ;
  s->acc_i   = s->acc_x = 0 ;
  s->insert  = s->xtract = 0 ;
  s->marker  = STREAM32_MARKER ;
  s->version = STREAM32_VERSION ;
  s->spare   = 0 ;

  return s ;
}

// free memory associated with stream
// s [INOUT] : stream32 struct
// return 0 in case of error, 1 or 2 for full / partial free
// if nothing can be freed, it is considered an error
int stream32_free(stream32 *s){
  if(s == NULL) return 0 ;
  if( ! BITSTREAM_VALID(*s) ) return 0 ;
  if(s->alloc == 1){               // whole struct was malloc(ed)
    free(s) ;
    return 1 ;
  }else if(s->ualloc == 1){        // buffer was malloc(ed)
    free(s->first) ;
    s->first  = NULL ;
    s->ualloc = 0 ;                // nothing to free any more
    return 2 ;
  }
  return 0 ;
}

// resize an existing stream
// if size is smaller than or equal to the actual size of the stream32 buffer, nothing is done
// if the alloc flag is false, nothing is done
// size [IN] : size of buffer in 32 bit units
// return pointer to new stream32 struct (NULL in case of error)
stream32 *stream32_resize(stream32 *s, uint32_t size){
  uint64_t in, out ;

  if(s == NULL) return NULL ;
  if( ! BITSTREAM_VALID(*s) ) return NULL ;
  if(s->limit - s->first < size){  // current size is smaller then requested size
    in  = s->in  - s->first ;      // in index from pointer
    out = s->out - s->first ;      // out index from pointer
    if(s->alloc) {                 // the whole stream32 struc was malloc(ed)
      s = (stream32 *) realloc(s, sizeof(stream32) + size * sizeof(uint32_t)) ;
      s->first = (uint32_t *)(s + 1) ;
    }else if(s->ualloc) {          // only buffer was malloc(ed)
      s->first = (uint32_t *) realloc(s->first, size * sizeof(uint32_t)) ;
    }else{
      return NULL ;                // no way to realloc anything
    }
    s->limit = s->first + size ;   // new limit
    s->in    = s->first + in ;     // in pointer from index
    s->out   = s->first + out ;    // out pointer from index
  }  // requested size larger that old size
  return s ;
}

// rewind stream32, set extraction data pointer to beginning of data
// s [INOUT] : stream32 struct
// return pointer to extraction point
void *stream32_rewind(stream32 *s){
  if(s == NULL) return NULL ;
  if( ! BITSTREAM_VALID(*s) ) return NULL ;
  s->out = s->first ;
  s->acc_x = 0 ;
  s->xtract = 0 ;
  return s->out ;
}

// rewrite stream32, set insertion data pointer to beginning of data
// s [INOUT] : stream32 struct
// return pointer to insertion point
void *stream32_rewrite(stream32 *s){
  if(s == NULL) return NULL ;
  if( ! BITSTREAM_VALID(*s) ) return NULL ;
  s->in = s->first ;
  s->acc_i = 0 ;
  s->insert = 0 ;
  return s->in ;
}

// stream packer, pack the lower nbits bits of unp into stream32
// s    [INOUT] : stream32 struct
// unp     [IN] : pointer to array of 32 bit words to pack
// nbits   [IN] : number of rightmost bits to keep in unp
// n       [IN] : number of 32 bit elements in unp array
// options [IN] : packing options (PUT_NO_INIT / PUT_NO_FLUSH)
// return index of the current insertion position into stream32 buffer
uint32_t stream32_pack(stream32 *s, void *unp, int nbits, int n, uint32_t options){
  uint32_t *unp_u = (uint32_t *) unp ;
  if(s == NULL) return 0xFFFFFFFF ;
  if( ! BITSTREAM_VALID(*s) ) return 0xFFFFFFFF ;
  // get accum, packed stream pointer, count from stream
  EZSTREAM_DCL_IN ;      // EZ declarations for insertion
  EZSTREAM_GET_IN(*s) ;  // get info from stream

  if((PUT_NO_INIT & options) == 0) EZSTREAM_PUT_INIT ;
  if(nbits < 9){         //  8 bits or less, safe to put 4 elements back to back
    while(n>3) {
      EZSTREAM_PUT_CHECK ; n -= 4 ;
      EZSTREAM_PUT_FAST(*unp_u, nbits) ; unp_u++ ;
      EZSTREAM_PUT_FAST(*unp_u, nbits) ; unp_u++ ;
      EZSTREAM_PUT_FAST(*unp_u, nbits) ; unp_u++ ;
      EZSTREAM_PUT_FAST(*unp_u, nbits) ; unp_u++ ;
    }
  }
  if(nbits < 17){        // 16 bits or less, safe to put 2 elements back to back
    while(n>1) {
      EZSTREAM_PUT_CHECK ; n -= 2 ;
      EZSTREAM_PUT_FAST(*unp_u, nbits) ; unp_u++ ;
      EZSTREAM_PUT_FAST(*unp_u, nbits) ; unp_u++ ;
    }
  }
  while(n--){ EZSTREAM_PUT_SAFE(*unp_u, nbits) ; unp_u++ ; }
  if((PUT_NO_FLUSH & options) == 0) EZSTREAM_PUT_FLUSH ;
  EZSTREAM_SAVE_IN(*s) ;  // save updated info into stream
  return EZSTREAM_IN(*s) ;
}

// unsigned unpack
// s    [INOUT]   : stream32 struct
// unp    [OUT]   : pointer to array of 32 bit words to receive signed unpacked data
// nbits   [IN]   : number of bits of packed items
// n       [IN]   : number of 32 bit elements in unp array
// options [IN] : unpacking options (GET_NO_INIT / GET_NO_FINALIZE)
// return index of the current extraction position from stream32 buffer
uint32_t stream32_unpack_u32(stream32 *s,void *unp,  int nbits, int n, uint32_t options){
  uint32_t *unp_u = (uint32_t *) unp ;
  if(s == NULL) return 0xFFFFFFFF ;
  if( ! BITSTREAM_VALID(*s) ) return 0xFFFFFFFF ;
  // get accum, packed stream pointer, count from stream
  EZSTREAM_DCL_OUT ;      // EZ declarations for extraction
  EZSTREAM_GET_OUT(*s) ;  // get info from stream

  if((GET_NO_INIT & options) == 0) EZSTREAM_GET_INIT ;
  if(nbits < 9){
    while(n>3) {         // 8 bits or less, safe to get 4 elements back to back
      EZSTREAM_GET_CHECK ; n -= 4 ;
      EZSTREAM_GET_FAST(*unp_u, nbits) ; unp_u++ ;
      EZSTREAM_GET_FAST(*unp_u, nbits) ; unp_u++ ;
      EZSTREAM_GET_FAST(*unp_u, nbits) ; unp_u++ ;
      EZSTREAM_GET_FAST(*unp_u, nbits) ; unp_u++ ;
    }
  }
  if(nbits < 17){        // 16 bits or less, safe to get 2 elements back to back
    while(n>1) {
      EZSTREAM_GET_CHECK ; n -= 2 ;
      EZSTREAM_GET_FAST(*unp_u, nbits) ; unp_u++ ;
      EZSTREAM_GET_FAST(*unp_u, nbits) ; unp_u++ ;
    }
  }
  while(n--) { EZSTREAM_GET_SAFE(*unp_u, nbits) ; unp_u++ ; } ;
  if((GET_NO_FINALIZE & options) == 0) EZSTREAM_GET_FINALIZE ;
  EZSTREAM_SAVE_OUT(*s) ;  // save updated info into stream
  return EZSTREAM_OUT(*s) ;
}

// signed unpack
// s    [INOUT] : stream32 struct
// unp    [OUT] : pointer to array of 32 bit words to receive signed unpacked data
// nbits   [IN] : number of bits of packed items
// n       [IN] : number of 32 bit elements in unp array
// options [IN] : unpacking options (GET_NO_INIT / GET_NO_FINALIZE)
// return index of the current extraction position from stream32 buffer
uint32_t stream32_unpack_i32(stream32 *s,void *unp,  int nbits, int n, uint32_t options){
  int32_t *unp_s = (int32_t *) unp ;
  if(s == NULL) return 0xFFFFFFFF ;
  if( ! BITSTREAM_VALID(*s) ) return 0xFFFFFFFF ;
  // get accum, packed stream pointer, count from stream
  EZSTREAM_DCL_OUT ;      // EZ declarations for extraction
  EZSTREAM_GET_OUT(*s) ;  // get info from stream

  if((GET_NO_INIT & options) == 0) EZSTREAM_GET_INIT ;
  if(nbits < 9){
    while(n>3) {         // 8 bits or less, safe to get 4 elements back to back
      EZSTREAM_GET_CHECK ; n -= 4 ;
      EZSTREAM_GET_FAST(*unp_s, nbits) ; unp_s++ ;
      EZSTREAM_GET_FAST(*unp_s, nbits) ; unp_s++ ;
      EZSTREAM_GET_FAST(*unp_s, nbits) ; unp_s++ ;
      EZSTREAM_GET_FAST(*unp_s, nbits) ; unp_s++ ;
    }
  }
  if(nbits < 17){        // 16 bits or less, safe to get 2 elements back to back
    while(n>1) {
      EZSTREAM_GET_CHECK ; n -= 2 ;
      EZSTREAM_GET_FAST(*unp_s, nbits) ; unp_s++ ;
      EZSTREAM_GET_FAST(*unp_s, nbits) ; unp_s++ ;
    }
  }
  while(n--) { EZSTREAM_GET_SAFE(*unp_s, nbits) ; unp_s++ ; } ;
  if((GET_NO_FINALIZE & options) == 0) EZSTREAM_GET_FINALIZE ;
  EZSTREAM_SAVE_OUT(*s) ;  // save updated info into stream
  return EZSTREAM_OUT(*s) ;
}
