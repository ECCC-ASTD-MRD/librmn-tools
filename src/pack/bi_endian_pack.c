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
//     M. Valin,   Recherche en Prevision Numerique, 2022
//
// set of functions to help manage insertion/extraction of integers into/from a bit stream
// the bit stream is a sequence of unsigned 32 bit integers
// both Big and Little endian style insertion/extraction support is provided
//
#include <stdint.h>
#include <stdio.h>

#include <rmn/bits.h>
#define STATIC extern
#include <rmn/bi_endian_pack.h>

#if 0
// deprecated macros, formerly in bi_endian_pack.h
// ===============================================================================================
// declare state variables (unsigned)
#define STREAM_DCL_STATE_VARS(acc, ind, ptr) uint64_t acc ; int32_t ind  ; uint32_t *ptr
// declare state variables (signed)
#define STREAM_DCL_STATE_VARS_S(acc, ind, ptr) int64_t acc ; int32_t ind  ; uint32_t *ptr

// get/set stream insertion state
#define STREAM_GET_INSERT_STATE(s, acc, ind, ptr) acc = (s).acc_i ; ind = (s).insert ; ptr = (s).in
#define STREAM_SET_INSERT_STATE(s, acc, ind, ptr) (s).acc_i = acc ; (s).insert = ind ; (s).in = ptr

// get/set stream extraction state (unsigned)
#define STREAM_GET_XTRACT_STATE(s, acc, ind, ptr) acc = (s).acc_x ; ind = (s).xtract ; ptr = (s).out
#define STREAM_SET_XTRACT_STATE(s, acc, ind, ptr) (s).acc_x = acc ; (s).xtract = ind ; (s).out = ptr
// get stream extraction state (signed)
#define STREAM_GET_XTRACT_STATE_S(s, acc, ind, ptr) acc = (int64_t)(s).acc_x ; ind = (s).xtract ; ptr = (s).out
#endif

// little endian style insertion of values into a bit stream
// p     : stream                                [INOUT]
// w32   : array of values to insert             [IN]
// nbits : number of bits kept for each value    [IN]
// nw    : number of values from w32             [IN}
int  LeStreamInsert(bitstream *p, uint32_t *w32, int nbits, int nw){
  int i = 0, n = (nw < 0) ? -nw : nw ;
//   STREAM_DCL_STATE_VARS(accum, insert, stream) ;
//   STREAM_GET_INSERT_STATE(*p, accum, insert, stream) ;
  EZ_NEW_INSERT_VARS(*p) ;

//   if(insert < 0) return 0;      // ERROR: not in insert mode
  if(! STREAM_INSERT_MODE(*p) )      return 0;   // ERROR: not in insert mode
  if(! STREAM_IS_LITTLE_ENDIAN(*p) ) return 0 ;  // wrong endianness

  if(nbits <= 16) {       // process values two at a time
    uint32_t t, mask = RMASK32(nbits), nb = nbits + nbits ;
    for(    ; i<n-1 ; i+=2){
      // little endian => upper part [i+1] | lower part [i]
      t  = (w32[i  ] & mask) | ((w32[i+1] & mask) << nbits) ;
//       LE64_PUT_NBITS(accum, insert, t, nb, stream) ;   // insert a pair of values
      LE64_EZ_PUT_NBITS(t, nb) ;
    }
  }
  for(    ; i<n ; i++){
//     LE64_PUT_NBITS(accum, insert, w32[i], nbits, stream) ;
    LE64_EZ_PUT_NBITS(w32[i], nbits) ;
  }
  if(nw <= 0) {
//     LE64_INSERT_FINAL(accum, insert, stream) ;
    LE64_EZ_INSERT_FINAL ;
  }else{
//     LE64_PUSH(accum, insert, stream) ;
    LE64_EZ_PUSH ;
  }
//   STREAM_SET_INSERT_STATE(*p, accum, insert, stream) ;
  EZ_SET_INSERT_VARS(*p) ;
  return n ;
}

