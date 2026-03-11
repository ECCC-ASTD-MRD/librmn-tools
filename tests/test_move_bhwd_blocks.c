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
#include <rmn/split_dimension.h>
#include <rmn/timers.h>

#define GNI 198
#define GNJ 129
// #define GNI 64
// #define GNJ 64
#define NI 64
#define NJ 64
// #define NI 32
// #define NJ 32

#define DNIJ ((GNI-NI)/3 + (GNI*((GNJ-NJ)/5)))

#define OR_X 0x00

#define NITER 5000

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
//       if(diff == 1) fprintf(stderr, "i,j = [%d,%d], expecting %4.4x, got %4.4x, ", i, j, ref[j][i], new[j][i]) ;
      if(diff > 0 && j == 0 ) fprintf(stderr, "i,j = [%d,%d], expecting %4.4x, got %4.4x, ", i, j, ref[j][i], new[j][i]) ;
    }
  }
  if(diff) fprintf(stderr, "%d differences\n", diff) ;
  return diff ;
}

int check_8(void *ref, void *new, int lni, int ni, int nj){
  return check_8_(lni, ni, nj, ref,  new) ;
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
  if(diff) fprintf(stderr, "[%d,%d] %d differences / %d\n", ni, nj, diff, ni*nj) ;
  return diff ;
}

int check_16(void *ref, void *new, int lni, int ni, int nj){
  return check_16_(lni, ni, nj, ref,  new) ;
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

int check_32(void *ref, void *new, int lni, int ni, int nj){
  return check_32_(lni, ni, nj, ref,  new) ;
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
      if(diff >0 && diff < 2) fprintf(stderr, "i,j = [%d,%d], expecting %16.16lx, got %16.16lx, ", i, j, ref[j][i], new[i][i]) ;
    }
  }
  if(diff) fprintf(stderr, "%d differences\n", diff) ;
//   fprintf(stderr, "%d differences, [%d,%d]\n", diff, ni, nj) ;
  return diff ;
}

int check_64(void *ref, void *new, int lni, int ni, int nj){
  return check_64_(lni, ni, nj, ref,  new) ;
}

// src      [IN] : input array[gnj][gni]
// src_type [IN] : input array type code
// rst     [OUT] : output array[gnj][gni]
// use element_bytes(src_type) 
int copy_to_block_and_back(int gni, int gnj, int bni, int bnj, void *src_, int src_type, void *rst_){
  uint32_t blk[bnj*bni*4] ;            // temporary block
  uint32_t *pblk = (uint32_t *)blk ;  // pointer to above
  uint8_t *src = (uint8_t *)src_ ;
  uint8_t *rst = (uint8_t *)rst_ ;
  int i0, i1, j0, j1, index, bytes/*, ndiff*/ ;
  bhwd_fn into, from ;
  block_properties bp ;
  array_axis axi, axj ;

  set_bhwd_debug(1) ;
fprintf(stderr, "gni = %d, gnj = %d, bni = %d, bnj = %d, src_type = %d", gni, gnj, bni, bnj, src_type) ;
  if(src_type > MAX_ARRAY_TYPES || src_type < 0) return -1 ;
  into = into_bhwd[src_type] ;
  from = from_bhwd[src_type] ;
  bytes = element_bytes[src_type] ;
fprintf(stderr, ", bytes = %d", bytes) ;
  axi = split_axis(gni, bni) ;
  axj = split_axis(gnj, bnj) ;
fprintf(stderr, ", axi = %d/%d/%d, axj = %d/%d/%d\n", axi.nbk, axi.ln0, axi.ln1, axj.nbk, axj.ln0, axi.ln1) ;
  for(j0 = 0, j1 = axj.ln0 ; j0+bnj <= gnj ; j0 += j1, j1 = axj.ln1){
    for(i0 = 0, i1 = axi.ln0 ; i0+bni <= gni ; i0 += i1, i1 = axi.ln1){
      index = (j0 * gni) + i0 ;
      index *= bytes ;
// fprintf(stderr, "[%4d:%4d,%4d%4d], index = %6d\n", i0, i0+i1-1, j0, j0+j1-1, index) ;
      copy_into_block(into, (void *)pblk, (void *)(src+index), gni, i1, j1, &bp) ;
      copy_from_block(from, (void *)(rst+index), (void *)pblk, gni, i1, j1) ;
//       ndiff = check_8_(gni, i1, j1, (void *)(src+index),  (void *)(rst+index)) ;
  set_bhwd_debug(0) ;
    }
  }

  return 0 ;
}

#define check_bhwd(ref, new, lni, ni, nj) \
   _Generic((ref), \
    uint8_t  *:  check_8( ref, new, lni, ni, nj) , \
    int8_t   *:  check_8( ref, new, lni, ni, nj) , \
    uint16_t *: check_16( ref, new, lni, ni, nj) , \
    int16_t  *: check_16( ref, new, lni, ni, nj) , \
    int32_t  *: check_32( ref, new, lni, ni, nj) , \
    uint32_t *: check_32( ref, new, lni, ni, nj) , \
    float    *: check_32( ref, new, lni, ni, nj) , \
    uint64_t *: check_64( ref, new, lni, ni, nj) , \
    int64_t  *: check_64( ref, new, lni, ni, nj) , \
    double   *: check_64( ref, new, lni, ni, nj) , \
    _Float16 *: check_16( ref, new, lni, ni, nj)   \
   )

float b16_to_f32(__bf16 bf16);
__bf16 f32_to_b16(float f32);
void move_b16_to_f32(float * restrict f32, __bf16* restrict b16, int lni, int ni, int nj);

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
  _Float16 src_f16[GNI*GNJ], rst_f16[GNI*GNJ] ;
  __bf16   src_b16[GNI*GNJ], rst_b16[GNI*GNJ] ;
  char dummy[2] ;
  bhwd_fn fwd, inv ;
  int32_t i ;
  char *msg ;
  TIME_LOOP_DATA ;
  block_properties tmm ;
  int kind ;

  goto OK ;
fail :
  fprintf(stderr, "ERROR : %s\n", msg) ;
  return 1 ;
OK :

  start_of_test("bdh block move functions") ;
  fprintf(stderr, "global values = %d, block values = %d\n", GNI*GNJ, NI*NJ) ;

  check_bhwd(src_u8,    src_u8,           GNI, NI, NJ) ;
  check_bhwd(&src_u8[0], &src_u8[0],        GNI, NI, NJ) ;
//   check_bhwd(PBHWD(src_u8   ), PBHWD(src_u8   ), GNI, NI, NJ) ;
//   check_bhwd(PBHWD(src_u8[0]), PBHWD(src_u8[0]), GNI, NI, NJ) ;

