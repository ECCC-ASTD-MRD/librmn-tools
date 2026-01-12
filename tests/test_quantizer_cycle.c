// Hopefully useful code for C (memory block movers)
// Copyright (C) 2025  Recherche en Prevision Numerique
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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <rmn/test_helpers.h>
#include <rmn/c_record_io.h>
#include <rmn/fp_qlin.h>
#include <rmn/fp_qflog.h>

#define NCYCLES 10

int32_t fix_abs(void *a, float ref, int32_t n){
  int i, count = 0 ;
  float *f = (float *)a, af ;
  for(i=0 ;i<n ; i++){
    if(f[i] >= 0){
      if(f[i] < ref) f[i] = ref ;
    }else{
      if((-f[i]) < ref) f[i] = -ref ;
    }
    af = (f[i] < 0) ? (-f[i]) : f[i] ;
    if(af <= ref) count++ ;
  }
  return count ;
}

int32_t count_le(void *a, float ref, int32_t n){
  int i, count = 0 ;
  float *f = (float *)a, af ;
  for(i=0 ;i<n ; i++){
    af = (f[i] < 0) ? (-f[i]) : f[i] ;
    if(af <= ref) count++ ;
  }
  return count ;
}

int32_t count_lt(void *a, float ref, int32_t n){
  int i, count = 0 ;
  float *f = (float *)a, af ;
  for(i=0 ;i<n ; i++){
    af = (f[i] < 0) ? (-f[i]) : f[i] ;
    if(af <= ref) count++ ;
  }
  return count ;
}

int32_t count_diff(void *a, void *b, int32_t n){
  int i, count = 0 ;
  float *f1 = (float *)a, *f2 = (float *)b ;
  for(i=0 ;i<n ; i++){
    if(f1[i] != f2[i]) count++ ;
  }
  return count ;
}

float max_abs_err(void *a, void *b, int32_t n){
  int i ;
  float *f1 = (float *)a, *f2 = (float *)b, err = 0.0f ;
  for(i=0 ;i<n ; i++){
    float diff = f1[i] - f2[i] ;
    diff = (diff < 0) ? (-diff) : diff ;
    err = (diff > err) ? diff : err ;
  }
  return err ;
}

int max_rel_err(void *a, void *b, int32_t n){
  int i ;
  float *f1 = (float *)a, *f2 = (float *)b, err = 0.0f ;
  for(i=0 ;i<n ; i++){
    if(f1[i] != 0.0f && f2[i] != 0.0f) {
      float diff = (f1[i] - f2[i]) / f1[i] ;
      diff = (diff < 0) ? (-diff) : diff ;
      err = (diff > err) ? diff : err ;
    }
  }
  if(err < 1.0/0x7FFFFFFF) return 0x7FFFFFFF ;
  return 1.0f/err ;
}

void minmax(void *a, int32_t n, float *fmin, float *fmax){
  float *f = (float *)a, min, max ;
  int i;
  min = max = f[0] ;
  for(i=0 ;i<n ; i++){
    min = (f[i] < min) ? f[i] : min ;
    max = (f[i] > max) ? f[i] : max ;
  }
  *fmin = min ;
  *fmax = max ;
}

float f_adjust(float z){
  union{float f ; int32_t i ;} fi;
  fi.f = z ;
  fi.i &= 0xFF800000 ;
  return fi.f ;
}

int32_t linear_cycles(int ni, int nj, float f[nj][ni], float quant){
  int32_t count = 0, offset = 0, iter ;
  // allocate q and ref
  float ref[nj][ni], t[nj][ni] ;
  int32_t q[nj][ni] ;
  // quantize f to q, restore q to ref
  memset((void *)q, 0, sizeof(int32_t)*ni*nj) ;     // set q to 0
  memset((void *)ref, 0, sizeof(float)*ni*nj) ;     // set ref to 0
  int32_t e_base = fp_to_qlin_n((void *)f, (void *)q, ni*nj, quant, offset) ;
  qflin_to_fp((void *)ref, (void *)q, ni*nj, e_base, offset) ;
  // loop for NCYCLES
  for(iter=0 ; iter <NCYCLES ; iter++){
    // quantize ref to q
    memset((void *)q, 0, sizeof(int32_t)*ni*nj) ;     // set q to 0
    e_base = fp_to_qlin_n((void *)ref, (void *)q, ni*nj, quant, offset) ;
    // restore q to t
    memset((void *)t, 0, sizeof(float)*ni*nj) ;      // set t to 0
    qflin_to_fp((void *)t, (void *)q, ni*nj, e_base, offset) ;
    // compare ref to t, fail if not identical
    count = count_diff((void *)ref, (void *)t, ni*nj) ;
    if(count > 0) break ;
  }
  fprintf(stderr, "%d linear cycles (quant = %f) : %d differences (%d with original)", iter, quant, count, count_diff((void *)f, (void *)t, ni*nj)) ;
  fprintf(stderr, ", max abs err = %f\n", max_abs_err((void *)f, (void *)t, ni*nj)) ;
  if(max_abs_err((void *)f, (void *)t, ni*nj) > .500001f * quant) count = ni*nj ;
  return count ;
}

