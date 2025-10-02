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
// use Big Endian stream encoding
#include <rmn/be_stream.h>
#include <rmn/compare_count.h>
#include <rmn/split_dimension.h>
// deliberate double inclusion
#include <rmn/tile_encoders.h>

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
static int nb32 = 0 ;

static int verbose = 0 ;

void print_encode_stats(int reset){
  fprintf(stderr, "encoding stats : %d blocks (%d constant, %d >=0, %d <=0, %d +/-) (%d with offset), nb32 = %d, ",
                                    nblocks, constant_block, all_plus, all_minus, plus_minus, with_offset, nb32) ;
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
    nb32 = 0 ;
    short_long[0] = short_long[1] = short_long[2] = short_long[3] = 0 ;
  }
}

// decode nval values into tile[nval] from a bit stream
// tile   [OUT] : decoded values
// nval    [IN] : number of values
// s_in [INOUT] : pointer to bitstream descriptor (see bitstream.h)
// return nuber of bits extracted from bitstream
// a safety check is performed to make sure we had enough data available in stream
int decode_tile(bitstream *s_in, int32_t *tile, int32_t nval){
  int i, nbits, totbits, offset, SS, M, E, allminus, iszigzag, lhead, status = 0 ;
  uint32_t token, ee, nboffset, uvalue, *u_tile = (uint32_t *)tile;
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
      u_tile[i] = token ;
// fprintf(stderr, " %d", token);
    }
    totbits = totbits + nval * nbits ;
