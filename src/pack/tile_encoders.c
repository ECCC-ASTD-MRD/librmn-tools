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
static int saved_bits = 0 ;
static int short_long[4] = {0,0,0,0} ;

void print_encode_stats(int reset){
  fprintf(stderr, "encoding stats : %d blocks (%d constant, %d >=0, %d <=0, %d +/-) (%d with offset), ",
                                    nblocks, constant_block, all_plus, all_minus, plus_minus, with_offset) ;
  fprintf(stderr, "   ee encoding : %d %d %d %d, saved bits = %d\n", short_long[0], short_long[1], short_long[2], short_long[3], saved_bits) ;
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
    saved_bits = 0 ;
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
  int i, nbits, totbits, offset, M, E, head3, isminus, iszigzag, lhead, status ;
  uint32_t token, ee, nboffset, uvalue ;
  bitstream s ;

  if(s_in != NULL) s = *s_in ;
  if(s.endian != PACK_ENDIAN) return -1 ;     // stream has the wrong endianness

  ssize_t available_data = StreamStrictAvailableBits(s_in) ;

  STREAM_XTRACT_CHECK(s) ;
  lhead = 8 ;
  STREAM_GET_NBITS(s, token, lhead) ;        // header (8 bits)
  head3 = (token >> 5) ;           // top 3 bits of token
  isminus = iszigzag = 0 ;
// fprintf(stderr, "accum = %16.16lx, header = %8.8x, head3 = %8.8x\n", s.acc_x, token, head3) ;
  switch(head3){
    case 0b000 :                   // constant tile  000bbbbb
      nbits = token & 0x1F ;
      goto constant_tile ;
//       break ;
    case 0b001 :                   // reserved header
      status = -1 ;
      goto error ;
//       break ;
    case 0b010 :                   // all values >= 0
    case 0b011 :
// fprintf(stderr, "all values >=0\n") ;
      break ;
    case 0b100 :                   // all values <= 0, |value| was stored
    case 0b101 :
// fprintf(stderr, "all values <=0\n") ;
      isminus = 1 ;
      break ;
    case 0b110 :                   // mixed signs, zigzag encoding
// fprintf(stderr, "all values zigzag\n") ;
// fprintf(stderr, "stream = %8.8x %8.8x %8.8x %8.8x %8.8x %8.8x\n", (s.out)[0], (s.out)[1], (s.out)[2], (s.out)[3], (s.out)[4], (s.out)[5]) ;
      iszigzag = 1 ;
      break ;
    case 0b111 :                   // long header block, nbits > 16
//       if((token >> 4) == 0b1111) goto error ;   // reserved header
      lhead = 13 ;
  }
  M = E = 0 ;
  if(lhead == 8){                  // SSMEbbbb
// fprintf(stderr, "decode_tile short header :");
    nbits = token & 0xF ;          // nbits -1
    M = 1 & (token >> 5) ;
    E = 1 & (token >> 4) ;
  }else{                           // 1111SSME nnnnn
    int SS = (token >> 2) & 0x3 ;
    isminus  = (SS == 0b10) ;      // all values <= 0
    iszigzag = (SS == 0b11) ;      // mixed signs
    M = 1 & (token >> 1) ;
    E = 1 & (token) ;
//     status = -2 ;
//     if(M == 1 && E == 1) goto error ;
    STREAM_GET_NBITS(s,nbits,5) ;  // nbits -1
// fprintf(stderr, "decode_tile long header :");
  }
  totbits = lhead ;
  nbits++ ;   // header contains nbits - 1
// fprintf(stderr, " M = %d, E = %d, isminus = %d, iszigzag = %d, nbits = %d, nval = %d\n", M, E, isminus, iszigzag, nbits, nval) ;
  if(E != 0){
// fprintf(stderr, "get ee\n") ;
    STREAM_GET_NBITS(s, ee, 2) ;
    totbits += 2 ;
  }
  offset = 0 ;
  if(M != 0){
    STREAM_GET_NBITS(s, nboffset, 5) ;       // get number of bits used for offset
    nboffset++ ;
    STREAM_GET_NBITS(s, token, nboffset) ;   // get offset value
//     offset = from_zigzag_32(token) ;         // translate from zigzag to signed integer
    offset = token ;
// fprintf(stderr, "get token = %d, offset = %d, nboffset = %d\n", token, offset, nboffset) ;
    totbits = totbits + 5 + nboffset ;
  }
  if(E == 0){                    // NO short/long encoding
// fprintf(stderr, "E == 0, nval = %d, nbits = %d, offset = %d, ", nval, nbits, offset) ;
    for(i=0 ; i<nval ; i++){
      STREAM_GET_NBITS(s, token, nbits) ;
      tile[i] = token ;
// fprintf(stderr, " %d", token);
    }
    totbits = totbits + nval * nbits ;
// fprintf(stderr, "\n");
  }else{                         // USED short/long encoding
    int nshort = (stab[nbits] >> (ee * 8)) & 0xF ;
// fprintf(stderr, "E == 1, ee = %d, nshort = %d\n", ee, nshort) ;
    for(i=0 ; i<nval ; i++){
      uint32_t flag ; STREAM_GET_1(s, flag) ; token = 0 ;
      if(flag){                  // long token
        STREAM_GET_NBITS(s, token, nbits) ;
        totbits += nbits ;
      }else{                     // short token
        if(nshort > 0){ STREAM_GET_NBITS(s, token, nshort) ; totbits += nshort ; }
      }
      tile[i] = token ;
// fprintf(stderr, "%d ", token) ;
    }
    totbits += nval ;   // account for flag bit
// fprintf(stderr, "\n");
  }

  if(M != 0){    // add offset if needed
    for(i=0 ; i<nval ; i++) tile[i] = tile[i] + offset ;
  }
  if(isminus){   // invert sign (mutually exclusive with iszigzag)
    for(i=0 ; i<nval ; i++) tile[i] = -tile[i] ;
  }
  if(iszigzag){  // restore signed form (mutually exclusive with isminus)
    for(i=0 ; i<nval ; i++) tile[i] = from_zigzag_32((uint32_t) tile[i]) ;
  }

end:
// fprintf(stderr, "available bits = %ld, used %d\n", available_data, totbits) ;
//   if(available_data < totbits){    // bogus bits were used during decoding
//     status = -3 ;
//     goto error ;
//   }
  *s_in = s ;
  return totbits ;

error :
  return status ;

constant_tile:
  nbits++ ;
  STREAM_GET_NBITS(s, uvalue, nbits) ; // get zigzag encoded value
  int32_t value = from_zigzag_32(uvalue) ;
  for(i=0 ; i<nval ; i++){             // restore tile values
    tile[i] = value ;
  }
  totbits = 8 ;
  totbits += nbits ;
// fprintf(stderr, "constant tile, nbits = %d, nval= %d, value = %d\n", nbits, nval, value);
  goto end ;
}