// big endian style insertion of values into a bit stream
// p     : stream                                [INOUT]
// w32   : array of values to insert             [IN]
// nbits : number of bits kept for each value    [IN]
// nw    : number of values from w32             [IN}
int  BeStreamInsert(bitstream *p, uint32_t *w32, int nbits, int nw){
  int i = 0, n = (nw < 0) ? -nw : nw ;
//   STREAM_DCL_STATE_VARS(accum, insert, stream) ;
//   STREAM_GET_INSERT_STATE(*p, accum, insert, stream) ;
  EZ_NEW_INSERT_VARS(*p) ;

//   if(insert < 0) return 0;      // ERROR: not in insert mode
  if(! STREAM_INSERT_MODE(*p) )   return 0;     // ERROR: not in insert mode
  if(! STREAM_IS_BIG_ENDIAN(*p) ) return 0 ;    // wrong endianness

  if(nbits <= 16) {       // process values two at a time
    uint32_t t, mask = RMASK32(nbits), nb = nbits + nbits ;
    for(    ; i<n-1 ; i+=2){
      // big endian => upper part [i] | lower part [i+1]
      t  = (w32[i+1] & mask) | ((w32[i  ] & mask) << nbits) ;
//       BE64_PUT_NBITS(accum, insert, t, nb, stream) ;   // insert a pair of values
      BE64_EZ_PUT_NBITS(t, nb) ;
    }
  }
  for(    ; i<n ; i++){
//     BE64_PUT_NBITS(accum, insert, w32[i], nbits, stream) ;
    BE64_EZ_PUT_NBITS(w32[i], nbits) ;
  }
  if(nw <= 0) {
//     BE64_INSERT_FINAL(accum, insert, stream) ;
    BE64_EZ_INSERT_FINAL ;
  }else{ 
//     BE64_PUSH(accum, insert, stream) ;
    BE64_EZ_PUSH ;
  }
//   STREAM_SET_INSERT_STATE(*p, accum, insert, stream) ;
  EZ_SET_INSERT_VARS(*p) ;
  return n ;
}

// little endian style extraction of unsigned values from a bit stream
// p     : stream                                [INOUT]
// w32   : array of unsigned values extracted    [OUT]
// nbits : number of bits kept for each value    [IN]
// n     : number of values from w32             [IN}
int  LeStreamXtract(bitstream *p, uint32_t *w32, int nbits, int n){
  int i = 0 ;
//   STREAM_DCL_STATE_VARS(accum, xtract, stream) ;
//   STREAM_GET_XTRACT_STATE(*p, accum, xtract, stream) ;
  EZ_NEW_XTRACT_VARS(*p) ;

//   if(xtract < 0) return 0;      // ERROR: not in extract mode
  if(! STREAM_XTRACT_MODE(*p) )      return 0 ;  // ERROR: not in extract mode
  if(! STREAM_IS_LITTLE_ENDIAN(*p) ) return 0 ;  // wrong endianness

  if(nbits <= 16) {       // process values two at a time
    uint32_t t, mask = RMASK32(nbits), nb = nbits + nbits ;

    for(    ; i<n-1 ; i+=2){
//       LE64_XTRACT_CHECK(accum, xtract, stream) ;
      LE64_EZ_XTRACT_CHECK ;
//       LE64_GET_NBITS(accum, xtract, t, nb, stream) ;   // get a pair of values
      LE64_EZ_GET_NBITS(t, nb) ;
      w32[i  ] = t & mask ;                    // little endian means lower part first
      w32[i+1] = t >> nbits ;                  // then upper part
    }
  }
  for(    ; i<n ; i++){
//     LE64_GET_NBITS(accum, xtract, w32[i], nbits, stream) ;
    LE64_EZ_GET_NBITS(w32[i], nbits) ;
  }
//   STREAM_SET_XTRACT_STATE(*p, accum, xtract, stream) ;
  EZ_SET_XTRACT_VARS(*p) ;
  return n ;
}

