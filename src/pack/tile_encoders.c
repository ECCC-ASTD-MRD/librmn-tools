// Hopefully useful code for C
// Copyright (C) 2022-2025  Recherche en Prevision Numerique
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
// Author:
//     M. Valin,   Recherche en Prevision Numerique, 2022
//
#include <stdio.h>
// #include <rmn/print_bitstream.h>
#include <rmn/tile_encoders.h>
#include <rmn/be_stream.h>
#include <rmn/compare_count.h>

// inline functions borrowed from other source code to minimize code dependencies

// leading zeros count (32 bit word)
static inline int32_t lzcnt_32(uint32_t what){
  uint32_t cnt ;
  __asm__ __volatile__ ("lzcnt %1, %0" : "=r"(cnt) : "r"(what) : "cc" ) ;
  return cnt ;
}

// number of bits needed to represent a 32 bit unsigned number
// uses lzcnt_32 function, that uses the lzcnt instruction
static inline int32_t BitsNeeded_u32(uint32_t what){
  return 32 - lzcnt_32(what) ;
}

// convert signed integer to sign and magnitude form, sign becomes Least Significant Bit
static inline uint32_t to_zigzag_32(int32_t what){
  return (what << 1) ^ (what >> 31) ;
}

// convert to signed integer from sign and magnitude form, sign from Least Significant Bit
static inline int32_t from_zigzag_32(uint32_t what){
  int32_t sign = -(what & 1) ;
  return ((what >> 1) ^ sign) ;
}

static int nblocks = 0 ;
static int constant_block = 0 ;
static int with_offset = 0 ;
static int all_plus = 0 ;
static int all_minus = 0 ;
static int plus_minus = 0 ;
static int short_long[4] = {0,0,0,0} ;

void print_encode_stats(int reset){
  fprintf(stderr, "encoding stats : %d blocks (%d constant, %d >=0, %d <=0) (%d with offset)\n", nblocks, constant_block, all_plus, all_minus, with_offset) ;
  fprintf(stderr, "   ee encoding : %d %d %d %d\n", short_long[0], short_long[1], short_long[2], short_long[3]) ;
  if(nblocks != constant_block + all_plus + all_minus + plus_minus){
    fprintf(stderr, "               inconsistent block count, %d vs %d\n", nblocks, constant_block + all_plus + all_minus + plus_minus) ;
  }
  if(reset){
    nblocks = 0 ;
    constant_block = 0 ;
    with_offset = 0 ;
    all_plus = 0 ;
    all_minus = 0 ;
    plus_minus = 0 ;
    short_long[0] = short_long[1] = short_long[2] = short_long[3] = 0 ;
  }
}

#include <rmn/tile_encoders.h>

// shift count table for short/long encoding
// 4 choices for the value of nshort are provided for each value of nbits
// 0 is always a possible choice
// if value <  (1 << nshort) value may be encoded as a "short" token = value)
// if value >= (1 << nshort) value has to be encoded as a " long" token = value | (1 << nbits)
// a "short" token needs nshort+1 bits, a "long" token needs nbits+1 bits
// nbits > 4 :
// 1+nbits/2 , nbits/2, 1, 0
// nbits > 7 :
// 1+nbits/2 , nbits/2, nbits/4-1, 0
static uint32_t stab[33] = {
  0x00000000 ,    // nbits =  0  (irrelevant, not eligible for short/long encoding)
  0x00000000 ,    // nbits =  1  (irrelevant, not eligible for short/long encoding)
  0x00000000 ,    // nbits =  2  (0 is the only eligible value)
  0x01010100 ,    // nbits =  3  (0, 1 are the only eligible values)
  0x02020100 ,    // nbits =  4  (0, 1, 2 are the only eligible values)
  0x03020100 ,    // nbits =  5
  0x04030100 ,    // nbits =  6
  0x04030100 ,    // nbits =  7
  0x05040100 ,    // nbits =  8
  0x05040100 ,    // nbits =  9
  0x06050100 ,    // nbits = 10
  0x06050100 ,    // nbits = 11
  0x07060200 ,    // nbits = 12
  0x07060200 ,    // nbits = 13
  0x08070200 ,    // nbits = 14
  0x08070200 ,    // nbits = 15
  0x09080300 ,    // nbits = 16
  0x09080300 ,    // nbits = 17
  0x0A090300 ,    // nbits = 18
  0x0A090300 ,    // nbits = 19
  0x0B0A0400 ,    // nbits = 20
  0x0B0A0400 ,    // nbits = 21
  0x0C0B0400 ,    // nbits = 22
  0x0C0B0400 ,    // nbits = 23
  0x0D0C0500 ,    // nbits = 24
  0x0D0C0500 ,    // nbits = 25
  0x0E0D0500 ,    // nbits = 26
  0x0E0D0500 ,    // nbits = 27
  0x0F0E0600 ,    // nbits = 28
  0x0F0E0600 ,    // nbits = 29
  0x100F0600 ,    // nbits = 30
  0x100F0600 ,    // nbits = 31
  0x11100700      // nbits = 32
} ;

// decode nval values from tile[nval] into a bit stream
// tile    OUT] : values to be encoded
// nval    [IN] : number of values
// bp      [IN] : tile properties (see move_blocks.h)
// s_in [INOUT] : pointer to bitstream descriptor (see bitstream.h)
// return nuber of bits extracted from bitstream buffer
// TODO add safety check to make sure we had enough data in stream
int decode_tile(bitstream *s_in, int32_t *tile, int32_t nval){
  int i, nbits, totbits, offset, M, E, head3, head, isminus, iszigzag, lhead, status ;
  uint32_t token, ee, nboffset, uvalue ;
  bitstream s ;

  if(s_in != NULL) s = *s_in ;
  if(s.endian != PACK_ENDIAN) return -1 ;     // stream has the wrong endianness

  STREAM_XTRACT_CHECK(s) ;
  lhead = 12 ;
  STREAM_PEEK_NBITS(s, token, lhead) ;        // header (8 or 12 bits)
  head3 = (token >> 9) ;           // top 3 bits of token
  isminus = iszigzag = 0 ;
fprintf(stderr, "accum = %16.16lx, header = %8.8x, head3 = %8.8x\n", s.acc_x, token, head3) ;
  lhead = 8 ;
  switch(head3){
    case 0b000 :                   // constant tile  000bbbbb....
      nbits = token >> 4 ;
      goto constant_tile ;
      break ;
    case 0b001 :                   // reserved header
      status = -1 ;
      goto error ;
      break ;
    case 0b010 :                   // all values >= 0
    case 0b011 :
fprintf(stderr, "all values >=0\n") ;
      break ;
    case 0b100 :                   // all values <= 0, |value| was stored
    case 0b101 :
fprintf(stderr, "all values <=0\n") ;
      isminus = 1 ;
      break ;
    case 0b110 :                   // mixed signs, zigzag encoding
fprintf(stderr, "all values zigzag\n") ;
      iszigzag = 1 ;
      break ;
    case 0b111 :                   // long header block, nbits > 16
      status = -2 ;
      if((token >> 7) == 0b11100) goto error ;   // reserved header
      lhead = 12 ;
  }
  if(lhead == 8){                  // SSMEbbbb
    token >>= 4 ;
    nbits = token & 0xF ;          // nbits -1
    M = 1 & (token >> 5) ;
    E = 1 & (token >> 4) ;
  }else{                           // 111SSMEnnnnn
    nbits = token & 0x1F ;         // nbits -1
    M = 1 & (token >> 6) ;
    E = 1 & (token >> 5) ;
  }
  STREAM_SKIP_NBITS(s, lhead) ;
  totbits = lhead ;
  nbits++ ;

  if(E != 0){
    STREAM_GET_NBITS(s, ee, 2) ;
    totbits += 2 ;
  }
  if(M != 0){
    STREAM_GET_NBITS(s, nboffset, 5) ;
    STREAM_GET_NBITS(s, token, nboffset) ;
    offset = from_zigzag_32(token) ;
    totbits = totbits + 5 + nboffset ;
  }
  if(E == 0){
    for(i=0 ; i<nval ; i++){
      STREAM_GET_NBITS(s, token, nbits) ;
      tile[i] = token ;
    }
    totbits = totbits + nval * nbits ;
  }

end:
  if(isminus){
    for(i=0 ; i<nval ; i++) tile[i] = -tile[i] ;
  }
  if(iszigzag){
    for(i=0 ; i<nval ; i++) tile[i] = from_zigzag_32((uint32_t) tile[i]) ;
  }
  *s_in = s ;
  return totbits ;

error :
  return status ;

constant_tile:
  STREAM_SKIP_NBITS(s, 8) ;            // skip header
  nbits++ ;
  STREAM_GET_NBITS(s, uvalue, nbits) ; // get zigzag encoded value
  for(i=0 ; i<nval ; i++)              // restore tile values
    tile[i] = from_zigzag_32(uvalue) ;
  totbits = nbits + 8 ;
fprintf(stderr, "constant tile, nbits = %d, value = %d\n", nbits, tile[0]);
  goto end ;
}

