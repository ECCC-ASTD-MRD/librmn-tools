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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#include <rmn/eval_compress.h>

#define NI 127
#define NJ 127

int main(){
  float f[NJ][NI], bpp, errmax = .125f, min, max ;
  int i, j, nbits ;
  fprintf(stderr, "================= test eval_compress ================\n") ;
  min = 999999.0 ;
  max = -min ;
  for(j=0 ; j<NJ ; j++){
    for(i=0 ; i<NI ; i++){
      f[j][i] = sqrtf((i-31.5f)*(i-31.5f) + (j-31.5f)*(j-31.5f)) ;
      max = (f[j][i] > max) ? f[j][i] : max ;
      min = (f[j][i] < min) ? f[j][i] : min ;
    }
  }
  fprintf(stderr, "min = %12G, max = %12G, max error = %12G\n", min, max, errmax) ;
  fprintf(stderr, "================= without predictor ================\n") ;
  nbits = float_compressed_bits(NI, NJ, (void *)f, errmax, 0);
  bpp = nbits ;
  bpp = bpp / (NI*NJ) ;
  fprintf(stderr, " error = %5.2f, nbits = %d (%5.2f bits/value)\n", errmax, nbits, bpp) ;

  fprintf(stderr, "================= with predictor ===================\n") ;
  nbits = float_compressed_bits(NI, NJ, (void *)f, errmax, 1);
  bpp = nbits ;
  bpp = bpp / (NI*NJ) ;
  fprintf(stderr, " error = %5.2f, nbits = %d (%5.2f bits/value)\n", errmax, nbits, bpp) ;
}

