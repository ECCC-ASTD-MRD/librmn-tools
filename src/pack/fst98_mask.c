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

// partial encoding of a single byte repeat count
// byte  [IN] : byte to be repeated
// count [IN] : repeat count (up to 
// return byte/encoded token/length in bits of encoded token/residual repeat count
// in a fst_rle_code struct
// in case of error, all members of the returned struc will be 0
fst_rle_code fst_rle_token(uint8_t byte, uint32_t count){
  fst_rle_code r = (fst_rle_code) { 0, 0, 0, 0} ;
  if(count == 0)     goto end ;       // ERROR : zero count
  if(count > 0xFFFF) goto end ;       // ERROR : count needs more than 16 bits
  r.byte = byte ;
  if(byte != 0x00 && byte != 0xFF){   // byte not eligible for repeat count
    r.code = byte ;                   // 0bbyyttee : 0 bit followed by byte
    r.bits = 9 ;
    r.left = count - 1 ;              // count is automatically 1
    goto end ;
  }
// code 000000000b or 011111111 will never be returned
// 1010b or 1011b would be the returned code
  if(count < RLE_SMALL_OFFSET){           // 4 bit micro RLE count (1 - 2)
    r.code = ((byte == 0) ? RLE_MICRO_0 : RLE_MICRO_1) | (count - RLE_MICRO_OFFSET) ;
    r.bits = 4 ;
  }else if(count < RLE_LARGE_OFFSET){     // 8 bit small RLE count (3 - 18)
    r.code = ((byte == 0) ? RLE_SMALL_0 : RLE_SMALL_1) | (count - RLE_SMALL_OFFSET) ;
    r.bits = 8 ;
  }else if(count <= RLE_MAX_COUNT){        // 12 bit large RLE count (RLE_LARGE_OFFSET - RLE_MAX_COUNT)
    r.code = ((byte == 0) ? RLE_LARGE_0 : RLE_LARGE_1) | (count - RLE_LARGE_OFFSET) ;
    r.bits = 12 ;
  }else{                                  // > RLE_MAX_COUNT,  12 bit large RLE count (RLE_MAX_COUNT)
    r.code = ((byte == 0) ? RLE_LARGE_0 : RLE_LARGE_1) | 0xFF ;
    r.bits = 12 ;
    r.left = count - (RLE_MAX_COUNT) ;    // we have leftovers
  }
end:
  return r ;
}

// partial encoding of a single byte repeat count
// byte      [IN] : byte to be repeated
// count     [IN] : repeat count (MUST BE <= 0xFF)
// stream [INOUT] : bit stream to receive encoded data
// returns the total number of bits used for encoding
static int32_t put_rle_count_(uint8_t byte, uint32_t count, bitstream *stream){
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

// encode a single byte repeat count
// byte      [IN] : byte to be repeated
// count     [IN] : repeat count
// stream [INOUT] : bit stream to receive encoded data
// returns the total number of bits used for encoding
// if count == 0, put end of sequence code into stream
int32_t put_rle_count(uint8_t byte, uint32_t count, bitstream *stream){
  int32_t totbits = 0 ;

  if(count == 0) STREAM_PUT_NBITS(*stream, 0xFF, 9) ;  // end of sequence code

  while(count > 0xFFFF){
    totbits += put_rle_count_(byte, 0xFFFF, stream) ;
    count -= 0xFFFF ;
  }
  if(count > 0) totbits += put_rle_count_(byte, count, stream) ;

  return totbits ;
}

// decode a single byte repeat count
// stream [INOUT] : bit stream containing encoded data
// return fst_rle_code struct
// if an end of sequence code is detected, a count of 0 is returned
fst_rle_code get_rle_token(bitstream *stream){
  fst_rle_code r = (fst_rle_code) { 0, 0, 0, 0} ;
  uint32_t bits, token ;
  STREAM_PEEK_NBITS(*stream, bits, 4) ;
  if(0 == (bits >> 3)){
    STREAM_FAST_GET_NBITS(*stream, bits, 9) ;
//     if(bits == 0xFF) bits = 0 ;        // 000000000 or 011111111 are equivalent
    r.bits = 9 ;
    r.code = r.byte = bits ;
    r.count = ((bits == 0) || (bits == 0xFF)) ? 0 : 1 ;
  }else{
    r.byte = (bits & 0x4) ? 0xFF : 0x00 ;  // 11xx is a repeat count for 0xFF, 10xx is a repeat count for 0x00
    bits &= 0x7 ;
    token = 0 ;
    switch(bits){
      case 2:    // 1010b
      case 3:    // 1011b
      case 4:    // 1100b
      case 5:    // 1101b
        STREAM_FAST_GET_NBITS(*stream, token, 4) ;
        r.code = token ;
        r.count = 1 + (token & 1) ;
        r.bits = 4 ;
        break ;
      case 1:    // 1001b
      case 6:    // 1110b
        STREAM_FAST_GET_NBITS(*stream, token, 8) ;
        r.code = token ;
        r.count = 3 + (token & 0xF) ;
        r.bits = 8 ;
        break ;
      case 0:    // 1000b
      case 7:    // 1111b
        STREAM_FAST_GET_NBITS(*stream, token, 12) ;
        r.code = token ;
        r.count = 19 + (token & 0xFF) ;
        r.bits = 12 ;
    }
  }
  return r ;
}
