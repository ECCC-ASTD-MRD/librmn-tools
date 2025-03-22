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

void print_tile(int32_t *tile_in, int nval, char *msg){
  int i ;
  fprintf(stderr,"%s", msg) ;
  for(i=0 ; i<nval ; i++) fprintf(stderr," %d", tile_in[i]) ;
  fprintf(stderr,"\n") ;
}

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

// decode nval values from tile[nval] into a bit stream
// tile    OUT] : values to be encoded
// nval    [IN] : number of values
// bp      [IN] : tile properties (see move_blocks.h)
// s_in [INOUT] : pointer to bitstream descriptor (see bitstream.h)
// return nuber of bits extracted from bitstream buffer
// TODO add safety check to make sure we had enough data in stream
int decode_tile(bitstream *s_in, int32_t *tile, int32_t nval){
  int i, nbits, totbits, offset, SS, M, E, allminus, iszigzag, lhead, status = 0 ;
  uint32_t token, ee, nboffset, uvalue ;
  bitstream s ;

  if(s_in != NULL) s = *s_in ;
  if(s.endian != PACK_ENDIAN) return -1 ;     // stream has the wrong endianness

  ssize_t available_data = StreamStrictAvailableBits(s_in) ;

  STREAM_XTRACT_CHECK(s) ;
  lhead = 8 ;
  STREAM_GET_NBITS(s, token, lhead) ;  // primary header (8 bits)
  allminus = iszigzag = 0 ;
  SS = (token >> 6) & 3 ;
  M  = (token >> 5) & 1 ;
  E  = (token >> 4) & 1 ;
  nbits = 1 + (token & 0xF) ;

  switch(SS){
    case 0b00 :
      if(M == 0){                      // constant tile  000bbbbb, SS == 0, M == 0
        nbits = 1 + (token & 0x1F) ;
        goto constant_tile ;
      }
      if(E == 0) goto error ;          // reserved header 0010 (M == 1, E == 0)
      STREAM_GET_NBITS(s,nbits,4) ;    // long header          (M == 1, E == 1)
      nbits += 17 ;
      SS = (token >> 2) & 3 ;          // decode SS, M, E
      M  = (token >> 1) & 1 ;
      E  = (token     ) & 1 ;
      allminus  = (SS == 0b10) ;       // all values <= 0
      iszigzag  = (SS == 0b11) ;       // mixed signs
      lhead = 12 ;                     // 12 bit header ( 8 + 4 )
      break ;
    case 0b01 :                        // all values >= 0, SS == 0b01
      break ;
    case 0b10 :                        // all values <= 0, |value| was stored, SS == 0b10
      allminus = 1 ;
      break ;
    case 0b11 :                        // mixed signs, zigzag encoding, SS == 3
      iszigzag = 1 ;                   // unless M was used : iszigzag = (M == 0) ? 1 : 0 ;
      break ;
  }
// fprintf(stderr, "decode_tile : SS = %d, M = %d, E= %d, nbits = %d\n", SS, M, E, nbits) ;
  totbits = lhead ;
  if(E != 0){
    STREAM_GET_NBITS(s, ee, 2) ;
    totbits += 2 ;
  }
  offset = 0 ;
  if(M != 0){
    STREAM_GET_NBITS(s, nboffset, 5) ;       // get number of bits used for offset
    nboffset++ ;
    STREAM_GET_NBITS(s, token, nboffset) ;   // get offset value
    offset = token ;                         // offset is always positive (applied before sign reversal)
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
  }else{                         // USING short/long encoding
    int nref[4] ;
    nref[0] = nbits/8 ; nref[1] = nbits/2-1 ; nref[2] = nref[1]+1 ; nref[3] = nref[1]+2 ;
    int nshort = nref[ee] ;
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
    totbits += nval ;   // account for flag bits
// fprintf(stderr, "\n");
  }

  if(M != 0){    // add offset if needed (must be done before eventual sign reversal)
    for(i=0 ; i<nval ; i++) tile[i] = tile[i] + offset ;
  }
  if(allminus){   // invert sign (mutually exclusive with iszigzag)
    for(i=0 ; i<nval ; i++) tile[i] = -tile[i] ;
  }
  if(iszigzag){  // restore signed form (mutually exclusive with allminus)
    for(i=0 ; i<nval ; i++) tile[i] = from_zigzag_32((uint32_t) tile[i]) ;
  }

end:
// print_tile(tile, nval, "decode_tile :") ;
// fprintf(stderr, "available bits = %ld, used %d\n", available_data, totbits) ;
  if(available_data < totbits){    // bogus bits were used during decoding
    status = -3 ;
    goto error ;
  }
  *s_in = s ;
  return totbits ;

error :
  return status ;

constant_tile:
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
int encode_tile(bitstream *s_in, int32_t *tile_in, int32_t nval, block_properties *bp){
  int i, nbits, nbits0, offset, nboffset, totbits, SS, M, E, ee, SSME, token, ntoken, range, maxabs, minabs, status ;
  bitstream s ;
//   uint32_t shift ;
  int32_t tile[nval] ;
  block_properties bp_ ;
// print_tile(tile_in, nval, "encode_tile :") ;
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
// print_int_props(bp_) ;
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
    // number of bits needed to represent encoded value (at least 1)
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
  M = E = 0 ;
  // determine number of bits needed to encode the largest value
  if(bp->maxs.i > 0 && bp->mins.i < 0){      // both positive and negative values are present
    int32_t minz, maxz ;
    plus_minus++ ;
    SS = 3 ;                                 // both signs are present
    M  = 0 ;                                 // offset will not be used (might get used in later implementation)
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

// fprintf(stderr, "all minus\n") ;
    }else if(bp->mins.i >= 0){               // all values >= 0
      all_plus++ ;
      SS = 1 ;                               // all values positive flag
      maxabs = bp->maxs.i ;                  // largest number
      minabs = bp->mins.i ;                  // smallest number
      for(i=0 ; i<nval ; i++)                // straight copy if all >= 0
        tile[i] = tile_in[i] ;               // copy tile values to local array
// fprintf(stderr, "all plus : maxabs = %d, minabs = %d\n", maxabs, minabs) ;
    }
    M = 0 ;                                  // a priori, NO OFFSET
    nbits  = BitsNeeded_u32(maxabs) ;        // bits needed for largest absolute value
    range  = maxabs - minabs ;
    nbits0 = BitsNeeded_u32(range) ;         // bits needed for range
// fprintf(stderr, "maxabs = %d, minabs = %d, range = %d, nbits0 = %d\n", maxabs, minabs, range, nbits0) ;
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
//   shift = stab[nbits] ;
  nref[0] = nbits/8 ; nref[1] = nbits/2-1 ; nref[2] = nref[1]+1 ; nref[3] = nref[1]+2 ;
  for(i=0 ; i<4 ; i++) ref[i] = 1 << nref[i] ;
//   for(i=0 ; i<4 ; i++){         // 4 canditate number of bits, nref[0] = 0, ref[0] = 1
//     nref[i] = shift & 0xFF ;    // candidate number of bits for short values
//     ref[i] = 1 << nref[i] ;     // value < ref[i] will be a good vcandidate
//     shift >>= 8 ;
//   }
  count_lt(count, (int *)tile, ref, nval) ;   // compare tile values to reference values for this value of nbits
  int nshort, nbitsmax, shortref  ;
  E = 0 ;
  ee = 0 ;
  nbitsmax = nbits * nval ;         // worst case, nbits used for each value
  if(nbits > 1){   // pointless if nbits < 2
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
// fprintf(stderr, "encode tile : SS= %d, M = %d, E = %d, nbits = %d\n", SS, M, E, nbits) ;
  if(nbits > 16){                              // more than 16 bits, use 0011 prefix (long header)
//     token = (0b1111 << 4) | SSME ;             // 1111SSME
    token = (0b0011 << 4) | SSME ;             // 0011SSME
    ntoken = 8 ;
//     CONCAT_TOKENS(token,ntoken,(nbits-1),5) ;  // 1111SSMEbbbbb (8 bits + 5 bits))
    CONCAT_TOKENS(token,ntoken,(nbits-17),4) ;  // 0011SSMEbbbb (8 bits + 4 bits))
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
//     int checkbits = 0 ;                         // temporary diagnostic variable
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
//         checkbits += (nshort + 1) ;
      }else{                                    // use "long" token
        STREAM_INSERT_1(s) ;                    // "long" token marker
        STREAM_PUT_NBITS(s, token, nbits) ;     // follow with nbits bits
//         checkbits += (nbits + 1) ;
      }
    }
// fprintf(stderr, "\n");
    saved_bits += (nval * nbits - nbitsmax - 2) ;
//     if(checkbits != nbitsmax) fprintf(stderr,"checkbits = %d, expecting %d\n", checkbits, nbitsmax) ;
  }
  totbits += nbitsmax ;

end:
// fprintf(stderr, "available space = %ld, used %d\n", available_space, totbits) ;
  STREAM_INSERT_PUSH(s) ;
  *s_in = s ;
// fprintf(stderr, "encode_tile : SS = %d, M = %d, E = %d, nbits = %d, offset = %d\n", SS, M, E, nbits, offset) ;
  return totbits ;
}

