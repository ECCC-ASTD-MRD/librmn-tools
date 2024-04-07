// Hopefully useful functions for C and FORTRAN
// Copyright (C) 2023-2024  Recherche en Prevision Numerique
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
// N.B. the AVX2 versions are 2-3 x faster than the plain C versions (compiler dependent)
//
#include <stdio.h>
// #undef __AVX2__
// #undef __AVX512F__
#include <rmn/compress_expand.h>
#include <rmn/bits.h>

static uint32_t word32 = 1 ;
static uint8_t *le = (uint8_t *) &word32 ;
// ================================ Copy_items family ===============================
// copy variable length (8/16/32/64 bit) items into 8/16/32/64 bit containers
// larger item filled from right to left _r2l functions)
// larger item filled from left to right _l2r functions)
// BIG ENDIAN CODE IS UNTESTED
//
// src     [IN] : pointer to source data
// srclen  [IN] : length in bytes of src elements (1/2/4/8)
// dst    [OUT] : pointer to destination
// dstlen  [IN] : length in bytes of dst elements (1/2/4/8)
// ns      [IN] : number of src elements to cpy into dst
int Copy_items_r2l(void *src, uint32_t srclen, void *dst, uint32_t dstlen, uint32_t ns){
  uint32_t nd = (ns*srclen + dstlen - 1) / dstlen ;
  if( (_mm_popcnt_u32(srclen) > 1) || (_mm_popcnt_u32(dstlen) > 1) || (srclen > 8) || (dstlen > 8) ) return -1 ;
  if(*le){                                   // little endian host
    if(src != dst){                          // in place is a NO-OP
      memcpy(dst, src, ns*srclen) ;          // straight copy on a little endian host
    }
  }else{                                     // big endian host
    if(srclen == dstlen){                    // same length for source and destination elements
      if(src != dst) memcpy(dst, src, ns*srclen) ;
      return nd = ns ;
    }
    return -1 ;                              // BIG ENDIAN VERSION NOT IMPLEMENTED YET
  }
  return nd ;
}
int Copy_items_l2r(void *src, uint32_t srclen, void *dst, uint32_t dstlen, uint32_t ns){
  uint32_t nd ;
  uint8_t  *s8  = (uint8_t  *)src, *d8  = (uint8_t  *)dst, t8 ;
  uint16_t *s16 = (uint16_t *)src, *d16 = (uint16_t *)dst, t16 ;
  uint32_t *s32 = (uint32_t *)src, *d32 = (uint32_t *)dst, t32 ;
  uint64_t *s64 = (uint64_t *)src, *d64 = (uint64_t *)dst, t64 ;
  int i, j ;
  if( (_mm_popcnt_u32(srclen) > 1) || (_mm_popcnt_u32(dstlen) > 1) || (srclen > 8) || (dstlen > 8) ) return -1 ;

  if(srclen == dstlen){                      // same length for source and destination elements
    if(src != dst) {                         // in place is a NO-OP
      memcpy(dst, src, ns*srclen) ;           // straight copy
    }
    return nd = ns ;
  }

  if(*le){                                   // little endian host
    switch(dstlen) {
      case 1 :
        nd = ns * srclen ;
        switch (srclen){
          case 2 :       // 16 bits to 8 bits
            for(i=0 ; i<ns ; i++) { d8[0] = *s16 >> 8 ; d8[1] = *s16 ; s16++ ; d8 += 2 ; } ;
            break ;
          case 4 :       // 32 bits to 8 bits
            for(i=0 ; i<ns ; i++) { d8[0] = *s32>>24 ; d8[1] = *s32>>16 ; d8[2] = *s32 >> 8 ; d8[3] = *s32 ; s32++ ; d8 += 4 ; } ;
            break ;
          case 8 :       // 64 bits to 8 bits
            for(i=0 ; i<ns ; i++) { d8[0] = *s64>>56 ; d8[1] = *s64>>48 ; d8[2] = *s64>>40 ; d8[3] = *s64>>32 ;
                                    d8[4] = *s64>>24 ; d8[5] = *s64>>16 ; d8[6] = *s64>>8  ; d8[7] = *s64 ; s64++ ; d8 += 8 ; } ;
            break ;
        }
        break ;

      case 2 :
        switch (srclen){
          case 1 :       // 8 bits to 16 bits
            nd = (ns+1) / 2 ;
            for(i=0 ; i<ns ; i++) { t16 = s8[0] ; t16 = (t16 << 8) | s8[1] ; *d16 = t16 ; d16++ ; s8 += 2 ; }
            break ;
          case 4 :       // 32 bits to 16 bits
            nd = ns * 2 ;
            for(i=0 ; i<ns ; i++) { d16[0] = *s32 >> 16 ; d16[1] = *s32 ; s32++ ; d16 += 2 ; }
            break ;
          case 8 :       // 64 bits to 16 bits
            nd = ns * 4 ;
            for(i=0 ; i<ns ; i++) { d16[0] = *s64>>48 ; d16[1] = *s64>>32 ; d16[2] = *s64>>16 ; d16[3] = *s64 ; s64++ ; d16 += 4 ; }
            break ;
        }
        break ;

      case 4 :
        switch (srclen){
          case 1 :       // 8 bits to 32 bits
            nd = (ns+3) / 4 ;
            for(i=0 ; i<nd ; i++) { t32 = (s8[0] << 24) | (s8[1] << 16) | (s8[2] << 8) | s8[3] ; *d32 = t32 ; d32++ ; s8 += 4 ; }
            break ;
          case 2 :       // 16 bits to 32 bits
            nd = (ns+1) / 2 ;
            for(i=0 ; i<nd ; i++) { t32 = (s16[0] << 16) | s16[1] ; *d32 = t32 ; d32++ ; s32 += 2 ; }
            break ;
          case 8 :       // 64 bits to 32 bits
            nd = ns * 2 ;
            for(i=0 ; i<ns ; i++) { d32[0] = *s64 >> 32 ; d32[1] = *s64 ; s64++ ; d32 += 2 ; }
            break ;
        }
        break ;

      case 8 :
        switch (srclen){
          case 1 :       // 8 bits to 64 bits
            nd = (ns+7) / 8 ;
            for(i=0 ; i<nd ; i++) { t64 = s8[0] ;          t64 = (t64<<8)|s8[1] ; t64 = (t64<<8)|s8[2] ; t64 = (t64<<8)|s8[3] ;
                                    t64 = (t64<<8)|s8[4] ; t64 = (t64<<8)|s8[5] ; t64 = (t64<<8)|s8[6] ; t64 = (t64<<8)|s8[7] ;
                                    *d64 = t64 ; d64++ ; s8 += 8 ; }
            break ;
          case 2 :       // 16 bits to 64 bits
            nd = (ns+3) / 4 ;
            for(i=0 ; i<nd ; i++) { t64 = s16[0] ; t64 = (t64<<16)|s16[1] ; t64 = (t64<<16)|s16[2] ; t64 = (t64<<16)|s16[3] ;
                                    *d64 = t64 ; t64++ ; s16 += 4 ; }
            break ;
          case 4 :       // 32 bits to 64 bits
            nd = (ns+1) / 2 ;
            for(i=0 ; i<nd ; i++) { t64 = s32[0] ; t64 = (t64<<32)|s32[1] ; *d64 = t64 ; t64++ ; s32 += 2 ; }
            break ;
        }
        break ;
    }
  }else{                                     // big endian host
    if(src != dst){                          // in place is a NO-OP
      memcpy(dst, src, ns*srclen) ;           // straight copy on a big endian host
    }
  }
  return nd ;
}
// ================================ Put_n_into_m family ===============================
// larger item filled from right to left _r2l functions)
// larger item filled from left to right _l2r functions)
// BIG ENDIAN CODE IS UNTESTED
//
// pack n 8 bit items into 32 bit words
// s8   [IN] : pointer to an array of 1 byte items
// n    [IN] : number of 1 byte items
// d32 [OUT] : pointer to an array of 32 bit (4 byte) words
// return the number of 32 bit words used
int Put_8_into_32_r2l(void *s8, uint32_t n, void *d32){    // fill d32 from right (LSB) to left (MSB)
  uint32_t  nw = (n+3)/4 ;

  if(*le){                                   // little endian host
    if(s8 != d32){                           // in place is a NO-OP
      memcpy(d32, s8, n*sizeof(uint8_t)) ;   // straight copy on a little endian host
    }
  }else{                                     // big endian host
    uint8_t *c8   = (uint8_t *)s8 ;
    uint32_t *i32 = (uint32_t *)d32 ;
    while(nw--){
      uint32_t t ;    // t = c8[3] c8[2] c8[1] c8[0]
      t = c8[3] ; t = (t << 8) | c8[2] ; t = (t << 8) | c8[1] ; t = (t << 8) | c8[0] ;
      *i32 = t ; i32++ ; c8 += 4 ;
    }
  }
  return nw ;
}
int Put_8_into_32_l2r(void *s8, uint32_t n, void *d32){    // fill d32 from left (MSB) to right (LSB)
  uint32_t  nw = (n+3)/4 ;

  if(*le){                                   // little endian host
    uint8_t *c8   = (uint8_t *)s8 ;
    uint32_t *i32 = (uint32_t *)d32 ;
    while(nw--){
      uint32_t t ;    // t = c8[0] c8[1] c8[2] c8[3]
      t = c8[0] ; t = (t << 8) | c8[1] ; t = (t << 8) | c8[2] ; t = (t << 8) | c8[3] ;
      *i32 = t ; i32++ ; c8 += 4 ;
    }
  }else{                                     // big endian host
    if(s8 != d32){                           // in place is a NO-OP
      memcpy(d32, s8, n*sizeof(uint8_t)) ;   // straight copy on a big endian host
    }
  }
  return nw ;
}
// pack n 16 bit items into 32 bit words
// s16  [IN] : pointer to an array of 2 byte items
// n    [IN] : number of 2 byte items
// d32 [OUT] : pointer to an array of 32 bit (4 byte) words
// return the number of 32 bit words used
int Put_16_into_32_r2l(void *s16, uint32_t n, void *d32){    // fill d32 from right (LSB) to left (MSB)
  uint32_t  nw = (n+1)/2 ;

  if(*le){                                   // little endian host
    if(s16 != d32){                          // in place is a NO-OP
      memcpy(d32, s16, n*sizeof(uint16_t)) ; // straight copy on a little endian host
    }
  }else{                                     // big endian host
    uint16_t *h16 = (uint16_t *)s16 ;
    uint32_t *i32 = (uint32_t *)d32 ;
    while(nw--){
      uint32_t t ;    // t = h16[1] h16[0]
      t = h16[1] ; t = (t << 16) | h16[0] ;
      *i32 = t ; i32++ ; h16 += 2 ;
    }
  }
  return nw ;
}
int Put_16_into_32_l2r(void *s16, uint32_t n, void *d32){    // fill d32 from left (MSB) to right (LSB)
  uint32_t  nw = (n+1)/2 ;

  if(*le){                                   // little endian host
    uint16_t *h16 = (uint16_t *)s16 ;
    uint32_t *i32 = (uint32_t *)d32 ;
    while(nw--){
      uint32_t t ;    // t = h16[0] h16[1]
      t = h16[0] ; t = (t << 16) | h16[1] ;
      *i32 = t ; i32++ ; h16 += 2 ;
    }
  }else{                                     // big endian host
    if(s16 != d32){                          // in place is a NO-OP
      memcpy(d32, s16, n*sizeof(uint16_t)) ; // straight copy on a big endian host
    }
  }
  return nw ;
}
// "pack" n 64 bit items into 32 bit words
// s64  [IN] : pointer to an array of 8 byte items
// n    [IN] : number of 8 byte items
// d32 [OUT] : pointer to an array of 32 bit (4 byte) words
// return the number of 32 bit words used
int Put_64_into_32_r2l(void *s64, uint32_t n, void *d32){    // right part (LSB) of s64, then left part (MSB)
  uint32_t  nw = n * 2 ;

  if(*le){                                   // little endian host
    if(s64 != d32){                          // in place is a NO-OP
      memcpy(d32, s64, n*sizeof(uint64_t)) ; // straight copy on a little endian host
    }
  }else{                                     // big endian host
    uint64_t *l64 = (uint64_t *)s64 ;
    uint32_t *i32 = (uint32_t *)d32 ;
    while(n--){
      i32[0] = *l64 & 0xFFFFFFFFu ;          // lower part first
      i32[1] = *l64 >> 32 ;                  // upper part after
      i32 += 2 ; l64++ ;
    }
  }
  return nw ;
}
int Put_64_into_32_l2r(void *s64, uint32_t n, void *d32){    // left part (MSB) of s64, then right part (LSB)
  uint32_t  nw = n * 2 ;

  if(*le){                                   // little endian host
    uint64_t *l64 = (uint64_t *)s64 ;
    uint32_t *i32 = (uint32_t *)d32 ;
    while(n--){
      i32[0] = *l64 >> 32 ;                  // upper part first
      i32[1] = *l64 & 0xFFFFFFFFu ;          // lower part after
      i32 += 2 ; l64++ ;
    }
  }else{                                     // big endian host
    if(s64 != d32){                          // in place is a NO-OP
      memcpy(d32, s64, n*sizeof(uint64_t)) ; // straight copy on a big endian host
    }
  }
  return nw ;
}
// ================================ Get_n_from_m family ===============================
// unpack n 8 bit items from 32 bit words
// d8  [OUT] : pointer to an array of 1 byte items
// n    [IN] : number of 1 byte items
// s32  [IN] : pointer to an array of 32 bit (4 byte) words
// return the number of 32 bit words used
int Get_8_from_32_r2l(void *d8, uint32_t n, void *s32){
  uint32_t  nw = (n+3)/4 ;

  if(*le){                                   // little endian host
    if(d8 != s32){                           // in place is a NO-OP
      memcpy(d8, s32, n*sizeof(uint8_t)) ;   // straight copy on a little endian host
    }
  }else{                                     // big endian host
    uint8_t *c8   = (uint8_t *)d8 ;
    uint32_t *i32 = (uint32_t *)s32 ;
    while(nw--){
      uint32_t t = *i32 ;    // t = c8[3] c8[2] c8[1] c8[0]
      c8[0] = t ; c8[1] = t >> 8 ;  c8[2] = t >> 16 ;  c8[3] = t >> 24 ;
      i32++ ; c8 += 4 ;
    }
  }
  return nw ;
}
int Get_8_from_32_l2r(void *d8, uint32_t n, void *s32){
  uint32_t  nw = (n+3)/4 ;

  if(*le){                                   // little endian host
    uint8_t *c8   = (uint8_t *)d8 ;
    uint32_t *i32 = (uint32_t *)s32 ;
    while(nw--){
      uint32_t t = *i32 ;    // t = c8[0] c8[1] c8[2] c8[3]
      c8[3] = t ; c8[2] = t >> 8 ;  c8[1] = t >> 16 ;  c8[0] = t >> 24 ;
      i32++ ; c8 += 4 ;
    }
  }else{                                     // big endian host
    if(d8 != s32){                           // in place is a NO-OP
      memcpy(d8, s32, n*sizeof(uint8_t)) ;   // straight copy on a big endian host
    }
  }
  return nw ;
}
// unpack n 16 bit items from 32 bit words
// d16 [OUT] : pointer to an array of 2 byte items
// n    [IN] : number of 2 byte items
// s32  [IN] : pointer to an array of 32 bit (4 byte) words
// return the number of 32 bit words used
int Get_16_from_32_r2l(void *d16, uint32_t n, void *s32){
  uint32_t  nw = (n+1)/2 ;

  if(*le){                                   // little endian host
    if(d16 != s32){                          // in place is a NO-OP
      memcpy(d16, s32, n*sizeof(uint16_t)) ; // straight copy on a little endian host
    }
  }else{                                     // big endian host
    uint16_t *h16 = (uint16_t *)d16 ;
    uint32_t *i32 = (uint32_t *)s32 ;
    while(nw--){
      uint32_t t = *i32 ;    // t = h16[1] h16[0]
      h16[0] = t ; h16[1] = t >> 16 ;
      i32++ ; h16 += 2 ;
    }
  }
  return nw ;
}
int Get_16_from_32_l2r(void *d16, uint32_t n, void *s32){
  uint32_t  nw = (n+1)/2 ;

  if(*le){                                   // little endian host
    uint16_t *h16 = (uint16_t *)d16 ;
    uint32_t *i32 = (uint32_t *)s32 ;
    while(nw--){
      uint32_t t = *i32 ;    // t = h16[0] h16[1]
      h16[0] = t >> 16 ; h16[1] = t ;
      i32++ ; h16 += 2 ;
    }
  }else{                                     // big endian host
    if(d16 != s32){                          // in place is a NO-OP
      memcpy(d16, s32, n*sizeof(uint16_t)) ; // straight copy on a big endian host
    }
  }
  return nw ;
}
// "unpack" n 64 bit items from 32 bit words
// d64 [OUT] : pointer to an array of 8 byte items
// n    [IN] : number of 8 byte items
// s32  [IN] : pointer to an array of 32 bit (4 byte) words
// return the number of 32 bit words used
int Get_64_from_32_r2l(void *d64, uint32_t n, void *s32){
  uint32_t  nw = n * 2 ;

  if(*le){                                   // little endian host
    if(d64 != s32){                          // in place is a NO-OP
      memcpy(d64, s32, n*sizeof(uint64_t)) ; // straight copy on a little endian host
    }
  }else{                                     // big endian host
    uint64_t *l64 = (uint64_t *)d64 ;
    uint32_t *i32 = (uint32_t *)s32 ;
    while(n--){
      uint64_t t ;    // i32[1] i32[0]
      t = i32[0] ; t = t << 32 ; t |= i32[1] ;
      *l64 = t ;
      i32 += 2 ; l64++ ;
    }
  }
  return nw ;
}
int Get_64_from_32_l2r(void *d64, uint32_t n, void *s32){
  uint32_t  nw = n * 2 ;

  if(*le){                                   // little endian host
    uint64_t *l64 = (uint64_t *)d64 ;
    uint32_t *i32 = (uint32_t *)s32 ;
    while(n--){
      uint64_t t = *l64 ;    // i32[0] i32[1]
      i32[0] = t >> 32 ; i32[1] = t ;
      i32 += 2 ; l64++ ;
    }
  }else{                                     // big endian host
    if(d64 != s32){                          // in place is a NO-OP
      memcpy(d64, s32, n*sizeof(uint64_t)) ; // straight copy on a big endian host
    }
  }
  return nw ;
}
// ================================ MaskCompress family ===============================
// copy from s to d where mask0[i:i] == 1  (bit i, bit 0 is LSB)
static uint32_t *Mask0copy_c_be(uint32_t *s, uint32_t *d, uint32_t mask0){
  int i ;
  for(i=0 ; mask0 != 0 ; i++){
    *d = *s ;
    s++ ;
    d += (mask0 & 1) ;
    mask0 >>= 1 ;
  }
  return d ;
}
// compute mask with 1s where s[i] == value, 0s otherwise
// n MUST BE < 33
static uint32_t Mask0EqualValue_c_be(uint32_t *s, uint32_t value, int n){
  uint32_t mask0 = 0, m1 = 0x80000000u ;
  while(n-- > 0) {                          // n < 33 value slices
    mask0 |= ( (*s == value) ? m1 : 0 ) ;   // or m1 if src1 greater than src2
    m1 >>= 1 ;                              // next bit to the right toward LSB
    s++ ;                                   // increment pointer
  }
  return mask0 ;
}
// source  [IN] : first vector of values (or single value if nsrc1 == 1)
// nsource [IN] : number of values in src1
// value   [IN] : second vector of values (or single value if nsrc2 == 1)
// mask   [OUT] : mask, 1s where src1[i] > src2[i], 0s otherwise
// comp   [OUT] : vector of compressed values (where mask bit is 1)
// negate  [IN] : invert test, produce 0s where src1[i] > src2[i], 1s otherwise
int32_t MaskEqualCompress_c_be(void *source, int nsource, void *value, void *mask, void *comp, int negate){
  uint32_t *src = (uint32_t *) source ;
  uint32_t *ref = (uint32_t *) value ;
  uint32_t ref0 = *ref ;
  uint32_t *msk = (uint32_t *) mask ;
  uint32_t *dst = (uint32_t *) comp ;
  uint32_t complement = ( negate ? 0xFFFFFFFFu : 0 ), mask0, m1 ;
  int nmask = 0, i0 ;

  if(nsource <= 0) return -1 ;
  for(i0=0 ; i0 < (nsource-31) ; i0 += 32){
    mask0 = Mask0EqualValue_c_be(source, ref0, 32) ^ complement ;   // negate mask if necessary
    nmask += popcnt_32(mask0) ;                // count 1s in masks
    *msk++ = mask0 ;
    source += 32 ;
  }

  return nmask ;
}

