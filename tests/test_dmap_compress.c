// Hopefully useful code for C (memory block movers)
// Copyright (C) 2025  Recherche en Prevision Numerique
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
#include <string.h>

#include <rmn/test_helpers.h>
#include <rmn/c_record_io.h>
#include <rmn/split_dimension.h>
#include <rmn/move_blocks.h>
#include <rmn/data_map.h>
#include <rmn/dmap_filters.h>

// suppress mantissa of floating point value z
static float f_adjust(float z){
  union{float f ; int32_t i ;} fi;
  fi.f = z ;
  fi.i &= 0xFF800000 ;
  return fi.f ;
}

// find min and max value in array f
static void minmax(float *f, int32_t n, float *fmin, float *fmax){
  float min, max ;
  int i;
  min = max = f[0] ;
  for(i=0 ;i<n ; i++){
    min = (f[i] < min) ? f[i] : min ;
    max = (f[i] > max) ? f[i] : max ;
  }
  *fmin = min ;
  *fmax = max ;
}

#define BSIZE 64

// int32_t get_array_block(){
// }

int main(int argc, char **argv){
  char *filename = NULL, *msg = "" ;
  float quant = 0.0, minsig = 0.0, min, max, zval = 0.0 ;
  int32_t nij, dims[10], ndim = 0, fd = 0, ndata, ncases = 0, j, nbits = 16, stripe = 1, mextra = 0 ;
  void *buf ;
  array_axis i_axis, j_axis ;
  zmap *map ;

  start_of_test("test quantizer cycles");

  msg = "invalid arguments" ;

  switch(argc){                 // if argc > 2, use pseudo log quantizer)
    case 6:
      zval = atof(argv[5]) ;  // smallest absolute significant value (pseudo_log quantizer)
    case 5:
      minsig = atof(argv[4]) ;  // smallest absolute significant value (pseudo_log quantizer)
      minsig = f_adjust(minsig) ;
    case 4:
      nbits = atoi(argv[3]) ;   // nbits (linear or pseudo_log quantizer)
    case 3:
      quant = atof(argv[2]) ;   // quantum (linear quantizer), smallest absolute significant value  (pseudo_log quantizer)
    case 2:
      filename = argv[1] ;
      break ;
    default:
      fprintf(stderr, "expected 2,3, 4 or 5 arguments, got %d\n", argc-1) ;
      goto fail ;
  }

  fprintf(stderr,"filename  = '%s', quant = %f, minsig = %f, zval = %f, nbits = %d\n", filename, quant, minsig, zval, nbits) ;
  buf = read_32bit_data_record(filename, &fd, dims, &ndim, &ndata) ;  // get data record
  while(buf != NULL && ncases < 99){
    ncases++ ;
    if((ncases % 19) != 1) goto next ;
    nij = dims[0]*dims[1] ;
    msg = "dimension mismatch or not a 2D record" ;
    if(nij != ndata || ndim != 2) goto fail ;
    i_axis = split_axis_32(dims[0], BSIZE) ;
    j_axis = split_axis_32(dims[1], BSIZE) ;

    minmax(buf, nij, &min, &max) ;
    fprintf(stderr, "[%2d] %d dimensions : ", ncases, ndim) ;
    for(j=0 ; j<ndim ; j++) { fprintf(stderr, "[%d]", dims[j]) ; } ;
    fprintf(stderr, " (%d), [ %10f -> %10f ]", nij, min, max);
    fprintf(stderr, " [%2d blocks %2d,%2d] [%2d blocks %2d,%2d]\n", i_axis.nbk, i_axis.ln0, i_axis.ln1, j_axis.nbk, j_axis.ln0, j_axis.ln1) ;

    // create data map (mextra = 16 bytes)
    map = new_zmap(dims[0], dims[1], stripe = 1, sizeof(int32_t), mextra = 16) ;
    free_zmap(map, 1) ;
//     nw32 = map_compress_2d(buf, dims, map, filters, bitstream) ;

    fprintf(stderr, "\n");
next :
    free(buf) ;
    buf = NULL ;
    buf = read_32bit_data_record(filename, &fd, dims, &ndim, &ndata) ;   // try to get next data record
  }

  fprintf(stderr, "SUCCESS : ncases = %d, quant = %f, nbits = %d\n", ncases, quant, nbits) ;
  return 0 ;

fail:
  fprintf(stderr, "\nFAIL : %s\n", msg) ;
  return 1 ;
}
