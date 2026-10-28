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
#if defined(__x86_64__)
#include <immintrin.h>
#else
#define USE_C_CODE
#endif

#include <rmn/fst_mask.h>

#define STATIC static

// build 8 bit mask from 8 values (1 where s[x] == value, 0 elsewhere
// s     [IN] : source array
// value [IN] : special value
// return 8 bit mask (LittleEndian style) in lower 8 bits of 32 bit integer
// LSB is associated with s[0]
STATIC  uint32_t build_mask_w08(uint32_t *s, uint32_t value){
#if defined(USE_C_CODE) || ! defined(__AVX2__)
  uint32_t m8 = 0 ;
  for(int i=0 ; i<8 ; i++){
    m8 |= ( (s[i] == value) ? (1 << i) : 0 ) ;
  }
#else
  __m256i s8 = _mm256_loadu_si256 ((__m256i const *)s) ;
  __m256i v8 = _mm256_set1_epi32(value) ;
  __m256i t8 = _mm256_cmpeq_epi32(s8, v8) ;
  int32_t m8 = _mm256_movemask_ps((__m256)t8) ;
#endif
  return m8 ;
}

// where s[0..7] != value, copy s[i] into destination array d
// d    [OUT] : destination array
// m    [OUT] : result 8 bit mask
// s     [IN] : source array
// value [IN] : special value
// return number of values copied from s into d, set m[0] to 8 bit mask value
STATIC int32_t compress_masked_w08(uint32_t * restrict d, uint8_t * restrict m, uint32_t * restrict s, uint32_t value){
  uint32_t mask = build_mask_w08(s, value), count = 0 ;
  *m = mask ;
  if(mask == 0xFF) return 0 ;

  for(int i=0 ; i<8 ; i++){
    if( (mask & (1 << i)) == 0 ) { *d = s[i] ; d ++ ; count++ ; }
  }
  return count ;
}

// build 32 bit mask from 32 values (1 where s[x] == value, 0 elsewhere
// s     [IN] : source array
// value [IN] : special value
// return 32 bit mask (LittleEndian style)
// LSB is associated with s[0]
STATIC uint32_t build_mask_w32(uint32_t *s, uint32_t value){
  int32_t m0 = build_mask_w08(s     , value) ;
  int32_t m1 = build_mask_w08(s +  8, value) ;
  int32_t m2 = build_mask_w08(s + 16, value) ;
  int32_t m3 = build_mask_w08(s + 24, value) ;
  return ( m0 | (m1 << 8) | (m2 << 16) | (m3 << 24) ) ;
}

// where s[0..31] != value, copy s[i] into destination array d
// d    [OUT] : destination array
// m    [OUT] : result 32 bit mask
// s     [IN] : source array
// value [IN] : special value
// return number of values copied from s into d, set m[0] to 32 bit mask value
STATIC int32_t compress_masked_w32(uint32_t * restrict d, uint32_t * restrict m, uint32_t * restrict s, uint32_t value){
  uint32_t mask = build_mask_w32(s, value), count = 0 ;
  *m = mask ;
  if(mask == 0xFFFFFFFF) return 0 ;

  for(int i=0 ; i<32 ; i++){
    if( (mask & (1 << i)) == 0 ) { *d = s[i] ; d++ ; count++ ; }
  }
  return count ;
}

// where s[0..n-1] != value, copy s[i] into destination array d
// d    [OUT] : destination array
// m    [OUT] : result 32 bit mask array
// s     [IN] : source array
// value [IN] : special value
// n     [IN] : number of values in source array
// return number of values copied from s into d
// this function should work "in place" as destination array is at worst the same length as the source array
int32_t compress_masked_w(uint32_t * restrict d, uint32_t * restrict m, uint32_t * restrict s, uint32_t value, int n){
  int32_t total = 0 ;
  while(n > 31){                    // blocks of 32 values
    int32_t count = compress_masked_w32(d, m, s, value) ;
    s += 32 ; d += count ; m++ ; n -= 32 ; total += count ;
  }
  uint8_t *c = (uint8_t *)m ;       // LITTLE ENDIAN MEMORY STORAGE IS ASSUMED
  if(n > 0){
    *m = 0 ;                        // initialize mask element to 32 zero bits
    for(int i=0 ; i< 3 ; i++){      // blocks of 8 values, at most 3 rounds
      if(n < 8) break ;
      int32_t count = compress_masked_w08(d, c, s, value) ;
      s += 8 ; d += count ; c++ ; n -= 8 ; total += count ;
    }
  }
  if(n > 0){                        // up to 7 values left
    uint32_t m8 = 0 ;
    for(int i=0 ; i<8 ; i++){
      if(i == n) break ;
      if(s[i] == value){
        m8 |= (1 << i) ;
      }else{
        total++ ;
        *d = s[i] ; d ++ ;
      }
    }
    *c = m8 ;
  }
  return total ;
}