// encode nval values from tile[nval] into a bit stream
// tile_in [IN] : values to be encoded
// nval    [IN] : number of values
// bp      [IN] : tile properties (see move_blocks.h)
// s_in [INOUT] : pointer to bitstream descriptor (see bitstream.h)
// return nuber of bits inserted into bitstream buffer
// tile may be modified by this function
// TODO add safety check to make sure we have enough space in stream
int encode_tile(bitstream *s_in, int32_t *tile_in, int32_t nval, block_properties *bp){
  int i, nbits, nbits0, nnbits, offset, nboffset, totbits, SS, M, E, ee, SSME, token, ntoken, range, maxabs, minabs ;
  bitstream s ;
  uint32_t shift ;
  int32_t tile[nval] ;

  if(s_in != NULL) s = *s_in ;
  if(s.endian != PACK_ENDIAN) return -1 ;     // stream has the wrong endianness

  nblocks++ ;
  SS = M = E = ee = 0 ;
  totbits = 0 ;
  offset = 0 ;
  nboffset = 0 ;
  // bp->maxs, bp->nins : signed max and min values in tile
  // bp->maxu, bp->minu : largest and smallest ABSOLUTE values in tile
  // =============================== constant tile ===============================
  if(bp->maxs.i == bp->mins.i){              // constant tile
    uint32_t zigzag ;
    zigzag = to_zigzag_32(tile_in[0]) ;      // encode constant value as sign/magnitude
    nbits = BitsNeeded_u32(zigzag) ;         // number of bits needed to represent encoded value
    token = (0b000 << 5) ;                   // header will be ( 000bbbbb )
    token = token | (nbits-1) ;              // put nbits into header
    STREAM_PUT_NBITS(s, token, 8) ;          // 8 bit header
    STREAM_PUT_NBITS(s, zigzag, nbits) ;     // constant value encoded as zigzag (nbits bits)
    totbits = 8 + nbits ;
    constant_block++ ;
    goto end ;                               // done
  }
  // =============================== convert to positive values ===============================
  // determine number of bits needed to encode largest value
  if(bp->maxs.i > 0 && bp->mins.i < 0){    // both positive and negative values are present
    plus_minus++ ;
    SS = 3 ;                                 // both signs are present
    M  = 0 ;                                 // offset will not be used
    for(i=0 ; i<nval ; i++)                  // convert to sign/magnitude (zigzag)
      tile[i] = to_zigzag_32(tile_in[i]) ;
    nbits = BitsNeeded_u32(to_zigzag_32(bp->maxu.u)) ;   // number of bits needed for largest value

  }else{                                     // all >= 0 or all <= 0
    if(bp->maxs.i <= 0){                     // all values negative or zero
      all_minus++ ;
      SS = 2 ;                               // all values negative flag
      maxabs = -bp->mins.i ;                 // most negative number
      minabs = -bp->maxs.i ;                 // least negative number
      for(i=0 ; i<nval ; i++)                // use absolute value if all <= 0
        tile[i] = -tile_in[i] ;
    }else if(bp->mins.i >= 0){               // all values positive or zero
      all_plus++ ;
      SS = 1 ;                               // all values positive flag
      maxabs = bp->maxs.i ;                  // largest number
      minabs = bp->mins.i ;                  // smallest number
      for(i=0 ; i<nval ; i++)                // straight copy if all >= 0
        tile[i] = tile_in[i] ;
    }
    M = 0 ;                                  // NO OFFSET flag
    nbits  = BitsNeeded_u32(maxabs) ;        // bits needed for largest absolute value
    range  = maxabs - minabs ;
    nbits0 = BitsNeeded_u32(range) ;         // bits needed for range
    if(nbits0 < nbits){                      // less bits needed if subtracting offset
      with_offset++ ;
      offset = minabs ;                      // smallest absolute value
      nboffset = BitsNeeded_u32(offset) ;    // number of bits needed to store offset
      nbits = nbits0 ;
      M = 1 ;                                // set offset flag
      for(i=0 ; i<nval ; i++)
        tile[i] = tile[i] - offset ;         // subtract offset from tile values
    }
  }
  // =============================== determine encoding format ===============================
  // determine if it is worth using short/long encoding
  int ref[4], nref[4], count[4] ;
  shift = stab[nbits] ;
  for(i=0 ; i<4 ; i++){
    nref[i] = shift & 0xFF ;
    ref[i] = 1 << nref[i] ;
    shift >>= 8 ;
  }
  count_lt(count, (int *)tile, ref, nval) ;   // compare tile values to reference values for this value of nbits
fprintf(stderr, "nbits = %d, ref = %8.8x, %8.8x, %8.8x, %8.8x, count = %d, %d, %d, %d\n",
                 nbits, ref[0], ref[1], ref[2], ref[3], count[0], count[1], count[2], count[3]) ;
  int nshort, nlong, nbitsmax, longref, shortref  ;
  E = 0 ;
  ee = 0 ;
  nbitsmax = nbits * nval ;
  if(nbits > 1){   // pointless if nbits < 2
    for(i=0 ; i<4 ; i++){
      int nbitsi = count[i] * (nref[i]+1) + (nval-count[i]) * (nbits+1) ;  // short/long with nref[i]/nbits
      if(nbitsmax > nbitsi){
        nbitsmax = nbitsi ;
        nshort   = nref[i] ;
        E = 1 ;
        ee = i ;
        short_long[ee]++ ;
      }
    }
  }
  // at this point nbitsmax is equal to the number or bits needed for encoding thevalues
  // =============================== store header ===============================
  SSME = (SS << 2) | (M << 1) | E ;
  nnbits = (nbits > 16) ? 5 : 4 ;       // number of bits needed to store nbits
  if(nbits > 16){
    token = 0b111 ;
    ntoken = 3 ;
  }else{
    token = 0 ;
    ntoken = 0 ;
  }
  token = (token << 4) | SSME ;
  token = (token << nnbits) | (nbits -1) ;     // SSMEbbbb or 111SSMEbbbbb
  ntoken = ntoken + nnbits + 4 ;
  if(E != 0){
    token = (token << 2) | ee ;                // SSM1bbbbee or 111SSM1bbbbbee
    ntoken = ntoken + 2 ;
  }
  STREAM_PUT_NBITS(s, token, ntoken) ;
  totbits += ntoken ;
  // NOTE dangerous if nboffset > 27
  if(M != 0){
    STREAM_PUT_NBITS(s, (nboffset-1), 5) ;     // number of bits needed for offset
    STREAM_PUT_NBITS(s, offset, nboffset) ;    // offset
    totbits = totbits + 5 + nboffset ;
  }
  // =============================== store encoded values ===============================
  if(E == 0){                        // no short/long encoding, all tokens will be nbits long
    for(i=0 ; i<nval ; i++){
      STREAM_PUT_NBITS(s, tile[i], nbits) ;
    }
  }else{                             // use short/long encoding, tokens will be nshort+1 or nbits+1 bits long
    longref = (1 << nbits) ;         // upper 1 bit (above token)
    nlong = nbits + 1 ;              // nb of bits used for long tokens
    shortref = (1 << nshort) ;       // anything < shortref can be coded as a "short" token
    nshort++ ;                       // nb of bits used for short tokens
    for(i=0 ; i<nval ; i++){         // encode values
      token = tile[i] ;              // value to encode
      if(token < shortref){          // use "short" token
        STREAM_PUT_NBITS(s, token, nshort) ;
      }else{                         // use "long" token
        token |= longref ;           // add "long" token marker
        STREAM_PUT_NBITS(s, token, nlong) ;
      }
    }
  }
  totbits += nbitsmax ;

end:
  *s_in = s ;
  return totbits ;
}

// deactivate old code
#if 0

#define USE_INTEL_SIMD_INTRINSICS
#include <rmn/simd_functions.h>
#define WITH_SIMD
// #include <with_simd.h>

#define NB0 (2)

// print tile properties as set by encode_tile_properties
// p64 [IN] : blind token containing tile properties 
// (from encode_tile_scheme/constant_tile_scheme/encode_tile_properties)
// TODO: npti, nptj or npij ?
void print_tile_properties(uint64_t p64){
  tile_properties p ;
  int nij, nbits, nbitss, nb0, nbi ;

  p.u64 = p64 ;
  nij = p.t.h.npij + 1 ;    // (p.t.h.npti+1)*(p.t.h.nptj+1) ;
  nbi = p.t.h.nbts + 1 ;
  nbits = nij * nbi ;
//   nb0 = (nbi + NB0) >> 1 ;
//   nbts0 = (nij - p.t.nzero)*(nbi + 1) + p.t.nzero ;
//   nbitss = (nij - p.t.nshort)*(nbi + 1) + p.t.nshort * (nb0 + 1) ;
//   fprintf(stderr, "(t_p) nbits = %d(%d), sign = %d, encoding = %d, %d x %d, min0 = %d, min = %8.8x, nzero = %3d, nshort = %3d, bits = %d/%d/%d\n",
//           nbi, nb0, p.t.h.sign, p.t.h.encd, p.t.h.npti+1, p.t.h.nptj+1, p.t.h.min0, p.t.min, p.t.nzero, p.t.nshort, nbits, nbts0, nbitss) ;
  nb0 = p.t.bshort ;   // number of bits for short tokens ( 0 -> nbi )
  nbitss = (nij - p.t.nshort)*(nbi + 1) + p.t.nshort * (nb0 + 1) ;
//   fprintf(stderr, "(t_p) nbits = %d(%d), sign = %d, encoding = %d, %d x %d, min0 = %d, min = %8.8x, bshort = %3d, nshort = %3d, bits = %d/%d\n",
//           nbi, nb0, p.t.h.sign, p.t.h.encd, p.t.h.npti+1, p.t.h.nptj+1, p.t.h.min0, p.t.min, p.t.bshort, p.t.nshort, nbits, nbitss) ;
  fprintf(stderr, "(t_p) nbits = %d(%d), sign = %d, encoding = %d, n = %d, min0 = %d, min = %8.8x, bshort = %3d, nshort = %3d, bits = %d/%d\n",
          nbi, nb0, p.t.h.sign, p.t.h.encd, p.t.h.npij+1, p.t.h.min0, p.t.min, p.t.bshort, p.t.nshort, nbits, nbitss) ;
}

#if 0
// set encoding scheme deemed appropriate given tile properties
// p64 [IN] : tile parameter structure as an "opaque" 64 bit value
// the function returns a tile parameter structure as an "opaque" 64 bit value
// this value will eventually be passed to the encoder itself
// used by encode_tile_properties
// TODO: npti, nptj or npij ?
static uint64_t encode_tile_scheme(uint64_t p64){
  tile_properties p ;
//   int nbits_temp, nbits_full, nbits_zero, nbits_short, nij, nbits, nbits0, nzero, nshort ;
  int nbits_temp, nbits_full, nbits_short, nij, nbits, nbits0, nshort ;

  p.u64 = p64 ;
fprintf(stderr, "DEBUG: encd = %d, nshort = %3d, bshort = %2d, nbits = %2d\n", p.t.h.encd, p.t.nshort, p.t.bshort, p.t.h.nbts + 1) ;
  nij    = p.t.h.npij + 1 ;      //  (p.t.h.npti + 1) * (p.t.h.nptj + 1) ;                         // number of points
  nbits = p.t.h.nbts + 1 ;
//   nbits0 = (nbits + NB0) >> 1 ;  // number of bits for "short" values
  nshort = p.t.nshort ;
  nbits0 = p.t.bshort ;   // number of bits for short tokens ( 0 -> nbits )

  nbits_full  = nij * nbits ;                                            // all tokens at full length
//   nbits_zero  = nzero + (nij - nzero) * (nbits + 1) ;                    // nzero tokens at length 1, others nbits + 1
  nbits_short = nshort * (nbits0 + 1) + (nij - nshort) * (nbits + 1) ;   // short tokens tokens nbits0 + 1, others nbits + 1

  nbits_temp = nbits_full ;
  p.t.h.encd = 0 ;                        // default is no special encoding
//   p.t.nshort = p.t.nzero = 0 ;            // no "short" or "0" token
//   if(nbits_zero < nbits_temp){            // encoding 2 better ?
//     nbits_temp = nbits_zero ;
//     p.t.h.encd = 2 ;                      // use 0 , 1//nbits encoding
//   }
  if(nbits_short < nbits_temp){           // encoding 1 better ?
    p.t.h.encd = nbits0 ? 1 : 2 ;         // use 0//nbits0 , 1//nbits (1) OR  0 , 1//nbits (2) encoding
    p.t.nshort = nshort ;                 // "short" tokens used
    p.t.bshort = nbits0 ;                 // number of bits for short tokens
  }
// fprintf(stderr, "normal tile, min = %8.8x (%d x %d)\n", p.t.min, p.t.h.npti+1, p.t.h.nptj+1) ;
fprintf(stderr, "DEBUG: encd = %d, nshort = %3d, bshort = %2d\n", p.t.h.encd, p.t.nshort, p.t.bshort) ;
  return p.u64 ;
}
#endif

