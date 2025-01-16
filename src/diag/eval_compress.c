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

#include <rmn/eval_compress.h>

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
  int nbits = 0 ;
  while(n > 0){
    nbits++ ;
    n >>= 1 ;
  }
  return nbits ;
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

// return estimate of number of bits needed to encode block
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

// return power of 2 <= err
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

// return estimate of number of bits necessary to quantize and encode float array f with prediction
// bsize  [IN]: dimension of quantization/prediction blocks
// quant  [IN]: quantization interval (power of 2 <= quant will be used)
// btab  [OUT]: more detailed information
int float_compressed_bits(int ni, int nj, float f[nj][ni], float quant, int *btab, int bsize, float *diffmax){
  int q[nj][ni] ;
  int p[nj][ni] ;
  int block[bsize*bsize] ;
  int pred[bsize*bsize] ;
  int block8[8*8] ;
  int nbits = 0, nblocks = 0, nblock8 = 0, nbits64 = 0, npred = 0, nbits8 = 0 , npred8 = 0, nbitsg = 0, nbitsp = 0, asym = 0, rawp8 = 0, nraw8 = 0 ;
  int i0, j0, i, j, in, jn, ix, i8, j8, min, max ;

  quant = power2_err(quant) ;  // power of 2 <= quant
  *diffmax = 0.0f ;
  // quantize the whole array
  for(j=0 ; j<nj ; j++){
    for(i=0 ; i<ni ; i++){
      float diff ;
      q[j][i] = f[j][i] / quant + ((f[j][i] < 0.0f) ? (-.5f) : .5f) ;
      diff = f[j][i] - (q[j][i] * quant) ;
      diff = (diff < 0) ? -diff : diff ;
      *diffmax = (diff > *diffmax) ? diff : *diffmax ;
    }
  }
  get_min_max_i((void *)q, ni*nj, &min, &max);
  nbitsg = 64 + count_bits(ni, nj, (void *)q) ;  // nbits for the whole array
  // predict the whole array
  lorenzo(ni, nj, (void *)q, (void *)p) ;
  p[0][0] = 0 ;
  nbitsp = 96 + count_bits(ni, nj, (void *)p) ;  // nbits for the whole predicted array
  rawp8 = 96 ;
  // subdivide the whole predicted array into 8x8 blocks
  for(j8=0 ; j8<nj ; j8+=8){
    int j8n = ((j8+8) > nj) ? (nj - j8) : 8 ;
    for(i8=0 ; i8<ni ; i8+=8){
      nraw8++ ;
      int i8n = ((i8+8) > ni) ? (ni - i8) : 8 ;
      rawp8 += 20 ; // average encoding block overhead
      get_block(ni, nj, i8, j8, (void *)p, i8n, j8n, (void *)block8) ;
// if(i8 == 512 && j8 == 512) print_block(i8n, j8n, (void *)block8) ;
      rawp8 += count_bits(i8n, j8n, (void *)block8) ;
    }
  }

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
//           block[ix] = f[j0+j][i0+i] / quant + .5f ;
          block[ix] = q[j0+j][i0+i] ;
          ix++ ;
        }
      }
      // bits needed if not subdividing quantization block
      nbits64 = nbits64 + 64 + count_bits(in, jn, (void *)block) ;
      // subdivide large block into 8 x 8 encoding blocks, count bits
      nbits8 += 64 ;
      for(j8=0 ; j8<jn ; j8+=8){
        int j8n = ((j8+8) > jn) ? (jn - j8) : 8 ;
        for(i8=0 ; i8<in ; i8+=8){
          int i8n = ((i8+8) > in) ? (in - i8) : 8 ;
          nbits8 += 20 ; // average encoding block overhead
          get_block(in, jn, i8, j8, (void *)block, i8n, j8n, (void *)block8) ;
// if(i0 == 512 && j0 == 512 && j8 == 0 && i8 == 0) print_block(i8n, j8n, (void *)block8) ;
          nbits8 += count_bits(i8n, j8n, (void *)block8) ;
        }
      }
      // apply predictor to quantization block
      lorenzo(in, jn, (void *)block, (void *)pred) ;
      pred[0] = 0 ;
      // bits needed if not subdividing predicted block
      npred = npred + 64 + 32 + count_bits(in, jn, (void *)pred) ;
      npred8 = npred8 + 64 + 32 ;  // large block overhead
      // subdivide predicted blocks into 8 x 8 encoding blocks, count bits
      // TODO collect distribution of nbits
      for(j8=0 ; j8<jn ; j8+=8){
        int j8n = ((j8+8) > jn) ? (jn - j8) : 8 ;
        for(i8=0 ; i8<in ; i8+=8){
          nblock8++ ;   // count encoding blocks
          int i8n = ((i8+8) > in) ? (in - i8) : 8 ;
          npred8 += 20 ; // average encoding block overhead
          get_block(in, jn, i8, j8, (void *)pred, i8n, j8n, (void *)block8) ;
// if(i0 == 512 && j0 == 512 && j8 == 0 && i8 == 0) print_block(i8n, j8n, (void *)block8) ;
          int nbi = count_bits(i8n, j8n, (void *)block8) ;
          npred8 += nbi ;
          get_min_max_i((void *)block8, i8n*j8n, &min, &max);
          if(bits_needed(max-min)*i8n*j8n > nbi) asym++ ;
        }
      }
    }
  }
  if(nraw8 != nblock8) exit(1);
  // detailed stats
  btab[0] = nblocks ;   // number of quantization/prediction blocks
  btab[1] = nblock8 ;   // number of encoding blocks
  btab[2] = nbits64 ;   // number of bits for quantized only blocks
  btab[3] = nbits8 ;    // number of bits for quantized encoded blocks
  btab[4] = npred ;     // number of bits for quantized predicted blocks
  btab[5] = npred8 ;    // number of bits for quantized predicted encoded blocks
  btab[6] = nbitsg ;    // number of bits for global quantized array
  btab[7] = nbitsp ;    // number of bits for global predicted array
  btab[8] = asym ;      // number of "asymmetric" blocks
  btab[9] = rawp8 ;     // number of bits for quantized encoded 8x8 global array
//   fprintf(stderr, "float_compressed_bits: %d large blocks, %d encoding blocks, nbits64 = %d, nbits8 = %d, npred = %d, npred8 = %d\n",
//                   nblocks, nblock8, nbits64, nbits8, npred, npred8) ;
  return npred8 ;
}
