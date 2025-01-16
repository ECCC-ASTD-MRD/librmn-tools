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

void get_min_max(float *buf, int ninj, float *min, float *max){
  int i ;
  float mi = buf[0], ma = buf[0];
  for(i=1 ; i<ninj ; i++){
    mi = (buf[i] < mi) ? buf[i] : mi ;
    ma = (buf[i] > ma) ? buf[i] : ma ;
  }
  *min = mi ;
  *max = ma ;
}

int main(int argc, char **argv){
  float f[NJ][NI], bpp, errmax = .125f, min, max, npts, diffmax ;
  char *filename = NULL ;
  int i, j, nbits, fd = 0, ndim = 0, ndata, nij ;
  int dims[10], btab[10] ;
  void *buf ;

  fprintf(stderr, "================= test eval_compress ================\n") ;
#if 0
  fprintf(stderr, "================= float resolution ==================\n") ;
  uf f1, f2, f3, n1, n2, n3 ;

  f1.f=1.0f ; f2.f=2.0f ; f3.f=5.0f ;
  for(i=0 ; i<5 ; i++){
    n1.f = f1.f ; n2.f = f2.f ; n3.f = f3.f ;
    n1.u ^= XOR ; n2.u ^= XOR ; n3.u ^= XOR ; 
    fprintf(stderr, "%7.1f(%9.7f) %7.1f(%9.7f) %7.1f(%9.7f)\n", f1.f, ABS(f1.f-n1.f), f2.f, ABS(f2.f-n2.f), f3.f, ABS(f3.f-n3.f)) ;
    f1.f *= 10 ; f2.f *= 10 ; f3.f *= 10 ;
  }
#endif
  if(argc > 1) errmax = atof(argv[1]) ;
  errmax = power2_err(errmax) ;
  fprintf(stderr, "errmax = %G\n", errmax) ;
  if(argc < 3) goto synthetic ;
  filename = argv[2] ;

  // ndim == number of dimensions, dims[] == dimensions, ndata == number of data elements, fd == file descriptor
  buf = read_32bit_data_record(filename, &fd, dims, &ndim, &ndata) ;  // get data record
  while(buf != NULL){
    fprintf(stderr, "\nnumber of dimensions = %d : (", ndim) ;
    for(j=0 ; j<ndim ; j++) { fprintf(stderr, "%d ", dims[j]) ; } ;
    fprintf(stderr, ") [%d]", ndata);
    nij = dims[0]*dims[1] ; npts = nij ;
    get_min_max(buf, nij, &min, &max) ;
    diffmax = 0.0f ;
    nbits = float_compressed_bits(dims[0], dims[1], (void *)buf, errmax, btab, 64, &diffmax);
    fprintf(stderr, ", min = %G, max = %G, range = %G, Q = %G, E = %4.2f, f16(Q = %G), f12(Q = %G)\n", min, max, max-min, errmax, diffmax/errmax, (max-min)/65536, (max-min)/4096) ;
    bpp = nbits ; bpp = bpp / npts ;
    fprintf(stderr, "compressed_bits : %d blocks, %d encoding blocks, quant = %d, quant64 = %d, quant64-8 = %d, pred64 = %d, pred64-8 = %d\n",
                    btab[0], btab[1], btab[6], btab[2], btab[3], btab[4], btab[5]) ;
    fprintf(stderr, "bits/value      : quant = %5.2f, pred =%5.2f, pred-8 =%5.2f, quant64 = %5.2f, quant64-8 = %5.2f, pred64 = %5.2f, pred64-8 = %5.2f\n",
                    btab[6]/npts, btab[7]/npts, btab[9]/npts, btab[2]/npts, btab[3]/npts, btab[4]/npts, btab[5]/npts) ;

    free(buf) ;

    buf = read_32bit_data_record(filename, &fd, dims, &ndim, &ndata) ;   // try to get next data record
  }
  return 0 ;

  fprintf(stderr, "================= synthetic data =====================\n") ;
synthetic:     // in case no filename is given
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

  fprintf(stderr, "================= with predictor ====================\n") ;
  nbits = float_compressed_bits(NI, NJ, (void *)f, errmax, btab, 64, &diffmax);
  bpp = nbits ;
  bpp = bpp / (NI*NJ) ;
  fprintf(stderr, " error = %5.2f, nbits = %d (%5.2f bits/value)\n", errmax, nbits, bpp) ;
  return 0 ;
}

