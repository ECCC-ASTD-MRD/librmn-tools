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

static void print_stream_params(bitstream s, char *msg, char *expected_mode){
  int32_t available = StreamAvailableBits(&s) ;
  int32_t strict_available = StreamStrictAvailableBits(&s) ;
  int32_t space_available = StreamAvailableSpace(&s) ;
  available = (available < 0) ? 0 : available ;
  strict_available = (strict_available < 0) ? 0 : strict_available ;
  TEE_FPRINTF(stderr,2, "%s: bits used = %d(%d), bits free= %d, first/in/out/limit = %p/%ld/%ld/%ld [%ld], insert/xtract = %d/%d",
    msg, available, strict_available, space_available, 
    s.first, s.in-s.first, s.out-s.first, s.limit-s.first, s.in-s.out, s.insert, s.xtract ) ;
  TEE_FPRINTF(stderr,2, ", full/part/user = %d/%d/%d\n", s.full, s.part, s.user) ;

  if(expected_mode){
    TEE_FPRINTF(stderr,2, "Stream mode = %s(%d) (%s expected)\n", StreamMode(s), StreamModeCode(s), expected_mode) ;
    if(strcmp(StreamMode(s), expected_mode) != 0) { 
      TEE_FPRINTF(stderr,2, "Bad mode, exiting\n") ;
      exit(1) ;
    }
  }
}

int main(int argc, char **argv){
  bitstream streams[NSTREAMS] ;
  bitstream *pstream = &(streams[0]) ;
  bitstream_state states[NSTREAMS];
  bitstream_state *state = &(states[0]) ;
  int i0, i, j0, j, nbits, value, ntot, errors ;
  uint32_t w32 ;

  start_of_test("test_bitstream C") ;

  BeStreamInit(pstream, NULL, 1024, 0) ;   // 1024 byte stream
  print_stream_params(STREAM0, "1024 byte stream", NULL) ;

  uint64_t accum   = pstream->acc_i ;    // get control values from stream struct
  int32_t  insert  = pstream->insert ;
  uint32_t *in = pstream->in ;
  ntot = 0 ;
  for(i0 = 0 ; i0 < 32 ; i0 += 8){
    for(i=0 ; i<8 ; i++){
      value = (i & 1) ? -(i0+i) : i0+i ;
      w32 = to_zigzag_32(value) ;
      nbits = 32 - lzcnt_32(w32) ;
      ntot += nbits ;
      BE64_PUT_NBITS(accum, insert, w32, nbits, in) ;
      TEE_FPRINTF(stderr,2, ".");
    }
    BE64_PUSH(accum, insert, in) ;
    pstream->acc_i  = accum ;
    pstream->insert = insert ;
    pstream->in    = in ;
    TEE_FPRINTF(stderr,2, " ntot = %d ", ntot) ;
    print_stream_params(*pstream, "after insertion", NULL) ;
  }
  BE64_INSERT_FINAL(accum, insert, in) ;
  print_stream_params(*pstream, "after all insertions", NULL) ;

  accum   = pstream->acc_x ;    // get control values from stream struct
  int32_t  xtract  = pstream->xtract ;
  uint32_t *out = pstream->out ;
  ntot = 0 ;
  errors = 0 ;
  for(i0 = 0 ; i0 < 32 ; i0 += 8){
    for(i=0 ; i<8 ; i++){
      value = (i & 1) ? -(i0+i) : i0+i ;
      w32 = to_zigzag_32(value) ;
      nbits = 32 - lzcnt_32(w32) ;
      ntot += nbits ;
      BE64_GET_NBITS(accum, xtract, w32, nbits, out) ;
      if(value != from_zigzag_32(w32)) errors++ ;
      TEE_FPRINTF(stderr,2, ".");
    }
    pstream->acc_x  = accum ;
    pstream->xtract = xtract ;
    pstream->out    = out ;
    TEE_FPRINTF(stderr,2, " ntot = %d ", ntot) ;
    print_stream_params(*pstream, "after extraction", NULL) ;
  }
  print_stream_params(*pstream, "after all extractions", NULL) ;
  TEE_FPRINTF(stderr,2, "extraction errors = %d\n", errors) ;

  return 0 ;
}