// fill arrays
  for(i=0 ; i<GNI*GNJ ; i++){
    src_u8[i]  = ( OR_X | (i & 0xFF)) ;
    src_u16[i] = ( OR_X | (i & 0xFFFF)) ;
    src_u32[i] = ( OR_X | (i)) ;
    src_u64[i] = ( OR_X | (src_u32[i])) ;
    src_i8[i]  = ( OR_X | ((int32_t)(i & 0xFF) - 128)) ;
    src_i16[i] = ( OR_X | ((int32_t)i - (GNI*GNJ)/2)) ;
    src_i32[i] = ( OR_X | ((int32_t)i - (GNI*GNJ)/2)) ;
//     src_i32[i] = ( OR_X | (i - (GNI*GNJ)/2)) ;
    src_i32[i] = (i - (GNI*GNJ)/2) ;
    src_i64[i] = ( OR_X | (src_i32[i])) ;
    src_f32[i] = ( OR_X | (i)) ;
    if(i & 1) src_f32[i] = (-src_f32[i]) ;
    src_d64[i] = src_f32[i] ;
    src_f16[i] = src_f32[i] ;
    src_b16[i] = f32_to_b16(src_f32[i]) ;
  }
  int diff = 0 ;
  float maxdiff = 0.0, maxerr = 0.0, maxabs = 0.0f, minabs = 999999999.0f ;

  for(i=0 ; i<GNI*GNJ ; i++){
    if(src_f16[i] != src_f32[i]) diff++ ;
    float diff = src_f16[i] - src_f32[i] ;
    diff = (diff < 0) ? -diff : diff ;
    maxdiff = (diff > maxdiff) ? diff : maxdiff ;
    float err = diff / src_f32[i] ;
    err = (err < 0) ? -err : err ;
    maxerr = (err > maxerr) ? err : maxerr ;
    float abs = (src_f32[i] < 0) ? -src_f32[i] : src_f32[i] ;
    maxabs = (abs > maxabs) ? abs : maxabs ;
    if(abs != 0.0f) minabs = (abs < minabs) ? abs : minabs ;
  }
  fprintf(stderr, "size of src_f16 element = %ld\n", sizeof(src_f16[0])) ;
  fprintf(stderr, "src_f16 vs src_f32 : %d differences, maxdiff = %f, maxerr = 1 part in %f, maxabs = %f, minabs = %f\n", diff, maxdiff, 1.0f / maxerr, maxabs, minabs) ;

  fprintf(stderr, "size of src_b16 element = %ld\n", sizeof(src_b16[0])) ;
  move_b16_to_f32(rst_f32, src_b16, GNI, GNI, GNJ);
  maxdiff = 0.0 ; maxerr = 0.0 ; maxabs = 0.0f ; minabs = 999999999.0f ; diff = 0 ;
  for(i=0 ; i<GNI*GNJ ; i++){
    float t ;
    t = rst_f32[i] ;
//     t = b16_to_f32(src_b16[i]) ;
    if( t != src_f32[i]) diff++ ;
    float diff = t - src_f32[i] ;
    diff = (diff < 0) ? -diff : diff ;
    maxdiff = (diff > maxdiff) ? diff : maxdiff ;
    float err = diff / src_f32[i] ;
    err = (err < 0) ? -err : err ;
    maxerr = (err > maxerr) ? err : maxerr ;
    float abs = (src_f32[i] < 0) ? -src_f32[i] : src_f32[i] ;
    maxabs = (abs > maxabs) ? abs : maxabs ;
    if(abs != 0.0f) minabs = (abs < minabs) ? abs : minabs ;
  }
  fprintf(stderr, "src_b16 vs src_f32 : %d differences, maxdiff = %f, maxerr = 1 part in %f, maxabs = %f, minabs = %f\n", diff, maxdiff, 1.0f / maxerr, maxabs, minabs) ;
