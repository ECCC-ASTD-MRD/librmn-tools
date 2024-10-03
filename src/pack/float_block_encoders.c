// Hopefully useful functions for C and FORTRAN
// Copyright (C) 2024  Recherche en Prevision Numerique
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
//     M. Valin,   Recherche en Prevision Numerique, 2024

#include <stdint.h>
#include <immintrin.h>

#include <rmn/bits.h>

// encode a 4 x 4 block of floats using nbits bits per value
// store result as sign  / delta exponent / mantissa
// delta exponent = value exponent - lowest exponent in block
// if all values have the same sign, the sign bit is not encoded, the header will provide it
// if all values have the same exponent, the exponent is not encoded, the header will provide it
// return encoding stream pointer, NULL if error
// effective dimensions of f : f[nj][lni]
// effective size of block to extract [min(nj,4)][min(ni,4)]
uint16_t *float_encode_4x4(float *f, int lni, int nbits, uint16_t *stream16, uint32_t ni, uint32_t nj, uint32_t *head){
  uint64_t *stream64 = (uint64_t *) stream16 ;
//   uint16_t *stream16 = (uint16_t *) stream ;
   __m256i vdata0, vdata1 ;
//   uint16_t *stream16 = (uint16_t *) stream32 ;
  nbits = (nbits > 23) ? 23 : nbits ;   // at most 23 bits
  nbits = (nbits <  3) ?  3 : nbits ;   // at least 3 bits
  ni = (ni > 4) ? 4 : ni ;
  nj = (nj > 4) ? 4 : nj ;

  if(ni < 4 || nj < 4){                 // NOT a full 4x4 block
fprintf(stderr, "partial block lni = %4d, ni = %d, nj=%d, stream = %p\n", lni, ni, nj, stream16) ;
    float fl[4][4] ;                    // local 4x4 block that will be used for encoding
    uint32_t i, j ;
    for(j=0 ; j<4 ; j++){               // fill local block with first value from f
      for(i=0 ; i<4 ; i++){ fl[j][i] = f[0] ; }
    }
    for(j=0 ; j<4 && j<nj ; j++){       // copy useful part of block from source array f
      for(i=0 ; i<4 && i<ni ; i++){ fl[j][i] = f[i] ; }   // copy row
      f += lni ;                                          // next row from source array f
    }
// for(j=3 ; j<4 ; j--){
// for(i=0 ; i<4 ; i++){ fprintf(stderr, "%8.2f ", fl[j][i]) ; } ; fprintf(stderr, "\n" ) ;
// }
    vdata0 = (__m256i) _mm256_loadu2_m128( &(fl[1][0]) , &(fl[0][0]) ) ;   // first 8 values (2 rows of 4)
    vdata1 = (__m256i) _mm256_loadu2_m128( &(fl[3][0]) , &(fl[2][0]) ) ;   // next 8 values (2 rows of 4)

  }else{                                // FULL 4x4 block
// float fl[4][4] ;                    // local 4x4 block that will be used for encoding
// uint32_t i, j ;
fprintf(stderr, "full block lni = %4d, ni = %d, nj=%d, stream = %p\n", lni, 4, 4, stream16) ;
    vdata0 = (__m256i) _mm256_loadu2_m128( f+lni , f ) ;   // first 8 values (2 rows of 4)
    f += (lni+lni) ;
    vdata1 = (__m256i) _mm256_loadu2_m128( f+lni , f ) ;   // next 8 values (2 rows of 4)
// _mm256_storeu_si256((__m256i *) &(fl[0][0]) , vdata0) ;
// _mm256_storeu_si256((__m256i *) &(fl[2][0]) , vdata1) ;
// for(j=3 ; j<4 ; j--){
// for(i=0 ; i<4 ; i++){ fprintf(stderr, "%8.2f ", fl[j][i]) ; } ; fprintf(stderr, "\n" ) ;
// }
  }
  // ===================================== analysis phase =====================================
  // signs == 0           means all values are >= 0
  // signs == 0xFF + 0xFF means all values are < 0
  int signs = _mm256_movemask_ps((__m256)vdata0) + _mm256_movemask_ps((__m256)vdata1) ;
  // number of bits needed for sign (0 if all signs are identical, 1 otherwise)
  int sbits = ((signs == 0) || (signs == 0xFF + 0xFF)) ? 0 : 1 ;
  // if sign bits are stored as part of the encoded value, signs becomes irrelevant
  if(sbits) signs = 0 ;
  __m256i vsign = _mm256_set1_epi32(sbits << 31) ;       // mask for sign bit

  __m256i vmaske = _mm256_set1_epi32(0x7F800000) ;       // IEEE exponent mask
  __m256i vexp0  = _mm256_and_si256(vdata0, vmaske) ;    // extract IEEE exponent
  __m256i vexp1  = _mm256_and_si256(vdata1, vmaske) ;
  __m256i vmaskm = _mm256_set1_epi32(0x007FFFFF) ;       // mantissa mask (also largest value of mantissa)
  __m256i vmant0 = _mm256_and_si256(vdata0, vmaskm) ;    // extract mantissa
  __m256i vmant1 = _mm256_and_si256(vdata1, vmaskm) ;

  __m256i vmax8  = _mm256_max_epu32(vexp0, vexp1) ;      // find largest IEEE exponent
  __m256i vmin8  = _mm256_min_epu32(vexp0, vexp1) ;      // find smallest IEEE exponent
  __m128i vmax4  = _mm_max_epu32( _mm256_extracti128_si256(vmax8,0) , _mm256_extracti128_si256(vmax8,1) ) ;
  __m128i vmin4  = _mm_min_epu32( _mm256_extracti128_si256(vmin8,0) , _mm256_extracti128_si256(vmin8,1) ) ;
  vmax4  = _mm_max_epu32( vmax4 , _mm_bsrli_si128(vmax4,8) ) ;
  vmin4  = _mm_min_epu32( vmin4 , _mm_bsrli_si128(vmin4,8) ) ;
  vmax4  = _mm_max_epu32( vmax4 , _mm_bsrli_si128(vmax4,4) ) ;
  vmin4  = _mm_min_epu32( vmin4 , _mm_bsrli_si128(vmin4,4) ) ;
  int emax = _mm_extract_epi32(vmax4, 0) >> 23 ;         // largest IEEE exponent (with bias)
  int emin = _mm_extract_epi32(vmin4, 0) >> 23 ;         // smallest IEEE exponent (with bias)
//   emax = (emax - emin) ;                                 // largest IEEE exponent difference (bits 0:7)
  int ebits = 32 -lzcnt_32(emax - emin);                 // number of bits needed for largest difference
  if(ebits > 7) return NULL ;                            // ERROR : largest value / smallest value > 2 ** 127
  int mbits = nbits - ebits - sbits ;                    // number of bits left for mantissa
  if(mbits < 0) return NULL ;                            // ERROR: not enough mantisssa bits left
  // ===================================== encoding phase =====================================
  int round = 1 << (23 - 1 - mbits) ;                    // rounding value
  __m256i vround = _mm256_set1_epi32(round) ;
  emin <<= 23 ;
  __m256i vemin = _mm256_set1_epi32(emin) ;              // smallest IEEE exponent at the proper position
  vmant0 = _mm256_add_epi32(vmant0, vround) ;            // add rounding
  vmant1 = _mm256_add_epi32(vmant1, vround) ;
  vmant0 = _mm256_min_epi32(vmant0, vmaskm) ;            // clip to 0x007FFFFF
  vmant1 = _mm256_min_epi32(vmant1, vmaskm) ;
  vexp0  = _mm256_sub_epi32(vexp0, vemin) ;              // value exponent - min exponent
  vexp1  = _mm256_sub_epi32(vexp1, vemin) ;
  vmant0 = _mm256_or_si256(vmant0, vexp0) ;              // add delta exponent to mantissa
  vmant1 = _mm256_or_si256(vmant1, vexp1) ;
  // eliminate unneeded bits at top
  __m256i vshift = _mm256_set1_epi32(9 - sbits - ebits) ;
  vmant0 = _mm256_sllv_epi32(vmant0, vshift) ;
  vmant1 = _mm256_sllv_epi32(vmant1, vshift) ;
  // add sign if needed
  vmant0 = _mm256_or_si256(vmant0, _mm256_and_si256(vdata0, vsign) ) ;
  vmant1 = _mm256_or_si256(vmant1, _mm256_and_si256(vdata1, vsign) ) ;
  // ===================================== stream packing phase =====================================
  // header (left to right) =  emin:8 , spare:3 , sign:1, sbits:1 , ebits:3
  // bit number                 15:8      7:5       4        3        2:0
  // emin  = min IEEE exponent (same as max exponent if all values have same exponent)
  // sign  = 1 if values < 0 , 0 if values >= 0 (irrelevant if sbits == 0)
  // sbits = number of bits to store sign (0 if all values have same sign)
  // ebits = number of bits for exponent - min exponent (0 if all values have same exponent)
  uint32_t header = (ebits | (sbits << 3) | (((signs > 0) ? 1 : 0) << 4) | (emin >> 15) ) ;
  // shift to lower nbits
  vshift = _mm256_set1_epi32(32 - nbits) ;
  // pack and store
//   _mm256_storeu_si256((__m256i *)(stream32 + 0) , _mm256_srlv_epi32(vmant0, vshift) ) ;
//   _mm256_storeu_si256((__m256i *)(stream32 + 8) , _mm256_srlv_epi32(vmant1, vshift) ) ;
//   goto end ;
//   stream32 += 16 ; stream64 += 8 ;
  __m256i v8i0 ;
  __m128i v4i0, v4i1 ;
  uint32_t t[4] ;
  uint64_t s ;
  vmant0 = _mm256_srlv_epi32(vmant0, vshift) ;
  vmant1 = _mm256_srlv_epi32(vmant1, vshift) ;
  switch(nbits){
    case  0:
    case  1:
    case  2:
    case  3:
      v8i0 = _mm256_or_si256( vmant0 , _mm256_slli_epi32(vmant1, 3) ) ;                     // 16 x 3 -> 8 x 6
      v4i0 = _mm256_extracti128_si256(v8i0,0) ; v4i1 = _mm256_extracti128_si256(v8i0,1) ;   // 4 x 6 , 4 x 6
      v4i0 = _mm_or_si128( v4i0 , _mm_slli_epi32(v4i1,6) ) ;                                // 4 x 12
      _mm_storeu_si128((__m128i *)t, v4i0) ;                                                // store 4 x 32 bits (12/12/12/12)
      s = header ; s = (s<<12)|t[3] ; s = (s<<12)|t[2] ; s = (s<<12)|t[1] ; s = (s<<12)|t[0] ;
      stream64[0] = s ;                                                                     // 16-12-12-12-12
      break ;
    case  4:
    case  5:
    case  6:
    case  7:
      v8i0 = _mm256_or_si256( vmant0 , _mm256_slli_epi32(vmant1, 7) ) ;                     // 16 x 7 -> 8 x 14
      v4i0 = _mm256_extracti128_si256(v8i0,0) ; v4i1 = _mm256_extracti128_si256(v8i0,1) ;   // 4 x 14 , 4 x 14
      v4i0 = _mm_or_si128( v4i0 , _mm_slli_epi32(v4i1,14) ) ;                               // 4 x 28
      _mm_storeu_si128((__m128i *)t, v4i0) ;                                                // store 4 x 32 bits (28/28/28/28)
      s = (header & 0x00FF) ; s = (s<<28) | t[1] ; s = (s<<28) | t[0] ; stream64[0] = s ;   // 8-28-28
      s = (header   >>   8) ; s = (s<<28) | t[3] ; s = (s<<28) | t[2] ; stream64[1] = s ;   // 8-28-28
      break ;
    case  8:
    case  9:
    case 10:
    case 11:
      v8i0 = _mm256_or_si256( vmant0 , _mm256_slli_epi32(vmant1,11) ) ;                     // 16 x 11 -> 8 x 22
      v4i0 = _mm256_extracti128_si256(v8i0,0) ; v4i1 = _mm256_extracti128_si256(v8i0,1) ;   // 4 x 22 , 4 x 22
      v4i0 = _mm_or_si128( v4i0 , _mm_slli_epi32(v4i1,22) ) ;                               // 4 x 22 | (4 x 10) << 22
      _mm_storeu_si128((__m128i *)(stream64), v4i0) ;                                       // store 2 x 64 bits (2 x 10-22,10-22)
      v4i1 = _mm_srli_epi32(v4i1,10) ;                                                      // 4 x 12
      _mm_storeu_si128((__m128i *)t, v4i1) ;                                                // store 4 x 12 bits (12/12/12/12)
      s = header ; s = (s<<12)|t[3] ; s = (s<<12)|t[2] ; s = (s<<12)|t[1] ; s = (s<<12)|t[0] ;
      stream64[2] = s ;                                                                     // 16-12-12-12-12
      break ;
    case 12:
    case 13:
    case 14:
    case 15:
      v8i0 = _mm256_or_si256( vmant0 , _mm256_slli_epi32(vmant1,15) ) ;                     // 16 x 15 -> 8 x 30
      v4i0 = _mm256_extracti128_si256(v8i0,0) ; v4i1 = _mm256_extracti128_si256(v8i0,1) ;   // 4 x 30 , 4 x 30
      v4i0 = _mm_or_si128( v4i0 , _mm_slli_epi32(v4i1,30) ) ;                               // 4 x 30 | (4 x 2) << 30
      _mm_storeu_si128((__m128i *)(stream64), v4i0) ;                                       // store 2 x 64 bits (2 x 2-30,2-30)
      v4i1 = _mm_srli_epi32(v4i1,2) ;                                                       // 4 x 28
      _mm_storeu_si128((__m128i *)t, v4i1) ;                                                // store 4 x 32 bits (28/28/28/28)
      s = (header & 0x00FF) ; s = (s<<28) | t[1] ; s = (s<<28) | t[0] ; stream64[2] = s ;   // 8-28-28
      s = (header   >>   8) ; s = (s<<28) | t[3] ; s = (s<<28) | t[2] ; stream64[3] = s ;   // 8-28-28
      break ;
    case 16:
    case 17:
    case 18:
    case 19:
      v8i0 = _mm256_or_si256( vmant0 , _mm256_slli_epi32(vmant1,19) ) ;                     // 8 x 19 | (8 x 13) << 19
      _mm256_storeu_si256((__m256i *)stream64, v8i0) ;                                      // store 4 x 64 bits (4 x 13-19,13-19)
      v8i0 =  _mm256_srli_epi32(vmant1,13) ;                                                // 8 x 6 (upper 6 bits)
      v4i0 = _mm256_extracti128_si256(v8i0,0) ; v4i1 = _mm256_extracti128_si256(v8i0,1) ;   // 4 x 6 , 4 x 6
      v4i0 = _mm_or_si128( v4i0 , _mm_slli_epi32(v4i1,6) ) ;                                // 4 x 12
      _mm_storeu_si128((__m128i *)t, v4i0) ;                                                // store 4 x 32 bits (12/12/12/12)
      s = header ; s = (s<<12)|t[3] ; s = (s<<12)|t[2] ; s = (s<<12)|t[1] ; s = (s<<12)|t[0] ;
      stream64[4] = s ;                                                                     // 16-12-12-12-12
      break ;
    case 20:
    case 21:
    case 22:
    case 23:
      v8i0 = _mm256_or_si256( vmant0 , _mm256_slli_epi32(vmant1,23) ) ;                     // 8 x 23 | (8 x 9) << 23
      _mm256_storeu_si256((__m256i *)stream64, v8i0) ;                                      // store 4 x 64 bits (4 x 9-23,9-23)
      v8i0 =  _mm256_srli_epi32(vmant1,9) ;                                                 // 8 x 14
      v4i0 = _mm256_extracti128_si256(v8i0,0) ; v4i1 = _mm256_extracti128_si256(v8i0,1) ;   // 4 x 14 , 4 x 14
      v4i0 = _mm_or_si128( v4i0 , _mm_slli_epi32(v4i1,14) ) ;                               // 4 x 28
      _mm_storeu_si128((__m128i *)t, v4i0) ;                                                // store 4 x 32 bits (28/28/28/28)
      s = (header & 0x00FF) ; s = (s<<28) | t[1] ; s = (s<<28) | t[0] ; stream64[4] = s ;   // 8-28-28
      s = (header   >>   8) ; s = (s<<28) | t[3] ; s = (s<<28) | t[2] ; stream64[5] = s ;   // 8-28-28
      break ;
    default:
      break ;
  }
// end:
  if(head != NULL) *head = header ;
  return  stream16 + nbits + 1 ;
}

