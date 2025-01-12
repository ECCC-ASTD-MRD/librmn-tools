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

#include <rmn/eval_compress.h>

#define BLOCK 64

static void get_block(int lni, int lnj, int i0, int j0, int src[lnj][lni], int ni, int nj, int dst[nj][ni]){
  int i, j ;
  for(j=0 ; j<nj ; j++){
    for(i=0 ; i<ni ; i++){
      dst[j][i] = src[j0+j][i0+i] ;
    }
  }
}

static int count_bits(int ni, int nj, int block[nj][ni]){
  int i, j, max, min, nbits, range, range0 ;
  max = min = block[0][0] ;
  for(j=0 ; j<nj ; j++){
    for(i=0 ; i<ni ; i++){
      max = (block[j][i] > max) ? block[j][i] : max ;
      min = (block[j][i] < min) ? block[j][i] : min ;
    }
  }
  range0 = range = max - min ;
  nbits = 0 ;
  while(range > 0){
    nbits++ ;
    range >>= 1 ;
  }
// fprintf(stderr, "min = %6d, max=%6d, range = %6d, nbits = %6d\n", min, max, range0, nbits) ;
  return ni * nj * nbits ;
}

static void lorenzo(int ni, int nj, int block[nj][ni], int pred[nj][ni]){
  int i, j ;
  pred[0][0] = block[0][0] ;                                        // lower left corner as is
  for(i=1 ; i<ni ; i++) pred[0][i] = block[0][i] - block[0][i-1] ;  // bottom row, 1D predict
  for(j=1 ; j<nj ; j++){
    pred[j][0] = block[j][0] - block[j-1][0] ;                      // left column, 1D predict
    for(i=1 ; i<ni ; i++) pred[j][i] = block[j][i] + block[j-1][i-1] - block[j][i-1] - block[j-1][i] ;
  }
}

int float_compressed_bits(int ni, int nj, float f[nj][ni], float errmax, int *btab){
  int block[BLOCK*BLOCK] ;
  int pred[BLOCK*BLOCK] ;
  int block8[8*8] ;
  int nbits = 0, nblocks = 0, nblock8 = 0, nbits64 = 0, npred = 0, nbits8 = 0 , npred8 = 0 ;
  int i0, j0, i, j, in, jn, ix, i8, j8 ;
  for(j0=0 ; j0<nj ; j0+=BLOCK){
    jn = ((j0 + BLOCK) > nj) ? (nj - j0) : BLOCK ;
    for(i0=0 ; i0<ni ; i0+=BLOCK){
      nblocks++ ;
      in = ((i0 + BLOCK) > ni) ? (ni - i0) : BLOCK ;
      ix = 0 ;
      nbits += 64 ; // block overhead
      // quantize
      for(j=0 ; j<jn ; j++){
        for(i=0 ; i<in ; i++){
          block[ix] = f[j0+j][i0+i] / errmax + .5f ;
          ix++ ;
        }
      }
      nbits64 = nbits64 + 64 + count_bits(in, jn, (void *)block) ;
      // subdivide into 8 x 8 encoding blocks, count bits
      nbits8 += 64 ;
      for(j8=0 ; j8<jn ; j8+=8){
        int j8n = ((j8+8) > jn) ? (jn - j8) : 8 ;
        for(i8=0 ; i8<in ; i8+=8){
          int i8n = ((i8+8) > in) ? (in - i8) : 8 ;
          nbits8 += 16 ; // block overhead
          get_block(in, jn, i8, j8, (void *)block, i8n, j8n, (void *)block8) ;
          nbits8 += count_bits(i8n, j8n, (void *)block8) ;
        }
      }
      // apply predictor
      lorenzo(in, jn, (void *)block, (void *)pred) ;
      pred[0] = 0 ;
      npred = npred + 64 + 32 + count_bits(in, jn, (void *)pred) ;
      npred8 = npred8 + 64 + 32 ;
//       if(predict){
//         lorenzo(in, jn, (void *)block, (void *)pred) ;
//         pred[0] = 0 ;
//         nbits += 32 ;
//       }
      // subdivide into 8 x 8 encoding blocks, count bits
      // TODO collect distribution of nbits
      for(j8=0 ; j8<jn ; j8+=8){
        int j8n = ((j8+8) > jn) ? (jn - j8) : 8 ;
        for(i8=0 ; i8<in ; i8+=8){
          nblock8++ ;
          int i8n = ((i8+8) > in) ? (in - i8) : 8 ;
          npred8 += 16 ; // block overhead
          get_block(in, jn, i8, j8, (void *)pred, i8n, j8n, (void *)block8) ;
          npred8 += count_bits(i8n, j8n, (void *)block8) ;
        }
      }
    }
  }
  btab[0] = nblocks ; btab[1] = nblock8 ; btab[2] = nbits64 ; btab[3] = nbits8 ; btab[4] = npred ; btab[5] = npred8 ;
//   fprintf(stderr, "float_compressed_bits: %d large blocks, %d encoding blocks, nbits64 = %d, nbits8 = %d, npred = %d, npred8 = %d\n",
//                   nblocks, nblock8, nbits64, nbits8, npred, npred8) ;
  return npred8 ;
}
