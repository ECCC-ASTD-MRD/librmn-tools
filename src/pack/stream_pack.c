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

// stream packer, pack the lower nbits bits of unp into pak
// unp   [IN] : pointer to array of 32 bit words to pack
// pak  [OUT] : packed bit stream
// nbits [IN] : number of rightmost bits to keep in unp
// n     [IN] : number of 32 bit elements in unp array
// return number of 32 bit words used
#if 1
uint32_t pack_w32(void *unp, void *pak, int nbits, int nn){
  int n = (nn < 0) ? -nn : nn ;
  stream32 s ;
  s.in = s.first = pak ;
  s.limit = s.first + (n * nbits + 31) / 32 ;
  return stream32_pack(&s, unp, nbits, n) ;
}
#else
uint32_t pack_w32(void *unp, void *pak, int nbits, int nn){
  int n = (nn < 0) ? -nn : nn ;
  uint64_t accum ;
  uint32_t *unp_u = (uint32_t *) unp, *pak_u = (uint32_t *) pak, count ;
  uint32_t *pak_0 = pak_u ;
  BITS_PUT_START(accum, count) ;
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
  BITS_PUT_END(accum, count, pak_u) ;
  return pak_u - pak_0 ;
}
#endif

// unsigned unpacker, unpack into the lower nbits bits of unp
// unp  [OUT] : pointer to array of 32 bit words to receive unsigned unpacked data
// pak   [IN] : packed bit stream
// nbits [IN] : number of bits of packed items
// n     [IN] : number of 32 bit elements in unp array
// return index to the current extraction position
#if 1
uint32_t unpack_u32(void *unp, void *pak, int nbits, int nn){
  int n = (nn < 0) ? -nn : nn ;
  stream32 s ;
  s.out = s.first = pak ;
  s.limit = s.first + (n * nbits + 31) / 32 ;
  return stream32_unpack_u32(&s, unp, nbits, n) ;
}
#else
uint32_t unpack_u32(void *unp, void *pak, int nbits, int nn){
  int n = (nn < 0) ? -nn : nn ;
  uint64_t accum ;
  uint32_t *unp_u = (uint32_t *) unp, *pak_u = (uint32_t *) pak, count ;
  uint32_t *pak_0 = pak_u ;

  BITS_GET_START(accum, count, pak_u) ;
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
#if 1
uint32_t unpack_i32(void *unp, void *pak, int nbits, int nn){
  int n = (nn < 0) ? -nn : nn ;
  stream32 s ;
  s.out = s.first = pak ;
  s.limit = s.first + (n * nbits + 31) / 32 ;
  return stream32_unpack_i32(&s, unp, nbits, n) ;
}
#else
uint32_t unpack_i32(void *unp, void *pak, int nbits, int nn){
  int n = (nn < 0) ? -nn : nn ;
  uint64_t accum ;
  int32_t *unp_s = (int32_t *) unp ;
  uint32_t *pak_u = (uint32_t *) pak, count ;
  uint32_t *pak_0 = pak_u ;

  BITS_GET_START(accum, count, pak_u) ;
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

// create stream32
// buf  [IN] : buffer address for stream32 (if NULL, it will be allocated internally)
// size [IN] : size of buffer in 32 bit units (
//             negative size means use buffer can be free(d)/realloc(ed)
// return pointer to stream32 struct
stream32 *stream32_create(void *buf, uint32_t size){
  stream32 *s ;
  size_t sz = (size < 0) ? -size : size ;

  s = (stream32 *) malloc( sizeof(stream32) + ( (buf == NULL) ? (sz * sizeof(uint32_t)) : 0 ) ) ;
  if(s == NULL) goto end ;

  s->first = (buf == NULL) ? &(s->data) : buf ;
  s->in = s->out = s->first ;
  s->limit = s->first + sz ;
  s->acc_i  = s->acc_x = 0 ;
  s->insert = s->xtract = 0 ;
  s->valid  = 0xDEADBEEF ;
  s->spare  = 0 ;
  s->ualloc = ((size < 0) && (buf != NULL)) ;
  s->alloc  = (buf == NULL) ;
end:
  return s ;
}
#include <stdio.h>
// resize an existing stream
// if size is smaller than or equal to the actual size of the stream32 buffer, nothing is done
// if the alloc flag is false, nothing is done
// size [IN] : size of buffer in 32 bit units
// return pointer to new stream32 struct
stream32 *stream32_resize(stream32 *s_old, uint32_t size){
  stream32 *s_new = s_old ;
  uint64_t in, out ;
  size_t sz = (size < 0) ? -size : size ;
// fprintf(stderr, "resize to %ld %d %d", sz, s_old->alloc, s_old->ualloc) ;
  if(s_old->limit - s_old->first < sz){
    in  = s_old->in  - s_old->first ;      // in index from pointer
    out = s_old->out - s_old->first ;      // out index from pointer
    if(s_old->alloc) {                     // the whole stream32 struc was malloc(ed)
      s_new = (stream32 *) realloc(s_old, sizeof(stream32) + sz * sizeof(uint32_t)) ;
      s_new->first = &(s_new->data[0]) ;
      s_new->limit = s_new->first + sz ;   // new limit
      s_new->in    = s_new->first + in ;   // in pointer from index
      s_new->out   = s_new->first + out ;  // out pointer from index
    }else if(s_old->ualloc) {              // only buffer was malloc(ed)
      s_new->first = (uint32_t *) realloc(s_new->first, sz * sizeof(uint32_t)) ;
      s_new->limit = s_new->first + sz ;   // new limit
      s_new->in    = s_new->first + in ;   // in pointer from index
      s_new->out   = s_new->first + out ;  // out pointer from index
    }
  }  // sz larger that old size
  return s_new ;
}

// rewind stream32, set extraction data pointer to beginning of data
// s [INOUT] : stream32 struct
void *stream32_rewind(stream32 *s){
  s->out = s->first ;
  s->acc_x = 0 ;
  s->xtract = 0 ;
  return s->out ;
}

// rewrite stream32, set insertion data pointer to beginning of data
// s [INOUT] : stream32 struct
void *stream32_rewrite(stream32 *s){
  s->in = s->first ;
  s->acc_i = 0 ;
  s->insert = 0 ;
  return s->in ;
}

// stream packer, pack the lower nbits bits of unp into stream32
// s  [INOUT] : stream32 struct
// unp   [IN] : pointer to array of 32 bit words to pack
// nbits [IN] : number of rightmost bits to keep in unp
// n     [IN] : number of 32 bit elements in unp array
//              if n < 0 , do not initialize/finalize packed stream
// return index to the current insertion position into stream32 buffer
uint32_t stream32_pack(stream32 *s, void *unp, int nbits, int nn){
  int n = (nn < 0) ? -nn : nn ;
  uint32_t *unp_u = (uint32_t *) unp ;
  // get accum, packed stream pointer, count from stream
  EZSTREAM_DCL_IN ;      // EZ declarations for insertion
  EZSTREAM_GET_IN(*s) ;  // get info from stream

  if(nn >= 0) EZSTREAM_PUT_START ;
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
  if(nn >= 0) EZSTREAM_PUT_END ;
  EZSTREAM_SAVE_IN(*s) ;  // save updated info into stream
  return EZSTREAM_IN(*s) ;
}

// unsigned unpack
// s  [INOUT] : stream32 struct
// unp  [OUT] : pointer to array of 32 bit words to receive signed unpacked data
// nbits [IN] : number of bits of packed items
// n     [IN] : number of 32 bit elements in unp array
//              if n < 0 , do not initialize stream before extraction
// return pointer to the current extraction position from stream32 buffer
uint32_t stream32_unpack_u32(stream32 *s,void *unp,  int nbits, int nn){
  int n = (nn < 0) ? -nn : nn ;
  uint32_t *unp_u = (uint32_t *) unp ;
  // get accum, packed stream pointer, count from stream
  EZSTREAM_DCL_OUT ;      // EZ declarations for extraction
  EZSTREAM_GET_OUT(*s) ;  // get info from stream

  if(nn >= 0) EZSTREAM_GET_START ;
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
  EZSTREAM_SAVE_OUT(*s) ;  // save updated info into stream
  return EZSTREAM_OUT(*s) ;
}

// signed unpack
// s  [INOUT] : stream32 struct
// unp  [OUT] : pointer to array of 32 bit words to receive signed unpacked data
// nbits [IN] : number of bits of packed items
// n     [IN] : number of 32 bit elements in unp array
//              if n < 0 , do not initialize stream before extraction
// return pointer to the current extraction position from stream32 buffer
uint32_t stream32_unpack_i32(stream32 *s,void *unp,  int nbits, int nn){
  int n = (nn < 0) ? -nn : nn ;
  int32_t *unp_s = (int32_t *) unp ;
  // get accum, packed stream pointer, count from stream
  EZSTREAM_DCL_OUT ;      // EZ declarations for extraction
  EZSTREAM_GET_OUT(*s) ;  // get info from stream

  if(nn >= 0) EZSTREAM_GET_START ;
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
  EZSTREAM_SAVE_OUT(*s) ;  // save updated info into stream
  return EZSTREAM_OUT(*s) ;
}