// ================================ MaskEqual ===============================
// src1   [IN] vector of 32 values (32 bit) (any of float/signed integer/usigned integer)
// nsrc1  [IN] number of values in src1 (MUST BE same as nsrc2 or 1)
// src2   [IN] vector of 32 values (32 bit) (any of float/signed integer/usigned integer)
// nsrc2  [IN] number of values in src2 (MUST BE same as nsrc1 or 1)
// mask  [OUT] comparison mask
// negate [IN] if non zero, complement result mask
// compute a 32 bit mask, 1s where src1[i] OP src2[i], 0s otherwise
// return number of 1s in mask


// src1   [IN] : first vector of values (or single value if nsrc1 == 1)
// nsrc1  [IN] : number of values in src1
// src2   [IN] : second vector of values (or single value if nsrc2 == 1)
// nsrc2  [IN] : number of values in src2
// mask  [OUT] : mask, 1s where src1[i] > src2[i], 0s otherwise
// negate [IN] : inverse test, produce 0s where src1[i] > src2[i], 1s otherwise
// N.B. nsrc1 MUST be equal to nsrc1 unless nsrc1 ==1 or nsrc2 == 1
//      src1 and src2 are expected to be any of float/signed integer/usigned integer (32 bits)
// return number of 1s in mask
// _be function : Big Endian style, [0] is MSB
// _le function : Little Endian style, [0] is LSB
// plain C version
static uint32_t Mask0Equal_c_be(uint32_t *s1, int inc1, uint32_t *s2, int inc2, int n){
  uint32_t mask0 = 0, m1 = 0x80000000u ;
  while(n-- > 0) {                           // n < 33 value slices
    mask0 |= ( (*s1 == *s2) ? m1 : 0 ) ;     // or m1 if src1 greater than src2
    m1 >>= 1 ;                               // next bit to the right toward LSB
    s1 += inc1 ; s2 += inc2 ;                // add increments to pointers
  }
  return mask0 ;
}
// int32_t MaskEqual_c_be(void *src1, int nsrc1, void *src2, int nsrc2, void *mask, int negate){
//   uint32_t *s1 = (uint32_t *) src1 ;
//   uint32_t *s2 = (uint32_t *) src2 ;
//   uint32_t *mk = (uint32_t *) mask ;
//   uint32_t complement = ( negate ? 0xFFFFFFFFu : 0 ), mask0, m1 ;
//   int i0, i, inc1 = ((nsrc1 > 1) ? 1 : 0), inc2 = ((nsrc2 > 1) ? 1 : 0) ;
//   int n = (nsrc1 > nsrc2) ? nsrc1 : nsrc2 ;
//   int nmask = 0 ;
// 
//   if(nsrc1 != n && nsrc1 != 1) return -1 ;
//   if(nsrc2 != n && nsrc2 != 1) return -1 ;
// 
//   for(i0=0 ; i0 < (n-31) ; i0 += 32){
//     mask0 = Mask0Equal_c_be(s1, inc1, s2, inc2, 32) ^ complement ;   // negate mask if necessary
//     nmask += popcnt_32(mask0) ;                // count 1s in masks
//     *mk++ = mask0 ;
//     s1 += inc1*32 ; s2 += inc2*32 ;
//   }
//   if(n-i0 > 0) {
//     int scount = 32 - (n-i0) ;
//     mask0 = Mask0Equal_c_be(s1, inc1, s2, inc2, n-i0) ^ complement ;   // negate mask if necessary
//     mask0 = (mask0 >> scount) << scount ;        // get rid of extra 1s from negate
//     nmask += popcnt_32(mask0) ;                  // count 1s in masks
//     *mk = mask0 ;
//   }
//   return nmask ;
// }
static uint32_t Mask0Equal_c_le(uint32_t *s1, int inc1, uint32_t *s2, int inc2, int n){
  uint32_t mask0 = 0, m1 = 1 ;               // start with LSB
  while(n-- > 0) {                           // n < 33 value slices
    mask0 |= ( (*s1 == *s2) ? m1 : 0 ) ;      // or m1 if src1 greater than src2
    m1 <<= 1 ;                               // next bit to the left toward MSB
    s1 += inc1 ; s2 += inc2 ;                // add increments to pointers
  }
  return mask0 ;
}
int32_t MaskEqual_c_le(void *src1, int nsrc1, void *src2, int nsrc2, void *mask, int negate){
  uint32_t *s1 = (uint32_t *) src1 ;
  uint32_t *s2 = (uint32_t *) src2 ;
  uint32_t *mk = (uint32_t *) mask ;
  uint32_t complement = ( negate ? 0xFFFFFFFFu : 0 ), mask0, m1 ;
  int i0, i, inc1 = ((nsrc1 > 1) ? 1 : 0), inc2 = ((nsrc2 > 1) ? 1 : 0) ;
  int n = (nsrc1 > nsrc2) ? nsrc1 : nsrc2 ;
  int nmask = 0 ;

  if(nsrc1 != n && nsrc1 != 1) return -1 ;
  if(nsrc2 != n && nsrc2 != 1) return -1 ;

  for(i0=0 ; i0 < (n-31) ; i0 += 32){
    mask0 = Mask0Equal_c_le(s1, inc1, s2, inc2, 32) ^ complement ;   // negate mask if necessary
    s1 += inc1*32 ; s2 += inc2*32 ;
    nmask += popcnt_32(mask0) ;
    *mk++ = mask0 ;   // negate xor is MISSING
  }
  if(n-i0 > 0) {
    int scount = 32 - (n-i0) ;
    mask0 = Mask0Equal_c_le(s1, inc1, s2, inc2, n-i0) ^ complement ;   // negate mask if necessary
    mask0 = (mask0 << scount) >> scount ;        // get rid of extra 1s from negate
    nmask += popcnt_32(mask0) ;                  // count 1s in masks
    *mk = mask0 ;
  }
  return nmask ;
}
#if defined(__x86_64__) && defined(__AVX2__)
// AVX2 version
// int32_t MaskEqual_avx2_be(void *src1, int nsrc1, void *src2, int nsrc2, uint32_t *mask, int negate){
//   return MaskEqual_c_be(src1, nsrc1, src2, nsrc2, mask, negate) ;  // plain C version for now
// }
int32_t MaskEqual_avx2_le(void *src1, int nsrc1, void *src2, int nsrc2, void *mask, int negate){
  uint32_t *s1 = (uint32_t *) src1, *s2 = (uint32_t *) src2 ;
  uint8_t *mk = (uint8_t *) mask ;
  uint32_t complement = ( negate ? 0xFFFFFFFFu : 0 ), mask0, m1 ;
  uint8_t complement8 = ( negate ? 0xFFu : 0 ) ;
  int n = (nsrc1 > nsrc2) ? nsrc1 : nsrc2 ;
  int nmask = 0 ;
  int i0, i, inc1 = ((nsrc1 > 1) ? 1 : 0), inc2 = ((nsrc2 > 1) ? 1 : 0) ;
  __m256i v1a, v1b, v1c, v1d, v2a, v2b, v2c, v2d, vm1, vm2, vc1, vc2 ;

  if(nsrc1 != n && nsrc1 != 1) return -1 ;
  if(nsrc2 != n && nsrc2 != 1) return -1 ;

  vc1 = _mm256_set1_epi32((nsrc1 == 1) ? *s1 : 0 ) ;   // s1[0] broadcasted if single value, 0 otherwise
  vm1 = _mm256_set1_epi32(nsrc1 > 1 ? -1 : 0) ;
  vc2 = _mm256_set1_epi32((nsrc2 == 1) ? *s2 : 0 ) ;   // s2[0] broadcasted if single value, 0 otherwise
  vm2 = _mm256_set1_epi32(nsrc2 > 1 ? -1 : 0) ;
  // vc1 contains 0 if s1 is not single valued (nsrc1 > 1), a broadcast of s1[0] if single valued
  // vm1 contains 0 if s1 is single valued, all 1s otherwise
  // v1x loaded with vm1 as a mask contains 0 if s1 is single valued, a vector of s1 values otherwise
  // v1x ORed with vc1 will contain s1[0] or a vector of s1 values
  // same for vc2/vm2/v2x
  for(i0=0 ; i0 < (n-31) ; i0 += 32){                               // 4 x 8 element slices
    v1a =  _mm256_maskload_epi32((int32_t *)(s1   ), vm1) | vc1 ;   // v1x = vector of s1 if nsrc1 > 1, 0 otherwise
    v1b =  _mm256_maskload_epi32((int32_t *)(s1+ 8), vm1) | vc1 ;   // vm1 = 0 if nsrc1 > 1, *s1 otherwise
    v1c =  _mm256_maskload_epi32((int32_t *)(s1+16), vm1) | vc1 ;   // vector or scalar according to nsrc1 > 1
    v1d =  _mm256_maskload_epi32((int32_t *)(s1+24), vm1) | vc1 ;
    v2a =  _mm256_maskload_epi32((int32_t *)(s2   ), vm2) | vc2 ;   // vector of s2 if nsrc2 > 1, 0 otherwise
    v2b =  _mm256_maskload_epi32((int32_t *)(s2+ 8), vm2) | vc2 ;   // vm2 = 0 if nsrc2 > 1, *s1 otherwise
    v2c =  _mm256_maskload_epi32((int32_t *)(s2+16), vm2) | vc2 ;   // vector or scalar according to nsrc2 > 1
    v2d =  _mm256_maskload_epi32((int32_t *)(s2+24), vm2) | vc2 ;
    v1a = _mm256_cmpeq_epi32(v1a, v2a) ;
    v1b = _mm256_cmpeq_epi32(v1b, v2b) ;
    v1c = _mm256_cmpeq_epi32(v1c, v2c) ;
    v1d = _mm256_cmpeq_epi32(v1d, v2d) ;
    mk[0] = _mm256_movemask_ps((__m256) v1a) ^ complement8 ;   // negate mask if necessary
    mk[1] = _mm256_movemask_ps((__m256) v1b) ^ complement8 ;   // negate mask if necessary
    mk[2] = _mm256_movemask_ps((__m256) v1c) ^ complement8 ;   // negate mask if necessary
    mk[3] = _mm256_movemask_ps((__m256) v1d) ^ complement8 ;   // negate mask if necessary
    nmask = nmask + _mm_popcnt_u32(mk[0]) + _mm_popcnt_u32(mk[1]) + _mm_popcnt_u32(mk[2]) + _mm_popcnt_u32(mk[3]) ;
    s1 += 32*inc1 ;
    s2 += 32*inc2 ;
  }
  if(n-i0 > 0){
    int scount = 32 - (n-i0) ;
    mask0 = Mask0Equal_c_le(s1, inc1, s2, inc2, n-i0) ^ complement ;  // negate mask if necessary
    mask0 = (mask0 << scount) >> scount ;                             // get rid of extra 1s from negate
    nmask += popcnt_32(mask0) ;
    mk[0] = (mask0      ) & 0xFF ;
    mk[1] = (mask0 >>  8) & 0xFF ;
    mk[2] = (mask0 >> 16) & 0xFF ;
    mk[3] = (mask0 >> 24) & 0xFF ;
  }
  return nmask ;
}
#endif
#if defined(__x86_64__) && defined(__AVX512F__)
// AVX512 version
// int32_t MaskEqual_avx512_be(void *src1, int nsrc1, void *src2, int nsrc2, void *mask, int negate){
//   return MaskEqual_c_be(src1, nsrc1, src2, nsrc2, mask, negate) ;  // plain C version for now
// }
int32_t MaskEqual_avx512_le(void *src1, int nsrc1, void *src2, int nsrc2, void *mask, int negate){
  uint32_t *s1 = (uint32_t *)src1, *s2 = (uint32_t *)src2 ;
  uint32_t mask0, m1 ;
  uint8_t *mk = (uint8_t *)mask ;
  uint16_t complement16 = ( negate ? 0xFFFFu : 0 ) ;
  uint32_t complement   = ( negate ? 0xFFFFFFFFu : 0 ) ;
  int n = (nsrc1 > nsrc2) ? nsrc1 : nsrc2, nmask = 0 ;
  int i0, i, inc1 = ((nsrc1 > 1) ? 1 : 0), inc2 = ((nsrc2 > 1) ? 1 : 0) ;
  int incv1 = ((nsrc1 > 1) ? 32 : 0), incv2 = ((nsrc2 > 1) ? 32 : 0) ;
  uint16_t mb1 = ((nsrc1 == 1) ? 0 : 0xFFFFu ), mb2 = ((nsrc2 == 1) ? 0 : 0xFFFFu ) ;
  __m512i v1a, v2a, v1b, v2b, vb1, vb2 ;

  if(nsrc1 != n && nsrc1 != 1) return -1 ;
  if(nsrc2 != n && nsrc2 != 1) return -1 ;

  vb1 = _mm512_set1_epi32(*s1) ;               // src1 single valued
  vb2 = _mm512_set1_epi32(*s2) ;               // src2 single valued
  for(i0=0 ; i0 < (n-31) ; i0 += 32){          // 2 x 16 element slices
    v1a =  _mm512_mask_loadu_epi32(vb1, mb1, s1   ) ;
    v2a =  _mm512_mask_loadu_epi32(vb2, mb2, s2   ) ;
    v1b =  _mm512_mask_loadu_epi32(vb1, mb1, s1+16) ;
    v2b =  _mm512_mask_loadu_epi32(vb2, mb2, s2+16) ;
    uint16_t k1 = _mm512_cmpeq_epi32_mask(v1a, v2a) ^ complement16 ;   // negate mask if necessary
    uint16_t k2 = _mm512_cmpeq_epi32_mask(v1b, v2b) ^ complement16 ;   // negate mask if necessary
    nmask = nmask + _mm_popcnt_u32(k1) + _mm_popcnt_u32(k2) ;
    mk[0] = (k1 & 0xFF) ;
    mk[1] = (k1 >> 16) ;
    mk[2] = (k2 & 0xFF) ;
    mk[3] = (k2 >> 16) ;
    s1 += incv1 ; s2 += incv2 ; mk += 4 ;      // add increments to pointers
  }
  if(n-i0 > 0){
    int scount = 32 - (n-i0) ;
    mask0 = Mask0Equal_c_le(s1, inc1, s2, inc2, n-i0) ^ complement ;  // negate mask if necessary
    mask0 = (mask0 << scount) >> scount ;                             // get rid of extra 1s from negate
    nmask += popcnt_32(mask0) ;
    mk[0] = (mask0      ) & 0xFF ;
    mk[1] = (mask0 >>  8) & 0xFF ;
    mk[2] = (mask0 >> 16) & 0xFF ;
    mk[3] = (mask0 >> 24) & 0xFF ;
  }
  return nmask ;
}
#endif

