//
// Copyright (C) 2025  Environnement Canada
//
// This is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation,
// version 2.1 of the License.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details .
//
// Author:
//     M. Valin,   Recherche en Prevision Numerique, 2025
//
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <rmn/test_helpers.h>
#include <rmn/split_dimension.h>

int main(int argc, char **argv){
  (void)(argc) ;
  (void)(argv) ;
  int npts, ix, bsize, i, i0, in, ncheck ;
  array_axis axis ;
  index_range range ;

  start_of_test("dimension split test") ;

  bsize = 5 ;
  fprintf(stderr, "=============== 1..3 blocks of size 5 limits <--> ordinal check ===============\n") ;
// block size 5, 1 to 3*bsize elements, verbose test of consistency of block_limits with block_ordinal
  for(npts=1 ; npts <= 3*bsize ; npts+=2){
    axis = split_axis(npts, bsize) ;
    fprintf(stderr, "n = %2d, nblks = %2d, l0 = %2d, ln = %2d", npts, axis.nbk, axis.ln0, axis.ln1) ;
    for(i=0 ; i<axis.nbk ; i++){
      range = block_limits(axis, i) ;
      fprintf(stderr, " [%2d:%2d]", range.ix0, range.ixn) ;
      for(ix=range.ix0 ; ix<=range.ixn ; ix++){
        if(i != block_ordinal(axis, ix)){
          fprintf(stderr, "\n ERROR: index = %d, expecting block ordinal %d, got %d", ix, i, block_ordinal(axis, ix));
          goto fail ;
        }
      }
    }
    fprintf(stderr, "\n") ;
  }
  fprintf(stderr, "SUCCESS\n") ;
  fprintf(stderr, "=============== 1..5 blocks of size 1..16 limits <--> ordinal check ===============\n") ;
// block size 4 to 16, 1 to 5*bsize elements, test consistency of block_limits with block_ordinal
  for(bsize=1 ; bsize<17 ; bsize++){
    for(npts=1 ; npts <= 5*bsize ; npts++){
      axis = split_axis(npts, bsize) ;
      in = -1 ;
      ncheck = 0 ;
      for(i=0 ; i<axis.nbk ; i++){
        range = block_limits(axis, i) ;
        i0 = range.ix0 ;
        if(i0 != in+1){      // hole/overlap in range
          fprintf(stderr, "ERROR (n = %d, bsize %d) : expecting start of block %d at %d, got %d\n", npts, bsize, i, in+1, i0) ;
          goto fail ;
        }
        in = range.ixn ;
        for(ix=i0 ; ix<=in ; ix++){  // check that all elements in block i return i as ordinal
          ncheck++ ;
          if(i != block_ordinal(axis, ix)){
            fprintf(stderr, "ERROR: bsize = %d, index = %d, expecting block ordinal %d, got %d\n", bsize, ix, i, block_ordinal(axis, ix));
            goto fail ;
          }
        }
      }
      if(npts != ncheck){
        fprintf(stderr, "ERROR: n = %d, bsize %d, checked %d\n", npts, bsize, ncheck) ;
        goto fail ;
      }
    }
  }
  fprintf(stderr, "SUCCESS\n") ;
  return 0 ;

fail:
  fprintf(stderr, "FAILED\n") ;
  exit(1) ;
}
