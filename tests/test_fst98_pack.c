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
#include <unistd.h>
#include <stdlib.h>

#include <rmn/fst98_pack.h>
#include <rmn/misc_helpers.h>

#define GNI 64
#define GNJ 64

void encode_decode_float(int ni, int nj, float f_in[nj][ni], float f_out[nj][ni], int nbits, int datyp){
  int32_t stream[ni*nj+128] ;
  RANGE(int32_t) field_out = (RANGE(int32_t)) {stream, stream+ni*nj+128} ;
  RANGE(int32_t) encoded ;
  int data_kind ;

  RANGE_TYPEDEF(float) ;
  RANGE(float) r_float = RANGE_NULL(float) ;
  RANGE_NULL_CONST(float) ;
  if(RANGE_NULL_VAR(float).bot != RANGE_NULL_VAR(float).top) exit(1) ;
  r_float = RANGE_CAST(field_out, float) ;
  if(RANGE_ELEMENTS(r_float) < ni*nj) exit(1) ;
  fprintf(stderr, "downgrade_32, xdf_double, xdf_short, xdf_byte, xdf_stride = %d %d %d %d %d\n", downgrade_32, xdf_double, xdf_short, xdf_byte, xdf_stride) ;

  fprintf(stderr, "F_in corners\n") ;
  fprintf(stderr, "  %10f %10f\n", f_in[GNJ-1][0], f_in[GNJ-1][GNI-1]) ;
  fprintf(stderr, "  %10f %10f\n", f_in[    0][0], f_in[    0][GNI-1]) ;
  encoded = fst98_encode((void *)f_in, field_out, -nbits, ni, nj, 1, datyp, &data_kind) ;
  fprintf(stderr, "encoded size = %ld elements (%ld bytes)\n", RANGE_ELEMENTS(encoded), RANGE_BYTES(encoded));
  fst98_decode(f_out,  encoded.bot, ni, nj, 1, data_kind) ;
  fprintf(stderr, "F_out corners\n") ;
  fprintf(stderr, "  %10f %10f\n", f_out[GNJ-1][0], f_out[GNJ-1][GNI-1]) ;
  fprintf(stderr, "  %10f %10f\n", f_out[    0][0], f_out[    0][GNI-1]) ;

  float maxabs = 0.0f , maxrel = 0.0f  ;
  for(int j=0 ; j<nj ; j++){
    for(int i=0 ; i<ni ; i++){
      float err = f_out[j][i] - f_in[j][i] ;
      float rel = (f_in[j][i] == 0.0f) ? 0.0f : (err / f_in[j][i]) ;
      err = (err < 0.0f) ? (-err) : err ;
      rel = (rel < 0.0f) ? (-rel) : rel ;
      maxabs = (err > maxabs) ? err : maxabs ;
      maxrel = (rel > maxrel) ? rel : maxrel ;
    }
  }
  maxrel = (maxrel < 1.0E-10) ? (1.0E-10) : maxrel ;
  int relpart = 1.0f/maxrel ;
  int offset = (nbits <= 16) ? 1 : 0 ;   // less than 17 bits, expected relative error is halved
  fprintf(stderr, "maxabs = %10f, maxrel = 1 part in %d (expected 1 in %d)\n", maxabs, relpart, 1<<(nbits+offset)) ;
}

