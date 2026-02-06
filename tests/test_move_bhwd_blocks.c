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
#include <string.h>

#include <rmn/test_helpers.h>
#include <rmn/move_bhwd_blocks.h>

// #define GNI 95
// #define GNJ 97
#define GNI 64
#define GNJ 64
#define NI 64
#define NJ 64

void set_8(void *new_, int n){
  uint8_t *new = (uint8_t *)new_ ;
  for(int i=0 ; i<n ; i++) new[i] = 0 ;
}

void set_16(void *new_, int n){
  uint16_t *new = (uint16_t *)new_ ;
  for(int i=0 ; i<n ; i++) new[i] = 0 ;
}

void set_32(void *new_, int n){
  uint32_t *new = (uint32_t *)new_ ;
  for(int i=0 ; i<n ; i++) new[i] = 0 ;
}

void set_64(void *new_, int n){
  uint64_t *new = (uint64_t *)new_ ;
  for(int i=0 ; i<n ; i++) new[i] = 0 ;
}

int check_8(void *ref_, void *new_, int n){
  uint8_t *ref = (uint8_t *)ref_, *new = (uint8_t *)new_ ;
  int diff = 0 ;
  for(int i=0 ; i<n ; i++){
    if(ref[i] != new[i]) diff++ ;
    if(diff == 1) fprintf(stderr, "i = %d, expecting %4.4x, got %4.4x, ", i, ref[i], new[i]) ;
  }
  if(diff) fprintf(stderr, "%d differences\n", diff) ;
  return diff ;
}

int check_16(void *ref_, void *new_, int n){
  uint16_t *ref = (uint16_t *)ref_, *new = (uint16_t *)new_ ;
  int diff = 0 ;
  for(int i=0 ; i<n ; i++){
    if(ref[i] != new[i]) diff++ ;
  }
  return diff ;
}

int check_32(void *ref_, void *new_, int n){
  uint32_t *ref = (uint32_t *)ref_, *new = (uint32_t *)new_ ;
  int diff = 0 ;
  for(int i=0 ; i<n ; i++){
    if(ref[i] != new[i]) diff++ ;
    if(diff == 1) fprintf(stderr, "i = %d, expecting %8.8x, got %8.8x, ", i, ref[i], new[i]) ;
  }
  if(diff) fprintf(stderr, "%d differences\n", diff) ;
  return diff ;
}

int check_64(void *ref_, void *new_, int n){
  uint64_t *ref = (uint64_t *)ref_, *new = (uint64_t *)new_ ;
  int diff = 0 ;
  for(int i=0 ; i<n ; i++){
    if(ref[i] != new[i]) diff++ ;
    if(diff == 1) fprintf(stderr, "i = %d, expecting %16.16lx, got %16.16lx, ", i, ref[i], new[i]) ;
  }
  if(diff) fprintf(stderr, "%d differences\n", diff) ;
  return diff ;
}