// ================================= MaskGreater ===============================
// src1   [IN] : first vector of values (or single value if nsrc1 == 1)
// nsrc1  [IN] : number of values in src1
// src2   [IN] : second vector of values (or single value if nsrc2 == 1)
// nsrc2  [IN] : number of values in src2
// mask  [OUT] : mask, 1s where src1[i] > src2[i], 0s otherwise
// negate [IN] : inverse test, produce 0s where src1[i] > src2[i], 1s otherwise
// N.B. nsrc1 MUST be equal to nsrc1 unless nsrc1 ==1 or nsrc2 == 1
//      src1 and src2 are expected to be SIGNED integers
// return number of 1s in mask
// _be function : Big Endian style, [0] is MSB
// _le function : Little Endian style, [0] is LSB
// plain C version
static inline uint32_t Mask0Greater_c_be(int32_t *s1, int inc1, int32_t *s2, int inc2, int n){
  uint32_t mask0 = 0, m1 = 0x80000000u ;
  while(n-- > 0) {                           // n < 33 value slices
    mask0 |= ( (*s1 > *s2) ? m1 : 0 ) ;      // or m1 if src1 greater than src2
    m1 >>= 1 ;                               // next bit to the right toward LSB
    s1 += inc1 ; s2 += inc2 ;                // add increments to pointers
  }
  return mask0 ;
}
int32_t MaskGreater_c_be(void *src1, int nsrc1, void *src2, int nsrc2, void *mask, int negate){
  int32_t *s1 = (int32_t *) src1 ;
  int32_t *s2 = (int32_t *) src2 ;
  uint32_t *mk = (uint32_t *) mask ;
  uint32_t complement = ( negate ? 0xFFFFFFFFu : 0 ), mask0, m1 ;
  int i0, i, inc1 = ((nsrc1 > 1) ? 1 : 0), inc2 = ((nsrc2 > 1) ? 1 : 0) ;
  int n = (nsrc1 > nsrc2) ? nsrc1 : nsrc2 ;
  int nmask = 0 ;

  if(nsrc1 != n && nsrc1 != 1) return -1 ;
  if(nsrc2 != n && nsrc2 != 1) return -1 ;

  for(i0=0 ; i0 < (n-31) ; i0 += 32){
    mask0 = Mask0Greater_c_be(s1, inc1, s2, inc2, 32) ;
    mask0 ^= complement ;                      // negate mask if necessary (xor with 0 or FFFFFFFF)
    nmask += popcnt_32(mask0) ;                // count 1s in masks
    *mk++ = mask0 ;
  }
  if(n-i0 > 0){
    int scount = 32 - (n-i0) ;
    mask0 = Mask0Greater_c_be(s1, inc1, s2, inc2, n-i0) ^ complement ; // negate mask if necessary
    mask0 = (mask0 >> scount) << scount ;                             // get rid of extra 1s from negate
    nmask += popcnt_32(mask0) ;                  // count 1s in masks
    *mk = mask0 ;
  }
  return nmask ;
}
static inline uint32_t Mask0Greater_c_le(int32_t *s1, int inc1, int32_t *s2, int inc2, int n){
  uint32_t mask0 = 0, m1 = 1 ;               // start with LSB
  while(n-- > 0) {                           // n < 33 value slices
    mask0 |= ( (*s1 > *s2) ? m1 : 0 ) ;      // or m1 if src1 greater than src2
    m1 <<= 1 ;                               // next bit to the left toward MSB
    s1 += inc1 ; s2 += inc2 ;                // add increments to pointers
  }
  return mask0 ;
}
int32_t MaskGreater_c_le(void *src1, int nsrc1, void *src2, int nsrc2, void *mask, int negate){
  int32_t *s1 = (int32_t *) src1 ;
  int32_t *s2 = (int32_t *) src2 ;
  uint32_t *mk = (uint32_t *) mask ;
  uint32_t complement = ( negate ? 0xFFFFFFFFu : 0 ), mask0, m1 ;
  int i0, i, inc1 = ((nsrc1 > 1) ? 1 : 0), inc2 = ((nsrc2 > 1) ? 1 : 0) ;
  int n = (nsrc1 > nsrc2) ? nsrc1 : nsrc2 ;
  int nmask = 0 ;

  if(nsrc1 != n && nsrc1 != 1) return -1 ;
  if(nsrc2 != n && nsrc2 != 1) return -1 ;

  for(i0=0 ; i0 < (n-31) ; i0 += 32){
    mask0 = Mask0Greater_c_le(s1, inc1, s2, inc2, 32) ^ complement ;
    s1 += inc1*32 ; s2 += inc2*32 ;
    nmask += popcnt_32(mask0) ;
    *mk++ = mask0 ;
  }
  if(n-i0 > 0){
    int scount = 32 - (n-i0) ;
    mask0 = Mask0Greater_c_le(s1, inc1, s2, inc2, n-i0) ^ complement ;
    mask0 = (mask0 << scount) >> scount ;                             // get rid of extra 1s from negate
    nmask += popcnt_32(mask0) ;
    *mk = mask0 ;
  }
  return nmask ;
}
#if defined(__x86_64__) && defined(__AVX2__)
// AVX2 version
int32_t MaskGreater_avx2_be(void *src1, int nsrc1, void *src2, int nsrc2, uint32_t *mask, int negate){
  return MaskGreater_c_be(src1, nsrc1, src2, nsrc2, mask, negate) ;
}
int32_t MaskGreater_avx2_le(void *src1, int nsrc1, void *src2, int nsrc2, void *mask, int negate){
  int32_t *s1 = (int32_t *) src1, *s2 = (int32_t *) src2 ;
  uint8_t *mk = (uint8_t *) mask ;
  uint32_t complement = ( negate ? 0xFFFFFFFFu : 0 ), mask0, m1 ;
  uint8_t complement8 = ( negate ? 0xFFu : 0 );
  int n = (nsrc1 > nsrc2) ? nsrc1 : nsrc2 ;
  int nmask = 0 ;
  int i0, i, inc1 = ((nsrc1 > 1) ? 1 : 0), inc2 = ((nsrc2 > 1) ? 1 : 0) ;
  __m256i v1a, v1b, v1c, v1d, v2a, v2b, v2c, v2d, vm1, vm2, vc1, vc2 ;

  if(nsrc1 != n && nsrc1 != 1) return -1 ;
  if(nsrc2 != n && nsrc2 != 1) return -1 ;

  vc1 = _mm256_set1_epi32((nsrc1 == 1) ? *s1 : 0 ) ;   // s1[0] broadcasted if single value, 0 otherwise
  vm1 = _mm256_set1_epi32(nsrc1 > 1 ? -1 : 0) ;
  vc2 = _mm256_set1_epi32((nsrc2 == 1) ? *s2 : 0 ) ;   // s2[0] broadcasted if single value, 0 otherwise
  vm2 = _mm256_set1_epi32(nsrc2 > 1 ? -1 : 0) ;
  // vc1 contains 0 if s1 is not single valued (nsrc1 > 1), a broadcast of s1[0] if single valued
  // vm1 contains 0 if s1 is single valued, all 1s otherwise
  // v1x loaded with vm1 as a mask contains 0 if s1 is single valued, a vector of s1 values otherwise
  // v1x ORed with vc1 will contain s1[0] or a vector of s1 values
  // same for vc2/vm2/v2x
  for(i0=0 ; i0 < (n-31) ; i0 += 32){                               // 4 x 8 element slices
    v1a =  _mm256_maskload_epi32((int32_t *)(s1   ), vm1) | vc1 ;   // v1x = vector of s1 if nsrc1 > 1, 0 otherwise
    v1b =  _mm256_maskload_epi32((int32_t *)(s1+ 8), vm1) | vc1 ;   // vm1 = 0 if nsrc1 > 1, *s1 otherwise
    v1c =  _mm256_maskload_epi32((int32_t *)(s1+16), vm1) | vc1 ;   // vector or scalar according to nsrc1 > 1
    v1d =  _mm256_maskload_epi32((int32_t *)(s1+24), vm1) | vc1 ;
    v2a =  _mm256_maskload_epi32((int32_t *)(s2   ), vm2) | vc2 ;   // vector of s2 if nsrc2 > 1, 0 otherwise
    v2b =  _mm256_maskload_epi32((int32_t *)(s2+ 8), vm2) | vc2 ;   // vm2 = 0 if nsrc2 > 1, *s1 otherwise
    v2c =  _mm256_maskload_epi32((int32_t *)(s2+16), vm2) | vc2 ;   // vector or scalar according to nsrc2 > 1
    v2d =  _mm256_maskload_epi32((int32_t *)(s2+24), vm2) | vc2 ;
    v1a = _mm256_cmpgt_epi32(v1a, v2a) ;
    v1b = _mm256_cmpgt_epi32(v1b, v2b) ;
    v1c = _mm256_cmpgt_epi32(v1c, v2c) ;
    v1d = _mm256_cmpgt_epi32(v1d, v2d) ;
    mk[0] = _mm256_movemask_ps((__m256) v1a) ^ complement8 ;
    mk[1] = _mm256_movemask_ps((__m256) v1b) ^ complement8 ;
    mk[2] = _mm256_movemask_ps((__m256) v1c) ^ complement8 ;
    mk[3] = _mm256_movemask_ps((__m256) v1d) ^ complement8 ;
    nmask = nmask + _mm_popcnt_u32(mk[0]) + _mm_popcnt_u32(mk[1]) + _mm_popcnt_u32(mk[2]) + _mm_popcnt_u32(mk[3]) ;
    s1 += 32*inc1 ;
    s2 += 32*inc2 ;
  }
  if(n-i0 > 0){
    int scount = 32 - (n-i0) ;
    mask0 = Mask0Greater_c_le(s1, inc1, s2, inc2, n-i0) ^ complement ; // negate mask if necessary
    mask0 = (mask0 << scount) >> scount ;                              // get rid of extra 1s from negate
    nmask += popcnt_32(mask0) ;
    *mk = mask0 ;
  }
  return nmask ;
}
#endif
#if defined(__x86_64__) && defined(__AVX512F__)
// AVX512 version
int32_t MaskGreater_avx512_be(void *src1, int nsrc1, void *src2, int nsrc2, void *mask, int negate){
  return MaskGreater_c_be(src1, nsrc1, src2, nsrc2, mask, negate) ;
}
int32_t MaskGreater_avx512_le(void *src1, int nsrc1, void *src2, int nsrc2, void *mask, int negate){
  int32_t *s1 = (int32_t *)src1, *s2 = (int32_t *)src2 ;
  uint32_t mask0, m1 ;
  uint8_t *mk = (uint8_t *)mask ;
  uint16_t complement16 = ( negate ? 0xFFFFu : 0 ) ;
  uint32_t complement   = ( negate ? 0xFFFFFFFFu : 0 ) ;
  int n = (nsrc1 > nsrc2) ? nsrc1 : nsrc2, nmask = 0 ;
  int i0, i, inc1 = ((nsrc1 > 1) ? 1 : 0), inc2 = ((nsrc2 > 1) ? 1 : 0) ;
  int incv1 = ((nsrc1 > 1) ? 32 : 0), incv2 = ((nsrc2 > 1) ? 32 : 0) ;
  uint16_t mb1 = ((nsrc1 == 1) ? 0 : 0xFFFFu ), mb2 = ((nsrc2 == 1) ? 0 : 0xFFFFu ) ;
  __m512i v1a, v2a, v1b, v2b, vb1, vb2 ;

  if(nsrc1 != n && nsrc1 != 1) return -1 ;
  if(nsrc2 != n && nsrc2 != 1) return -1 ;

  vb1 = _mm512_set1_epi32(*s1) ;               // src1 single valued
  vb2 = _mm512_set1_epi32(*s2) ;               // src2 single valued
  for(i0=0 ; i0 < (n-31) ; i0 += 32){          // 2 x 16 element slices
    v1a =  _mm512_mask_loadu_epi32(vb1, mb1, s1   ) ;
    v2a =  _mm512_mask_loadu_epi32(vb2, mb2, s2   ) ;
    v1b =  _mm512_mask_loadu_epi32(vb1, mb1, s1+16) ;
    v2b =  _mm512_mask_loadu_epi32(vb2, mb2, s2+16) ;
    uint16_t k1 = _mm512_cmpgt_epi32_mask(v1a, v2a) ^ complement16 ;
    uint16_t k2 = _mm512_cmpgt_epi32_mask(v1b, v2b) ^ complement16 ;
    nmask = nmask + _mm_popcnt_u32(k1) + _mm_popcnt_u32(k2) ;
    mk[0] = (k1 & 0xFF) ;
    mk[1] = (k1 >> 16) ;
    mk[2] = (k2 & 0xFF) ;
    mk[3] = (k2 >> 16) ;
    s1 += incv1 ; s2 += incv2 ; mk += 4 ;      // add increments to pointers
  }
  if(n-i0 > 0){
    int scount = 32 - (n-i0) ;
    mask0 = Mask0Greater_c_le(s1, inc1, s2, inc2, n-i0) ^ complement ; // negate mask if necessary
    mask0 = (mask0 << scount) >> scount ;                              // get rid of extra 1s from negate
    nmask += _mm_popcnt_u32(mask0) ;
    mk[0] = (mask0      ) & 0xFF ;
    mk[1] = (mask0 >>  8) & 0xFF ;
    mk[2] = (mask0 >> 16) & 0xFF ;
    mk[3] = (mask0 >> 24) & 0xFF ;
  }
  return nmask ;
}
#endif
// ================================ MaskedMerge/MaskedFill family ===============================
// _le only versions for now
// AVX512F version
#if defined(__x86_64__) && defined(__AVX512F__)
void MaskedMerge_avx512_le(void *s, void *d, uint32_t le_mask, void *values, int n){
  uint32_t *src = (uint32_t *) s ;
  uint32_t *dst = (uint32_t *) d ;
  uint32_t *val = (uint32_t *) values ;
  while(n > 31){
    MaskedMerge_32_avx512_le(src, dst, le_mask, val) ;
    src += 32 ; dst += 32 ; val += 32 ;
    n = n - 32 ;
  }
  if(n > 0) MaskedMerge_1_32_c_le(src, dst, le_mask, val, n) ;
//   while(n--){
//     *dst++ = (le_mask & 1) ? *src : *val++ ; src++ ;
//     le_mask >>= 1 ;
//   }
}
void MaskedFill_avx512_le(void *s, void *d, uint32_t le_mask, uint32_t value, int n){
  uint32_t *src = (uint32_t *) s ;
  uint32_t *dst = (uint32_t *) d ;
  while(n > 31){
    MaskedFill_32_avx512_le(src, dst, le_mask, value) ;
    src += 32 ; dst += 32 ;
    n = n - 32 ;
  }
  if(n > 0) MaskedFill_1_32_c_le(src, dst, le_mask, value, n) ;
//   while(n--){
//     *dst++ = (le_mask & 1) ? *src : value ; src++ ;
//     le_mask >>= 1 ;
//   }
}
#endif
// AVX2 version
#if defined(__x86_64__) && defined(__AVX2__)
void MaskedMerge_avx2_le(void *s, void *d, uint32_t le_mask, void *values, int n){
  uint32_t *src = (uint32_t *) s ;
  uint32_t *dst = (uint32_t *) d ;
  uint32_t *val = (uint32_t *) values ;
  while(n > 31){
    MaskedMerge_32_avx2_le(src, dst, le_mask, val) ;
    src += 32 ; dst += 32 ; val += 32 ;
    n = n - 32 ;
  }
  if(n > 0) MaskedMerge_1_32_c_le(src, dst, le_mask, val, n) ;
//   while(n--){
//     *dst++ = (le_mask & 1) ? *src : *val++ ; src++ ;
//     le_mask >>= 1 ;
//   }
}
void MaskedFill_avx2_le(void *s, void *d, uint32_t le_mask, uint32_t value, int n){
  uint32_t *src = (uint32_t *) s ;
  uint32_t *dst = (uint32_t *) d ;
  while(n > 31){
    MaskedFill_32_avx2_le(src, dst, le_mask, value) ;
    src += 32 ; dst += 32 ;
    n = n - 32 ;
  }
  if(n > 0) MaskedFill_1_32_c_le(src, dst, le_mask, value, n) ;
//   while(n--){
//     *dst++ = (le_mask & 1) ? *src : value ; src++ ;
//     le_mask >>= 1 ;
//   }
}
#endif
// plain C version
void MaskedMerge_c_le(void *s, void *d, uint32_t le_mask, void *values, int n){
  uint32_t *src = (uint32_t *) s ;
  uint32_t *dst = (uint32_t *) d ;
  uint32_t *val = (uint32_t *) values ;
  while(n > 31){
    MaskedMerge_1_32_c_le(src, dst, le_mask, val, 32) ;
    src += 32 ; dst += 32 ; val += 32 ;
    n = n - 32 ;
  }
  if(n > 0) MaskedMerge_1_32_c_le(src, dst, le_mask, val, n) ;
//   while(n--){
//     *dst++ = (le_mask & 1) ? *src : *val++ ; src++ ;
//     le_mask >>= 1 ;
//   }
}
void MaskedFill_c_le(void *s, void *d, uint32_t le_mask, uint32_t value, int n){
  uint32_t *src = (uint32_t *) s ;
  uint32_t *dst = (uint32_t *) d ;
  while(n > 31){
    MaskedFill_1_32_c_le(src, dst, le_mask, value, 32) ;
    src += 32 ; dst += 32 ;
    n = n - 32 ;
  }
  if(n > 0) MaskedFill_1_32_c_le(src, dst, le_mask, value, n) ;
//   while(n--){
//     *dst++ = (le_mask & 1) ? *src : value ; src++ ;
//     le_mask >>= 1 ;
//   }
}
void MaskedMerge_le(void *s, void *d, uint32_t le_mask, void *values, int n){
#if defined(__x86_64__) && defined(__AVX512F__)
  MaskedMerge_avx512_le(s, d, le_mask, values, n) ;
#elif defined(__x86_64__) && defined(__AVX2__)
  MaskedMerge_avx2_le(s, d, le_mask, values, n) ;
#else
  MaskedMerge_c_le(s, d, le_mask, values, n) ;
#endif
}
void MaskedFill_le(void *s, void *d, uint32_t le_mask, uint32_t value, int n){
#if defined(__x86_64__) && defined(__AVX512F__)
  MaskedFill_avx512_le(s, d, le_mask, value, n) ;
#elif defined(__x86_64__) && defined(__AVX2__)
  MaskedFill_avx2_le(s, d, le_mask, value, n) ;
#else
  MaskedFill_c_le(s, d, le_mask, value, n) ;
#endif
}

