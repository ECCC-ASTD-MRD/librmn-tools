//
// Copyright (C) 2026  Environnement Canada
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
//     M. Valin,   Recherche en Prevision Numerique, 2026
//
// test the memory block movers
//
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <rmn/test_helpers.h>
#include <rmn/move_bhd_blocks.h>

#define GNI 95
#define GNJ 97
#define NI 64
#define NJ 64

int main(int argc, char **argv){
  (void) (argc) ;
  (void) (argv) ;
  uint32_t blocku[NI*NJ] ;
  int32_t  blocki[NI*NJ] ;
  float    blockf[NI*NJ] ;
  uint8_t  src_u8[GNI*GNJ] ;
  int8_t   src_i8[GNI*GNJ] ;
  uint16_t src_u16[GNI*GNJ] ;
  int16_t  src_i16[GNI*GNJ] ;
  uint64_t src_u64[GNI*GNJ] ;
  int64_t  src_i64[GNI*GNJ] ;
  double   src_d64[GNI*GNJ] ;

  start_of_test("bdh block move functions") ;
  fprintf(stderr, "syntax check\n") ;

  move_from_bhd(blocku, src_u8, GNI, NI, NJ) ;
  move_to_bhd(src_u8, blocku, GNI, NI, NJ) ;

  move_from_bhd(blocki, src_i8, GNI, NI, NJ) ;
  move_to_bhd(src_i8, blocki, GNI, NI, NJ) ;

  move_from_bhd(blocku, src_u16, GNI, NI, NJ) ;
  move_to_bhd(src_u16, blocku, GNI, NI, NJ) ;

  move_from_bhd(blocki, src_i16, GNI, NI, NJ) ;
  move_to_bhd(src_i16, blocki, GNI, NI, NJ) ;

  move_from_bhd(blocku, src_u64, GNI, NI, NJ) ;
  move_to_bhd(src_u64, blocku, GNI, NI, NJ) ;

  move_from_bhd(blocki, src_i64, GNI, NI, NJ) ;
  move_to_bhd(src_i64, blocki, GNI, NI, NJ) ;

  move_from_bhd(blockf, src_d64, GNI, NI, NJ) ;
  move_to_bhd(src_d64, blockf, GNI, NI, NJ) ;
}