return 0 ;
// check that signed 32 -> 64 bits copy is done right
for(i=0 ; i<2 ; i++) fprintf(stderr, " i = %d, src_i64[i] = %ld, src_i32[i] = %d\n", i, src_i64[i], src_i32[i]) ;
// syntax check
  fprintf(stderr, "===================== check codes, kinds, function names =====================\n") ;

  fprintf(stderr, "char code = %2d(%s), fn = %p/%p, '%s/%s'",
          array_type_code(dummy), array_type_name(dummy[0]), to_block_fn(dummy), from_block_fn(dummy), to_block_name(dummy), from_block_name(dummy));
  kind=block_kind(dummy) ;
  fprintf(stderr,",      block kind %2d %10s %10s\n", kind, block_kind_name(dummy), printable_type[kind]);

  fprintf(stderr, "u8  code = %2d(%8s), fn = '%15s / %15s'",
          array_type_code(src_u8), array_type_name(src_u8[0]), to_block_name(src_u8), from_block_name(src_u8)) ;
  kind=block_kind(src_u8) ;
  fprintf(stderr,", block kind %2d %10s %10s\n", kind, block_kind_name(src_u8), printable_type[kind]);

  fprintf(stderr, "i8  code = %2d(%8s), fn = '%15s / %15s'",
          array_type_code(src_i8), array_type_name(src_i8[0]), to_block_name(src_i8), from_block_name(src_i8)) ;
  kind=block_kind(src_i8) ;
  fprintf(stderr,", block kind %2d %10s %10s\n", kind, block_kind_name(src_i8), printable_type[kind]);

  fprintf(stderr, "u16 code = %2d(%8s), fn = '%15s / %15s'",
          array_type_code(src_u16), array_type_name(src_u16[0]), to_block_name(src_u16), from_block_name(src_u16)) ;
  kind=block_kind(src_u16) ;
  fprintf(stderr,", block kind %2d %10s %10s\n", kind, block_kind_name(src_u16), printable_type[kind]);

  fprintf(stderr, "i16 code = %2d(%8s), fn = '%15s / %15s'",
          array_type_code(src_i16), array_type_name(src_i16[0]), to_block_name(src_i16), from_block_name(src_i16)) ;
  kind=block_kind(src_i16) ;
  fprintf(stderr,", block kind %2d %10s %10s\n", kind, block_kind_name(src_i16), printable_type[kind]);

  fprintf(stderr, "u32 code = %2d(%8s), fn = '%15s / %15s'",
          array_type_code(src_u32), array_type_name(src_u32[0]), to_block_name(src_u32), from_block_name(src_u32)) ;
  kind=block_kind(src_u32) ;
  fprintf(stderr,", block kind %2d %10s %10s\n", kind, block_kind_name(src_u32), printable_type[kind]);

  fprintf(stderr, "i32 code = %2d(%8s), fn = '%15s / %15s'",
          array_type_code(src_i32), array_type_name(src_i32[0]), to_block_name(src_i32), from_block_name(src_i32)) ;
  kind=block_kind(src_i32) ;
  fprintf(stderr,", block kind %2d %10s %10s\n", kind, block_kind_name(src_i32), printable_type[kind]);

  fprintf(stderr, "f32 code = %2d(%8s), fn = '%15s / %15s'",
          array_type_code(src_f32), array_type_name(src_f32[0]), to_block_name(src_f32), from_block_name(src_f32)) ;
  kind=block_kind(src_f32) ;
  fprintf(stderr,", block kind %2d %10s %10s\n", kind, block_kind_name(src_f32), printable_type[kind]);

  fprintf(stderr, "u64 code = %2d(%8s), fn = '%15s / %15s'",
          array_type_code(src_u64), array_type_name(src_u64[0]), to_block_name(src_u64), from_block_name(src_u64)) ;
  kind=block_kind(src_u64) ;
  fprintf(stderr,", block kind %2d %10s %10s\n", kind, block_kind_name(src_u64), printable_type[kind]);

  fprintf(stderr, "i64 code = %2d(%8s), fn = '%15s / %15s'",
          array_type_code(src_i64), array_type_name(src_i64[0]), to_block_name(src_i64), from_block_name(src_i64)) ;
  kind=block_kind(src_i64) ;
  fprintf(stderr,", block kind %2d %10s %10s\n", kind, block_kind_name(src_i64), printable_type[kind]);

  fprintf(stderr, "d64 code = %2d(%8s), fn = '%15s / %15s'",
          array_type_code(src_d64), array_type_name(src_d64[0]), to_block_name(src_d64), from_block_name(src_d64)) ;
  kind=block_kind(src_d64) ;
  fprintf(stderr,", block kind %2d %10s %10s\n", kind, block_kind_name(src_d64), printable_type[kind]);

  fprintf(stderr, "\n===================== syntax checks =====================\n\n") ;

  fwd = to_block_fn(src_u8) ;
  copy_block_fn(fwd, blocku, src_u8, GNI, NI, NJ) ;          // copy into block, no properties computed
  copy_into_block(fwd, blocku, src_u8, GNI, NI, NJ, NULL) ;  // copy into block, no properties computed
  copy_into_block(fwd, blocku, src_u8, GNI, NI, NJ, &tmm) ;  // copy into block, properties will be computed
  inv = from_block_fn(src_u8) ;
  copy_from_block(inv, src_u8, blocku, GNI, NI, NJ) ;

  fwd = to_block_fn(src_i8) ;
  inv = from_block_fn(src_i8) ;

  uint32_t *tdum1 = w32_cast(blockf, src_u8)  ;
  bhwd2block(tdum1, src_u8, GNI, NI, NJ, &tmm) ;                     // block should be unsigned 32 bit
  block2bhwd(src_u8, tdum1, GNI, NI, NJ) ;                           // block should be unsigned 32 bit
  bhwd2block(w32_cast(blockf, src_u8), src_u8, GNI, NI, NJ, &tmm) ;  // block should be unsigned 32 bit
  block2bhwd(src_u8, w32_cast(blockf, src_u8), GNI, NI, NJ) ;        // block should be unsigned 32 bit
  bhwd2block((uint32_t *)blockf, src_u8, GNI, NI, NJ, &tmm) ;        // block should be unsigned 32 bit
  block2bhwd(src_u8, (uint32_t *)blockf, GNI, NI, NJ) ;              // block should be unsigned 32 bit

  fprintf(stderr, "\n===================== single block copy, all types =====================\n\n") ;

  msg = "src_u8-1" ; set_8(rst_u8, GNI*GNJ) ; set_32(blocku, NI*NJ) ;
  bhwd2block(blocku, src_u8, GNI, NI, NJ, &tmm) ;  // unsigned 8 bit
  block2bhwd(rst_u8, blocku, GNI, NI, NJ) ;
  if(check_bhwd(src_u8, rst_u8, GNI, NI, NJ) != 0) goto fail ;
  msg = "src_u2" ; set_8(rst_u8, GNI*GNJ) ; set_32(blocku, NI*NJ) ;
  bhwd2block(blocku, &src_u8[DNIJ], GNI, NI, NJ, &tmm) ;  // unsigned 8 bit
  block2bhwd(&rst_u8[DNIJ], blocku, GNI, NI, NJ) ;
  if(check_bhwd(&src_u8[DNIJ], &rst_u8[DNIJ], GNI, NI, NJ) != 0) goto fail ;
  msg = "src_u8-3" ; set_8(rst_u8, GNI*GNJ) ; set_32(blocku, NI*NJ) ;
  bhwd2block(blocku, src_u8, GNI, NI, NJ, &tmm) ;  // unsigned 8 bit
// print_block_properties(tmm) ;
  block2bhwd(rst_u8, blocku, GNI, NI, NJ) ;
  if(check_bhwd(src_u8, rst_u8, GNI, NI, NJ) != 0) goto fail ;
  TIME_LOOP_EZ(NITER, (NI*NJ), bhwd2block_nobp(blocku, src_u8, GNI, NI, NJ) ) ;
  fprintf(stderr, "%s", timer_msg) ;
  TIME_LOOP_EZ(NITER, (NI*NJ), bhwd2block(blocku, src_u8, GNI, NI, NJ, &tmm) ) ;
  fprintf(stderr, " | %s", timer_msg) ;
  TIME_LOOP_EZ(NITER, (NI*NJ), block2bhwd(rst_u8, blocku, GNI, NI, NJ) ) ;
  fprintf(stderr, " | %s\n", timer_msg) ;
//   tmm = block_zminmax((void *)blocku, NI*NJ) ; set_block_properties(&tmm, blocku) ;
  tmm = get_block_properties(blocku, NI*NJ) ;
  print_block_properties(tmm) ;
  fprintf(stderr, "SUCCESS: u8\n") ;

  msg = "src_i8-1" ; set_8(rst_i8, GNI*GNJ) ; set_32(blocki, NI*NJ) ;
  bhwd2block(blocki, src_i8, GNI, NI, NJ, &tmm) ;  // signed 8 bit
  block2bhwd(rst_i8, blocki, GNI, NI, NJ) ;
  if(check_bhwd(src_i8, rst_i8, GNI, NI, NJ) != 0) goto fail ;
  msg = "src_i8-2" ; set_8(rst_i8, GNI*GNJ) ; set_32(blocki, NI*NJ) ;
  bhwd2block(blocki, &src_i8[DNIJ], GNI, NI, NJ, &tmm) ;  // signed 8 bit
  block2bhwd(&rst_i8[DNIJ], blocki, GNI, NI, NJ) ;
  if(check_bhwd(&src_i8[DNIJ], &rst_i8[DNIJ], GNI, NI, NJ) != 0) goto fail ;
  msg = "src_i8-3" ; set_8(rst_i8, GNI*GNJ) ; set_32(blocki, NI*NJ) ;
  bhwd2block(blocki, src_i8, GNI, NI, NJ, &tmm) ;  // signed 8 bit
// print_block_properties(tmm) ;
  block2bhwd(rst_i8, blocki, GNI, NI, NJ) ;
  if(check_bhwd(src_i8, rst_i8, GNI, NI, NJ) != 0) goto fail ;
  TIME_LOOP_EZ(NITER, (NI*NJ), bhwd2block_nobp(blocki, src_i8, GNI, NI, NJ)  ) ;
  fprintf(stderr, "%s", timer_msg) ;
  TIME_LOOP_EZ(NITER, (NI*NJ), bhwd2block(blocki, src_i8, GNI, NI, NJ, &tmm)  ) ;
  fprintf(stderr, " | %s", timer_msg) ;
  TIME_LOOP_EZ(NITER, (NI*NJ), block2bhwd(rst_i8, blocki, GNI, NI, NJ) ) ;
  fprintf(stderr, " | %s\n", timer_msg) ;
