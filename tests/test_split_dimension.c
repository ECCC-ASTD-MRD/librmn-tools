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
  int npts, ix, bsize, i, i0, in, ncheck, minsize, j ;
  array_axis axis ;
  index_range range ;
  int ax[] = { 7 , 9, 32, 39, 40, 41, 63, 64, 65, 72, 97 } ;
  int bx[] = { 15, 32, 64, 95, 96, 127, 128, 223, 224 } ;

  start_of_test("dimension split test") ;

  bsize = 32 ;
  minsize = 8 ;
  fprintf(stderr, "=============== blocks of size %d, minsize %d check ===============\n", bsize, minsize) ;
  ncheck = sizeof(ax)/sizeof(int) ;
  for(j = 0 ; j < ncheck ; j++){
    axis = split_axis_min(ax[j], bsize, minsize) ;
    fprintf(stderr, "n = %3d, nblks = %3d, l0 = %3d, ln = %3d", ax[j], axis.nbk, axis.ln0, axis.ln1) ;
    for(i=-2 ; i<(axis.nbk+2) ; i++){    // i<0 and i>=axis.nbk are expected to return invalid block limits
      range = block_limits(i, axis) ;
      if(range.ix0 > range.ixn){
        if(i < 0 || i >= axis.nbk) continue ;
        goto fail ;
      }
      fprintf(stderr, " [%3d:%3d]", range.ix0, range.ixn) ;
      for(ix=range.ix0 ; ix<=range.ixn ; ix++){
        if(i != block_ordinal(ix, axis)){
          fprintf(stderr, "\n ERROR: index = %d, expecting block ordinal %d, got %d", ix, i, block_ordinal(ix, axis));
          goto fail ;
        }
      }
    }
    fprintf(stderr, "\n") ;
  }

  bsize = 64 ;
  minsize = 32 ;
  fprintf(stderr, "=============== blocks of size %d, minsize %d check ===============\n", bsize, minsize) ;
  ncheck = sizeof(bx)/sizeof(int) ;
  for(j = 0 ; j < ncheck ; j++){
    axis = split_axis_min(bx[j], bsize, minsize) ;
    fprintf(stderr, "n = %3d, nblks = %3d, l0 = %3d, ln = %3d", bx[j], axis.nbk, axis.ln0, axis.ln1) ;
    for(i=-1 ; i<axis.nbk+1 ; i++){
      range = block_limits(i, axis) ;
      if(range.ix0 > range.ixn){
        if(i < 0 || i >= axis.nbk) continue ;
        goto fail ;
      }
      fprintf(stderr, " [%3d:%3d]", range.ix0, range.ixn) ;
      for(ix=range.ix0 ; ix<=range.ixn ; ix++){
        if(i != block_ordinal(ix, axis)){
          fprintf(stderr, "\n ERROR: index = %d, expecting block ordinal %d, got %d", ix, i, block_ordinal(ix, axis));
          goto fail ;
        }
      }
    }
    fprintf(stderr, "\n") ;
  }

  bsize = 5 ;
  fprintf(stderr, "=============== 1..3 blocks of size 5 limits <--> ordinal check ===============\n") ;
// block size 5, 1 to 3*bsize elements, verbose test of consistency of block_limits with block_ordinal
  for(npts=1 ; npts <= 3*bsize ; npts+=2){
    axis = split_axis_min(npts, bsize, (bsize+1)/2) ;
//     axis = split_axis(npts, bsize) ;
    fprintf(stderr, "n = %3d, nblks = %3d, l0 = %3d, ln = %3d", npts, axis.nbk, axis.ln0, axis.ln1) ;
    for(i=-1 ; i<axis.nbk+1 ; i++){
      range = block_limits(i, axis) ;
      if(range.ix0 > range.ixn){
        if(i < 0 || i >= axis.nbk) continue ;
        goto fail ;
      }
      fprintf(stderr, " [%3d:%3d]", range.ix0, range.ixn) ;
      for(ix=range.ix0 ; ix<=range.ixn ; ix++){
        if(i != block_ordinal(ix, axis)){
          fprintf(stderr, "\n ERROR: index = %d, expecting block ordinal %d, got %d", ix, i, block_ordinal(ix, axis));
          goto fail ;
        }
      }
    }
    fprintf(stderr, "\n") ;
  }
  fprintf(stderr, "SUCCESS\n") ;
  fprintf(stderr, "=============== 1..5 blocks of size 1..16 limits <--> ordinal check ===============\n") ;
// block size 2 to 16, 1 to 5*bsize elements, test consistency of block_limits with block_ordinal
  for(bsize=2 ; bsize<17 ; bsize++){
    for(npts=1 ; npts <= 5*bsize ; npts++){
      axis = split_axis_min(npts, bsize, bsize/2) ;
//       axis = split_axis(npts, bsize) ;
//       fprintf(stderr, "n = %3d, nblks = %3d, l0 = %3d, ln = %3d", npts, axis.nbk, axis.ln0, axis.ln1) ;
      in = -1 ;
      ncheck = 0 ;
      for(i=-1 ; i<axis.nbk+1 ; i++){
        range = block_limits(i, axis) ;
        if(range.ix0 > range.ixn){
          if(i < 0 || i >= axis.nbk) continue ;
          fprintf(stderr, "ERROR : i = %d, nbk = %d, range - %3d:%3d\n", i, axis.nbk, range.ix0, range.ixn) ;
          goto fail ;
        }
//         fprintf(stderr, " [%3d:%3d]", range.ix0, range.ixn) ;
        i0 = range.ix0 ;
        if(i0 != in+1){      // hole/overlap in range
          fprintf(stderr, "ERROR (n = %d, bsize %d) : expecting start of block %d at %d, got %d\n", npts, bsize, i, in+1, i0) ;
          goto fail ;
        }
        in = range.ixn ;
        for(ix=i0 ; ix<=in ; ix++){  // check that all elements in block i return i as ordinal
          ncheck++ ;
          if(i != block_ordinal(ix, axis)){
            fprintf(stderr, "ERROR: bsize = %d, index = %d, expecting block ordinal %d, got %d\n", bsize, ix, i, block_ordinal(ix, axis));
            goto fail ;
          }
        }
      }
//       fprintf(stderr, "\n") ;
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