// set information appropriate to a constant valued tile
// p64 [IN] : tile parameter structure as an "opaque" 64 bit value
// the function returns a tile parameter structure as an "opaque" 64 bit value
// this value will eventually be passed to the encoder itself
// used by encode_tile_properties
static uint64_t constant_tile_scheme(uint64_t p64){
  tile_properties p ;

  p.u64   = p64 ;
// fprintf(stderr, "constant tile, min = %8.8x (%d x %d)\n", p.t.min, p.t.h.npti+1, p.t.h.nptj+1) ;
  p.t.h.encd = 3 ;     // constant tile
  p.t.h.min0 = 0 ;     // not needed
  p.t.bshort = 0 ;     // not needed
  p.t.nshort = 0 ;     // not needed

  if(p.t.min == 0){    // is constant value (minimum) zero ?
    p.t.h.sign = 0 ;   // all values 0
    p.t.h.nbts = 0 ;   // nbts is 0 if zero tile (0 means 1 bit)
  }else{
    p.t.h.sign = 3 ;   // full ZigZag coding
    p.t.h.nbts = 32 - lzcnt_32(p.t.min) - 1 ;  // number of bits needed to encode constant value
  }
  return p.u64 ;
}

// gather tile to be encoded, determine useful data properties
// f     [IN] : array of 32 bit values from which tile will be extracted
// ni    [IN] : first dimension of tile to be encoded (1 <= ni <= 8)
// lni   [IN] : row storage length in array f
// nj    [IN] : second dimension of tile to be encoded (1 <= nj <= 8)
// tile [OUT] : extracted tile (in sign/magnitude form and with minimum value possibly subtracted)
// the function returns a tile parameter structure as an "opaque" 64 bit value
// this value will then be passed to the encoder itself
// generic version with no explicit SIMD function calls
// called by encode_tile_properties for non 8x8 tiles or if SIMD functions are not available
// TODO: process nj = 1, 1 <= ni <= 64, lni = dont't care (should be == ni) correctly
// TODO: npti, nptj or npij ?
// PURE C code, no SIMD primitives used
static uint64_t encode_tile_properties_c(void *f, int ni, int lni, int nj, uint32_t tile[64]){
  uint32_t *block = (uint32_t *)f ;
  uint32_t pos, neg;
  int32_t nshort, nshort1, nzero, ntot, nref, nnew ;
  uint32_t max, min, nbits, nbits0, mask0, mask1 ;
  tile_properties p ;
  int32_t i, j, i0, nij ;

  p.u64 = 0 ;
  // TODO: ERROR if ni * nj > 64 or <= 0
  // TODO: fix code if nj == 1, 1 <= ni <= 64
//   p.t.h.npti = ni - 1 ;
//   p.t.h.nptj = nj - 1;
  p.t.h.npij = ni * nj - 1 ;
  i0 = 0 ;
  nij = ni * nj ;
  for(j=0 ; j<nj ; j++){                     // gather tile from array f
    for(i=0 ; i<ni ; i++) tile[i+i0] = block[i] ;
    block += lni ;
    i0 += ni ;
  }
  neg = 0xFFFFFFFFu ;
  pos = 0xFFFFFFFFu ;
  min = 0xFFFFFFFFu ;
  max = 0u ;
  for(i=0 ; i<nij ; i++){
    neg &= tile[i] ;                         // MSB will be 1 only if all values < 0
    pos &= (~tile[i]) ;                      // MSB will be 1 only if all values >= 0
    tile[i] = to_zigzag_32(tile[i]) ;
    max = (tile[i] > max) ? tile[i] : max ;  // largest ZigZag value
    min = (tile[i] < min) ? tile[i] : min ;  // smallest ZigZag value
  }
  if(max == min) goto constant ;             // same value for entire tile (zero or nonzero)

  nbits = 32 - lzcnt_32(max) ;               // max controls nbits (if vmax is 0, nbits will be 0)
//   if( (32 - (lzcnt_32(max-min)) < nbits) || 1 ){   // we gain one bit if subtracting the minimum value
  if( (32 - (lzcnt_32(max-min)) < nbits) ){  // we gain one bit if subtracting the minimum value
    max = max - min ;
    for(i=0 ; i<nij ; i++) tile[i] -= min ;
    p.t.h.min0 = 1 ;                         // min will be used
  }else{
    p.t.h.min0 = 0 ;                         // min will not be used
  }
  nbits = 32 - lzcnt_32(max) ;               // number of bits needed to encode largest ZigZag value

  neg >>= 31 ;                               // 1 if all values < 0
  pos >>= 31 ;                               // 1 if all values >= 0
  if(neg || pos) {                           // no need to encode sign, it is the same for all values
    nbits-- ;                                // and will be found in the tile header
    min >>= 1 ;                              // get rid of sign bit for min
    for(i=0 ; i<nij ; i++)
      tile[i] >>= 1 ;                        // get rid of sign bit from ZigZag
    p.t.h.sign = (neg << 1) | pos ;          // store proper sign code
  }else{
    p.t.h.sign = 3 ;                         // both positive and negative values, ZigZag encoding with sign bit in LSB
  }
  if(min == 0) p.t.h.min0 = 0 ;              // minimum value is 0, no need to keep it
  p.t.min = min ;
  p.t.h.nbts = nbits - 1 ;
  p.t.h.encd = 0 ;                           // default is NO special encoding
  nbits0 = (nbits + NB0) >> 1 ;              // number of bits for "short" values
  mask0 = RMASK31(nbits0) ;                  // value & mask0 will be 0 if nbits0 or less bits are needed
  mask1 = mask0 >> 1 ;
  mask0 = ~mask0 ;                           // keep only the upper bits
  mask1 = ~mask1 ;

  nshort = nzero = nshort1 = 0 ;
// can cut it short here if NO ENCODING will be forced, 
// if() { nshort = bshort = nsaved = 0 ; goto end ; }
  for(i=0 ; i<nij ; i++){
    if(tile[i] == 0)           nzero++ ;      // zero value
    if((tile[i] & mask0) == 0) nshort++ ;     // value needing nbits0   bits or less
    if((tile[i] & mask1) == 0) nshort1++ ;    // value needing nbits0-1 bits or less
  }
  p.t.bshort = 0 ;
  p.t.nshort = nzero ;
  ntot = nzero + (nij - nzero) * (nbits + 1) ;
  nref = nbits * nij ;
// fprintf(stderr, "DEBUG: (tile_properties_c) nshort = %3d, bshort = %2d, bsaved = %5d\n", p.t.nshort, p.t.bshort, nref - ntot) ;
  nnew = 5 + nshort * (nbits0 + 1) + (nij - nshort) * (nbits + 1) ;
// fprintf(stderr, "DEBUG: (tile_properties_c) nshort = %3d, bshort = %2d, bsaved = %5d\n", nshort, nbits0, nref - nnew) ;
  if(ntot > nnew){
    p.t.bshort = nbits0 ;
    p.t.nshort = nshort ;
    ntot = nnew ;
  }
  nnew = 5 + nshort1 * (nbits0) + (nij - nshort1) * (nbits + 1) ;
// fprintf(stderr, "DEBUG: (tile_properties_c) nshort = %3d, bshort = %2d, bsaved = %5d\n", nshort1, nbits0-1, nref - nnew) ;
  if(ntot > nnew){
    p.t.bshort = nbits0 - 1 ;
    p.t.nshort = nshort1 ;
    ntot = nnew ;
  }
  if(ntot >= nref){                    // no bit saving through encoding
    p.t.h.encd = 0 ;                          // no encoding
  }else{
    p.t.h.encd = (p.t.bshort == 0) ? 2 : 1 ;  // 0 , 1//full or 0//short , 1//full encoding
  }

// end:
// fprintf(stderr, "DEBUG: (tile_properties_c) nshort = %3d, bshort = %2d, bsaved = %5d, encoding = %d\n", p.t.nshort, p.t.bshort, nref - ntot, p.t.h.encd) ;
  return p.u64 ;
//   return encode_tile_scheme(p.u64) ;          // determine appropriate encoding scheme

constant:
  p.t.min = min ;                             // constant value in min
  return constant_tile_scheme(p.u64) ;        // setup for constant values
}