// ================================ ExpandReplace family ===============================
// AVX512 version
#if defined(__x86_64__) && defined(__AVX512F__)
// _le only versions for now
void ExpandReplace_avx512_le(void *s_, void *d_, void *map_, int n){
  uint32_t *s = (uint32_t *)s_ ;
  uint32_t *d = (uint32_t *)d_ ;
  uint32_t *map = (uint32_t *)map_ ;
  int j ;
  uint32_t mask ;

  for(j=0 ; j<n-31 ; j+=32){                            // chunks of 32
    mask = *map++ ;
    s = ExpandReplace_32_avx512_le(s, d, mask) ; d += 32 ;
  }
  mask = *map++ ;
  ExpandReplace_0_31_c_le(s, d, mask, n - j) ;             // leftovers (0 -> 31)
}
#endif

// AVX2 version
#if defined(__x86_64__) && defined(__AVX2__)
void ExpandReplace_avx2_be(void *s_, void *d_, void *map_, int n){
  uint32_t *s = (uint32_t *)s_ ;
  uint32_t *d = (uint32_t *)d_ ;
  uint32_t *map = (uint32_t *)map_ ;
  int j ;
  uint32_t mask ;

  for(j=0 ; j<n-31 ; j+=32){                            // chunks of 32
    mask = *map++ ;
    s = ExpandReplace_32_avx2_be(s, d, mask) ; d += 32 ;
  }
  mask = *map++ ;
  ExpandReplace_0_31_c_be(s, d, mask, n - j) ;             // leftovers (0 -> 31)
}
void ExpandReplace_avx2_le(void *s_, void *d_, void *map_, int n){
  uint32_t *s = (uint32_t *)s_ ;
  uint32_t *d = (uint32_t *)d_ ;
  uint32_t *map = (uint32_t *)map_ ;
  int j ;
  uint32_t mask ;

  for(j=0 ; j<n-31 ; j+=32){                            // chunks of 32
    mask = *map++ ;
    s = ExpandReplace_32_avx2_le(s, d, mask) ; d += 32 ;
  }
  mask = *map++ ;
  ExpandReplace_0_31_c_le(s, d, mask, n - j) ;             // leftovers (0 -> 31)
}
#endif

