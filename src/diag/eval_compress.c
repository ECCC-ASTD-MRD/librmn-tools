// Hopefully useful code for C (memory block movers)
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
#include<stdio.h>
#include<stdint.h>
#include<stdlib.h>
#include<string.h>
#include <immintrin.h>

#include <rmn/eval_compress.h>
#include <rmn/dwt_i_lgt53.h>

#define STATIC static

// leading zeros count (32 bit word)
STATIC inline uint32_t lzcnt_32(uint32_t what){
  uint32_t cnt ;
  __asm__ __volatile__ ("lzcnt %1, %0" : "=r"(cnt) : "r"(what) : "cc" ) ;
  return cnt ;
}

// number of bits needed to represent a 32 bit signed number
// uses lzcnt_32 function, that uses the lzcnt instruction
STATIC inline uint32_t BitsNeeded_32(int32_t what){
  union {
    int32_t  i ;
    uint32_t u ;
  }iu ;
  uint32_t nbits = 33 - lzcnt_32(what) ; // there must be a 0 bit at the front
  if(what >= 0) goto end ;
  iu.i = what ;                          // what < 0
  nbits = 33 - lzcnt_32(~iu.u) ;         // one's complement, then count leading zeros
end:
  return (nbits > 32) ? 32 : nbits ;     // max is 32 bits
}

// number of bits needed to represent a 32 bit unsigned number
// uses lzcnt_32 function, that uses the lzcnt instruction
STATIC inline uint32_t BitsNeeded_u32(uint32_t what){
  return 32 - lzcnt_32(what) ;
}

// 2's complement to/from negabinary (base -2) conversion

#define NBMASK 0xaaaaaaaau /* negabinary<-> 2's complement binary conversion mask */

// signed integer (2's complement) to negabinary (base -2) conversion
STATIC inline uint32_t int_to_negabinary(int32_t x)
{
  return ((uint32_t)x + NBMASK) ^ NBMASK;
}

// negabinary (base -2) to signed integer (2's complement) conversion
STATIC inline int32_t negabinary_to_int(uint32_t x)
{
  return (int32_t)((x ^ NBMASK) - NBMASK);
}

// convert to sign and magnitude form, sign is Least Significant Bit
STATIC inline uint32_t to_zigzag_32(int32_t what){
  return (what << 1) ^ (what >> 31) ;
}

// convert from sign and magnitude form, sign is Least Significant Bit
STATIC inline int32_t from_zigzag_32(uint32_t what){
  int32_t sign = -(what & 1) ;
  return ((what >> 1) ^ sign) ;
}

// get a smaller contiguous block from large array src
static void get_block(int lni, int lnj, int i0, int j0, int src[lnj][lni], int ni, int nj, int dst[nj][ni]){
  int i, j ;
  for(j=0 ; j<nj ; j++){
    for(i=0 ; i<ni ; i++){
      dst[j][i] = src[j0+j][i0+i] ;
    }
  }
}

// return number of bits needed to represent n (assumed >= 0)
static int bits_needed(int n){
  return BitsNeeded_u32((uint32_t) n) ;
//   int nbits = 0 ;
//   while(n > 0){
//     nbits++ ;
//     n >>= 1 ;
//   }
//   return nbits ;
}

// get min and max of integer array
static void get_min_max_i(int *buf, int ninj, int *min, int *max){
  int i ;
  int mi = buf[0], ma = buf[0];
  for(i=1 ; i<ninj ; i++){
    mi = (buf[i] < mi) ? buf[i] : mi ;
    ma = (buf[i] > ma) ? buf[i] : ma ;
  }
  *min = mi ;
  *max = ma ;
}

// print block
static void print_block(int ni, int nj, int block[nj][ni]){
  int i, j ;
  for(j=nj-1 ; j>=0 ; j--){
    for(i=0 ; i<ni ; i++){
      fprintf(stderr, "%10d", block[j][i]);
    }
    fprintf(stderr, "\n");
  }
  fprintf(stderr, "\n");
}

