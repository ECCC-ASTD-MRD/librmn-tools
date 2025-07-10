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
#include <string.h>

#include <rmn/bitstream.h>

#define CONCAT_(A,B) A##B
#define CONCAT(A,B) CONCAT_(A,B)

// test functions in Big Endian mode
#include <rmn/be_stream.h>
#define PREFIX be_
#include "test_endian_bitstream.h"

// test functions in Little Endian mode
#include <rmn/le_stream.h>
#define PREFIX le_
#include "test_endian_bitstream.h"

int main(int argc, char **argv){
  int err ;

  fprintf(stderr, "============================== %s ", argv[0]);
  while(--argc > 0){
    argv++ ;
    fprintf(stderr, "%s ", argv[0]);
  }
  fprintf(stderr, " (debug = %d) ==============================\n", StreamDebugGet()) ;

  fprintf(stderr, "\n============================== BE test ==============================\n\n") ;
  if((err = be_test()) != 0) goto fail ;

  fprintf(stderr, "\n============================== LE test ==============================\n\n") ;
  if((err = le_test()) != 0) goto fail ;

  fprintf(stderr, "SUCCESS(status = %d)\n", err) ;
  return 0 ;

fail:
  fprintf(stderr, "FAILED (status = %d)\n", err) ;
  exit(1) ;
}