// pure C code version
// s   [IN] : store-compressed source array (32 bit items)
// d  [OUT] : load-expanded destination array  (32 bit items)
// map [IN] : bitmap used to load-expand (BIG-ENDIAN style)
// n   [IN] : number of items
// where there is a 0 in the mask, nothing is done to array d
// where there is a 1 in the mask, the next value from s is stored into array d
//       (at the corresponding position in the map)
// values from s replace some values in d
void ExpandReplace_c_be(void *s_, void *d_, void *map_, int n){
  uint32_t *s = (uint32_t *)s_ ;
  uint32_t *d = (uint32_t *)d_ ;
  uint32_t *map = (uint32_t *)map_ ;
  int j ;
  uint32_t mask ;
  for(j=0 ; j<n-31 ; j+=32){                        // chunks of 32
    mask = *map++ ;
    s = ExpandReplace_32_c_be(s, d, mask) ; d += 32 ;
  }
  mask = *map++ ;
  ExpandReplace_0_31_c_be(s, d, mask, n - j) ;             // leftovers (0 -> 31)
}
void ExpandReplace_c_le(void *s_, void *d_, void *map_, int n){
  uint32_t *s = (uint32_t *)s_ ;
  uint32_t *d = (uint32_t *)d_ ;
  uint32_t *map = (uint32_t *)map_ ;
  int j ;
  uint32_t mask ;
  for(j=0 ; j<n-31 ; j+=32){                        // chunks of 32
    mask = *map++ ;
    s = ExpandReplace_32_c_le(s, d, mask) ; d += 32 ;
  }
  mask = *map++ ;
  ExpandReplace_0_31_c_le(s, d, mask, n - j) ;             // leftovers (0 -> 31)
}