// fprintf(stderr, "\n");
  }else{                         // USING short/long encoding
    int nref[4] ;
    nref[0] = nbits/8 ; nref[1] = nbits/2-1 ; nref[2] = nref[1]+1 ; nref[3] = nref[1]+2 ;
    int nshort = nref[ee] ;
// fprintf(stderr, "E == 1, ee = %d, nshort = %d\n", ee, nshort) ;
if(verbose) fprintf(stderr, "decoding : E == 1, ee = %d, nshort = %d, nbits = %d\n", ee, nshort, nbits) ;
    for(i=0 ; i<nval ; i++){
      uint32_t flag ;
      STREAM_GET_1(s, flag) ;
      token = 0 ;
      if(flag){                  // long token
        STREAM_GET_NBITS(s, token, nbits) ;
if(verbose) fprintf(stderr, "L(%8.8x)", token) ;
        totbits += nbits ;
      }else{                     // short token
        if(nshort > 0){
          STREAM_GET_NBITS(s, token, nshort) ; totbits += nshort ;
if(verbose) fprintf(stderr, "S(%8.8x)", token) ;
        }
      }
      u_tile[i] = token ;
// fprintf(stderr, "%d ", token) ;
if(verbose){
  if((i&7) == 7) fprintf(stderr, "\n") ;
}
    }
    totbits += nval ;   // account for flag bits
// fprintf(stderr, "\n");
if(verbose) fprintf(stderr, "\n") ;
  }

  if(M != 0){    // add offset if needed (must be done before eventual sign reversal)
    for(i=0 ; i<nval ; i++) tile[i] = tile[i] + offset ;
  }
  if(allminus){   // invert sign (mutually exclusive with iszigzag)
    for(i=0 ; i<nval ; i++) tile[i] = -tile[i] ;
  }
  if(iszigzag){  // restore signed form (mutually exclusive with allminus)
if(verbose) fprintf(stderr, "iszigzag\n");
if(verbose){ 
  for(i=0 ; i<nval ; i++){
    fprintf(stderr, "%8.8x ", tile[i]) ;
    if( (i&7) == 7) fprintf(stderr, "\n");
  }
  fprintf(stderr, "\n");
}
    for(i=0 ; i<nval ; i++){
      tile[i] = from_zigzag_32(u_tile[i]) ;
if(verbose){ 
  fprintf(stderr, "%8.8x ", tile[i]) ;
  if( (i&7) == 7) fprintf(stderr, "\n");
}
    }
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
// options [IN] : encoding options , ENCODE_DRY_RUN | ENCODE_NO_SHORT_LONG
// return nuber of bits inserted into bitstream buffer, -1 in case of error
// a dry run seems to need about 1 third of the time of a full encoding run
int encode_tile(bitstream *s_in, int32_t *tile_in, int32_t nval, block_properties *bp, int options){
  int i, nbits, nbits0, offset, nboffset, totbits, SS, M, E, ee, SSME, ntoken, range, maxabs, minabs, status ;
  uint32_t token ;
  bitstream s ;
  int32_t tile[nval] ;
  uint32_t *u_tile = (uint32_t *) tile ;     // address contents of tile as unsigned integers
  block_properties bp_ ;
  int dry_run = ( (options & ENCODE_DRY_RUN) != 0 ) ;  // a dry run only evaluates the number of bits needed for encoding

  if(tile_in == NULL || nval <= 0) goto error ;        // invalid arguments
  if(s_in == NULL && (! dry_run)) goto error ;         // s_in can only be NULL for a dry run

  if(! dry_run){
    s = *s_in ;                                        // local copy of stream state (in case of error)
    if(s.endian != PACK_ENDIAN) goto error ;           // stream has the wrong endianness
    if(StreamAvailableSpace(s_in) < 64) goto error ;   // not enough room for header + basic encoding information
  }

  if(bp == NULL){                            // block properties not available, compute them
    bp = &bp_;                               // use local block properties struct
    status = analyze_data32_block(tile_in, nval, nval, 1, bp);
    if(status != nval){                      // error detected in analyze_data32_block
      fprintf(stderr, "ERROR: analyze_data32_block status = %d, expected %d\n", status, nval) ;
      goto error ;
    }
    bp_.kind = int_data ;                    // no need to call adjust_block_properties for signed integer data
  }

  if(! dry_run) nblocks++ ;                  // do not update statistics if dry run
  SS = M = E = ee = 0 ;                      // nullify all components of header
  totbits  = 0 ;                             // total number of bits needed/used
  offset   = 0 ;                             // encoding offset
  nboffset = 0 ;                             // number of bits needed to encode offset
  // bp->maxs.i, bp->mins.i : signed max and min values in tile
  // =============================== constant tile ===============================
  if(bp->maxs.i == bp->mins.i){              // constant value in tile
    uint32_t zigzag ;
    zigzag = to_zigzag_32(tile_in[0]) ;      // encode constant value as sign/magnitude
    // number of bits needed to represent encoded value (at least 1)
    nbits = (zigzag == 0) ? 1 : BitsNeeded_u32(zigzag) ;
    totbits = 8 + nbits ;
    if(dry_run) goto dry_end ;               // quick exit

    token = (0b000 << 5) ;                   // header will be ( 000bbbbb )
    token = token | (nbits - 1) ;            // insert nbits - 1 into header (bbbbb)
    STREAM_PUT_NBITS(s, token, 8) ;          // 8 bit header
    STREAM_PUT_NBITS(s, zigzag, nbits) ;     // constant value encoded as zigzag (nbits bits)
    constant_block++ ;

    goto end ;                               // done
  }
  // =============================== convert to positive values ===============================
  M = E = 0 ;
  // determine number of bits needed to encode the largest value
  if(bp->maxs.i > 0 && bp->mins.i < 0){      // both positive and negative values are present
    uint32_t minz, maxz ;
    if(! dry_run) plus_minus++ ;             // do not update statistics if dry run
    SS = 3 ;                                 // both signs are present
    M  = 0 ;                                 // offset will not be used (might get used in later implementation)
    for(i=0 ; i<nval ; i++)                  // convert to sign/magnitude (zigzag)
      u_tile[i] = to_zigzag_32(tile_in[i]) ; // use local storage for converted tile values
    minz = to_zigzag_32(bp->mins.i) ;        // negative number with largest absolute value
    maxz = to_zigzag_32(bp->maxs.i) ;        // positive number with largest absolute value
    maxz = (minz > maxz) ? minz : maxz ;
    nbits = BitsNeeded_u32(maxz) ;           // number of bits needed for largest zigzag value

  }else{                                     // all >= 0 or all <= 0
    if(bp->maxs.i <= 0){                     // all values <= 0
      if(! dry_run) all_minus++ ;            // do not update statistics if dry run
      SS = 2 ;                               // all values negative flag
      maxabs = -bp->mins.i ;                 // most negative number
      minabs = -bp->maxs.i ;                 // least negative number
      for(i=0 ; i<nval ; i++)                // use absolute value if all <= 0
        tile[i] = -tile_in[i] ;              // use local array for tile values

    }else if(bp->mins.i >= 0){               // all values >= 0
      if(! dry_run) all_plus++ ;             // do not update statistics if dry run
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
      if(! dry_run) with_offset++ ;          // do not update statistics if dry run
      offset = minabs ;                      // smallest absolute value
      nboffset = BitsNeeded_u32(offset) ;    // number of bits needed to store offset (minimum 1)
      nboffset = (nboffset > 0) ? nboffset : 1 ;
      nbits = nbits0 ;                       // reduced number of bits
      M = 1 ;                                // OFFSET IS USED
      for(i=0 ; i<nval ; i++){
        tile[i] = tile[i] - offset ;         // subtract offset from tile values (result guaranteed >= 0)
      }
    }
  }
  if(verbose) fprintf(stderr, "nbits = %d\n", nbits) ;
  if((nbits > 30) && (! dry_run)) nb32++ ;   // do not update statistics if dry run
  // =============================== determine encoding format ===============================
  // is it worth using short/long encoding ?
  int ref[4], nref[4], count[4] ;
  nref[0] = nbits/8 ; nref[1] = nbits/2-1 ; nref[2] = nref[1]+1 ; nref[3] = nref[1]+2 ;
  for(i=0 ; i<4 ; i++) ref[i] = 1 << nref[i] ;
  count_lt(count, (int *)tile, ref, nval) ;  // compare tile values to reference values for this value of nbits
  int nshort, nbitsmax  ;
  uint32_t shortref ;
  int ee_ok = ( (options & ENCODE_NO_SHORT_LONG) == 0) ;
  E = 0 ;
  ee = 0 ;
  nbitsmax = nbits * nval ;                  // worst case, nbits used for each value
  ee_ok |= (nbits > 1) ;
  if(ee_ok){                                 // ee encoding is pointless if nbits < 2 or ENCODE_NO_SHORT_LONG
    for(i=0 ; i<4 ; i++){
      int nbitsi ;
      nbitsi  = count[i] * (nref[i]+1) ;     // count[i] "short" values, needing nref[i]+1 bits
      nbitsi += (nval-count[i]) * (nbits+1) ;// nval - count[i] "long" values, needing nbits+1 bits
      if(nbitsmax > nbitsi){                 // we have a better candidate
        nbitsmax = nbitsi ;                  // new worts case
        nshort   = nref[i] ;                 // length of "short" values
        E = 1 ;                              // short/long encoding will be used
        ee = i ;                             // identify option used
        if(! dry_run) short_long[ee]++ ;     // do not update statistics if dry run
      }
    }
  }
  // at this point nbitsmax is equal to the number or bits needed for encoding the values
  // =============================== store header ===============================
  SSME = (SS << 2) | (M << 1) | E ;
  if(nbits > 16){                              // more than 16 bits, use 0011 prefix (long header)
    token = (0b0011 << 4) | SSME ;             // 0011SSME
    ntoken = 8 ;
    CONCAT_TOKENS(token,ntoken,(nbits-17),4) ;  // 0011SSMEbbbb (8 bits + 4 bits))
  }else{                                       // 16 bits or less, use short header
    token = (SSME << 4) | (nbits-1)  ;         // SSMEbbbb (8 bits)
    ntoken = 8 ;
  }
  if(E != 0){                                  // add ee (2 bits)
    CONCAT_TOKENS(token,ntoken,ee,2) ;         // SSMEbbbbee or 0011SSMEbbbbee
  }
  if(M != 0){                                  // add number of bits needed for offset (5 bits)
    CONCAT_TOKENS(token,ntoken,(nboffset-1), 5) ;
  }
  totbits += ntoken ;                          // header, 8 bits to 20 bits
  if(! dry_run)
    STREAM_PUT_NBITS(s, token, ntoken) ;       // SSMEbbbb[ee][nnnnn] or 0011SSMEbbbb[ee][nnnnn]
  if(M != 0){                                  // OFFSET is used
    totbits += nboffset ;                      // nboffset bits for offset
    if(! dry_run)
      STREAM_PUT_NBITS(s, offset, nboffset) ;  // offset
  }
  // =============================== encode and store values ===============================
  totbits += nbitsmax ;
  if(dry_run) goto dry_end ;                   // dry run, job is done

  if(StreamAvailableSpace(&s) < nbitsmax)      // not enough room for encoded data
    goto error ;

  if(E == 0){                                  // no short/long encoding, all tokens will be nbits long
    for(i=0 ; i<nval ; i++){
      STREAM_PUT_NBITS(s, u_tile[i], nbits) ;
    }
  }else{                                       // use short/long encoding, tokens will be nshort+1 or nbits+1 bits long
    shortref = (1 << nshort) ;                 // anything < shortref can be coded as a "short" token
    for(i=0 ; i<nval ; i++){                   // encode nval values
      token = u_tile[i] ;                      // value to encode
      STREAM_INSERT_CHECK(s) ;                 // make sure there is room for up to 32 bits
      if(token < shortref){                    // use "short" token
        STREAM_FAST_PUT_0(s) ;  ;              // "short" token marker
        if(nshort > 0){                        // follow with nshort bits
          STREAM_PUT_NBITS(s, token, nshort) ;
        }
      }else{                                    // use "long" token
        STREAM_FAST_PUT_1(s) ;                  // "long" token marker
        STREAM_PUT_NBITS(s, token, nbits) ;     // follow with nbits bits
      }
    }
    saved_bits += (nval * nbits - nbitsmax - 2) ;
  }

end:
  STREAM_INSERT_PUSH(s) ;
  *s_in = s ;        // propagate stream state

dry_end:             // only return number of bits needed if dry run
  return totbits ;

error:
  return -1 ;
}

// encode a block as multiple tiles into a bit stream
// block   [IN] : values to be encoded
// lnis    [IN] : storage length of block rows
// ni      [IN] : number of values in a block row
//                number of values (encode_block_1d)
// nj      [IN] : number of rows to encode
// tsize   [IN] : base dimension of encoded tiles (8 suggested, >= 4 mandatory)
// s_in [INOUT] : pointer to bitstream descriptor (see bitstream.h)
// return nuber of bits inserted into bitstream buffer
// tsize/2 <= actual tile dimension < tsize+tsize/2  (for both i and j tile dimensions)
// the dimension of the last slice along i or j may be shorter or longer than tsize
// in case of error, s_in is left as it was upon entry
// store tsize and imensions into bitstream for verification by decoder
//       - XXb nb de dimensions -1 
//       - 00b/8 bits, 01b/16 bits, 11b/32 bits (dimensions)
//       - 00b/8 bits, 01b/16 bits, 11b/32 bits (tsize)
//
// 1D encoder, called by the 2D encoder
static int encode_block_1d(bitstream *s_in, int32_t *block, int n, int tsize, int options){
  int i0, ln, status = -1, totbits ;
  int32_t *tile ;
  block_properties bp ;

  array_axis ri = split_axis(n, tsize) ;

  totbits = 0 ;
  for(i0=0, ln = ri.ln0 ; i0<n ; i0+=ln, ln = tsize){
    tile = block + i0 ;
    analyze_data32_block(tile, ln, ln, 1, &bp) ;
    status = encode_tile(s_in, tile, ln, &bp, options) ;
    if(status <= 0) goto error ;
    totbits += status ;
  }
  return totbits ;   // return number of bits inserted

error:
  return status ;    // *s_in will be restored by encode_block
}

// get value and code from bitstream
uint32_t stream_get_hbw(bitstream *s, int *totbits){   // replace with STREAM_GET_BHW macro ?
  uint32_t nbits, value ;
  STREAM_GET_NBITS(*s, nbits, 2) ;
  nbits++ ; nbits <<= 3 ;   // (nbits+1) * 8 = number of bits
  STREAM_GET_NBITS(*s, value, nbits) ;
  *totbits = *totbits + 2 + nbits ;
  return value ;
}

// put value and code into bitstream
// 2 bit code = (nbytes needed for value - 1)
// 8/16/32 bits only
// return number of bits inserted
uint32_t stream_put_hbw(bitstream *s, int32_t value){   // replace with STREAM_PUT_BHW macro ?
  if(value & 0xFFFF){
    STREAM_PUT_NBITS(*s, 3, 2) ;
    STREAM_PUT_NBITS(*s, value, 32) ;        // there is NOT guaranteed room for 32 bits
    return 34 ;
  }else if(value & 0xFF){
    STREAM_PUT_NBITS(*s, 1, 2) ;
    STREAM_FAST_PUT_NBITS(*s, value, 16) ;   // there is guaranteed room for 16 bits
    return 18 ;
  }else{
    STREAM_PUT_NBITS(*s, 0, 2) ;
    STREAM_FAST_PUT_NBITS(*s, value, 8) ;    // there is guaranteed room for 8 bits
    return 10 ;
  }
};

// 2D encoder
int encode_block(bitstream *s_in, int32_t *block, int lnis, int ni, int nj, int tsize, int options){
  int i0, lni, j0, lnj, status = -1, totbits = 0 ;
  bitstream s ;

  if(block == NULL || lnis <=0 || ni <= 0 || nj <= 0 || tsize < 4) return status ;

  tsize = tsize & 0xFFFE ;     //force tsize to reasonable EVEN value
  int32_t tile[tsize*tsize*4] ;
  block_properties bp ;

  if(s_in){
    s = *s_in ;                // save bitstream descriptor in case of error
    // put block dimensions and tsize into bit stream
    STREAM_PUT_NBITS(*s_in, 1, 2) ;                 // 2 dimensions
    totbits = 2 ;
    totbits += stream_put_hbw(s_in, ni) ;    // STREAM_PUT_BHW(s, v, nbits)
    totbits += stream_put_hbw(s_in, nj) ;
    totbits += stream_put_hbw(s_in, tsize) ;
  }

  if(ni == 1 || nj == 1){
    status = encode_block_1d(s_in, block, ni*nj, tsize*tsize, options) ;
    if(status <= 0) goto error ;
    totbits += status ;
    goto end ;
  }

  array_axis ri, rj ;
  ri = split_axis(ni, tsize) ;
  rj = split_axis(nj, tsize) ;

  for(j0=0, lnj = rj.ln0 ; j0<nj ; j0+=lnj, lnj = tsize){
    int32_t *src = block ;
    for(i0=0, lni = ri.ln0 ; i0<ni ; i0+=lni, lni = tsize){
      move_w32_block(src, lnis, tile, lni, lni, lnj, &bp) ;             // get tile from block
      status = encode_tile(s_in, tile, lni*lnj, &bp, options) ;         // encode tile
      if(status <= 0) goto error ;
      totbits += status ;
      src += lni ;
    }
    block += lnj * lnis ;
  }

end:
  return totbits ;

error:
  if(s_in) *s_in = s ;    // restore bitstream descriptor if one was supplied
  return status ;         // return error status
}

// decode a block as multiple tiles from a bit stream
// block  [OUT] : storage for values to be decoded
// lnid    [IN] : storage length of block rows
// ni      [IN] : number of values in a block row
//                number of values (decode_block_1d)
// nj      [IN] : number of rows to decode
// tsize   [IN] : base dimension of encoding tiles (MUST be the same size used by encoder)
// s_in [INOUT] : pointer to bitstream descriptor (see bitstream.h)
// return nuber of bits extracted from bitstream buffer
// tsize/2 <= actual tile dimension < tsize+tsize/2  (for both i and j tile dimensions)
// the dimension of the last slice along i or j may be shorter or longer than tsize
// in case of error, s_in is left as it was upon entry
// get tsize and dimensions from bit stream (from encoder) for consistency checking
//       - XXb nb de dimensions -1 (2 bits)
//       - 00b/8 bits, 01b/16 bits, 11b/32 bits (dimensions)
//       - 00b/8 bits, 01b/16 bits, 11b/32 bits (tsize)
// 1D decoder
static int decode_block_1d(bitstream *s_in, int32_t *block, int ni, int tsize){
  tsize = tsize & 0xFFFE ;   //force tsize to reasonable EVEN value
  int i0, lni, status = -1, totbits ;
  int32_t *tile ;
  array_axis ri ;

  ri = split_axis(ni, tsize) ;

  totbits = 0 ;
  for(i0=0, lni = ri.ln0 ; i0<ni ; i0+=lni, lni = tsize){
    tile = block + i0 ;
    status = decode_tile(s_in, tile, lni) ;
    if(status <= 0) goto error ;
    totbits += status ;
  }

  return totbits ;

error:
  return status ;
}

// 2D decoder
int decode_block(bitstream *s_in, int32_t *block, int lnid, int ni, int nj, int tsize){
  int i0, lni, j0, lnj, status = -1, totbits ;
  bitstream s = *s_in ;
  goto code ;

error:                    // error block at beginning because of dynamic allocation of tile
  if(s_in) *s_in = s ;    // restore bitstream descriptor if one was supplied
  return status ;

code:
  if(s_in == NULL || block == NULL || lnid <= 0 || ni <= 0 || nj <= 0 || tsize < 4) return status ;

  tsize = tsize & 0xFFFE ;   //force tsize to reasonable EVEN value
  uint32_t code ;

  STREAM_GET_NBITS(*s_in, code, 2) ;    // get and check number of dimensions from bit stream
  if(code != 1) goto error ;            // OOPS, expected 2 dimensions (code == 1)
  totbits = 2 ;
  uint32_t ni_    = stream_get_hbw(s_in, &totbits) ;    // STREAM_GET_BHW(s, v, nbits)
  uint32_t nj_    = stream_get_hbw(s_in, &totbits) ;
  uint32_t tsize_ = stream_get_hbw(s_in, &totbits) ;

  if((uint32_t)ni != ni_ || (uint32_t)nj != nj_ || (uint32_t)tsize != tsize_) goto error ;
  int32_t tile[tsize*tsize*4] ;         // worst case size

  if(ni == 1 || nj == 1){
    // in 1 D case, use square of tile size value as tile size
    status = decode_block_1d(s_in, block, ni*nj, tsize*tsize) ;
    if(status <= 0) goto error ;
    totbits += status ;
    goto end ;
  }

  array_axis ri, rj ;
  ri = split_axis(ni, tsize) ;
  rj = split_axis(nj, tsize) ;

  for(j0=0, lnj = rj.ln0 ; j0<nj ; j0+=lnj, lnj = tsize){
    int32_t *dst = block ;
    for(i0=0, lni = ri.ln0 ; i0<ni ; i0+=lni, lni = tsize){
      status = decode_tile(s_in, tile, lni*lnj) ;                // decode tile
      move_w32_block(tile, lni, dst, lnid, lni, lnj, NULL) ;   // put tile into block
      if(status <= 0) goto error ;
      totbits += status ;
      dst += lni ;
    }
    block += lnj * lnid ;
  }
end:
  return totbits ;
}

// not implemented yet
#if 0
// encode an array as multiple blocks/tiles into a bit stream
// array   [IN] : values to be encoded
// lnis    [IN] : storage length of block rows
// ni      [IN] : number of values in a block row
//                number of values (encode_block_1d)
// nj      [IN] : number of rows to encode
// bsize   [IN] : desired size of encoding blocks (64 suggested)
// tsize   [IN] : desired size of encoding tiles (8 suggested)
// s_in [INOUT] : pointer to bitstream descriptor (see bitstream.h)
// return nuber of bits inserted into bitstream buffer
// tsize/2 <= dimension < tsize+tsize/2  (for both i and j tile dimensions)
// the dimension of the last blocks along i or j may be shorter or longer than bsize/tsize
// in case of error, s_in is left as it was upon entry
// 1 bit : block size is 8/16 bits, followed by 8/16 bits for bsize value
int encode_array(bitstream *s_in, int32_t *array, int lnis, int ni, int nj, int bsize, int tsize, int options){
  tsize = tsize & 0x7FFFFFF8 ;   //force tsize to multiple of 8
  bsize = bsize & 0x7FFFFFFE ;   //force bsize to EVEN value
  return -1 ;
}

// decode a block as multiple blocks/tiles from a bit stream
// array  [OUT] : storage for values to be decoded
// lnid    [IN] : storage length of block rows
// ni      [IN] : number of values in a block row
//                number of values (decode_block_1d)
// nj      [IN] : number of rows to decode
// bsize   [IN] : base size of encoding blocks (MUST be the same size used by encoder)
// tsize   [IN] : base size of encoding tiles (MUST be the same size used by encoder)
// s_in [INOUT] : pointer to bitstream descriptor (see bitstream.h)
// return nuber of bits extracted from bitstream buffer
// tsize/2 <= dimension < tsize+tsize/2  (for both i and j tile dimensions)
// the dimension of the last blocks along i or j may be shorter or longer than bsize/tsize
// in case of error, s_in is left as it was upon entry
// 1 bit : block size is 8/16 bits, followed by 8/16 bits for bsize value
int decode_array(bitstream *s_in, int32_t *array, int lnis, int ni, int nj, int bsize, int tsize){
  tsize = tsize & 0x7FFFFFF8 ;   //force tsize to multiple of 8
  bsize = bsize & 0x7FFFFFFE ;   //force bsize to EVEN value
  return -1 ;
}
#endif

// #define USE_AEC_COMPRESSION
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