int main(int argc, char **argv){
  (void) (argc) ;
  (void) (argv) ;
  uint32_t blocku[NI*NJ] ;
  int32_t  blocki[NI*NJ] ;
  float    blockf[NI*NJ] ;
  uint8_t  src_u8[GNI*GNJ], rst_u8[GNI*GNJ] ;
  int8_t   src_i8[GNI*GNJ], rst_i8[GNI*GNJ] ; ;
  uint16_t src_u16[GNI*GNJ], rst_u16[GNI*GNJ] ;
  int16_t  src_i16[GNI*GNJ], rst_i16[GNI*GNJ] ;
  uint32_t src_u32[GNI*GNJ], rst_u32[GNI*GNJ] ;
  int32_t  src_i32[GNI*GNJ], rst_i32[GNI*GNJ] ;
  float    src_f32[GNI*GNJ], rst_f32[GNI*GNJ] ;
  uint64_t src_u64[GNI*GNJ], rst_u64[GNI*GNJ] ;
  int64_t  src_i64[GNI*GNJ], rst_i64[GNI*GNJ] ;
  double   src_d64[GNI*GNJ], rst_d64[GNI*GNJ] ;
  char dummy[2] ;
  bhwd_fn fwd, inv ;
  uint32_t i ;
  char *msg ;

  start_of_test("bdh block move functions") ;

// fill arrays
  for(i=0 ; i<GNI*GNJ ; i++){
    src_u8[i]  = i & 0xFF ;
    src_u16[i] = i & 0xFFFF ;
    src_u32[i] = i ;
    src_u64[i] = src_u32[i] ;
    src_i8[i]  = i & 0xFF - 128 ;
    src_i16[i] = i - (GNI*GNJ)/2 ;
    src_i32[i] = i - (GNI*GNJ)/2 ;
    src_i64[i] = src_i32[i] ;
    src_f32[i] = i ;
    src_d64[i] = i ;
  }

// syntax check

  fprintf(stderr, "char code = %d(%s), fn = %p/%p, '%s/%s'\n",
          block_type(dummy), block_type_name(dummy[0]), to_block_fn(dummy), from_block_fn(dummy), to_block_name(dummy), from_block_name(dummy));

  fprintf(stderr, "u8 code = %d(%s), fn = '%s/%s'\n",
          block_type(src_u8), block_type_name(src_u8[0]), to_block_name(src_u8), from_block_name(src_u8)) ;

  msg = "src_u8" ; set_8(rst_u8, GNI*GNJ) ; set_32(blocku, NI*NJ) ;
  bhwd2block(blocku, src_u8, GNI, NI, NJ) ;  // unsigned 8 bit
  block2bhwd(rst_u8, blocku, GNI, NI, NJ) ;
  if(check_8(src_u8, rst_u8, GNI*GNJ) != 0) goto fail ;
  fprintf(stderr, "SUCCESS: u8\n") ;

  fwd = to_block_fn(src_u8) ;
  block_fn(fwd, blocku, src_u8, GNI, NI, NJ) ;
  inv = from_block_fn(src_u8) ;
  block_fn(inv, src_u8, blocku, GNI, NI, NJ) ;

  uint32_t *tdum1 = w32_cast(blockf, src_u8)  ;
  bhwd2block(tdum1, src_u8, GNI, NI, NJ) ;                     // block should be unsigned 32 bit
  block2bhwd(src_u8, tdum1, GNI, NI, NJ) ;                     // block should be unsigned 32 bit
  bhwd2block(w32_cast(blockf, src_u8), src_u8, GNI, NI, NJ) ;  // block should be unsigned 32 bit
  block2bhwd(src_u8, w32_cast(blockf, src_u8), GNI, NI, NJ) ;  // block should be unsigned 32 bit
  bhwd2block((uint32_t *)blockf, src_u8, GNI, NI, NJ) ;        // block should be unsigned 32 bit
  block2bhwd(src_u8, (uint32_t *)blockf, GNI, NI, NJ) ;        // block should be unsigned 32 bit

  fprintf(stderr, "i8 code = %d(%s), fn = '%s/%s'\n",
          block_type(src_i8), block_type_name(src_i8[0]), to_block_name(src_i8), from_block_name(src_i8)) ;
  msg = "src_i8" ; set_8(rst_i8, GNI*GNJ) ; set_32(blocki, NI*NJ) ;
  fwd = to_block_fn(src_i8) ;
  inv = from_block_fn(src_i8) ;
  bhwd2block(blocki, src_i8, GNI, NI, NJ) ;  // signed 8 bit
  block2bhwd(rst_i8, blocki, GNI, NI, NJ) ;
  if(check_8(src_i8, rst_i8, GNI*GNJ) != 0) goto fail ;
  fprintf(stderr, "SUCCESS: i8\n") ;

  msg = "src_u16" ; set_16(rst_u16, GNI*GNJ) ; set_32(blocku, NI*NJ) ;
  bhwd2block(blocku, src_u16, GNI, NI, NJ) ;  // unsigned 16 bit
  block2bhwd(rst_u16, blocku, GNI, NI, NJ) ;
  if(check_16(src_u16, rst_u16, GNI*GNJ) != 0) goto fail ;
  fprintf(stderr, "SUCCESS: u16\n") ;

  msg = "src_i16" ; set_16(rst_i16, GNI*GNJ) ; set_32(blocki, NI*NJ) ;
  bhwd2block(blocki, src_i16, GNI, NI, NJ) ;  // signed 16 bit
  block2bhwd(rst_i16, blocki, GNI, NI, NJ) ;
   if(check_16(src_u16, rst_u16, GNI*GNJ) != 0) goto fail ;
  fprintf(stderr, "SUCCESS: i16\n") ;

  msg = "src_u32" ; set_32(rst_u32, GNI*GNJ) ; set_32(blocku, NI*NJ) ;
  bhwd2block(blocku, src_u32, GNI, NI, NJ) ;  // unsigned 32 bit
  block2bhwd(rst_u32, blocku, GNI, NI, NJ) ;
  if(check_32(src_u32, rst_u32, GNI*GNJ) != 0) goto fail ;
  fprintf(stderr, "SUCCESS: u32\n") ;

  msg = "src_i32" ; set_32(rst_i32, GNI*GNJ) ; set_32(blocki, NI*NJ) ;
  bhwd2block(blocki, src_i32, GNI, NI, NJ) ;  // signed 32 bit
  block2bhwd(rst_i32, blocki, GNI, NI, NJ) ;
  if(check_32(src_i32, rst_i32, GNI*GNJ) != 0) goto fail ;
  fprintf(stderr, "SUCCESS: i32\n") ;

  msg = "src_f32" ; set_32(rst_f32, GNI*GNJ) ; set_32(blockf, NI*NJ) ;
  bhwd2block(blockf, src_f32, GNI, NI, NJ) ;  // float 32 bit
  block2bhwd(rst_f32, blockf, GNI, NI, NJ) ;
  if(check_32(src_f32, rst_f32, GNI*GNJ) != 0) goto fail ;
  fprintf(stderr, "SUCCESS: f32\n") ;

  msg = "src_u64" ; set_64(rst_u64, GNI*GNJ) ; set_64(blocku, NI*NJ) ;
  bhwd2block(blocku, src_u64, GNI, NI, NJ) ;  // unsigned 64 bit
  block2bhwd(rst_u64, blocku, GNI, NI, NJ) ;
  if(check_64(src_u64, rst_u64, GNI*GNJ) != 0) goto fail ;
  fprintf(stderr, "SUCCESS: u64\n") ;

  msg = "src_i64" ; set_64(rst_i64, GNI*GNJ) ; set_64(blocki, NI*NJ) ;
  bhwd2block(blocki, src_i64, GNI, NI, NJ) ;  // signed 64 bit
  block2bhwd(rst_i64, blocki, GNI, NI, NJ) ;
  if(check_64(src_i64, rst_i64, GNI*GNJ) != 0) goto fail ;
  fprintf(stderr, "SUCCESS: i64\n") ;

  fprintf(stderr, "d64 code = %d(%s), fn = '%s/%s'\n",
          block_type(src_d64), block_type_name(src_d64[0]), to_block_name(src_d64), from_block_name(src_d64)) ;
  msg = "src_d64" ; set_64(rst_d64, GNI*GNJ) ; set_64(blockf, NI*NJ) ;
  bhwd2block(blockf, src_d64, GNI, NI, NJ) ;  // double 64 bit
  block2bhwd(rst_d64, blockf, GNI, NI, NJ) ;
  if(check_64(src_d64, rst_d64, GNI*GNJ) != 0) goto fail ;
  fprintf(stderr, "SUCCESS: d64\n") ;

  return 0 ;

fail :
  fprintf(stderr, "ERROR : %s\n", msg) ;
  return 1 ;
}