//   tmm = block_zminmax((void *)blocki, NI*NJ) ; set_block_properties(&tmm, blocki) ;
  tmm = get_block_properties(blocki, NI*NJ) ;
  print_block_properties(tmm) ;
  fprintf(stderr, "SUCCESS: i8\n") ;

  msg = "src_u16-1" ; set_16(rst_u16, GNI*GNJ) ; set_32(blocku, NI*NJ) ;
  bhwd2block(blocku, src_u16, GNI, NI, NJ, &tmm) ;  // unsigned 16 bit
  block2bhwd(rst_u16, blocku, GNI, NI, NJ) ;
  if(check_bhwd(src_u16, rst_u16, GNI, NI, NJ) != 0) goto fail ;
  msg = "src_u16-2" ; set_16(rst_u16, GNI*GNJ) ; set_32(blocku, NI*NJ) ;
  bhwd2block(blocku, &src_u16[DNIJ], GNI, NI, NJ, &tmm) ;  // unsigned 16 bit
  block2bhwd(&rst_u16[DNIJ], blocku, GNI, NI, NJ) ;
  if(check_bhwd(&src_u16[DNIJ], &rst_u16[DNIJ], GNI, NI, NJ) != 0) goto fail ;
  msg = "src_u16-3" ; set_16(rst_u16, GNI*GNJ) ; set_32(blocku, NI*NJ) ;
  bhwd2block(blocku, src_u16, GNI, NI, NJ, &tmm) ;  // unsigned 16 bit
// print_block_properties(tmm) ;
  block2bhwd(rst_u16, blocku, GNI, NI, NJ) ;
  if(check_bhwd(src_u16, rst_u16, GNI, NI, NJ) != 0) goto fail ;
  TIME_LOOP_EZ(NITER, (NI*NJ), bhwd2block_nobp(blocku, src_u16, GNI, NI, NJ)  ) ;
  fprintf(stderr, "%s", timer_msg) ;
  TIME_LOOP_EZ(NITER, (NI*NJ), bhwd2block(blocku, src_u16, GNI, NI, NJ, &tmm)  ) ;
  fprintf(stderr, " | %s", timer_msg) ;
  TIME_LOOP_EZ(NITER, (NI*NJ), block2bhwd(rst_u16, blocku, GNI, NI, NJ) ) ;
  fprintf(stderr, " | %s\n", timer_msg) ;
//   tmm = block_zminmax((void *)blocku, NI*NJ) ; set_block_properties(&tmm, blocku) ;
  tmm = get_block_properties(blocku, NI*NJ) ;
  print_block_properties(tmm) ;
  fprintf(stderr, "SUCCESS: u16\n") ;

  msg = "src_i16-1" ; set_16(rst_i16, GNI*GNJ) ; set_32(blocki, NI*NJ) ;
  bhwd2block(blocki, src_i16, GNI, NI, NJ, &tmm) ;  // signed 16 bit
  block2bhwd(rst_i16, blocki, GNI, NI, NJ) ;
  if(check_bhwd(src_i16, rst_i16, GNI, NI, NJ) != 0) goto fail ;
  msg = "src_i16-2" ; set_16(rst_i16, GNI*GNJ) ; set_32(blocki, NI*NJ) ;
  bhwd2block(blocki, &src_i16[DNIJ], GNI, NI, NJ, &tmm) ;  // signed 16 bit
  block2bhwd(&rst_i16[DNIJ], blocki, GNI, NI, NJ) ;
  if(check_bhwd(&src_i16[DNIJ], &rst_i16[DNIJ], GNI, NI, NJ) != 0) goto fail ;
  msg = "src_i16-3" ; set_16(rst_i16, GNI*GNJ) ; set_32(blocki, NI*NJ) ;
  bhwd2block(blocki, src_i16, GNI, NI, NJ, &tmm) ;  // signed 16 bit
// print_block_properties(tmm) ;
  block2bhwd(rst_i16, blocki, GNI, NI, NJ) ;
  if(check_bhwd(src_i16, rst_i16, GNI, NI, NJ) != 0) goto fail ;
  TIME_LOOP_EZ(NITER, (NI*NJ), bhwd2block_nobp(blocki, src_i16, GNI, NI, NJ)  ) ;
  fprintf(stderr, "%s", timer_msg) ;
  TIME_LOOP_EZ(NITER, (NI*NJ), bhwd2block(blocki, src_i16, GNI, NI, NJ, &tmm)  ) ;
  fprintf(stderr, " | %s", timer_msg) ;
  TIME_LOOP_EZ(NITER, (NI*NJ), block2bhwd(rst_i16, blocki, GNI, NI, NJ) ) ;
  fprintf(stderr, " | %s\n", timer_msg) ;
//   tmm = block_zminmax((void *)blocki, NI*NJ) ; set_block_properties(&tmm, blocki) ;
  tmm = get_block_properties(blocki, NI*NJ) ;
  print_block_properties(tmm) ;
  fprintf(stderr, "SUCCESS: i16\n") ;

  msg = "src_u32-1" ; set_32(rst_u32, GNI*GNJ) ; set_32(blocku, NI*NJ) ;
  bhwd2block(blocku, src_u32, GNI, NI, NJ, &tmm) ;  // unsigned 32 bit
  block2bhwd(rst_u32, blocku, GNI, NI, NJ) ;
  if(check_bhwd(src_u32, rst_u32, GNI, NI, NJ) != 0) goto fail ;
  msg = "src_u32-2" ; set_32(rst_u32, GNI*GNJ) ; set_32(blocku, NI*NJ) ;
  bhwd2block(blocku, &src_u32[DNIJ], GNI, NI, NJ, &tmm) ;  // unsigned 32 bit
  block2bhwd(&rst_u32[DNIJ], blocku, GNI, NI, NJ) ;
  if(check_bhwd(&src_u32[DNIJ], &rst_u32[DNIJ], GNI, NI, NJ) != 0) goto fail ;
  msg = "src_u32-3" ; set_32(rst_u32, GNI*GNJ) ; set_32(blocku, NI*NJ) ;
  bhwd2block(blocku, src_u32, GNI, NI, NJ, &tmm) ;  // unsigned 32 bit
// print_block_properties(tmm) ;
  block2bhwd(rst_u32, blocku, GNI, NI, NJ) ;
  if(check_bhwd(src_u32, rst_u32, GNI, NI, NJ) != 0) goto fail ;
  TIME_LOOP_EZ(NITER, (NI*NJ), bhwd2block_nobp(blocku, src_u32, GNI, NI, NJ)  ) ;
  fprintf(stderr, "%s", timer_msg) ;
  TIME_LOOP_EZ(NITER, (NI*NJ), bhwd2block(blocku, src_u32, GNI, NI, NJ, &tmm)  ) ;
  fprintf(stderr, " | %s", timer_msg) ;
  TIME_LOOP_EZ(NITER, (NI*NJ), block2bhwd(rst_u32, blocku, GNI, NI, NJ) ) ;
  fprintf(stderr, " | %s\n", timer_msg) ;