// tile [IN] : array of 64 x 32 bit values to be compared with 4 reference values
// pop [OUT] : 4 element array to receive population counts
// ref  [IN] : 4 element array containing reference values
// pop[i] will receive the number of values in tile < ref[i]
// NOTE: the AVX2 version is way faster than the dumb vanilla C version
//       the chunked by 8 C version is 1.3 - 3 x slower than the AVX2 version with most compilers
//       and much slower (up to 10x) with some others
// the comparison is performed in SIGNED mode
static void tile_population_64(void *tile_in, int32_t pop[4], void *ref_in){
  int32_t *tile = (int32_t *) tile_in ;
  int32_t *ref = (int32_t *) ref_in ;
#if defined(__x86_64__) && defined(__AVX2__) && defined(WITH_SIMD)
  __m256i v0, v1, v2, v3, v4, v5, v6, v7 ;
  __m256i t0, t1, s0, s1 ;
  __m128i i0, i1 ;

  v0 = _mm256_loadu_si256((__m256i *)(tile+ 0)) ;   // load 8 x 8 values from tile
  v1 = _mm256_loadu_si256((__m256i *)(tile+ 8)) ;
  v2 = _mm256_loadu_si256((__m256i *)(tile+16)) ;
  v3 = _mm256_loadu_si256((__m256i *)(tile+24)) ;
  v4 = _mm256_loadu_si256((__m256i *)(tile+32)) ;
  v5 = _mm256_loadu_si256((__m256i *)(tile+40)) ;
  v6 = _mm256_loadu_si256((__m256i *)(tile+48)) ;
  v7 = _mm256_loadu_si256((__m256i *)(tile+56)) ;

  t0 = _mm256_set1_epi32(ref[0]) ;
  t1 = _mm256_set1_epi32(ref[1]) ;
  // cmpgt result is -1 where reference is larger than value, 0 otherwise
  s0 =                      _mm256_cmpgt_epi32(t0, v0)  ; s1 =                      _mm256_cmpgt_epi32(t1, v0)  ;
  s0 = _mm256_add_epi32(s0, _mm256_cmpgt_epi32(t0, v1)) ; s1 = _mm256_add_epi32(s1, _mm256_cmpgt_epi32(t1, v1)) ;
  s0 = _mm256_add_epi32(s0, _mm256_cmpgt_epi32(t0, v2)) ; s1 = _mm256_add_epi32(s1, _mm256_cmpgt_epi32(t1, v2)) ;
  s0 = _mm256_add_epi32(s0, _mm256_cmpgt_epi32(t0, v3)) ; s1 = _mm256_add_epi32(s1, _mm256_cmpgt_epi32(t1, v3)) ;
  s0 = _mm256_add_epi32(s0, _mm256_cmpgt_epi32(t0, v4)) ; s1 = _mm256_add_epi32(s1, _mm256_cmpgt_epi32(t1, v4)) ;
  s0 = _mm256_add_epi32(s0, _mm256_cmpgt_epi32(t0, v5)) ; s1 = _mm256_add_epi32(s1, _mm256_cmpgt_epi32(t1, v5)) ;
  s0 = _mm256_add_epi32(s0, _mm256_cmpgt_epi32(t0, v6)) ; s1 = _mm256_add_epi32(s1, _mm256_cmpgt_epi32(t1, v6)) ;
  s0 = _mm256_add_epi32(s0, _mm256_cmpgt_epi32(t0, v7)) ; s1 = _mm256_add_epi32(s1, _mm256_cmpgt_epi32(t1, v7)) ;
  // fold sums
  i0 = _mm_add_epi32(_mm256_extracti128_si256(s0, 0) , _mm256_extracti128_si256(s0, 1)) ;   // 8 sums -> 4 sums
  i1 = _mm_add_epi32(_mm256_extracti128_si256(s1, 0) , _mm256_extracti128_si256(s1, 1)) ;
  i0 = _mm_add_epi32(i0, _mm_shuffle_epi32(i0, 0b11101110)) ;         // 4 sums -> 2 sums
  i1 = _mm_add_epi32(i1, _mm_shuffle_epi32(i1, 0b11101110)) ;
  i0 = _mm_add_epi32(i0, _mm_shuffle_epi32(i0, 0b01010101)) ;         // 2 sums -> 1 sum
  i1 = _mm_add_epi32(i1, _mm_shuffle_epi32(i1, 0b01010101)) ;
  _mm_storeu_si32(pop  , i0) ; pop[0] = -pop[0] ;  // make sum positive
  _mm_storeu_si32(pop+1, i1) ; pop[1] = -pop[1] ;

  t0 = _mm256_set1_epi32(ref[2]) ;
  t1 = _mm256_set1_epi32(ref[3]) ;
  s0 =                      _mm256_cmpgt_epi32(t0, v0)  ; s1 =                      _mm256_cmpgt_epi32(t1, v0)  ;
  s0 = _mm256_add_epi32(s0, _mm256_cmpgt_epi32(t0, v1)) ; s1 = _mm256_add_epi32(s1, _mm256_cmpgt_epi32(t1, v1)) ;
  s0 = _mm256_add_epi32(s0, _mm256_cmpgt_epi32(t0, v2)) ; s1 = _mm256_add_epi32(s1, _mm256_cmpgt_epi32(t1, v2)) ;
  s0 = _mm256_add_epi32(s0, _mm256_cmpgt_epi32(t0, v3)) ; s1 = _mm256_add_epi32(s1, _mm256_cmpgt_epi32(t1, v3)) ;
  s0 = _mm256_add_epi32(s0, _mm256_cmpgt_epi32(t0, v4)) ; s1 = _mm256_add_epi32(s1, _mm256_cmpgt_epi32(t1, v4)) ;
  s0 = _mm256_add_epi32(s0, _mm256_cmpgt_epi32(t0, v5)) ; s1 = _mm256_add_epi32(s1, _mm256_cmpgt_epi32(t1, v5)) ;
  s0 = _mm256_add_epi32(s0, _mm256_cmpgt_epi32(t0, v6)) ; s1 = _mm256_add_epi32(s1, _mm256_cmpgt_epi32(t1, v6)) ;
  s0 = _mm256_add_epi32(s0, _mm256_cmpgt_epi32(t0, v7)) ; s1 = _mm256_add_epi32(s1, _mm256_cmpgt_epi32(t1, v7)) ;
  // fold sums
  i0 = _mm_add_epi32(_mm256_extracti128_si256(s0, 0) , _mm256_extracti128_si256(s0, 1)) ;
  i1 = _mm_add_epi32(_mm256_extracti128_si256(s1, 0) , _mm256_extracti128_si256(s1, 1)) ;
  i0 = _mm_add_epi32(i0, _mm_shuffle_epi32(i0, 0b11101110)) ;
  i1 = _mm_add_epi32(i1, _mm_shuffle_epi32(i1, 0b11101110)) ;
  i0 = _mm_add_epi32(i0, _mm_shuffle_epi32(i0, 0b01010101)) ;
  i1 = _mm_add_epi32(i1, _mm_shuffle_epi32(i1, 0b01010101)) ;
  _mm_storeu_si32(pop+2, i0) ; pop[2] = -pop[2] ;  // make sum positive
  _mm_storeu_si32(pop+3, i1) ; pop[3] = -pop[3] ;

#else
  int i0, i, ns0[8], ns1[8], ns2[8], ns3[8] ;

  for(i=0 ; i<8 ; i++) {
    ns0[i] = ns1[i] = ns2[i] = ns3[i] = 0 ;
  }
  for(i0=0 ; i0<57 ; i0+=8){
    for(i=0 ; i<8 ; i++){
      ns0[i] += (( ref[0] > tile[i]) ? 1 : 0) ;
      ns1[i] += (( ref[1] > tile[i]) ? 1 : 0) ;
      ns2[i] += (( ref[2] > tile[i]) ? 1 : 0) ;
      ns3[i] += (( ref[3] > tile[i]) ? 1 : 0) ;
    }
    tile += 8 ;
  }
  pop[0] = pop[1] = pop[2] = pop[3] = 0 ;
  for(i=0 ; i<8 ; i++){
    pop[0] += ns0[i] ;
    pop[1] += ns1[i] ;
    pop[2] += ns2[i] ;
    pop[3] += ns3[i] ;
  }

#endif
}

// tile [IN] : array of 32 bit elements to be compared with 4 reference values
// n    [IN] : size of tile
// pop [OUT] : 4 element array to receive population counts
// ref  [IN] : 4 element array containing reference values
// pop[i] will receive the number of values in tile < ref[i]
// this function handles n != 64, calls tile_population_64 if n == 64
void tile_population(void *tile_in, int n, int32_t pop[4], void *ref_in){
  int32_t *tile = (int32_t *) tile_in ;
  int32_t *ref = (int32_t *) ref_in ;
  int i0, i, ns0[8], ns1[8], ns2[8], ns3[8] ;

  if(n == 64){  // full tile (64 values)
    tile_population_64(tile, pop, ref) ;
    return ;
  }

  for(i=0 ; i<8 ; i++) {ns0[i] = ns1[i] = ns2[i] = ns3[i] = 0 ; }
  for(i0=0 ; i0<n-7 ; i0+=8){
    for(i=0 ; i<8 ; i++){
      ns0[i] += (( ref[0] > tile[i]) ? 1 : 0) ;
      ns1[i] += (( ref[1] > tile[i]) ? 1 : 0) ;
      ns2[i] += (( ref[2] > tile[i]) ? 1 : 0) ;
      ns3[i] += (( ref[3] > tile[i]) ? 1 : 0) ;
    }
    tile += 8 ;
  }
  for(i=0 ; i<(n&7) ; i++){
    ns0[i] += (( ref[0] > tile[i]) ? 1 : 0) ;
    ns1[i] += (( ref[1] > tile[i]) ? 1 : 0) ;
    ns2[i] += (( ref[2] > tile[i]) ? 1 : 0) ;
    ns3[i] += (( ref[3] > tile[i]) ? 1 : 0) ;
  }
  for(i=0 ; i<4 ; i++) pop[i] = 0 ;
  for(i=0 ; i<8 ; i++){
    pop[0] += ns0[i] ;
    pop[1] += ns1[i] ;
    pop[2] += ns2[i] ;
    pop[3] += ns3[i] ;
  }
// dumb C version, kept for reference
//   pop[0] = pop[1] = pop[2] = pop[3] = 0 ;
//   for(i=0 ; i<n ; i++){
//     pop[0] += (( ref[0] > tile[i]) ? 1 : 0) ;
//     pop[1] += (( ref[1] > tile[i]) ? 1 : 0) ;
//     pop[2] += (( ref[2] > tile[i]) ? 1 : 0) ;
//     pop[3] += (( ref[3] > tile[i]) ? 1 : 0) ;
//   }
// }
}

