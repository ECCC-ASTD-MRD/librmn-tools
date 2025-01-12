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
#include <rmn/c_record_io.h>

#define NI 127
#define NJ 127

typedef union{
  uint32_t u ;
  float    f ;
}uf ;

#define ABS(X) (((X)>0) ? (X) : -(X))
#define XOR 0x7

int main(int argc, char **argv){
  float f[NJ][NI], bpp, errmax = .125f, min, max, npts ;
  uf f1, f2, f3, n1, n2, n3 ;
  char *filename = NULL ;
  int i, j, nbits, fd = 0, ndim = 0, ndata, nij ;
  int dims[10], btab[10] ;
  void *buf ;

  fprintf(stderr, "================= test eval_compress ================\n") ;
  fprintf(stderr, "================= float resolution ==================\n") ;
  f1.f=1.0f ; f2.f=2.0f ; f3.f=5.0f ;
  for(i=0 ; i<5 ; i++){
    n1.f = f1.f ; n2.f = f2.f ; n3.f = f3.f ;
    n1.u ^= XOR ; n2.u ^= XOR ; n3.u ^= XOR ; 
    fprintf(stderr, "%7.1f(%9.7f) %7.1f(%9.7f) %7.1f(%9.7f)\n", f1.f, ABS(f1.f-n1.f), f2.f, ABS(f2.f-n2.f), f3.f, ABS(f3.f-n3.f)) ;
    f1.f *= 10 ; f2.f *= 10 ; f3.f *= 10 ;
  }
  fprintf(stderr, "================= original data =====================\n") ;
  if(argc > 1) errmax = atof(argv[1]) ;
  if(argc > 2) {
    filename = argv[2] ;
    buf = read_32bit_data_record(filename, &fd, dims, &ndim, &ndata) ;
    while(buf != NULL){
      fprintf(stderr, "number of dimensions = %d : (", ndim) ;
      for(j=0 ; j<ndim ; j++) { fprintf(stderr, "%d ", dims[j]) ; } ;
      fprintf(stderr, ") [%d]\n", ndata);
      nij = dims[0]*dims[1] ; npts = nij ;
      nbits = float_compressed_bits(dims[0], dims[1], (void *)buf, errmax, btab);
      bpp = nbits ; bpp = bpp / nij ;
      fprintf(stderr, "with prediction : error = %5.2f, nbits = %d (%5.2f bits/value)\n", errmax, nbits, bpp) ;
      fprintf(stderr, "float_compressed_bits: %d large blocks, %d encoding blocks, nbits64 = %d, nbits8 = %d, npred = %d, npred8 = %d\n",
                      btab[0], btab[1], btab[2], btab[3], btab[4], btab[5]) ;
      fprintf(stderr, "float_compressed_bits: block64 = %5.2f bits/value, block8 = %5.2f bits/value, pred64 = %5.2f bits/value, pred8 = %5.2f bits/value\n",
                      btab[2]/npts, btab[3]/npts, btab[4]/npts, btab[5]/npts) ;

      free(buf) ;
      buf = read_32bit_data_record(filename, &fd, dims, &ndim, &ndata) ;
    }
  }
return 0 ;
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
  fprintf(stderr, "================= without predictor =================\n") ;
  nbits = float_compressed_bits(NI, NJ, (void *)f, errmax, btab);
  bpp = nbits ;
  bpp = bpp / (NI*NJ) ;
  fprintf(stderr, " error = %5.2f, nbits = %d (%5.2f bits/value)\n", errmax, nbits, bpp) ;

  fprintf(stderr, "================= with predictor ====================\n") ;
  nbits = float_compressed_bits(NI, NJ, (void *)f, errmax, btab);
  bpp = nbits ;
  bpp = bpp / (NI*NJ) ;
  fprintf(stderr, " error = %5.2f, nbits = %d (%5.2f bits/value)\n", errmax, nbits, bpp) ;
}

