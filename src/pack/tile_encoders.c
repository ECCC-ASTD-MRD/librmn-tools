// Hopefully useful code for C
// Copyright (C) 2022  Recherche en Prevision Numerique
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

#include <stdio.h>
#include <rmn/tile_encoders.h>

#include <with_simd.h>

#define NB0 (2)

// internal use for debug purposes
static void print_stream_params(bitstream s, char *msg, char *expected_mode){
  fprintf(stderr, "%s: filled = %d(%d), free= %d, first/in/out = %p/%p/%p [%ld], insert/xtract = %d/%d, in = %ld, out = %ld \n",
    msg, StreamAvailableBits(&s), StreamStrictAvailableBits(&s), StreamAvailableSpace(&s), 
    s.first, s.out, s.in, s.in-s.out, s.insert, s.xtract, s.in-s.first, s.out-s.first ) ;
  if(expected_mode){
    fprintf(stderr, "Stream mode = %s(%d) (%s expected)\n", StreamMode(s), StreamModeCode(s), expected_mode) ;
    if(strcmp(StreamMode(s), expected_mode) != 0) { 
      fprintf(stderr, "Bad mode, exiting\n") ;
      exit(1) ;
    }
  }
}

// print tile properties as set by encode_tile_properties
// p64 [IN] : blind token containing tile properties 
// (from encode_tile_scheme/constant_tile_scheme/encode_tile_properties)
void print_tile_properties(uint64_t p64){
  tile_properties p ;
  int nij, nbits, nbts0, nbitss, nb0, nbi ;

  p.u64 = p64 ;
  nij = (p.t.h.npti+1)*(p.t.h.nptj+1) ;
  nbi = p.t.h.nbts + 1 ;
  nb0 = (nbi + NB0) >> 1 ;
  nbits = nij * nbi ;
  nbts0 = (nij - p.t.nzero)*(nbi + 1) + p.t.nzero ;
  nbitss = (nij - p.t.nshort)*(nbi + 1) + p.t.nshort * (nb0 + 1) ;
  fprintf(stderr, "(t_p) nbits = %d(%d), sign = %d, encoding = %d, %d x %d, min0 = %d, min = %8.8x, nzero = %3d, nshort = %3d, bits = %d/%d/%d\n",
          nbi, nb0, p.t.h.sign, p.t.h.encd, p.t.h.npti+1, p.t.h.nptj+1, p.t.h.min0, p.t.min, p.t.nzero, p.t.nshort, nbits, nbts0, nbitss) ;
}

// set encoding scheme deemed appropriate given tile properties
// p64 [IN] : tile parameter structure as an "opaque" 64 bit value
// the function returns a tile parameter structure as an "opaque" 64 bit value
// this value will eventually be passed to the encoder itself
// used by encode_tile_properties
static uint64_t encode_tile_scheme(uint64_t p64){
  tile_properties p ;
  int nbits_temp, nbits_full, nbits_zero, nbits_short, nij, nbits, nbits0, nzero, nshort ;

  p.u64 = p64 ;
  nij    = (p.t.h.npti + 1) * (p.t.h.nptj + 1) ;                         // number of points
  nbits = p.t.h.nbts + 1 ;
  nbits0 = (nbits + NB0) >> 1 ;  // number of bits for "short" values
  nzero  = p.t.nzero ;
  nshort = p.t.nshort ;

  nbits_full  = nij * nbits ;                                            // all tokens at full length
  nbits_zero  = nzero + (nij - nzero) * (nbits + 1) ;                    // nzero tokens at length 1, others nbits + 1
  nbits_short = nshort * (nbits0 + 1) + (nij - nshort) * (nbits + 1) ;   // short tokens tokens nbits0 + 1, others nbits + 1

  nbits_temp = nbits_full ;
  p.t.h.encd = 0 ;                        // default is no special encoding
  p.t.nshort = p.t.nzero = 0 ;            // no "short" or "0" token
  if(nbits_zero < nbits_temp){            // encoding 2 better ?
    nbits_temp = nbits_zero ;
    p.t.h.encd = 2 ;                      // use 0 , 1//nbits encoding
    p.t.nzero = nzero ;                   // "0" tokens used
  }
  if(nbits_short < nbits_temp){           // encoding 1 better ?
    p.t.h.encd = 1 ;                      // use 0//nbits0 , 1//nbits encoding
    p.t.nshort = nshort ;                 // "short" tokens used
    p.t.nzero = 0 ;                       // no "0" token
  }
fprintf(stderr, "normal tile, min = %8.8x (%d x %d)\n", p.t.min, p.t.h.npti+1, p.t.h.nptj+1) ;
  return p.u64 ;
}