// gather tile to be encoded, determine useful data properties
// f     [IN] : array from which block will be extracted
// ni    [IN] : first dimension of tile to be encoded (1 <= ni <= 8)
// lni   [IN] : row storage length in array f
// nj    [IN] : second dimension of tile to be encoded (1 <= nj <= 8)
// tile [OUT] : extracted tile (in sign/magnitude form and with minimum value possibly subtracted)
// the function returns a tile parameter structure as an "opaque" 64 bit value
// this value will then be passed to the encoder itself
// X86_64 AVX2 version, 8 x 8 tiles only (ni == 8, nj == 8) (FULL TILE)
// NOTE: for a 1D full tile, ni = 8, nj = 8, lni = ni
//       otherwise, for a 1D tile, nj = 1, 1 <= ni <= 64, lni = ni
// TODO: npti, nptj or npij ?
uint64_t encode_tile_properties(void *f, int ni, int lni, int nj, uint32_t tile[64]){
  uint32_t *block = (uint32_t *)f ;
  int32_t pos, neg, nzero, nsaved, nij = ni * nj ;
  uint32_t max, min, nbits, nbits0, nshort, bshort ;
  tile_properties p ;

#if defined(__x86_64__) && defined(__AVX2__) && defined(WITH_SIMD)
  __m256i v0, v1, v2, v3, v4, v5, v6, v7 ;
  __m256i vp, vn, vz, vs, vm0, vm1, vmax, vmin, vz0 ;
  __m128i v128 ;
  int32_t bsaved = 0 ;

//   if(ni != 8 || nj != 8) return encode_tile_properties_c(f, ni, lni, nj, tile) ; // not a full 8x8 tile
  if(nij <= 0 || nij > 64) return 0xFFFFFFFFFFFFFFFFlu ;                // invalid tile dimensions
  if(nij != 64) return encode_tile_properties_c(f, ni, lni, nj, tile) ; // not a full 8x8 tile

  p.u64 = 0 ;
  p.t.h.npij = nij - 1 ;
//   if(ni > 8){   // 1D tile
//     nij = nij - 1 ;
//     p.t.h.npti = nij & 0x7 ;
//     p.t.h.nptj = nij >> 3 ;
//   }else{
//     p.t.h.npti = ni - 1 ;
//     p.t.h.nptj = nj - 1;
//   }

  v0 = _mm256_loadu_si256((__m256i *)(block)) ; block += lni ;  // load 8 x 8 values from original array
  v1 = _mm256_loadu_si256((__m256i *)(block)) ; block += lni ;
  v2 = _mm256_loadu_si256((__m256i *)(block)) ; block += lni ;
  v3 = _mm256_loadu_si256((__m256i *)(block)) ; block += lni ;
  v4 = _mm256_loadu_si256((__m256i *)(block)) ; block += lni ;
  v5 = _mm256_loadu_si256((__m256i *)(block)) ; block += lni ;
  v6 = _mm256_loadu_si256((__m256i *)(block)) ; block += lni ;
  v7 = _mm256_loadu_si256((__m256i *)(block)) ;

  vn = _mm256_and_si256(v0, v1) ; vp = _mm256_or_si256(v0, v1) ;  // "and" for negative, "or" for non negative
  vn = _mm256_and_si256(vn, v2) ; vp = _mm256_or_si256(vp, v2) ;
  vn = _mm256_and_si256(vn, v3) ; vp = _mm256_or_si256(vp, v3) ;
  vn = _mm256_and_si256(vn, v4) ; vp = _mm256_or_si256(vp, v4) ;
  vn = _mm256_and_si256(vn, v5) ; vp = _mm256_or_si256(vp, v5) ;
  vn = _mm256_and_si256(vn, v6) ; vp = _mm256_or_si256(vp, v6) ;
  vn = _mm256_and_si256(vn, v7) ; vp = _mm256_or_si256(vp, v7) ;

  pos = (_mm256_movemask_ps((__m256) vp) == 0x00) ? 1 : 0 ;       // check that all MSB bits are off (no negative value)
  neg = (_mm256_movemask_ps((__m256) vn) == 0xFF) ? 1 : 0 ;       // check that all MSB bits are on (all values negative)

#define VX2(vx) _mm256_add_epi32(vx, vx)
  if(pos || neg){                                                 // all values have same sign
    v0 = _mm256_xor_si256(v0 ,      _mm256_srai_epi32(v0, 31) ) ; // convert to unsigned ZigZag
    v1 = _mm256_xor_si256(v1 ,      _mm256_srai_epi32(v1, 31) ) ; // xor value with sign >> 31
    v2 = _mm256_xor_si256(v2 ,      _mm256_srai_epi32(v2, 31) ) ;
    v3 = _mm256_xor_si256(v3 ,      _mm256_srai_epi32(v3, 31) ) ;
    v4 = _mm256_xor_si256(v4 ,      _mm256_srai_epi32(v4, 31) ) ;
    v5 = _mm256_xor_si256(v5 ,      _mm256_srai_epi32(v5, 31) ) ;
    v6 = _mm256_xor_si256(v6 ,      _mm256_srai_epi32(v6, 31) ) ;
    v7 = _mm256_xor_si256(v7 ,      _mm256_srai_epi32(v7, 31) ) ;
    p.t.h.sign = (neg << 1) | pos ;
  }else{                                                          // mixed signs, convert to signed ZigZag (sign in LSB)
    v0 = _mm256_xor_si256(VX2(v0) , _mm256_srai_epi32(v0, 31) ) ; // xor 2*value with sign >> 31
    v1 = _mm256_xor_si256(VX2(v1) , _mm256_srai_epi32(v1, 31) ) ;
    v2 = _mm256_xor_si256(VX2(v2) , _mm256_srai_epi32(v2, 31) ) ;
    v3 = _mm256_xor_si256(VX2(v3) , _mm256_srai_epi32(v3, 31) ) ;
    v4 = _mm256_xor_si256(VX2(v4) , _mm256_srai_epi32(v4, 31) ) ;
    v5 = _mm256_xor_si256(VX2(v5) , _mm256_srai_epi32(v5, 31) ) ;
    v6 = _mm256_xor_si256(VX2(v6) , _mm256_srai_epi32(v6, 31) ) ;
    v7 = _mm256_xor_si256(VX2(v7) , _mm256_srai_epi32(v7, 31) ) ;
    p.t.h.sign = 3 ;                  // both positive and negative values, ZigZag encoding with sign bit
  }

  vmax = _mm256_max_epu32(v0  , v1) ; vmin = _mm256_min_epu32(v0  , v1) ;  // max and min of ZigZag values
  vmax = _mm256_max_epu32(vmax, v2) ; vmin = _mm256_min_epu32(vmin, v2) ;
  vmax = _mm256_max_epu32(vmax, v3) ; vmin = _mm256_min_epu32(vmin, v3) ;
  vmax = _mm256_max_epu32(vmax, v4) ; vmin = _mm256_min_epu32(vmin, v4) ;
  vmax = _mm256_max_epu32(vmax, v5) ; vmin = _mm256_min_epu32(vmin, v5) ;
  vmax = _mm256_max_epu32(vmax, v6) ; vmin = _mm256_min_epu32(vmin, v6) ;
  vmax = _mm256_max_epu32(vmax, v7) ; vmin = _mm256_min_epu32(vmin, v7) ;

  v128 = _mm_min_epu32( _mm256_extractf128_si256 (vmin, 0) ,  _mm256_extractf128_si256 (vmin, 1) ) ;  // reduce to 1 value for min
  v128 = _mm_min_epu32(v128, _mm_shuffle_epi32(v128, 0b11101110) ) ; // [0,1,2,3] [2,2,2,3]
  v128 = _mm_min_epu32(v128, _mm_shuffle_epi32(v128, 0b01010101) ) ; // [0,1,2,3] [1,1,1,1]
  vmin = _mm256_broadcastd_epi32(v128) ;                             // [0,0,0,0,0,0,0,0]
  _mm_storeu_si32(&min, v128) ;

  v128 = _mm_max_epu32( _mm256_extractf128_si256 (vmax, 0) ,  _mm256_extractf128_si256 (vmax, 1) ) ;  // reduce to 1 value for max
  v128 = _mm_max_epu32(v128, _mm_shuffle_epi32(v128, 0b11101110) ) ; // [0,1,2,3] [2,2,2,3]
  v128 = _mm_max_epu32(v128, _mm_shuffle_epi32(v128, 0b01010101) ) ; // [0,1,2,3] [1,1,1,1]
  _mm_storeu_si32(&max, v128) ;

  if(max == min) {
    min <<= 1 ;      // signed zigzag coding for min
    min |= neg ;     // restore sign if negative
    goto constant;   // constant field
  }
  nbits = 32 - lzcnt_32(max) ;
//   if( (32 - (lzcnt_32(max-min)) < nbits) || 1 ){   // we gain one bit if subtracting the minimum value
  if( (32 - (lzcnt_32(max-min)) < nbits) ){   // we gain at least one bit if subtracting the minimum value
    max = max - min ;
    v0 = _mm256_sub_epi32(v0, vmin) ; // subtract min from max and vo -> v7
    v1 = _mm256_sub_epi32(v1, vmin) ;
    v2 = _mm256_sub_epi32(v2, vmin) ;
    v3 = _mm256_sub_epi32(v3, vmin) ;
    v4 = _mm256_sub_epi32(v4, vmin) ;
    v5 = _mm256_sub_epi32(v5, vmin) ;
    v6 = _mm256_sub_epi32(v6, vmin) ;
    v7 = _mm256_sub_epi32(v7, vmin) ;
    p.t.h.min0 = 1 ;                  // min will be used
  }else{
    p.t.h.min0 = 0 ;                  // min will not be used
  }
  nbits = 32 - lzcnt_32(max) ;        // number of bits needed to encode max

  _mm256_storeu_si256((__m256i *)(tile   ), v0) ;  // store ZigZag values (possibly shifted) into tile
  _mm256_storeu_si256((__m256i *)(tile+ 8), v1) ;
  _mm256_storeu_si256((__m256i *)(tile+16), v2) ;
  _mm256_storeu_si256((__m256i *)(tile+24), v3) ;
  _mm256_storeu_si256((__m256i *)(tile+32), v4) ;
  _mm256_storeu_si256((__m256i *)(tile+40), v5) ;
  _mm256_storeu_si256((__m256i *)(tile+48), v6) ;
  _mm256_storeu_si256((__m256i *)(tile+56), v7) ;

  p.t.h.min0 = (min != 0) ? 1 : 0 ; // if min == 0, flag it as not used
  p.t.min = min ;                   // store min value in header
  p.t.h.nbts = nbits - 1 ;          // store nbits in header
  p.t.h.encd = 0 ;                  // by default, no encoding
  vz0 = _mm256_xor_si256(v0, v0) ;  // vector of zeros
// we can cut it short here if NO ENCODING is requested, 
// if() { nshort = bshort = nsaved = 0 ; goto end ; }
  vz = _mm256_cmpeq_epi32(v0, vz0) ;                         // count zero values
  vz = _mm256_add_epi32(vz, _mm256_cmpeq_epi32(v1, vz0)) ;
  vz = _mm256_add_epi32(vz, _mm256_cmpeq_epi32(v2, vz0)) ;
  vz = _mm256_add_epi32(vz, _mm256_cmpeq_epi32(v3, vz0)) ;
  vz = _mm256_add_epi32(vz, _mm256_cmpeq_epi32(v4, vz0)) ;
  vz = _mm256_add_epi32(vz, _mm256_cmpeq_epi32(v5, vz0)) ;
  vz = _mm256_add_epi32(vz, _mm256_cmpeq_epi32(v6, vz0)) ;
  vz = _mm256_add_epi32(vz, _mm256_cmpeq_epi32(v7, vz0)) ;
  v128 = _mm_add_epi32(_mm256_extractf128_si256 (vz, 0) ,  _mm256_extractf128_si256 (vz, 1)) ;
  v128 = _mm_add_epi32(v128,  _mm_shuffle_epi32(v128, 0b11101110) ) ; // [0,1,2,3] [2,3,2,3] (could also use 128 bit shift by 8)
  v128 = _mm_add_epi32(v128,  _mm_shuffle_epi32(v128, 0b01010101) ) ; // [0,1,2,3] [1,1,1,1] (could also use 128 bit shift by 4)
  nzero = _mm_cvtsi128_si32(v128) ;  // nzero is always <= 0
  nsaved = nbits * nzero + 64 ;      // bits saved (the more negative, the better)
  nshort = nzero ; bshort = 0 ;      // zero length tokens

  uint32_t ref[4], kount ;
  bsaved = nsaved ;

  nbits0 = (nbits + NB0) >> 1 ;     // number of bits for "short" values
// fprintf(stderr, "DEBUG-1: kount = %3d, bshort = %2d, bsaved = %5d, nsaved = %5d, ref = %8.8x, bits = %d(%d)\n", -nshort, bshort, bsaved, nsaved, 0, nbits0, nbits) ;
  ref[0] = (1 << nbits0) ;          // will look for ref[0] > value  (nbits0)
  ref[1] = ref[0] >> 1 ;            // will look for ref[1] > value  (nbits0 - 1)
  ref[2] = ref[0] << 1 ;            // will look for ref[2] > value  (nbits0 + 1)
  ref[3] = ref[1] >> 1 ;            // will look for ref[2] > value  (nbits0 - 2)

  vm0 = _mm256_set1_epi32(ref[0]) ; // vector of ref[0] (reflects nbits0)
  vm1 = _mm256_set1_epi32(ref[1]);  // vector of ref[1] (reflects nbits0 - 1)
  // count values < mask[01]
  vs =                      _mm256_cmpgt_epi32(vm0, v0)  ; vz =                      _mm256_cmpgt_epi32(vm1, v0)  ;
  vs = _mm256_add_epi32(vs, _mm256_cmpgt_epi32(vm0, v1)) ; vz = _mm256_add_epi32(vz, _mm256_cmpgt_epi32(vm1, v1)) ;
  vs = _mm256_add_epi32(vs, _mm256_cmpgt_epi32(vm0, v2)) ; vz = _mm256_add_epi32(vz, _mm256_cmpgt_epi32(vm1, v2)) ;
  vs = _mm256_add_epi32(vs, _mm256_cmpgt_epi32(vm0, v3)) ; vz = _mm256_add_epi32(vz, _mm256_cmpgt_epi32(vm1, v3)) ;
  vs = _mm256_add_epi32(vs, _mm256_cmpgt_epi32(vm0, v4)) ; vz = _mm256_add_epi32(vz, _mm256_cmpgt_epi32(vm1, v4)) ;
  vs = _mm256_add_epi32(vs, _mm256_cmpgt_epi32(vm0, v5)) ; vz = _mm256_add_epi32(vz, _mm256_cmpgt_epi32(vm1, v5)) ;
  vs = _mm256_add_epi32(vs, _mm256_cmpgt_epi32(vm0, v6)) ; vz = _mm256_add_epi32(vz, _mm256_cmpgt_epi32(vm1, v6)) ;
  vs = _mm256_add_epi32(vs, _mm256_cmpgt_epi32(vm0, v7)) ; vz = _mm256_add_epi32(vz, _mm256_cmpgt_epi32(vm1, v7)) ;

  v128 = _mm_add_epi32(_mm256_extractf128_si256 (vs, 0) ,  _mm256_extractf128_si256 (vs, 1)) ;
  v128 = _mm_add_epi32(v128,  _mm_shuffle_epi32(v128, 0b11101110) ) ; // [0,1,2,3] [2,3,2,3] (could also use 128 bit shift by 8)
  v128 = _mm_add_epi32(v128,  _mm_shuffle_epi32(v128, 0b01010101) ) ; // [0,1,2,3] [1,1,1,1] (could also use 128 bit shift by 4)
  kount = _mm_cvtsi128_si32(v128) ;               // kount is always <= 0
  bsaved = (nbits - nbits0    ) * kount + 69 ;    // bits saved (the more negative, the better)
// fprintf(stderr, "DEBUG-2: kount = %3d, bshort = %2d, bsaved = %5d, nsaved = %5d, ref = %8.8x\n", -kount, nbits0, bsaved, nsaved, ref[0]) ;
  if(bsaved < nsaved){ nshort = kount ; nsaved = bsaved ; bshort = nbits0 ; /*fprintf(stderr, "DEBUG: bshort = %d\n", bshort)*/ ; }

  v128 = _mm_add_epi32(_mm256_extractf128_si256 (vz, 0) ,  _mm256_extractf128_si256 (vz, 1)) ;
  v128 = _mm_add_epi32(v128,  _mm_shuffle_epi32(v128, 0b11101110) ) ; // [0,1,2,3] [2,3,2,3] (could also use 128 bit shift by 8)
  v128 = _mm_add_epi32(v128,  _mm_shuffle_epi32(v128, 0b01010101) ) ; // [0,1,2,3] [1,1,1,1] (could also use 128 bit shift by 4)
  kount = _mm_cvtsi128_si32(v128) ;               // kount is always <= 0
  bsaved = (nbits - nbits0 + 1) * kount + 69 ;    // bits saved (the more negative, the better)
// fprintf(stderr, "DEBUG-3: kount = %3d, bshort = %2d, bsaved = %5d, nsaved = %5d, ref = %8.8x\n", -kount, nbits0-1, bsaved, nsaved, ref[1]) ;
  if(bsaved < nsaved){ nshort = kount ; nsaved = bsaved ; bshort = nbits0-1 ; /*fprintf(stderr, "DEBUG: bshort = %d\n", bshort)*/ ; }

  if(nbits < 8) goto end ;

  vm0 = _mm256_set1_epi32(ref[2]) ; // vector of ref[2] (reflects nbits0 + 1)
  vm1 = _mm256_set1_epi32(ref[3]);  // vector of ref[3] (reflects nbits0 - 2)
  // count values < mask[23]
  vs =                      _mm256_cmpgt_epi32(vm0, v0)  ; vz =                      _mm256_cmpgt_epi32(vm1, v0)  ;
  vs = _mm256_add_epi32(vs, _mm256_cmpgt_epi32(vm0, v1)) ; vz = _mm256_add_epi32(vz, _mm256_cmpgt_epi32(vm1, v1)) ;
  vs = _mm256_add_epi32(vs, _mm256_cmpgt_epi32(vm0, v2)) ; vz = _mm256_add_epi32(vz, _mm256_cmpgt_epi32(vm1, v2)) ;
  vs = _mm256_add_epi32(vs, _mm256_cmpgt_epi32(vm0, v3)) ; vz = _mm256_add_epi32(vz, _mm256_cmpgt_epi32(vm1, v3)) ;
  vs = _mm256_add_epi32(vs, _mm256_cmpgt_epi32(vm0, v4)) ; vz = _mm256_add_epi32(vz, _mm256_cmpgt_epi32(vm1, v4)) ;
  vs = _mm256_add_epi32(vs, _mm256_cmpgt_epi32(vm0, v5)) ; vz = _mm256_add_epi32(vz, _mm256_cmpgt_epi32(vm1, v5)) ;
  vs = _mm256_add_epi32(vs, _mm256_cmpgt_epi32(vm0, v6)) ; vz = _mm256_add_epi32(vz, _mm256_cmpgt_epi32(vm1, v6)) ;
  vs = _mm256_add_epi32(vs, _mm256_cmpgt_epi32(vm0, v7)) ; vz = _mm256_add_epi32(vz, _mm256_cmpgt_epi32(vm1, v7)) ;

  v128 = _mm_add_epi32(_mm256_extractf128_si256 (vs, 0) ,  _mm256_extractf128_si256 (vs, 1)) ;
  v128 = _mm_add_epi32(v128,  _mm_shuffle_epi32(v128, 0b11101110) ) ; // [0,1,2,3] [2,3,2,3] (could also use 128 bit shift by 8)
  v128 = _mm_add_epi32(v128,  _mm_shuffle_epi32(v128, 0b01010101) ) ; // [0,1,2,3] [1,1,1,1] (could also use 128 bit shift by 4)
  kount = _mm_cvtsi128_si32(v128) ;               // kount is always <= 0
  bsaved = (nbits - nbits0 - 1) * kount + 69 ;    // bits saved (the more negative, the better)
// fprintf(stderr, "DEBUG-4: kount = %3d, bshort = %2d, bsaved = %5d, nsaved = %5d, ref = %8.8x\n", -kount, nbits0+1, bsaved, nsaved, ref[2]) ;
  if(bsaved < nsaved){ nshort = kount ; nsaved = bsaved ; bshort = nbits0+1 ; /*fprintf(stderr, "DEBUG: bshort = %d\n", bshort)*/ ; }

  v128 = _mm_add_epi32(_mm256_extractf128_si256 (vz, 0) ,  _mm256_extractf128_si256 (vz, 1)) ;
  v128 = _mm_add_epi32(v128,  _mm_shuffle_epi32(v128, 0b11101110) ) ; // [0,1,2,3] [2,3,2,3] (could also use 128 bit shift by 8)
  v128 = _mm_add_epi32(v128,  _mm_shuffle_epi32(v128, 0b01010101) ) ; // [0,1,2,3] [1,1,1,1] (could also use 128 bit shift by 4)
  kount = _mm_cvtsi128_si32(v128) ;               // kount is always <= 0
  bsaved = (nbits - nbits0 + 2) * kount + 69 ;    // bits saved (the more negative, the better)
// fprintf(stderr, "DEBUG-5: kount = %3d, bshort = %2d, bsaved = %5d, nsaved = %5d, ref = %8.8x\n", -kount, nbits0-2, bsaved, nsaved, ref[3]) ;
  if(bsaved < nsaved){ nshort = kount ; nsaved = bsaved ; bshort = nbits0-2 ; /*fprintf(stderr, "DEBUG: bshort = %d\n", bshort)*/ ; }

end:
  p.t.bshort = bshort ;                       // number of bits for "short" tokens
  p.t.nshort = -nshort ;                      // nshort ia always <= 0
  if(nsaved >= 0){                            // no bit saving through encoding
    p.t.h.encd = 0 ;                          // no encoding
    p.t.bshort = 0 ;
    p.t.nshort = 0 ;
  }else{
    p.t.h.encd = (bshort == 0) ? 2 : 1 ;      // 0 , 1//full or 0//short , 1//full encoding
  }

// fprintf(stderr, "DEBUG: (tile_properties) nshort = %3d, bshort = %2d, bsaved = %5d, encoding = %d\n", p.t.nshort, p.t.bshort, nsaved, p.t.h.encd) ;
  return p.u64 ;
//   return encode_tile_scheme(p.u64) ;

constant:
  p.t.min = min ;
  return constant_tile_scheme(p.u64) ;         // encoding type will be 3

#else

  p.u64 = encode_tile_properties_c(f, ni, lni, nj, tile) ; // NON SIMD (AVX2) version
  return p.u64 ;

#endif

}