// return estimate of number of bits needed to encode a block of size ni X nj
static int count_encoded_bits(int ni, int nj, int block[nj][ni], int *info){
  int i, j, max, min, nbits, extra = 0, btab[33], maxbits, blockij, nshort, nij = ni * nj, nbits_i, allneg = 0, delta, tmp ;

  max = min = block[0][0] ;
  for(j=0 ; j<nj ; j++){       // get extrema
    for(i=0 ; i<ni ; i++){
      max = (block[j][i] > max) ? block[j][i] : max ;
      min = (block[j][i] < min) ? block[j][i] : min ;
    }
  }
  if(max == min){                         // constant block
    info[36]++ ;
    max = (max < 0) ? -max : max ;
    info[37] = info[37] + (1 + bits_needed(max)) * (nij - 1) ;   // bits gained
    if(max == 0) return 9 ;               // special case, zero block
    int needed = BitsNeeded_32(max) ;
    needed = (needed > 16) ? (needed + 13) : (needed + 8) ;   // long header if more thatn 16 bits
    return needed ;
  }

  if(max > 0 && min < 0){   // positive and negative values are present, use zigzag encoding
    max = 0 ;
    for(j=0 ; j<nj ; j++){
      for(i=0 ; i<ni ; i++){
        block[j][i] = to_zigzag_32(block[j][i]) ;
        max = (block[j][i] > max) ? block[j][i] : max ;
      }
    }
    min = 0 ;  // min is assumed to be 0 for zigzag encoding
  }else if(max <= 0){   // all values <= 0
    allneg = 1 ;
    for(j=0 ; j<nj ; j++){
      for(i=0 ; i<ni ; i++){
        block[j][i] = (-block[j][i]) ;  // negate number to make it positive
      }
    }
    tmp = max ;      // swap min and max values after taking absolute values
    max = (-min) ;   // largest absolute value
    min = (-tmp) ;   // smallest absolute value
  }

  // at this point, all values are >= 0, min is always >= 0

  if(BitsNeeded_u32((max-min)) < BitsNeeded_u32(max)){  // will we save bits by using an offset ?
    extra = extra + 5 + BitsNeeded_32(min) ;            // offset is stored as 2's complement
    for(j=0 ; j<nj ; j++){
      for(i=0 ; i<ni ; i++){
        block[j][i] = block[j][i] - min ;               // subtract minimum value
      }
    }
    max = max - min ;
    min = 0 ;
  }

  // is short/long encoding appropriate ?
  for(i=0 ; i<33 ; i++){ btab[i] = 0 ; }
  maxbits = 0 ;
  for(j=0 ; j<nj ; j++){        // tabulate bits needed per sample
    for(i=0 ; i<ni ; i++){
      nbits = BitsNeeded_u32(block[j][i]) ;
      maxbits = (nbits > maxbits) ? nbits : maxbits ;
      btab[nbits]++ ;           // bump count for nbits
    }
  }
  for(i=1 ; i<33 ; i++) { btab[i] = btab[i] + btab[i-1] ; }  // btab[i] == nb of values needing i bits or less
  nbits = maxbits * nij ;   // worst case, maxbits per value
  if(maxbits < 2){          // maxbits is 1, no point in short/long encoding, no bits gained
    info[34]++ ;
    return nbits + 8 + extra ;    // nbits < 16, short header
  }
  nshort = 32 ;                   // impossible value
  //        btab[0] elements 0 bits long
  nbits_i = btab[0] + (maxbits + 1) * (nij - btab[0]) ;    // bits needed if nshort == 0
  //                   nij - btab[0] elements need more than 0 bits
  if(nbits_i < nbits){            // any gain ?
    nbits = nbits_i ;             // yes
    nshort = 0 ;
  }
  delta = 1000 ;
  for(i=1 ; i<maxbits-1 ; i++){           // nshort == maxbits-1 cold not profide any gain
    if( (i >= maxbits/2-0) && (i <= maxbits/2+2) ){     // limited range of 3 values around maxbits/2
      //        btab[i] elements i bits long (or shorter)
      nbits_i = (i + 1) * btab[i] + (maxbits + 1) * (nij - btab[i]) ; // short/long encoding
      //                            nij - btab[i] elements need more than i bits
      if(nbits_i < nbits){        // is nshort == i better than previous cases ?
        nbits = nbits_i ;
        nshort = i ;
        delta = (maxbits/2) - nshort ;
      }
    }
  }
  if(nshort < 32){  // is short/long encoding appropriate ?
    info[nshort]++ ;
    info[33]++ ;
    if(delta < 6) info[64 - delta] = info[64 - delta] + 1 ;
    info[64-6] = info[0] ;
    info[37] = info[37] + maxbits * nij - (nbits + 4) ;   // bits gained by encoding
    extra += 2 ;  // ee field needed
  }else{
    info[35]++ ;  // short/long encoding is not appropriate
    if(maxbits > 16) extra += 4 ; // long header
  }

  return 8 + nbits + extra ;   // base header + extra header fields
}