uint16_t *float_decode_4x4(float *f, int nbits, uint16_t *stream16, uint32_t *head){
  uint64_t *stream64 = (uint64_t *) stream16 ;
  int header ;
  __m256i v8i0, v8i1, vmant0, vmant1 ;
  __m128i v4i0, v4i1 ;
  uint32_t t[4] ;

  switch(nbits){
    case  0:
    case  1:
    case  2:
    case  3:
      header = stream64[0] >> 48 ;
      t[0]   = stream64[0] & 0xFFF ;         t[1] = (stream64[0] >> 12) & 0xFFF ;   // 16-12-12-12-12 -> 12 , 12
      t[2]   = (stream64[0] >> 24) & 0xFFF ; t[3] = (stream64[0] >> 36) & 0xFFF ;   // 16-12-12-12-12 -> 12 , 12
      v4i0   = _mm_loadu_si128((__m128i *)(t)) ;                                    // 12-12 , 12-12
      v4i1   = _mm_srli_epi32(v4i0, 6) ;                                            // 4 x bits 5:0
      v4i0   = _mm_slli_epi32(v4i0,26) ; v4i0 = _mm_srli_epi32(v4i0,26) ;           // 4 x bits 5:0
      vmant0 = _mm256_setzero_si256() ;
      vmant0 = _mm256_inserti128_si256(vmant0 , v4i0, 0) ;                          // concatenate to 8 x bits 5:0
      vmant0 = _mm256_inserti128_si256(vmant0 , v4i1, 1) ;
      vmant1 = _mm256_srli_epi32(vmant0, 3) ;                                       // 8 x bits 2:0 ( 5:3 from vmant0)
      vmant0 = _mm256_slli_epi32(vmant0,29); vmant0 = _mm256_srli_epi32(vmant0,29); // 8 x bits 2:0 (lower 3 from vmant0)
      break ;
    case  4:
    case  5:
    case  6:
    case  7:
      header = ((stream64[0] >> 56) & 0xFF) | ((stream64[1] >> 48) & 0xFF00) ;
      t[0]   = stream64[0] & 0xFFFFFFF ; t[1] = (stream64[0] >> 28) & 0xFFFFFFF ;   // 8-28-28 -> 28 , 28
      t[2]   = stream64[1] & 0xFFFFFFF ; t[3] = (stream64[1] >> 28) & 0xFFFFFFF ;   // 8-28-28 -> 28 , 28
      v4i0   = _mm_loadu_si128((__m128i *)(t)) ;                                    // 4 x 32 bits (28/28/28/28)
      v4i1   = _mm_srli_epi32(v4i0,14) ;                                            // 4 x bits 13:0
      v4i0   = _mm_slli_epi32(v4i0,18) ; v4i0 = _mm_srli_epi32(v4i0,18) ;           // 4 x bits 13:0
      vmant0 = _mm256_setzero_si256() ;
      vmant0 = _mm256_inserti128_si256(vmant0 , v4i0, 0) ;                          // concatenate to 8 x bits 13:0
      vmant0 = _mm256_inserti128_si256(vmant0 , v4i1, 1) ;
      vmant1 = _mm256_srli_epi32(vmant0,7) ;                                        // 8 x bits 6:0 ( 13:7 from vmant0)
      vmant0 = _mm256_slli_epi32(vmant0,25); vmant0 = _mm256_srli_epi32(vmant0,25); // 8 x bits 6:0 (lower 7 from vmant0)
      break ;
    case  8:
    case  9:
    case 10:
    case 11:
      header = stream64[2] >> 48 ;
      t[0]   = stream64[2] & 0xFFF ;         t[1] = (stream64[2] >> 12) & 0xFFF ;   // 16-12-12-12-12 -> 12 , 12
      t[2]   = (stream64[2] >> 24) & 0xFFF ; t[3] = (stream64[2] >> 36) & 0xFFF ;   // 16-12-12-12-12 -> 12 , 12
      v4i0   = _mm_loadu_si128((__m128i *)(stream64)) ;                             // 2 x 64 bits (2 x 10-22,10-22)
      v4i1   = _mm_loadu_si128((__m128i *)(t)) ;                                    // 4 x 32 bits (12/12/12/12)
      v4i1   = _mm_slli_epi32(v4i1,10) ;                                            // bits 21:12 (4 top values)
      v4i1   = _mm_or_si128(v4i1, _mm_srli_epi32(v4i0,22)) ;                        // add bits 11:0 (from v4i0)
      vmant0 = _mm256_setzero_si256() ;
      vmant0 = _mm256_inserti128_si256(vmant0 , v4i0, 0) ;                          // concatenate to 8 x bits 10:0
      vmant0 = _mm256_inserti128_si256(vmant0 , v4i1, 1) ;
      vmant1 = _mm256_srli_epi32(vmant0, 11) ;
      vmant0 = _mm256_slli_epi32(vmant0,11) ; vmant0 = _mm256_srli_epi32(vmant0,11) ;
      break ;
    case 12:
    case 13:
    case 14:
    case 15:
      header = ((stream64[2] >> 56) & 0xFF) | ((stream64[3] >> 48) & 0xFF00) ;
      t[0]   = stream64[2] & 0xFFFFFFF ; t[1] = (stream64[2] >> 28) & 0xFFFFFFF ;   // 8-28-28 -> 28 , 28
      t[2]   = stream64[3] & 0xFFFFFFF ; t[3] = (stream64[3] >> 28) & 0xFFFFFFF ;   // 8-28-28 -> 28 , 28
      v4i0   = _mm_loadu_si128((__m128i *)(stream64)) ;                             // 2 x 64 bits (2 x 2-30,2-30)
      v4i1   = _mm_loadu_si128((__m128i *)(t)) ;                                    // 4 x 32 bits (28/28/28/28)
      v4i1   = _mm_slli_epi32(v4i1, 2) ;                                            // left aligned
      v4i1   = _mm_or_si128(v4i1, _mm_srli_epi32(v4i0,30) ) ;                       // 4 x bits 29:0
      v4i0   = _mm_slli_epi32(v4i0, 2) ; v4i0   = _mm_srli_epi32(v4i0, 2) ;         // 4 x bits 29:0
      vmant0 = _mm256_setzero_si256() ;
      vmant0 = _mm256_inserti128_si256(vmant0 , v4i0, 0) ;                          // concatenate to 8 x bits 29:0
      vmant0 = _mm256_inserti128_si256(vmant0 , v4i1, 1) ;
      vmant1 = _mm256_srli_epi32(vmant0, 15) ;                                      // bits 14:0 next 8 values
      vmant0 = _mm256_slli_epi32(vmant0,17); vmant0 = _mm256_srli_epi32(vmant0,17); // bits 14:0 first 8 values
      break ;
    case 16:
    case 17:
    case 18:
    case 19:
      header = stream64[4] >> 48 ;                                                  // 16-12-12-12-12 -> 16 bits header
      t[0]   = stream64[4] & 0xFFF ;         t[1] = (stream64[4] >> 12) & 0xFFF ;   // 16-12-12-12-12 -> 12 , 12
      t[2]   = (stream64[4] >> 24) & 0xFFF ; t[3] = (stream64[4] >> 36) & 0xFFF ;   // 16-12-12-12-12 -> 12 , 12
      v8i0   = _mm256_loadu_si256((__m256i *)(stream64+0)) ;
      vmant1 = _mm256_srli_epi32(v8i0,19) ;                                         // upper 13 bits -> lower 13 bits
      v8i0   = _mm256_slli_epi32(v8i0,13) ;                                         // remove upper 13 bits
      vmant0 = _mm256_srli_epi32(v8i0,13) ;
      v4i0   = _mm_loadu_si128((__m128i *)(t)) ;                                    // 12-12 , 12-12
      v4i1   = _mm_srli_epi32(v4i0,6) ; v4i1 = _mm_slli_epi32(v4i1, 13) ;           // 4 x bits 18:13
      v4i0   = _mm_slli_epi32(v4i0,26) ; v4i0 = _mm_srli_epi32(v4i0,13) ;           // 4 x bits 18:13
      v8i1   = _mm256_inserti128_si256(v8i0 , v4i0, 0) ;                            // concatenate to 8 x bits 18:13
      v8i1   = _mm256_inserti128_si256(v8i1 , v4i1, 1) ;
      vmant1 = _mm256_or_si256(vmant1, v8i1) ;                                      // or bits 12:0
      break ;
    case 20:
    case 21:
    case 22:
    case 23:
      header = ((stream64[4] >> 56) & 0xFF) | ((stream64[5] >> 48) & 0xFF00) ;      // 8-28-28 8-28-28 -> 16 bits header
      t[0]   = stream64[4] & 0xFFFFFFF ; t[1] = (stream64[4] >> 28) & 0xFFFFFFF ;   // 8-28-28 -> 28 , 28
      t[2]   = stream64[5] & 0xFFFFFFF ; t[3] = (stream64[5] >> 28) & 0xFFFFFFF ;   // 8-28-28 -> 28 , 28
      v8i0   = _mm256_loadu_si256((__m256i *)(stream64+0)) ;
      vmant1 = _mm256_srli_epi32(v8i0,23) ;                                         // upper 9 bits -> lower 9 bits
      v8i0   = _mm256_slli_epi32(v8i0, 9) ;                                         // remove upper 9 bits
      vmant0 = _mm256_srli_epi32(v8i0, 9) ;                                         // bits 22:0 left
      v4i0   = _mm_loadu_si128((__m128i *)(t)) ;                                    // 28-28 , 28-28
      v4i1   = _mm_srli_epi32(v4i0,14) ; v4i1 = _mm_slli_epi32(v4i1, 9) ;           // 4 x bits 22:9
      v4i0   = _mm_slli_epi32(v4i0,18) ; v4i0 = _mm_srli_epi32(v4i0, 9) ;           // 4 x bits 22:9
      v8i1   = _mm256_inserti128_si256(v8i0 , v4i0, 0) ;                            // concatenate to 8 x bits 22:9
      v8i1   = _mm256_inserti128_si256(v8i1 , v4i1, 1) ;
      vmant1 = _mm256_or_si256(vmant1, v8i1) ;                                      // or bits 8:0
      break ;
    default:
      // we should never get here
      header = 0 ;
      vmant0 = _mm256_setzero_si256() ;
      vmant1 = _mm256_setzero_si256() ;
      break ;
  }
// with most compilers, the SIMD version does not perform much better than the purely scalar one
// uncomment the following line to use scalar version
// #define USE_SCALAR_DECODE
#if defined(USE_SCALAR_DECODE)
  uint32_t stream32b[16] ;
  _mm256_storeu_si256((__m256i *)(stream32b+0), vmant0) ;    // first 8 values
  _mm256_storeu_si256((__m256i *)(stream32b+8), vmant1) ;    // next 8 values
#endif

  nbits = (nbits > 23) ? 23 : nbits ;
  uint32_t *fi = (uint32_t *) f ;
  int ebits = (header &  7) ;
  int sbits = (header >> 3) & 1 ;
  int emin  = (header >> 8) ;
  int esign = (header >> 4) & 1 ;
  int mbits = nbits - sbits - ebits ;
  uint32_t maskm = ~((~0u) << mbits) ;                    // mantissa mask
  uint32_t maske = (ebits == 0) ? 0 : ~((~0u) << ebits) ; // exponent mask
  int sign  = (sbits == 0) ? esign : 0 ;                  // 1 if sign bit is 1 everywhere, 0 otherwise

#if ! defined(USE_SCALAR_DECODE)
  __m256i vs, vt, ve ;
  __m256i vmaskm  = _mm256_set1_epi32(maskm) ;
  __m256i vmaske  = _mm256_set1_epi32(maske) ;
  __m256i vshift1 = _mm256_set1_epi32(23 - mbits) ;
  __m256i vmbits  = _mm256_set1_epi32(mbits) ;
  __m256i vemin   = _mm256_set1_epi32(emin) ;
  __m256i vebits  = _mm256_set1_epi32(ebits) ;
  __m256i vsign   = _mm256_set1_epi32(sign) ;

    vs = vmant0 ;
    vt = _mm256_and_si256(vs, vmaskm) ; vt = _mm256_sllv_epi32(vt, vshift1) ;
    vs = _mm256_srlv_epi32(vs, vmbits) ;
    ve = _mm256_add_epi32( vemin , _mm256_and_si256(vs , vmaske) ) ;
    vt = _mm256_or_si256(vt , _mm256_slli_epi32(ve , 23) ) ;
    vs = _mm256_srlv_epi32(vs, vebits) ;
    vs = _mm256_or_si256(vs, vsign) ;
    vt = _mm256_or_si256(vt , _mm256_slli_epi32(vs, 31) ) ;
    _mm256_storeu_si256((__m256i *)(fi+0) , vt) ;

    vs = vmant1 ;
    vt = _mm256_and_si256(vs, vmaskm) ; vt = _mm256_sllv_epi32(vt, vshift1) ;
    vs = _mm256_srlv_epi32(vs, vmbits) ;
    ve = _mm256_add_epi32( vemin , _mm256_and_si256(vs , vmaske) ) ;
    vt = _mm256_or_si256(vt , _mm256_slli_epi32(ve , 23) ) ;
    vs = _mm256_srlv_epi32(vs, vebits) ;
    vs = _mm256_or_si256(vs, vsign) ;
    vt = _mm256_or_si256(vt , _mm256_slli_epi32(vs, 31) ) ;
    _mm256_storeu_si256((__m256i *)(fi+8) , vt) ;
#else
  int i ;
  for(i=0 ; i<16 ; i++){
    uint32_t s = stream32b[i] ;
    uint32_t t = (s & maskm) << (23 - mbits) ;
    s >>= mbits ;
    uint32_t e = emin + (s & maske) ;
    t |= (e << 23) ;
    s >>= ebits ;
    s |= sign ;
    t |= (s << 31) ;
    fi[i] = t ;
  }
#endif
  if(head) *head = header ;
  return stream16 + nbits + 1 ;
}
