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
//     M. Valin,   Recherche en Prevision Numerique, 2022-2023
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
// word stream macros and functions
// #include <rmn/word_stream.h>
// bit stream macros and functions
#include <rmn/bit_stream.h>

// little endian style insertion of values into a bit stream
// p     : stream                                [INOUT]
// w32   : array of values to insert             [IN]
// nbits : number of bits kept for each value    [IN]
// nw    : number of values from w32             [IN}
int  LeStreamInsert(bitstream *p, uint32_t *w32, int nbits, int nw){
  int i = 0, n = (nw < 0) ? -nw : nw ;
  EZ_NEW_INSERT_VARS(*p) ;                       // declare and get insertion EZ variables

  if(! STREAM_INSERT_MODE(*p) )      return 0;   // ERROR: not in insert mode
  if(! STREAM_IS_LITTLE_ENDIAN(*p) ) return 0 ;  // wrong endianness

  if(nbits <= 8) {                              // process values two at a time
    uint32_t t, mask = RMASK32(nbits), nb = nbits + nbits ;
    for(    ; i<n-3 ; i+=4){
      // little endian =>  lower part [i] | upper part [i+1]
      t  = (w32[i  ] & mask) | ((w32[i+1] & mask) << nbits) | ((w32[i+2] & mask) << nb) | ((w32[i+3] & mask) << (nb+nbits)) ;
      LE64_EZ_FAST_PUT_NBITS(t, nb+nb) ;                 // insert a pair of values
    }
  }
  if(nbits <= 16) {                              // process values two at a time
    uint32_t t, mask = RMASK32(nbits), nb = nbits + nbits ;
    for(    ; i<n-1 ; i+=2){
      // little endian =>  lower part [i] | upper part [i+1]
      t  =              (w32[i  ] & mask) | ((w32[i+1] & mask) << nbits) ;
      LE64_EZ_FAST_PUT_NBITS(t, nb) ;                 // insert a pair of values
    }
  }
  for(    ; i<n ; i++){
    LE64_EZ_PUT_NBITS(w32[i], nbits) ;           // insert values
  }
  if(nw <= 0) {
    LE64_EZ_INSERT_FINAL ;                       // flush accumulator
  }else{
    LE64_EZ_PUSH ;                               // jush push data to buffer
  }
  EZ_SET_INSERT_VARS(*p) ;                       // update stream using insertion EZ variables
  return n ;
}

// big endian style insertion of values into a bit stream
// p     : stream                                [INOUT]
// w32   : array of values to insert             [IN]
// nbits : number of bits kept for each value    [IN]
// nw    : number of values from w32             [IN}
int  BeStreamInsert(bitstream *p, uint32_t *w32, int nbits, int nw){
  int i = 0, n = (nw < 0) ? -nw : nw ;
  EZ_NEW_INSERT_VARS(*p) ;                       // declare and get insertion EZ variables

  if(! STREAM_INSERT_MODE(*p) )   return 0;      // ERROR: not in insert mode
  if(! STREAM_IS_BIG_ENDIAN(*p) ) return 0 ;     // wrong endianness

  uint32_t mask = RMASK32(nbits) ;
  if(nbits <= 8) {       // process values four at a time
    uint32_t t, nb = nbits + nbits ;
    for(    ; i<n-3 ; i+=4){
      // big endian => lower part [i+1] | upper part [i]
      t  = (w32[i+3] & mask) | ((w32[i+2] & mask) << nbits) | ((w32[i+1] & mask) << nb) | ((w32[i+0] & mask) << (nb+nbits)) ;
      BE64_EZ_FAST_PUT_NBITS(t, nb+nb) ;                 // insert a pair of values
    }
  }
  if(nbits <= 16) {       // process values two at a time
    uint32_t t, nb = nbits + nbits ;
    for(    ; i<n-1 ; i+=2){
      // big endian => lower part [i+1] | upper part [i]
      t  =            (w32[i+1] & mask) | ((w32[i  ] & mask) << nbits) ;
      BE64_EZ_FAST_PUT_NBITS(t, nb) ;                 // insert a pair of values
    }
  }
  for(    ; i<n ; i++){
    BE64_EZ_PUT_NBITS(w32[i], nbits) ;           // insert values
  }
  if(nw <= 0) {
    BE64_EZ_INSERT_FINAL ;                       // flush accumulator
  }else{ 
    BE64_EZ_PUSH ;                               // jush push data to buffer
  }
  EZ_SET_INSERT_VARS(*p) ;                       // update stream using insertion EZ variables
  return n ;
}