// return estimate of number of bits needed to represent block
static int count_bits(int ni, int nj, int block[nj][ni]){
  int i, j, max, min, nbits, range, extra ;
  max = min = block[0][0] ;
  for(j=nj-1 ; j>=0 ; j--){
    for(i=0 ; i<ni ; i++){
      max = (block[j][i] > max) ? block[j][i] : max ;
      min = (block[j][i] < min) ? block[j][i] : min ;
    }
  }
//   range = max - min ;
//   extra = 21 ;
  if(max * min < 0){   // positive and negative numbers are present
    min = -min ;       // abs(min)
    max = (min > max) ? min : max ;   // largest absolute value between min and max
    range = 2 * max ;                 // zigzag coding
    extra = 0 ;
  }else{               // all numbers have the same sign, use an offset
    range = max - min ;
    if(min < range/4 && min > 0) range = max ;
    extra = 21 ;       // offset + nbits
  }
  nbits = bits_needed(range) ;
//   int range0 = range ;
// if(ni*nj > 1000000) fprintf(stderr, "count_bits : min = %d, max = %d, range = %d, nbits = %d\n", min, max, range0, nbits) ;
  return ni * nj * nbits + extra ;
}

// apply 2D Lorenzo predictor to block, result in pred
static void lorenzo(int ni, int nj, int block[nj][ni], int pred[nj][ni]){
  int i, j ;
  pred[0][0] = block[0][0] ;                                        // lower left corner as is
  for(i=1 ; i<ni ; i++) pred[0][i] = block[0][i] - block[0][i-1] ;  // bottom row, 1D predict
  for(j=1 ; j<nj ; j++){
    pred[j][0] = block[j][0] - block[j-1][0] ;                      // left column, 1D predict
    for(i=1 ; i<ni ; i++) pred[j][i] = block[j][i] + block[j-1][i-1] - block[j][i-1] - block[j-1][i] ;
  }
}

// return power of 2 >= err
static float power2_err(float err){
  union{
    uint32_t u ;
    float    f ;
  } uf ;
  uf.f = err ;
  uf.u += 0x007FFFFFu ;
  uf.u &= 0xFF800000u ;
  return uf.f ;
}

int block_diff(int *a, int *b, int n){
  int err = 0 ;
  int i ;
  for(i=0 ; i<n ; i++){
    if(a[i] != b[i]) err++ ;
  }
  return err ;
}

