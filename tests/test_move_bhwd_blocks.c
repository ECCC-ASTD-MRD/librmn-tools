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
#include <rmn/move_bhwd_blocks.h>

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
  uint32_t src_u32[GNI*GNJ] ;
  int32_t  src_i32[GNI*GNJ] ;
  float    src_f32[GNI*GNJ] ;
  uint64_t src_u64[GNI*GNJ] ;
  int64_t  src_i64[GNI*GNJ] ;
  double   src_d64[GNI*GNJ] ;
  char dummy[2] ;
  bhwd_fn fwd, inv ;
//   int kind ;

  start_of_test("bdh block move functions") ;

// syntax check

  fprintf(stderr, "char code = %d(%s), fn = %p/%p, '%s/%s'\n",
          block_type(dummy), block_type_name(dummy[0]), to_block_fn(dummy), from_block_fn(dummy), to_block_name(dummy), from_block_name(dummy));

  fprintf(stderr, "u8 code = %d(%s), fn = '%s/%s'\n",
          block_type(src_u8), block_type_name(src_u8[0]), to_block_name(src_u8), from_block_name(src_u8)) ;
  fwd = to_block_fn(src_u8) ;
  inv = from_block_fn(src_u8) ;
  bhwd2block(blocku, src_u8, GNI, NI, NJ) ;  // unsigned 8 bit
  (*fwd)(blocku, src_u8, GNI, NI, NJ) ;
  block2bhwd(src_u8, blocku, GNI, NI, NJ) ;
  (*inv)(src_u8, blocku, GNI, NI, NJ) ;

  bhwd2block(blockf, src_u8, GNI, NI, NJ) ;  // failing unsigned 8 bit

  fprintf(stderr, "i8 code = %d(%s), fn = '%s/%s'\n",
          block_type(src_i8), block_type_name(src_i8[0]), to_block_name(src_i8), from_block_name(src_i8)) ;
  fwd = to_block_fn(src_i8) ;
  inv = from_block_fn(src_i8) ;
  bhwd2block(blocki, src_i8, GNI, NI, NJ) ;  // signed 8 bit
  (*fwd)(blocku, src_i8, GNI, NI, NJ) ;
  block2bhwd(src_i8, blocki, GNI, NI, NJ) ;
  (*inv)(src_i8, blocku, GNI, NI, NJ) ;

  bhwd2block(blocku, src_u16, GNI, NI, NJ) ;  // unsigned 16 bit
  block2bhwd(src_u16, blocku, GNI, NI, NJ) ;

  bhwd2block(blocki, src_i16, GNI, NI, NJ) ;  // signed 16 bit
  block2bhwd(src_i16, blocki, GNI, NI, NJ) ;

  bhwd2block(blocku, src_u32, GNI, NI, NJ) ;  // unsigned 32 bit
  block2bhwd(src_u32, blocku, GNI, NI, NJ) ;

  bhwd2block(blocki, src_i32, GNI, NI, NJ) ;  // signed 32 bit
  block2bhwd(src_i32, blocki, GNI, NI, NJ) ;

  bhwd2block(blockf, src_f32, GNI, NI, NJ) ;  // float 32 bit
  block2bhwd(src_f32, blockf, GNI, NI, NJ) ;

  bhwd2block(blocku, src_u64, GNI, NI, NJ) ;  // unsigned 64 bit
  block2bhwd(src_u64, blocku, GNI, NI, NJ) ;

  bhwd2block(blocki, src_i64, GNI, NI, NJ) ;  // signed 64 bit
  block2bhwd(src_i64, blocki, GNI, NI, NJ) ;

  fprintf(stderr, "d64 code = %d(%s), fn = '%s/%s'\n",
          block_type(src_d64), block_type_name(src_d64[0]), to_block_name(src_d64), from_block_name(src_d64)) ;
  fwd = to_block_fn(src_d64) ;
  inv = from_block_fn(src_d64) ;
  bhwd2block(blockf, src_d64, GNI, NI, NJ) ;  // double 64 bit
  (*fwd)(blockf, src_d64, GNI, NI, NJ) ;
  block2bhwd(src_d64, blockf, GNI, NI, NJ) ;
  (*inv)(src_d64, blockf, GNI, NI, NJ) ;

}