// little endian style extraction of unsigned values from a bit stream
// p     : stream                                [INOUT]
// w32   : array of unsigned values extracted    [OUT]
// nbits : number of bits kept for each value    [IN]
// n     : number of values from w32             [IN}
int  LeStreamXtract(bitstream *p, uint32_t *w32, int nbits, int n){
  int i = 0 ;
  EZ_NEW_XTRACT_VARS(*p) ;                       // declare and get extraction EZ variables

  if(! STREAM_XTRACT_MODE(*p) )      return 0 ;  // ERROR: not in extract mode
  if(! STREAM_IS_LITTLE_ENDIAN(*p) ) return 0 ;  // wrong endianness

  if(nbits <= 16) {                              // process values two at a time
    uint32_t t, mask = RMASK32(nbits), nb = nbits + nbits ;
    for(    ; i<n-1 ; i+=2){
      LE64_EZ_GET_NBITS(t, nb) ;                 // get a pair of values
      w32[i  ] = t & mask ;                      // little endian means lower part first
      w32[i+1] = t >> nbits ;                    // then upper part
    }
  }
  for(    ; i<n ; i++){
    LE64_EZ_GET_NBITS(w32[i], nbits) ;           // get values
  }
  EZ_SET_XTRACT_VARS(*p) ;                       // update stream using extraction EZ variables
  return n ;
}

// little endian style extraction of signed values from a bit stream
// p     : stream                                [INOUT]
// w32   : array of unsigned values extracted    [OUT]
// nbits : number of bits kept for each value    [IN]
// n     : number of values from w32             [IN}
int  LeStreamXtractSigned(bitstream *p, int32_t *w32, int nbits, int n){
  int i = 0, n32 = 32 - nbits ;
  EZ_NEW_XTRACT_VARS(*p) ;                       // declare and get extraction EZ variables

  if(! STREAM_XTRACT_MODE(*p) )      return 0 ;  // ERROR: not in extract mode
  if(! STREAM_IS_LITTLE_ENDIAN(*p) ) return 0 ;  // wrong endianness

  if(nbits <= 16) {                              // process values two at a time
    int32_t t, nb = nbits + nbits ;              // mask = RMASK32(nbits) ;
    for(    ; i<n-1 ; i+=2){
      LE64_EZ_GET_NBITS(t, nb) ;                 // get a pair of values
      // use shift to propagate sign
      w32[i  ] = (t << (n32)) >> (n32) ;         // little endian means lower part first
      w32[i+1] = t >> nbits ;                    // then upper part
      w32[i+1] = (w32[i+1] << n32) >> n32 ;      // restore sign
    }
  }
  for(    ; i<n ; i++){
    LE64_EZ_GET_NBITS(w32[i], nbits) ;           // get values
    w32[i] = (w32[i] << n32) >> n32 ;            // restore sign
  }
  EZ_SET_XTRACT_VARS(*p) ;                       // update stream using extraction EZ variables
  return n ;
}

// big endian style extraction of unsigned values from a bit stream
// p     : stream                                [INOUT]
// w32   : array of unsigned values extracted    [OUT]
// nbits : number of bits kept for each value    [IN]
// n     : number of values from w32             [IN}
int  BeStreamXtract(bitstream *p, uint32_t *w32, int nbits, int n){
  int i = 0 ;
  EZ_NEW_XTRACT_VARS(*p) ;                       // declare and get extraction EZ variables

  if(! STREAM_XTRACT_MODE(*p) )   return 0 ;     // ERROR: not in extract mode
  if(! STREAM_IS_BIG_ENDIAN(*p) ) return 0 ;     // wrong endianness

  if(nbits <= 16) {                              // process values two at a time
    uint32_t t, mask = RMASK32(nbits), nb = nbits + nbits ;

    for(    ; i<n-1 ; i+=2){
      BE64_EZ_GET_NBITS(t, nb) ;                 // get a pair of values
      w32[i  ] = t >> nbits ;                    // big endian means upper part first
      w32[i+1] = t & mask ;                      // then lower part
    }
  }
  for(    ; i<n ; i++){
    BE64_EZ_GET_NBITS(w32[i], nbits) ;           // get values
  }
  EZ_SET_XTRACT_VARS(*p) ;                       // update stream using extraction EZ variables
  return n ;
}