//   tmm = block_zminmax((void *)blocku, NI*NJ) ; set_block_properties(&tmm, blocku) ;
  tmm = get_block_properties(blocku, NI*NJ) ;
  print_block_properties(tmm) ;
  fprintf(stderr, "SUCCESS: u32\n") ;

  msg = "src_i32-1" ; set_32(rst_i32, GNI*GNJ) ; set_32(blocki, NI*NJ) ;
  bhwd2block(blocki, src_i32, GNI, NI, NJ, &tmm) ;  // signed 32 bit
  block2bhwd(rst_i32, blocki, GNI, NI, NJ) ;
  if(check_bhwd(src_i32, rst_i32, GNI, NI, NJ) != 0) goto fail ;
  msg = "src_i32-2" ; set_32(rst_i32, GNI*GNJ) ; set_32(blocki, NI*NJ) ;
  bhwd2block(blocki, &src_i32[DNIJ], GNI, NI, NJ, &tmm) ;  // signed 32 bit
  block2bhwd(&rst_i32[DNIJ], blocki, GNI, NI, NJ) ;
  if(check_bhwd(&src_i32[DNIJ], &rst_i32[DNIJ], GNI, NI, NJ) != 0) goto fail ;
  msg = "src_i32-3" ; set_32(rst_i32, GNI*GNJ) ; set_32(blocki, NI*NJ) ;
  bhwd2block(blocki, src_i32, GNI, NI, NJ, &tmm) ;  // signed 32 bit
// print_block_properties(tmm) ;
  block2bhwd(rst_i32, blocki, GNI, NI, NJ) ;
  if(check_bhwd(src_i32, rst_i32, GNI, NI, NJ) != 0) goto fail ;
  TIME_LOOP_EZ(NITER, (NI*NJ), bhwd2block_nobp(blocki, src_i32, GNI, NI, NJ)  ) ;
  fprintf(stderr, "%s", timer_msg) ;
  TIME_LOOP_EZ(NITER, (NI*NJ), bhwd2block(blocki, src_i32, GNI, NI, NJ, &tmm)  ) ;
  fprintf(stderr, " | %s", timer_msg) ;
  TIME_LOOP_EZ(NITER, (NI*NJ), block2bhwd(rst_i32, blocki, GNI, NI, NJ) ) ;
  fprintf(stderr, " | %s\n", timer_msg) ;
//   tmm = block_zminmax((void *)blocki, NI*NJ) ; set_block_properties(&tmm, blocki) ;
  tmm = get_block_properties(blocki, NI*NJ) ;
  print_block_properties(tmm) ;
  fprintf(stderr, "SUCCESS: i32\n") ;

  msg = "src_f32-1" ; set_32(rst_f32, GNI*GNJ) ; set_32(blockf, NI*NJ) ;
  bhwd2block(blockf, src_f32, GNI, NI, NJ, &tmm) ;  // float 32 bit
  block2bhwd(rst_f32, blockf, GNI, NI, NJ) ;
  if(check_bhwd(src_f32, rst_f32, GNI, NI, NJ) != 0) goto fail ;
  msg = "src_f32-2" ; set_32(rst_f32, GNI*GNJ) ; set_32(blockf, NI*NJ) ;
  bhwd2block(blockf, &src_f32[DNIJ], GNI, NI, NJ, &tmm) ;  // float 32 bit
  block2bhwd(&rst_f32[DNIJ], blockf, GNI, NI, NJ) ;
  if(check_bhwd(&src_f32[DNIJ], &rst_f32[DNIJ], GNI, NI, NJ) != 0) goto fail ;
  msg = "src_f32-3" ; set_32(rst_f32, GNI*GNJ) ; set_32(blockf, NI*NJ) ;
  bhwd2block(blockf, src_f32, GNI, NI, NJ, &tmm) ;  // float 32 bit
// print_block_properties(tmm) ;
  block2bhwd(rst_f32, blockf, GNI, NI, NJ) ;
  if(check_bhwd(src_f32, rst_f32, GNI, NI, NJ) != 0) goto fail ;
  TIME_LOOP_EZ(NITER, (NI*NJ), bhwd2block_nobp(blockf, src_f32, GNI, NI, NJ)  ) ;
  fprintf(stderr, "%s", timer_msg) ;
  TIME_LOOP_EZ(NITER, (NI*NJ), bhwd2block(blockf, src_f32, GNI, NI, NJ, &tmm)  ) ;
  fprintf(stderr, " | %s", timer_msg) ;
  TIME_LOOP_EZ(NITER, (NI*NJ), block2bhwd(rst_f32, blockf, GNI, NI, NJ) ) ;
  fprintf(stderr, " | %s\n", timer_msg) ;
//   tmm = block_zminmax((void *)blockf, NI*NJ) ; set_block_properties(&tmm, blockf) ;
  tmm = get_block_properties(blockf, NI*NJ) ;
  print_block_properties(tmm) ;
  fprintf(stderr, "SUCCESS: f32\n") ;

  msg = "src_u64-1" ; set_64(rst_u64, GNI*GNJ) ; set_32(blocku, NI*NJ) ;
  bhwd2block(blocku, src_u64, GNI, NI, NJ, &tmm) ;  // unsigned 64 bit
  block2bhwd(rst_u64, blocku, GNI, NI, NJ) ;
  if(check_bhwd(src_u64, rst_u64, GNI, NI, NJ) != 0) goto fail ;
  msg = "src_u64-2" ; set_64(rst_u64, GNI*GNJ) ; set_32(blocku, NI*NJ) ;
  bhwd2block(blocku, &src_u64[DNIJ], GNI, NI, NJ, &tmm) ;  // unsigned 64 bit
  block2bhwd(&rst_u64[DNIJ], blocku, GNI, NI, NJ) ;
  if(check_bhwd(&src_u64[DNIJ], &rst_u64[DNIJ], GNI, NI, NJ) != 0) goto fail ;
  msg = "src_u64-3" ; set_64(rst_u64, GNI*GNJ) ; set_32(blocku, NI*NJ) ;
  bhwd2block(blocku, src_u64, GNI, NI, NJ, &tmm) ;  // unsigned 64 bit
// print_block_properties(tmm) ;
  block2bhwd(rst_u64, blocku, GNI, NI, NJ) ;
  if(check_bhwd(src_u64, rst_u64, GNI, NI, NJ) != 0) goto fail ;
  TIME_LOOP_EZ(NITER, (NI*NJ), bhwd2block_nobp(blocku, src_u64, GNI, NI, NJ) ) ;
  fprintf(stderr, "%s", timer_msg) ;
  TIME_LOOP_EZ(NITER, (NI*NJ), bhwd2block(blocku, src_u64, GNI, NI, NJ, &tmm) ) ;
  fprintf(stderr, " | %s", timer_msg) ;
  TIME_LOOP_EZ(NITER, (NI*NJ), block2bhwd(rst_u64, blocku, GNI, NI, NJ) ) ;
  fprintf(stderr, " | %s\n", timer_msg) ;
