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
// Lesser General Public License for more details .
//
// Author:
//     M. Valin,   Recherche en Prevision Numerique, 2023
//

#if ! defined(PRINT_BITSTREAM_DIAG)

#define PRINT_BITSTREAM_DIAG

#include <stdint.h>
#include <rmn/tee_print.h>
#include <rmn/bi_endian_pack.h>

// helper functions to help diagnose bitstream management functions

// print some elements at the beginning and at the end of the bit stream data buffer
// (see bi_endian_pack.h)
// s   [IN] : bitstream structure
// msg [IN] : user message
static void print_stream_data(bitstream s, char *msg){
  uint32_t *in = s.in ;
  uint32_t *first = s.first ;
  uint32_t *cur = first ;

  TEE_FPRINTF(stderr,2, "%s : ", msg) ;
  TEE_FPRINTF(stderr,2, "accum = %16.16lx", s.acc_i << (64 - s.insert)) ;
  TEE_FPRINTF(stderr,2, ", guard = %8.8x, data =", *cur) ;
  while(cur <= in){
    if(in - cur == 0 && s.insert == 0) break ;                // last element not used
    if(in - cur == 2 && s.insert != 0) {cur++ ; continue ; }  // last element used
    if(cur-first < 3 || in-cur < 3) {
      TEE_FPRINTF(stderr,2, " %8.8x", *cur) ;
    }else{
      TEE_FPRINTF(stderr,2, ".") ;
    }
    cur++ ;
  }
  TEE_FPRINTF(stderr,2, "\n") ;
}

// print bit stream control information
// s             [IN] : bitstream structure
// msg           [IN] : user message
// expected_mode [IN] : "R", "W", or "RW"  read/write/read-write, expected mode for bit stream
static void print_stream_params(bitstream s, char *msg, char *expected_mode){
  int32_t available = StreamAvailableBits(&s) ;
  int32_t strict_available = StreamStrictAvailableBits(&s) ;
  int32_t space_available = StreamAvailableSpace(&s) ;
  available = (available < 0) ? 0 : available ;
  strict_available = (strict_available < 0) ? 0 : strict_available ;
  TEE_FPRINTF(stderr,2, "%s: filled = %d(%d), free= %d, first/in/out/limit [in - out] = %p/%ld/%ld/%ld [%ld], insert/xtract = %d/%d",
    msg, available, strict_available, space_available, 
    s.first, s.in-s.first, s.out-s.first, s.limit-s.first, s.in-s.out, s.insert, s.xtract ) ;
  TEE_FPRINTF(stderr,2, ", full/alloc/user = %d/%d/%d", s.full, s.alloc, s.user) ;
  TEE_FPRINTF(stderr,2, ", |%8.8x|%d|", s.valid, s.endian) ;
  if(expected_mode){
    TEE_FPRINTF(stderr,2, ", Mode = %s(%d) (%s expected)", StreamMode(s), StreamModeCode(s), expected_mode) ;
    if(strcmp(StreamMode(s), expected_mode) != 0) { 
      TEE_FPRINTF(stderr,2, "\nBad mode, exiting\n") ;
      exit(1) ;
    }
  }
  TEE_FPRINTF(stderr,2, "\n") ;
}
#endif