// little endian style extraction of signed values from a bit stream
// p     : stream                                [INOUT]
// w32   : array of unsigned values extracted    [OUT]
// nbits : number of bits kept for each value    [IN]
// n     : number of values from w32             [IN}
int  LeStreamXtractSigned(bitstream *p, int32_t *w32, int nbits, int n){
  int i = 0 ;
//   STREAM_DCL_STATE_VARS_S(accum, xtract, stream) ;
//   STREAM_GET_XTRACT_STATE_S(*p, accum, xtract, stream) ;
  EZ_NEW_XTRACT_VARS(*p) ;

//   if(xtract < 0) return 0;      // ERROR: not in extract mode
  if(! STREAM_XTRACT_MODE(*p) )      return 0 ;  // ERROR: not in extract mode
  if(! STREAM_IS_LITTLE_ENDIAN(*p) ) return 0 ;  // wrong endianness

  if(nbits <= 16) {       // process values two at a time
    int32_t t, nb = nbits + nbits ; // mask = RMASK32(nbits) ;
    for(    ; i<n-1 ; i+=2){
//       LE64_GET_NBITS(accum, xtract, t, nb, stream) ;   // get a pair of values
      LE64_EZ_GET_NBITS(t, nb) ;
      // use shift to propagate sign
      w32[i  ] = (t << (32-nbits)) >> (32-nbits) ;     // little endian means lower part first
      w32[i+1] = t >> nbits ;                          // then upper part
    }
  }
  for(    ; i<n ; i++){
//     LE64_GET_NBITS(accum, xtract, w32[i], nbits, stream) ;
    LE64_EZ_GET_NBITS(w32[i], nbits) ;
  }
//   STREAM_SET_XTRACT_STATE(*p, accum, xtract, stream) ;
  EZ_SET_XTRACT_VARS(*p) ;
  return n ;
}

// big endian style extraction of unsigned values from a bit stream
// p     : stream                                [INOUT]
// w32   : array of unsigned values extracted    [OUT]
// nbits : number of bits kept for each value    [IN]
// n     : number of values from w32             [IN}
int  BeStreamXtract(bitstream *p, uint32_t *w32, int nbits, int n){
  int i = 0 ;
//   STREAM_DCL_STATE_VARS(accum, xtract, stream) ;
//   STREAM_GET_XTRACT_STATE(*p, accum, xtract, stream) ;
  EZ_NEW_XTRACT_VARS(*p) ;

//   if(xtract < 0) return 0;      // ERROR: not in extract mode
  if(! STREAM_XTRACT_MODE(*p) )   return 0 ;  // ERROR: not in extract mode
  if(! STREAM_IS_BIG_ENDIAN(*p) ) return 0 ;  // wrong endianness

  if(nbits <= 16) {       // process values two at a time
    uint32_t t, mask = RMASK32(nbits), nb = nbits + nbits ;

    for(    ; i<n-1 ; i+=2){
//       BE64_XTRACT_CHECK(accum, xtract, stream) ;
      BE64_EZ_XTRACT_CHECK ;
//       BE64_GET_NBITS(accum, xtract, t, nb, stream) ;      // get a pair of values
      BE64_EZ_GET_NBITS(t, nb) ;
      w32[i  ] = t >> nbits ;                     // big endian means upper part first
      w32[i+1] = t & mask ;                       // then lower part
    }
  }
  for(    ; i<n ; i++){
//     BE64_GET_NBITS(accum, xtract, w32[i], nbits, stream) ;
    BE64_EZ_GET_NBITS(w32[i], nbits) ;
  }
//   STREAM_SET_XTRACT_STATE(*p, accum, xtract, stream) ;
  EZ_SET_XTRACT_VARS(*p) ;
  return n ;
}