void ExpandReplace_be(void *s_, void *d_, void *map_, int n){
#if defined(__x86_64__) && defined(__AVX2__)
  ExpandReplace_avx2_be(s_, d_, map_, n) ;
#else
  ExpandReplace_c_be(s_, d_, map_, n) ;
#endif
}

void ExpandReplace_le(void *s_, void *d_, void *map_, int n){
#if defined(__x86_64__) && defined(__AVX512F__)
  ExpandReplace_avx512_le(s_, d_, map_, n) ;
#elif defined(__x86_64__) && defined(__AVX2__)
  ExpandReplace_avx2_le(s_, d_, map_, n) ;
#else
  ExpandReplace_c_le(s_, d_, map_, n) ;
#endif
}

// ================================ ExpandFill family ===============================
// AVX512 version
#if defined(__x86_64__) && defined(__AVX512F__)
// _le only versions for now
void ExpandFill_avx512_le(void *s_, void *d_, void *map_, int n, void *fill_){
  uint32_t *s = (uint32_t *) s_ ;
  uint32_t *d = (uint32_t *) d_ ;
  uint32_t *map = (uint32_t *) map_ ;
  uint32_t *pfill = (uint32_t *) fill_ ;
  int j ;
  uint32_t mask, fill ;

  if(fill_ == NULL){
    ExpandReplace_avx512_le(s, d, map, n) ;
    return ;
  }
  fill = *pfill ;
  for(j=0 ; j<n-31 ; j+=32){                            // chunks of 32
    mask = *map++ ;
    s = ExpandFill_32_avx512_le(s, d, mask, fill) ; d += 32 ;
  }
  mask = *map++ ;
  ExpandFill_0_31_c_le(s, d, mask, fill, n - j) ;              // leftovers (0 -> 31)
}
#endif

