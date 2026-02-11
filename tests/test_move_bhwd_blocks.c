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
#include <rmn/timers.h>

#define GNI 195
#define GNJ 97
// #define GNI 64
// #define GNJ 64
#define NI 64
#define NJ 64

#define DNIJ ((GNI-NI)/3 + (GNI*((GNJ-NJ)/5)))

#define OR_1 0x00

#define NITER 5000

void  print_minmax(zminmax tmm){
  fprintf(stderr, "minu = %8.8x, maxu = %8.8x, mins = %8.8x, maxs = %8.8x, zeros = %d\n",
          tmm.minu, tmm.maxu, tmm.mins, tmm.maxs, tmm.zero) ;
}

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

int nz_8(void *f_, int n){
  uint8_t *f = (uint8_t *) f_ ;
  int nz = 0 ;
  for(int i=0 ; i<n ; i++){ if(f[i] != 0) nz++ ; }
  return nz ;
}

int check_8_(int lni, int ni, int nj,  uint8_t ref[nj][lni],  uint8_t new[nj][lni]){
  int diff = 0 ;
  for(int j=0 ; j<nj ; j++){
    for(int i=0 ; i<ni ; i++){
      if(ref[j][i] != new[j][i]) diff++ ;
      if(diff == 1) fprintf(stderr, "i,j = [%d,%d], expecting %4.4x, got %4.4x, ", i, j, ref[j][i], new[i][i]) ;
    }
  }
  if(diff) fprintf(stderr, "%d differences\n", diff) ;
  return diff ;
}

int check_8(void *ref, void *new, int n){
  (void) (n) ;
  return check_8_(GNI, NI, NJ, ref,  new) ;
}

int nz_16(void *f_, int n){
  uint16_t *f = (uint16_t *) f_ ;
  int nz = 0 ;
  for(int i=0 ; i<n ; i++){ if(f[i] != 0) nz++ ; }
  return nz ;
}

int check_16_(int lni, int ni, int nj,  uint16_t ref[nj][lni],  uint16_t new[nj][lni]){
  int diff = 0 ;
  for(int j=0 ; j<nj ; j++){
    for(int i=0 ; i<ni ; i++){
      if(ref[j][i] != new[j][i]) diff++ ;
      if(diff == 1) fprintf(stderr, "i,j = [%d,%d], expecting %8.8x, got %8.8x, ", i, j, ref[j][i], new[i][i]) ;
    }
  }
  if(diff) fprintf(stderr, "[%d,%d] %d differences\n", ni, nj, diff) ;
  return diff ;
}

int check_16(void *ref, void *new, int n){
  (void) (n) ;
  return check_16_(GNI, NI, NJ, ref,  new) ;
}

int nz_32(void *f_, int n){
  uint32_t *f = (uint32_t *) f_ ;
  int nz = 0 ;
  for(int i=0 ; i<n ; i++){ if(f[i] != 0) nz++ ; }
  return nz ;
}

int check_32_(int lni, int ni, int nj,  uint32_t ref[nj][lni],  uint32_t new[nj][lni]){
  int diff = 0 ;
  for(int j=0 ; j<nj ; j++){
    for(int i=0 ; i<ni ; i++){
      if(ref[j][i] != new[j][i]) diff++ ;
      if(diff == 1) fprintf(stderr, "i,j = [%d,%d], expecting %8.8x, got %8.8x, ", i, j, ref[j][i], new[i][i]) ;
    }
  }
  if(diff) fprintf(stderr, "%d differences\n", diff) ;
  return diff ;
}

int check_32(void *ref, void *new, int n){
  (void) (n) ;
  return check_32_(GNI, NI, NJ, ref,  new) ;
}

int nz_64(void *f_, int n){
  uint64_t *f = (uint64_t *) f_ ;
  int nz = 0 ;
  for(int i=0 ; i<n ; i++){ if(f[i] != 0) nz++ ; }
  return nz ;
}

