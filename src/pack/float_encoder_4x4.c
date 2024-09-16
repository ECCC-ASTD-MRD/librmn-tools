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
// return encoding header if successful, -1 if error
int32_t float_encode_4x4(float *f, int lni, int nbits, uint32_t *stream){
  nbits = (nbits > 23) ? 23 : nbits ;   // at most 23 bits
  nbits = (nbits <  3) ?  3 : nbits ;   // at least 3 bits
  __m256i vdata0 = (__m256i) _mm256_loadu2_m128( f+lni , f ) ;   // first 8 values (2 rows of 4)
  f += (lni+lni) ;
  __m256i vdata1 = (__m256i) _mm256_loadu2_m128( f+lni , f ) ;   // next 8 values (2 rows of 4)
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
  if(ebits > 7) return -1 ;                              // ERROR : largest value / smallest value > 2 ** 127
  int mbits = nbits - ebits - sbits ;                    // number of bits left for mantissa
  if(mbits < 0) return -1 ;                              // ERROR: not enough mantisssa bits left
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
  __m256i vshift = _mm256_set1_epi32(9 -sbits - ebits) ;
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
  _mm256_storeu_si256((__m256i *)(stream + 0) , _mm256_srlv_epi32(vmant0, vshift) ) ;
  _mm256_storeu_si256((__m256i *)(stream + 8) , _mm256_srlv_epi32(vmant1, vshift) ) ;
//   goto end ;
  stream += 16 ;
  __m256i v8i0 ;
  __m128i v4i0, v4i1 ;
  uint32_t t32[4] ;
  switch(nbits){
    case  0:
    case  1:
    case  2:
    case  3:
      v8i0 = _mm256_or_si256( vmant0 , _mm256_slli_epi32(vmant1, 3) ) ;                     // 16 x 3 -> 8 x 6
      v4i0 = _mm256_extracti128_si256(v8i0,0) ; v4i1 = _mm256_extracti128_si256(v8i0,1) ;   // 4 x 6 , 4 x 6
      v4i0 = _mm_or_si128( v4i0 , _mm_slli_epi32(v4i1,6) ) ;                                // 4 x 12
      _mm_storeu_si128((__m128i *)t32, v4i0) ;                                              // store 4 words
      stream[ 0] = t32[0] + (t32[1] << 12) ;                                                 // 24 bits
      stream[ 1] = t32[2] + (t32[3] << 12) ;                                                 // 24 bits
      break ;
    case  4:
    case  5:
    case  6:
    case  7:
      v8i0 = _mm256_or_si256( vmant0 , _mm256_slli_epi32(vmant1, 7) ) ;                     // 16 x 7 -> 8 x 14
      v4i0 = _mm256_extracti128_si256(v8i0,0) ; v4i1 = _mm256_extracti128_si256(v8i0,1) ;   // 4 x 14 , 4 x 14
      v4i0 = _mm_or_si128( v4i0 , _mm_slli_epi32(v4i1,14) ) ;                               // 4 x 28
      _mm_storeu_si128((__m128i *)t32, v4i0) ;
      stream[ 0] = t32[0] + (t32[1] << 28) ;                                                // 32 bits
      stream[ 1] = (t32[1] >> 4) ;                                                          // 24 bits
      stream[ 2] = t32[2] + (t32[3] << 28) ;                                                // 32 bits
      stream[ 3] = (t32[3] >> 4) ;                                                          // 24 bits
      break ;
    case  8:
    case  9:
    case 10:
    case 11:
      v8i0 = _mm256_or_si256( vmant0 , _mm256_slli_epi32(vmant1,11) ) ;                     // 16 x 11 -> 8 x 22
      v4i0 = _mm256_extracti128_si256(v8i0,0) ; v4i1 = _mm256_extracti128_si256(v8i0,1) ;   // 4 x 22 , 4 x 22
      v4i0 = _mm_or_si128( v4i0 , _mm_slli_epi32(v4i1,22) ) ;                               // 4 x 22 | (4 x 10) << 22
      _mm_storeu_si128((__m128i *)(stream), v4i0) ;                                         // store 4 words
      v4i1 = _mm_srli_epi32(v4i1,10) ;                                                      // 4 x 12
      _mm_storeu_si128((__m128i *)t32, v4i0) ;
      stream[ 4] = t32[0] + (t32[1] << 12) ;
      stream[ 5] = t32[2] + (t32[3] << 12) ;
      break ;
    case 12:
    case 13:
    case 14:
    case 15:
      v8i0 = _mm256_or_si256( vmant0 , _mm256_slli_epi32(vmant1,15) ) ;                     // 16 x 15 -> 8 x 30
      v4i0 = _mm256_extracti128_si256(v8i0,0) ; v4i1 = _mm256_extracti128_si256(v8i0,1) ;   // 4 x 30 , 4 x 30
      v4i0 = _mm_or_si128( v4i0 , _mm_slli_epi32(v4i1,30) ) ;                               // 4 x 30 | (4 x 2) << 30
      _mm_storeu_si128((__m128i *)(stream), v4i0) ;                                         // store 4 words
      v4i1 = _mm_srli_epi32(v4i1,2) ;                                                       // 4 x 28
      _mm_storeu_si128((__m128i *)t32, v4i0) ;
      stream[ 4] = t32[0] + (t32[1] << 28) ;
      stream[ 5] = (t32[1] >> 4) ;
      stream[ 6] = t32[2] + (t32[3] << 28) ;
      stream[ 7] = (t32[3] >> 4) ;
      break ;
    case 16:
    case 17:
    case 18:
    case 19:
      v8i0 = _mm256_or_si256( vmant0 , _mm256_slli_epi32(vmant1,19) ) ;                     // 8 x 19 | (8 x 13) >> 19
      _mm256_storeu_si256((__m256i *)stream, v8i0) ;                                        // store 8 words
      v8i0 =  _mm256_srli_epi32(vmant1,13) ;                                                // 8 x 6
      v4i0 = _mm256_extracti128_si256(v8i0,0) ; v4i1 = _mm256_extracti128_si256(v8i0,1) ;   // 4 x 6 , 4 x 6
      v4i0 = _mm_or_si128( v4i0 , _mm_slli_epi32(v4i1,6) ) ;                                // 4 x 12
      _mm_storeu_si128((__m128i *)t32, v4i0) ;
      stream[ 8] = t32[0] + (t32[1] << 12) ;                                                 // 24 bits
      stream[ 9] = t32[2] + (t32[3] << 12) ;                                                 // 24 bits
      break ;
    case 20:
    case 21:
    case 22:
    case 23:
      v8i0 = _mm256_or_si256( vmant0 , _mm256_slli_epi32(vmant1,23) ) ;                     // 8 x 23 | (8 x 9) << 23
      _mm256_storeu_si256((__m256i *)stream, v8i0) ;                                        // store 8 words
      v8i0 =  _mm256_srli_epi32(vmant1,9) ;                                                 // 8 x 14
      v4i0 = _mm256_extracti128_si256(v8i0,0) ; v4i1 = _mm256_extracti128_si256(v8i0,1) ;   // 4 x 14 , 4 x 14
      v4i0 = _mm_or_si128( v4i0 , _mm_slli_epi32(v4i1,14) ) ;                               // 4 x 28
      _mm_storeu_si128((__m128i *)t32, v4i0) ;
      stream[ 8] = t32[0] + (t32[1] << 28) ;                                                // 32 bits
      stream[ 9] = (t32[1] >> 4) ;                                                          // 24 bits
      stream[10] = t32[2] + (t32[3] << 28) ;                                                // 32 bits
      stream[11] = (t32[3] >> 4) ;                                                          // 24 bits
      break ;
    default:
      break ;
  }
end:
  return  header ;
}

void float_decode_4x4(float *f, int nbits, uint32_t *stream, int header){
  nbits = (nbits > 23) ? 23 : nbits ;
  uint32_t *fi = (uint32_t *) f ;
  int i ;
  int ebits = (header & 7) ;
  int sbits = (header >> 3) & 1 ;
  int emin  = (header >> 8) ;
  int esign = ((header >> 4) & 1) ;
  int mbits = nbits - sbits - ebits ;
  int maskm = ~(-1 << mbits) ;                      // mantissa mask
  int maske = (ebits == 0) ? 0 : ~(-1 << ebits) ;   // exponent mask
  int sign  = (sbits == 0) ? esign : 0 ;            // 1 if sign bit is 1 everywhere
  for(i=0 ; i<16 ; i++){
    uint32_t s = stream[i] ;
    uint32_t t = (s & maskm) << (23 - mbits) ;
    s >>= mbits ;
    uint32_t e = emin + (s & maske) ;
    t |= (e << 23) ;
    s >>= ebits ;
    s |= sign ;
     t |= (s << 31) ;
     fi[i] = t ;
  }
}