int32_t qlog_cycles(int ni, int nj, float f[nj][ni], int nbits, float zval){
  int32_t count = 0, iter ;
  // allocate q and ref
  float ref[nj][ni], t[nj][ni] ;
  int32_t q[nj][ni] ;

  // quantize f to q, restore q to ref
  memset((void *)q, 0, sizeof(int32_t)*ni*nj) ;     // set q to 0
  memset((void *)ref, 0, sizeof(float)*ni*nj) ;     // set ref to 0
  fp_to_qlog((float *)f, (int32_t *)q, ni*nj, nbits, zval, zval) ;
  qlog_to_fp((float *)ref, (int32_t *)q, ni*nj, nbits, zval) ;
  // loop for NCYCLES
  for(iter=0 ; iter <NCYCLES ; iter++){
    // quantize ref to q
    memset((void *)q, 0, sizeof(int32_t)*ni*nj) ;     // set q to 0
    fp_to_qlog((float *)ref, (int32_t *)q, ni*nj, nbits, zval, zval) ;
    // restore q to t
    memset((void *)t, 0, sizeof(float)*ni*nj) ;      // set t to 0
    qlog_to_fp((float *)t, (int32_t *)q, ni*nj, nbits, zval) ;
    // compare ref to t, fail if not identical
    count = count_diff((void *)ref, (void *)t, ni*nj) ;
    if(count > 0) break ;
  }
  fprintf(stderr, "%d qlog cycles (%d bits, z = %f) : %d differences (%d with original)",
                  iter, nbits, zval, count, count_diff((void *)f, (void *)t, ni*nj)) ;
  fprintf(stderr, ", max abs err = %f, max rel err = 1 part in %d\n", max_abs_err((void *)f, (void *)t, ni*nj), max_rel_err((void *)f, (void *)t, ni*nj)) ;
  if(max_rel_err((void *)f, (void *)t, ni*nj) < (1 << (nbits+1)) ) count = ni*nj ;
//   fprintf(stderr, "values <= %f : original = %d, ref = %d, t = %d\n", zval,
//                   count_le((void *)f, zval, ni*nj), count_le((void *)ref, zval, ni*nj), count_le((void *)t, zval, ni*nj)) ;
  return count ;
}

int main(int argc, char **argv){
  char *filename = NULL, *msg = "" ;
  float quant = 0.0, minsig = 0.0, min, max ;
  int nij, dims[10], ndim = 0, fd = 0, ndata, ncases = 0, j, nbits = 16, status ;
  void *buf ;

  start_of_test("test quantizer cycles");

  msg = "invalid arguments" ;
  switch(argc){
    case 5:
      minsig = atof(argv[4]) ;  // smallest absolute significant value (pseudo_log quantizer)
      minsig = f_adjust(minsig) ;
    case 4:
      nbits = atoi(argv[3]) ;   // nbits (linear or pseudo_log quantizer)
    case 3:
      quant = atof(argv[2]) ;   // quantum (linear quantizer), smallest absolute significant value  (pseudo_log quantizer)
      quant = f_adjust(quant) ;
    case 2:
      filename = argv[1] ;
      break ;
    default:
      fprintf(stderr, "expected 2,3 or 4 arguments, got %d\n", argc-1) ;
      goto fail ;
  }
  // if argc > 2, use pseudo log quantizer)

  fprintf(stderr,"filename  = '%s', quant = %f, minsig = %f, nbits = %d\n", filename, quant, minsig, nbits) ;
  buf = read_32bit_data_record(filename, &fd, dims, &ndim, &ndata) ;  // get data record
  while(buf != NULL){
    nij = dims[0]*dims[1] ;
    msg = "dimension mismatch or not a 2D record" ;
    if(nij != ndata || ndim != 2) goto fail ;
    minmax(buf, nij, &min, &max) ;
    fprintf(stderr, "%d dimensions : ", ndim) ;
    for(j=0 ; j<ndim ; j++) { fprintf(stderr, "[%d]", dims[j]) ; } ;
    fprintf(stderr, " (%d), min = %f, max = %f\n", nij, min, max);

    msg = "differences detected after first quantize" ;
    status = linear_cycles(dims[0], dims[1], buf, quant) ;
    if(status != 0) goto fail ;

    fix_abs(buf, minsig, nij) ;
    status = qlog_cycles(dims[0], dims[1], buf, nbits, minsig) ;
    if(status != 0) goto fail ;

    ncases++ ;
    free(buf) ;
    buf = NULL ;
    buf = read_32bit_data_record(filename, &fd, dims, &ndim, &ndata) ;   // try to get next data record
  }

  fprintf(stderr, "SUCCESS : ncases = %d, quant = %f, nbits = %d\n", ncases, quant, nbits) ;
  return 0 ;

fail:
  fprintf(stderr, "FAIL : %s\n", msg) ;
  return 1 ;
}
