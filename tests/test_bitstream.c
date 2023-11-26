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
//     M. Valin,   Recherche en Prevision Numerique, 2023
//
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>

#include <rmn/ct_assert.h>
#include <rmn/timers.h>
#include <rmn/misc_operators.h>
#include <rmn/bi_endian_pack.h>
#include <rmn/tee_print.h>
#include <rmn/test_helpers.h>

#define NSTREAMS 16
#define STREAM0 streams[0]
#define STREAM1 streams[1]

// one liner printout of stream information
static void print_stream_params(bitstream s, char *msg, char *expected_mode){
  int32_t available = StreamAvailableBits(&s) ;
  int32_t strict_available = StreamStrictAvailableBits(&s) ;
  int32_t space_available = StreamAvailableSpace(&s) ;
  available = (available < 0) ? 0 : available ;
  strict_available = (strict_available < 0) ? 0 : strict_available ;
  TEE_FPRINTF(stderr,2, "%s: bits used = %d(%d), unused = %d, first/in/out/limit = %p/%ld/%ld/%ld [%ld], insert/xtract = %d/%d",
    msg, available, strict_available, space_available, 
    s.first, s.in-s.first, s.out-s.first, s.limit-s.first, s.in-s.out, s.insert, s.xtract ) ;
  TEE_FPRINTF(stderr,2, ", full/part/user = %d/%d/%d", s.full, s.part, s.user) ;

  if(expected_mode){
    TEE_FPRINTF(stderr,2, ", mode = %s(%d) (%s expected)\n", StreamMode(s), StreamModeCode(s), expected_mode) ;
    if(strcmp(StreamMode(s), expected_mode) != 0) { 
      TEE_FPRINTF(stderr,2, "Bad mode, exiting\n") ;
      exit(1) ;
    }
  }else{
    TEE_FPRINTF(stderr,2, ", mode = %s(%d)\n", StreamMode(s), StreamModeCode(s)) ;
  }
}

 /*
 testing functions and macros:
 - stream initialization (new buffer)
 - stream initialization (existing buffer)
 - stream resizing
 - bit insertion
 - bit extraction
 - insertion accumulator push and flush
 - get/set insertion/extraction control variables
 - set number of bits filled in stream
 - copy of stream buffer to user buffer
*/
int main(int argc, char **argv){
  bitstream streams[NSTREAMS] ;
  bitstream *pstream ;
  bitstream_state states[NSTREAMS];
  bitstream_state *state = &(states[0]) ;
  int i0, i, j0, j, nbits, value, ntot, errors ;
  uint32_t w32 ;
  size_t ssize = 128 ;
  int64_t bsize ;
  uint32_t buffer[4096] ;

  start_of_test("basic test of bit stream functions and macros") ;

  pstream = &(STREAM0) ;
  BeStreamInit(pstream, NULL, ssize, 0) ;                  // create a bit stream (1024 bits initially)
  print_stream_params(STREAM0, "1024 bit stream", NULL) ;

  STREAM_DCL_STATE_VARS(acci, insert, in) ;                // declare insertion control values from stream struct
  STREAM_GET_INSERT_STATE(*pstream, acci, insert, in) ;    // get insertion control values from stream struct

  ntot = 0 ;
  for(i0 = 0 ; i0 < 48 ; i0 += 12){
    for(i=0 ; i<12 ; i++){
      value = (i & 1) ? -(i0+i) : i0+i ;
      w32 = to_zigzag_32(value) ;
      nbits = 32 - lzcnt_32(w32) ;
      ntot += nbits ;
      BE64_PUT_NBITS(acci, insert, w32, nbits, in) ;         // insert nbits into stream
      TEE_FPRINTF(stderr,2, ".");
    }
    BE64_PUSH(acci, insert, in) ;                            // push accumulator contents into buffer
    STREAM_SET_INSERT_STATE(*pstream, acci, insert, in) ;    // update insertion control values in stream struct
    TEE_FPRINTF(stderr,2, " inserted %d bits, ", ntot) ;
    print_stream_params(*pstream, "after insertion", NULL) ;
    ssize += 128 ;                                           // add 1024 bits to stream size
    StreamResize(pstream, NULL, ssize) ;                     // resize stream
    STREAM_GET_INSERT_STATE(*pstream, acci, insert, in) ;    // get updated insertion control values from stream struct
    TEE_FPRINTF(stderr,2, "(buffer size + 128 bytes) ") ;
    print_stream_params(*pstream, "after resize", NULL) ;
  }
  print_stream_params(*pstream, "before insertions finalize", NULL) ;
#if 0
  BE64_STREAM_INSERT_FINAL(*pstream) ;
#else
  BE64_INSERT_FINAL(acci, insert, in) ;                      // flush accumulator into stream
  STREAM_SET_INSERT_STATE(*pstream, acci, insert, in) ;      // store insertion control values into stream struct
#endif
  print_stream_params(*pstream, "after insertions finalize", NULL) ;
  TEE_FPRINTF(stderr,2, "\n") ;

  bsize = StreamCopy(pstream, buffer, sizeof(buffer)) ;      // copy original stream data into local buffer
  TEE_FPRINTF(stderr,2, "copied %ld bits from original stream\n", bsize) ;
  TEE_FPRINTF(stderr,2, "\n") ;
  BeStreamInit(&(STREAM1), buffer, sizeof(buffer), BIT_XTRACT) ;      // initialize new stream using local buffer
  pstream = &(STREAM1) ;
  StreamSetFilledBits(pstream, ntot) ;                       // set number of valid bits in stream
  print_stream_params(*pstream, "before extraction", NULL) ;
  STREAM_DCL_STATE_VARS(acco, xtract, out) ;                 // declare insertion control values from stream struct
  STREAM_GET_XTRACT_STATE(*pstream, acco, xtract, out) ;     // get extraction control values from stream struct
  ntot = 0 ;
  errors = 0 ;
  for(i0 = 0 ; i0 < 48 ; i0 += 12){
    for(i=0 ; i<12 ; i++){
      value = (i & 1) ? -(i0+i) : i0+i ;
      w32 = to_zigzag_32(value) ;
      nbits = 32 - lzcnt_32(w32) ;
      ntot += nbits ;
      BE64_GET_NBITS(acco, xtract, w32, nbits, out) ;        // get nbits from stream
      if(value != from_zigzag_32(w32)) errors++ ;            // check that we get the expected value
      TEE_FPRINTF(stderr,2, ".");
    }
    STREAM_SET_XTRACT_STATE(*pstream, acco, xtract, out) ;   // update extraction control values into stream struct
    TEE_FPRINTF(stderr,2, " extracted %d bits, ", ntot) ;
    print_stream_params(*pstream, "after extraction", NULL) ;
  }
  print_stream_params(*pstream, "after all extractions", NULL) ;
  TEE_FPRINTF(stderr,2, "extraction errors = %d\n", errors) ;

  return 0 ;
}
