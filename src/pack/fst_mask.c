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
#include <rmn/fst_mask.h>

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

  if(count == 0) STREAM_PUT_NBITS(*stream, 0x00, 9) ;  // end of sequence code (byte == 0x00)

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

// used to prevent compiler from calling memcpy
static uint32_t dummy = 0 ;
void set_useless_symbol(void){
  dummy++ ;
}
// expand using mask, up to 32 values
// d   [OUT] : destination array
// s    [IN] : source array
// mask [IN] : 32 bit control mask
// n    [IN] : number of valid bits in mask (up to 32)
// return number of values copied from source to destination
static int32_t load_expand_n(uint32_t * restrict d, uint32_t * restrict s, uint32_t plug, uint32_t mask, uint32_t n){
  if(n == 0) return 0 ;     // nothing to do
  uint32_t *s0 = s ;        // start of source
  n = (n > 32) ? 32 : n ;
  uint32_t emask = 0xFFFFFFFF >> (32 - n) ;

  if(mask == 0){                 // copy all values from source
    if(n == 32){
      for(uint32_t i=0 ; i<32 ; i++) { d[i] = s[i] ; }
    }else{
      for(uint32_t i=0 ; i<32 ; i++) { if(i == n) break ; d[i] = s[i] + dummy ; }
    }
    return n ;
  }
  if((mask & emask) == emask){   // plug all values
    if(n == 32){
      for(uint32_t i=0 ; i<32 ; i++) { d[i] = plug ; }
    }else if(n == 8){
      for(uint32_t i=0 ; i<8 ; i++) { if(i == n) break ; d[i] = plug ; }
    }else{
      for(uint32_t i=0 ; i<32 ; i++) { if(i == n) break ; d[i] = plug ; }
    }
    return 0 ;
  }

  for(uint32_t i=0 ; i<32 ; i++){         // plug if mask bit is 1, copy otherwise
    if(i == n) break ;
    if((mask >> i) & 1){
      d[i] = plug ;                  // plug destination
    }else{
      d[i] = *s ; s++ ;              // copy, then bump source pointer
    }
  }
  return (s - s0) ;
}
// specialized (n == 32) version of above
static int32_t load_expand_32(uint32_t * restrict d, uint32_t * restrict s, uint32_t plug, uint32_t mask){
  uint32_t *s0 = s ;        // start of source
  if(mask == 0         ){ for(uint32_t i=0 ; i<32 ; i++) { d[i] = s[i] ; return 32 ; } }
  if(mask == 0xFFFFFFFF){ for(uint32_t i=0 ; i<32 ; i++) { d[i] = plug ; return  0 ; } }
  for(int i=0 ; i<32 ; i++){
    if((mask >> i) & 1){
      d[i] = plug ;                  // plug destination
    }else{
      d[i] = *s ; s++ ;              // copy, then bump source pointer
    }
  }
  return (s - s0) ;
}

// expand using mask
// d   [OUT] : destination array
// s    [IN] : source array
// mask [IN] : control mask array (32 bit items)
// n    [IN] : number of valid bits in mask array
// return number of values copied from source to destination
int32_t load_expand(uint32_t * restrict d, uint32_t * restrict s, uint32_t plug, uint32_t * restrict mask, uint32_t n){
  int32_t total = 0 ;
  while(n > 31){
    int32_t partial = load_expand_32(d, s, plug, *mask) ; // 32 values slice
    s += partial ; d += 32 ; mask++ ;
    n -= 32 ;
    total += partial ;
  }
  if(n > 0) total += load_expand_n(d, s, plug, *mask, n) ;          // remaining (< 32) values
  return total ;
}
