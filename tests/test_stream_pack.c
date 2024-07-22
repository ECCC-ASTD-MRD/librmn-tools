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
// test the set of macros to manage insertion into/extraction from a 32 bit word based bit stream
//
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <rmn/stream_pack.h>

#define WITH_TIMING

#define NITER 100
#if defined(WITH_TIMING)
#include <rmn/timers.h>
#else
#define TIME_LOOP_DATA ;
#define TIME_LOOP_EZ(niter, npts, code) ;
  char *timer_msg = "" ;
  float NaNoSeC = 0.0f ;
#endif

// compare 2 sets of 32 bit words for identity
// wold [IN] : pointer to reference set
// wold [IN] : pointer to new set
// n    [IN] : number of 32 bit elements in each set
// return number of discrepancies
  static inline int w32_compare(void *wold, void *wnew, int n){
  uint32_t *old = (uint32_t *) wold ;
  uint32_t *new = (uint32_t *) wnew ;
  int errors = 0, i ;
  for(i=0 ; i<n ; i++){
    errors += (*old != *new) ? 1 : 0 ;
    if(errors == 1) fprintf(stderr, "at %d : expected %8.8x, got %8.8x\n", i, *old, *new) ;
  }
  return errors ;
}

// stream packer, pack the lower nbits bits of unp into pak
// unp   [IN] : pointer to array of 32 bit words to pack
// pak  [OUT] : packed bit stream
// nbits [IN] : number of rightmost bits to keep in unp
// n     [IN] : number of 32 bit elements in unp array
void *pack_w32(void *unp, void *pak, int nbits, int n){
  uint64_t accum ;
  uint32_t *unp_u = (uint32_t *) unp, *pak_u = (uint32_t *) pak, count ;
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
  return pak_u ;
}