// encode contiguous tile (1 -> 64 values) into bit stream s, using properties tp64
// tp64 [IN] : properties from encode_tile_properties
// s   [OUT] : bit stream
// tile [IN] : pre extracted tile (1->64 values)
// return total number of bits inserted into bit stream
// if not enough room in stream, return a negative error reflecting the number of extra bits needed
// TODO: npti, nptj or npij ?
int32_t encode_contiguous(uint64_t tp64, bitstream *s, uint32_t tile[64]){
  EZ_NEW_INSERT_VARS(*s) ;
  uint32_t avail ;
  tile_properties p ;
  int nij, nbits, nbits0, nbitsi, nbtot, i ;
  uint32_t min, w32, mask0, mask1, nbitsm ;
  uint32_t nshort, needed ;

  p.u64 = tp64 ;
// fprintf(stderr, "encode header = %4.4x\n", p.u16[3]);
// print_tile_properties(tp64) ;
  nij   = p.t.h.npij + 1 ;                              // number of points
  nbtot = -1 ;                                          // precondition for error
  if(nij < 1 || nij > 64) goto error ;
  min    = p.t.min ;
  nshort = p.t.nshort ;
  nbits = p.t.h.nbts + 1 ;
  nbits0 = p.t.bshort ;                                 // number of bits for "short" values

  // evaluate how many bits we will need for encoding
  needed = nbits0 ? 21 : 16 ;                           // header + 5 bits for nbits0 if nbits0 is not 0
  if(p.t.h.min0) needed += (5 + 32 - lzcnt_32(min)) ;   // if minimum value is used
  if(p.t.h.encd == 3) nij = 1 ;                         // constant tile, only one value is used
  needed = needed + 
           nbits0 * nshort +                            // "short" tokens
           nbits * (nij - nshort) ;                     // full tokens
  if(p.t.h.encd) needed += nij ;                        // one extra bit per value for short/full encoding if necessary
  avail = STREAM_BITS_EMPTY(*s) ;                       // free bits in stream
//   fprintf(stderr, "worst case %d bits, %d free\n", needed, avail) ;
  nbtot = (avail - needed - 1) ;
  if(avail < needed) goto error ;                       // ERROR, not enough space to encode

  mask0 = RMASK31(nbits0) ;                             // value & mask0 will be 0 if nbits0 or less bits are needed
  mask0 = ~mask0 ;                                      // keep only the upper bits
  mask1 = 1 << nbits ;                                  // full token flag

// fprintf(stderr, "(e_c) nbits = %d(%d), sign = %d, encoding = %d, nshort = %2d, nzero = %2d", nbits, nbits0, p.t.h.sign, p.t.h.encd, nshort, nzero) ;
// fprintf(stderr, ", nbits_full = %4d, nbits_zero = %4d, nbits_short = %4d, mask0 = %8.8x\n", nbits_full, nbits_zero, nbits_short, mask0) ;
  w32 = p.u16[3] ;               // tile header
// fprintf(stderr, "DEBUG: (encode_contiguous) encd = %d, nbits0 = %d, header = %8.8x\n", p.t.h.encd, nbits0, w32) ;
  BE64_EZ_PUT_NBITS(w32, 16) ;                          // insert header into packed stream
  nbtot = 16 ;
  if(p.t.h.encd == 1){
    w32 = nbits0 ;
    BE64_EZ_PUT_NBITS(w32, 5) ;                         // insert size of short tokens for encoding mode 1
    nbtot += 5 ;
  }
  if(p.t.h.encd == 3) goto constant ;                   // special case for constant tile

  // will have to insert both number of bits and value
  if(p.t.h.min0){                                       // store nbitsm -1 into bit stream
    nbitsm = 32 - lzcnt_32(min) ;
    BE64_EZ_PUT_NBITS(nbitsm-1, 5) ;                    // insert number of bits -1 for min value (5 bits)
    BE64_EZ_PUT_NBITS(min, nbitsm) ;                    // insert minimum value (nbitsm bits)
    nbtot += (nbitsm+5) ;                               // update bits inserted count
  }
  // 3 mutually exclusive alternatives (case 3 handled before)
  switch(p.t.h.encd)
  {
  case 0 :                                              // no special encoding
    for(i=0 ; i<nij ; i++){
      w32 = tile[i] - min ;
      BE64_EZ_PUT_NBITS(w32, nbits) ;                   // insert nbits bits into stream
      nbtot += nbits ;                                  // update bits inserted count
    }
    break ;
  case 1 :                                              // 0//short , 1//full encoding
    for(i=0 ; i<nij ; i++){
      w32 = tile[i] - min ;
      if((w32 & mask0) == 0){                           // value uses nbits0 bits or less
        nbitsi = nbits0+1 ;                             // nbits0+1 bits
      }else{
        nbitsi = nbits+1 ;                              // nbits+1 bits
        w32 |= mask1 ;                                  // add 1 bit to identify "full" value
      }
      nbtot += nbitsi ;                                 // update bits inserted count
      BE64_EZ_PUT_NBITS(w32, nbitsi) ;                  // insert nbitsi bits into stream
    }
    break ;
  case 2 :                                              // 0 , 1//full encoding
    for(i=0 ; i<nij ; i++){
      w32 = tile[i] - min ;
      if(w32 == 0){                                     // value is zero
        nbitsi = 1 ;                                    // 1 bit
      }else{
        nbitsi = nbits+1 ;                              // nbits+1 bits
        w32 |= mask1 ;                                  // add 1 bit to identify "full" value
      }
      nbtot += nbitsi ;                                 // update bits inserted count
      BE64_EZ_PUT_NBITS(w32, nbitsi) ;                  // insert nbitsi bits into stream
    }
    break ;
  }

end:
  BE64_EZ_PUSH ;                                        // push accumulator into bit stream
  EZ_SET_INSERT_VARS(*s) ;                              // update bit stream insertion variables from local variables
  return nbtot ;                                        // return total number of bits inserted

error:
  return nbtot ;

constant:
  if(min != 0) {
    BE64_EZ_PUT_NBITS(min, nbits) ;                     // insert nbits bits into stream
    nbtot += nbits ;                                    // update bits inserted count
  }
  goto end ;
}

