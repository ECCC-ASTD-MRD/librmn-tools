// Hopefully useful code for C
// Copyright (C) 2024  Recherche en Prevision Numerique
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
// Author:
//     M. Valin,   Recherche en Prevision Numerique, 2024
//
#include <stdio.h>
#include <stdlib.h>
#include <rmn/data_map.h>

#define NTI 10
#define NTJ 11
#define SF0  4

// #define NTI  4
// #define NTJ  3
// #define SF0  2

int main(int argc, char **argv){
  int i, j, x[NTI], y[NTI] ;
  ij_index ij ;
  for(j=NTJ-1 ; j>=0 ; j--){ 
    for(i=0 ; i<NTI ; i++) { 
      x[i] = Zindex_from_i_j(i, j, NTI, NTJ, SF0) ;
      y[i] = Zindex_from_i_j_(i, j, NTI, NTJ, SF0) ;
      ij   = Zindex_to_i_j_(y[i], NTI, NTJ, SF0) ;
      if(ij.i != i || ij.j != j){
        fprintf(stderr, "ERROR: zij = %3d, expecting i,j = (%2d,%2d), got (%2d,%2d)\n", x[i], i, j, ij.i, ij.j) ;
        exit(1) ;
      }
    }
    for(i=0 ; i<NTI ; i++) { fprintf(stderr, "+------"             ) ; } fprintf(stderr, "+\n") ;
//     for(i=0 ; i<NTI ; i++) { fprintf(stderr, "|      "             ) ; } fprintf(stderr, "|\n") ;
    for(i=0 ; i<NTI ; i++) { fprintf(stderr, "| %3d  " ,       x[i]) ; } fprintf(stderr, "| (Z index)\n") ;
    for(i=0 ; i<NTI ; i++) { fprintf(stderr, "|%2d,%3d",    i,    j) ; } fprintf(stderr, "| (expected i,j)\n") ;
    for(i=0 ; i<NTI ; i++) { 
      ij   = Zindex_to_i_j(x[i], NTI, NTJ, SF0) ;
      fprintf(stderr, "|%2d,%3d", ij.i, ij.j) ; 
    } fprintf(stderr, "| (computed i,j)\n") ;
//     for(i=0 ; i<NTI ; i++) { fprintf(stderr, "|      "             ) ; } fprintf(stderr, "|\n") ;
  }
  for(i=0 ; i<NTI ; i++) { fprintf(stderr, "+------"        ) ; } fprintf(stderr, "+\n") ;
}
