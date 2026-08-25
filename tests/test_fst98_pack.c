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

static double get_double(int ni, int nj, double d[nj][ni], int i, int j){
  return d[j][i];
}

static float get_float(int ni, int nj, float d[nj][ni], int i, int j){
  return d[j][i];
}

// void encode_decode_int(int ni, int nj, int32_t f_in[nj][ni], int32_t f_out[nj][ni], int nbits, int datyp, int nodiag, int data_control){
void encode_decode_int(int ni, int nj, void *f_in, void *f_out, int nbits, int datyp, int nodiag, int data_control){
  int32_t stream[ni*nj+128] ;
  RANGE(int32_t) field_out = (RANGE(int32_t)) {stream, stream+ni*nj+128} ;
  RANGE(int32_t) encoded ;
  int data_kind ;

  RANGE(int32_t) r_int = RANGE_NULL(int32_t) ;
  r_int = RANGE_CAST(field_out, int32_t) ;
  if(RANGE_ITEMS(r_int) < ni*nj) exit(1) ;
  r_int = RANGE_NULL(int32_t) ;
  int src_byte  = (data_control & SRC_BYTE) ;
  int src_short = (data_control & SRC_SHORT) ;
  int dst_byte  = (data_control & DST_BYTE) ;
  int dst_short = (data_control & DST_SHORT) ;

  // encode f_in
  encoded = fst98_encode((void *)f_in, r_int, -nbits, ni, nj, 1, datyp | data_control, &data_kind) ;
  fprintf(stderr, "encoded size = %ld items (%ld bytes), datyp = %d(%d), nbits = %d\n", RANGE_ITEMS(encoded), RANGE_BYTES(encoded), data_kind&0xFFFF, datyp, data_kind>>16);
  // decode into f_out
  size_t sizeout = sizeof(int32_t) ;
  if (data_control & SRC_SHORT) sizeout = sizeof(int16_t) ;
  if (data_control & SRC_BYTE ) sizeout = sizeof(int8_t) ;
  memset(f_out, 0, (ni*nj)*sizeout) ;
  fst98_decode(f_out,  encoded.bot, ni, nj, 1, data_kind | data_control) ;

  if(nodiag) return ;
  // check output of decoder
  int errors = 0 ;
  int32_t *o32 = (int32_t *)f_out ;
  int16_t *o16 = (int16_t *)f_out ;
  int8_t *o8  = (int8_t *)f_out ;
  if(src_byte){                              // bytes
    int8_t *t = (int8_t *)f_in ;
    for(int i=0 ; i<ni*nj ; i++){
      int32_t       out = o32[i] ;
      if(dst_byte ) out = o8[i] ;
      if(dst_short) out = o16[i] ;
      if(t[i] != out) errors++ ;
//       if(t[i] != o32[i]) errors++ ;
    }
  }
  if(src_short){                             // short integers
    int16_t *t = (int16_t *)f_in ;
    for(int i=0 ; i<ni*nj ; i++){
      int32_t       out = o32[i] ;
      if(dst_byte ) out = o8[i] ;
      if(dst_short) out = o16[i] ;
      if(t[i] != out) errors++ ;
//       if(t[i] != o32[i]) errors++ ;
    }
  }
  if((src_short == 0) && (src_byte == 0) ){  // normal integers
    int32_t *t = (int32_t *)f_in ;
    for(int i=0 ; i<ni*nj ; i++){
      int32_t       out = o32[i] ;
      if(dst_byte ) out = o8[i] ;
      if(dst_short) out = o16[i] ;
      if(t[i] != out) errors++ ;
//       if(t[i] != o32[i]) errors++ ;
    }
  }
  fprintf(stderr, "number of errors = %d src = [%s%s%s] dst = [%s%s%s]\n",
          errors, src_byte ? "byte" : "", src_short ? "short" : "" , (src_byte || src_short) ? "" : "int",
          dst_byte ? "byte" : "", dst_short ? "short" : "" , (dst_byte || dst_short) ? "" : "int" ) ;
  if(errors > 0) exit(1) ;
}

