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
#include <string.h>

#include <rmn/fst98_pack.h>
#include <rmn/misc_helpers.h>

#define GNI 64
#define GNJ 64

void encode_decode_int(int ni, int nj, int32_t f_in[nj][ni], int32_t f_out[nj][ni], int nbits, int datyp, int nodiag){
  int32_t stream[ni*nj+128] ;
  RANGE(int32_t) field_out = (RANGE(int32_t)) {stream, stream+ni*nj+128} ;
  RANGE(int32_t) encoded ;
  int data_kind ;

  RANGE(int32_t) r_int = RANGE_NULL(int32_t) ;
  r_int = RANGE_CAST(field_out, int32_t) ;
  if(RANGE_ELEMENTS(r_int) < ni*nj) exit(1) ;
  r_int = RANGE_NULL(int32_t) ;
//   fprintf(stderr, "downgrade_32, xdf_double, xdf_short, xdf_byte, xdf_stride = %d %d %d %d %d\n", downgrade_32, xdf_double, xdf_short, xdf_byte, xdf_stride) ;

//   if(! nodiag){
//     fprintf(stderr, "F_in corners, datyp = %d, nbits = %d\n", datyp, nbits) ;
//     fprintf(stderr, "  %10d %10d\n", f_in[GNJ-1][0], f_in[GNJ-1][GNI-1]) ;
//     fprintf(stderr, "  %10d %10d\n", f_in[    0][0], f_in[    0][GNI-1]) ;
//   }

//   encoded = fst98_encode((void *)f_in, field_out, -nbits, ni, nj, 1, datyp, &data_kind) ;
  encoded = fst98_encode((void *)f_in, r_int, -nbits, ni, nj, 1, datyp, &data_kind) ;
  fprintf(stderr, "encoded size = %ld elements (%ld bytes), datyp = %d(%d), nbits = %d\n", RANGE_ELEMENTS(encoded), RANGE_BYTES(encoded), data_kind&0xFFFF, datyp, data_kind>>16);

  memset(f_out, 0, (ni*nj)*sizeof(int32_t)) ;
  fst98_decode(f_out,  encoded.bot, ni, nj, 1, data_kind) ;

  if(nodiag) return ;

//   fprintf(stderr, "F_out corners\n") ;
//   fprintf(stderr, "  %10d %10d\n", f_out[GNJ-1][0], f_out[GNJ-1][GNI-1]) ;
//   fprintf(stderr, "  %10d %10d\n", f_out[    0][0], f_out[    0][GNI-1]) ;

  int errors = 0 ;
  for(int j=0 ; j<nj ; j++){
    for(int i=0 ; i<ni ; i++){
      if(f_in[j][i] != f_out[j][i]) errors++ ;
    }
  }
  fprintf(stderr, "number of errors = %d\n", errors) ;
}