// create stream32
// buf  [IN] : buffer address for stream32 (if NULL, it will be allocated internally)
// size [IN] : size of buffer
// return pointer to stream32 struct
stream32 *stream32_create(void *buf, uint32_t size){
  stream32 *s ;

  s = (stream32 *) malloc( sizeof(stream32) + ( (buf == NULL) ? (size * sizeof(uint32_t)) : 0 ) ) ;
  if(s == NULL) goto end ;

  s->first = (buf == NULL) ? &(s->data) : buf ;
  s->in = s->out = s->first ;
  s->limit = s->first + size ;
  s->acc_i = s->acc_x = 0 ;
  s->insert = s->xtract = 0 ;
  s->valid = 0xDEADBEEF ;
  s->spare = 0 ;
  s->alloc = (buf == NULL) ;
end:
  return s ;
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
// return pointer to the current insertion position into stream32 buffer
void *stream32_pack(stream32 *s, void *unp, int nbits, int n){
  uint32_t *unp_u = (uint32_t *) unp ;
  // get accum, packed stream pointer, count from stream
  uint64_t accum = s->acc_i ;
  uint32_t *pak_u = s->in, count = s->insert ;

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
//   BITSTREAM_PUT_START(*s) ;
//   if(nbits < 9){
//     while(n>3) {
//       BITSTREAM_PUT_FAST(*s, *unp_u, nbits) ; unp_u++ ;
//       BITSTREAM_PUT_FAST(*s, *unp_u, nbits) ; unp_u++ ;
//       BITSTREAM_PUT_FAST(*s, *unp_u, nbits) ; unp_u++ ;
//       BITSTREAM_PUT_FAST(*s, *unp_u, nbits) ; unp_u++ ;
//       BITSTREAM_PUT_CHECK(*s) ; n -= 4 ;
//     }
//   }
//   if(nbits < 17){
//     while(n>1) {
//       BITSTREAM_PUT_FAST(*s, *unp_u, nbits) ; unp_u++ ;
//       BITSTREAM_PUT_FAST(*s, *unp_u, nbits) ; unp_u++ ;
//       BITSTREAM_PUT_CHECK(*s) ; n -= 2 ;
//     }
//   }
//   while(n--){ BITSTREAM_PUT_SAFE(*s, *unp_u, nbits) ; unp_u++ ; }
//   BITSTREAM_PUT_END(*s) ;
  s->acc_i = accum ;
  s->in = pak_u ;
  s->insert = count ;
  return s->in ;
}

// unsigned unpacker, unpack into the lower nbits bits of unp
// unp  [OUT] : pointer to array of 32 bit words to receive unsigned unpacked data
// pak   [IN] : packed bit stream
// nbits [IN] : number of bits of packed items
// n     [IN] : number of 32 bit elements in unp array
// return pointer to the current extraction position
void *unpack_u32(void *unp, void *pak, int nbits, int n){
  uint64_t accum ;
  uint32_t *unp_u = (uint32_t *) unp, *pak_u = (uint32_t *) pak, count ;
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
  return pak_u ;
}

// signed unpacker, unpack into unp
// unp  [OUT] : pointer to array of 32 bit words to receive signed unpacked data
// pak   [IN] : packed bit stream
// nbits [IN] : number of bits of packed items
// n     [IN] : number of 32 bit elements in unp array
// return pointer to the current extraction position
void *unpack_i32(void *unp, void *pak, int nbits, int n){
  int64_t accum ;
  int32_t *unp_s = (int32_t *) unp ;
  uint32_t *pak_u = (uint32_t *) pak, count ;
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
  return pak_u ;
}

// rewind stream32, set extraction data pointer to beginning of data
// s [INOUT] : stream32 struct
void *stream32_rewind(stream32 *s){
  s->out = s->first ;
  s->acc_x = 0 ;
  s->xtract = 0 ;
  return s->out ;
}

// unsigned unpack
// s  [INOUT] : stream32 struct
// unp  [OUT] : pointer to array of 32 bit words to receive signed unpacked data
// nbits [IN] : number of bits of packed items
// n     [IN] : number of 32 bit elements in unp array
// return pointer to the current extraction position from stream32 buffer
void *stream32_unpack_u32(stream32 *s,void *unp,  int nbits, int n){
  uint32_t *unp_u = (uint32_t *) unp ;
  // get accum, packed stream pointer, count from stream
  uint64_t accum = s->acc_x ;
  uint32_t *pak_u = s->out, count = s->xtract ;

  BITS_GET_START(accum, count, pak_u) ;
  while(n--) { BITS_GET_SAFE(accum, count, *unp_u, nbits, pak_u) ; unp_u++ ; } ;
  if(count >= 32) pak_u-- ;
  s->out = pak_u ;
  s->acc_x = accum ;
  s->xtract = count ;
  return s->out ;
}

// signed unpack
// s  [INOUT] : stream32 struct
// unp  [OUT] : pointer to array of 32 bit words to receive signed unpacked data
// nbits [IN] : number of bits of packed items
// n     [IN] : number of 32 bit elements in unp array
// return pointer to the current extraction position from stream32 buffer
void *stream32_unpack_i32(stream32 *s,void *unp,  int nbits, int n){
  int32_t *unp_s = (int32_t *) unp ;
  // get accum, packed stream pointer, count from stream
  int64_t accum = s->acc_x ;
  uint32_t *pak_u = s->out, count = s->xtract ;

  BITS_GET_START(accum, count, pak_u) ;
  while(n--) { BITS_GET_SAFE(accum, count, *unp_s, nbits, pak_u) ; unp_s++ ; } ;
  if(count >= 32) pak_u-- ;
  s->out = pak_u ;
  s->acc_x = accum ;
  s->xtract = count ;
  return s->out ;
}

int main(int argc, char **argv){
  int npts = 32*1024 + 17 ;
  uint32_t *unpacked_u, *packed, *restored_u, *stream_pos1, *stream_pos2, *stream_pos3, *stream_pos4 ;
  int32_t  *unpacked_s, *restored_s ;
  int i, nbits, errors ;
  int32_t maskn ;
  TIME_LOOP_DATA ;
  float t0 , t1, t2, t3, t4, t5 ;
  stream32 *s ;

  fprintf(stderr, "test for stream packing macros\n") ;
  if(cycles_overhead > 0)fprintf(stderr, ", timing overhead = %4.2f ns\n", cycles_overhead * NaNoSeC);

  unpacked_u = (uint32_t *) malloc(npts * sizeof(uint32_t)) ; if(unpacked_u == NULL) exit(1) ;
  restored_u = (uint32_t *) malloc(npts * sizeof(uint32_t)) ; if(restored_u == NULL) exit(1) ;
  packed     = (uint32_t *) malloc(npts * sizeof(uint32_t)) ; if(packed == NULL) exit(1) ;
  unpacked_s = (int32_t *)  malloc(npts * sizeof(uint32_t)) ; if(unpacked_s == NULL) exit(1) ;
  restored_s = (int32_t *)  malloc(npts * sizeof(uint32_t)) ; if(restored_s == NULL) exit(1) ;

  s = stream32_create(NULL, npts) ;

  fprintf(stderr, "                                      macros                          stream\n") ;
  fprintf(stderr, "timings :                     pack unpack(u) unpack(s)      pack unpack(u) unpack(s)\n") ;
  for(nbits=1 ; nbits <33 ; nbits+=1) {

    maskn = ~((0xFFFFFFFFu) << nbits) ; maskn >>= 1 ;
    for(i=0 ; i<npts ; i+=1) { unpacked_u[i] = maskn & i ; } ;
    for(i=0 ; i<npts ; i+=2) { unpacked_u[i] |= (1 << (nbits-1)) ; } ;  // set high bit of every other value
    for(i=0 ; i<npts ; i++) {                                           // propagate high bit up to sign bit
      unpacked_s[i] = unpacked_u[i] << (32 - nbits) ; unpacked_s[i] >>= (32 - nbits) ;
    } ;
    fprintf(stderr, "nbits = %2d, mask = %8.8x", nbits, maskn) ;

    // unsigned pack/unpack base functions correctness test
    stream_pos1 = pack_w32(unpacked_u, packed, nbits, npts) ;
    stream_pos2 = unpack_u32(restored_u, packed, nbits, npts) ;
    errors = w32_compare(unpacked_u, restored_u, npts) ;                // check unsigned pack/unpack errors
    if(errors > 0) { fprintf(stderr, ", errors_u = %d\n", errors) ; exit(1) ; }
    if(stream_pos1 != stream_pos2) { fprintf(stderr, "\ninconsistent position after unpack\n") ; exit(1) ; }

    // signed pack/unpack base functions correctness test
    stream_pos3 = pack_w32(unpacked_s, packed, nbits, npts) ;
    if(stream_pos3 != stream_pos1) { fprintf(stderr, "\ninconsistent position after pack\n") ; exit(1) ; }
    stream_pos4 = unpack_i32(restored_s, packed, nbits, npts) ;
    errors = w32_compare(unpacked_s, restored_s, npts) ;                // check signed pack/unpack errors
    if(errors > 0) { fprintf(stderr, ", errors_u = %d\n", errors) ; exit(1) ; }
    if(stream_pos3 != stream_pos4) { fprintf(stderr, "\ninconsistent position after unpack\n") ; exit(1) ; }

    // unsigned pack/unpack stream functions correctness test
    stream32_rewrite(s) ;
    stream_pos3 = stream32_pack(s, unpacked_u, nbits, npts) ;
    if(stream_pos3 - s->first != stream_pos1 - packed) { fprintf(stderr, "\ninconsistent position after pack\n") ; exit(1) ; }
    stream32_rewind(s) ;
    stream_pos4 = stream32_unpack_u32(s,restored_u,  nbits, npts) ;
    if(stream_pos3 != stream_pos4) { fprintf(stderr, "\ninconsistent position after unpack\n") ; exit(1) ; }
    errors = w32_compare(unpacked_u, restored_u, npts) ;                // check unsigned pack/unpack errors
    if(errors > 0) { fprintf(stderr, ", errors_u stream = %d\n", errors) ; exit(1) ; }

    // signed pack/unpack stream functions correctness test
    stream32_rewrite(s) ;
    stream_pos3 = stream32_pack(s, unpacked_s, nbits, npts) ;
    if(stream_pos3 - s->first != stream_pos1 - packed) { fprintf(stderr, "\ninconsistent position after pack\n") ; exit(1) ; }
    stream32_rewind(s) ;
    stream_pos4 = stream32_unpack_i32(s,restored_s,  nbits, npts) ;
    if(stream_pos3 != stream_pos4) { fprintf(stderr, "\ninconsistent position after unpack\n") ; exit(1) ; }
    errors = w32_compare(unpacked_s, restored_s, npts) ;                // check unsigned pack/unpack errors
    if(errors > 0) { fprintf(stderr, ", errors_s stream = %d\n", errors) ; exit(1) ; }

    // base functions timing tests
    TIME_LOOP_EZ(NITER, npts, pack_w32(unpacked_u, packed, nbits, npts))
    t0 = timer_min * NaNoSeC / (npts) ;

    pack_w32(unpacked_u, packed, nbits, npts) ;
    TIME_LOOP_EZ(NITER, npts, unpack_u32(restored_u, packed, nbits, npts))
    t1 = timer_min * NaNoSeC / (npts) ;

    pack_w32(unpacked_s, packed, nbits, npts) ;
    TIME_LOOP_EZ(NITER, npts, unpack_i32(restored_s, packed, nbits, npts))
    t2 = timer_min * NaNoSeC / (npts) ;

    // stream32 functions timing tests
    stream32_pack(s, unpacked_u, nbits, npts) ;
    TIME_LOOP_EZ(NITER, npts, stream32_rewrite(s) ; stream32_pack(s, unpacked_u, nbits, npts) )
    t3 = timer_min * NaNoSeC / (npts) ;

    stream32_pack(s, unpacked_u, nbits, npts) ;
    TIME_LOOP_EZ(NITER, npts, stream32_rewind(s) ; stream32_unpack_u32(s,restored_u,  nbits, npts) )
    t4 = timer_min * NaNoSeC / (npts) ;

    stream32_pack(s, unpacked_s, nbits, npts) ;
    TIME_LOOP_EZ(NITER, npts, stream32_rewind(s) ; stream32_unpack_i32(s,restored_s,  nbits, npts) )
    t5 = timer_min * NaNoSeC / (npts) ;

    fprintf(stderr, ",  %4.2f      %4.2f      %4.2f      %4.2f      %4.2f      %4.2f\n", t0, t1, t2, t3, t4, t5) ;
    if(timer_min == timer_max) timer_avg = timer_min ;
  }

}
