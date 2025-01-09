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
fprintf(stderr, "min = %6d, max=%6d, range = %6d, nbits = %6d\n", min, max, range0, nbits) ;
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

int float_compressed_bits(int ni, int nj, float f[nj][ni], float errmax, int predict){
  int block[BLOCK*BLOCK] ;
  int pred[BLOCK*BLOCK] ;
  int nbits = ni * nj * .25 ;   //  estimated overhead
  int i0, j0, i, j, in, jn, ix ;
  for(j0=0 ; j0<nj ; j0+=BLOCK){
    jn = ((j0 + BLOCK) > nj) ? (nj - j0) : BLOCK ;
    for(i0=0 ; i0<ni ; i0+=BLOCK){
      in = ((i0 + BLOCK) > ni) ? (ni - i0) : BLOCK ;
      ix = 0 ;
      // quantize
      for(j=0 ; j<jn ; j++){
        for(i=0 ; i<in ; i++){
          block[ix] = pred[ix] = f[j0+j][i0+i] / errmax + .5f ;
          ix++ ;
        }
      }
      // apply predictor
      if(predict){
        lorenzo(in, jn, (void *)block, (void *)pred) ;
        pred[0] = 0 ;
        nbits += 32 ;
      }
      // count bits
      nbits += count_bits(in, jn, (void *)pred) ;
    }
  }
  return nbits ;
}