// void encode_decode_float(int ni, int nj, float f_in[nj][ni], float f_out[nj][ni], int nbits, int datyp, int nodiag, int data_control){
void encode_decode_float(int ni, int nj, void *f_in, void *f_out, int nbits, int datyp, int nodiag, int data_control){
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
  size_t sizeout = (data_control & DST_DOUBLE) ? sizeof(double) : sizeof(float) ;
  size_t sizein  = (data_control & SRC_DOUBLE) ? sizeof(double) : sizeof(float) ;

  ieee_data = ( (datyp & 0x3f) == 5) || ( (datyp & 0x3f) == 8) ;
  RANGE_TYPEDEF(float) ;
  RANGE(float) r_float = RANGE_NULL(float) ;
  RANGE_NULL_CONST(float) ;
  if(RANGE_NULL_NAME(float).bot != RANGE_NULL_NAME(float).top) exit(1) ;
  r_float = RANGE_CAST(field_out, float) ;
  if(RANGE_ITEMS(r_float) < ni*nj) exit(1) ;

  encoded = fst98_encode((void *)f_in, field_out, -nbits, ni, nj, 1, datyp | data_control, &data_kind) ;
  fprintf(stderr, "encoded size = %ld items (%ld bytes), datyp = %d(%d), nbits = %d, sizein = %ld, sizeout = %ld\n",
          RANGE_ITEMS(encoded), RANGE_BYTES(encoded), data_kind&0xFFFF, datyp, data_kind>>16, sizein, sizeout);

  memset(f_out, 0, (ni*nj)*sizeout) ;
  fst98_decode((void *)f_out,  encoded.bot, ni, nj, 1, data_kind | data_control) ;
  datyp = data_kind&0xFFFF ;
  ieee_data = ( (datyp & 0x3f) == 5) || ( (datyp & 0x3f) == 8) ;

  if(nodiag) return ;

//   fprintf(stderr, "F_out corners\n") ;
//   fprintf(stderr, "  %10f %10f\n", f_out[GNJ-1][0], f_out[GNJ-1][GNI-1]) ;
//   fprintf(stderr, "  %10f %10f\n", f_out[    0][0], f_out[    0][GNI-1]) ;

  float maxabs = 0.0f , maxrel = 0.0f  ;
  float fmax = -1.0E-35f, fmin = 1.0E+35f ;

  for(int j=0 ; j<nj ; j++){
    for(int i=0 ; i<ni ; i++){
      float fout = (sizeout == 4) ? get_float(ni, nj, (void *)f_out, i, j) : get_double(ni, nj, (void *)f_out, i, j) ;
      float fin  = (sizein  == 4) ? get_float(ni, nj, (void *)f_in , i, j) : get_double(ni, nj, (void *)f_in , i, j) ;
      float err = fout - fin ; ;
      float rel = (fin == 0.0f) ? 0.0f : (err / fin) ;
      fmax = (fin > fmax) ? fin : fmax ;
      fmin = (fin < fmin) ? fin : fmin ;
      err = (err < 0.0f) ? (-err) : err ;
      rel = (rel < 0.0f) ? (-rel) : rel ;
      maxabs = (err > maxabs) ? err : maxabs ;
    }
  }
  iuf.f = (fmax - fmin) ;       // float values range
  iuf.u += 0x3FFFFFFF ;         // bump to power of 2 >= range
  iuf.u &= (~0x3FFFFFFF) ;
  maxrel = maxabs / iuf.f ;     // largest error / bumped range
  maxrel = (maxrel < 1.0E-10) ? (1.0E-10) : maxrel ;
  int relpart = 1.0f/maxrel ;
  fprintf(stderr, "fmin = %f, fmax = %f, ", fmin, fmax) ;
  if(maxabs == 0.0f || ieee_data){
    fprintf(stderr, "maxabs = %10f, fmin = %f, fmax = %f\n", maxabs, fmin, fmax) ;
  }else{
    fprintf(stderr, "maxabs = %10f, maxrel = 1 part in %d (expected 1 in %d)\n", maxabs, relpart, 1<<(nbits+1)) ;
    if( relpart < (1<<(nbits+1)) ) exit(1) ;
  }
}