// big endian style extraction of signed values from a bit stream
// p     : stream                                [INOUT]
// w32   : array of signed values extracted      [OUT]
// nbits : number of bits kept for each value    [IN]
// n     : number of values from w32             [IN}
int  BeStreamXtractSigned(bitstream *p, int32_t *w32, int nbits, int n){
  int i = 0 ;
//   STREAM_DCL_STATE_VARS_S(accum, xtract, stream) ;
//   STREAM_GET_XTRACT_STATE_S(*p, accum, xtract, stream) ;
  EZ_NEW_XTRACT_VARS(*p) ;

//   if(xtract < 0) return 0;      // ERROR: not in extract mode
  if(! STREAM_XTRACT_MODE(*p) )   return 0 ;  // ERROR: not in extract mode
  if(! STREAM_IS_BIG_ENDIAN(*p) ) return 0 ;  // wrong endianness

  if(nbits <= 16) {       // process values two at a time
    int32_t t, nb = nbits + nbits ; //  mask = RMASK32(nbits) ;
    for(    ; i<n-1 ; i+=2){
//       BE64_GET_NBITS(accum, xtract, t, nb, stream) ;         // get a pair of values
      BE64_EZ_GET_NBITS(t, nb) ;
      // use shift to propagate sign
      w32[i  ] = t >> nbits ;                        // big endian means upper part first
      w32[i+1] = (t << (32-nbits)) >> (32-nbits) ;   // then lower part
    }
  }
  for(    ; i<n ; i++){
//     BE64_GET_NBITS(accum, xtract, w32[i], nbits, stream) ;
    BE64_EZ_GET_NBITS(w32[i], nbits) ;
  }
//   STREAM_SET_XTRACT_STATE(*p, accum, xtract, stream) ;
  EZ_SET_XTRACT_VARS(*p) ;
  return n ;
}

// little endian style insertion of values into a bit stream
// p     : stream                                [INOUT]
// w32   : array of values inserted              [IN]
// nbits : array of nuber of bits to keep        [IN]
// n     : number of values to insert            [IN]
// n[i], nbits[i] is an associated (nbits , n) pair
// n[i] <= 0 marks the end of the pair list
// returned value : total number of values inserted
int  LeStreamInsertM(bitstream *p, uint32_t *w32, int *nbits, int *n){
  int i, nw = 0 ;
//   STREAM_DCL_STATE_VARS(accum, insert, stream) ;
//   STREAM_GET_INSERT_STATE(*p, accum, insert, stream) ;
  EZ_NEW_INSERT_VARS(*p) ;

//   if(insert < 0) return 0;      // ERROR: not in insert mode
  if(! STREAM_INSERT_MODE(*p) )      return 0 ;  // ERROR: not in insert mode
  if(! STREAM_IS_LITTLE_ENDIAN(*p) ) return 0 ;  // wrong endianness

  while(n[0] > 0 ){     // loop until end of list (non positive value)
    nw += n[0] ;
    for(i=0 ; i<n[0] ; i++){
//       LE64_PUT_NBITS(accum, insert, w32[i], nbits[0], stream) ;
      LE64_EZ_PUT_NBITS(w32[i], nbits[0]) ;
    }
    nbits++ ;           // next pair
    n++ ;
  }
//   if(n[0] == -1) LE64_INSERT_FINAL(accum, insert, stream) ;
  if(n[0] == -1) LE64_EZ_INSERT_FINAL ;
//   STREAM_SET_INSERT_STATE(*p, accum, insert, stream) ;
  EZ_SET_INSERT_VARS(*p) ;
  return nw ;
}

// big endian style insertion of values into a bit stream
// p     : stream                                [INOUT]
// w32   : array of values inserted              [IN]
// nbits : array of nuber of bits to keep        [IN]
// n     : number of values to insert            [IN]
// n[i], nbits[i] is an associated (nbits , n) pair
// n[i] <= 0 marks the end of the pair list
// returned value : total number of values inserted
int  BeStreamInsertM(bitstream *p, uint32_t *w32, int *nbits, int *n){
  int i, nw = 0 ;
//   STREAM_DCL_STATE_VARS(accum, insert, stream) ;
//   STREAM_GET_INSERT_STATE(*p, accum, insert, stream) ;
  EZ_NEW_INSERT_VARS(*p) ;

//   if(insert < 0) return 0;      // ERROR: not in insert mode
  if(! STREAM_INSERT_MODE(*p) )   return 0 ;  // ERROR: not in insert mode
  if(! STREAM_IS_BIG_ENDIAN(*p) ) return 0 ;  // wrong endianness

  while(n[0] > 0 ){     // loop until end of list (non positive value)
    nw += n[0] ;
    for(i=0 ; i<n[0] ; i++){
//       BE64_PUT_NBITS(accum, insert, w32[i], nbits[0], stream) ;
      LE64_EZ_PUT_NBITS(w32[i], nbits[0]) ;
    }
    nbits++ ;           // next pair
    n++ ;
  }
//   if(n[0] == -1) BE64_INSERT_FINAL(accum, insert, stream) ;
  if(n[0] == -1) LE64_EZ_INSERT_FINAL ;
//   STREAM_SET_INSERT_STATE(*p, accum, insert, stream) ;
  EZ_SET_INSERT_VARS(*p) ;
  return nw ;
}