int check_64_(int lni, int ni, int nj,  uint64_t ref[nj][lni],  uint64_t new[nj][lni]){
  int diff = 0 ;
  for(int j=0 ; j<nj ; j++){
    for(int i=0 ; i<ni ; i++){
      if(ref[j][i] != new[j][i]) diff++ ;
      if(diff == 1) fprintf(stderr, "i,j = [%d,%d], expecting %16.16lx, got %16.16lx, ", i, j, ref[j][i], new[i][i]) ;
    }
  }
  if(diff) fprintf(stderr, "%d differences\n", diff) ;
//   fprintf(stderr, "%d differences, [%d,%d]\n", diff, ni, nj) ;
  return diff ;
}

int check_64(void *ref, void *new, int n){
  (void) (n) ;
  return check_64_(GNI, NI, NJ, ref,  new) ;
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
  TIME_LOOP_DATA ;
  zminmax tmm ;

  start_of_test("bdh block move functions") ;
  fprintf(stderr, "global values = %d, block values = %d\n", GNI*GNJ, NI*NJ) ;

// fill arrays
  for(i=0 ; i<GNI*GNJ ; i++){
    src_u8[i]  = ( OR_1 | (i & 0xFF)) ;
    src_u16[i] = ( OR_1 | (i & 0xFFFF)) ;
    src_u32[i] = ( OR_1 | (i)) ;
    src_u64[i] = ( OR_1 | (src_u32[i])) ;
    src_i8[i]  = ( OR_1 | ((i & 0xFF) - 128)) ;
    src_i16[i] = ( OR_1 | (i - (GNI*GNJ)/2)) ;
    src_i32[i] = ( OR_1 | (i - (GNI*GNJ)/2)) ;
    src_i64[i] = ( OR_1 | (src_i32[i])) ;
    src_f32[i] = ( OR_1 | (i)) ;
    if(i & 1) src_f32[i] = (-src_f32[i]) ;
    src_d64[i] = src_f32[i] ;
  }

// syntax check

  fprintf(stderr, "char code = %2d(%s), fn = %p/%p, '%s/%s'\n",
          block_type(dummy), block_type_name(dummy[0]), to_block_fn(dummy), from_block_fn(dummy), to_block_name(dummy), from_block_name(dummy));

  fprintf(stderr, "u8  code = %2d(%8s), fn = '%15s / %15s'\n",
          block_type(src_u8), block_type_name(src_u8[0]), to_block_name(src_u8), from_block_name(src_u8)) ;

  fprintf(stderr, "i8  code = %2d(%8s), fn = '%15s / %15s'\n",
          block_type(src_i8), block_type_name(src_i8[0]), to_block_name(src_i8), from_block_name(src_i8)) ;

  fprintf(stderr, "u16 code = %2d(%8s), fn = '%15s / %15s'\n",
          block_type(src_u16), block_type_name(src_u16[0]), to_block_name(src_u16), from_block_name(src_u16)) ;

  fprintf(stderr, "i16 code = %2d(%8s), fn = '%15s / %15s'\n",
          block_type(src_i16), block_type_name(src_i16[0]), to_block_name(src_i16), from_block_name(src_i16)) ;

  fprintf(stderr, "u32 code = %2d(%8s), fn = '%15s / %15s'\n",
          block_type(src_u32), block_type_name(src_u32[0]), to_block_name(src_u32), from_block_name(src_u32)) ;

  fprintf(stderr, "i32 code = %2d(%8s), fn = '%15s / %15s'\n",
          block_type(src_i32), block_type_name(src_i32[0]), to_block_name(src_i32), from_block_name(src_i32)) ;

  fprintf(stderr, "f32 code = %2d(%8s), fn = '%15s / %15s'\n",
          block_type(src_f32), block_type_name(src_f32[0]), to_block_name(src_f32), from_block_name(src_f32)) ;

  fprintf(stderr, "u64 code = %2d(%8s), fn = '%15s / %15s'\n",
          block_type(src_u64), block_type_name(src_u64[0]), to_block_name(src_u64), from_block_name(src_u64)) ;

  fprintf(stderr, "i64 code = %2d(%8s), fn = '%15s / %15s'\n",
          block_type(src_i64), block_type_name(src_i64[0]), to_block_name(src_i64), from_block_name(src_i64)) ;

  fprintf(stderr, "d64 code = %2d(%8s), fn = '%15s / %15s'\n",
          block_type(src_d64), block_type_name(src_d64[0]), to_block_name(src_d64), from_block_name(src_d64)) ;

  fprintf(stderr, "\n") ;

  msg = "src_u8-1" ; set_8(rst_u8, GNI*GNJ) ; set_32(blocku, NI*NJ) ;
  bhwd2block(blocku, src_u8, GNI, NI, NJ) ;  // unsigned 8 bit
  block2bhwd(rst_u8, blocku, GNI, NI, NJ) ;
  if(check_8(src_u8, rst_u8, GNI*GNJ) != 0) goto fail ;
  msg = "src_u2" ; set_8(rst_u8, GNI*GNJ) ; set_32(blocku, NI*NJ) ;
  bhwd2block(blocku, src_u8+DNIJ, GNI, NI, NJ) ;  // unsigned 8 bit
  block2bhwd(rst_u8+DNIJ, blocku, GNI, NI, NJ) ;
  if(check_8(src_u8+DNIJ, rst_u8+DNIJ, GNI*GNJ) != 0) goto fail ;
  msg = "src_u8-3" ; set_8(rst_u8, GNI*GNJ) ; set_32(blocku, NI*NJ) ;
  bhwd2block(blocku, src_u8, GNI, NI, NJ) ;  // unsigned 8 bit
  block2bhwd(rst_u8, blocku, GNI, NI, NJ) ;
  if(check_8(src_u8, rst_u8, GNI*GNJ) != 0) goto fail ;
  TIME_LOOP_EZ(NITER, (NI*NJ), bhwd2block(blocku, src_u8, GNI, NI, NJ) ) ;
  fprintf(stderr, "%s", timer_msg) ;
  TIME_LOOP_EZ(NITER, (NI*NJ), block2bhwd(rst_u8, blocku, GNI, NI, NJ) ) ;
  fprintf(stderr, " | %s\n", timer_msg) ;
  tmm = block_zminmax((void *)blocku, NI*NJ) ;
  print_minmax(tmm) ;
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

  fwd = to_block_fn(src_i8) ;
  inv = from_block_fn(src_i8) ;
  msg = "src_i8-1" ; set_8(rst_i8, GNI*GNJ) ; set_32(blocki, NI*NJ) ;
  bhwd2block(blocki, src_i8, GNI, NI, NJ) ;  // signed 8 bit
  block2bhwd(rst_i8, blocki, GNI, NI, NJ) ;
  if(check_8(src_i8, rst_i8, GNI*GNJ) != 0) goto fail ;
  msg = "src_i8-2" ; set_8(rst_i8, GNI*GNJ) ; set_32(blocki, NI*NJ) ;
  bhwd2block(blocki, src_i8+DNIJ, GNI, NI, NJ) ;  // signed 8 bit
  block2bhwd(rst_i8+DNIJ, blocki, GNI, NI, NJ) ;
  if(check_8(src_i8+DNIJ, rst_i8+DNIJ, GNI*GNJ) != 0) goto fail ;
  msg = "src_i8-2" ; set_8(rst_i8, GNI*GNJ) ; set_32(blocki, NI*NJ) ;
  bhwd2block(blocki, src_i8, GNI, NI, NJ) ;  // signed 8 bit
  block2bhwd(rst_i8, blocki, GNI, NI, NJ) ;
  if(check_8(src_i8, rst_i8, GNI*GNJ) != 0) goto fail ;
  TIME_LOOP_EZ(NITER, (NI*NJ), bhwd2block(blocki, src_i8, GNI, NI, NJ)  ) ;
  fprintf(stderr, "%s", timer_msg) ;
  TIME_LOOP_EZ(NITER, (NI*NJ), block2bhwd(rst_i8, blocki, GNI, NI, NJ) ) ;
  fprintf(stderr, " | %s\n", timer_msg) ;
  tmm = block_zminmax((void *)blocki, NI*NJ) ;
  print_minmax(tmm) ;
  fprintf(stderr, "SUCCESS: i8\n") ;

  msg = "src_u16-1" ; set_16(rst_u16, GNI*GNJ) ; set_32(blocku, NI*NJ) ;
  bhwd2block(blocku, src_u16, GNI, NI, NJ) ;  // unsigned 16 bit
  block2bhwd(rst_u16, blocku, GNI, NI, NJ) ;
  if(check_16(src_u16, rst_u16, GNI*GNJ) != 0) goto fail ;
  msg = "src_u16-2" ; set_16(rst_u16, GNI*GNJ) ; set_32(blocku, NI*NJ) ;
  bhwd2block(blocku, src_u16+DNIJ, GNI, NI, NJ) ;  // unsigned 16 bit
  block2bhwd(rst_u16+DNIJ, blocku, GNI, NI, NJ) ;
  if(check_16(src_u16+DNIJ, rst_u16+DNIJ, GNI*GNJ) != 0) goto fail ;
  msg = "src_u16-3" ; set_16(rst_u16, GNI*GNJ) ; set_32(blocku, NI*NJ) ;
  bhwd2block(blocku, src_u16, GNI, NI, NJ) ;  // unsigned 16 bit
  block2bhwd(rst_u16, blocku, GNI, NI, NJ) ;
  if(check_16(src_u16, rst_u16, GNI*GNJ) != 0) goto fail ;
  TIME_LOOP_EZ(NITER, (NI*NJ), bhwd2block(blocku, src_u16, GNI, NI, NJ)  ) ;
  fprintf(stderr, "%s", timer_msg) ;
  TIME_LOOP_EZ(NITER, (NI*NJ), block2bhwd(rst_u16, blocku, GNI, NI, NJ) ) ;
  fprintf(stderr, " | %s\n", timer_msg) ;
  tmm = block_zminmax((void *)blocku, NI*NJ) ;
  print_minmax(tmm) ;
  fprintf(stderr, "SUCCESS: u16\n") ;

  msg = "src_i16-1" ; set_16(rst_i16, GNI*GNJ) ; set_32(blocki, NI*NJ) ;
  bhwd2block(blocki, src_i16, GNI, NI, NJ) ;  // signed 16 bit
  block2bhwd(rst_i16, blocki, GNI, NI, NJ) ;
   if(check_16(src_u16, rst_u16, GNI*GNJ) != 0) goto fail ;
  msg = "src_i16-2" ; set_16(rst_i16, GNI*GNJ) ; set_32(blocki, NI*NJ) ;
  bhwd2block(blocki, src_i16+DNIJ, GNI, NI, NJ) ;  // signed 16 bit
  block2bhwd(rst_i16+DNIJ, blocki, GNI, NI, NJ) ;
   if(check_16(src_i16+DNIJ, rst_i16+DNIJ, GNI*GNJ) != 0) goto fail ;
  msg = "src_i16-2" ; set_16(rst_i16, GNI*GNJ) ; set_32(blocki, NI*NJ) ;
  bhwd2block(blocki, src_i16, GNI, NI, NJ) ;  // signed 16 bit
  block2bhwd(rst_i16, blocki, GNI, NI, NJ) ;
   if(check_16(src_i16, rst_i16, GNI*GNJ) != 0) goto fail ;
  TIME_LOOP_EZ(NITER, (NI*NJ), bhwd2block(blocki, src_i16, GNI, NI, NJ)  ) ;
  fprintf(stderr, "%s", timer_msg) ;
  TIME_LOOP_EZ(NITER, (NI*NJ), block2bhwd(rst_i16, blocki, GNI, NI, NJ) ) ;
  fprintf(stderr, " | %s\n", timer_msg) ;
  tmm = block_zminmax((void *)blocki, NI*NJ) ;
  print_minmax(tmm) ;
  fprintf(stderr, "SUCCESS: i16\n") ;

  msg = "src_u32-1" ; set_32(rst_u32, GNI*GNJ) ; set_32(blocku, NI*NJ) ;
  bhwd2block(blocku, src_u32, GNI, NI, NJ) ;  // unsigned 32 bit
  block2bhwd(rst_u32, blocku, GNI, NI, NJ) ;
  if(check_32(src_u32, rst_u32, GNI*GNJ) != 0) goto fail ;
  msg = "src_u32-2" ; set_32(rst_u32, GNI*GNJ) ; set_32(blocku, NI*NJ) ;
  bhwd2block(blocku, src_u32+DNIJ, GNI, NI, NJ) ;  // unsigned 32 bit
  block2bhwd(rst_u32+DNIJ, blocku, GNI, NI, NJ) ;
  if(check_32(src_u32+DNIJ, rst_u32+DNIJ, GNI*GNJ) != 0) goto fail ;
  msg = "src_u32-3" ; set_32(rst_u32, GNI*GNJ) ; set_32(blocku, NI*NJ) ;
  bhwd2block(blocku, src_u32, GNI, NI, NJ) ;  // unsigned 32 bit
  block2bhwd(rst_u32, blocku, GNI, NI, NJ) ;
  if(check_32(src_u32, rst_u32, GNI*GNJ) != 0) goto fail ;
  TIME_LOOP_EZ(NITER, (NI*NJ), bhwd2block(blocku, src_u32, GNI, NI, NJ)  ) ;
  fprintf(stderr, "%s", timer_msg) ;
  TIME_LOOP_EZ(NITER, (NI*NJ), block2bhwd(rst_u32, blocku, GNI, NI, NJ) ) ;
  fprintf(stderr, " | %s\n", timer_msg) ;
  tmm = block_zminmax((void *)blocku, NI*NJ) ;
  print_minmax(tmm) ;
  fprintf(stderr, "SUCCESS: u32\n") ;

  msg = "src_i32-1" ; set_32(rst_i32, GNI*GNJ) ; set_32(blocki, NI*NJ) ;
  bhwd2block(blocki, src_i32, GNI, NI, NJ) ;  // signed 32 bit
  block2bhwd(rst_i32, blocki, GNI, NI, NJ) ;
  if(check_32(src_i32, rst_i32, GNI*GNJ) != 0) goto fail ;
  msg = "src_i32-2" ; set_32(rst_i32, GNI*GNJ) ; set_32(blocki, NI*NJ) ;
  bhwd2block(blocki, src_i32+DNIJ, GNI, NI, NJ) ;  // signed 32 bit
  block2bhwd(rst_i32+DNIJ, blocki, GNI, NI, NJ) ;
  if(check_32(src_i32+DNIJ, rst_i32+DNIJ, GNI*GNJ) != 0) goto fail ;
  msg = "src_i32-3" ; set_32(rst_i32, GNI*GNJ) ; set_32(blocki, NI*NJ) ;
  bhwd2block(blocki, src_i32, GNI, NI, NJ) ;  // signed 32 bit
  block2bhwd(rst_i32, blocki, GNI, NI, NJ) ;
  if(check_32(src_i32, rst_i32, GNI*GNJ) != 0) goto fail ;
  TIME_LOOP_EZ(NITER, (NI*NJ), bhwd2block(blocki, src_i32, GNI, NI, NJ)  ) ;
  fprintf(stderr, "%s", timer_msg) ;
  TIME_LOOP_EZ(NITER, (NI*NJ), block2bhwd(rst_i32, blocki, GNI, NI, NJ) ) ;
  fprintf(stderr, " | %s\n", timer_msg) ;
  tmm = block_zminmax((void *)blocki, NI*NJ) ;
  print_minmax(tmm) ;
  fprintf(stderr, "SUCCESS: i32\n") ;

  msg = "src_f32-1" ; set_32(rst_f32, GNI*GNJ) ; set_32(blockf, NI*NJ) ;
  bhwd2block(blockf, src_f32, GNI, NI, NJ) ;  // float 32 bit
  block2bhwd(rst_f32, blockf, GNI, NI, NJ) ;
  if(check_32(src_f32, rst_f32, GNI*GNJ) != 0) goto fail ;
  msg = "src_f32-2" ; set_32(rst_f32, GNI*GNJ) ; set_32(blockf, NI*NJ) ;
  bhwd2block(blockf, src_f32+DNIJ, GNI, NI, NJ) ;  // float 32 bit
  block2bhwd(rst_f32+DNIJ, blockf, GNI, NI, NJ) ;
  if(check_32(src_f32+DNIJ, rst_f32+DNIJ, GNI*GNJ) != 0) goto fail ;
  msg = "src_f32-3" ; set_32(rst_f32, GNI*GNJ) ; set_32(blockf, NI*NJ) ;
  bhwd2block(blockf, src_f32, GNI, NI, NJ) ;  // float 32 bit
  block2bhwd(rst_f32, blockf, GNI, NI, NJ) ;
  if(check_32(src_f32, rst_f32, GNI*GNJ) != 0) goto fail ;
  TIME_LOOP_EZ(NITER, (NI*NJ), bhwd2block(blockf, src_f32, GNI, NI, NJ)  ) ;
  fprintf(stderr, "%s", timer_msg) ;
  TIME_LOOP_EZ(NITER, (NI*NJ), block2bhwd(rst_f32, blockf, GNI, NI, NJ) ) ;
  fprintf(stderr, " | %s\n", timer_msg) ;
  tmm = block_zminmax((void *)blockf, NI*NJ) ;
  print_minmax(tmm) ;
  fprintf(stderr, "SUCCESS: f32\n") ;

  msg = "src_u64-1" ; set_64(rst_u64, GNI*GNJ) ; set_32(blocku, NI*NJ) ;
  bhwd2block(blocku, src_u64, GNI, NI, NJ) ;  // unsigned 64 bit
  block2bhwd(rst_u64, blocku, GNI, NI, NJ) ;
  if(check_64(src_u64, rst_u64, GNI*GNJ) != 0) goto fail ;
  msg = "src_u64-2" ; set_64(rst_u64, GNI*GNJ) ; set_32(blocku, NI*NJ) ;
  bhwd2block(blocku, src_u64+DNIJ, GNI, NI, NJ) ;  // unsigned 64 bit
  block2bhwd(rst_u64+DNIJ, blocku, GNI, NI, NJ) ;
  if(check_64(src_u64+DNIJ, rst_u64+DNIJ, GNI*GNJ) != 0) goto fail ;
  msg = "src_u64-2" ; set_64(rst_u64, GNI*GNJ) ; set_32(blocku, NI*NJ) ;
  bhwd2block(blocku, src_u64, GNI, NI, NJ) ;  // unsigned 64 bit
  block2bhwd(rst_u64, blocku, GNI, NI, NJ) ;
  if(check_64(src_u64, rst_u64, GNI*GNJ) != 0) goto fail ;
  TIME_LOOP_EZ(NITER, (NI*NJ), bhwd2block(blocku, src_u64, GNI, NI, NJ)  ) ;
  fprintf(stderr, "%s", timer_msg) ;
  TIME_LOOP_EZ(NITER, (NI*NJ), block2bhwd(rst_u64, blocku, GNI, NI, NJ) ) ;
  fprintf(stderr, " | %s\n", timer_msg) ;
  tmm = block_zminmax((void *)blocku, NI*NJ) ;
  print_minmax(tmm) ;
  fprintf(stderr, "SUCCESS: u64\n") ;

  msg = "src_i64-1" ; set_64(rst_i64, GNI*GNJ) ; set_32(blocki, NI*NJ) ;
  bhwd2block(blocki, src_i64, GNI, NI, NJ) ;  // signed 64 bit
  block2bhwd(rst_i64, blocki, GNI, NI, NJ) ;
  if(check_64(src_i64, rst_i64, GNI*GNJ) != 0) goto fail ;
  msg = "src_i64-2" ; set_64(rst_i64, GNI*GNJ) ; set_32(blocki, NI*NJ) ;
  bhwd2block(blocki, src_i64+DNIJ, GNI, NI, NJ) ;  // signed 64 bit
  block2bhwd(rst_i64+DNIJ, blocki, GNI, NI, NJ) ;
  if(check_64(src_i64+DNIJ, rst_i64+DNIJ, GNI*GNJ) != 0) goto fail ;
  msg = "src_i64-3" ; set_64(rst_i64, GNI*GNJ) ; set_32(blocki, NI*NJ) ;
  bhwd2block(blocki, src_i64, GNI, NI, NJ) ;  // signed 64 bit
  block2bhwd(rst_i64, blocki, GNI, NI, NJ) ;
  if(check_64(src_i64, rst_i64, GNI*GNJ) != 0) goto fail ;
  TIME_LOOP_EZ(NITER, (NI*NJ), bhwd2block(blocki, src_i64, GNI, NI, NJ)  ) ;
  fprintf(stderr, "%s", timer_msg) ;
  TIME_LOOP_EZ(NITER, (NI*NJ), block2bhwd(rst_i64, blocki, GNI, NI, NJ) ) ;
  fprintf(stderr, " | %s\n", timer_msg) ;
  tmm = block_zminmax((void *)blocki, NI*NJ) ;
  print_minmax(tmm) ;
  fprintf(stderr, "SUCCESS: i64\n") ;

  msg = "src_d64-1" ; set_64(rst_d64, GNI*GNJ) ; set_32(blockf, NI*NJ) ;
  bhwd2block(blockf, src_d64, GNI, NI, NJ) ;  // double 64 bit
  block2bhwd(rst_d64, blockf, GNI, NI, NJ) ;
  if(check_64(src_d64, rst_d64, GNI*GNJ) != 0) goto fail ;
  msg = "src_d64-2" ; set_64(rst_d64, GNI*GNJ) ; set_32(blockf, NI*NJ) ;
  bhwd2block(blockf, src_d64+DNIJ, GNI, NI, NJ) ;  // double 64 bit
  block2bhwd(rst_d64+DNIJ, blockf, GNI, NI, NJ) ;
  if(check_64(src_d64+DNIJ, rst_d64+DNIJ, GNI*GNJ) != 0) goto fail ;
  msg = "src_d64-3" ; set_64(rst_d64, GNI*GNJ) ; set_32(blockf, NI*NJ) ;
  bhwd2block(blockf, src_d64, GNI, NI, NJ) ;  // double 64 bit
  block2bhwd(rst_d64, blockf, GNI, NI, NJ) ;
  if(check_64(src_d64, rst_d64, GNI*GNJ) != 0) goto fail ;
  TIME_LOOP_EZ(NITER, (NI*NJ), bhwd2block(blockf, src_d64, GNI, NI, NJ)  ) ;
  fprintf(stderr, "%s", timer_msg) ;
  TIME_LOOP_EZ(NITER, (NI*NJ), block2bhwd(rst_d64, blockf, GNI, NI, NJ) ) ;
  fprintf(stderr, " | %s\n", timer_msg) ;
  tmm = block_zminmax((void *)blockf, NI*NJ) ;
  print_minmax(tmm) ;
  fprintf(stderr, "SUCCESS: d64\n") ;
  if(timer_min == timer_max) fprintf(stderr, "this is unlikely to print\n") ; // get rid of warning set but unused

  tmm = block_zminmax((void *)blockf, NI*NJ) ;
  print_minmax(tmm) ;
  TIME_LOOP_EZ(NITER, (NI*NJ), tmm = block_zminmax((void *)blockf, NI*NJ) ) ;
  fprintf(stderr, "block_zminmax : %s\n", timer_msg) ;

  return 0 ;

fail :
  fprintf(stderr, "ERROR : %s\n", msg) ;
  return 1 ;
}