//   tmm = block_zminmax((void *)blocku, NI*NJ) ; set_block_properties(&tmm, blocku) ;
  tmm = get_block_properties(blocku, NI*NJ) ;
  print_block_properties(tmm) ;
  fprintf(stderr, "SUCCESS: u64\n") ;

  msg = "src_i64-1" ; set_64(rst_i64, GNI*GNJ) ; set_32(blocki, NI*NJ) ;
  bhwd2block(blocki, src_i64, GNI, NI, NJ, &tmm) ;  // signed 64 bit
  block2bhwd(rst_i64, blocki, GNI, NI, NJ) ;
  if(check_bhwd(src_i64, rst_i64, GNI, NI, NJ) != 0) goto fail ;
  msg = "src_i64-2" ; set_64(rst_i64, GNI*GNJ) ; set_32(blocki, NI*NJ) ;
  bhwd2block(blocki, &src_i64[DNIJ], GNI, NI, NJ, &tmm) ;  // signed 64 bit
  block2bhwd(&rst_i64[DNIJ], blocki, GNI, NI, NJ) ;
  if(check_bhwd(&src_i64[DNIJ], &rst_i64[DNIJ], GNI, NI, NJ) != 0) goto fail ;
  msg = "src_i64-3" ; set_64(rst_i64, GNI*GNJ) ; set_32(blocki, NI*NJ) ;
  bhwd2block(blocki, src_i64, GNI, NI, NJ, &tmm) ;  // signed 64 bit
// print_block_properties(tmm) ;
  block2bhwd(rst_i64, blocki, GNI, NI, NJ) ;
  if(check_bhwd(src_i64, rst_i64, GNI, NI, NJ) != 0) goto fail ;
  TIME_LOOP_EZ(NITER, (NI*NJ), bhwd2block_nobp(blocki, src_i64, GNI, NI, NJ) ) ;
  fprintf(stderr, "%s", timer_msg) ;
  TIME_LOOP_EZ(NITER, (NI*NJ), bhwd2block(blocki, src_i64, GNI, NI, NJ, &tmm) ) ;
  fprintf(stderr, " | %s", timer_msg) ;
  TIME_LOOP_EZ(NITER, (NI*NJ), block2bhwd(rst_i64, blocki, GNI, NI, NJ) ) ;
  fprintf(stderr, " | %s\n", timer_msg) ;
//   tmm = block_zminmax((void *)blocki, NI*NJ) ; set_block_properties(&tmm, blocki) ;
  tmm = get_block_properties(blocki, NI*NJ) ;
  print_block_properties(tmm) ;
  fprintf(stderr, "SUCCESS: i64\n") ;

  msg = "src_d64-1" ; set_64(rst_d64, GNI*GNJ) ; set_32(blockf, NI*NJ) ;
  bhwd2block(blockf, src_d64, GNI, NI, NJ, &tmm) ;  // double 64 bit
  block2bhwd(rst_d64, blockf, GNI, NI, NJ) ;
  if(check_bhwd(src_d64, rst_d64, GNI, NI, NJ) != 0) goto fail ;
  msg = "src_d64-2" ; set_64(rst_d64, GNI*GNJ) ; set_32(blockf, NI*NJ) ;
  bhwd2block(blockf, &src_d64[DNIJ], GNI, NI, NJ, &tmm) ;  // double 64 bit
  block2bhwd(&rst_d64[DNIJ], blockf, GNI, NI, NJ) ;
  if(check_bhwd(&src_d64[DNIJ], &rst_d64[DNIJ], GNI, NI, NJ) != 0) goto fail ;
  msg = "src_d64-3" ; set_64(rst_d64, GNI*GNJ) ; set_32(blockf, NI*NJ) ;
  bhwd2block(blockf, src_d64, GNI, NI, NJ, &tmm) ;  // double 64 bit
// print_block_properties(tmm) ;
  block2bhwd(rst_d64, blockf, GNI, NI, NJ) ;
  if(check_bhwd(src_d64, rst_d64, GNI, NI, NJ) != 0) goto fail ;
  TIME_LOOP_EZ(NITER, (NI*NJ), bhwd2block_nobp(blockf, src_d64, GNI, NI, NJ)  ) ;
  fprintf(stderr, "%s", timer_msg) ;
  TIME_LOOP_EZ(NITER, (NI*NJ), bhwd2block(blockf, src_d64, GNI, NI, NJ, &tmm)  ) ;
  fprintf(stderr, " | %s", timer_msg) ;
  TIME_LOOP_EZ(NITER, (NI*NJ), block2bhwd(rst_d64, blockf, GNI, NI, NJ) ) ;
  fprintf(stderr, " | %s\n", timer_msg) ;
//   tmm = block_zminmax((void *)blockf, NI*NJ) ; set_block_properties(&tmm, blockf) ;
  tmm = get_block_properties(blockf, NI*NJ) ;
  print_block_properties(tmm) ;
  fprintf(stderr, "SUCCESS: d64\n") ;

  msg = "src_f16-1" ; set_32(blockf, NI*NJ) ; set_16(rst_f16, GNI*GNJ) ; 
  bhwd2block(blockf, src_f16, GNI, NI, NJ, NULL) ;  // float 16 bit
  block2bhwd(rst_f16, blockf, GNI, NI, NJ) ;
  if(check_bhwd(src_f16, rst_f16, GNI, NI, NJ) != 0) goto fail ;
  msg = "src_f16-2" ; set_32(blockf, NI*NJ) ; set_16(rst_f16, GNI*GNJ) ; 
  bhwd2block(blockf, &src_f16[DNIJ], GNI, NI, NJ, &tmm) ;  // double 64 bit
  block2bhwd(&rst_f16[DNIJ], blockf, GNI, NI, NJ) ;
  if(check_bhwd(&src_f16[DNIJ], &rst_f16[DNIJ], GNI, NI, NJ) != 0) goto fail ;
  msg = "src_f16-3" ; set_32(blockf, NI*NJ) ; set_16(rst_f16, GNI*GNJ) ; 
  bhwd2block(blockf, src_f16, GNI, NI, NJ, NULL) ;  // float 16 bit
  block2bhwd(rst_f16, blockf, GNI, NI, NJ) ;
  if(check_bhwd(src_f16, rst_f16, GNI, NI, NJ) != 0) goto fail ;
  TIME_LOOP_EZ(NITER, (NI*NJ), bhwd2block_nobp(blockf, src_f16, GNI, NI, NJ)  ) ;
  fprintf(stderr, "%s", timer_msg) ;
  TIME_LOOP_EZ(NITER, (NI*NJ), bhwd2block(blockf, src_f16, GNI, NI, NJ, &tmm)  ) ;
  fprintf(stderr, " | %s", timer_msg) ;
  TIME_LOOP_EZ(NITER, (NI*NJ), block2bhwd(rst_f16, blockf, GNI, NI, NJ) ) ;
  fprintf(stderr, " | %s\n", timer_msg) ;
  set_32(blockf, NI*NJ) ;
  bhwd2block(blockf, src_f16, GNI, NI, NJ, NULL) ;  // float 16 bit
  tmm = get_block_properties(blockf, NI*NJ) ;
  print_block_properties(tmm) ;
  fprintf(stderr, "SUCCESS: f16\n") ;

  if(timer_min == timer_max) fprintf(stderr, "this is unlikely to print\n") ; // get rid of warning set but unused

  fprintf(stderr, "\n") ;
  tmm = get_block_properties(blockf, NI*NJ) ;
  print_block_properties(tmm) ;
  TIME_LOOP_EZ(NITER, (NI*NJ), tmm = block_zminmax((void *)blockf, NI*NJ) ) ;
  fprintf(stderr, "%s\n", timer_msg) ;
  fprintf(stderr, "SUCCESS: block_zminmax\n") ;

  fprintf(stderr, "\nunsupported block pointer types, expecting kind = INVALID\n") ;
  tmm = get_block_properties((char *)blockf, NI*NJ) ;
  print_block_properties(tmm) ;
  if(tmm.kind != bad_data) goto fail ;
  tmm = get_block_properties((void *)blockf, NI*NJ) ;
  print_block_properties(tmm) ;
  if(tmm.kind != bad_data) goto fail ;
  tmm = get_block_properties((ssize_t *)blockf, NI*NJ) ;
  print_block_properties(tmm) ;
  if(tmm.kind != bad_data) goto fail ;
  fprintf(stderr, "SUCCESS: unsupported types\n") ;

  fprintf(stderr, "\n===================== full multiple block copy, all types =====================\n\n") ;

  // copy_to_block_and_back( GNI, GNJ, bni, bnj, source, restore, fwd_function, inv_function)
  // compare source and restored arrays