// set information appropriate to a constant valued tile
// p64 [IN] : tile parameter structure as an "opaque" 64 bit value
// the function returns a tile parameter structure as an "opaque" 64 bit value
// this value will eventually be passed to the encoder itself
// used by encode_tile_properties
static uint64_t constant_tile_scheme(uint64_t p64){
  tile_properties p ;

  p.u64   = p64 ;
fprintf(stderr, "constant tile, min = %8.8x (%d x %d)\n", p.t.min, p.t.h.npti+1, p.t.h.nptj+1) ;
  p.t.h.encd = 3 ;     // constant tile
  p.t.h.min0 = 0 ;     // not needed
  p.t.nzero  = 0 ;     // not needed
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
static uint64_t encode_tile_properties_c(void *f, int ni, int lni, int nj, uint32_t tile[64]){
  uint32_t *block = (uint32_t *)f ;
  uint32_t pos, neg, nshort, nzero ;
  uint32_t max, min, nbits, nbits0, mask0 ;
  tile_properties p ;
  int32_t i, j, i0, nij ;

  p.u64 = 0 ;
  p.t.h.npti = ni - 1 ;
  p.t.h.nptj = nj - 1;
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
  if( (32 - (lzcnt_32(max-min)) < nbits) || 1 ){   // we gain one bit if subtracting the minimum value
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
  mask0 = ~mask0 ;                           // keep only the upper bits

  nshort = nzero = 0 ;
  for(i=0 ; i<nij ; i++){
    if(tile[i] == 0)           nzero++ ;      // zero value
    if((tile[i] & mask0) == 0) nshort++ ;     // value needing nbits0 bits or less
  }
  p.t.nzero  = nzero ;
  p.t.nshort = nshort ;

  return encode_tile_scheme(p.u64) ;          // determine appropriate encoding scheme

constant:
  p.t.min = min ;                             // constant value in min
  return constant_tile_scheme(p.u64) ;        // setup for constant values
}

// gather tile to be encoded, determine useful data properties
// f     [IN] : array from which block will be extracted
// ni    [IN] : first dimension of tile to be encoded (1 <= ni <= 8)
// lni   [IN] : row storage length in array f
// nj    [IN] : second dimension of tile to be encoded (1 <= nj <= 8)
// tile [OUT] : extracted tile (in sign/magnitude form and with minimum value possibly subtracted)
// the function returns a tile parameter structure as an "opaque" 64 bit value
// this value will then be passed to the encoder itself
// X86_64 AVX2 version, 8 x 8 tiles only (ni ==8, nj == 8)
uint64_t encode_tile_properties(void *f, int ni, int lni, int nj, uint32_t tile[64]){
  uint32_t *block = (uint32_t *)f ;
  int32_t pos, neg, nshort, nzero ;
  uint32_t max, min, nbits, nbits0, mask0 ;
  tile_properties p ;

#if defined(__x86_64__) && defined(__AVX2__) && defined(WITH_SIMD)
  __m256i v0, v1, v2, v3, v4, v5, v6, v7 ;
  __m256i vp, vn, vz, vs, vm0, vmax, vmin, vz0 ;
  __m128i v128 ;

  if(ni != 8 || nj != 8) return encode_tile_properties_c(f, ni, lni, nj, tile) ; // not a full 8x8 tile

  p.u64 = 0 ;
  p.t.h.npti = ni - 1 ;
  p.t.h.nptj = nj - 1;

  v0 = _mm256_loadu_si256((__m256i *)(block)) ; block += lni ;  // load 8 x 8 values from original array
  v1 = _mm256_loadu_si256((__m256i *)(block)) ; block += lni ;
  v2 = _mm256_loadu_si256((__m256i *)(block)) ; block += lni ;
  v3 = _mm256_loadu_si256((__m256i *)(block)) ; block += lni ;
  v4 = _mm256_loadu_si256((__m256i *)(block)) ; block += lni ;
  v5 = _mm256_loadu_si256((__m256i *)(block)) ; block += lni ;
  v6 = _mm256_loadu_si256((__m256i *)(block)) ; block += lni ;
  v7 = _mm256_loadu_si256((__m256i *)(block)) ;

  vn = _mm256_and_si256(v0, v1) ; vp = _mm256_or_si256(v0, v1) ;  // "and" for negative, "and not" for non negative
  vn = _mm256_and_si256(vn, v2) ; vp = _mm256_or_si256(vp, v2) ;
  vn = _mm256_and_si256(vn, v3) ; vp = _mm256_or_si256(vp, v3) ;
  vn = _mm256_and_si256(vn, v4) ; vp = _mm256_or_si256(vp, v4) ;
  vn = _mm256_and_si256(vn, v5) ; vp = _mm256_or_si256(vp, v5) ;
  vn = _mm256_and_si256(vn, v6) ; vp = _mm256_or_si256(vp, v6) ;
  vn = _mm256_and_si256(vn, v7) ; vp = _mm256_or_si256(vp, v7) ;
  pos = (_mm256_movemask_ps((__m256) vp) == 0x00) ? 1 : 0 ;           // check that all MSB bits are on 
  neg = (_mm256_movemask_ps((__m256) vn) == 0xFF) ? 1 : 0 ;

  v0 = _mm256_xor_si256( _mm256_add_epi32(v0, v0) , _mm256_srai_epi32(v0, 31) ) ;  // convert to ZigZag
  v1 = _mm256_xor_si256( _mm256_add_epi32(v1, v1) , _mm256_srai_epi32(v1, 31) ) ;
  v2 = _mm256_xor_si256( _mm256_add_epi32(v2, v2) , _mm256_srai_epi32(v2, 31) ) ;
  v3 = _mm256_xor_si256( _mm256_add_epi32(v3, v3) , _mm256_srai_epi32(v3, 31) ) ;
  v4 = _mm256_xor_si256( _mm256_add_epi32(v4, v4) , _mm256_srai_epi32(v4, 31) ) ;
  v5 = _mm256_xor_si256( _mm256_add_epi32(v5, v5) , _mm256_srai_epi32(v5, 31) ) ;
  v6 = _mm256_xor_si256( _mm256_add_epi32(v6, v6) , _mm256_srai_epi32(v6, 31) ) ;
  v7 = _mm256_xor_si256( _mm256_add_epi32(v7, v7) , _mm256_srai_epi32(v7, 31) ) ;

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
  vmin = _mm256_broadcastd_epi32(v128) ;
  _mm_storeu_si32(&min, v128) ;

  v128 = _mm_max_epu32( _mm256_extractf128_si256 (vmax, 0) ,  _mm256_extractf128_si256 (vmax, 1) ) ;  // reduce to 1 value for max
  v128 = _mm_max_epu32(v128, _mm_shuffle_epi32(v128, 0b11101110) ) ; // [0,1,2,3] [2,2,2,3]
  v128 = _mm_max_epu32(v128, _mm_shuffle_epi32(v128, 0b01010101) ) ; // [0,1,2,3] [1,1,1,1]
  _mm_storeu_si32(&max, v128) ;

  if(max == min) goto constant;   // constant field
  // subtract min from max and vo -> v7
  // set  min0 to 1
  nbits = 32 - lzcnt_32(max) ;
  if( (32 - (lzcnt_32(max-min)) < nbits) || 1 ){   // we gain one bit if subtracting the minimum value
    max = max - min ;
    v0 = _mm256_sub_epi32(v0, vmin) ;
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
  nbits = 32 - lzcnt_32(max) ;

  if(pos || neg){                     // all values have same sign
    nbits-- ;
    v0 =  _mm256_srli_epi32(v0, 1) ;  // suppress ZigZag sign for values and min
    v1 =  _mm256_srli_epi32(v1, 1) ;
    v2 =  _mm256_srli_epi32(v2, 1) ;
    v3 =  _mm256_srli_epi32(v3, 1) ;
    v4 =  _mm256_srli_epi32(v4, 1) ;
    v5 =  _mm256_srli_epi32(v5, 1) ;
    v6 =  _mm256_srli_epi32(v6, 1) ;
    v7 =  _mm256_srli_epi32(v7, 1) ;
    min >>= 1 ;
    p.t.h.sign = (neg << 1) | pos ;
  }else{
    p.t.h.sign = 3 ;                  // both positive and negative values, ZigZag encoding with sign bit
  }

  _mm256_storeu_si256((__m256i *)(tile   ), v0) ;  // store ZigZag values (possibly shifted) into tile
  _mm256_storeu_si256((__m256i *)(tile+ 8), v1) ;
  _mm256_storeu_si256((__m256i *)(tile+16), v2) ;
  _mm256_storeu_si256((__m256i *)(tile+24), v3) ;
  _mm256_storeu_si256((__m256i *)(tile+32), v4) ;
  _mm256_storeu_si256((__m256i *)(tile+40), v5) ;
  _mm256_storeu_si256((__m256i *)(tile+48), v6) ;
  _mm256_storeu_si256((__m256i *)(tile+56), v7) ;

  if(min == 0) p.t.h.min0 = 0 ;    // if min == 0, flag it as not used
  p.t.min = min ;
  if(min != 0) p.t.h.min0 = 1 ;
  p.t.h.nbts = nbits - 1 ;
  p.t.h.encd = 0 ;
  vz0 = _mm256_xor_si256(v0, v0) ;  // vector of zeros

  vz = _mm256_cmpeq_epi32(v0, vz0) ;                         // count zero values
  vz = _mm256_add_epi32(vz, _mm256_cmpeq_epi32(v1, vz0)) ;
  vz = _mm256_add_epi32(vz, _mm256_cmpeq_epi32(v2, vz0)) ;
  vz = _mm256_add_epi32(vz, _mm256_cmpeq_epi32(v3, vz0)) ;
  vz = _mm256_add_epi32(vz, _mm256_cmpeq_epi32(v4, vz0)) ;
  vz = _mm256_add_epi32(vz, _mm256_cmpeq_epi32(v5, vz0)) ;
  vz = _mm256_add_epi32(vz, _mm256_cmpeq_epi32(v6, vz0)) ;
  vz = _mm256_add_epi32(vz, _mm256_cmpeq_epi32(v7, vz0)) ;
  v128 = _mm_add_epi32(_mm256_extractf128_si256 (vz, 0) ,  _mm256_extractf128_si256 (vz, 1)) ;
  v128 = _mm_add_epi32(v128,  _mm_shuffle_epi32(v128, 0b11101110) ) ; // [0,1,2,3] [2,2,2,3]
  v128 = _mm_add_epi32(v128,  _mm_shuffle_epi32(v128, 0b01010101) ) ; // [0,1,2,3] [1,1,1,1]
  _mm_storeu_si32(&nzero, v128) ; p.t.nzero = -nzero ;

  nbits0 = (nbits + NB0) >> 1 ;     // number of bits for "short" values
  mask0 = RMASK31(nbits0) ;         // value & ~mask0 will be 0 if nbits0 or less bits are needed
  mask0 = ~mask0 ;                  // keep only the upper bits
  vm0 = _mm256_set1_epi32(mask0) ;  // vector of mask0

  vs = _mm256_cmpeq_epi32(_mm256_and_si256(v0, vm0), vz0) ;  // count values <= mask0
  vs = _mm256_add_epi32(vs, _mm256_cmpeq_epi32(_mm256_and_si256(v1, vm0), vz0)) ;
  vs = _mm256_add_epi32(vs, _mm256_cmpeq_epi32(_mm256_and_si256(v2, vm0), vz0)) ;
  vs = _mm256_add_epi32(vs, _mm256_cmpeq_epi32(_mm256_and_si256(v3, vm0), vz0)) ;
  vs = _mm256_add_epi32(vs, _mm256_cmpeq_epi32(_mm256_and_si256(v4, vm0), vz0)) ;
  vs = _mm256_add_epi32(vs, _mm256_cmpeq_epi32(_mm256_and_si256(v5, vm0), vz0)) ;
  vs = _mm256_add_epi32(vs, _mm256_cmpeq_epi32(_mm256_and_si256(v6, vm0), vz0)) ;
  vs = _mm256_add_epi32(vs, _mm256_cmpeq_epi32(_mm256_and_si256(v7, vm0), vz0)) ;
  v128 = _mm_add_epi32(_mm256_extractf128_si256 (vs, 0) ,  _mm256_extractf128_si256 (vs, 1)) ;
  v128 = _mm_add_epi32(v128,  _mm_shuffle_epi32(v128, 0b11101110) ) ; // [0,1,2,3] [2,2,2,3]
  v128 = _mm_add_epi32(v128,  _mm_shuffle_epi32(v128, 0b01010101) ) ; // [0,1,2,3] [1,1,1,1]
  _mm_storeu_si32(&nshort, v128) ; p.t.nshort = -nshort ;

  return encode_tile_scheme(p.u64) ;

constant:
  p.t.min = min ;
  return constant_tile_scheme(p.u64) ;

#else

  return encode_tile_properties_c(f, ni, lni, nj, tile) ; // NON SIMD (AVX2) version

#endif

}

// encode contiguous tile (1 -> 64 values) into bit stream s, using properties tp64
// tp64 [IN] : properties from encode_tile_properties
// s   [OUT] : bit stream
// tile [IN] : pre extracted tile (1->64 values)
// return total number of bits added to bit stream
// if not enough room in stream, return a negative error reflecting the number of extra bits needed
int32_t encode_contiguous(uint64_t tp64, bitstream *s, uint32_t tile[64]){
  STREAM_DCL_STATE_VARS(accum, insert, stream) ;
  STREAM_GET_INSERT_STATE(*s, accum, insert, stream) ;
  uint32_t avail ;
  tile_properties p ;
  int nij, nbits, nbits0, nbitsi, nbtot, i ;
  uint32_t min, w32, mask0, mask1, nbitsm ;
  uint32_t nzero, nshort, needed ;

  p.u64 = tp64 ;
// print_tile_properties(tp64) ;
  nij    = (p.t.h.npti + 1) * (p.t.h.nptj + 1) ;          // number of points
  nbtot = -1 ;                                            // precondition for error
  if(nij < 1 || nij > 64) goto error ;
  min    = p.t.min ;
  nzero  = p.t.nzero ;
  nshort = p.t.nshort ;
  nbits = p.t.h.nbts + 1 ;
  nbits0 = (nbits + NB0) >> 1 ;  // number of bits for "short" values

  // evaluate how many bits we may need for encoding
  needed = 16 ;                                           // header
  if(p.t.h.min0) needed += (5 + 32 - lzcnt_32(min)) ;     // in case min is used
  if(p.t.h.encd == 3) nij = 1 ;
  needed = needed + nbits0 * nshort + nbits * (nij - nzero - nshort) ;
  if(p.t.h.encd) needed += nij ;                          // one extra bit per value for encoding if used
  avail = STREAM_BITS_EMPTY(*s) ;                         // free bits in stream
//   fprintf(stderr, "worst case %d bits, %d free\n", needed, avail) ;
  nbtot = (avail - needed - 1) ;
  if(avail < needed) goto error ; 

  mask0 = RMASK31(nbits0) ;      // value & mask0 will be 0 if nbits0 or less bits are needed
  mask0 = ~mask0 ;               // keep only the upper bits
  mask1 = 1 << nbits ;           // full token flag

// fprintf(stderr, "(e_c) nbits = %d(%d), sign = %d, encoding = %d, nshort = %2d, nzero = %2d", nbits, nbits0, p.t.h.sign, p.t.h.encd, nshort, nzero) ;
// fprintf(stderr, ", nbits_full = %4d, nbits_zero = %4d, nbits_short = %4d, mask0 = %8.8x\n", nbits_full, nbits_zero, nbits_short, mask0) ;
  w32 = p.u16[3] ;               // tile header
  BE64_PUT_NBITS(accum, insert, w32, 16, stream) ;       // insert header into packed stream
  nbtot = 16 ;
  if(p.t.h.encd == 3) goto constant ;

  // will have to insert both number of bits and value
  if(p.t.h.min0){                                        // store nbitsm -1 into bit stream
    nbitsm = 32 - lzcnt_32(min) ;
    BE64_PUT_NBITS(accum, insert, nbitsm-1, 5, stream) ; // insert number of bits for min value (5 bits)
    BE64_PUT_NBITS(accum, insert, min, nbitsm, stream) ; // insert minimum value (nbitsm bits)
    nbtot += (nbitsm+5) ;
  }
  // 3 mutually exclusive alternatives (case 3 handled before)
  switch(p.t.h.encd)
  {
  case 0 :                                                      // no special encoding
    for(i=0 ; i<nij ; i++){
      BE64_PUT_NBITS(accum, insert, tile[i], nbits, stream) ;   // insert nbits bits into stream
      nbtot += nbits ;
    }
    break ;
  case 1 :                                                      // 0//short , 1//full encoding
    for(i=0 ; i<nij ; i++){
      w32 = tile[i] ;
      if((w32 & mask0) == 0){                                   // value uses nbits0 bits or less
        nbitsi = nbits0+1 ;                                     // nbits0+1 bits
      }else{
        nbitsi = nbits+1 ;                                      // nbits+1 bits
        w32 |= mask1 ;                                          // add 1 bit to identify "full" value
      }
      nbtot += nbitsi ;
      BE64_PUT_NBITS(accum, insert, w32,  nbitsi, stream) ;     // insert nbitsi bits into stream
    }
    break ;
  case 2 :                                                      // 0 , 1//full encoding
    for(i=0 ; i<nij ; i++){
      w32 = tile[i] ;
      if(w32 == 0){                                             // value is zero
        nbitsi = 1 ;                                            // 1 bit
      }else{
        nbitsi = nbits+1 ;                                      // nbits+1 bits
        w32 |= mask1 ;                                          // add 1 bit to identify "full" value
      }
      nbtot += nbitsi ;
      BE64_PUT_NBITS(accum, insert, w32,  nbitsi, stream) ;     // insert nbitsi bits into stream
    }
    break ;
  }

end:
  BE64_PUSH(accum, insert, stream) ;
  STREAM_SET_INSERT_STATE(*s, accum, insert, stream) ;
  return nbtot ;

error:
  return nbtot ;

constant:
  if(min != 0) {
    BE64_PUT_NBITS(accum, insert, min,  nbits, stream) ;     // insert nbits bits into stream
    nbtot += nbits ;
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
int32_t encode_tile(void *f, int ni, int lni, int nj, bitstream *s, uint32_t tile[64]){
  uint64_t tp64 ;
  int32_t used ;

  tp64 = encode_tile_properties(f, ni, lni, nj, tile) ;   // extract tile, compute data properties
//   print_stream_params(*s, "before tile encode", NULL) ;
  used = encode_contiguous(tp64, s, tile) ;               // encode extracted tile into bit stream
  fprintf(stderr, "used bits = %d\n", used) ;
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
      fprintf(stderr, "encoding : nbtile = %d, indexi/indexj = %d/%d, LLcorner = %8.8x\n\n", nbtile, indexi, indexj, fi[indexi+indexj]) ;
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
int32_t decode_tile(void *f, int *ni, int lni, int *nj, bitstream *s){
  int32_t *fi = (int32_t *) f ;
//   uint32_t *fu = (uint32_t *) f ;
  uint32_t fe[64] ;
  tile_header th ;
  uint32_t w32 ;
  int32_t iw32 ;
  int i, i0, j, nbits, nij, nbits0, nbtot, nbitsi, nbitsm ;
  uint32_t min ;
  STREAM_DCL_STATE_VARS(accum, xtract, stream) ;
  STREAM_GET_XTRACT_STATE(*s, accum, xtract, stream) ;

  if( (f == NULL) || (stream == NULL) ) goto error ;
  BE64_GET_NBITS(accum, xtract, w32, 16, stream) ;
  nbtot = 16 ;
  th.s = w32 ;
  *ni = th.h.npti+1 ;              // first dimension of tile
  *nj = th.h.nptj+1 ;              // second dimension of tile
  nij = (*ni) * (*nj) ;
  nbits = th.h.nbts + 1 ;
  nbits0 = (nbits + NB0) >> 1 ;    // number of bits for "short" values

  if(th.h.encd == 3) goto constant ;

  min = 0 ;
  // will have to get both number of bits and value (get nbits -1 from bit stream)
  if(th.h.min0){                   // get min value if there is one
    BE64_GET_NBITS(accum, xtract, nbitsm, 5, stream) ;      // number of bits for min value
    nbitsm++ ;                                              // restore nbitsm
    BE64_GET_NBITS(accum, xtract, min, nbitsm, stream) ;    // min value
    nbtot += (nbitsm+5) ;
  }else{
    nbitsm = 0 ;
  }

fprintf(stderr, " TILE : ni = %d, nj = %d, nbits = %d, encoding = %d, sign = %d, min0 = %d, min = %8.8x, nbitsm = %d\n", 
        *ni, *nj, nbits, th.h.encd, th.h.sign, th.h.min0, min, nbitsm) ;

  switch(th.h.encd)
  {
  case 1 :                                                 // 0//short , 1//full encoding
    for(i=0 ; i<nij ; i++){
      BE64_GET_NBITS(accum, xtract, w32, 1, stream) ;        // get 1 bit
      if(w32 != 0){
        BE64_GET_NBITS(accum, xtract, w32, nbits, stream) ;  // get nbits bits
        nbitsi = nbits + 1 ;
      }else{
        BE64_GET_NBITS(accum, xtract, w32, nbits0, stream) ; // get nbits0 bits
        nbitsi = nbits0 + 1 ;
      }
      nbtot += nbitsi ;
      fe[i] = w32 + min ;
    }
    break ;
  case 2 :                                                 // 0 , 1//full encoding
    for(i=0 ; i<nij ; i++){
      BE64_GET_NBITS(accum, xtract, w32, 1, stream) ;        // get 1 bit
      if(w32 != 0){
        BE64_GET_NBITS(accum, xtract, w32, nbits, stream) ;  // get nbits bits
        nbitsi = nbits + 1 ;
      }else{
        nbitsi = 1 ;
      }
      nbtot += nbitsi ;
      fe[i] = w32 + min ;
    }
    break ;
  case 0 :                                                 // no encoding
    for(i=0 ; i<nij ; i++){
      BE64_GET_NBITS(accum, xtract, w32, nbits, stream) ;
      fe[i] = w32 + min ;
      nbtot += nbits ;
    }
    break ;
  }

  i0 = 0 ;
  if(th.h.sign == 1){    // all values >= 0
    for(j=0 ; j<*nj ; j++){
      for(i=0 ; i<*ni ; i++){
        fi[i] = fe[i0+i] ;     // ZigZag O.K. if non negative
      }
      fi += lni ;
      i0 += *ni ;
    }
//     for(i=0 ; i<nij ; i++){ fi[i] = fe[i] + min ; }
  }
  if(th.h.sign == 2){    // all values < 0
    for(j=0 ; j<*nj ; j++){
      for(i=0 ; i<*ni ; i++){
        fi[i] = ~(fe[i0+i]) ;  // undo right shifted ZigZag
      }
      fi += lni ;
      i0 += *ni ;
    }
//     for(i=0 ; i<nij ; i++){ fi[i] = ~(fe[i] + min) ; }  // undo right shifted ZigZag
  }
  if(th.h.sign == 3){    // ZigZag
    for(j=0 ; j<*nj ; j++){
      for(i=0 ; i<*ni ; i++){
        fi[i] = from_zigzag_32((fe[i0+i])) ;  // restore from ZigZag
      }
      fi += lni ;
      i0 += *ni ;
    }
//     for(i=0 ; i<nij ; i++){ fi[i] = from_zigzag_32((fe[i]+min)) ; }  // restore from ZigZag
  }

end:
  STREAM_SET_XTRACT_STATE(*s, accum, xtract, stream) ;
  return nbtot ;

error:
  return -1 ;

constant:
  iw32 = 0 ;
  if(th.h.sign == 3){
    BE64_GET_NBITS(accum, xtract, w32, nbits, stream) ;          // extract nbits bits
    nbtot += nbits ;
    iw32 = from_zigzag_32(w32) ;
  }
fprintf(stderr, " CONSTANT : ni = %d, nj = %d, nbits = %d, encoding = %d, sign = %d, min0 = %d, value = %8.8x, nij = %d\n", 
        *ni, *nj, nbits, th.h.encd, th.h.sign, th.h.min0, iw32, nij) ;
  for(j=0 ; j<*nj ; j++){
    for(i=0 ; i<*ni ; i++){
      fi[i] = iw32 ;  // restore from ZigZag
    }
    fi += lni ;
    i0 += *ni ;
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
  int ni0, nj0, nit, njt, i0, j0, indexi, indexj, nbtile, nbtot ;
#if 0
  bitstream_state state ;
#endif

  indexj = 0 ;
  for(j0 = 0 ; j0 < nj ; j0 += 8){
    nj0 = ((nj - j0) > 8) ? 8 : (nj - j0) ;
    for(i0 = 0 ; i0 < ni ; i0 += 8){
      indexi = i0 ;
      ni0 = ((ni - i0) > 8) ? 8 : (ni - i0) ;
      print_stream_params(*s, "before tile decode", NULL) ;
      fprintf(stderr,"tile (%3d,%3d) -> (%3d,%3d) [%d x %d] [%4d,%4d]\n", i0, j0, i0+ni0-1, j0+nj0-1, ni0, nj0, indexi, indexj) ;
#if 0
StreamSaveState(s, &state) ;
print_stream_params(*s, "after save state", NULL) ;
nbtile = decode_tile(fi+indexi+indexj, &nit, lni, &njt, s) ;
print_stream_params(*s, "before restore state", NULL) ;
StreamRestoreState(s, &state, 0) ;
print_stream_params(*s, "after restore state", NULL) ;
#endif
      nbtile = decode_tile(fi+indexi+indexj, &nit, lni, &njt, s) ;
      fprintf(stderr,"nit = %d, njt = %d, nbtile = %d\n", nit, njt, nbtile) ;
      if(ni0 != nit || nj0 != njt) return -1 ;        // decoding error
      nbtot += nbtile ;
      print_stream_params(*s, "after tile decode", NULL) ;
      fprintf(stderr,"\n");
      if(nit != ni0 || njt != nj0) return 1 ;
    }
    indexj += lni*8 ;
  }
  return nbtot ;
}