// return estimate of number of bits necessary to quantize and encode float array f with prediction
// bsize    [IN]: dimension of quantization/prediction blocks
// quant [INOUT]: quantization interval (power of 2 <= quant will be used)
//                if quant < 0, it is interpreted as a number of significant bits
// btab    [OUT]: more detailed information
// dmax    [OUT]: largest absolute difference when restoring quantized float values
int float_compressed_bits(int ni, int nj, float f[nj][ni], float *quant_, int btab[MAXBTAB], int bsize, float *dmax){
  float quant = *quant_ ;
  int q[nj][ni] ;
  int p[nj][ni] ;
  int block[bsize*bsize] ;
  int pred[bsize*bsize] ;
  int block8[8*8] ;
  int info[1024] ;
  int nbits = 0, nblocks = 0, nblock8 = 0, nbits64 = 0, npred = 0, nbits8 = 0 ;
  int npred8 = 0, nbitsg = 0, nbitsp = 0, asym = 0, rawp8 = 0, nraw8 = 0, ndwt8 = 0 ;
  int i0, j0, i, j, in, jn, ix, i8, j8, min, max, nbi, range, ndiff ;

  if(quant < 0){
    nbits = (-quant) ;
    range = 1 << (nbits -1) ;
fprintf(stderr, "\nnbits = %d, intervals = %d", nbits, range) ;
    float minf, maxf ;
    minf = maxf = f[0][0] ;
    for(j=0 ; j<nj ; j++){
      for(i=0 ; i<ni ; i++){
        maxf = (f[j][i] > maxf) ? f[j][i] : maxf ;
        minf = (f[j][i] < minf) ? f[j][i] : minf ;
      }
    }
    quant = (maxf - minf) / range / 2 ;
    quant = power2_err(quant) ;  // power of 2 >= quant
fprintf(stderr, ", err = %G", quant*.5f) ;
fprintf(stderr, ", quant = %G\n", quant) ;
  }else{
    quant = power2_err(quant) ;  // power of 2 >= quant
  }

  for(i=0 ; i<sizeof(info)/sizeof(int) ; i++){ info[i] = 0 ; }
  *dmax = 0.0f ;
  // quantize the whole array, unquantize and get max absolute difference
  for(j=0 ; j<nj ; j++){
    for(i=0 ; i<ni ; i++){
      float diff ;
      q[j][i] = f[j][i] / quant + ((f[j][i] < 0.0f) ? (-.5f) : .5f) ; // different rounding for positive and negative numbers
      diff = f[j][i] - (q[j][i] * quant) ;
      diff = (diff < 0) ? -diff : diff ;
      *dmax = (diff > *dmax) ? diff : *dmax ;
    }
  }
  *quant_ = quant ;
  get_min_max_i((void *)q, ni*nj, &min, &max);
  nbitsg = 64 + count_bits(ni, nj, (void *)q) ;  // nbits for the whole array
  // predict the whole array
  lorenzo(ni, nj, (void *)q, (void *)p) ;
  p[0][0] = 0 ;
  nbitsp = 96 + count_bits(ni, nj, (void *)p) ;  // nbits for the whole predicted array
  rawp8 = 96 ;
  // subdivide the whole predicted array into 8x8 blocks
  for(i=0 ; i<sizeof(info)/sizeof(int) ; i++){ info[i] = 0 ; }
  for(j8=0 ; j8<nj ; j8+=8){
    int j8n = ((j8+8) > nj) ? (nj - j8) : 8 ;
    for(i8=0 ; i8<ni ; i8+=8){
      nraw8++ ;
      int i8n = ((i8+8) > ni) ? (ni - i8) : 8 ;
//       rawp8 += 20 ; // average encoding block overhead
      get_block(ni, nj, i8, j8, (void *)p, i8n, j8n, (void *)block8) ;
// if(i8 == 512 && j8 == 512) print_block(i8n, j8n, (void *)block8) ;
//       nbi = count_bits(i8n, j8n, (void *)block8) ;
      nbi = count_encoded_bits(i8n, j8n, (void *)block8, info) ;
      rawp8 += nbi ;
    }
  }
fprintf(stderr, "%s[%d,%d,%d,%d]: ", nbits ? "" : "\n", info[33], info[34], info[35], info[36]) ;
for(i=0 ; i<16 ; i++){ fprintf(stderr, "%4d ", info[i]) ; } ;
fprintf(stderr, " |%d,%d,%d,%d,%d,%d, %d ,%d,%d,%d,%d,%d,%d|\n",info[58],info[59],info[60],info[61],info[62],info[63],info[64],info[65],info[66],info[67],info[68],info[69],info[70]) ;

  for(i=0 ; i<sizeof(info)/sizeof(int) ; i++){ info[i] = 0 ; }
  // loops over quantization/prediction blocks
  for(j0=0 ; j0<nj ; j0+=bsize){
    jn = ((j0 + bsize) > nj) ? (nj - j0) : bsize ;
    for(i0=0 ; i0<ni ; i0+=bsize){
      nblocks++ ;    // count quantization/prediction blocks
      in = ((i0 + bsize) > ni) ? (ni - i0) : bsize ;
      ix = 0 ;
      nbits += 64 ; // large block overhead
      // get small quantized block
      for(j=0 ; j<jn ; j++){
        for(i=0 ; i<in ; i++){
          block[ix] = q[j0+j][i0+i] ;
          ix++ ;
        }
      }
      // bits needed if not subdividing quantization block
      nbits64 = nbits64 + 64 + count_bits(in, jn, (void *)block) ;
      // subdivide quantization block into 8 x 8 encoding blocks, count bits
      nbits8 += 64 ;
      for(j8=0 ; j8<jn ; j8+=8){
        int j8n = ((j8+8) > jn) ? (jn - j8) : 8 ;
        for(i8=0 ; i8<in ; i8+=8){
          int i8n = ((i8+8) > in) ? (in - i8) : 8 ;
//           nbits8 += 20 ; // average encoding block overhead
          get_block(in, jn, i8, j8, (void *)block, i8n, j8n, (void *)block8) ;
// if(i0 == 512 && j0 == 512 && j8 == 0 && i8 == 0) print_block(i8n, j8n, (void *)block8) ;
          nbits8 += count_bits(i8n, j8n, (void *)block8) ;
        }
      }
      // apply Le GAll transform to quantization block
      memcpy((void *)pred, (void *)block, in*jn*sizeof(int32_t)) ;
      fwd_2d_lgt53_n((void *)pred, in, in, jn, 2);
//       inv_2d_lgt53_n((void *)pred, in, in, jn, 2);
//       ndiff = block_diff(pred, block, in*jn) ;
//       if(ndiff != 0){
//         fprintf(stderr, "ERROR in DWT, derrors = %d / %d\n", ndiff, in*jn) ;
//         exit(1) ;
//       }
      ndwt8 = ndwt8 + 64 ;  // large block overhead
      // subdivide transformed block into 8 x 8 encoding blocks, count bits
      for(j8=0 ; j8<jn ; j8+=8){
        int j8n = ((j8+8) > jn) ? (jn - j8) : 8 ;
        for(i8=0 ; i8<in ; i8+=8){
          int i8n = ((i8+8) > in) ? (in - i8) : 8 ;
          get_block(in, jn, i8, j8, (void *)pred, i8n, j8n, (void *)block8) ;
          nbi = count_encoded_bits(i8n, j8n, (void *)block8, info) ;
          ndwt8 += nbi ;
        }
      }

      // apply Lorenzo predictor to quantization block
      lorenzo(in, jn, (void *)block, (void *)pred) ;
      pred[0] = 0 ;
      // bits needed if not subdividing predicted block
      npred = npred + 64 + 32 + count_bits(in, jn, (void *)pred) ;
      npred8 = npred8 + 64 + 32 ;  // large block overhead
      // subdivide predicted block into 8 x 8 encoding blocks, count bits
      // TODO collect distribution of nbits
      for(j8=0 ; j8<jn ; j8+=8){
        int j8n = ((j8+8) > jn) ? (jn - j8) : 8 ;
        for(i8=0 ; i8<in ; i8+=8){
          nblock8++ ;   // count encoding blocks
          int i8n = ((i8+8) > in) ? (in - i8) : 8 ;
//           npred8 += 20 ; // average encoding block overhead
          get_block(in, jn, i8, j8, (void *)pred, i8n, j8n, (void *)block8) ;
// if(i0 == 512 && j0 == 512 && j8 == 0 && i8 == 0) print_block(i8n, j8n, (void *)block8) ;
//           nbi = count_bits(i8n, j8n, (void *)block8) ;
          nbi = count_encoded_bits(i8n, j8n, (void *)block8, info) ;
          npred8 += nbi ;
          get_min_max_i((void *)block8, i8n*j8n, &min, &max);
          if(bits_needed(max-min)*i8n*j8n > nbi) asym++ ;
        }
      }
    }
  }
fprintf(stderr, "[%d,%d,%d,%d]: ",info[33],info[34],info[35],info[36]) ;
for(i=0 ; i<16 ; i++){ fprintf(stderr, "%4d ", info[i]) ; } ;
fprintf(stderr, " |%d,%d,%d,%d,%d,%d, %d ,%d,%d,%d,%d,%d,%d|",info[58],info[59],info[60],info[61],info[62],info[63],info[64],info[65],info[66],info[67],info[68],info[69],info[70]) ;
  if(nraw8 != nblock8) exit(1);
  // detailed stats
  btab[ 0] = nblocks ;   // number of quantization/prediction blocks
  btab[ 1] = nblock8 ;   // number of encoding blocks
  btab[ 2] = nbits64 ;   // number of bits for quantized only blocks
  btab[ 3] = nbits8 ;    // number of bits for quantized encoded blocks
  btab[ 4] = npred ;     // number of bits for quantized predicted blocks
  btab[ 5] = npred8 ;    // number of bits for quantized predicted encoded blocks
  btab[ 6] = nbitsg ;    // number of bits for global quantized array
  btab[ 7] = nbitsp ;    // number of bits for global predicted array
  btab[ 8] = asym ;      // number of "asymmetric" blocks
  btab[ 9] = rawp8 ;     // number of bits for quantized encoded 8x8 global array
  btab[10] = info[37] ;  // bits gained
  btab[11] = ndwt8 ;     // wavelet blocks encoded 8x8
//   fprintf(stderr, "float_compressed_bits: %d large blocks, %d encoding blocks, nbits64 = %d, nbits8 = %d, npred = %d, npred8 = %d\n",
//                   nblocks, nblock8, nbits64, nbits8, npred, npred8) ;
  return npred8 ;
}