int main(int argc, char **argv){
  (void) (argc) ;
  (void) (argv) ;
  float f_data[GNJ][GNI] ;
  float r_data[GNJ][GNI] ;
  float x[GNI], y[GNJ] ;
  float ci = .5f * (GNI - 1) ;
  float cj = .5f * (GNJ - 1) ;
  float fi = 1.0f / ci ;
  float fj = 1.0f / cj ;
//   int32_t stream[GNI*GNJ+128] ;
//   RANGE(int32_t) field_out = (RANGE(int32_t)) {stream, stream+GNI*GNJ+128} ;
//   RANGE(int32_t) encoded ;
  int ni = GNI, nj = GNJ ;
//   int npak = 0, ni = GNI, nj = GNJ, nk = 1, xdf_double = 0, xdf_short = 0, xdf_byte = 0, xdf_stride = 1, downgrade_32 = 0 ;
//   int in_datyp_ori = -1 ;

  for(int i=0 ; i<GNI ; i++){ x[i] = (i - ci) * fi * .99f ; }
  for(int j=0 ; j<GNI ; j++){ y[j] = (j - cj) * fj * .99f ; }

  for(int j=0 ; j<GNI ; j++){
    for(int i=0 ; i<GNI ; i++){
      f_data[j][i] = ( x[i]*x[i] + y[j]*y[j] ) * .5f + 1.0f ;
      r_data[j][i] = 999.999f ;
    }
  }
  fprintf(stderr, "X %10f -> %10f\n", x[0], x[GNI-1]) ;
  fprintf(stderr, "Y %10f -> %10f\n", y[0], y[GNJ-1]) ;
  fprintf(stderr, "F corners\n") ;
  fprintf(stderr, "  %10f %10f\n", f_data[GNJ-1][0], f_data[GNJ-1][GNI-1]) ;
  fprintf(stderr, "  %10f %10f\n", f_data[    0][0], f_data[    0][GNI-1]) ;

//   in_datyp_ori = FST_TYPE_REAL | FST_TYPE_TURBOPACK ;
//   npak = -16 ;
//   encoded = fst98_encode((void *)f_data, field_out, npak, ni, nj, nk, in_datyp_ori, xdf_double, xdf_short, xdf_byte, xdf_stride) ;
//   fprintf(stderr, "encoded size = %ld elements, %ld bytes\n", RANGE_ELEMENTS(encoded), RANGE_BYTES(encoded));
// 
//   int datyp = in_datyp_ori, nbits_in = -npak ;
//   fst98_decode(r_data,  encoded.bot, ni, nj, nk, datyp, nbits_in, downgrade_32, xdf_double, xdf_short, xdf_byte, xdf_stride) ;
//   fprintf(stderr, "R corners\n") ;
//   fprintf(stderr, "  %10f %10f\n", r_data[GNJ-1][0], r_data[GNJ-1][GNI-1]) ;
//   fprintf(stderr, "  %10f %10f\n", r_data[    0][0], r_data[    0][GNI-1]) ;

//   nbits = 16 ; datyp = FST_TYPE_REAL | FST_TYPE_TURBOPACK ;

  fprintf(stderr, "========== FST_TYPE_REAL ==========\n") ;
  encode_decode_float(ni, nj, f_data, r_data, 15, FST_TYPE_REAL) ;

  fprintf(stderr, "========== FST_TYPE_REAL | FST_TYPE_TURBOPACK ==========\n") ;
  encode_decode_float(ni, nj, f_data, r_data, 15, FST_TYPE_REAL | FST_TYPE_TURBOPACK) ;

  fprintf(stderr, "========== FST_TYPE_REAL_OLD_QUANT ==========\n") ;
  encode_decode_float(ni, nj, f_data, r_data, 15, FST_TYPE_REAL_OLD_QUANT) ;

  fprintf(stderr, "========== FST_TYPE_REAL_OLD_QUANT | FST_TYPE_TURBOPACK ==========\n") ;
  encode_decode_float(ni, nj, f_data, r_data, 15, FST_TYPE_REAL | FST_TYPE_TURBOPACK) ;

  fprintf(stderr, "========== FST_TYPE_REAL (17 bits) ==========\n") ;
  encode_decode_float(ni, nj, f_data, r_data, 17, FST_TYPE_REAL) ;

  fprintf(stderr, "========== FST_TYPE_REAL | FST_TYPE_TURBOPACK (17 bits) ==========\n") ;
  encode_decode_float(ni, nj, f_data, r_data, 17, FST_TYPE_REAL | FST_TYPE_TURBOPACK) ;

  fprintf(stderr, "========== FST_TYPE_REAL_OLD_QUANT (17 bits) ==========\n") ;
  encode_decode_float(ni, nj, f_data, r_data, 17, FST_TYPE_REAL_OLD_QUANT) ;

  fprintf(stderr, "========== FST_TYPE_REAL_OLD_QUANT | FST_TYPE_TURBOPACK (17 bits) ==========\n") ;
  encode_decode_float(ni, nj, f_data, r_data, 17, FST_TYPE_REAL_OLD_QUANT | FST_TYPE_TURBOPACK) ;

}
