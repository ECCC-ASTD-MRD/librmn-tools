// Hopefully useful functions for C and FORTRAN
// Copyright (C) 2023  Recherche en Prevision Numerique
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
// uncomment following line to force plain C (non SIMD) code
// #undef __AVX2__
// #undef __BMI2__
// #undef __AVX512F__

#if ! defined(STORE_COMPRESS_LOAD_EXPAND)
#define STORE_COMPRESS_LOAD_EXPAND

#include <stdint.h>
#include <rmn/bits.h>
#include <rmn/bi_endian_pack.h>

#if defined(__x86_64__) && ( defined(__AVX2__) || defined(__AVX512F__) || defined(__AVX512VBMI2__) || defined(__BMI2__) )
#if defined(__AVX512VBMI2__)
#define __BMI2__ 1
#endif
#include <immintrin.h>
#endif

typedef struct{
  int8_t stab[16] ;
} tab_16x16 ;

// N.B. permutation tables are BYTE permutation tables,
//      using a vector index byte permutation instruction,
//      as there is no vector index word permutation instruction

// byte swap of 4 32 bit elements
static int8_t __attribute__((aligned(16))) byte_swap_32[16] = { 3,2,1,0,7,6,5,4,11,10,9,8,15,14,13,12 } ;
// bit reversal of lower 4 bit nibble for 16 bytes (upper_nibble_reverse << 4) (ends up in upper nibble after nibble swap)
// static int8_t __attribute__((aligned(16))) lower_nibble_reverse[16] = { 0,128,64,192,32,160,96,224,16,144,80,208,48,176,112,240 } ;
// bit reversal of upper 4 bit nibble for 16 bytes (ends up in lower nibble after nibble swap)
static int8_t __attribute__((aligned(16))) upper_nibble_reverse[16] = { 0,8,4,12,2,10,6,14,1,9,5,13,3,11,7,15 } ;

