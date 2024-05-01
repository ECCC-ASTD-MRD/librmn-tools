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

int main(int argc, char **argv){
  int i, j, x[NTI] ;
  for(j=NTJ-1 ; j>=0 ; j--){ 
    for(i=0 ; i<NTI ; i++) { x[i] = Zindex_from_i_j_(i, j, NTI, NTJ, SF0) ; }
    for(i=0 ; i<NTI ; i++) { fprintf(stderr, "+-----"      ) ; } fprintf(stderr, "+\n") ;
    for(i=0 ; i<NTI ; i++) { fprintf(stderr, "|     "      ) ; } fprintf(stderr, "|\n") ;
    for(i=0 ; i<NTI ; i++) { fprintf(stderr, "| %3d ", x[i]) ; } fprintf(stderr, "|\n") ;
    for(i=0 ; i<NTI ; i++) { fprintf(stderr, "|     "      ) ; } fprintf(stderr, "|\n") ;
  }
  for(i=0 ; i<NTI ; i++) { fprintf(stderr, "+-----"        ) ; } fprintf(stderr, "+\n") ;
}