//   set_bhwd_debug(1) ;
  msg = "src_u8-02" ; set_8(rst_u8, GNI*GNJ) ;
  copy_to_block_and_back(GNI, GNJ, NI, NJ, src_u8, array_type_code(src_u8), rst_u8) ;
  if(check_8_(GNI, GNI, GNJ, (void *)src_u8,  (void *)rst_u8) != 0) goto fail ;

  msg = "src_i8-02" ; set_8(rst_i8, GNI*GNJ) ;
  copy_to_block_and_back(GNI, GNJ, NI, NJ, src_i8, array_type_code(src_i8), rst_i8) ;
  if(check_8_(GNI, GNI, GNJ, (void *)src_i8,  (void *)rst_i8) != 0) goto fail ;

  msg = "src_u16-02" ; set_16(rst_u16, GNI*GNJ) ;
  copy_to_block_and_back(GNI, GNJ, NI, NJ, src_u16, array_type_code(src_u16), rst_u16) ;
  if(check_16_(GNI, GNI, GNJ, (void *)src_u16,  (void *)rst_u16) != 0) goto fail ;

  msg = "src_i16-02" ; set_16(rst_i16, GNI*GNJ) ;
  copy_to_block_and_back(GNI, GNJ, NI, NJ, src_i16, array_type_code(src_i16), rst_i16) ;
  if(check_16_(GNI, GNI, GNJ, (void *)src_i16,  (void *)rst_i16) != 0) goto fail ;

  msg = "src_u32-02" ; set_16(rst_u32, GNI*GNJ) ;
  copy_to_block_and_back(GNI, GNJ, NI, NJ, src_u32, array_type_code(src_u32), rst_u32) ;
  if(check_32_(GNI, GNI, GNJ, (void *)src_u32,  (void *)rst_u32) != 0) goto fail ;

  msg = "src_i32-02" ; set_32(rst_i32, GNI*GNJ) ;
  copy_to_block_and_back(GNI, GNJ, NI, NJ, src_i32, array_type_code(src_i32), rst_i32) ;
  if(check_32_(GNI, GNI, GNJ, (void *)src_i32,  (void *)rst_i32) != 0) goto fail ;

  msg = "src_f32-02" ; set_32(rst_f32, GNI*GNJ) ;
  copy_to_block_and_back(GNI, GNJ, NI, NJ, src_f32, array_type_code(src_f32), rst_f32) ;
  if(check_32_(GNI, GNI, GNJ, (void *)src_f32,  (void *)rst_f32) != 0) goto fail ;

  msg = "src_u64-02" ; set_64(rst_u64, GNI*GNJ) ;
  copy_to_block_and_back(GNI, GNJ, NI, NJ, src_u64, array_type_code(src_u64), rst_u64) ;
  if(check_64_(GNI, GNI, GNJ, (void *)src_u64,  (void *)rst_u64) != 0) goto fail ;

  msg = "src_i64-02" ; set_64(rst_i64, GNI*GNJ) ;
  copy_to_block_and_back(GNI, GNJ, NI, NJ, src_i64, array_type_code(src_i64), rst_i64) ;
  if(check_64_(GNI, GNI, GNJ, (void *)src_i64,  (void *)rst_i64) != 0) goto fail ;

  msg = "src_d64-02" ; set_64(rst_d64, GNI*GNJ) ;
  copy_to_block_and_back(GNI, GNJ, NI, NJ, src_d64, array_type_code(src_d64), rst_d64) ;
  if(check_64_(GNI, GNI, GNJ, (void *)src_d64,  (void *)rst_d64) != 0) goto fail ;

  fprintf(stderr, "SUCCESS: multiple block copy\n") ;

  fprintf(stderr, "\n===================== test with array_nd, all types =====================\n\n") ;

  array_2d a1 = array_2d_null, *ap1 = &a1 ;  // data arrays
  array_2d a2 = array_2d_null, *ap2 = &a2 ;
  block_2d b1 = block_2d_null, *bp1 = &b1 ;  // block arrays
  block_2d b2 = block_2d_null, *bp2 = &b2 ;

  int ni_nj = NI + NJ + (random() & 7) ;
  local_block_2d(bp3, ni_nj) ;               // local, monolithic
  fprintf(stderr, "sizeof(bp3_alias) = %ld, sizeof(*bp3) = %ld\n", sizeof(bp3_alias), sizeof(*bp3));

  block_2d b4 = block_2d_null, *bp4 = &b4 ;
  msg = "dynamic_block_2d failed for bp4" ;
  if(dynamic_block_2d(bp4, ni_nj*2) == 0) goto fail ;

  print_block_2d(&b1, "&b1") ;
  print_block_2d(&b2, "&b2") ;
  print_block_2d(bp3, "bp3") ;
  print_block_2d(bp4, "bp4") ;
  fprintf(stderr, "\n") ;

  msg = "mem_block_2d failed" ;
  if(sizeof(blocku) != mem_block_2d(bp1, blocku, sizeof(blocku))) goto fail ;
  print_block_2d(bp1, "bp1(I)") ;
  msg = "reshape bp1 failed" ; if(reshape_block_2d(bp1, NI, NJ) == NULL) goto fail ;
  print_block_2d(bp1, "bp1(R)") ;
  fprintf(stderr, "\n") ;

  bp2 = new_block_2d(NULL, NI*NJ*4, 1) ;     // request monolithic allocation for struct/data
  msg = "bp2 = new_block_2d failed" ; if(bp2 == NULL) goto fail ;
  print_block_2d(bp2, "bp2(A)") ;
  msg = "reshape bp2 failed" ; if(reshape_block_2d(bp2, NI*2, NJ*2) != bp2) goto fail ;
  print_block_2d(bp2, "bp2(R1)") ;
  msg = "reshape bp2 should have failed" ; if(reshape_block_2d(bp2, NI*2+1, NJ*2) != NULL) goto fail ;
  print_block_2d(bp2, "bp2(R2)") ;
  fprintf(stderr, "\n") ;

  msg = "dynamic_block_2d reshape failed for bp4" ;
  if(dynamic_block_2d(bp4, ni_nj*4) == 0) goto fail ;
  print_block_2d(bp4, "bp4(R1)") ;
  if(dynamic_block_2d(bp4, ni_nj*3) == 0) goto fail ;
  print_block_2d(bp4, "bp4(R2)") ;
  b4.flags = 0 ;
  if(dynamic_block_2d(bp4, ni_nj*4-1) == 0) goto fail ;
  print_block_2d(bp4, "bp4(R3)") ;
  if(dynamic_block_2d(bp4, ni_nj*5) != 0) goto fail ;
  print_block_2d(bp4, "bp4(R4)") ;
  fprintf(stderr, "\n") ;

  msg = "free bp1" ; if(free_block_2d(bp1) != bp1)  goto fail ;    // b1 made from external memory, cannot be freed
  msg = "free bp2" ; if(free_block_2d(bp2) != NULL) goto fail ;    // b2 is monolithic, must be freed
  msg = "free bp3" ; if(free_block_2d(bp3) != bp3)  goto fail ;    // b3 is monolithic, and local, cannot be freed

