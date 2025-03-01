//
// Copyright (C) 2025  Environnement Canada
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
//     M. Valin,   Recherche en Prevision Numerique, 2025
//
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <rmn/bitstream.h>

// print some elements at the beginning and at the end of the bit stream data buffer
// (see bi_endian_pack.h)
// s    [IN] : bitstream structure
// msg  [IN] : user message
// edge [IN] : do not print data elements more that edge positions from first or in
void print_stream_data(bitstream s, char *msg, int edge){
  uint32_t *in = s.in ;
  uint32_t *first = s.first ;
  uint32_t *cur, *start ;
  int inc, count = 0 ;

  fprintf(stderr, "[%2s] %s : ", STREAM_IS_LITTLE_ENDIAN(s) ? "LE" : "BE", msg) ;
  fprintf(stderr, "accum = %16.16lx", s.acc_i << (64 - s.insert)) ;
  fprintf(stderr, ", guard = %8.8x, data =", *in) ;

  if(STREAM_IS_LITTLE_ENDIAN(s)){
    cur = in ; inc = -1 ;
  }else{                            // big endian or not specified
    cur = first ; inc = +1 ;
  }

  for(start=first ; start <= in ; start++, cur = cur + inc){
    if(in - cur == 0 && s.insert == 0) continue ;          // last element not used
    if(cur-first < edge || in-cur <= edge) {
      fprintf(stderr, " %8.8x ", *cur) ;
    }else{
      count++ ;
      if((count & 0xFF) == 1) fprintf(stderr, ".") ;
    }
  }
  fprintf(stderr, "\n") ;
}

// print bit stream control information
// s             [IN] : bitstream structure
// msg           [IN] : user message
// expected_mode [IN] : "R", "W", or "RW"  read/write/read-write, expected mode for bit stream
void print_stream_params(bitstream s, char *msg, char *expected_mode){
  int32_t available        = StreamAvailableBits(&s) ;
  int32_t strict_available = StreamStrictAvailableBits(&s) ;
  int32_t space_available   = StreamAvailableSpace(&s) ;

  available = (available < 0) ? 0 : available ;
  strict_available = (strict_available < 0) ? 0 : strict_available ;
  fprintf(stderr, "%s: filled = %d(%d), free= %d, first/in/out/limit [in - out] = %p/%ld/%ld/%ld [%ld], insert/xtract = %d/%d",
    msg, available, strict_available, space_available, 
    (void *)s.first, s.in-s.first, s.out-s.first, s.limit-s.first, s.in-s.out, s.insert, s.xtract ) ;
  fprintf(stderr, ", full/alloc/user = %d/%d/%d", s.full, s.alloc, s.user) ;
  fprintf(stderr, ", |%8.8x|%2.2x|", s.valid, s.endian) ;
  fprintf(stderr, ", Mode = %s(%d)", StreamMode(s), StreamModeCode(s)) ;
  if(expected_mode){
    fprintf(stderr, " (%s expected)", expected_mode) ;
    if(strcmp(StreamMode(s), expected_mode) != 0) { 
      fprintf(stderr, "\nBad mode, exiting\n") ;
      exit(1) ;
    }
  }
  fprintf(stderr, "\n") ;
}

#include <rmn/be_stream.h>
#define PREFIX_BE
#include "test_endian_bitstream.h"

#include <rmn/le_stream.h>
#define PREFIX_LE
#include "test_endian_bitstream.h"

int main(int argc, char **argv){

  fprintf(stderr, "============================== %s ", argv[0]);
  while(--argc > 0){
    argv++ ;
    fprintf(stderr, "%s ", argv[0]);
  }
  fprintf(stderr, " ==============================\n") ;

  fprintf(stderr, "\n============================== LE test ==============================\n\n") ;
  if(le_test()) goto fail ;

  fprintf(stderr, "\n============================== BE test ==============================\n\n") ;
  if(be_test()) goto fail ;

//   fprintf(stderr, "SUCCESS\n") ;
  return 0 ;

fail:
  fprintf(stderr, "FAILED\n") ;
  exit(1) ;
}