// lookup permutation table used to perform a SIMD load-expand
// elements to pick from s[0,1,2,3], x means don't care (set to 0)
static tab_16x16 __attribute__((aligned(16))) exp_be[16] = {                                                                 // MASK s[]
  { -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 0000 [x,x,x,x]
  { -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1,     0,  1,  2,  3 } ,  // 0001 [x,x,x,0]
  { -1, -1, -1, -1,    -1, -1, -1, -1,     0,  1,  2,  3,    -1, -1, -1, -1 } ,  // 0010 [x,x,0,x]
  { -1, -1, -1, -1,    -1, -1, -1, -1,     0,  1,  2,  3,     4,  5,  6,  7 } ,  // 0011 [x,x,0,1]
  { -1, -1, -1, -1,     0,  1,  2,  3,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 0100 [x,0,x,x]
  { -1, -1, -1, -1,     0,  1,  2,  3,    -1, -1, -1, -1,     4,  5,  6,  7 } ,  // 0101 [x,0,x,1]
  { -1, -1, -1, -1,     0,  1,  2,  3,     4,  5,  6,  7,    -1, -1, -1, -1 } ,  // 0110 [x,0,1,x]
  { -1, -1, -1, -1,     0,  1,  2,  3,     4,  5,  6,  7,     8,  9, 10, 11 } ,  // 0111 [x,0,1,2]
  {  0,  1,  2,  3,    -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 1000 [0,x,x,x]
  {  0,  1,  2,  3,    -1, -1, -1, -1,    -1, -1, -1, -1,     4,  5,  6,  7 } ,  // 1001 [0,x,x,1]
  {  0,  1,  2,  3,    -1, -1, -1, -1,     4,  5,  6,  7,    -1, -1, -1, -1 } ,  // 1010 [0,x,1,x]
  {  0,  1,  2,  3,    -1, -1, -1, -1,     4,  5,  6,  7,     8,  9, 10, 11 } ,  // 1011 [0,x,1,2]
  {  0,  1,  2,  3,     4,  5,  6,  7,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 1100 [0,1,x,x]
  {  0,  1,  2,  3,     4,  5,  6,  7,    -1, -1, -1, -1,     8,  9, 10, 11 } ,  // 1101 [0,1,x,2]
  {  0,  1,  2,  3,     4,  5,  6,  7,     8,  9, 10, 11,    -1, -1, -1, -1 } ,  // 1110 [0,1,2,x]
  {  0,  1,  2,  3,     4,  5,  6,  7,     8,  9, 10, 11,    12, 13, 14, 15 } ,  // 1111 [0,1,2,3]
} ;

static tab_16x16 __attribute__((aligned(16))) exp_le[16] = {                                                                 // MASK s[]
  { -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 0000 [x,x,x,x]
  {  0,  1,  2,  3,    -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 0001 [0,x,x,x]
  { -1, -1, -1, -1,     0,  1,  2,  3,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 0010 [x,0,x,x]
  {  0,  1,  2,  3,     4,  5,  6,  7,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 0011 [0,1,x,x]
  { -1, -1, -1, -1,    -1, -1, -1, -1,     0,  1,  2,  3,    -1, -1, -1, -1 } ,  // 0100 [x,x,0,x]
  {  0,  1,  2,  3,    -1, -1, -1, -1,     4,  5,  6,  7,    -1, -1, -1, -1 } ,  // 0101 [0,x,1,x]
  { -1, -1, -1, -1,     0,  1,  2,  3,     4,  5,  6,  7,    -1, -1, -1, -1 } ,  // 0110 [x,0,1,x]
  {  0,  1,  2,  3,     4,  5,  6,  7,     8,  9, 10, 11,    -1, -1, -1, -1 } ,  // 0111 [0,1,2,x]
  { -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1,     0,  1,  2,  3 } ,  // 1000 [x,x,x,0]
  {  0,  1,  2,  3,    -1, -1, -1, -1,    -1, -1, -1, -1,     4,  5,  6,  7 } ,  // 1001 [0,x,x,1]
  { -1, -1, -1, -1,     0,  1,  2,  3,    -1, -1, -1, -1,     4,  5,  6,  7 } ,  // 1010 [x,0,x,1]
  {  0,  1,  2,  3,     4,  5,  6,  7,    -1, -1, -1, -1,     8,  9, 10, 11 } ,  // 1011 [0,1,x,2]
  { -1, -1, -1, -1,    -1, -1, -1, -1,     0,  1,  2,  3,     4,  5,  6,  7 } ,  // 1100 [x,x,0,1]
  {  0,  1,  2,  3,    -1, -1, -1, -1,     4,  5,  6,  7,     8,  9, 10, 11 } ,  // 1101 [0,x,1,2]
  { -1, -1, -1, -1,     0,  1,  2,  3,     4,  5,  6,  7,     8,  9, 10, 11 } ,  // 1110 [x,0,1,2]
  {  0,  1,  2,  3,     4,  5,  6,  7,     8,  9, 10, 11,    12, 13, 14, 15 } ,  // 1111 [0,1,2,3]
} ;

// lookup permutation table used to perform a SIMD store-compress
// elements from s[0,1,2,3] to store into d, x means fill or leave as is
static tab_16x16 __attribute__((aligned(16))) cmp_be[16] = {                                                                 // MASK
  { -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 0000 [x,x,x,x]
  { 12, 13, 14, 15,    -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 0001 [3,x,x,x]
  {  8,  9, 10, 11,    -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 0010 [2,x,x,x]
  {  8,  9, 10, 11,    12, 13, 14, 15,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 0011 [2,3,x,x]
  {  4,  5,  6,  7,    -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 0100 [1,x,x,x]
  {  4,  5,  6,  7,    12, 13, 14, 15,    -1, -1, -1, -1 ,   -1, -1, -1, -1 } ,  // 0101 [1,3,x,x]
  {  4,  5,  6,  7,     8,  9, 10, 11,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 0110 [1,2,x,x]
  {  4,  5,  6,  7,     8,  9, 10, 11,    12, 13, 14, 15,    -1, -1, -1, -1 } ,  // 0111 [1,2,3,x]
  {  0,  1,  2,  3,    -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 1000 [0,x,x,x]
  {  0,  1,  2,  3,    12, 13, 14, 15,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 1001 [0,3,x,x]
  {  0,  1,  2,  3,     8,  9, 10, 11,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 1010 [0,2,x,x]
  {  0,  1,  2,  3,     8,  9, 10, 11,    12, 13, 14, 15,    -1, -1, -1, -1 } ,  // 1011 [0,2,3,x]
  {  0,  1,  2,  3,     4,  5,  6,  7,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 1100 [0,1,x,x]
  {  0,  1,  2,  3,     4,  5,  6,  7,    12, 13, 14, 15,    -1, -1, -1, -1 } ,  // 1101 [0,1,3,x]
  {  0,  1,  2,  3,     4,  5,  6,  7,     8,  9, 10, 11,    -1, -1, -1, -1 } ,  // 1110 [0,1,2,x]
  {  0,  1,  2,  3,     4,  5,  6,  7,     8,  9, 10, 11,    12, 13, 14, 15 } ,  // 1111 [0,1,2,3]
} ;

// lookup permutation table used to perform a SIMD store-compress
// elements from s[0,1,2,3] to store into d, x means fill or leave as is
static tab_16x16 __attribute__((aligned(16))) cmp_le[16] = {                                                                 // MASK
  { -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 0000 [x,x,x,x]
  {  0,  1,  2,  3,    -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 0001 [0,x,x,x]
  {  4,  5,  6,  7,    -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 0010 [1,x,x,x]
  {  0,  1,  2,  3,     4,  5,  6,  7,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 0011 [0,1,x,x]
  {  8,  9, 10, 11,    -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 0100 [2,x,x,x]
  {  0,  1,  2,  3,     8,  9, 10, 11,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 0101 [0,2,x,x]
  {  4,  5,  6,  7,     8,  9, 10, 11,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 0110 [1,2,x,x]
  {  0,  1,  2,  3,     4,  5,  6,  7,     8,  9, 10, 11,    -1, -1, -1, -1 } ,  // 0111 [0,1,2,x]
  { 12, 13, 14, 15,    -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 1000 [3,x,x,x]
  {  0,  1,  2,  3,    12, 13, 14, 15,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 1001 [0,3,x,x]
  {  4,  5,  6,  7,    12, 13, 14, 15,    -1, -1, -1, -1 ,   -1, -1, -1, -1 } ,  // 1010 [1,3,x,x]
  {  0,  1,  2,  3,     4,  5,  6,  7,    12, 13, 14, 15,    -1, -1, -1, -1 } ,  // 1011 [0,1,3,x]
  {  8,  9, 10, 11,    12, 13, 14, 15,    -1, -1, -1, -1,    -1, -1, -1, -1 } ,  // 1100 [2,3,x,x]
  {  0,  1,  2,  3,     8,  9, 10, 11,    12, 13, 14, 15,    -1, -1, -1, -1 } ,  // 1101 [0,2,3,x]
  {  4,  5,  6,  7,     8,  9, 10, 11,    12, 13, 14, 15,    -1, -1, -1, -1 } ,  // 1110 [1,2,3,x]
  {  0,  1,  2,  3,     4,  5,  6,  7,     8,  9, 10, 11,    12, 13, 14, 15 } ,  // 1111 [0,1,2,3]
} ;

// tables to convert 4 bit nibble to Little/Big endian 128 bit mask for 4 x 32 bit elements
//                                  0 0 0 0     0 0 0 1     0 0 1 0     0 0 1 1     0 1 0 0     0 1 0 1     0 1 1 0     0 1 1 1
static uint32_t mask4_le[16] = { 0x00000000, 0x000000FF, 0x0000FF00, 0x0000FFFF, 0x00FF0000, 0x00FF00FF, 0x00FFFF00, 0x00FFFFFF,
                                 0xFF000000, 0xFF0000FF, 0xFF00FF00, 0xFF00FFFF, 0xFFFF0000, 0xFFFF00FF, 0xFFFFFF00, 0xFFFFFFFF} ;
static uint32_t mask4_be[16] = { 0x00000000, 0xFF000000, 0x00FF0000, 0xFFFF0000, 0x0000FF00, 0xFF00FF00, 0x00FFFF00, 0xFFFFFF00,
                                 0x000000FF, 0xFF0000FF, 0x00FF00FF, 0xFFFF00FF, 0x0000FFFF, 0xFF00FFFF, 0x00FFFFFF, 0xFFFFFFFF} ;

// ================================ bits interleave/detinterleave family ===============================
typedef struct{
  uint32_t l ;
  uint32_t h ;
}words2 ;

#if defined(__BMI2__)
// interleave a pair of unsigned integers into 64 bits
uint64_t interleave_32_64_bmi2(uint32_t in1, uint32_t in2)  {
    return _pdep_u64(in1, 0x5555555555555555) | _pdep_u64(in2,0xaaaaaaaaaaaaaaaa);
}
static inline uint32_t interleave_16_32_bmi2(uint32_t in1, uint32_t in2)  {
  return  _pdep_u32(in1, 0x55555555u) | _pdep_u32(in2, 0xAAAAAAAAu);
}
#endif
// interleave the lower 16 bits from unsigned integers into 32 bits
static inline uint32_t interleave_16_32_c(uint32_t in1, uint32_t in2)  {
  in1 = (in1 ^ (in1 << 8 )) & 0x00ff00ffu;
  in1 = (in1 ^ (in1 << 4 )) & 0x0f0f0f0fu;
  in1 = (in1 ^ (in1 << 2 )) & 0x33333333u;
  in1 = (in1 ^ (in1 << 1 )) & 0x55555555u;

  in2 = (in2 ^ (in2 << 8 )) & 0x00ff00ffu;
  in2 = (in2 ^ (in2 << 4 )) & 0x0f0f0f0fu;
  in2 = (in2 ^ (in2 << 2 )) & 0x33333333u;
  in2 = (in2 ^ (in2 << 1 )) & 0x55555555u;
  in2 <<= 1 ;
  return in1 | in2;
}
static inline uint32_t interleave_16_32(uint32_t in1, uint32_t in2)  {
#if !defined(__BMI2__)
  return interleave_16_32_c(in1, in2) ;
#else
  return interleave_16_32_bmi2(in1, in2) ;
#endif
}

#if defined(__BMI2__)
// deinterleave 64 bits into a pair of unsigned integers 
words2 deinterleave_64_32_bmi2(uint64_t w64)  {
  words2 t;
  t.l = _pext_u64(w64, 0x5555555555555555);
  t.h = _pext_u64(w64, 0xaaaaaaaaaaaaaaaa);
  return t;
}
// deinterleave 32 bits into the lower 16 bits of a pair of unsigned integers 
static inline words2 deinterleave_32_16_bmi2(uint32_t w32){
  words2 t ;
  t.l = (uint32_t)_pext_u32(w32, 0x55555555u);
  t.h = (uint32_t)_pext_u32(w32, 0xAAAAAAAAu);
  return t ;
}
#endif
// deinterleave 32 bits into the lower 16 bits of unsigned integers 
static inline words2 deinterleave_32_16_c(uint32_t word){
  words2 t ;
  uint32_t wl = word ;
  wl &= 0x55555555;
  wl = (wl ^ (wl >> 1 )) & 0x33333333u;
  wl = (wl ^ (wl >> 2 )) & 0x0f0f0f0fu;
  wl = (wl ^ (wl >> 4 )) & 0x00ff00ffu;
  wl = (wl ^ (wl >> 8 )) & 0x0000ffffu;
  uint32_t wh = word >> 1 ;
  wh &= 0x55555555;
  wh = (wh ^ (wh >> 1 )) & 0x33333333u;
  wh = (wh ^ (wh >> 2 )) & 0x0f0f0f0fu;
  wh = (wh ^ (wh >> 4 )) & 0x00ff00ffu;
  wh = (wh ^ (wh >> 8 )) & 0x0000ffffu;
  t.l = wl ;
  t.h = wh ;
  return t ;
}
static inline words2 deinterleave_32_16(uint32_t word){
#if !defined(__BMI2__)
  return deinterleave_32_16_c(word) ;
#else
  return deinterleave_32_16_bmi2(word) ;
#endif
}

// ================================ utility functions ===============================
// convert a binary number into a printable string of 0s and 1s
static void BinaryToString(void *w32, char *string, int ndigits){
  int i ;
  uint32_t *what = (uint32_t *) w32 ;
  uint32_t number = *what ;
  ndigits = (ndigits > 32) ? 32 : ndigits ;
  for(i=0 ; i<ndigits ; i++){
    string[ndigits-1-i] = (number & 1) ? '1' : '0' ;
    number >>= 1 ;
  }
  string[ndigits] = '\0' ;
}

// ================================ byte run length encoders/decoders family ===============================

// encoding table for 2**n repeat counts (n = 0 to 16)
// bit i will use i bits, from tab_2_n[i]
//                                 1       2       4       8      16      32      64     128
static uint32_t tab_2_n[] = { 0x0000, 0x0001, 0x0003, 0x0007, 0x000F, 0x001F, 0x003F, 0x007F,
                              0x00FF, 0x01FF, 0x03FF, 0x07FF, 0x0FFF, 0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF } ;
//                               256     512      1k      2k      4k      8k     16k     32k     64k
// compresssed stream format
// 0xxxxxxxx   : byte other that 00000000 or 11111111
// 1x0         :  1 byte of 0s (x is 0) or 1 byte of 1s (x is 1)
// 1x10        :  2 identical bytes
// 1x110       :  4 identical bytes
// 1x1110      :  8 identical bytes
// 1x11110     : 16 identical bytes
// 1x111110    : 32 identical bytes
// 1x1...10    : 2**n identical bytes (where n is the number of 1s after the x bit)
// 25 (16+8+1) identical bytes would be encoded as 1x111101x11101x0
// byte 11001010 would be encoded as 011001010
// encode byte, repeated repcount times
// if byte is other that 0x00 or 0xff, repcount is assumed to be 1
// byte     [IN] : byte to encode
// repcount [IN] : repeat count
// stream  [OUT] : bit stream to receive encoded sequence
// the function returns the number of bits inserted into the bit stream
static inline int32_t RleEncodeByte(uint8_t byte, uint32_t repcount, bitstream *stream){
  EZ_NEW_INSERT_VARS(*stream) ;
  int32_t nbits = 0 ;
  uint32_t token, token0, tokenc, countbits ;
char string[33] ;
fprintf(stderr, "encoding %3d repeated %3d times", byte, repcount) ;
  if(repcount == 0) return 0 ;
  if(byte == 0 || byte == 0xFFu){     // only 0x00 and 0xFF bytes can have a repeat count
    countbits = 0 ;
    token0 = (byte & 1) | 0b10 ;      // 0b1x
    tokenc = 0 ;
    while(repcount){                  // encode repeat bit (power of 2, starts at 1)
      if(repcount & 1){               // non zero multiplier
        token = (token0 << countbits) | tokenc ;
        token <<= 1 ;                 // trailing 0
        BE64_EZ_PUT_NBITS(token, countbits + 3) ;
BinaryToString(&token, string, countbits + 3) ;
fprintf(stderr, " %20s", string) ;
        nbits = nbits + countbits + 3 ;
      }
      repcount >>= 1 ;
      tokenc = (tokenc << 1) | 1 ;
      countbits++ ;
    }
  }else{                              // 0xxxxxxxx , arbitrary byte
    nbits += 9 ;
    token = byte ;
    BE64_EZ_PUT_NBITS(token, 9) ;
BinaryToString(&token, string, 9) ;
fprintf(stderr, " %20s", string) ;
  }
  EZ_SET_INSERT_VARS(*stream) ;
fprintf(stderr, ", nbits = %d\n", nbits);
  return nbits ;
}
// get bytes from encoded bit stream 
//   1 byte of no repeat count is present
//   variable number of bytes if a repeat coutn is found in stream
// the function returns the number of bytes decodes
static inline int32_t RleDecodeByte(uint8_t *bytes, bitstream *stream){
  int32_t i, nbytes = 1 ;
  uint32_t token, repcount ;
  uint8_t byte ;
  EZ_NEW_XTRACT_VARS(*stream) ;

  BE64_EZ_GET_NBITS(token, 1) ;
  if(token == 0){
    BE64_EZ_GET_NBITS(token, 8) ;
    byte = token ;
    nbytes = 1 ;
  }else{
fprintf(stderr, "%1d", token) ;
    BE64_EZ_GET_NBITS(token, 1) ;
fprintf(stderr, "%1d", token) ;
    byte = token ? 0xFFu : 0 ;
    nbytes = 1 ;
    BE64_EZ_GET_NBITS(token, 1) ;
fprintf(stderr, "%1d", token) ;
    while(token == 1){
      nbytes <<= 1 ;
      BE64_EZ_GET_NBITS(token, 1) ;
fprintf(stderr, "%1d", token) ;
    }
fprintf(stderr, "\n") ;
  }
  EZ_SET_XTRACT_VARS(*stream) ;
fprintf(stderr, "RleDecodeByte : byte = %4u, nbytes = %d\n", byte, nbytes) ;
  for(i=0 ; i<nbytes ; i++) bytes[i] = byte ;
  return nbytes ;
}

static inline uint32_t ByteRunLengthEncode(void *b, int32_t nbytes, void *s){
  uint8_t *bytes = (uint8_t *) b ;
  uint32_t *stream = (uint32_t *) s ;
  int32_t nbits = 0 ;
  uint32_t rep = 1024 ;
  int i ;
  while(nbytes-- > 0){  // encode nbytes bytes
  }
  return nbits ;
}

static inline uint32_t ByteRunLengthDecode(void *bytes, uint32_t nbytes, void *stream){
  return 0 ;
}

// ================================ vector mask from integer family ===============================
// 128 bit vector mask from Big Endian style nibble
static __m128i _mm_mask_be_si128(uint32_t nibble){
  __m128i v0 = _mm_loadu_si32( &mask4_be[nibble & 0xF] ) ;   // Big endian nibble to mask
  v0 = _mm_cvtepi8_epi32(v0) ;                               // 4 x 8 bits to 4 x 32 bits with sign extension
  return v0 ;
}

// 128 bit vector mask from Little Endian style nibble
static __m128i _mm_mask_le_si128(uint32_t nibble){
  __m128i v0 = _mm_loadu_si32( &mask4_le[nibble & 0xF] ) ;   // Little endian nibble to mask
  v0 = _mm_cvtepi8_epi32(v0) ;                               // 4 x 8 bits to 4 x 32 bits with sign extension
  return v0 ;
}

// ================================ BitReverse family ===============================
// bit reversal for 1 x 32 bit word
static inline uint32_t BitReverse_32(uint32_t w32){
  w32 =  (w32 >> 16)              |   (w32 << 16) ;                // swap halfwords in 32 words
  w32 = ((w32 & 0xFF00FF00) >> 8) | ( (w32 << 8) & 0xFF00FF00) ;   // swap bytes in halfwords
  w32 = ((w32 & 0xF0F0F0F0) >> 4) | ( (w32 << 4) & 0xF0F0F0F0) ;   // swap nibbles in bytes
  w32 = ((w32 & 0xCCCCCCCC) >> 2) | ( (w32 << 2) & 0xCCCCCCCC) ;   // swap 2bits in nibbles
  w32 = ((w32 & 0xAAAAAAAA) >> 1) | ( (w32 << 1) & 0xAAAAAAAA) ;   // swap bits in 2bits
  return w32 ;
}

// in place bit reversal for 4 x 32 bit word (128 bit AVX512 version)
#if defined(__x86_64__) && defined(__AVX512F__) && defined(__GFNI__)
static void BitReverse_128_avx512(void *w32){
  __m128i v0 = _mm_loadu_si128((__m128i *) w32) ;
  __m128i vs = _mm_loadu_si128((__m128i *) byte_swap_32) ;
  v0 = _mm_shuffle_epi8(v0, vs) ;              // step 1 : Perform byte swap within 32 bit word
  v0 = _mm_gf2p8affine_epi64_epi8(             // step 2 : Reverse bits within each byte
    v0,
    _mm_set1_epi64x( 0b1000000001000000001000000001000000001000000001000000001000000001 ),  // 0x8040201008040201
    0 ) ;   // We don't care to do a final `xor` to the result, so keep this 0
  _mm_storeu_si128((__m128i *) w32, v0) ;
}
#endif

// in place bit reversal for 4 x 32 bit word (128 bit AVX2 version)
#if defined(__x86_64__) && defined(__AVX2__)
static inline void BitReverse_128_avx2(void *w32){
  __m128i v0 = _mm_loadu_si128((__m128i *) w32) ;
  __m128i vs = _mm_loadu_si128((__m128i *) byte_swap_32) ;
  v0 = _mm_shuffle_epi8(v0, vs) ;              // perform byte swap on v0
  __m128i vu = _mm_loadu_si128((__m128i *) upper_nibble_reverse) ;
  __m128i vl = _mm_slli_epi32(vu, 4) ;         // lower nibble table computed from upper nibble table
  __m128i vm = _mm_set1_epi8(0xF) ;            // nibble mask
  __m128i ln = _mm_and_si128(v0, vm) ;         // lower nibble
  __m128i vt = _mm_srli_epi32(v0,4) ;          // shift upper nibble right 4 bits
  __m128i un = _mm_and_si128(vt, vm) ;         // upper nibble
  ln = _mm_shuffle_epi8(vl, ln) ;              // translate lower nibble
  un = _mm_shuffle_epi8(vu, un) ;              // translate upper nibble
  v0 = _mm_or_si128(ln, un) ;                  // reconstruct byte
  _mm_storeu_si128((__m128i *) w32, v0) ;
}
#endif

// in place bit reversal for 4 x 32 bit word (plain C version)
static inline void BitReverse_128_c(void *what){
  uint32_t *w32 = (uint32_t *) what ;
  int i ;
  for(i=0 ; i<8 ; i++){    // 4 x 32 bits
    w32[i] = BitReverse_32(w32[i]) ;
  }
}

// in place bit reversal for 4 x 32 bit word (generic version)
static inline void BitReverse_128(void *w32){
#if defined(__x86_64__) && defined(__AVX512F__)
  BitReverse_128_avx512(w32) ;
#elif defined(__x86_64__) && defined(__AVX2__)
  BitReverse_128_avx2(w32) ;
#else
  BitReverse_128_c(w32) ;
#endif
}

// ================================ MaskedMerge/MaskedFill family ===============================
// plain C versions
// merge src and scalar value into dst according to mask
static inline void MaskedFill_1_32_c_be(void *s, void *d, uint32_t be_mask, uint32_t value, int n){
  uint32_t *src = (uint32_t *) s ;
  uint32_t *dst = (uint32_t *) d ;
  n = (n>32) ? 32 : n ;
  while(n--){
    *dst++ = ((be_mask >> 31) & 1) ? *src : value ; src++ ;
    be_mask <<= 1 ;
  }
}
static inline void MaskedFill_1_32_c_le(void *s, void *d, uint32_t le_mask, uint32_t value, int n){
  uint32_t *src = (uint32_t *) s ;
  uint32_t *dst = (uint32_t *) d ;
  n = (n>32) ? 32 : n ;
  while(n--){
    *dst++ = (le_mask & 1) ? *src : value ; src++ ;
    le_mask >>= 1 ;
  }
}

// merge src and values into dst according to mask
static inline void MaskedMerge_1_32_c_be(void *s, void *d, uint32_t be_mask, void *values, int n){
  uint32_t *src = (uint32_t *) s ;
  uint32_t *dst = (uint32_t *) d ;
  uint32_t *val = (uint32_t *) values ;
  n = (n>32) ? 32 : n ;
  while(n--){
    *dst++ = ((be_mask >> 31) & 1) ? *src : *val++ ; src++ ;
    be_mask <<= 1 ;
  }
}
static inline void MaskedMerge_1_32_c_le(void *s, void *d, uint32_t le_mask, void *values, int n){
  uint32_t *src = (uint32_t *) s ;
  uint32_t *dst = (uint32_t *) d ;
  uint32_t *val = (uint32_t *) values ;
  n = (n>32) ? 32 : n ;
  while(n--){
    *dst++ = (le_mask & 1) ? *src : *val++ ; src++ ;
    le_mask >>= 1 ;
  }
}
// AVX2 versions (use plain C versions if not coded yet)
#if defined(__x86_64__) && defined(__AVX2__)
// merge src and scalar value into dst according to mask
static inline void MaskedFill_32_avx2_be(void *s, void *d, uint32_t be_mask, uint32_t value){
  uint32_t *src = (uint32_t *) s ;
  uint32_t *dst = (uint32_t *) d ;
  MaskedFill_1_32_c_be(s, d, be_mask, value, 32) ;
}
static inline void MaskedFill_32_avx2_le(void *s, void *d, uint32_t le_mask, uint32_t value){
  uint32_t *src = (uint32_t *) s ;
  uint32_t *dst = (uint32_t *) d ;
  MaskedFill_1_32_c_le(s, d, le_mask, value, 32) ;
}

// merge src and values into dst according to mask
static inline void MaskedMerge_32_avx2_be(void *s, void *d, uint32_t be_mask, void *values){
  uint32_t *src = (uint32_t *) s ;
  uint32_t *dst = (uint32_t *) d ;
  uint32_t *val = (uint32_t *) values ;
  MaskedMerge_1_32_c_be(s, d, be_mask, values, 32) ;
}
static inline void MaskedMerge_32_avx2_le(void *s, void *d, uint32_t le_mask, void *values){
  uint32_t *src = (uint32_t *) s ;
  uint32_t *dst = (uint32_t *) d ;
  uint32_t *val = (uint32_t *) values ;
  MaskedMerge_1_32_c_le(s, d, le_mask, values, 32) ;
}
#endif
// AVX512 versions (use AVX2 versions if not coded yet) (le version only for now)
#if defined(__x86_64__) && defined(__AVX512F__)
// merge src and scalar value into dst according to mask
static inline void MaskedFill_32_avx512_be(void *s, void *d, uint32_t be_mask, uint32_t value){
  uint32_t *src = (uint32_t *) s ;
  uint32_t *dst = (uint32_t *) d ;
  MaskedFill_32_avx2_be(s, d, be_mask, value) ;
}
static inline void MaskedFill_32_avx512_le(void *s, void *d, uint32_t le_mask, uint32_t value){
  uint32_t *src = (uint32_t *) s ;
  uint32_t *dst = (uint32_t *) d ;
//   MaskedFill_1_32_c_le(s, d, le_mask, value, 32) ;
  __m512i vs0 = _mm512_loadu_epi32(src     ) ;
  __m512i vs1 = _mm512_loadu_epi32(src + 16) ;
  __m512i vv0 = _mm512_set1_epi32(value) ;
  uint16_t mask0 = le_mask & 0xFFFF ;
  uint16_t mask1 = le_mask >> 16 ;
  vs0 = _mm512_mask_blend_epi32(mask0, vv0, vs0) ;
  vs1 = _mm512_mask_blend_epi32(mask1, vv0, vs1) ;
  _mm512_storeu_epi32(dst,      vs0) ;
  _mm512_storeu_epi32(dst + 16, vs1) ;
}

// merge src and values into dst according to mask
static inline void MaskedMerge_32_avx512_be(void *s, void *d, uint32_t be_mask, void *values){
  uint32_t *src = (uint32_t *) s ;
  uint32_t *dst = (uint32_t *) d ;
  uint32_t *val = (uint32_t *) values ;
  MaskedMerge_32_avx2_be(s, d, be_mask, values) ;
}
static inline void MaskedMerge_32_avx512_le(void *s, void *d, uint32_t le_mask, void *values){
  uint32_t *src = (uint32_t *) s ;
  uint32_t *dst = (uint32_t *) d ;
  uint32_t *val = (uint32_t *) values ;
//   MaskedMerge_32_avx2_le(s, d, le_mask, values) ;
  __m512i vs0 = _mm512_loadu_epi32(src     ) ;
  __m512i vs1 = _mm512_loadu_epi32(src + 16) ;
  __m512i vv0 = _mm512_loadu_epi32(val     ) ;
  __m512i vv1 = _mm512_loadu_epi32(val + 16) ;
  uint16_t mask0 = le_mask & 0xFFFF ;
  uint16_t mask1 = le_mask >> 16 ;
  vs0 = _mm512_mask_blend_epi32(mask0, vv0, vs0) ;
  vs1 = _mm512_mask_blend_epi32(mask1, vv1, vs1) ;
  _mm512_storeu_epi32(dst,      vs0) ;
  _mm512_storeu_epi32(dst + 16, vs1) ;
}
#endif

// ================================ ExpandReplace family ===============================
#if defined(__x86_64__) && defined(__AVX512F__)
static inline void *ExpandReplace_32_avx512_le(void *src, void *dst, uint32_t le_mask){
  uint32_t *w32 = (uint32_t *) src ;
  uint32_t *dest = (uint32_t *) dst ;
  __m512i vd0 = _mm512_loadu_epi32(dest     ) ;
  __m512i vd1 = _mm512_loadu_epi32(dest + 16) ;
  uint32_t mask0 = le_mask & 0xFFFF ;     // lower 16 bits of le_mask
  uint32_t mask1 = le_mask >> 16 ;        // upper 16 bits of le_mask
  __m512i vs0 = _mm512_mask_expandloadu_epi32(vd0, mask0, w32) ; w32 += popcnt_32(mask0) ;
  __m512i vs1 = _mm512_mask_expandloadu_epi32(vd0, mask1, w32) ; w32 += popcnt_32(mask1) ;
  _mm512_storeu_epi32(dest,      vs0) ;
  _mm512_storeu_epi32(dest + 16, vs1) ;
  return w32 ;
}
#endif

#if defined(__x86_64__) && defined(__AVX2__)
static inline void *ExpandReplace_32_avx2_be(void *src, void *dst, uint32_t mask){
  uint32_t *s = (uint32_t *) src ;
  uint32_t *d = (uint32_t *) dst ;
  int i ;
  __m128i vt0, vs0, vf0, vb0 ;
  uint32_t mask0, pop0 ;

  for(i=0 ; i<2 ; i++){                                            // explicit unroll by 4
      mask0 = mask >> 28 ; mask <<= 4 ; pop0 = popcnt_32(mask0) ;  // get 4 bit mask0 for this slice
      vt0 = _mm_loadu_si128((__m128i *)s) ; s += pop0 ;            // load items from source (compressed) array
//       vt0 = _mm_maskload_epi32((int const *)s, ~vs0) ;             // load using ~vs0 as a mask to avoid load beyond array
      vs0 = _mm_loadu_si128((__m128i *)(exp_be + mask0)) ;         // load permutation for this mask0
      vf0 = _mm_loadu_si128((__m128i *)d) ;                        // load items from destination (expanded) array
      vb0 = _mm_srai_epi32(vs0, 31) ;                              // 1s where keep, 0s where new from source
      vt0 = _mm_shuffle_epi8(vt0, vs0) ;                           // shufffle to get source items in proper position
      vt0 = _mm_blendv_epi8(vt0, vf0, vb0) ;                       // blend keep/new value
      _mm_storeu_si128((__m128i *)d, vt0) ; d += 4 ;               // store into destination
      mask0 = mask >> 28 ; mask <<= 4 ; pop0 = popcnt_32(mask0) ;
      vt0 = _mm_loadu_si128((__m128i *)s) ; s += pop0 ;
      vs0 = _mm_loadu_si128((__m128i *)(exp_be + mask0)) ;
      vf0 = _mm_loadu_si128((__m128i *)d) ;
      vb0 = _mm_srai_epi32(vs0, 31) ;
      vt0 = _mm_shuffle_epi8(vt0, vs0) ;
      vt0 = _mm_blendv_epi8(vt0, vf0, vb0) ;
      _mm_storeu_si128((__m128i *)d, vt0) ; d += 4 ;
      mask0 = mask >> 28 ; mask <<= 4 ; pop0 = popcnt_32(mask0) ;
      vt0 = _mm_loadu_si128((__m128i *)s) ; s += pop0 ;
      vs0 = _mm_loadu_si128((__m128i *)(exp_be + mask0)) ;
      vf0 = _mm_loadu_si128((__m128i *)d) ;
      vb0 = _mm_srai_epi32(vs0, 31) ;
      vt0 = _mm_shuffle_epi8(vt0, vs0) ;
      vt0 = _mm_blendv_epi8(vt0, vf0, vb0) ;
      _mm_storeu_si128((__m128i *)d, vt0) ; d += 4 ;
      mask0 = mask >> 28 ; mask <<= 4 ; pop0 = popcnt_32(mask0) ;
      vt0 = _mm_loadu_si128((__m128i *)s) ; s += pop0 ;
      vs0 = _mm_loadu_si128((__m128i *)(exp_be + mask0)) ;
      vf0 = _mm_loadu_si128((__m128i *)d) ;
      vb0 = _mm_srai_epi32(vs0, 31) ;
      vt0 = _mm_shuffle_epi8(vt0, vs0) ;
      vt0 = _mm_blendv_epi8(vt0, vf0, vb0) ;
      _mm_storeu_si128((__m128i *)d, vt0) ; d += 4 ;
  }
  return s ;
}

static inline void *ExpandReplace_32_avx2_le(void *src, void *dst, uint32_t mask){
  uint32_t *s = (uint32_t *) src ;
  uint32_t *d = (uint32_t *) dst ;
  int i ;
  __m128i vt0, vs0, vf0, vb0 ;
  uint32_t mask0, pop0 ;
  for(i=0 ; i<2 ; i++){                                            // explicit unroll by 4
    mask0 = mask & 0xF ; mask >>= 4 ; pop0 = popcnt_32(mask0) ;  // get 4 bit mask0 for this slice
    vt0 = _mm_loadu_si128((__m128i *)s) ; s += pop0 ;            // load items from source (compressed) array
//       vt0 = _mm_maskload_epi32((int const *)s, ~vs0) ;             // load using ~vs0 as a mask to avoid load beyond array
    vs0 = _mm_loadu_si128((__m128i *)(exp_le + mask0)) ;         // load permutation for this mask0
    vf0 = _mm_loadu_si128((__m128i *)d) ;                        // load items from destination (expanded) array
    vb0 = _mm_srai_epi32(vs0, 31) ;                              // 1s where keep, 0s where new from source
    vt0 = _mm_shuffle_epi8(vt0, vs0) ;                           // shufffle to get source items in proper position
    vt0 = _mm_blendv_epi8(vt0, vf0, vb0) ;                       // blend keep/new value
    _mm_storeu_si128((__m128i *)d, vt0) ; d += 4 ;               // store into destination
    mask0 = mask & 0xF ; mask >>= 4 ; pop0 = popcnt_32(mask0) ;  // get 4 bit mask0 for this slice
    vt0 = _mm_loadu_si128((__m128i *)s) ; s += pop0 ;
    vs0 = _mm_loadu_si128((__m128i *)(exp_le + mask0)) ;
    vf0 = _mm_loadu_si128((__m128i *)d) ;
    vb0 = _mm_srai_epi32(vs0, 31) ;
    vt0 = _mm_shuffle_epi8(vt0, vs0) ;
    vt0 = _mm_blendv_epi8(vt0, vf0, vb0) ;
    _mm_storeu_si128((__m128i *)d, vt0) ; d += 4 ;
    mask0 = mask & 0xF ; mask >>= 4 ; pop0 = popcnt_32(mask0) ;  // get 4 bit mask0 for this slice
    vt0 = _mm_loadu_si128((__m128i *)s) ; s += pop0 ;
    vs0 = _mm_loadu_si128((__m128i *)(exp_le + mask0)) ;
    vf0 = _mm_loadu_si128((__m128i *)d) ;
    vb0 = _mm_srai_epi32(vs0, 31) ;
    vt0 = _mm_shuffle_epi8(vt0, vs0) ;
    vt0 = _mm_blendv_epi8(vt0, vf0, vb0) ;
    _mm_storeu_si128((__m128i *)d, vt0) ; d += 4 ;
    mask0 = mask & 0xF ; mask >>= 4 ; pop0 = popcnt_32(mask0) ;  // get 4 bit mask0 for this slice
    vt0 = _mm_loadu_si128((__m128i *)s) ; s += pop0 ;
    vs0 = _mm_loadu_si128((__m128i *)(exp_le + mask0)) ;
    vf0 = _mm_loadu_si128((__m128i *)d) ;
    vb0 = _mm_srai_epi32(vs0, 31) ;
    vt0 = _mm_shuffle_epi8(vt0, vs0) ;
    vt0 = _mm_blendv_epi8(vt0, vf0, vb0) ;
    _mm_storeu_si128((__m128i *)d, vt0) ; d += 4 ;
  }
  return s ;
}
#endif

static void *ExpandReplace_32_c_be(void *src, void *dst, uint32_t mask){
  uint32_t *s = (uint32_t *) src ;
  uint32_t *d = (uint32_t *) dst ;
  int i ;

  for(i=0 ; i<8 ; i++){    // explicit unroll by 4
    uint32_t m31 ;
    m31 = mask >> 31 ; *d = m31 ? *s : *d ; d++ ; s += m31 ; mask <<= 1 ;
    m31 = mask >> 31 ; *d = m31 ? *s : *d ; d++ ; s += m31 ; mask <<= 1 ;
    m31 = mask >> 31 ; *d = m31 ? *s : *d ; d++ ; s += m31 ; mask <<= 1 ;
    m31 = mask >> 31 ; *d = m31 ? *s : *d ; d++ ; s += m31 ; mask <<= 1 ;
  }
  return s ;
}

static void *ExpandReplace_32_c_le(void *src, void *dst, uint32_t mask){
  uint32_t *s = (uint32_t *) src ;
  uint32_t *d = (uint32_t *) dst ;
  int i ;

  for(i=0 ; i<8 ; i++){    // explicit unroll by 4
    uint32_t m31 ;
    m31 = mask & 1 ; *d = m31 ? *s : *d ; d++ ; s += m31 ; mask >>= 1 ;
    m31 = mask & 1 ; *d = m31 ? *s : *d ; d++ ; s += m31 ; mask >>= 1 ;
    m31 = mask & 1 ; *d = m31 ? *s : *d ; d++ ; s += m31 ; mask >>= 1 ;
    m31 = mask & 1 ; *d = m31 ? *s : *d ; d++ ; s += m31 ; mask >>= 1 ;
  }
  return s ;
}

static void *ExpandReplace_0_31_c_be(void *src, void *dst, uint32_t mask, int n){
  uint32_t *s = (uint32_t *) src ;
  uint32_t *d = (uint32_t *) dst ;
  while(n--){
    uint32_t m31 = mask >> 31 ; *d = m31 ? *s : *d ; d++ ; s += m31 ; mask <<= 1 ;
  }
  return s ;
}

static void *ExpandReplace_0_31_c_le(void *src, void *dst, uint32_t mask, int n){
  uint32_t *s = (uint32_t *) src ;
  uint32_t *d = (uint32_t *) dst ;
  while(n--){
    uint32_t m31 = mask & 1 ; *d = m31 ? *s : *d ; d++ ; s += m31 ; mask >>= 1 ;
  }
  return s ;
}

// ================================ ExpandFill family ===============================
#if defined(__x86_64__) && defined(__AVX512F__)
static inline void *ExpandFill_32_avx512_le(void *src, void *dst, uint32_t le_mask, uint32_t fill){
  uint32_t *w32 = (uint32_t *) src ;
  uint32_t *dest = (uint32_t *) dst ;
  __m512i vf0 = _mm512_set1_epi32(fill) ;
  uint32_t mask0 = le_mask & 0xFFFF ;     // lower 16 bits of le_mask
  uint32_t mask1 = le_mask >> 16 ;        // upper 16 bits of le_mask
  __m512i vs0 = _mm512_mask_expandloadu_epi32(vf0, mask0, w32) ; w32 += popcnt_32(mask0) ;
  __m512i vs1 = _mm512_mask_expandloadu_epi32(vf0, mask1, w32) ; w32 += popcnt_32(mask1) ;
  _mm512_storeu_epi32(dest,      vs0) ;
  _mm512_storeu_epi32(dest + 16, vs1) ;
  return w32 ;
}
#endif

#if defined(__x86_64__) && defined(__AVX2__)
static inline void * ExpandFill_32_avx2_be(void *src, void *dst, uint32_t be_mask, uint32_t fill){
  uint32_t *s = (uint32_t *) src ;
  uint32_t *d = (uint32_t *) dst ;
  int i ;
  __m128i vt0, vs0, vf0, vb0 ;

  vf0 = _mm_set1_epi32(fill) ;                                   // fill value
  for(i=0 ; i<2 ; i++){                                          // explicit unroll by 4
    uint32_t mask0, pop0 ;
    mask0 = be_mask >> 28 ; be_mask <<= 4 ; pop0 = popcnt_32(mask0) ;  // get 4 bit mask0 for this slice
    vt0 = _mm_loadu_si128((__m128i *)s) ; s += pop0 ;            // load items from source (compressed) array
//     vt0 = _mm_maskload_epi32((int const *)s, ~vs0) ;             // load using ~vs0 as a mask to avoid load beyond array
    vs0 = _mm_loadu_si128((__m128i *)(exp_be + mask0)) ;         // load permutation for this mask0
    vb0 = _mm_srai_epi32(vs0, 31) ;                              // 1s where fill, 0s where new from source
    vt0 = _mm_shuffle_epi8(vt0, vs0) ;                           // shufffle to get source items in proper position
    vt0 = _mm_blendv_epi8(vt0, vf0, vb0) ;                       // blend in fill value
    _mm_storeu_si128((__m128i *)d, vt0) ; d += 4 ;               // store into destination
    mask0 = be_mask >> 28 ; be_mask <<= 4 ; pop0 = popcnt_32(mask0) ;
    vt0 = _mm_loadu_si128((__m128i *)s) ; s += pop0 ;
    vs0 = _mm_loadu_si128((__m128i *)(exp_be + mask0)) ;
    vb0 = _mm_srai_epi32(vs0, 31) ;
    vt0 = _mm_shuffle_epi8(vt0, vs0) ;
    vt0 = _mm_blendv_epi8(vt0, vf0, vb0) ;
    _mm_storeu_si128((__m128i *)d, vt0) ; d += 4 ;
    mask0 = be_mask >> 28 ; be_mask <<= 4 ; pop0 = popcnt_32(mask0) ;
    vt0 = _mm_loadu_si128((__m128i *)s) ; s += pop0 ;
    vs0 = _mm_loadu_si128((__m128i *)(exp_be + mask0)) ;
    vb0 = _mm_srai_epi32(vs0, 31) ;
    vt0 = _mm_shuffle_epi8(vt0, vs0) ;
    vt0 = _mm_blendv_epi8(vt0, vf0, vb0) ;
    _mm_storeu_si128((__m128i *)d, vt0) ; d += 4 ;
    mask0 = be_mask >> 28 ; be_mask <<= 4 ; pop0 = popcnt_32(mask0) ;
    vt0 = _mm_loadu_si128((__m128i *)s) ; s += pop0 ;
    vs0 = _mm_loadu_si128((__m128i *)(exp_be + mask0)) ;
    vb0 = _mm_srai_epi32(vs0, 31) ;
    vt0 = _mm_shuffle_epi8(vt0, vs0) ;
    vt0 = _mm_blendv_epi8(vt0, vf0, vb0) ;
    _mm_storeu_si128((__m128i *)d, vt0) ; d += 4 ;
  }
  return s ;
}

static inline void *ExpandFill_32_avx2_le(void *src, void *dst, uint32_t le_mask, uint32_t fill){
  uint32_t *s = (uint32_t *) src ;
  uint32_t *d = (uint32_t *) dst ;
  int i ;
  __m128i vt0, vs0, vf0, vb0 ;

  vf0 = _mm_set1_epi32(fill) ;                                   // fill value
  for(i=0 ; i<2 ; i++){                                          // explicit unroll by 4
    uint32_t mask0, pop0 ;
    mask0 = le_mask & 0xF ; le_mask >>= 4 ; pop0 = popcnt_32(mask0) ;  // get 4 bit mask0 for this slice
    vt0 = _mm_loadu_si128((__m128i *)s) ; s += pop0 ;            // load items from source (compressed) array
//     vt0 = _mm_maskload_epi32((int const *)s, ~vs0) ;             // load using ~vs0 as a le_mask to avoid load beyond array
    vs0 = _mm_loadu_si128((__m128i *)(exp_le + mask0)) ;         // load permutation for this mask0
    vb0 = _mm_srai_epi32(vs0, 31) ;                              // 1s where fill, 0s where new from source
    vt0 = _mm_shuffle_epi8(vt0, vs0) ;                           // shufffle to get source items in proper position
    vt0 = _mm_blendv_epi8(vt0, vf0, vb0) ;                       // blend in fill value
    _mm_storeu_si128((__m128i *)d, vt0) ; d += 4 ;               // store into destination
    mask0 = le_mask & 0xF ; le_mask >>= 4 ; pop0 = popcnt_32(mask0) ;  // get 4 bit mask0 for this slice
    vt0 = _mm_loadu_si128((__m128i *)s) ; s += pop0 ;
    vs0 = _mm_loadu_si128((__m128i *)(exp_le + mask0)) ;
    vb0 = _mm_srai_epi32(vs0, 31) ;
    vt0 = _mm_shuffle_epi8(vt0, vs0) ;
    vt0 = _mm_blendv_epi8(vt0, vf0, vb0) ;
    _mm_storeu_si128((__m128i *)d, vt0) ; d += 4 ;
    mask0 = le_mask & 0xF ; le_mask >>= 4 ; pop0 = popcnt_32(mask0) ;  // get 4 bit mask0 for this slice
    vt0 = _mm_loadu_si128((__m128i *)s) ; s += pop0 ;
    vs0 = _mm_loadu_si128((__m128i *)(exp_le + mask0)) ;
    vb0 = _mm_srai_epi32(vs0, 31) ;
    vt0 = _mm_shuffle_epi8(vt0, vs0) ;
    vt0 = _mm_blendv_epi8(vt0, vf0, vb0) ;
    _mm_storeu_si128((__m128i *)d, vt0) ; d += 4 ;
    mask0 = le_mask & 0xF ; le_mask >>= 4 ; pop0 = popcnt_32(mask0) ;  // get 4 bit mask0 for this slice
    vt0 = _mm_loadu_si128((__m128i *)s) ; s += pop0 ;
    vs0 = _mm_loadu_si128((__m128i *)(exp_le + mask0)) ;
    vb0 = _mm_srai_epi32(vs0, 31) ;
    vt0 = _mm_shuffle_epi8(vt0, vs0) ;
    vt0 = _mm_blendv_epi8(vt0, vf0, vb0) ;
    _mm_storeu_si128((__m128i *)d, vt0) ; d += 4 ;
  }
  return s ;
}
#endif    // defined(__AVX2__)

static inline void *ExpandFill_32_c_be(void *src, void *dst, uint32_t be_mask, uint32_t fill){
  uint32_t *s = (uint32_t *) src ;
  uint32_t *d = (uint32_t *) dst ;
  int i ;
  for(i=0 ; i<8 ; i++){    // explicit unroll by 4
    uint32_t m31 ;
    m31 = be_mask >> 31 ; *d = m31 ? *s : fill ; d++ ; s += m31 ; be_mask <<= 1 ;
    m31 = be_mask >> 31 ; *d = m31 ? *s : fill ; d++ ; s += m31 ; be_mask <<= 1 ;
    m31 = be_mask >> 31 ; *d = m31 ? *s : fill ; d++ ; s += m31 ; be_mask <<= 1 ;
    m31 = be_mask >> 31 ; *d = m31 ? *s : fill ; d++ ; s += m31 ; be_mask <<= 1 ;
  }
  return s ;
}

static inline void *ExpandFill_32_c_le(void *src, void *dst, uint32_t le_mask, uint32_t fill){
  uint32_t *s = (uint32_t *) src ;
  uint32_t *d = (uint32_t *) dst ;
  int i ;
  for(i=0 ; i<8 ; i++){    // explicit unroll by 4
    uint32_t m31 ;
    m31 = le_mask & 1 ; *d = m31 ? *s : fill ; d++ ; s += m31 ; le_mask >>= 1 ;
    m31 = le_mask & 1 ; *d = m31 ? *s : fill ; d++ ; s += m31 ; le_mask >>= 1 ;
    m31 = le_mask & 1 ; *d = m31 ? *s : fill ; d++ ; s += m31 ; le_mask >>= 1 ;
    m31 = le_mask & 1 ; *d = m31 ? *s : fill ; d++ ; s += m31 ; le_mask >>= 1 ;
  }
  return s ;
}

static inline void * ExpandFill_0_31_c_be(void *src, void *dst, uint32_t be_mask, uint32_t fill, int n){
  uint32_t *s = (uint32_t *) src ;
  uint32_t *d = (uint32_t *) dst ;
  while(n--){
    uint32_t m31 = be_mask >> 31 ; *d = m31 ? *s : fill ; d++ ; s += m31 ; be_mask <<= 1 ;
  }
  return s ;
}

static inline void * ExpandFill_0_31_c_le(void *src, void *dst, uint32_t le_mask, uint32_t fill, int n){
  uint32_t *s = (uint32_t *) src ;
  uint32_t *d = (uint32_t *) dst ;
  while(n--){
    uint32_t m31 = le_mask & 1 ; *d = m31 ? *s : fill ; d++ ; s += m31 ; le_mask >>= 1 ;
  }
  return s ;
}

// ================================ CompressStore family ===============================
#if defined(__x86_64__) && defined(__AVX2__)
// store-compress 32 items according to mask using AVX2 instructions
static inline void *CompressStore_32_avx2_be(void *src, void *dst, uint32_t be_mask){
  uint32_t *s = (uint32_t *) src ;
  uint32_t *d = (uint32_t *) dst ;
  uint32_t pop0, mask0, i ;
  __m128i vt0, vs0 ;
  for(i=0 ; i<2 ; i++){                                    // explicit unroll by 4
    mask0 = be_mask >> 28 ; be_mask <<= 4 ;
    vt0 = _mm_loadu_si128((__m128i *)s) ;  s += 4 ;        // load items from source array
    vs0 = _mm_loadu_si128((__m128i *)(cmp_be + mask0)) ;   // load permutation for this mask0
    vt0 = _mm_shuffle_epi8(vt0, vs0) ;                     // shufffle to get source items in proper position
    pop0 = popcnt_32(mask0) ;                              // number of useful items that will be stored
    _mm_storeu_si128((__m128i *)d, vt0) ; d += pop0 ;      // store into destination
//     _mm_maskmoveu_si128(vt0, ~vs0, (char *)d) ;            // mask store, using ~vs0 as mask
    mask0 = be_mask >> 28 ; be_mask <<= 4 ;
    vt0 = _mm_loadu_si128((__m128i *)s) ;  s += 4 ;
    vs0 = _mm_loadu_si128((__m128i *)(cmp_be + mask0)) ;
    vt0 = _mm_shuffle_epi8(vt0, vs0) ;
    pop0 = popcnt_32(mask0) ;
    _mm_storeu_si128((__m128i *)d, vt0) ; d += pop0 ;
    mask0 = be_mask >> 28 ; be_mask <<= 4 ;
    vt0 = _mm_loadu_si128((__m128i *)s) ;  s += 4 ;
    vs0 = _mm_loadu_si128((__m128i *)(cmp_be + mask0)) ;
    vt0 = _mm_shuffle_epi8(vt0, vs0) ;
    pop0 = popcnt_32(mask0) ;
    _mm_storeu_si128((__m128i *)d, vt0) ; d += pop0 ;
    mask0 = be_mask >> 28 ; be_mask <<= 4 ;
    vt0 = _mm_loadu_si128((__m128i *)s) ;  s += 4 ;
    vs0 = _mm_loadu_si128((__m128i *)(cmp_be + mask0)) ;
    vt0 = _mm_shuffle_epi8(vt0, vs0) ;
    pop0 = popcnt_32(mask0) ;
    _mm_storeu_si128((__m128i *)d, vt0) ; d += pop0 ;
  }
  return d ;
}
static inline void *CompressStore_32_avx2_le(void *src, void *dst, uint32_t le_mask){
  uint32_t *s = (uint32_t *) src ;
  uint32_t *d = (uint32_t *) dst ;
  uint32_t pop0, mask0, i ;
  __m128i vt0, vs0 ;
  for(i=0 ; i<2 ; i++){                                    // explicit unroll by 4, 16 values per loop iteration
    mask0 = le_mask & 0xF ; le_mask >>= 4 ;
    vt0 = _mm_loadu_si128((__m128i *)s) ;  s += 4 ;        // load items from source array
    vs0 = _mm_loadu_si128((__m128i *)(cmp_le + mask0)) ;   // load permutation for this mask0
    vt0 = _mm_shuffle_epi8(vt0, vs0) ;                     // shufffle to get source items in proper position
    pop0 = popcnt_32(mask0) ;                              // number of useful items that will be stored
    _mm_storeu_si128((__m128i *)d, vt0) ; d += pop0 ;      // store into destination
    mask0 = le_mask & 0xF ; le_mask >>= 4 ;
    vt0 = _mm_loadu_si128((__m128i *)s) ;  s += 4 ;
    vs0 = _mm_loadu_si128((__m128i *)(cmp_le + mask0)) ;
    vt0 = _mm_shuffle_epi8(vt0, vs0) ;
    pop0 = popcnt_32(mask0) ;
    _mm_storeu_si128((__m128i *)d, vt0) ; d += pop0 ;
    mask0 = le_mask & 0xF ; le_mask >>= 4 ;
    vt0 = _mm_loadu_si128((__m128i *)s) ;  s += 4 ;
    vs0 = _mm_loadu_si128((__m128i *)(cmp_le + mask0)) ;
    vt0 = _mm_shuffle_epi8(vt0, vs0) ;
    pop0 = popcnt_32(mask0) ;
    _mm_storeu_si128((__m128i *)d, vt0) ; d += pop0 ;
    mask0 = le_mask & 0xF ; le_mask >>= 4 ;
    vt0 = _mm_loadu_si128((__m128i *)s) ;  s += 4 ;
    vs0 = _mm_loadu_si128((__m128i *)(cmp_le + mask0)) ;
    vt0 = _mm_shuffle_epi8(vt0, vs0) ;
    pop0 = popcnt_32(mask0) ;
    _mm_storeu_si128((__m128i *)d, vt0) ; d += pop0 ;
  }
  return d ;
}
#endif

#if defined(__x86_64__) && defined(__AVX512F__)
// store-compress 32 items according to mask (AVX512 code)
// mask is little endian style, src[0] is controlled by the mask LSB
static void *CompressStore_32_avx512_le(void *src, void *dst, uint32_t le_mask){
  uint32_t *w32 = (uint32_t *) src ;
  uint32_t *dest = (uint32_t *) dst ;
  __m512i vd0 = _mm512_loadu_epi32(w32) ;
  __m512i vd1 = _mm512_loadu_epi32(w32+16) ;
  vd0 = _mm512_maskz_compress_epi32(le_mask & 0xFFFF, vd0) ; // in register compress
  _mm512_storeu_epi32(dest, vd0) ;
//   _mm512_mask_compressstoreu_epi32 (dest, le_mask & 0xFFFF, vd0) ;
  dest += _mm_popcnt_u32(le_mask & 0xFFFF) ;                 // bits 0-15
  vd1 = _mm512_maskz_compress_epi32(le_mask >> 16,  vd1) ;   // in register compress
  _mm512_storeu_epi32(dest, vd1) ;
//   _mm512_mask_compressstoreu_epi32 (dest, le_mask >> 16,  vd1) ;
  dest += _mm_popcnt_u32(le_mask >> 16) ;                    // bits 16-31
  return dest ;
}
#endif

// store-compress 32 items according to mask (plain C code)
// mask is big endian style, src[0] is controlled by the mask MSB
static inline void *CompressStore_32_c_be(void *src, void *dst, uint32_t be_mask){
  uint32_t *w32 = (uint32_t *) src ;
  uint32_t *dest = (uint32_t *) dst ;
  int i ;
  for(i=0 ; i<8 ; i++){    // explicit unroll by 4
    *dest = *w32++ ;
    dest += (be_mask >> 31) ;
    be_mask <<= 1 ;
    *dest = *w32++ ;
    dest += (be_mask >> 31) ;
    be_mask <<= 1 ;
    *dest = *w32++ ;
    dest += (be_mask >> 31) ;
    be_mask <<= 1 ;
    *dest = *w32++ ;
    dest += (be_mask >> 31) ;
    be_mask <<= 1 ;
  }
  return dest ;
}

// store-compress 32 items according to mask (plain C code)
// mask is little endian style, src[0] is controlled by the mask LSB
static inline void *CompressStore_32_c_le(void *src, void *dst, uint32_t le_mask){
  uint32_t *w32 = (uint32_t *) src ;
  uint32_t *dest = (uint32_t *) dst ;
  int i ;
  for(i=0 ; i<8 ; i++){    // explicit unroll by 4
    *dest = *w32++ ;
    dest += (le_mask & 1) ;
    le_mask >>= 1 ;
    *dest = *w32++ ;
    dest += (le_mask & 1) ;
    le_mask >>= 1 ;
    *dest = *w32++ ;
    dest += (le_mask & 1) ;
    le_mask >>= 1 ;
    *dest = *w32++ ;
    dest += (le_mask & 1) ;
    le_mask >>= 1 ;
  }
  return dest ;
}

// store-compress n ( 0 <= n < 32) items according to mask
// mask is big endian style, src[0] is controlled by the mask MSB
static inline void *CompressStore_0_31_c_be(void *src, void *dst, uint32_t be_mask, int nw32){
  uint32_t *w32 = (uint32_t *) src ;
  uint32_t *dest = (uint32_t *) dst ;
  if(nw32 > 31 || nw32 < 0) return NULL ;
  while(nw32--){
    *dest = *w32++ ;
    dest += (be_mask >> 31) ;
    be_mask <<= 1 ;
  }
  return dest ;
}

// src    [IN] : "full" array
// dst [OUT] : non masked elements from "full" array
// le_mask     [IN] : 32 bits mask, if bit[I] is 1, src[I] is stored, if 0, it is skipped
// the function returns the next address to be stored into for the "dst" array
// NULL in case of error nw32 > 31 or nw32 < 0
// the mask is interpreted Little Endian style, src[0] is controlled by the mask LSB
// this version processes 0 - 31 elements from "src"
static void *CompressStore_0_31_c_le(void *src, void *dst, uint32_t le_mask, int nw32){
  uint32_t *w32 = (uint32_t *) src ;
  uint32_t *dest = (uint32_t *) dst ;
//   int i ;
  if(nw32 > 31 || nw32 < 0) return NULL ;
  while(nw32--){
    *dest = *w32++ ;
    dest += (le_mask & 1) ;
    le_mask >>=1 ;
  }
  return dest ;
}

void *CompressStore_be(void *src, void *dst, void *le_map, int nw32);
void *CompressStore_c_be(void *s_, void *d_, void *map_, int n);
void *CompressStore_avx2_be(void *src, void *dst, void *be_mask, int n);

void *CompressStore_le(void *src, void *dst, void *le_map, int nw32);
void *CompressStore_c_le(void *s_, void *d_, void *map_, int n);
void *CompressStore_avx2_le(void *src, void *dst, void *be_mask, int n);
void *CompressStore_avx512_le(void *src, void *dst, void *le_mask, int n);


void ExpandReplace_be(void *s, void *d, void *map, int n);
void ExpandReplace_c_be(void *s, void *d, void *map, int n);
void ExpandReplace_avx2_be(void *s, void *d, void *map, int n);

void ExpandReplace_le(void *s, void *d, void *map, int n);
void ExpandReplace_c_le(void *s, void *d, void *map, int n);
void ExpandReplace_avx2_le(void *s, void *d, void *map, int n);
void ExpandReplace_avx512_le(void *s, void *d, void *map, int n);

void ExpandFill_be(void *s_, void *d_, void *map_, int n, void *fill_);
void ExpandFill_c_be(void *s_, void *d_, void *map_, int n, void *fill_);
void ExpandFill_avx2_be(void *s_, void *d_, void *map_, int n, void *fill_);

void ExpandFill_le(void *s_, void *d_, void *map_, int n, void *fill_);
void ExpandFill_c_le(void *s_, void *d_, void *map_, int n, void *fill_);
void ExpandFill_avx2_le(void *s_, void *d_, void *map_, int n, void *fill_);
void ExpandFill_avx512_le(void *s_, void *d_, void *map_, int n, void *fill_);

void MaskedMerge_le(void *s, void *d, uint32_t le_mask, void *values, int n);
void MaskedMerge_c_le(void *s, void *d, uint32_t le_mask, void *values, int n);
void MaskedMerge_avx2_le(void *s, void *d, uint32_t le_mask, void *values, int n);
void MaskedMerge_avx512_le(void *s, void *d, uint32_t le_mask, void *values, int n);

void MaskedFill_le(void *s, void *d, uint32_t le_mask, uint32_t value, int n);
void MaskedFill_c_le(void *s, void *d, uint32_t le_mask, uint32_t value, int n);
void MaskedFill_avx2_le(void *s, void *d, uint32_t le_mask, uint32_t value, int n);
void MaskedFill_avx512_le(void *s, void *d, uint32_t le_mask, uint32_t value, int n);

int32_t MaskGreater_c_be(void *src1, int nsrc1, void *src2, int nsrc2, uint32_t *mask, int negate);
int32_t MaskGreater_c_le(void *src1, int nsrc1, void *src2, int nsrc2, void *mask, int negate);
int32_t MaskGreater_avx2_le(void *src1, int nsrc1, void *src2, int nsrc2, void *mask, int negate);
int32_t MaskGreater_avx512_le(void *src1, int nsrc1, void *src2, int nsrc2, void *mask, int negate);

#endif