// gather tile to be encoded, determine useful data properties, encode tile
// f     [IN] : array from which tile will be extracted
// ni    [IN] : first dimension of tile to be encoded (1 <= ni <= 8)
// lni   [IN] : row storage length in array f
// nj    [IN] : second dimension of tile to be encoded (1 <= nj <= 8)
// s    [OUT] : bit stream
// tile [OUT] : extracted tile (in sign/magnitude form and with minimum value possibly subtracted)
// return total number of bits inserted into the bit stream
// f is expected to contain 32 bit values (treated as signed integers)
int32_t encode_a_tile(void *f, int ni, int lni, int nj, bitstream *s, uint32_t tile[64]){
  uint64_t tp64 ;
  int32_t used ;
//   tile_properties tp ;

  tp64 = encode_tile_properties(f, ni, lni, nj, tile) ;   // extract tile, compute data properties
//   print_stream_params(*s, "before tile encode", NULL) ;
  used = encode_contiguous(tp64, s, tile) ;               // encode extracted tile into bit stream
//   tp.u64 = tp64 ;
//   fprintf(stderr, "needed bits = %4d, nbits = %2d, encoding = %d, sign = %d, min0 = %d, value = %8.8x\n\n", 
//           used, tp.t.h.nbts+1, tp.t.h.encd, tp.t.h.sign, tp.t.h.min0, tp.t.min) ;
//   print_stream_params(*s, "after  tile encode", NULL) ;
//   fprintf(stderr, "\n");
  return used ;                                           // will be negative if encode_contiguous failed
}

// f     [IN] : array from which tiles will be extracted
// ni    [IN] : first dimension of array f
// lni   [IN] : row storage length in array f
// nj    [IN] : second dimension of array f
// s    [OUT] : bit stream
// return total number of bits added to bit stream (-1 in case of error)
// f is expected to contain a stream of 32 bit values
int32_t encode_as_tiles(void *f, int ni, int lni, int nj, bitstream *s){
  int i0, j0, indexi, indexj, ni0, nj0, nbtile ;
  uint32_t *fi = (uint32_t *) f ;
  uint32_t tile[64] ;
  int32_t nbtot = 0 ;

  indexj = 0 ;
  for(j0 = 0 ; j0 < nj ; j0 += 8){
    nj0 = ((nj - j0) > 8) ? 8 : (nj - j0) ;
    for(i0 = 0 ; i0 < ni ; i0 += 8){
      indexi = i0 ;
      ni0 = ((ni - i0) > 8) ? 8 : (ni - i0) ;
      // check that we have enough room left in stream to encode data
      nbtile = encode_tile(fi+indexj+indexi, ni0, lni, nj0, s, tile) ;
//       print_stream_params(*s, "after tile encode", NULL) ;
      nbtot += nbtile ;
//       fprintf(stderr, "encoding : nbtile = %d, indexi/indexj = %d/%d, LLcorner = %8.8x\n\n", nbtile, indexi, indexj/lni, fi[indexi+indexj]) ;
// fprintf(stderr,"tile (%3d,%3d) -> (%3d,%3d) [%d x %d][%4d,%4d], nbtile = %d (%4.1f/value)\n\n", 
//         i0, j0, i0+ni0-1, j0+nj0-1, ni0, nj0, indexi, indexj, nbtile, nbtile*1.0/(ni0*nj0)) ;
    }
    indexj += lni*8 ;
  }
  return nbtot ;
}