// little endian style extraction of unsigned values from a bit stream
// p     : stream                                [INOUT]
// w32   : array of unsigned values extracted    [OUT]
// nbits : array of nuber of bits to keep        [IN]
// n     : number of unsigned values to extract  [IN]
// n[i], nbits[i] is an associated (nbits , n) pair
// n[i] <= 0 marks the end of the pair list
int  LeStreamXtractM(bitstream *p, uint32_t *w32, int *nbits, int *n){
  int i, nw = 0 ;
//   STREAM_DCL_STATE_VARS(accum, xtract, stream) ;
//   STREAM_GET_XTRACT_STATE(*p, accum, xtract, stream) ;
  EZ_NEW_XTRACT_VARS(*p) ;

//   if(xtract < 0) return 0;      // ERROR: not in extract mode
  if(! STREAM_XTRACT_MODE(*p) )      return 0 ;  // ERROR: not in insert mode
  if(! STREAM_IS_LITTLE_ENDIAN(*p) ) return 0 ;  // wrong endianness

  while(n[0] > 0 ){     // loop until end of list (non positive value)
    nw += n[0] ;
    for(i=0 ; i<n[0] ; i++){
//       LE64_GET_NBITS(accum, xtract, w32[i], nbits[0], stream) ;
      LE64_EZ_GET_NBITS(w32[i], nbits[0]) ;
    }
    nbits++ ;           // next pair
    n++ ;
  }
//   STREAM_SET_XTRACT_STATE(*p, accum, xtract, stream) ;
  EZ_SET_XTRACT_VARS(*p) ;
  return nw ;
}

// big endian style extraction of unsigned values from a bit stream
// p     : stream                                [INOUT]
// w32   : array of unsigned values extracted    [OUT]
// nbits : array of nuber of bits to keep        [IN]
// n     : number of unsigned values to extract  [IN]
// n[i], nbits[i] is an associated (nbits , n) pair
// n[i] <= 0 marks the end of the pair list
int  BeStreamXtractM(bitstream *p, uint32_t *w32, int *nbits, int *n){
  int i, nw = 0 ;
//   STREAM_DCL_STATE_VARS(accum, xtract, stream) ;
//   STREAM_GET_XTRACT_STATE(*p, accum, xtract, stream) ;
  EZ_NEW_XTRACT_VARS(*p) ;

//   if(xtract < 0) return 0;      // ERROR: not in extract mode
  if(! STREAM_XTRACT_MODE(*p) )   return 0 ;  // ERROR: not in insert mode
  if(! STREAM_IS_BIG_ENDIAN(*p) ) return 0 ;  // wrong endianness

  while(n[0] > 0 ){     // loop until end of list (non positive value)
    nw += n[0] ;
    for(i=0 ; i<n[0] ; i++){
//       BE64_GET_NBITS(accum, xtract, w32[i], nbits[0], stream) ;
      BE64_EZ_GET_NBITS(w32[i], nbits[0]) ;
    }
    nbits++ ;           // next pair
    n++ ;
  }
//   STREAM_SET_XTRACT_STATE(*p, accum, xtract, stream) ;
  EZ_SET_XTRACT_VARS(*p) ;
  return nw ;
}
