// Hopefully useful functions for C and FORTRAN
// Copyright (C) 2026  Environnement Canada
//
// This code is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation,
// version 2.1 of the License.
//
// This code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
#include <rmn/fst98_mask.h>

// helper to encode a byte repeat count
// byte  [IN] : byte to be repeated
// count [IN] : repeat count
// return byte/encoded token/length in bits of encoded token/residual repeat count
// in a fst_rle_code struct
fst_rle_code fst_rle_token(uint8_t byte, uint32_t count){
  fst_rle_code r = (fst_rle_code) { 0, 0, 0, 0} ;
  r.byte = byte ;
  if(count == 0) goto end ;           // zero count
  if(byte != 0x00 && byte != 0xFF){   // byte not eligible for repeat count
    r.code = byte ;      // 0bbbbbbbb
    r.bits = 9 ;
    count-- ;
    goto end ;
  }
  if(count > RLE_MAX_COUNT){    // will need more that one RLE count
    r.code = ((byte == 0) ? RLE_LARGE_0 : RLE_LARGE_1) | 0xFF ;
    count -= RLE_MAX_COUNT ;
    r.bits = 12 ;
  }else if(count > RLE_LARGE_OFFSET){      // 12 bit large RLE count (19 - 274)
    r.code = ((byte == 0) ? RLE_LARGE_0 : RLE_LARGE_1) | (count - RLE_LARGE_OFFSET) ;
    count = 0 ;
    r.bits = 12 ;
  }else if(count > RLE_SMALL_OFFSET){      // 8 bit small RLE count (3 - 18)
    r.code = ((byte == 0) ? RLE_SMALL_0 : RLE_SMALL_1) | (count - RLE_SMALL_OFFSET) ;
    count = 0 ;
    r.bits = 8 ;
  }else if(count > RLE_MICRO_OFFSET){      // 1 bit micro RLE count (1 - 2)
    r.code = ((byte == 0) ? RLE_MICRO_0 : RLE_MICRO_1) | (count - RLE_MICRO_OFFSET) ;
    count = 0 ;
    r.bits = 4 ;
  }
end:
  r.left = count ;
  return r ;
}
int32_t fst_rle_encode(uint8_t byte, uint32_t count, bitstream *stream){
  int32_t totbits = 0 ;
  while(count > 0){
    fst_rle_code r = fst_rle_token(byte, count) ;
    uint32_t code = r.code ;
    STREAM_PUT_NBITS(*stream, code, r.bits) ;
    count = r.left ;
    totbits += r.bits ;
  }
  return totbits ;
}