typedef struct{
  int32_t ln0 ;   // size of first piece
  int32_t nbk ;   // number of pieces
} split_pair ;
// split n into pieces preferably of size bsize
// n     [IN] : total number of pieces
// bsize [IN] : desired size of pieces
// the first piece may be smaller or larger than the desired size
// if size is even, pieces will be >= bsize/2 or <  bsize + bsize/2
// if size is odd,  pieces will be >  bsize/2 or <= bsize + bsize/2
// pieces will smaller than the minimum only if n is also smaller
static inline split_pair split_by(int n, int bsize){
  split_pair r ;
  r.nbk = (n + bsize/2) / bsize ;      // number of pieces
  r.ln0 = n - (r.nbk - 1) * bsize ;    // size of first piece
  return r ;
}

// encode a block as multiple tiles into a bit stream
// block   [IN] : values to be encoded
// lnis    [IN] : storage length of block rows
// ni      [IN] : number of values to encode in a block row
// nj      [IN] : number of rows to encode
// tsize   [IN] : desired size of encoded tiles
// s_in [INOUT] : pointer to bitstream descriptor (see bitstream.h)
// return nuber of bits inserted into bitstream buffer
// tsize/2 <= dimension < tsize+tsize/2  (for both i and j tile dimensions)
// the dimension of the last slice along i or j may be shorter or longer than tsize
// in case of error, s_in is left as it was upon entry
int encode_block(bitstream *s_in, int32_t *block, int lnis, int ni, int nj, int tsize){
  split_pair ri, rj ;
  ri = split_by(ni, tsize) ;
  rj = split_by(nj, tsize) ;
fprintf(stderr, "ni = %d, nj = %d, blocks[%d(%d,%d),%d(%d,%d)]\n", ni, nj, ri.nbk, ri.ln0, tsize,  rj.nbk, rj.ln0, tsize) ;

tsize = tsize & 0x7FFFFFFF ;   // make sure tsize is EVEN
  int i0, lni, j0, lnj, status, totbits, tmax = tsize+(tsize>>1) ;
  int32_t tile[tsize*tsize*4] ;
  block_properties bp ;
  bitstream s ;

  status = -1 ;
  if(s_in == NULL) goto error ;
  s = *s_in ;        // take local copy of s_in

  totbits = 0 ;
  for(j0=0, lnj = rj.ln0 ; j0<nj ; j0+=lnj, lnj = tsize){
    int32_t *src = block ;
    for(i0=0, lni = ri.ln0 ; i0<ni ; i0+=lni, lni = tsize){
      move_w32_block(src, lnis, tile, lni, lni, lnj, &bp) ;  // get tile from block
      status = encode_tile(&s, tile, lni*lnj, &bp) ;           // encode tile
      if(status <= 0) goto error ;
      totbits += status ;
      src += lni ;
    }
    block += lnj * lnis ;
  }

  *s_in = s ;        // update s_in
  return totbits ;

error:
  return status ;
}