int main(int argc, char **argv){
  (void) (argc) ;
  (void) (argv) ;
  int32_t u_data[GNJ][GNI] ;
  int32_t i_data[GNJ][GNI] ;
  int16_t h_data[GNJ][GNI] ;
  uint16_t hu_data[GNJ][GNI] ;
  int8_t  b_data[GNJ][GNI] ;
  uint8_t  bu_data[GNJ][GNI] ;
  double d_data[GNJ][GNI] ;
  double rd_data[GNJ][GNI] ;
  float f_data[GNJ][GNI] ;
  float rf_data[GNJ][GNI] ;
  char *rs_data = (char *)rf_data ;
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
      rf_data[j][i] = 999.999f ;
      rd_data[j][i] = 999.999f ;
      u_data[j][i] = f_data[j][i] * 128 ;
      hu_data[j][i] = u_data[j][i]/2 ;
      bu_data[j][i] = u_data[j][i]/2 ;
      i_data[j][i] = ((i+j)&1) ? u_data[j][i] : (-u_data[j][i]) ;
      h_data[j][i] = i_data[j][i]/2 ;
      b_data[j][i] = i_data[j][i]/2 ;
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
if(argc > 100)
goto strings;
if(argc > 100)
goto uint;
  fprintf(stderr, "========== FST_TYPE_REAL (16 bits) ==========\n") ;
  encode_decode_float(ni, nj, (void *)f_data, (void *)rf_data, 16, FST_TYPE_REAL, 0, 0) ;
//
  fprintf(stderr, "========== FST_TYPE_REAL(SRC_DOUBLE) (16 bits) ==========\n") ;
  encode_decode_float(ni, nj, (void *)d_data, (void *)rf_data, 16, FST_TYPE_REAL, 0, SRC_DOUBLE) ;
//
  fprintf(stderr, "========== FST_TYPE_REAL(DST_DOUBLE) (16 bits) ==========\n") ;
  encode_decode_float(ni, nj, (void *)f_data, (void *)rd_data, 16, FST_TYPE_REAL, 0, DST_DOUBLE) ;
//
  fprintf(stderr, "========== FST_TYPE_REAL(SRC_DOUBLE + DST_DOUBLE) (16 bits) ==========\n") ;
  encode_decode_float(ni, nj, (void *)d_data, (void *)rd_data, 16, FST_TYPE_REAL, 0, SRC_DOUBLE + DST_DOUBLE) ;
//
  fprintf(stderr, "\n");
//
  fprintf(stderr, "========== FST_TYPE_REAL | FST_TYPE_TURBOPACK (16 bits) ==========\n") ;
  encode_decode_float(ni, nj, f_data, rf_data, 16, FST_TYPE_REAL | FST_TYPE_TURBOPACK, 0, 0) ;
//
  fprintf(stderr, "========== FST_TYPE_REAL(SRC_DOUBLE) | FST_TYPE_TURBOPACK (16 bits) ==========\n") ;
  encode_decode_float(ni, nj, d_data, rf_data, 16, FST_TYPE_REAL | FST_TYPE_TURBOPACK, 0, SRC_DOUBLE) ;
//
  fprintf(stderr, "========== FST_TYPE_REAL(DST_DOUBLE) | FST_TYPE_TURBOPACK (16 bits) ==========\n") ;
  encode_decode_float(ni, nj, f_data, rd_data, 16, FST_TYPE_REAL | FST_TYPE_TURBOPACK, 0, DST_DOUBLE) ;
//
  fprintf(stderr, "========== FST_TYPE_REAL(SRC_DOUBLE + DST_DOUBLE) | FST_TYPE_TURBOPACK (16 bits) ==========\n") ;
  encode_decode_float(ni, nj, d_data, rd_data, 16, FST_TYPE_REAL | FST_TYPE_TURBOPACK, 0, SRC_DOUBLE + DST_DOUBLE) ;
if(argc > 100)
goto end;
  fprintf(stderr, "\n");
//
  fprintf(stderr, "========== FST_TYPE_REAL_OLD_QUANT (15 bits) ==========\n") ;
  encode_decode_float(ni, nj, f_data, rf_data, 15, FST_TYPE_REAL_OLD_QUANT, 0, 0) ;
//
  fprintf(stderr, "========== FST_TYPE_REAL_OLD_QUANT | FST_TYPE_TURBOPACK (15 bits) ==========\n") ;
  encode_decode_float(ni, nj, f_data, rf_data, 15, FST_TYPE_REAL_OLD_QUANT | FST_TYPE_TURBOPACK, 0, 0) ;
//
  fprintf(stderr, "========== FST_TYPE_REAL_IEEE | FST_TYPE_TURBOPACK (24 bits) ==========\n") ;
  encode_decode_float(ni, nj, f_data, rf_data, 24, FST_TYPE_REAL_IEEE | FST_TYPE_TURBOPACK, 0, 0) ;
//
  fprintf(stderr, "========== FST_TYPE_REAL (20 bits) ==========\n") ;
  encode_decode_float(ni, nj, f_data, rf_data, 20, FST_TYPE_REAL, 0, 0) ;
//
  fprintf(stderr, "========== FST_TYPE_REAL | FST_TYPE_TURBOPACK (20 bits) ==========\n") ;
  encode_decode_float(ni, nj, f_data, rf_data, 20, FST_TYPE_REAL | FST_TYPE_TURBOPACK, 0, 0) ;
//
  fprintf(stderr, "========== FST_TYPE_REAL_OLD_QUANT (20 bits) ==========\n") ;
  encode_decode_float(ni, nj, f_data, rf_data, 20, FST_TYPE_REAL_OLD_QUANT, 0, 0) ;
//
  fprintf(stderr, "========== FST_TYPE_REAL_OLD_QUANT | FST_TYPE_TURBOPACK (20 bits) ==========\n") ;
  encode_decode_float(ni, nj, f_data, rf_data, 20, FST_TYPE_REAL_OLD_QUANT | FST_TYPE_TURBOPACK, 0, 0) ;
if(argc > 100)
goto end;
uint :
if(argc > 100)
goto sint ;
  fprintf(stderr, "\n");

  fprintf(stderr, "========== FST_TYPE_UNSIGNED (16 bits) ==========\n") ;
  encode_decode_int(ni, nj, u_data, (void *)rf_data, 16, FST_TYPE_UNSIGNED, 0, 0) ;

  fprintf(stderr, "========== FST_TYPE_UNSIGNED(SRC_SHORT) (16 bits) ==========\n") ;
  encode_decode_int(ni, nj, hu_data, (void *)rf_data, 16, FST_TYPE_UNSIGNED, 0, SRC_SHORT) ;

  fprintf(stderr, "========== FST_TYPE_UNSIGNED(SRC_BYTE) (16 bits) ==========\n") ;
  encode_decode_int(ni, nj, bu_data, (void *)rf_data, 16, FST_TYPE_UNSIGNED, 0, SRC_BYTE) ;

  fprintf(stderr, "========== FST_TYPE_UNSIGNED | FST_TYPE_TURBOPACK (16 bits) ==========\n") ;
  encode_decode_int(ni, nj, u_data, (void *)rf_data, 16, FST_TYPE_UNSIGNED | FST_TYPE_TURBOPACK, 0, 0) ;

  fprintf(stderr, "========== FST_TYPE_UNSIGNED(SRC_SHORT) | FST_TYPE_TURBOPACK (16 bits) ==========\n") ;
  encode_decode_int(ni, nj, hu_data, (void *)rf_data, 16, FST_TYPE_UNSIGNED | FST_TYPE_TURBOPACK, 0, SRC_SHORT) ;

  fprintf(stderr, "========== FST_TYPE_UNSIGNED(SRC_SHORT + DST_SHORT) | FST_TYPE_TURBOPACK (16 bits) ==========\n") ;
  encode_decode_int(ni, nj, hu_data, (void *)rf_data, 16, FST_TYPE_UNSIGNED | FST_TYPE_TURBOPACK, 0, SRC_SHORT + DST_SHORT) ;

  fprintf(stderr, "========== FST_TYPE_UNSIGNED(SRC_SHORT + DST_BYTE) | FST_TYPE_TURBOPACK (16 bits) ==========\n") ;
  encode_decode_int(ni, nj, hu_data, (void *)rf_data, 16, FST_TYPE_UNSIGNED | FST_TYPE_TURBOPACK, 0, SRC_SHORT + DST_BYTE) ;

  fprintf(stderr, "========== FST_TYPE_UNSIGNED(SRC_BYTE) | FST_TYPE_TURBOPACK (16 bits) ==========\n") ;
  encode_decode_int(ni, nj, bu_data, (void *)rf_data, 16, FST_TYPE_UNSIGNED | FST_TYPE_TURBOPACK, 0, SRC_BYTE) ;

  fprintf(stderr, "========== FST_TYPE_UNSIGNED(SRC_BYTE + DST_BYTE) | FST_TYPE_TURBOPACK (16 bits) ==========\n") ;
  encode_decode_int(ni, nj, bu_data, (void *)rf_data, 16, FST_TYPE_UNSIGNED | FST_TYPE_TURBOPACK, 0, SRC_BYTE + DST_BYTE) ;

  fprintf(stderr, "========== FST_TYPE_UNSIGNED(SRC_BYTE + DST_SHORT) | FST_TYPE_TURBOPACK (16 bits) ==========\n") ;
  encode_decode_int(ni, nj, bu_data, (void *)rf_data, 16, FST_TYPE_UNSIGNED | FST_TYPE_TURBOPACK, 0, SRC_BYTE + DST_SHORT) ;
if(argc > 100)
goto end;
sint :
  fprintf(stderr, "\n");

  fprintf(stderr, "========== FST_TYPE_SIGNED | FST_TYPE_TURBOPACK (12 bits) ==========\n") ;
  encode_decode_int(ni, nj, i_data, (void *)rf_data, 12, FST_TYPE_SIGNED | FST_TYPE_TURBOPACK, 0, 0) ;

  fprintf(stderr, "========== FST_TYPE_SIGNED(SRC_SHORT) (12 bits) ==========\n") ;
  encode_decode_int(ni, nj, h_data, (void *)rf_data, 12, FST_TYPE_SIGNED, 0, SRC_SHORT) ;

  fprintf(stderr, "========== FST_TYPE_SIGNED(SRC_SHORT + DST_SHORT) (12 bits) ==========\n") ;
  encode_decode_int(ni, nj, h_data, (void *)rf_data, 12, FST_TYPE_SIGNED, 0, SRC_SHORT + DST_SHORT) ;

  fprintf(stderr, "========== FST_TYPE_SIGNED(SRC_SHORT + DST_BYTE) (12 bits) ==========\n") ;
  encode_decode_int(ni, nj, h_data, (void *)rf_data, 12, FST_TYPE_SIGNED, 0, SRC_SHORT + DST_BYTE) ;

  fprintf(stderr, "========== FST_TYPE_SIGNED(SRC_BYTE) (12 bits) ==========\n") ;
  encode_decode_int(ni, nj, b_data, (void *)rf_data, 12, FST_TYPE_SIGNED, 0, SRC_BYTE) ;

  fprintf(stderr, "========== FST_TYPE_SIGNED(SRC_BYTE + DST_BYTE) (12 bits) ==========\n") ;
  encode_decode_int(ni, nj, b_data, (void *)rf_data, 12, FST_TYPE_SIGNED, 0, SRC_BYTE + DST_BYTE) ;

  fprintf(stderr, "========== FST_TYPE_SIGNED(SRC_BYTE + DST_SHORT) (12 bits) ==========\n") ;
  encode_decode_int(ni, nj, b_data, (void *)rf_data, 12, FST_TYPE_SIGNED, 0, SRC_BYTE + DST_SHORT) ;
if(argc > 100)
goto end;
strings:
  fprintf(stderr, "\n");

  fprintf(stderr, "========== FST_TYPE_STRING | FST_TYPE_TURBOPACK (8 bits) ==========\n") ;
  memset(rs_data,0,sizeof(rf_data)) ;
  char *c_data = "0123456789ABCDEF0123456789ABCDEF" ;
  encode_decode_int(18, 1, (void *)c_data, (void *)rs_data, 8, FST_TYPE_STRING | FST_TYPE_TURBOPACK, 1, 0) ;
  fprintf(stderr, "src = '%s' [%ld]\ndst = '%s' [%ld]\n", c_data, strnlen(c_data, 1024), rs_data, strnlen(rs_data, 1024)) ;
  if(strnlen(rs_data, 1024) != 18) exit(1) ;

  fprintf(stderr, "========== FST_TYPE_CHAR | FST_TYPE_TURBOPACK (8 bits) ==========\n") ;
  memset(rs_data,0,sizeof(rf_data)) ;
  encode_decode_int(18, 1, (void *)c_data, (void *)rs_data, 8, FST_TYPE_CHAR | FST_TYPE_TURBOPACK, 1, 0) ;
  fprintf(stderr, "src(%ld) = '%s'\ndst(%ld) = '%s'\n", strlen(c_data), c_data, strlen(rs_data), rs_data) ;
  if(strnlen(rs_data, 1024) != 20) exit(1) ;

  fprintf(stderr, "========== FST_TYPE_BINARY | FST_TYPE_TURBOPACK | FSTD_MISSING_FLAG (8 bits) ==========\n") ;
  memset(rf_data,0,sizeof(rf_data)) ;
  encode_decode_int(24, 1, (void *)c_data, (void *)rf_data, 8, FST_TYPE_BINARY | FST_TYPE_TURBOPACK | FSTD_MISSING_FLAG, 1, 0) ;
  fprintf(stderr, "src(%ld) = '%s'\ndst(%ld) = '%s'\n", strlen(c_data), c_data, strlen((char *)rf_data), (char *)rf_data) ;
  if(strnlen((void *)rf_data, 1024) != 24) exit(1) ;
if(argc > 100)
goto end;
  fprintf(stderr, "\n");
//
  fprintf(stderr, "========== FST_TYPE_REAL_IEEE (24 bits) ==========\n") ;
  encode_decode_float(ni, nj, f_data, rf_data, 24, FST_TYPE_REAL_IEEE, 0, 0) ;
//
  fprintf(stderr, "========== FST_TYPE_REAL_IEEE | FST_TYPE_TURBOPACK (24 bits) ==========\n") ;
  encode_decode_float(ni, nj, f_data, rf_data, 24, FST_TYPE_REAL_IEEE | FST_TYPE_TURBOPACK, 0, 0) ;
//
  fprintf(stderr, "========== FST_TYPE_REAL (17 bits) ==========\n") ;
  encode_decode_float(ni, nj, f_data, rf_data, 17, FST_TYPE_REAL, 0, 0) ;
//
  fprintf(stderr, "========== FST_TYPE_REAL | FST_TYPE_TURBOPACK (17 bits) ==========\n") ;
  encode_decode_float(ni, nj, f_data, rf_data, 17, FST_TYPE_REAL | FST_TYPE_TURBOPACK, 0, 0) ;
//
  fprintf(stderr, "========== FST_TYPE_REAL_OLD_QUANT (17 bits) ==========\n") ;
  encode_decode_float(ni, nj, f_data, rf_data, 17, FST_TYPE_REAL_OLD_QUANT, 0, 0) ;
//
  fprintf(stderr, "========== FST_TYPE_REAL_OLD_QUANT | FST_TYPE_TURBOPACK (17 bits) ==========\n") ;
  encode_decode_float(ni, nj, f_data, rf_data, 17, FST_TYPE_REAL_OLD_QUANT | FST_TYPE_TURBOPACK, 0, 0) ;
//
// if(argc > 100)
goto end;
  fprintf(stderr, "\n");
//
  fprintf(stderr, "========== FST_TYPE_COMPLEX | FST_TYPE_TURBOPACK | FSTD_MISSING_FLAG (24 bits) ==========\n") ;
  memset(rf_data,0,sizeof(rf_data)) ;
  encode_decode_float(ni/2, nj, f_data, rf_data, 24, FST_TYPE_COMPLEX | FST_TYPE_TURBOPACK | FSTD_MISSING_FLAG, 0, 0) ;

end:
  fprintf(stderr, "\nSUCCESS\n");
  return 0 ;
}