// where s[0..n-1] != value, set bit i in mask
// m    [OUT] : result 32 bit mask array
// s     [IN] : source array
// value [IN] : special value
// n     [IN] : number of values in source array
// return number of values copied from s into d
int32_t build_mask_w(uint32_t * restrict m, uint32_t * restrict s, uint32_t value, int n){
  int32_t total = 0 ;
  while(n > 31){                    // blocks of 32 values
    *m = build_mask_w32(s, value) ;
    s += 32 ; m++ ; n -= 32 ;
  }
  uint8_t *c = (uint8_t *)m ;       // LITTLE ENDIAN MEMORY STORAGE IS ASSUMED
  if(n > 0){
    *m = 0 ;                        // initialize mask element to 32 zero bits
    for(int i=0 ; i< 3 ; i++){      // blocks of 8 values, at most 3 rounds
      if(n < 8) break ;
      *c = build_mask_w08(s, value) ;
      s += 8 ; c++ ; n -= 8 ;
    }
  }
  if(n > 0){                        // up to 7 values left
    uint32_t m8 = 0 ;
    for(int i=0 ; i<8 ; i++){
      if(i == n) break ;
      if(s[i] == value){
        m8 |= (1 << i) ;
      }
    }
    *c = m8 ;
  }
  return n ;
}

// print mask in big endian style (LSB left)
void print_mask_w(FILE *f, char *header, uint32_t *m, int n){
  fprintf(f, "%s0123456789ABCDEF0123456789ABCDEF\n", header) ;
  char bits[32] ;
  while(n > 0){
    fprintf(f, "%s", header) ;
    for(int i=0 ; i<32 ; i++){ bits[i] = ' ' ; }
    for(int i=0 ; i<32 && i<n ; i++) { bits[31-i] = ((m[0] >> i) & 1) ? '1' : '0' ; }
    for(int i=0 ; i<32        ; i++) { fprintf(f, "%c", bits[31-i]) ; } ;
    fprintf(f, " %8.8x\n", m[0]) ;
    n -= 32 ; m++ ;
  }
}

// where mask bit [0..7] != 1, copy s[i] into destination array d
// d    [OUT] : destination array
// mask  [IN] : 8 bit control mask
// s     [IN] : source array
// return number of values copied from s into d
STATIC int32_t store_masked_w08(uint32_t * restrict d, uint32_t mask, uint32_t * restrict s){
  uint32_t *d0 = d ;
  if(mask == 0xFFFFFFFF) return 0 ;    // all values are masked

  for(int i=0 ; i<8 ; i++){
    if( (mask & (1 << i)) == 0 ) { *d = s[i] ; d++ ; }
  }
  return (d - d0) ;
}

// where mask bit [0..31] != 1, copy s[i] into destination array d
// d    [OUT] : destination array
// mask  [IN] : 32 bit control mask
// s     [IN] : source array
// return number of values copied from s into d
STATIC int32_t store_masked_w32(uint32_t * restrict d, uint32_t mask, uint32_t * restrict s){
  uint32_t *d0 = d ;
  if(mask == 0xFFFFFFFF) return 0 ;    // all values are masked

  for(int i=0 ; i<32 ; i++){
    if( (mask & (1 << i)) == 0 ) { *d = s[i] ; d++ ; }
  }
  return (d - d0) ;
}

