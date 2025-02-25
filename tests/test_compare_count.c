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

#include <rmn/compare_count.h>

int main(int argc, char **argv){
  int data[64], counta[4], countb[4] ;
  int ref4[] = { 8, 16, 32, 64 } ;
  int i, j, k, ix, nval ;
  int seq[64] ;
  int ref1[] = { 1, 2, 3, 4 } ;
  int ref2[4] ;

  ix = 13 ;
  for(i=0 ; i<sizeof(data)/sizeof(int) ; i++){
    seq[i] = i + 1 ;
    data[ix] = i ;
    ix = ix + 13 ;
    if(ix >= sizeof(data)/sizeof(int)) ix = ix - sizeof(data)/sizeof(int) ;
  }
  fprintf(stderr, "================= compare_count gt lt eq (4-64 values) ================\n") ;

  for(nval=4 ; nval<65 ; nval++){

    count_gt(counta, seq, ref1, nval) ;
    for(j=0 ; j<4 ; j++){
      if(counta[j] != nval-(j+1)) {
        fprintf(stderr, "expecting %d values > %d, got %d\n", nval-(j+1), ref1[j], counta[j]) ;
        goto fail ;
      }
    }

    ref2[0] = nval ; ref2[1] = ref2[0]-1 ; ref2[2] = ref2[1]-1 ; ref2[3] = ref2[2]-1 ;
    count_lt(counta, seq, ref2, nval) ;
    for(j=0 ; j<4 ; j++){
      if(counta[j] != nval-j-1) {
        fprintf(stderr, "expecting %d values < %d, got %d (nval = %d)\n", nval-j-1, ref2[j], counta[j], nval) ;
        goto fail ;
      }
    }

    count_eq(counta, seq, ref2, nval) ;
    for(j=0 ; j<4 ; j++){
      if(counta[j] != 1){
        fprintf(stderr, "expecting %d values == %d, got %d (nval = %d)\n", 1, ref2[j], counta[j], nval) ;
        goto fail ;
      }
    }
  }
  fprintf(stderr, "SUCCESS\n");

  fprintf(stderr, "================= compare_count lt ge  gt+le eq+ne lt ================\n") ;
  for(j=0 ; j<4 ; j++){
    for(i=0 ; i<16 ; i++){ fprintf(stderr, "%2d ", data[i+16*j]) ; } ; fprintf(stderr, "\n") ;
  }
  nval = 64 ;
  count_lt(counta, data, ref4, nval) ;
  fprintf(stderr, "%2d values :", nval) ;
  for(i=0 ; i<4 ; i++){ fprintf(stderr, " %2d values < %2d ", counta[i], ref4[i]) ; } ; fprintf(stderr, "\n\n") ;

  for(i=0 ; i<sizeof(data)/sizeof(int) ; i++) data[i] = i ;
  for(nval=8 ; nval <= sizeof(data)/sizeof(int) ; nval += 8){
    count_lt(counta, data, ref4, nval) ;
    fprintf(stderr, "%2d values :", nval) ;
    for(i=0 ; i<4 ; i++){ fprintf(stderr, " %2d values <  %2d ", counta[i], ref4[i]) ; } ; fprintf(stderr, "\n") ;
    count_ge(countb, data, ref4, nval) ;
    fprintf(stderr, "%2d values :", nval) ;
    for(i=0 ; i<4 ; i++){ fprintf(stderr, " %2d values >= %2d ", countb[i], ref4[i]) ; } ; fprintf(stderr, "\n\n") ;
    for(k=0 ; k<4 ; k++){
      if(counta[k]+countb[k] != nval) exit(1) ;
    }

    count_gt(counta, data, ref4, nval) ;
    count_le(countb, data, ref4, nval) ;
    for(k=0 ; k<4 ; k++){
      if(counta[k]+countb[k] != nval) exit(1) ;
    }

    count_eq(counta, data, ref4, nval) ;
    if(counta[0]>1 || counta[1]>1 || counta[2]>1 || counta[3]>1) exit(1) ;
    count_ne(countb, data, ref4, nval) ;
    for(k=0 ; k<4 ; k++){
      if(counta[k]+countb[k] != nval) exit(1) ;
    }
  }
  for(i=0 ; i<4 ; i++) ref4[i] = 64 ;
  for(j=0 ; j<sizeof(data)/sizeof(int) ; j++){
    count_lt(counta, data, ref4, j+1) ;
    for(k=0 ; k<4 ; k++){
      if(counta[k] != j+1) exit(1) ;
    }
  }
  fprintf(stderr, "SUCCESS\n");

  return 0 ;

fail:
  fprintf(stderr, "FAILED\n");
  return 1 ;
}