// AVX2 version of ExpandFill_32
#if defined(__x86_64__) && defined(__AVX2__)
void ExpandFill_avx2_be(void *s_, void *d_, void *map_, int n, void *fill_){
  uint32_t *s = (uint32_t *) s_ ;
  uint32_t *d = (uint32_t *) d_ ;
  uint32_t *map = (uint32_t *) map_ ;
  uint32_t *pfill = (uint32_t *) fill_ ;
  int j ;
  uint32_t mask, fill ;

  if(fill_ == NULL){
    ExpandReplace_avx2_be(s, d, map, n) ;
    return ;
  }
  fill = *pfill ;
  for(j=0 ; j<n-31 ; j+=32){                            // chunks of 32
    mask = *map++ ;
    s = ExpandFill_32_avx2_be(s, d, mask, fill) ; d += 32 ;
  }
  mask = *map++ ;
  ExpandFill_0_31_c_be(s, d, mask, fill, n - j) ;              // leftovers (0 -> 31)
}
void ExpandFill_avx2_le(void *s_, void *d_, void *map_, int n, void *fill_){
  uint32_t *s = (uint32_t *) s_ ;
  uint32_t *d = (uint32_t *) d_ ;
  uint32_t *map = (uint32_t *) map_ ;
  uint32_t *pfill = (uint32_t *) fill_ ;
  int j ;
  uint32_t mask, fill ;

  if(fill_ == NULL){
    ExpandReplace_avx2_le(s, d, map, n) ;
    return ;
  }
  fill = *pfill ;
  for(j=0 ; j<n-31 ; j+=32){                            // chunks of 32
    mask = *map++ ;
    s = ExpandFill_32_avx2_le(s, d, mask, fill) ; d += 32 ;
  }
  mask = *map++ ;
  ExpandFill_0_31_c_le(s, d, mask, fill, n - j) ;              // leftovers (0 -> 31)
}
#endif

// plain C version
void ExpandFill_c_be(void *s_, void *d_, void *map_, int n, void *fill_){
  uint32_t *s = (uint32_t *) s_ ;
  uint32_t *d = (uint32_t *) d_ ;
  uint32_t *map = (uint32_t *) map_ ;
  uint32_t *pfill = (uint32_t *) fill_ ;
  int j ;
  uint32_t mask, fill ;
  if(fill_ == NULL){
    ExpandReplace_c_be(s, d, map, n) ;
    return ;
  }
  fill = *pfill ;
  for(j=0 ; j<n-31 ; j+=32){                            // chunks of 32
    mask = *map++ ;
    s = ExpandFill_32_c_be(s, d, mask, fill) ; d += 32 ;
  }
  mask = *map++ ;
  ExpandFill_0_31_c_be(s, d, mask, fill, n - j) ;              // leftovers (0 -> 31)
}

// generic version
// s   [IN] : store-compressed source array (32 bit items)
// d  [OUT] : load-expanded destination array  (32 bit items)
// map [IN] : bitmap used to load-expand (BIG-ENDIAN style)
// n   [IN] : number of items
// where there is a 0 in the mask, value "fill" is stored into array d
// where there is a 1 in the mask, the next value from s is stored into array d
//       (at the corresponding position in the map)
// all elements of d receive a new value ("fill" of a value from s)
void ExpandFill_be(void *s_, void *d_, void *map_, int n, void *fill_){
#if defined(__x86_64__) && defined(__AVX2__)
  ExpandFill_avx2_be(s_, d_, map_, n, fill_) ;
#else
  ExpandFill_c_be(s_, d_, map_, n, fill_) ;
#endif
}
void ExpandFill_le(void *s_, void *d_, void *map_, int n, void *fill_){
#if defined(__x86_64__) && defined(__AVX512F__)
  ExpandFill_avx512_le(s_, d_, map_, n, fill_) ;
#elif defined(__x86_64__) && defined(__AVX2__)
  ExpandFill_avx2_be(s_, d_, map_, n, fill_) ;
#else
  ExpandFill_c_be(s_, d_, map_, n, fill_) ;
#endif
}
// ================================ CompressStore family ===============================
// src     [IN] : source array (32 bit items)
// dst    [OUT] : store-compressed destination array  (32 bit items)
// be_mask [IN] : bit array used for store-compress (BIG-ENDIAN style)
// n       [IN] : number of items
// return : next storage address for array dst
// where there is a 0 in the mask, nothing is added into array d
// where there is a 1 in the mask, the corresponding item in array s is appended to array d
// returned pointer - original dst = number of items stored into array dst
// the mask is interpreted Big Endian style, bit 0 is the MSB, bit 31 is the LSB
#if defined(__x86_64__) && defined(__AVX2__)
// AVX2 version of stream_compress_32
void *CompressStore_avx2_be(void *src, void *dst, void *be_mask, int n){
  uint32_t *map = (uint32_t *) be_mask, mask ;
  uint32_t *w32 = (uint32_t *) src ;
  int i ;

  for(i=0 ; i<n-31 ; i+=32, w32 += 32){                       // chunks of 32
    mask = *map++ ;
    dst = CompressStore_32_avx2_be(w32, dst, mask) ;
  }
  mask = *map ;
  dst = CompressStore_0_31_c_be(w32, dst, mask, n - i) ;      // leftovers (0 -> 31)
  return dst ;
}
void *CompressStore_avx2_le(void *src, void *dst, void *le_mask, int n){
  uint32_t *map = (uint32_t *) le_mask, mask ;
  uint32_t *w32 = (uint32_t *) src ;
  int i ;

  for(i=0 ; i<n-31 ; i+=32, w32 += 32){                       // chunks of 32
    mask = *map++ ;
    dst = CompressStore_32_avx2_le(w32, dst, mask) ;
  }
  mask = *map ;
  dst = CompressStore_0_31_c_le(w32, dst, mask, n - i) ;      // leftovers (0 -> 31)
  return dst ;
}
#endif

#if defined(__x86_64__) && defined(__AVX512F__)
void *CompressStore_avx512_le(void *src, void *dst, void *le_mask, int n){
  uint32_t *map = (uint32_t *) le_mask, mask ;
  uint32_t *w32 = (uint32_t *) src ;
  int i ;

  for(i=0 ; i<n-31 ; i+=32, w32 += 32){                       // chunks of 32
    mask = *map++ ;
    dst = CompressStore_32_avx512_le(w32, dst, mask) ;
  }
  mask = *map ;
  dst = CompressStore_0_31_c_le(w32, dst, mask, n - i) ;      // leftovers (0 -> 31)
  return dst ;
}
#endif