// where s[0..n-1] != value, copy s[i] into destination array d
// d    [OUT] : destination array
// m     [IN] : 32 bit mask array
// s     [IN] : source array
// n     [IN] : number of values in source array (and number of bits from m)
// return number of values copied from s into d
// this function should work "in place" as destination array is at worst the same length as the source array
int32_t store_masked_w(uint32_t * restrict d, uint32_t * restrict m, uint32_t * restrict s, int n){
  uint32_t *d0 = d ;
  int32_t total = 0 ;
  while(n > 31){                    // blocks of 32 values
    int32_t count = store_masked_w32(d, *m, s) ;
    s += 32 ; d += count ; m++ ; n -= 32 ;
  }
  uint8_t *c = (uint8_t *)m ;       // LITTLE ENDIAN MEMORY STORAGE IS ASSUMED
  if(n > 0){
    for(int i=0 ; i< 3 ; i++){      // blocks of 8 values, at most 3 rounds
      if(n < 8) break ;
      int32_t count = store_masked_w08(d, *c, s) ;
      s += 8 ; d += count ; c++ ; n -= 8 ;
    }
  }
  if(n > 0){                        // up to 7 values left
    uint32_t m8 = *c ;
    for(int i=0 ; i<8 ; i++){
      if(i == n) break ;
      if( (m8 & (1 << i)) == 0){
        *d = s[i] ; d ++ ;
      }
    }
  }
  return (d - d0) ;
}

// d    [OUT] : destination array
// mask  [IN] : 8 bit control mask (lower 8 bits)
// s     [IN] : source array
// value [IN] : special value stored in d[] if corresponding mask bit is set
// return number of values copied from s into d (<= 8)
STATIC int32_t expand_masked_w08(uint32_t * restrict d, uint32_t mask, uint32_t * restrict s, uint32_t value){
  int32_t count = 0 ;
  mask &= 0xFF ;
  if(mask == 0){ for(int i=0 ; i<8 ; i++){ d[i] = s[i] ; } return 8 ; }

  for(int i=0 ; i<8 ; i++){ d[i] = value ; }
  if(mask == 0xFF) return 0 ;

  for(int i=0 ; i<8 ; i++){
    if( ( mask & (1 << i) ) == 0 ){
      d[i] = s[count] ;                   // destination = source
      count++ ;                           // bump source count
    }
  }
  return count ;
}

// d    [OUT] : destination array
// mask  [IN] : 32 bit control mask
// s     [IN] : source array
// value [IN] : special value stored in d[] if corresponding mask bit is set
// return number of values copied from s into d (<= 32)
STATIC int32_t expand_masked_w32(uint32_t * restrict d, uint32_t mask, uint32_t * restrict s, uint32_t value){
  int32_t total = 0, count ;
  if(mask == 0){
    for(int i=0 ; i<32 ; i++){ d[i] = s[i] ; }
    return 32 ;
  }
  if(mask == 0xFFFFFFFF){
    for(int i=0 ; i<32 ; i++){ d[i] = value ; }
    return 32 ;
  }
  count = expand_masked_w08(d, mask      , s, value) ; d += 8 ; s += count ; total += count ;
  count = expand_masked_w08(d, mask >>  8, s, value) ; d += 8 ; s += count ; total += count ;
  count = expand_masked_w08(d, mask >> 16, s, value) ; d += 8 ; s += count ; total += count ;
  count = expand_masked_w08(d, mask >> 24, s, value) ; d += 8 ; s += count ; total += count ;
  return total ;
}

// copy s[i] or value into destination array d according to mask
// d    [OUT] : destination array
// m     [IN] : control mask
// s     [IN] : source array
// value [IN] : special value stored in d[] if corresponding mask bit is set
// n     [IN] : number of values copied from source array (<= n)
// return number of values copied from s into d
int32_t expand_masked_w(uint32_t * restrict d, uint32_t * restrict m, uint32_t * restrict s, uint32_t value, int n){
  int32_t total = 0 ;
  while(n > 31){                    // blocks of 32 values
    int32_t count = expand_masked_w32(d, *m, s, value) ;
    d += 32 ; s += count ; m++ ; n -= 32 ; total += count ;
  }
  uint8_t *c = (uint8_t *)m ;       // LITTLE ENDIAN MEMORY STORAGE IS ASSUMED
  while(n > 7){                     // blocks of 8 values
    int32_t count = expand_masked_w08(d, *c, s, value) ;
    d += 8 ; s += count ; c++ ; n -= 8 ; total += count ;
  }
  if(n > 0){                        // up to 7 values left
    uint32_t m8 = *c ;
    int32_t count = 0 ;
    for(int i=0 ; i<8 ; i++){
      if(i == n) break ;
      if( ( m8 & (1 << i) ) == 0 ){
        d[i] = s[count] ;           // destination = source
        count++ ;                   // bump source count
      }
    }
    total += count ;
  }
  return total ;
}

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

  if(count == 0){
    byte =  byte ? 0xFF : 0 ;
    STREAM_PUT_NBITS(*stream, byte, 9) ;  // end of sequence code (byte == 0x00)
  }

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