void encode_decode_double(int ni, int nj, double f_in[nj][ni], float f_out[nj][ni], int nbits, int datyp, int nodiag){
  (void) (nodiag) ;
  int32_t stream[ni*nj+32] ;
  RANGE(int32_t) field_out = (RANGE(int32_t)) {stream, stream+ni*nj+128} ;
  RANGE(int32_t) encoded ;
  int data_kind ;
  union{
    uint32_t u ;
    float    f ;
  }iuf ;
  int ieee_data = 0 ;

  ieee_data = ( (datyp & 0x3f) == 5) || ( (datyp & 0x3f) == 8) ;
  RANGE_TYPEDEF(float) ;
  RANGE(float) r_float = RANGE_NULL(float) ;
  RANGE_NULL_CONST(float) ;
  if(RANGE_NULL_NAME(float).bot != RANGE_NULL_NAME(float).top) exit(1) ;
  r_float = RANGE_CAST(field_out, float) ;
  if(RANGE_ELEMENTS(r_float) < ni*nj) exit(1) ;
//   fprintf(stderr, "downgrade_32, xdf_double, xdf_short, xdf_byte, xdf_stride = %d %d %d %d %d\n", downgrade_32, xdf_double, xdf_short, xdf_byte, xdf_stride) ;

  xdf_double = 1 ;
  encoded = fst98_encode((void *)f_in, field_out, -nbits, ni, nj, 1, datyp, &data_kind) ;
//   encoded = fst98_encode((void *)f_in, RANGE_NULL(int32_t), -nbits, ni, nj, 1, datyp, &data_kind) ;
  fprintf(stderr, "encoded size = %ld elements (%ld bytes), datyp = %d(%d), nbits = %d\n", RANGE_ELEMENTS(encoded), RANGE_BYTES(encoded), data_kind&0xFFFF, datyp, data_kind>>16);

  memset(f_out, 0, (ni*nj)*sizeof(float)) ;
  fst98_decode(f_out,  encoded.bot, ni, nj, 1, data_kind) ;

  if(nodiag) return ;

//   fprintf(stderr, "F_out corners\n") ;
//   fprintf(stderr, "  %10f %10f\n", f_out[GNJ-1][0], f_out[GNJ-1][GNI-1]) ;
//   fprintf(stderr, "  %10f %10f\n", f_out[    0][0], f_out[    0][GNI-1]) ;

  float maxabs = 0.0f , maxrel = 0.0f  ;
  float fmax, fmin ;
  fmax = fmin = f_in[0][0] ;
  for(int j=0 ; j<nj ; j++){
    for(int i=0 ; i<ni ; i++){
      float err = f_out[j][i] - f_in[j][i] ;
      float rel = (f_in[j][i] == 0.0f) ? 0.0f : (err / f_in[j][i]) ;
      fmax = (f_in[j][i] > fmax) ? f_in[j][i] : fmax ;
      fmin = (f_in[j][i] < fmin) ? f_in[j][i] : fmin ;
      err = (err < 0.0f) ? (-err) : err ;
      rel = (rel < 0.0f) ? (-rel) : rel ;
      maxabs = (err > maxabs) ? err : maxabs ;
//       maxrel = (rel > maxrel) ? rel : maxrel ;
    }
  }
  iuf.f = (fmax - fmin) ;       // float values range
  iuf.u += 0x3FFFFFFF ;         // bump to power of 2 >= range
  iuf.u &= (~0x3FFFFFFF) ;
  maxrel = maxabs / iuf.f ;     // largest error / bumped range
  maxrel = (maxrel < 1.0E-10) ? (1.0E-10) : maxrel ;
  int relpart = 1.0f/maxrel ;
  if(maxabs == 0.0f || ieee_data){
    fprintf(stderr, "maxabs = %10f, fmin = %f, fmax = %f\n", maxabs, fmin, fmax) ;
  }else{
    fprintf(stderr, "maxabs = %10f, maxrel = 1 part in %d (expected 1 in %d)\n", maxabs, relpart, 1<<(nbits+1)) ;
  }
}