// encode nval values from tile[nval] into a bit stream
// tile_in [IN] : values to be encoded
// nval    [IN] : number of values
// bp      [IN] : tile properties (see move_blocks.h)
// s_in [INOUT] : pointer to bitstream descriptor (see bitstream.h)
// return nuber of bits inserted into bitstream buffer
// tile may be modified by this function
int encode_tile(bitstream *s_in, int32_t *tile_in, int32_t nval, block_properties *bp){
  int i, nbits, nbits0, offset, nboffset, totbits, SS, M, E, ee, SSME, token, ntoken, range, maxabs, minabs, status ;
  bitstream s ;
  uint32_t shift ;
  int32_t tile[nval] ;
  block_properties bp_ ;

  // check for bad arguments
  if(s_in == NULL || tile_in == NULL || nval <= 0) return -1 ;

  if(s_in != NULL) s = *s_in ;
  if(s.endian != PACK_ENDIAN) return -1 ;     // stream has the wrong endianness

  ssize_t available_space = StreamAvailableSpace(s_in) ;
  if(available_space < 64)         // not enough room for header + basic encoding information
    return -1 ;

// bp = NULL ;
  if(bp == NULL){            // bp not available, get tile extrema
    bp = &bp_;
    status = analyze_data32_block(tile_in, nval, nval, 1, bp);
    bp_.kind = int_data ;    // no need to call adjust_block_properties for integer data
//     print_int_props(bp_) ;
    if(status != nval){
      fprintf(stderr, "analyze_data32_block status = %d, expected %d\n", status, nval) ;
      return -1 ;
    }
  }

  nblocks++ ;
  SS = M = E = ee = 0 ;
  totbits = 0 ;
  offset = 0 ;
  nboffset = 0 ;
  // bp->maxs, bp->mins : signed max and min values in tile
  // bp->maxu, bp->minu : least positive and least negative values (if mixed signs)
  // =============================== constant tile ===============================
  if(bp->maxs.i == bp->mins.i){              // constant tile
    uint32_t zigzag ;
    zigzag = to_zigzag_32(tile_in[0]) ;      // encode constant value as sign/magnitude
    // number of bits needed to represent encoded value
    nbits = (zigzag == 0) ? 1 : BitsNeeded_u32(zigzag) ;
    token = (0b000 << 5) ;                   // header will be ( 000bbbbb )
    token = token | (nbits-1) ;              // put nbits into header
    STREAM_PUT_NBITS(s, token, 8) ;          // 8 bit header
    STREAM_PUT_NBITS(s, zigzag, nbits) ;     // constant value encoded as zigzag (nbits bits)
    totbits = 8 + nbits ;
    constant_block++ ;
// fprintf(stderr, "nbits = %d, constant value = %d (%d)\n", nbits, tile_in[0], zigzag) ;
    goto end ;                               // done
  }
  // =============================== convert to positive values ===============================
  // determine number of bits needed to encode the largest value
  if(bp->maxs.i > 0 && bp->mins.i < 0){      // both positive and negative values are present
    int32_t minz, maxz ;
    plus_minus++ ;
    SS = 3 ;                                 // both signs are present
    M  = 0 ;                                 // offset will not be used
    for(i=0 ; i<nval ; i++)                  // convert to sign/magnitude (zigzag)
      tile[i] = to_zigzag_32(tile_in[i]) ;   // use local storage for tile values
    minz = to_zigzag_32(bp->mins.i) ;        // negative number with largest absolute value
    maxz = to_zigzag_32(bp->maxs.i) ;        // positive number with largest absolute value
    maxz = (minz > maxz) ? minz : maxz ;
    nbits = BitsNeeded_u32(maxz) ;           // number of bits needed for largest zigzag value

  }else{                                     // all >= 0 or all <= 0
    if(bp->maxs.i <= 0){                     // all values <= 0
      all_minus++ ;
      SS = 2 ;                               // all values negative flag
      maxabs = -bp->mins.i ;                 // most negative number
      minabs = -bp->maxs.i ;                 // least negative number
      for(i=0 ; i<nval ; i++)                // use absolute value if all <= 0
        tile[i] = -tile_in[i] ;              // use local array for tile values

    }else if(bp->mins.i >= 0){               // all values >= 0
      all_plus++ ;
      SS = 1 ;                               // all values positive flag
      maxabs = bp->maxs.i ;                  // largest number
      minabs = bp->mins.i ;                  // smallest number
      for(i=0 ; i<nval ; i++)                // straight copy if all >= 0
        tile[i] = tile_in[i] ;               // copy tile values to local array
    }
    M = 0 ;                                  // a priori, NO OFFSET
    nbits  = BitsNeeded_u32(maxabs) ;        // bits needed for largest absolute value
    range  = maxabs - minabs ;
    nbits0 = BitsNeeded_u32(range) ;         // bits needed for range
    if(nbits0 < nbits){                      // less bits needed if subtracting offset
      with_offset++ ;
      offset = minabs ;                      // smallest absolute value
      nboffset = BitsNeeded_u32(offset) ;    // number of bits needed to store offset
      nbits = nbits0 ;                       // reduced number of bits
      M = 1 ;                                // OFFSET ON
// fprintf(stderr, ">>offset :");
      for(i=0 ; i<nval ; i++){
        tile[i] = tile[i] - offset ;         // subtract offset from tile values
// fprintf(stderr, " %d", tile[i]) ;
      }
    }
// fprintf(stderr, "\n");
// fprintf(stderr, "M = %d, offset = %d, nboffset = %d\n", M, offset, nboffset) ;
  }
  // =============================== determine encoding format ===============================
  // determine if it is worth using short/long encoding
  int ref[4], nref[4], count[4] ;
  shift = stab[nbits] ;
  for(i=0 ; i<4 ; i++){         // 4 canditate number of bits, nref[0] = 0, ref[0] = 1
    nref[i] = shift & 0xFF ;    // candidate number of bits for short values
    ref[i] = 1 << nref[i] ;     // value < ref[i] will be a good vcandidate
    shift >>= 8 ;
  }
  count_lt(count, (int *)tile, ref, nval) ;   // compare tile values to reference values for this value of nbits
  int nshort, nbitsmax, shortref  ;
  E = 0 ;
  ee = 0 ;
  nbitsmax = nbits * nval ;         // worst case, nbits used for each value
  if(nbits > 2){   // pointless if nbits < 3
    for(i=0 ; i<4 ; i++){
      int nbitsi ;
      nbitsi  = count[i] * (nref[i]+1) ;       // count[i] "short" values, needing nref[i]+1 bits
      nbitsi += (nval-count[i]) * (nbits+1) ;  // nval - count[i] "long" values, needing nbits+1 bits
      if(nbitsmax > nbitsi){        // we have a better candidate
        nbitsmax = nbitsi ;         // new worts case
        nshort   = nref[i] ;        // length of "short" values
        E = 1 ;                     // short/long encoding will be used
        ee = i ;                    // identify option used
        short_long[ee]++ ;
      }
    }
  }
// fprintf(stderr, "nbits = %d, ee = %d, ref = %8.8x, %8.8x, %8.8x, %8.8x, count = %d, %d, %d, %d\n",
//                  nbits, ee, ref[0], ref[1], ref[2], ref[3], count[0], count[1], count[2], count[3]) ;
  // at this point nbitsmax is equal to the number or bits needed for encoding thevalues
  // =============================== store header ===============================
  SSME = (SS << 2) | (M << 1) | E ;
  if(nbits > 16){                              // more than 16 bits, use 1111 prefix (long header)
    token = (0b1111 << 4) | SSME ;             // 1111SSME
    ntoken = 8 ;
    CONCAT_TOKENS(token,ntoken,(nbits-1),5) ;  // 1111SSMEbbbbb (8 bits + 5 bits))
  }else{                                       // 16 bits or less, use short header
    token = (SSME << 4) | (nbits-1)  ;         // SSMEbbbb (8 bits)
    ntoken = 8 ;
  }
  if(E != 0){                                  // add ee (2 bits)
    CONCAT_TOKENS(token,ntoken,ee,2) ;         // SSM1bbbbee or 1111SSM1bbbbbee
  }
  if(M != 0){                                  // add number of bits needed for offset (5 bits)
    CONCAT_TOKENS(token,ntoken,(nboffset-1), 5) ;
  }
  STREAM_PUT_NBITS(s, token, ntoken) ;         // SSM1bbbb[ee][nnnnn] or 1111SSM1bbbbb[ee][nnnnn]
  totbits += ntoken ;                          // 8 bits up to 20 bits
  if(M != 0){
    STREAM_PUT_NBITS(s, offset, nboffset) ;    // offset
    totbits += nboffset ;                      // nboffset bits for offset
  }
  // =============================== store encoded values ===============================

  if(StreamAvailableSpace(&s) < nbitsmax)    // not enough room for encoded data
    return -1 ;

  if(E == 0){                        // no short/long encoding, all tokens will be nbits long
// uint32_t *in = s.in ;
// fprintf(stderr, "E == 0, nbits = %d, nval = %d :", nbits, nval) ;
    for(i=0 ; i<nval ; i++){
      STREAM_PUT_NBITS(s, tile[i], nbits) ;
//       fprintf(stderr, " %d", tile[i]) ;
    }
// fprintf(stderr, "\n") ;
// fprintf(stderr, "stream :");
// fprintf(stderr, " %8.8x %8.8x %8.8x %8.8x %8.8x %8.8x", in[0], in[1], in[2], in[3], in[4], in[5]) ;
// fprintf(stderr, "\n") ;
  }else{                             // use short/long encoding, tokens will be nshort+1 or nbits+1 bits long
    int checkbits = 0 ;                         // temporary diagnostic variable
    shortref = (1 << nshort) ;                  // anything < shortref can be coded as a "short" token
// fprintf(stderr, "E == 1, nbits = %d, nval = %d, ee = %d, shortref = %d :", nbits, nval, ee, shortref) ;
    for(i=0 ; i<nval ; i++){                    // encode nval values
      token = tile[i] ;                         // value to encode
// fprintf(stderr, " %d", token) ;
      STREAM_INSERT_CHECK(s) ;                  // make sure there is room for up to 32 bits
      if(token < shortref){                     // use "short" token
        STREAM_INSERT_0(s) ;  ;                 // "short" token marker
        if(nshort > 0){
          STREAM_PUT_NBITS(s, token, nshort) ;  // follow with nshort bits
        }
        checkbits += (nshort + 1) ;
      }else{                                    // use "long" token
        STREAM_INSERT_1(s) ;                    // "long" token marker
        STREAM_PUT_NBITS(s, token, nbits) ;     // follow with nbits bits
        checkbits += (nbits + 1) ;
      }
    }
// fprintf(stderr, "\n");
    saved_bits += (nval*nbits - nbitsmax - 2) ;
    if(checkbits != nbitsmax) fprintf(stderr,"checkbits = %d, expecting %d\n", checkbits, nbitsmax) ;
  }
  totbits += nbitsmax ;

end:
// fprintf(stderr, "available space = %ld, used %d\n", available_space, totbits) ;
  STREAM_INSERT_PUSH(s) ;
  *s_in = s ;
  return totbits ;
}

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
