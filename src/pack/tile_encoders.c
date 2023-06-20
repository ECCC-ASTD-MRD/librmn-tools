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

void print_tile_properties(uint64_t p64){
  tile_properties p ;

  p.u64 = p64 ;
  fprintf(stderr, "nbits = %d, sign = %d, encoding = %d, %d x %d, min0 = %d, min = %8.8x, nzero = %3d, nshort = %3d\n\n",
          p.t.h.nbts, p.t.h.sign, p.t.h.encd, p.t.h.npti+1, p.t.h.nptj+1, p.t.h.min0, p.t.min, p.t.nzero, p.t.nshort) ;
}

// gather tile to be encoded, determine useful data properties
// f       : array from which block will be extracted
// ni      : first dimension of tile to be encoded (1 <= ni <= 8)
// lni     : row storage length in array f
// nj      : second dimension of tile to be encoded (1 <= nj <= 8)
// tile    : extracted tile (in sign/magnitude form and with minimum value possibly subtracted)
// the function returns a tile parameter structure as an "opaque" 64 bit value
// this value will then be passed to the encoder itself
// generic, NON SIMD version
uint64_t encode_tile_properties_(void *f, int ni, int lni, int nj, uint32_t tile[64]){
  uint32_t *block = (uint32_t *)f ;
  uint32_t pos, neg, nshort, nzero ;
  uint32_t max, min, nbits, nbits0, mask0, mask1 ;
  tile_properties p ;
  int32_t i, j, i0, nij ;

  p.u64 = 0 ;
  p.t.h.npti = ni - 1 ;
  p.t.h.nptj = nj - 1;
  i0 = 0 ;
  nij = ni * nj ;
  for(j=0 ; j<nj ; j++){
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
  nbits = 32 - lzcnt_32(max) ;               // max controls nbits (if vmax is 0, nbits will be 0)
  if( (32 - (_lzcnt_u32(max-min)) < nbits) || 1 ){   // we gain one bit if subtracting the minimum value
// fprintf(stderr, "(%dx%d) nbits original = %d, max = %8.8x, min = %8.8x", ni, nj, nbits, max, min) ;
    max = max - min ;
// fprintf(stderr, ", new max = %8.8x\n", max) ;
    for(i=0 ; i<nij ; i++) tile[i] -= min ;
    p.t.h.min0 = 1 ;            // min will be used
  }else{
    p.t.h.min0 = 0 ;            // min will not be used
  }
  nbits = 32 - _lzcnt_u32(max) ;

  neg >>= 31 ;                               // 1 if all values < 0
  pos >>= 31 ;                               // 1 if all values >= 0
  if(neg || pos) {                           // no need to encode sign, it is the same for all values
    nbits-- ;                                // and will be found in the tile header
    min >>= 1 ;
    for(i=0 ; i<nij ; i++)
      tile[i] >>= 1 ;                        // get rid of sign bit from ZigZag
    p.t.h.sign = (neg << 1) | pos ;
  }else{
    p.t.h.sign = 3 ;                         // both positive and negative values, ZigZag encoding with sign bit
  }
  if(min == 0) p.t.h.min0 = 0 ;
  p.t.min = min ;
  p.t.h.nbts = nbits ;
  p.t.h.encd = 0 ;
  nbits0 = (nbits + 1) >> 1 ;    // number of bits for "short" values
  mask0 = RMASK31(nbits0) ;      // value & mask0 will be 0 if nbits0 or less bits are needed
  mask0 = ~mask0 ;               // keep only the upper bits
  mask1 = 1 << nbits ;           // full token flag

  nshort = nzero = 0 ;
  for(i=0 ; i<nij ; i++){
    if(tile[i] == 0)           nzero++ ;        // zero value
    if((tile[i] & mask0) == 0) nshort++ ;       // value using nbits0 bits or less
  }
  p.t.nzero  = nzero ;
  p.t.nshort = nshort ;

  return p.u64 ;
}

// gather tile to be encoded, determine useful data properties
// f       : array from which block will be extracted
// ni      : first dimension of tile to be encoded
// lni     : row storage length in array f
// nj      : second dimension of tile to be encoded
// tile    : extracted tile (in sign/magnitude form and with minimum value possibly subtracted)
// the function returns a tile parameter structure as an "opaque" 64 bit value
// this value will then be passed to the encoder itself
// X86_64 AVX2 version, 8 x 8 tiles only (ni ==8, nj == 8)
uint64_t encode_tile_properties(void *f, int ni, int lni, int nj, uint32_t tile[64]){
  uint32_t *block = (uint32_t *)f ;
  int32_t pos, neg, nshort, nzero ;
  uint32_t max, min, nbits, nbits0, mask0 ;
  tile_properties p ;

  if(ni != 8 || nj != 8) return encode_tile_properties_(f, ni, lni, nj, tile) ; // not a full 8x8 tile

#if defined(__x86_64__) && defined(__AVX2__) && defined(WITH_SIMD)
  __m256i v0, v1, v2, v3, v4, v5, v6, v7 ;
  __m256i vp, vn, vz, vs, vm0, vmax, vmin, vz0 ;
  __m128i v128 ;

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
  // subtract min from max and vo -> v7
  // set .min0 to 1
  nbits = 32 - _lzcnt_u32(max) ;
  if( (32 - (_lzcnt_u32(max-min)) < nbits) || 1 ){   // we gain one bit if subtracting the minimum value
// fprintf(stderr, "[8x8] nbits original = %d, max = %8.8x, min = %8.8x", nbits, max, min) ;
    max = max - min ;
// fprintf(stderr, ", new max = %8.8x\n", max) ;
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
  nbits = 32 - _lzcnt_u32(max) ;

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
  p.t.h.nbts = nbits ;
  p.t.h.encd = 0 ;
  nbits0 = (nbits + 1) >> 1 ;       // number of bits for "short" values
  mask0 = RMASK31(nbits0) ;         // value & ~mask0 will be 0 if nbits0 or less bits are needed
  mask0 = ~mask0 ;                  // keep only the upper bits
  vm0 = _mm256_set1_epi32(mask0) ;  // vector of mask0
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

  return p.u64 ;
#else
  return encode_tile_properties_(f, ni, lni, nj, tile) ; // NON SIMD (AVX2) version
#endif
}

// encode tile (1 -> 64 values) into bit stream s, using properties p64
int32_t encode_tile_(uint32_t tile[64], bitstream *s, uint64_t tp64){
  tile_properties p ;
  int ni, nj ;

  p.u64 = tp64 ;
  print_tile_properties(tp64) ;
  ni = p.t.h.npti - 1 ;
  nj = p.t.h.nptj - 1 ;
}

int32_t encode_tile(void *f, int ni, int lni, int nj, bitstream *s, uint32_t fe[64]){
  int32_t *fi = (int32_t *) f ;
  uint32_t *fu = (uint32_t *) f ;
//   uint32_t fe[64] ;
  int i, nbtot, nbits, nbits0, nbitsi, nshort, nzero, nbits_short, nbits_zero, nbits_full ;
  int nij = ni * nj ;
  tile_header th ;
  uint32_t vneg, vpos, vmax, vmin, w32, mask0, mask1 ;
  uint64_t accum   = s->acc_i ;
  int32_t  insert  = s->insert ;
  uint32_t *stream = s->in ;

  nbtot = 0 ;
  if( (ni < 1) || (nj < 1) || (ni > 8) || (nj > 8) ) goto error ;
  if( (f == NULL) || (s == NULL) ) goto error ;

  th.s = 0 ;                     // set all header fields to 0
  th.h.npti = ni-1 ;             // block dimensions (1->8)
  th.h.nptj = nj-1 ;
  // ==================== analysis phase ====================
  // can be accelerated using SIMD instructions
  vneg = 0xFFFFFFFFu ;
  vpos = 0xFFFFFFFFu ;
  vmin = 0xFFFFFFFFu ;
  vmax = 0 ;
  for(i=0 ; i<nij ; i++){
    vneg &= fu[i] ;              // MSB will be 1 only if all values < 0
    vpos &= (~fu[i]) ;           // MSB will be 1 only if all values >= 0
    fe[i] = to_zigzag_32(fi[i]) ;
    vmax = (fe[i] > vmax) ? fe[i] : vmax ;  // largest ZigZag value
    vmin = (fe[i] < vmin) ? fe[i] : vmin ;  // smallest ZigZag value
  }
  nbits = 32 - lzcnt_32(vmax) ;  // vmax controls nbits (if vmax is 0, nbits will be 0)
  if(vmax == 0) goto zero ;      // special case (block only contains 0s)
  vneg >>= 31 ;                  // 1 if all values < 0
  vpos >>= 31 ;                  // 1 if all values >= 0
  if(vneg || vpos) {             // no need to encode sign, it is the same for all values
    nbits-- ;                    // and will be found in the tile header
    for(i=0 ; i<nij ; i++)
      fe[i] >>= 1 ;              // get rid of sign bit from ZigZag
    th.h.sign = (vneg << 1) | vpos ;
  }else{
    th.h.sign = 3 ;              // both positive and negative values, ZigZag encoding with sign bit
  }
  nbits0 = (nbits + 1) >> 1 ;    // number of bits for "short" values
  mask0 = RMASK31(nbits0) ;      // value & mask0 will be 0 if nbits0 or less bits are needed
  mask0 = ~mask0 ;               // keep only the upper bits
  mask1 = 1 << nbits ;           // full token flag
fprintf(stderr, "vmax = %8.8x(%8.8x), vneg = %d, vpos = %d, nbits = %d, nbits0 = %d\n", vmax, vmax>>1, vneg, vpos, nbits, nbits0) ;
  th.h.nbts = nbits ;            // number of bits neeeded for encoding
  th.h.encd = 0 ;                // a priori no special encoding
  // check if we should use short/full or 0/full encoding
  nshort = nzero = 0 ;
  for(i=0 ; i<nij ; i++){
    if(fe[i] == 0)           nzero++ ;        // zero value
    if((fe[i] & mask0) == 0) nshort++ ;       // value using nbits0 bits or less
  }
  // ==================== encoding phase ====================
  nbits_full  = nij * nbits ;                                            // all tokens at full length
  nbits_zero  = nzero + (nij - nzero) * (nbits + 1) ;                    // nzero tokens at length 1, others nbits + 1
  nbits_short = nshort * (nbits0 + 1) + (nij - nshort) * (nbits + 1) ;   // short tokens tokens nbits0 + 1, others nbits + 1
  if( (nbits_short <= nbits_zero)  && (nbits_short < nbits_full) ) th.h.encd = 1 ;  // 0 short / 1 full encoding
  if( (nbits_zero  <= nbits_short) && (nbits_zero  < nbits_full) ) th.h.encd = 2 ;  // 0 / 1 full encoding
  //
th.h.encd = 0 ; // force no encoding
th.h.encd = 2 ; // force 0 , 1//full encoding
th.h.encd = 1 ; // force 0//short , 1//full encoding
  // totally scalar
  w32 = th.s ;
fprintf(stderr, "sign = %d, encoding = %d, nshort = %2d, nzero = %2d\n", th.h.sign, th.h.encd, nshort, nzero) ;
fprintf(stderr, "nbits_full = %4d, nbits_zero = %4d, nbits_short = %4d, mask0 = %8.8x\n", nbits_full, nbits_zero, nbits_short, mask0) ;
  BE64_PUT_NBITS(accum, insert, w32, 16, stream) ; // insert header into packed stream
  nbtot = 16 ;
  // 3 mutually exclusive alternatives
  if(th.h.encd == 1){             // 0//short , 1//full encoding
    for(i=0 ; i<nij ; i++){
      w32 = fe[i] ;
      if((w32 & mask0) == 0){                                   // value uses nbits0 bits or less
        nbitsi = nbits0+1 ;                                     // nbits0+1 bits
      }else{
        nbitsi = nbits+1 ;                                      // nbits+1 bits
        w32 |= mask1 ;                                          // add 1 bit to identify "full" value
      }
      nbtot += nbitsi ;
      BE64_PUT_NBITS(accum, insert, w32,  nbitsi, stream) ;     // insert nbitsi bits into stream
    }
    goto end ;
  }
  if(th.h.encd == 2){             // 0 , 1//full encoding
    for(i=0 ; i<nij ; i++){
      w32 = fe[i] ;
      if(w32 == 0){                                             // value is zero
        nbitsi = 1 ;                                            // 1 bit
      }else{
        nbitsi = nbits+1 ;                                      // nbits+1 bits
        w32 |= mask1 ;                                          // add 1 bit to identify "full" value
      }
      nbtot += nbitsi ;
      BE64_PUT_NBITS(accum, insert, w32,  nbitsi, stream) ;     // insert nbitsi bits into stream
    }
    goto end ;
  }
  if(th.h.encd == 0){             // no encoding
    for(i=0 ; i<nij ; i++){
      BE64_PUT_NBITS(accum, insert, fe[i], nbits, stream) ;       // insert nbits bits into stream
      nbtot += nbits ;
    }
    goto end ;
  }

end:
  BE64_PUSH(accum, insert, stream) ;
  s->acc_i  = accum ;
  s->insert = insert ;
  s->in     = stream ;
  return nbtot ;

error:
  nbtot = -1 ;
  return nbtot ;

zero:
  th.h.nbts = 0 ;   // 1 bit
  th.h.encd = 3 ;   // constant block
  th.h.sign = 1 ;   // all values >= 0
  w32 = th.s ;
  w32 <<= 1 ;       // header + 1 bit with value 0
  BE64_PUT_NBITS(accum, insert, w32, 17, stream) ; // insert header and 0 into packed stream
  nbtot = 17 ;      // total of 17 bits inserted
  goto end ;
}

// encode 64 values (eventually SIMD version)
int32_t encode_tile_8x8(void *f, int lni, bitstream *stream, uint32_t fe[64]){
  return encode_tile(f, 8, lni, 8, stream, fe) ;
}

// decode 64 values (SIMD version)
int32_t decode_tile_8x8(void *f, int lni, bitstream *stream, uint16_t h16){
  tile_header th ;
  th.s = h16 ;
  return 0 ;
}

int32_t decode_tile(void *f, int *ni, int lni, int *nj, bitstream *s){
  int32_t *fi = (int32_t *) f ;
//   uint32_t *fu = (uint32_t *) f ;
  uint32_t fe[64] ;
  tile_header th ;
  uint32_t w32 ;
  int i, nbits, nij, nbits0 ;
  uint64_t accum   = s->acc_x ;
  int32_t  xtract  = s->xtract ;
  uint32_t *stream = s->out ;
  uint32_t min ;

  if( (f == NULL) || (stream == NULL) ) goto error ;
  BE64_GET_NBITS(accum, xtract, w32, 16, stream) ;
  th.s = w32 ;
  *ni = th.h.npti+1 ;
  *nj = th.h.nptj+1 ;
  nij = (*ni) * (*nj) ;
  nbits = th.h.nbts ;
  nbits0 = (nbits + 1) >> 1 ;    // number of bits for "short" values
fprintf(stderr, "ni = %d, nj = %d, nbits = %d, encoding = %d, sign = %d\n", *ni, *nj, nbits, th.h.encd, th.h.sign) ;

  if(th.h.encd == 3) goto constant ;

//   BeStreamXtract(stream, fe, nbits, nij) ;   // extract nij tokens
  if(th.h.encd == 1){             // 0//short , 1//full encoding
    for(i=0 ; i<nij ; i++){
      BE64_GET_NBITS(accum, xtract, w32, 1, stream) ;        // get 1 bit
      if(w32 != 0){
        BE64_GET_NBITS(accum, xtract, w32, nbits, stream) ;  // get nbits bits
      }else{
        BE64_GET_NBITS(accum, xtract, w32, nbits0, stream) ; // get nbits0 bits
      }
      fe[i] = w32 ;
    }
    goto transfer ;
  }
  if(th.h.encd == 2){             // 0 , 1//full encoding
    for(i=0 ; i<nij ; i++){
      BE64_GET_NBITS(accum, xtract, w32, 1, stream) ;        // get 1 bit
      if(w32 != 0){
        BE64_GET_NBITS(accum, xtract, w32, nbits, stream) ;  // get nbits bits
      }
      fe[i] = w32 ;
    }
    goto transfer ;
  }
  if(th.h.encd == 0){             // no encoding
    for(i=0 ; i<nij ; i++){
      BE64_GET_NBITS(accum, xtract, fe[i], nbits, stream) ;
    }
    goto transfer ;
  }
transfer:
  if(th.h.sign == 1){    // all values >= 0
    for(i=0 ; i<nij ; i++){ fi[i] = fe[i] ; }
  }
  if(th.h.sign == 2){    // all values < 0
    for(i=0 ; i<nij ; i++){ fi[i] = ~fe[i] ; }  // undo right shifted ZigZag
  }
  if(th.h.sign == 3){    // ZigZag
    for(i=0 ; i<nij ; i++){ fi[i] = from_zigzag_32(fe[i]) ; }
  }

end:
  s->acc_x  = accum ;
  s->xtract = xtract ;
  s->out    = stream ;
  return 0 ;

error:
  return -1 ;

constant:
  BE64_GET_NBITS(accum, xtract, w32, nbits, stream) ;            // extract nbits bits
  for(i=0 ; i<nij ; i++){ fi[i] = w32 ; }                        // plus constant value
  goto end ;
}

