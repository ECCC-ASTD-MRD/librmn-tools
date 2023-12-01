//
// Copyright (C) 2023  Environnement Canada
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

#include <rmn/misc_operators.h>
#include <rmn/bi_endian_pack.h>
#include <rmn/print_bitstream.h>
#include <rmn/tee_print.h>
#include <rmn/test_helpers.h>

#define NSTREAMS 16
#define STREAM0 streams[0]
#define STREAM1 streams[1]

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
  size_t ssize ;
  int64_t bsize ;
  uint32_t buffer[4096] ;

  start_of_test("basic test of bit stream functions and macros") ;

  pstream = &(STREAM0) ;
  ssize = 128 ;                                              // 128 bytes, 1024 bits
  BeStreamInit(pstream, NULL, ssize, 0) ;                    // create a bit stream (1024 bits initially)
  print_stream_params(STREAM0, "1024 bit stream", NULL) ;

  EZ_NEW_INSERT_VARS(*pstream) ;                             // declare and get insertion control values from stream

  ntot = 0 ;
  for(i0 = 0 ; i0 < 48 ; i0 += 12){
    for(i=0 ; i<12 ; i++){
      value = (i & 1) ? -(i0+i) : i0+i ;
      w32 = to_zigzag_32(value) ;
      nbits = 32 - lzcnt_32(w32) ;
      ntot += nbits ;
      BE64_EZ_PUT_NBITS(w32, nbits) ;
      TEE_FPRINTF(stderr,2, ".");
    }
    BE64_EZ_PUSH ;                                           // push accumulator contents into buffer
    EZ_SET_INSERT_VARS(*pstream) ;                           // update insertion control values in stream struct
    TEE_FPRINTF(stderr,2, " inserted %d bits, ", ntot) ;
    print_stream_params(*pstream, "after insertion", NULL) ;
    ssize += 128 ;                                           // add 1024 bits to stream size
    StreamResize(pstream, NULL, ssize) ;                     // resize stream
    EZ_GET_INSERT_VARS(*pstream) ;                           // get updated insertion control values from stream struct
    TEE_FPRINTF(stderr,2, "(buffer size + 128 bytes) ") ;
    print_stream_params(*pstream, "after resize", NULL) ;
  }
  print_stream_params(*pstream, "before insertions finalize", NULL) ;
  BE64_EZ_INSERT_FINAL ;                                     // flush accumulator into stream
  EZ_SET_INSERT_VARS(*pstream) ;                             // update insertion control values in stream struct
  print_stream_params(*pstream, "after  insertions finalize", NULL) ;
  TEE_FPRINTF(stderr,2, "\n") ;

  bsize = StreamDataCopy(pstream, buffer, sizeof(buffer)) ;  // copy original stream data into buffer
  TEE_FPRINTF(stderr,2, "copied %ld bits from original stream\n", bsize) ;
  TEE_FPRINTF(stderr,2, "\n") ;

  // initialize a new stream using buffer
  BeStreamInit(&(STREAM1), buffer, sizeof(buffer), BIT_XTRACT) ;
  pstream = &(STREAM1) ;
  StreamSetFilledBits(pstream, ntot) ;                       // set number of valid bits in stream
  print_stream_params(*pstream, "*pstream before extraction", NULL) ;

  // check now that we get back what was inserted
  EZ_NEW_XTRACT_VARS(*pstream) ;                             // declare and get extraction control values from stream struct
  ntot = 0 ;
  errors = 0 ;
  for(i0 = 0 ; i0 < 48 ; i0 += 12){
    for(i=0 ; i<12 ; i++){
      value = (i & 1) ? -(i0+i) : i0+i ;
      w32 = to_zigzag_32(value) ;
      nbits = 32 - lzcnt_32(w32) ;
      ntot += nbits ;
      BE64_EZ_GET_NBITS(w32, nbits) ;                        // get nbits from stream
      if(value != from_zigzag_32(w32)) {                     // check that we get the expected value
        errors++ ;
        if(errors < 4) TEE_FPRINTF(stderr,2, "expected %8.8x, got %8.8x\n", value, from_zigzag_32(w32)) ;
      }
      TEE_FPRINTF(stderr,2, ".");
    }
    EZ_SET_XTRACT_VARS(*pstream) ;                           // update extraction control values in stream struct
    TEE_FPRINTF(stderr,2, " extracted %4d bits, ", ntot) ;
    print_stream_params(*pstream, "*pstream after extraction", NULL) ;
  }
  print_stream_params(*pstream, "*pstream after all extractions", NULL) ;
  TEE_FPRINTF(stderr,2, "extraction errors = %d\n", errors) ;
  if(errors > 0){
    TEE_FPRINTF(stderr,2, "TEST FAILED\n") ;
    exit(1) ;
  }else{
    TEE_FPRINTF(stderr,2, "SUCCESS\n") ;
  }

  return 0 ;
}