// encode a block as multiple tiles into a bit stream
// block  [OUT] : storage for values to be decoded
// lnid    [IN] : storage length of block rows
// ni      [IN] : number of values to decode in a block row
// nj      [IN] : number of rows to decode
// tsize   [IN] : desired size of encoded tiles
// s_in [INOUT] : pointer to bitstream descriptor (see bitstream.h)
// return nuber of bits extracted into bitstream buffer
// tsize/2 <= dimension < tsize+tsize/2  (for both i and j tile dimensions)
// the dimension of the last slice along i or j may be shorter or longer than tsize
// in case of error, s_in is left as it was upon entry
int decode_block(bitstream *s_in, int32_t *block, int lnid, int ni, int nj, int tsize){
  split_pair ri, rj ;
  ri = split_by(ni, tsize) ;
  rj = split_by(nj, tsize) ;
fprintf(stderr, "ni = %d, nj = %d, blocks[%d(%d,%d),%d(%d,%d)]\n", ni, nj, ri.nbk, ri.ln0, tsize,  rj.nbk, rj.ln0, tsize) ;

  tsize = tsize & 0x7FFFFFFF ;   // make sure tsize is EVEN
  int i0, lni, j0, lnj, status, totbits, tmax = tsize+(tsize>>1) ;
  int32_t tile[tsize*tsize*4] ;
  bitstream s ;

  status = -1 ;
  if(s_in == NULL) goto error ;
  s = *s_in ;        // take local copy of s_in

  totbits = 0 ;
  for(j0=0, lnj = rj.ln0 ; j0<nj ; j0+=lnj, lnj = tsize){
    int32_t *dst = block ;
    for(i0=0, lni = ri.ln0 ; i0<ni ; i0+=lni, lni = tsize){
      status = decode_tile(&s, tile, lni*lnj) ;                // decode tile
      move_w32_block(tile, lni, dst, lnid, lni, lnj, NULL) ;   // put tile into block
      if(status <= 0) goto error ;
      totbits += status ;
      dst += lni ;
    }
    block += lnj * lnid ;
  }

  *s_in = s ;        // update s_in
  return totbits ;

error:
fprintf(stderr, "decode_block : error %d\n", status) ;
  return status ;
}

#if defined(USE_AEC_COMPRESSION)
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
#endif