#if 0
// used to prevent compiler from calling memcpy
static uint32_t dummy = 0 ;
void set_useless_symbol(void){
  dummy++ ;
}
// specialized (n == 32) version
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
// expand using mask, up to 32 values
// d   [OUT] : destination array
// s    [IN] : source array
// mask [IN] : 32 bit control mask
// n    [IN] : number of valid bits in mask (up to 32)
// return number of values copied from source to destination
static int32_t load_expand_n(uint32_t * restrict d, uint32_t * restrict s, uint32_t plug, uint32_t mask, uint32_t n){
  if(n ==  0) return 0 ;     // nothing to do
  if(n == 32) return load_expand_32(d, s, plug, mask) ;

  uint32_t *s0 = s ;        // start of source
  n = (n > 32) ? 32 : n ;
  uint32_t emask = 0xFFFFFFFF >> (32 - n) ;

  if(mask == 0){                 // copy all values from source
    for(uint32_t i=0 ; i<32 ; i++) { if(i == n) break ; d[i] = s[i] + dummy ; }
    return n ;
  }
  if((mask & emask) == emask){   // plug all values
    for(uint32_t i=0 ; i<32 ; i++) { if(i == n) break ; d[i] = plug ; }
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
    s += partial ; d += 32 ; mask++ ; n -= 32 ;
    total += partial ;
  }
  if(n > 0) total += load_expand_n(d, s, plug, *mask, n) ;          // remaining (< 32) values
  return total ;
}
int32_t store_compress_32(uint32_t * restrict d, uint32_t * restrict s, uint32_t mask){
  uint32_t *d0 = d ;        // start of destination
  if(mask == 0         ){ for(uint32_t i=0 ; i<32 ; i++) { d[i] = s[i] | dummy ; return 32 ; } }
  if(mask == 0xFFFFFFFF){ return  0 ; }
  for(int i=0 ; i<32 ; i++){     // skip if mask bit is 1, copy otherwise
    if((mask >> i) & 1){
      s++ ;                      // do not copy source
    }else{
      d[i] = *s ;                // copy
      s++ ;
    }
  }
  return (d - d0) ;
}

int32_t store_compress_n(uint32_t * restrict d, uint32_t * restrict s, uint32_t mask, uint32_t n){
  if(n == 32) return store_compress_32(d, s, mask) ;
  if(n ==  0) return 0 ;
  if(mask == 0){ for(uint32_t i=0 ; i<32 ; i++) { d[i] = s[i] | dummy ; return 32 ; } }

  uint32_t *d0 = d ;        // start of destination
  n = (n > 32) ? 32 : n ;
  uint32_t emask = 0xFFFFFFFF >> (32 - n) ;
  if((mask & emask) == emask){ return  0 ; }

  for(uint32_t i=0 ; i<32 ; i++){   // skip if mask bit is 1, copy otherwise
    if(i == n) break ;
    if((mask >> i) & 1){
      s++ ;                         // do not copy source
    }else{
      d[i] = *s ;                   // copy
      s++ ;
    }
  }
  return (d - d0) ;
}

// expand using mask
// d    [OUT] : destination array
// s     [IN] : source array
// mask [OUT] : control mask array (32 bit items)
// n     [IN] : number of values
// return number of values copied from source to destination
int32_t store_compress(uint32_t * restrict d, uint32_t * restrict s, uint32_t * restrict mask, uint32_t n){
  int32_t total = 0 ;
  while(n > 31){
    int32_t partial = store_compress_32(d, s, *mask) ;
    d += partial ; s += 32 ; mask++ ; n -= 32 ;
    total += partial ;
  }
  if(n > 0) total += store_compress_n(d, s, *mask, n) ;
  return total ;
}
#endif
