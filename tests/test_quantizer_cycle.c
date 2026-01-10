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

#include <rmn/test_helpers.h>
#include <rmn/c_record_io.h>

int main(int argc, char **argv){
  char *filename = NULL, *msg = "" ;
  float quant = 0.0, minsig = 0.0 ;
  int nij, dims[10], ndim = 0, fd = 0, ndata, ncases = 0, j, nbits = 0 ;
  void *buf ;

  start_of_test("test quantizer cycle");

  msg = "invalid arguments" ;
  switch(argc){
    case 5:
      minsig = atof(argv[4]) ;  // smallest absolute significant value (pseudo_log quantizer)
    case 4:
      nbits = atoi(argv[3]) ;   // nbits (linear or pseudo_log quantizer)
    case 3:
      quant = atof(argv[2]) ;   // quantum (linear quantizer), max rel error (pseudo_log quantizer)
    case 2:
      filename = argv[1] ;
      break ;
    default:
      fprintf(stderr, "expected at least 1 argument, got %d\n", argc-1) ;
      goto fail ;
  }
  // if argc > 2, use pseudo log quantizer)

  fprintf(stderr,"filename  = '%s', quant = %f, minsig = %f, nbits = %d\n", filename, quant, minsig, nbits) ;
  buf = read_32bit_data_record(filename, &fd, dims, &ndim, &ndata) ;  // get data record
  while(buf != NULL){
    nij = dims[0]*dims[1] ;
    msg = "dimension mismatch or not a 2D record" ;
    if(nij != ndata || ndim != 2) goto fail ;
    fprintf(stderr, "%d dimensions : ", ndim) ;
    for(j=0 ; j<ndim ; j++) { fprintf(stderr, "[%d]", dims[j]) ; } ;
    fprintf(stderr, " (%d)\n", nij);

    // allocate q and ref ( ndata * sizeof(float/int) )
    // quantize buf to q, restore q to ref
    // loop for NCYCLES
      // quantize ref to q, restore q to buf
      // compare ref to buf, fail if not identical

    ncases++ ;
    free(buf) ;
    buf = read_32bit_data_record(filename, &fd, dims, &ndim, &ndata) ;   // try to get next data record
  }

  fprintf(stderr, "SUCCESS : ncases = %d, quant = %f\n", ncases, quant) ;
  return 0 ;

fail:
  fprintf(stderr, "FAIL : %s\n", msg) ;
  return 1 ;
}