void encode_decode_float(int ni, int nj, float f_in[nj][ni], float f_out[nj][ni], int nbits, int datyp, int nodiag){
  (void) (nodiag) ;
  int32_t stream[ni*nj+32] ;
  RANGE(int32_t) field_out = (RANGE(int32_t)) {stream, stream+ni*nj+128} ;
  RANGE(int32_t) encoded ;
  int data_kind ;
  union{
    uint32_t u ;
    float    f ;
  }iuf ;
  int ieee_data = 0 ;

  ieee_data = ( (datyp & 0x3f) == 5) || ( (datyp & 0x3f) == 8) ;
  RANGE_TYPEDEF(float) ;
  RANGE(float) r_float = RANGE_NULL(float) ;
  RANGE_NULL_CONST(float) ;
  if(RANGE_NULL_NAME(float).bot != RANGE_NULL_NAME(float).top) exit(1) ;
  r_float = RANGE_CAST(field_out, float) ;
  if(RANGE_ELEMENTS(r_float) < ni*nj) exit(1) ;
//   fprintf(stderr, "downgrade_32, xdf_double, xdf_short, xdf_byte, xdf_stride = %d %d %d %d %d\n", downgrade_32, xdf_double, xdf_short, xdf_byte, xdf_stride) ;

  encoded = fst98_encode((void *)f_in, field_out, -nbits, ni, nj, 1, datyp, &data_kind) ;
//   encoded = fst98_encode((void *)f_in, RANGE_NULL(int32_t), -nbits, ni, nj, 1, datyp, &data_kind) ;
  fprintf(stderr, "encoded size = %ld elements (%ld bytes), datyp = %d(%d), nbits = %d\n", RANGE_ELEMENTS(encoded), RANGE_BYTES(encoded), data_kind&0xFFFF, datyp, data_kind>>16);

  memset(f_out, 0, (ni*nj)*sizeof(float)) ;
  fst98_decode(f_out,  encoded.bot, ni, nj, 1, data_kind) ;

  if(nodiag) return ;

//   fprintf(stderr, "F_out corners\n") ;
//   fprintf(stderr, "  %10f %10f\n", f_out[GNJ-1][0], f_out[GNJ-1][GNI-1]) ;
//   fprintf(stderr, "  %10f %10f\n", f_out[    0][0], f_out[    0][GNI-1]) ;

  float maxabs = 0.0f , maxrel = 0.0f  ;
  float fmax, fmin ;
  fmax = fmin = f_in[0][0] ;
  for(int j=0 ; j<nj ; j++){
    for(int i=0 ; i<ni ; i++){
      float err = f_out[j][i] - f_in[j][i] ;
      float rel = (f_in[j][i] == 0.0f) ? 0.0f : (err / f_in[j][i]) ;
      fmax = (f_in[j][i] > fmax) ? f_in[j][i] : fmax ;
      fmin = (f_in[j][i] < fmin) ? f_in[j][i] : fmin ;
      err = (err < 0.0f) ? (-err) : err ;
      rel = (rel < 0.0f) ? (-rel) : rel ;
      maxabs = (err > maxabs) ? err : maxabs ;
//       maxrel = (rel > maxrel) ? rel : maxrel ;
    }
  }
  iuf.f = (fmax - fmin) ;       // float values range
  iuf.u += 0x3FFFFFFF ;         // bump to power of 2 >= range
  iuf.u &= (~0x3FFFFFFF) ;
  maxrel = maxabs / iuf.f ;     // largest error / bumped range
  maxrel = (maxrel < 1.0E-10) ? (1.0E-10) : maxrel ;
  int relpart = 1.0f/maxrel ;
  if(maxabs == 0.0f || ieee_data){
    fprintf(stderr, "maxabs = %10f, fmin = %f, fmax = %f\n", maxabs, fmin, fmax) ;
  }else{
    fprintf(stderr, "maxabs = %10f, maxrel = 1 part in %d (expected 1 in %d)\n", maxabs, relpart, 1<<(nbits+1)) ;
  }
}