// pure C code version (non SIMD)
void *CompressStore_c_be(void *src, void *dst, void *be_mask, int n){
  uint32_t *map = (uint32_t *) be_mask, mask ;
  uint32_t *w32 = (uint32_t *) src ;
  int i ;
  for(i=0 ; i<n-31 ; i+=32, w32 += 32){                       // chunks of 32
    mask = *map++ ;
    dst = CompressStore_32_c_be(w32, dst, mask) ;
  }
  mask = *map ;
  dst = CompressStore_0_31_c_be(w32, dst, mask, n - i) ;      // leftovers (0 -> 31)
  return dst ;
}
void *CompressStore_c_le(void *src, void *dst, void *le_mask, int n){
  uint32_t *map = (uint32_t *) le_mask, mask ;
  uint32_t *w32 = (uint32_t *) src ;
  int i ;
  for(i=0 ; i<n-31 ; i+=32, w32 += 32){                       // chunks of 32
    mask = *map++ ;
    dst = CompressStore_32_c_le(w32, dst, mask) ;
  }
  mask = *map ;
  dst = CompressStore_0_31_c_le(w32, dst, mask, n - i) ;      // leftovers (0 -> 31)
  return dst ;
}

// generic version, switches between the optimized versions
// void *CompressStore_be(void *src, void *dst, void *be_mask, int n){
// #if defined(__x86_64__) && defined(__AVX2__)
//   return CompressStore_avx2_be(src, dst, be_mask, n) ;
// #else
//   return CompressStore_c_be(src, dst, be_mask, n) ;
// #endif
// }

// the AVX512 version does not use the compressed store with mask instruction
// the alternative is just plain C code
// src    [IN] : "full" array
// dst [OUT] : non masked elements from "full" array
// le_mask        [IN] : 32 bits mask, if bit[I] is 1, src[I] is stored, if 0, it is skipped
// the function returns the next address to be stored into for the "dst" array
// the mask is interpreted Little Endian style, bit[0] is the LSB, bit[31] is the MSB
// this version processes 32 elements from "src"
// this version uses a store rather than a compressed store for performance reasons
// and can store up to 32 extra (irrelevant) values after the useful end of dst
static void *CompressStore_32_le(void *src, void *dst, uint32_t le_mask){
// generic version, switches between the optimized versions
#if defined(__AVX512F__)
  dst = CompressStore_32_avx512_le(src, dst, le_mask) ;
#elif defined(__AVX2__)
  dst = CompressStore_32_avx2_le(src, dst, le_mask) ;
#else
  dst = CompressStore_32_c_le(src, dst, le_mask) ;
#endif
return dst ;
}
static void *CompressStore_32_be(void *src, void *dst, uint32_t le_mask){
// generic version, switches between the optimized versions
#if defined(__AVX2__)
  dst = CompressStore_32_avx2_be(src, dst, le_mask) ;
#else
  dst = CompressStore_32_c_be(src, dst, le_mask) ;
#endif
return dst ;
}

// src     [IN] : "source" array
// nw32    [IN] : number of elements in "source" array
// dst    [OUT] : non masked elements from "source" array
// be_mask [IN] : array of masks (Big Endian style)       bit 0 is MSB
// le_mask [IN] : array of masks (Little Endian style)    bit 0 is LSB
//                32 bits elements, if bit[I] is 1, src[I] is stored, if 0, it is skipped
// the function returns the next address to be stored into for the "dst" array
void *CompressStore_be(void *src, void *dst, void *be_map, int nw32){
  int i0 ;
  uint32_t *w32 = (uint32_t *) src ;
  uint32_t *be_mask = (uint32_t *) be_map ;
  for(i0 = 0 ; i0 < nw32 - 31 ; i0 += 32){
    dst = CompressStore_32_be(w32 + i0, dst, *be_mask) ;
    be_mask++ ;
  }
  dst = CompressStore_0_31_c_be(w32 + i0, dst, *be_mask, nw32 - i0) ;
  return dst ;
}
void *CompressStore_le(void *src, void *dst, void *le_map, int nw32){
  int i0 ;
  uint32_t *w32 = (uint32_t *) src ;
  uint32_t *le_mask = (uint32_t *) le_map ;
  for(i0 = 0 ; i0 < nw32 - 31 ; i0 += 32){
    dst = CompressStore_32_le(w32 + i0, dst, *le_mask) ;
    le_mask++ ;
  }
  dst = CompressStore_0_31_c_le(w32 + i0, dst, *le_mask, nw32 - i0) ;
  return dst ;
}
#if 0
// le_map   [OUT] : 1s where src[i] != reference value, 0s otherwise
static int32_t CompressStoreSpecialValue(void *src, void *dst, void *le_map, int nw32, const void *value){
  int32_t nval = 0, i, i0, j ;
  uint32_t *w32 = (uint32_t *) src ;
  uint32_t *vref = (uint32_t *) value ;
  uint32_t *mask = (uint32_t *) le_map ;
  uint32_t *dest = (uint32_t *) dst ;
  uint32_t mask0, bits ;
  uint32_t ref = *vref ;

#if defined(__AVX512F__)
  __m512i vr0 = _mm512_set1_epi32(ref) ;
// fprintf(stderr, "AVX512 code : 2 subslices of 16 values\n");
#elif defined(__AVX2__)
  __m256i vr0 = _mm256_set1_epi32(ref) ;
// fprintf(stderr, "AVX2 code : 4 subslices of 8 values\n");
#else
// fprintf(stderr, "plain C code : 1 slice of 32 values\n");
#endif

  for(i0 = 0 ; i0 < nw32 - 31 ; i0 += 32){   // 32 values per slice
#if defined(__AVX512F__)
//   AVX512 code : 2 subslices of 16 values
  __m512i vd0, vd1 ;
  __mmask16 mk0, mk1 ;
  uint32_t pop0, pop1 ;
  vd0 = _mm512_loadu_epi32(w32 + i0) ;
  vd1 = _mm512_loadu_epi32(w32 + i0 + 16) ;
  mk0 = _mm512_mask_cmp_epu32_mask (0xFFFF, vr0, vd0, _MM_CMPINT_NE) ;
  _mm512_mask_compressstoreu_epi32 (dest, mk0, vd0) ;          // store non special values
  pop0 = _mm_popcnt_u32(mk0) ;
  dest += pop0 ; nval += pop0 ;
  mk1 = _mm512_mask_cmp_epu32_mask (0xFFFF, vr0, vd1, _MM_CMPINT_NE) ;
  _mm512_mask_compressstoreu_epi32 (dest, mk1, vd1) ;          // store non special values
  pop1 = _mm_popcnt_u32(mk1) ;
  dest += pop1 ; nval += pop1 ;
  mask0 = mk1 ;
  mask0 = (mask0 << 16) | mk0 ;
#elif defined(__AVX2__)
//   AVX2 code : 4 subslices of 8 values, probably better to use the plain C code, because of the compressed store
  __m256i vd0, vd1, vd2, vd3, vs0, vc0, vc1, vc2, vc3 ;
  uint32_t pop ;
  vd0 = _mm256_loadu_si256((__m256i *)(w32 + i0     )) ;
  vd1 = _mm256_loadu_si256((__m256i *)(w32 + i0 +  8)) ;
  vd2 = _mm256_loadu_si256((__m256i *)(w32 + i0 + 16)) ;
  vd3 = _mm256_loadu_si256((__m256i *)(w32 + i0 + 24)) ;
  vc3 = _mm256_cmpeq_epi32(vd3, vr0) ; mask0  = _mm256_movemask_ps((__m256) vc3) ; mask0 <<= 8 ;
  vc2 = _mm256_cmpeq_epi32(vd2, vr0) ; mask0 |= _mm256_movemask_ps((__m256) vc2) ; mask0 <<= 8 ;
  vc1 = _mm256_cmpeq_epi32(vd1, vr0) ; mask0 |= _mm256_movemask_ps((__m256) vc1) ; mask0 <<= 8 ;
  vc0 = _mm256_cmpeq_epi32(vd0, vr0) ; mask0 |= _mm256_movemask_ps((__m256) vc0) ; mask0 = (~mask0) ;
  pop = _mm_popcnt_u32(mask0) ;
  nval += pop ;
  bits = mask0 ;
  for(j=0 ; j<32 ; j++, bits >>= 1){   // "compressed store"
    *dest = w32[i0 + j] ;              // always store value
    dest += (bits & 1) ;               // but increment pointer only if the store is "useful"
  }
#else
//   plain C code
  bits = 1 ; mask0 = 0 ;
  for(i = 0 ; i < 32 ; i++){
    int onoff = (w32[i+i0] != ref) ? 1 : 0 ;
    mask0 |= (onoff << i) ;   // insert onoff at the correct bit position
    *dest = w32[i+i0] ;       // always store
    dest += onoff ;           // increment only if the store is useful
    nval += onoff ;
  }
#endif

  *mask = mask0 ; mask++ ;
  }
  bits = 1 ; mask0 = 0 ;
  for(i = i0 ; i < nw32 ; i++){
    if(w32[i] != ref){
      mask0 |= bits ;
      *dest = w32[i] ; dest++ ;
      nval++ ;
    }
    bits <<= 1 ;
  }
  *mask = mask0 ;
  return nval ;
}
#endif