// array_nd *new_array_nd(array_nd *a, void *mem, int32_t esize, int8_t type, int32_t ndims, int32_t nlb5, __i32__5__ lb5);
// new_array(ARRAY_PTR, MEM, ESIZE, TYP, ...)
  ap1 = (array_2d *)new_array( ap1, NULL,    4, int_data, NI, (NJ*4) ) ;    // worst size block
  msg = "error creating ap1" ; if(ap1 == NULL) goto fail ;
  ap2 = (array_2d *)new_array( ap2, src_i32, 4, int_data, GNI, GNJ ) ;      // data array[GNJ][GNI]
  msg = "error creating ap2" ; if(ap2 == NULL) goto fail ;

  fprintf(stderr, "global array bounds[%d:%d,%d:%d]\n", 0, GNI-1, 0, GNJ-1) ;
  fprintf(stderr, "subarray bounds[%d:%d,%d:%d]\n", 0, NI-1, 0, NJ-1) ;
  set_array_lbounds(ap2, 0, NI-1, 0, NJ-1) ;
  set_bhwd_debug(1) ;

  local_block_2d(bp0, (NI*NJ*3)) ;               // local, monolithic
  msg = "error in array_to_block" ;
  a2.type = bf16_data   ; if(array_to_block(ap2, bp0, NULL) != NULL) goto fail ;
  a2.type = fp16_data   ; if(array_to_block(ap2, bp0, NULL) != NULL) goto fail ;
  a2.type = any_data    ; if(array_to_block(ap2, bp0, NULL) != NULL) goto fail ;
  a2.type = large_data  ; if(array_to_block(ap2, bp0, NULL) != NULL) goto fail ;
  a2.type = bad_data    ; if(array_to_block(ap2, bp0, NULL) != NULL) goto fail ;
  a2.type = raw_data    ; if(array_to_block(ap2, bp0, NULL) == NULL) goto fail ;
  a2.type = ubyte_data  ; if(array_to_block(ap2, bp0, NULL) == NULL) goto fail ;
  a2.type = byte_data   ; if(array_to_block(ap2, bp0, NULL) == NULL) goto fail ;
  a2.type = ushort_data ; if(array_to_block(ap2, bp0, NULL) == NULL) goto fail ;
  a2.type = short_data  ; if(array_to_block(ap2, bp0, NULL) == NULL) goto fail ;
  a2.type = uint_data   ; if(array_to_block(ap2, bp0, NULL) == NULL) goto fail ;
  a2.type = int_data    ; if(array_to_block(ap2, bp0, NULL) == NULL) goto fail ;
  a2.type = ulong_data  ; if(array_to_block(ap2, bp0, NULL) == NULL) goto fail ;
  a2.type = long_data   ; if(array_to_block(ap2, bp0, NULL) == NULL) goto fail ;
  a2.type = float_data  ; if(array_to_block(ap2, bp0, NULL) == NULL) goto fail ;
  a2.type = double_data ; if(array_to_block(ap2, bp0, NULL) == NULL) goto fail ;

  set_bhwd_debug(1) ;
  fprintf(stderr, "subarray bounds[%d:%d,%d:%d]\n", GNI-NI, GNI-1, GNJ-NJ, GNJ-1) ;
  set_array_lbounds(ap2, GNI-NI, GNI-1, GNJ-NJ, GNJ-1) ;
  msg = "error in block_to_array" ;
  a2.type = bf16_data   ; bp0->type = float_data ; if(block_to_array(ap2, bp0) != NULL) goto fail ;
  a2.type = fp16_data   ; bp0->type = float_data ; if(block_to_array(ap2, bp0) != NULL) goto fail ;
  a2.type = any_data    ; bp0->type = any_data   ; if(block_to_array(ap2, bp0) != NULL) goto fail ;
  a2.type = large_data  ; bp0->type = large_data ; if(block_to_array(ap2, bp0) != NULL) goto fail ;
  a2.type = bad_data    ; bp0->type = bad_data   ; if(block_to_array(ap2, bp0) != NULL) goto fail ;
  a2.type = raw_data    ; bp0->type = uint_data  ; if(block_to_array(ap2, bp0) == NULL) goto fail ;
  a2.type = ubyte_data  ; bp0->type = uint_data  ; if(block_to_array(ap2, bp0) == NULL) goto fail ;
  a2.type = byte_data   ; bp0->type = int_data   ; if(block_to_array(ap2, bp0) == NULL) goto fail ;
  a2.type = ushort_data ; bp0->type = uint_data  ; if(block_to_array(ap2, bp0) == NULL) goto fail ;
  a2.type = short_data  ; bp0->type = int_data   ; if(block_to_array(ap2, bp0) == NULL) goto fail ;
  a2.type = uint_data   ; bp0->type = uint_data  ; if(block_to_array(ap2, bp0) == NULL) goto fail ;
  a2.type = int_data    ; bp0->type = int_data   ; if(block_to_array(ap2, bp0) == NULL) goto fail ;
  a2.type = ulong_data  ; bp0->type = uint_data  ; if(block_to_array(ap2, bp0) == NULL) goto fail ;
  a2.type = long_data   ; bp0->type = int_data   ; if(block_to_array(ap2, bp0) == NULL) goto fail ;
  a2.type = float_data  ; bp0->type = float_data ; if(block_to_array(ap2, bp0) == NULL) goto fail ;
  a2.type = double_data ; bp0->type = float_data ; if(block_to_array(ap2, bp0) == NULL) goto fail ;

  set_bhwd_debug(0) ;


//   typedef struct{
//     uint16_t h ;
//   }_BFloat16 ;
//   _Float16 src_f16[1000] ;
  fprintf(stderr, "type of src_f16 is %s\n", block_kind_name(src_f16[0])) ;
  fprintf(stderr, "type of src_b16 is %s\n", block_kind_name(src_b16[0])) ;

  fprintf(stderr, "SUCCESS\n") ;
  return 0 ;

// fail :
//   fprintf(stderr, "ERROR : %s\n", msg) ;
//   return 1 ;
}