// decode a tile encoded with encode_tile
// f    [OUT] : array into which the tile will be restored
// ni    [IN] : first dimension of array f
// lni   [IN] : row storage length in array f
// nj    [IN] : second dimension of array f
// s  [INOUT] : bit stream
// return the number of bits extracted from the bit stream
// TODO: npti, nptj or npij ?
// int32_t decode_tile(void *f, int *ni, int lni, int *nj, bitstream *s){
int32_t decode_a_tile(void *f, int ni, int lni, int nj, int *nptsij, bitstream *s){
  int32_t *fi = (int32_t *) f ;
//   uint32_t *fu = (uint32_t *) f ;
  uint32_t fe[64] ;
  tile_header th ;
  uint32_t w32 ;
  int32_t iw32 ;
  int i, i0, j, nbits, nij, nbits0, nbtot, nbitsi, nbitsm ;
  uint32_t min ;
  int error_code = 0 ;
  EZ_DCL_XTRACT_VARS ;

  error_code = 1 ;
  if((f == NULL) || (s == NULL)) goto error ;

// fprintf(stderr, "DEBUG: (decode_tile) test valid\n");
// print_stream_params(*s, "(decode_tile)", NULL) ;
  error_code = 2 ;
  if( ! StreamIsValid(s) ) goto error ;
  EZ_GET_XTRACT_VARS(*s) ;            // get extraction control variables from bit stream
  BE64_EZ_GET_NBITS(w32, 16)          // get tile header
// fprintf(stderr, "DEBUG: (decode_tile) header = %8.8x, nbits = %2d, ni = %d, nj = %d, nij = %d\n", w32, nbits, ni, nj, nij) ;
  nbtot = 16 ;
  th.s = w32 ;                        // tile header
  nij = th.h.npij + 1 ;               //  number of values in tile
  *nptsij = nij ;                     // send number of values found to caller
  error_code = 3 ;
  if(ni * nj != nij) goto error ;     // dimension mismatch, this tile should contain ni * nj values
  nbits = th.h.nbts + 1 ;
  nbits0 = 0 ;                        // number of bits for "short" values
// fprintf(stderr, "DEBUG: (decode_tile) header = %8.8x, nbits = %2d, ni = %d, nj = %d, nij = %d\n", w32, nbits, ni, nj, nij) ;
  // GET the 5 bits giving the length of the short tokens if 0//short 1//nbits encoding is used
  if(th.h.encd == 1) {
    BE64_EZ_GET_NBITS(nbits0, 5)
// fprintf(stderr, "DEBUG: (decode_tile) short token length = %2d\n", nbits0) ;
    nbtot += 5 ;                      // update number of bits extracted
  }
  if(th.h.encd == 3) goto constant ;  // constant tile

  min = 0 ;                           // default value (min not used)
  // will have to get both number of bits and value (get nbits -1 from bit stream)
  if(th.h.min0){                      // get min value if there is one
    BE64_EZ_GET_NBITS(nbitsm, 5)      // number of bits for min value
    nbitsm++ ;                        // restore nbitsm
    BE64_EZ_GET_NBITS(min, nbitsm)    // min value
    nbtot += (nbitsm+5) ;             // update number of bits extracted
  }else{
    nbitsm = 0 ;                      // min value is not used
  }

  switch(th.h.encd)
  {
  case 1 :                               // 0//short , 1//full encoding
    for(i=0 ; i<nij ; i++){
      BE64_EZ_GET_NBITS(w32, 1)          // get 1 bit
      if(w32 != 0){                      // full token
        BE64_EZ_GET_NBITS(w32, nbits) ;  // get nbits bits
        nbitsi = nbits + 1 ;
      }else{                             // short token
        BE64_EZ_GET_NBITS(w32, nbits0) ; // get nbits0 bits
        nbitsi = nbits0 + 1 ;
      }
      nbtot += nbitsi ;                  // update number of bits extracted
      fe[i] = w32 + min ;                // add min to decoded value
    }
    break ;
  case 2 :                               // 0 , 1//full encoding
    for(i=0 ; i<nij ; i++){
      BE64_EZ_GET_NBITS(w32, 1) ;        // get 1 bit
      if(w32 != 0){                      // full token
        BE64_EZ_GET_NBITS(w32, nbits) ;  // get nbits bits
        nbitsi = nbits + 1 ;
      }else{                             // short token
        nbitsi = 1 ;
      }
      nbtot += nbitsi ;                  // update number of bits extracted
      fe[i] = w32 + min ;                // add min to decoded value
    }
    break ;
  case 0 :                               // no short/full encoding
    for(i=0 ; i<nij ; i++){
      BE64_EZ_GET_NBITS(w32, nbits) ;    // get nbits bits
      fe[i] = w32 + min ;                // add min to decoded value
      nbtot += nbits ;                   // update number of bits extracted
    }
    break ;
  }

  i0 = 0 ;
  if(th.h.sign == 1){                    // all values >= 0
    for(j=0 ; j<nj ; j++){
      for(i=0 ; i<ni ; i++){
        fi[i] = fe[i0+i] ;               // ZigZag O.K. if non negative
      }
      fi += lni ;
      i0 += ni ;
    }
  }
  if(th.h.sign == 2){                    // all values < 0
    for(j=0 ; j<nj ; j++){
      for(i=0 ; i<ni ; i++){
        fi[i] = ~(fe[i0+i]) ;            // undo right shifted ZigZag
      }
      fi += lni ;
      i0 += ni ;
    }
  }
  if(th.h.sign == 3){                    // full ZigZag
    for(j=0 ; j<nj ; j++){
      for(i=0 ; i<ni ; i++){             // restore from ZigZag
        fi[i] = from_zigzag_32((fe[i0+i])) ;
      }
      fi += lni ;
      i0 += ni ;
    }
  }
// fprintf(stderr, "DEBUG: (decode_tile) nbtot = %2d\n", nbtot) ;
// fprintf(stderr, " TILE     : bits = %4d, ni = %2d, nj = %2d, nbits = %2d, encoding = %d, sign = %d, min0 = %d, min   = %8.8x, nbitsm = %2d\n", 
//         nbtot, ni, nj, nbits, th.h.encd, th.h.sign, th.h.min0, min, nbitsm) ;

end:
  EZ_SET_XTRACT_VARS(*s) ;               // update bit stream extraction info from local variables
  return nbtot ;                         // return total number of bits extracted from stream

error:
fprintf(stderr, "ERROR: (decode_tile) header = %8.8x, nbits = %2d, ni = %d, nj = %d, nij = %d\n", w32, nbits, ni, nj, nij) ;
fprintf(stderr, "ERROR: (decode_tile), code = %d, nij = %d, ni = %d, nj = %d\n", error_code, nij, ni, nj);
  return -1 ;

constant:
  iw32 = 0 ;
  if(th.h.sign == 3){
    BE64_EZ_GET_NBITS(w32, nbits) ;      // extract nbits bits
    nbtot += nbits ;                     // update number of bits extracted
    iw32 = from_zigzag_32(w32) ;         // restore from ZigZag
  }
// fprintf(stderr, " CONSTANT : bits = %4d, ni = %2d, nj = %2d, nbits = %2d, encoding = %d, sign = %d, min0 = %d, value = %8.8x, nij = %2d\n", 
//         nbtot, ni, nj, nbits, th.h.encd, th.h.sign, th.h.min0, iw32, nij) ;
  for(j=0 ; j<nj ; j++){
    for(i=0 ; i<ni ; i++){
      fi[i] = iw32 ;                     // store constant value into destination array
    }
    fi += lni ;
    i0 += ni ;
  }
  goto end ;
}

// decode an array of 32 bit values encoded with encode_as_tiles
// f    [OUT] : array into which the tile will be restored
// ni    [IN] : first dimension of array f
// lni   [IN] : row storage length of array f
// nj    [IN] : second dimension of array f
// s  [INOUT] : bit stream
// return the total number of bits extracted from the bit stream (-1 in case of error)
int32_t decode_as_tiles(void *f, int ni, int lni, int nj, bitstream *s){
  int32_t *fi = (int32_t *) f ;
  int ni0, nj0, i0, j0, indexi, indexj, nbtile, nbtot, nptsij ;

  nbtot  = 0 ;
  indexj = 0 ;
  for(j0 = 0 ; j0 < nj ; j0 += 8){
    nj0 = ((nj - j0) > 8) ? 8 : (nj - j0) ;
    for(i0 = 0 ; i0 < ni ; i0 += 8){
      indexi = i0 ;
      ni0 = ((ni - i0) > 8) ? 8 : (ni - i0) ;
// fprintf(stderr,"decoding tile at (%d,%d)\n", i0, j0) ;
      nbtile = decode_tile(fi+indexi+indexj, ni0, lni, nj0, &nptsij, s) ;
      if(nptsij != ni0 * nj0) {
        fprintf(stderr,"decode_as_tiles : i0 = %d, j0 = %d, nbtile = %d, nij = %d, expecting %d\n", i0, j0, nbtile, nptsij, ni0 * nj0) ;
        return -1 ;        // decoding error
      }

      nbtot += nbtile ;    // add to total bits extracted from stream
    }
    indexj += lni*8 ;
  }
  return nbtot ;           // return total number of bits extracted from stream
}

#endif

#include <libaec.h>

static uint32_t block_size = 16;
static uint32_t rsi = 128 ;

int32_t AecDecodeUnsigned(void *source, int32_t source_length, void *dest, int32_t dest_length, int bits_per_sample){
  struct aec_stream strm;

  strm.bits_per_sample = bits_per_sample;
  strm.block_size = block_size;
  strm.rsi = rsi;
//   strm.flags = AEC_DATA_SIGNED ;
  strm.flags = 0;
  strm.next_in = source;
  strm.avail_in = source_length;
  strm.next_out = (unsigned char *)dest;
  strm.avail_out = dest_length * sizeof(int32_t);

  if (aec_decode_init(&strm) != AEC_OK)
    return -1;

  if (aec_decode(&strm, AEC_FLUSH) != AEC_OK)
    return -1;

  int32_t total_out = strm.total_out ;
fprintf(stderr, "AecDecodeUnsigned : decoded %d bytes = %d bits\n", total_out, total_out*8) ;
  aec_decode_end(&strm);

  return total_out ;
}

int32_t AecEncodeUnsigned(void *source, int32_t source_length, void *dest, int32_t dest_length, int bits_per_sample){
  struct aec_stream strm;
fprintf(stderr, "AecEncodeUnsigned : encoding %d samples 0f %d bits = %d bits\n", source_length, bits_per_sample, source_length*bits_per_sample);
  /* input data is bits_per_sample bits wide */
  strm.bits_per_sample = bits_per_sample;
  /* define a block size of 16 */
  strm.block_size = block_size;
  /* the reference sample interval is set to 128 blocks */
  strm.rsi = rsi;
  /* input data is signed and needs to be preprocessed */
//   strm.flags = AEC_DATA_SIGNED | AEC_DATA_PREPROCESS;
  /* input data is unsigned and does not need to be preprocessed */
  strm.flags = 0 ;
  /* pointer to input */
  strm.next_in = (unsigned char *)source;
  /* length of input in bytes */
//   strm.avail_in = source_length * sizeof(int32_t);
  strm.avail_in = source_length * (bits_per_sample/8);
  /* pointer to output buffer */
  strm.next_out = dest;
  /* length of output buffer in bytes */
  strm.avail_out = dest_length;

  /* initialize encoding */
  if (aec_encode_init(&strm) != AEC_OK){
fprintf(stderr, "aec_encode_init(&strm) != AEC_OK\n");
      return -1;
  }

  /* Perform encoding in one call and flush output. */
  /* In this example you must be sure that the output */
  /* buffer is large enough for all compressed output */
  if (aec_encode(&strm, AEC_FLUSH) != AEC_OK){
fprintf(stderr, "aec_encode(&strm, AEC_FLUSH) != AEC_OK\n") ;
      return -1;
  }

  int32_t total_out = strm.total_out ;
  /* free all resources used by encoder */
  aec_encode_end(&strm);

  return total_out ;
}
