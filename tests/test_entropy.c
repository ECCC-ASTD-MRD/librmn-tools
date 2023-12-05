// Copyright (C) 2023  Environnement Canada
//
// This is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation,
// version 2.1 of the License.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// Author:
//     M. Valin,   Recherche en Prevision Numerique, 2023
//

#include <stdio.h>
#include <rmn/entropy.h>
#include <rmn/entropy.h>
#include <rmn/tee_print.h>
#include <rmn/test_helpers.h>

#define NPTS 32

int main(int argc, char **argv){
  int data[NPTS] ;
  int i, np ;
  entropy_table *etab ;
  float entropy ;

  start_of_test("C entropy test");

  etab = NewEntropyTable(NPTS, 0x0000001Fu) ;
  ResetEntropyTable(etab) ;

  for(i=0 ; i<NPTS ; i++) {
//     data[i] = (i < NPTS/4) ? 15 : 1 ;
//     data[i] = (i < NPTS/2) ? 15 : 1 ;
    data[i] = i & 0x1E ;
  }

  for(i=0 ; i < NPTS ; i++) {
    np = UpdateEntropyTable(etab, data, NPTS - i) ;
//     np = UpdateEntropyTable(etab, data, NPTS) ;
//     fprintf(stderr, "np = %d, expecting %d\n", np, NPTS - i) ;
  }

  entropy = ComputeEntropy(etab) ;
  fprintf(stderr, "value\n");
  for(i=0 ; i<etab->size ; i++) fprintf(stderr, "%4d", i) ; fprintf(stderr, "\n") ;
  fprintf(stderr, "population\n");
  for(i=0 ; i<etab->size ; i++) fprintf(stderr, "%4d", etab->pop[i]) ; fprintf(stderr, "\n") ;
  fprintf(stderr, "computed entropy for %d values = %5.2f bits/value\n", etab->npop, entropy) ;
}