int main(int argc, char **argv){
  (void) (argc) ;
  (void) (argv) ;
  int32_t u_data[GNJ][GNI] ;
  int32_t i_data[GNJ][GNI] ;
  double d_data[GNJ][GNI] ;
  float f_data[GNJ][GNI] ;
  float r_data[GNJ][GNI] ;
  char *rs_data = (char *)r_data ;
  float x[GNI], y[GNJ] ;
  float ci = .5f * (GNI - 1) ;
  float cj = .5f * (GNJ - 1) ;
  float fi = 1.0f / ci ;
  float fj = 1.0f / cj ;
  int ni = GNI, nj = GNJ ;

  for(int i=0 ; i<GNI ; i++){ x[i] = (i - ci) * fi * .95f ; }
  for(int j=0 ; j<GNI ; j++){ y[j] = (j - cj) * fj * .95f ; }

  for(int j=0 ; j<GNI ; j++){
    for(int i=0 ; i<GNI ; i++){
      f_data[j][i] = ( x[i]*x[i] + y[j]*y[j] ) * .5f + 1.0f ;
      d_data[j][i] = f_data[j][i] ;
      r_data[j][i] = 999.999f ;
      u_data[j][i] = f_data[j][i] * 128 ;
      i_data[j][i] = ((i+j)&1) ? u_data[j][i] : (-u_data[j][i]) ;
    }
  }
//   fprintf(stderr, "X %10f -> %10f\n", x[0], x[GNI-1]) ;
//   fprintf(stderr, "Y %10f -> %10f\n", y[0], y[GNJ-1]) ;
  fprintf(stderr, "corners\n") ;
  fprintf(stderr, "F %10f %10f |", f_data[GNJ-1][0], f_data[GNJ-1][GNI-1]) ;
  fprintf(stderr, "I %10d %10d |", i_data[GNJ-1][0], i_data[GNJ-1][GNI-1]) ;
  fprintf(stderr, "U %10d %10d\n", u_data[GNJ-1][0], u_data[GNJ-1][GNI-1]) ;
  fprintf(stderr, "F %10f %10f |", f_data[    0][0], f_data[    0][GNI-1]) ;
  fprintf(stderr, "I %10d %10d |", i_data[    0][0], i_data[    0][GNI-1]) ;
  fprintf(stderr, "U %10d %10d\n", u_data[    0][0], u_data[    0][GNI-1]) ;
//
  fprintf(stderr, "========== FST_TYPE_REAL (16 bits) ==========\n") ;
  encode_decode_float(ni, nj, f_data, r_data, 16, FST_TYPE_REAL, 0) ;
//
  fprintf(stderr, "========== FST_TYPE_REAL | FST_TYPE_TURBOPACK (16 bits) ==========\n") ;
  encode_decode_float(ni, nj, f_data, r_data, 16, FST_TYPE_REAL | FST_TYPE_TURBOPACK, 0) ;
//
//   fprintf(stderr, "========== FST_TYPE_REAL_OLD_QUANT (15 bits) ==========\n") ;
//   encode_decode_float(ni, nj, f_data, r_data, 15, FST_TYPE_REAL_OLD_QUANT, 0) ;
//
//   fprintf(stderr, "========== FST_TYPE_REAL_OLD_QUANT | FST_TYPE_TURBOPACK (15 bits) ==========\n") ;
//   encode_decode_float(ni, nj, f_data, r_data, 15, FST_TYPE_REAL_OLD_QUANT | FST_TYPE_TURBOPACK, 0) ;
//
//   fprintf(stderr, "========== FST_TYPE_REAL_IEEE | FST_TYPE_TURBOPACK (24 bits) ==========\n") ;
//   encode_decode_float(ni, nj, f_data, r_data, 24, FST_TYPE_REAL_IEEE | FST_TYPE_TURBOPACK, 0) ;
//
//   fprintf(stderr, "========== FST_TYPE_REAL (20 bits) ==========\n") ;
//   encode_decode_float(ni, nj, f_data, r_data, 20, FST_TYPE_REAL, 0) ;
//
//   fprintf(stderr, "========== FST_TYPE_REAL | FST_TYPE_TURBOPACK (20 bits) ==========\n") ;
//   encode_decode_float(ni, nj, f_data, r_data, 20, FST_TYPE_REAL | FST_TYPE_TURBOPACK, 0) ;
//
//   fprintf(stderr, "========== FST_TYPE_REAL_OLD_QUANT (20 bits) ==========\n") ;
//   encode_decode_float(ni, nj, f_data, r_data, 20, FST_TYPE_REAL_OLD_QUANT, 0) ;
//
//   fprintf(stderr, "========== FST_TYPE_REAL_OLD_QUANT | FST_TYPE_TURBOPACK (20 bits) ==========\n") ;
//   encode_decode_float(ni, nj, f_data, r_data, 20, FST_TYPE_REAL_OLD_QUANT | FST_TYPE_TURBOPACK, 0) ;

  fprintf(stderr, "========== FST_TYPE_UNSIGNED (16 bits) ==========\n") ;
  encode_decode_int(ni, nj, u_data, (void *)r_data, 16, FST_TYPE_UNSIGNED, 0) ;

  fprintf(stderr, "========== FST_TYPE_UNSIGNED | FST_TYPE_TURBOPACK (16 bits) ==========\n") ;
  encode_decode_int(ni, nj, u_data, (void *)r_data, 16, FST_TYPE_UNSIGNED | FST_TYPE_TURBOPACK, 0) ;

//   fprintf(stderr, "========== FST_TYPE_UNSIGNED | FST_TYPE_TURBOPACK (24 bits) ==========\n") ;
//   encode_decode_int(ni, nj, u_data, (void *)r_data, 24, FST_TYPE_UNSIGNED | FST_TYPE_TURBOPACK, 0) ;
// 
//   fprintf(stderr, "========== FST_TYPE_UNSIGNED | FST_TYPE_TURBOPACK (32 bits) ==========\n") ;
//   encode_decode_int(ni, nj, u_data, (void *)r_data, 32, FST_TYPE_UNSIGNED | FST_TYPE_TURBOPACK, 0) ;

  fprintf(stderr, "========== FST_TYPE_SIGNED | FST_TYPE_TURBOPACK (12 bits) ==========\n") ;
  encode_decode_int(ni, nj, i_data, (void *)r_data, 12, FST_TYPE_SIGNED | FST_TYPE_TURBOPACK, 0) ;

  fprintf(stderr, "========== FST_TYPE_STRING | FST_TYPE_TURBOPACK (8 bits) ==========\n") ;
  memset(r_data,0,sizeof(r_data)) ;
  char *c_data = "0123456789ABCDEF0123456789ABCDEF" ;
  encode_decode_int(18, 1, (void *)c_data, (void *)rs_data, 8, FST_TYPE_STRING | FST_TYPE_TURBOPACK, 1) ;
  fprintf(stderr, "src = '%s'\ndst = '%s'\n", c_data, rs_data) ;

  fprintf(stderr, "========== FST_TYPE_CHAR | FST_TYPE_TURBOPACK (8 bits) ==========\n") ;
  memset(r_data,0,sizeof(r_data)) ;
  encode_decode_int(18, 1, (void *)c_data, (void *)rs_data, 8, FST_TYPE_CHAR | FST_TYPE_TURBOPACK, 1) ;
  fprintf(stderr, "src(%ld) = '%s'\ndst(%ld) = '%s'\n", strlen(c_data), c_data, strlen(rs_data), rs_data) ;

  fprintf(stderr, "========== FST_TYPE_BINARY | FST_TYPE_TURBOPACK | FSTD_MISSING_FLAG (8 bits) ==========\n") ;
  memset(r_data,0,sizeof(r_data)) ;
  encode_decode_int(24, 1, (void *)c_data, (void *)r_data, 8, FST_TYPE_BINARY | FST_TYPE_TURBOPACK | FSTD_MISSING_FLAG, 1) ;
  fprintf(stderr, "src(%ld) = '%s'\ndst(%ld) = '%s'\n", strlen(c_data), c_data, strlen((char *)r_data), rs_data) ;
//
  fprintf(stderr, "========== FST_TYPE_REAL_IEEE (24 bits) ==========\n") ;
  encode_decode_float(ni, nj, f_data, r_data, 24, FST_TYPE_REAL_IEEE, 0) ;
//
  fprintf(stderr, "========== FST_TYPE_COMPLEX | FST_TYPE_TURBOPACK | FSTD_MISSING_FLAG (24 bits) ==========\n") ;
  memset(r_data,0,sizeof(r_data)) ;
  encode_decode_float(ni/2, nj, f_data, r_data, 24, FST_TYPE_COMPLEX | FST_TYPE_TURBOPACK | FSTD_MISSING_FLAG, 0) ;
//
  fprintf(stderr, "========== FST_TYPE_REAL(DOUBLE) | FST_TYPE_TURBOPACK (20 bits) ==========\n") ;
  memset(r_data,0,sizeof(r_data)) ;
  encode_decode_double(ni, nj, d_data, r_data, 20, FST_TYPE_REAL | FST_TYPE_TURBOPACK, 0) ;
//
  fprintf(stderr, "========== FST_TYPE_REAL | FST_TYPE_TURBOPACK (20 bits) ==========\n") ;
  memset(r_data,0,sizeof(r_data)) ;
  encode_decode_float(ni, nj, f_data, r_data, 20, FST_TYPE_REAL | FST_TYPE_TURBOPACK, 0) ;
}