// big endian style extraction of signed values from a bit stream
// p     : stream                                [INOUT]
// w32   : array of signed values extracted      [OUT]
// nbits : number of bits kept for each value    [IN]
// n     : number of values from w32             [IN}
int  BeStreamXtractSigned(bitstream *p, int32_t *w32, int nbits, int n){
  int i = 0, n32 = 32 - nbits ;
  EZ_NEW_XTRACT_VARS(*p) ;

  if(! STREAM_XTRACT_MODE(*p) )   return 0 ;     // ERROR: not in extract mode
  if(! STREAM_IS_BIG_ENDIAN(*p) ) return 0 ;     // wrong endianness

  if(nbits <= 16) {                              // process values two at a time
    int32_t t, nb = nbits + nbits ;              //  mask = RMASK32(nbits) ;
    for(    ; i<n-1 ; i+=2){
      BE64_EZ_GET_NBITS(t, nb) ;                 // get a pair of values
      // use shift to propagate sign
      w32[i  ] = t >> nbits ;                    // big endian means upper part first
      w32[i  ] = (w32[i] << n32) >> n32 ;        // restore sign
      w32[i+1] = (t      << n32) >> n32 ;        // then lower part
    }
  }
  for(    ; i<n ; i++){
    BE64_EZ_GET_NBITS(w32[i], nbits) ;           // get values
    w32[i] = (w32[i] << n32) >> n32 ;            // restore sign
  }
  EZ_SET_XTRACT_VARS(*p) ;                       // update stream using extraction EZ variables
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
  EZ_NEW_INSERT_VARS(*p) ;                       // declare and get insertion EZ variables

  if(! STREAM_INSERT_MODE(*p) )      return 0 ;  // ERROR: not in insert mode
  if(! STREAM_IS_LITTLE_ENDIAN(*p) ) return 0 ;  // wrong endianness

  while(n[0] > 0 ){                              // loop until end of list (non positive value)
    for(i=0 ; i<n[0] ; i++){
      LE64_EZ_PUT_NBITS(w32[i], nbits[0]) ;      // insert values using specified number of bits
    }
    w32 += n[0] ;                                // bump source data array pointer
    nw += n[0] ;                                 // bump number of "words" inserted
    nbits++ ;                                    // next pair of n/nbits
    n++ ;
  }
  if(n[0] == -1) LE64_EZ_INSERT_FINAL ;          // flush accumulator
  EZ_SET_INSERT_VARS(*p) ;                       // update stream using insertion EZ variables
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
  EZ_NEW_INSERT_VARS(*p) ;                       // declare and get insertion EZ variables

  if(! STREAM_INSERT_MODE(*p) )   return 0 ;     // ERROR: not in insert mode
  if(! STREAM_IS_BIG_ENDIAN(*p) ) return 0 ;     // wrong endianness

  while(n[0] > 0 ){                              // loop until end of list (non positive value)
    for(i=0 ; i<n[0] ; i++){
      LE64_EZ_PUT_NBITS(w32[i], nbits[0]) ;      // insert values using specified number of bits
    }
    w32 += n[0] ;                                // bump source data array pointer
    nw += n[0] ;                                 // bump number of "words" inserted
    nbits++ ;                                    // next pair of n/nbits
    n++ ;
  }
  if(n[0] == -1) LE64_EZ_INSERT_FINAL ;          // flush accumulator
  EZ_SET_INSERT_VARS(*p) ;                       // update stream using insertion EZ variables
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
  EZ_NEW_XTRACT_VARS(*p) ;                       // declare and get extraction EZ variables

  if(! STREAM_XTRACT_MODE(*p) )      return 0 ;  // ERROR: not in insert mode
  if(! STREAM_IS_LITTLE_ENDIAN(*p) ) return 0 ;  // wrong endianness

  while(n[0] > 0 ){                              // loop until end of list (non positive value)
    for(i=0 ; i<n[0] ; i++){
      LE64_EZ_GET_NBITS(w32[i], nbits[0]) ;      // get values using specified number of bits
    }
    w32 += n[0] ;                                // bump destination data array pointer
    nw += n[0] ;                                 // bump number of "words" extracted
    nbits++ ;                                    // next pair of n/nbits
    n++ ;
  }
  EZ_SET_XTRACT_VARS(*p) ;                       // update stream using extraction EZ variables
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
  EZ_NEW_XTRACT_VARS(*p) ;                       // declare and get extraction EZ variables

  if(! STREAM_XTRACT_MODE(*p) )   return 0 ;     // ERROR: not in insert mode
  if(! STREAM_IS_BIG_ENDIAN(*p) ) return 0 ;     // wrong endianness

  while(n[0] > 0 ){                              // loop until end of list (non positive value)
    for(i=0 ; i<n[0] ; i++){
      BE64_EZ_GET_NBITS(w32[i], nbits[0]) ;      // get values using specified number of bits
    }
    w32 += n[0] ;                                // bump destination data array pointer
    nw += n[0] ;                                 // bump number of "words" extracted
    nbits++ ;                                    // next pair of n/nbits
    n++ ;
  }
  EZ_SET_XTRACT_VARS(*p) ;                       // update stream using extraction EZ variables
  return nw ;
}
