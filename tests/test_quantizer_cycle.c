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

// number of extra quantize/restore cycles to perform for consistency test
#define NCYCLES 10

float fix_zval(float minabs, float zval){
  union{ int32_t i ; float f ; } z, m ;
  m.f = minabs ;
  m.i &= 0x7F800000 ;             // truncate to power of 2 <= |value|
  z.f = zval   ;
  z.i &= 0x7FFFFFFF ;             // |zval|
  z.i = (z.i > m.i) ? m.i : z.i ; // shall not be larger than |minabs|
  return z.f ;
}

int32_t q_span(int32_t *q, int32_t n){
  int32_t t, i, mina, maxa ;
  mina = 0x7FFFFFFF ;
  maxa = 0 ;
  for(i=0 ; i<n ; i++){
    t = (q[i] < 0) ? (-q[i]) : q[i] ;
    mina = (t < mina) ? t : mina ;
    maxa = (t > maxa) ? t : maxa ;
  }
  return (maxa - mina) ;
}

// fix the absolute value of all elements of floating point array f
// if |value| < ref, set it to zval
static int32_t fix_abs(float *f, float ref, float zval, int32_t n){
  int i, count = 0 ;
  ref  = (ref  < 0) ? -ref  :  ref ;     // |ref|
  zval = (zval < 0) ? -zval : zval ;     // |zval|
// fprintf(stderr, "replacing abs values < %f with %f\n", ref, zval) ;
  for(i=0 ;i<n ; i++){
    if(f[i] >= 0){
      if(f[i] < ref){ f[i] = zval ; count++ ; }
    }else{
      if((-f[i]) < ref){ f[i] = -zval ; count++ ; }
    }
  }
  return count ;
}
#if 0
// count number of |values| less than or equal to ref in floating point array f
static int32_t count_le(float *f, float ref, int32_t n){
  int i, count = 0 ;
  float af ;
  for(i=0 ;i<n ; i++){
    af = (f[i] < 0) ? (-f[i]) : f[i] ;
    if(af <= ref) count++ ;
  }
  return count ;
}

// count number of |values| less than or equal to ref in floating point array f
static int32_t count_lt(float *f, float ref, int32_t n){
  int i, count = 0 ;
  float af ;
  for(i=0 ;i<n ; i++){
    af = (f[i] < 0) ? (-f[i]) : f[i] ;
    if(af < ref) count++ ;
  }
  return count ;
}

// count number of |values| equal to ref in floating point array f
static int32_t count_eq(float *f, float ref, int32_t n){
  int i, count = 0 ;
  float af ;
  for(i=0 ;i<n ; i++){
    af = (f[i] < 0) ? (-f[i]) : f[i] ;
    if(af == ref) count++ ;
  }
  return count ;
}
#endif
// count non identical values for corresponding values in floating point arrays f1 and f2
static int32_t count_diff(float *f1, float *f2, int32_t n){
  int i, count = 0 ;
  for(i=0 ;i<n ; i++){
    if(f1[i] != f2[i]) count++ ;
  }
  return count ;
}

// find the maximum absolute difference between corresponding values in
// floating point arrays f1 and f2
static float max_abs_err(float *f1, float *f2, int32_t n){
  int i ;
  float err = 0.0f ;
  for(i=0 ;i<n ; i++){
    float diff = f1[i] - f2[i] ;
    diff = (diff < 0) ? (-diff) : diff ;
    err = (diff > err) ? diff : err ;
  }
  return err ;
}

// find the maximum relative difference between corresponding values in
// floating point arrays f1 (reference) and f2 (restored)
// values of 0 in f1 or f2 are ignored
static int max_rel_err(float *f1, float *f2, int32_t n){
  int i/*, imax = 0*/ ;
  float err = 0.0f/*, v1, v2*/ ;
//   v1 = f1[0], v2 = f2[0] ;
  for(i=0 ;i<n ; i++){
    if(f1[i] != 0.0f && f2[i] != 0.0f) {
      float diff = (f1[i] - f2[i]) / f1[i] ;
      diff = (diff < 0) ? (-diff) : diff ;
      if(diff > .99) { fprintf(stderr, "%f vs %f at i = %d\n", f1[i], f2[i], i) ; return 1 ; }
      if(diff > err){
//         imax = i ;
//         v1 = f1[i] ;
//         v2 = f2[i] ;
      }
      err = (diff > err) ? diff : err ;
    }
  }
  if(err < 1.0/0x7FFFFFFF) return 0x7FFFFFFF ;
// fprintf(stderr, "max rel error at %d, %f vs %f", imax, v1, v2) ;
  return 1.0f/err ;
}

float f_abs(float f){
  return (f < 0) ? (-f) : f ;
}

// find min and max value in array f
static void minmax(float *f, int32_t n, float *fmin, float *fmax){
  float min, max ;
  int i;
  min = max = f[0] ;
  for(i=0 ;i<n ; i++){
    min = (f[i] < min) ? f[i] : min ;
    max = (f[i] > max) ? f[i] : max ;
  }
  *fmin = min ;
  *fmax = max ;
}

// find min and max value in array f
// static void minmax_abs(float *f, int32_t n, float *fmin, float *fmax){
//   float min, max, t ;
//   int i;
//   min = max = f_abs(f[0]) ;
//   for(i=0 ;i<n ; i++){
//     t = f_abs(f[i]) ;
//     min = (t < min) ? t : min ;
//     max = (t > max) ? t : max ;
//   }
//   *fmin = min ;
//   *fmax = max ;
// }

// suppress mantissa of floating point value z
static float f_adjust(float z){
  union{float f ; int32_t i ;} fi;
  fi.f = z ;
  fi.i &= 0xFF800000 ;
  return fi.f ;
}

// check that after the first quantize/restore the restored values remain identical
// and that the maximum absolute difference remains within the expected limits
// set by the quantization value quant
// linear quantizer
static int32_t linear_cycles(int ni, int nj, float f[nj][ni], float quant){
  int32_t count = 0, offset = 0, iter ;
  // allocate q and ref
  float ref[nj][ni], t[nj][ni] ;
  int32_t q[nj][ni] ;
  // quantize f to q, restore q to ref
  memset((void *)q  , 0, sizeof(int32_t)*ni*nj) ;   // set q to 0
  int32_t e_base = fp_to_qlin_n((void *)f, (void *)q, ni*nj, quant, offset) ;
  memset((void *)ref, 0, sizeof(float)*ni*nj) ;     // set ref to 0
  qflin_to_fp((void *)ref, (void *)q, ni*nj, e_base, offset) ;
  // loop for NCYCLES
  for(iter=0 ; iter<NCYCLES ; iter++){
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
  // check that the max absolute error is less than 1/2 of the quantification value
  if(max_abs_err((void *)f, (void *)t, ni*nj) > .500001f * quant) count = ni*nj ;
  return count ;
}

// check that after the first quantize/restore the restored values remain identical
// and that the maximum relative difference remains within the expected limits
// set by nbits
// qlog quantizer
static int32_t qlog_cycles(int ni, int nj, float f[nj][ni], int nbits, float msig){
  int32_t count = 0, iter/*, nfix*/ ;
  // allocate q and ref
  float ref[nj][ni], t[nj][ni], zval ;
  int32_t q[nj][ni] ;

  // fix absolute values <= msig in restored array and in source array
  // set to 0.0 in order for them to be ignored
  zval = msig * 0.001f ;
//   zval = msig * 0.999f ;
//   zval = msig * 4.0f ;
//   zval = msig ;
//   zval = 0.0f ;
  zval = fix_zval(msig, zval) ;
  // quantize f to q, restore q to ref
  memset((void *)q  , 0, sizeof(int32_t)*ni*nj) ;   // preset q to 0
  fp_to_qlog((float *)f  , (int32_t *)q, ni*nj, nbits, msig, zval) ;  // quantize
  memset((void *)ref, 0, sizeof(float)*ni*nj) ;     // preset ref to 0
  qlog_to_fp((float *)ref, (int32_t *)q, ni*nj, nbits, msig, zval) ;  // restore
  // loop for NCYCLES
  for(iter=0 ; iter<NCYCLES ; iter++){
    // quantize ref to q
    memset((void *)q, 0, sizeof(int32_t)*ni*nj) ;     // set q to 0
    fp_to_qlog((float *)ref, (int32_t *)q, ni*nj, nbits, msig, zval) ;
    // restore q to t
    memset((void *)t, 0, sizeof(float)*ni*nj) ;      // set t to 0
    qlog_to_fp((float *)t, (int32_t *)q, ni*nj, nbits, msig, zval) ;
    // compare ref to t, fail if not identical
    count = count_diff((void *)ref, (void *)t, ni*nj) ;
    if(count > 0) fprintf(stderr, "qlog_cycles : failed at iter = %d\n", iter) ;
    if(count > 0) break ;
  }
// fprintf(stderr, "=== %d abs values < %f in f\n", count_lt((float *)f, msig, ni*nj), msig) ;
// fprintf(stderr, "=== %d zeroes in f, %d zeroes in ref ===\n", count_eq((float *)f, 0.0f, ni*nj), count_eq((float *)ref, 0.0f, ni*nj));
  /*nfix =*/ fix_abs((float *)f, msig, zval, ni*nj) ;    // fix original array before comparison
// fprintf(stderr, "=== %d zeroes in f, %d zeroes in ref ===\n", count_eq((float *)f, 0.0f, ni*nj), count_eq((float *)ref, 0.0f, ni*nj));
// float fmin, fmax, tmin, tmax ;
// minmax((float *)f, ni*nj, &fmin, &fmax) ;
// minmax((float *)t, ni*nj, &tmin, &tmax) ;
// fprintf(stderr, "fmin =%f, fmax = %f, tmin = %f, tmax = %f\n", fmin, fmax, tmin, tmax) ;
  fprintf(stderr, "%d qlog cycles (%d bits, z = %f) : %d differences (%d with original)",
                  iter, nbits, msig, count, count_diff((void *)f, (void *)t, ni*nj)) ;
  fprintf(stderr, ", max abs err = %f, max rel err = 1 part in %d, span = %d\n",
                  max_abs_err((void *)f, (void *)t, ni*nj), max_rel_err((void *)f, (void *)t, ni*nj), q_span((void *)q, ni*nj)) ;
  // check that the max relative error is less than 1 part in 2 ** (nbits+1)
  if(max_rel_err((void *)f, (void *)t, ni*nj) < (1 << (nbits+1)) ) count = ni*nj ;

  return count ;
}

#define NPTS 16
static int32_t synthetic_qlog(float quant, int nbits, float msig, float zval){
  int npts = NPTS ;
  float f[npts], r[npts] ;
  int32_t q[npts] ;
  int i ;
  fprintf(stderr, "quant = %f, msig = %f, zval = %f, nbits = %d\n", quant, msig, zval, nbits) ;
  for(i=0 ; i<npts ; i++) f[i] = quant * i ;
  fp_to_qlog((float *)f  , (int32_t *)q, npts, nbits, msig, zval) ;
  qlog_to_fp((float *)r, (int32_t *)q, npts, nbits, msig, zval) ;
  for(i=0 ; i<npts ; i++){ fprintf(stderr, "%9f ", f[i]) ; } ; fprintf(stderr, "\n") ;
  for(i=0 ; i<npts ; i++){ fprintf(stderr, "%9d ", q[i]) ; } ; fprintf(stderr, "\n") ;
  for(i=0 ; i<npts ; i++){ fprintf(stderr, "%9f ", r[i]) ; } ; fprintf(stderr, "\n") ;
  return 0 ;
}

int main(int argc, char **argv){
  char *filename = NULL, *msg = "" ;
  float quant = 0.0, minsig = 0.0, min, max, zval = 0.0 ;
  int nij, dims[10], ndim = 0, fd = 0, ndata, ncases = 0, j, nbits = 16, status ;
  void *buf ;

  start_of_test("test quantizer cycles");

  msg = "invalid arguments" ;

  switch(argc){                 // if argc > 2, use pseudo log quantizer)
    case 6:
      zval = atof(argv[5]) ;  // smallest absolute significant value (pseudo_log quantizer)
    case 5:
      minsig = atof(argv[4]) ;  // smallest absolute significant value (pseudo_log quantizer)
      minsig = f_adjust(minsig) ;
    case 4:
      nbits = atoi(argv[3]) ;   // nbits (linear or pseudo_log quantizer)
    case 3:
      quant = atof(argv[2]) ;   // quantum (linear quantizer), smallest absolute significant value  (pseudo_log quantizer)
    case 2:
      filename = argv[1] ;
      break ;
    default:
      fprintf(stderr, "expected 2,3, 4 or 5 arguments, got %d\n", argc-1) ;
      goto fail ;
  }
  msg = "synthetic_qlog failed" ;
  if(synthetic_qlog(quant, nbits, minsig, zval)) goto fail ;
  quant = f_adjust(quant) ;

  fprintf(stderr,"filename  = '%s', quant = %f, minsig = %f, nbits = %d\n", filename, quant, minsig, nbits) ;
  buf = read_32bit_data_record(filename, &fd, dims, &ndim, &ndata) ;  // get data record
  while(buf != NULL && ncases < 99){
    ncases++ ;
    nij = dims[0]*dims[1] ;
    msg = "dimension mismatch or not a 2D record" ;
    if(nij != ndata || ndim != 2) goto fail ;
    minmax(buf, nij, &min, &max) ;
    fprintf(stderr, "%d dimensions : ", ndim) ;
    for(j=0 ; j<ndim ; j++) { fprintf(stderr, "[%d]", dims[j]) ; } ;
    fprintf(stderr, " (%d), min = %f, max = %f\n", nij, min, max);

    msg = "differences detected after first linear quantize" ;
    status = linear_cycles(dims[0], dims[1], buf, quant) ;
    if(status != 0) goto fail ;

    msg = "differences detected after first qlog quantize" ;
    status = qlog_cycles(dims[0], dims[1], buf, nbits, minsig) ;    // modifies buf (applies minsig)
    if(status != 0) goto fail ;
    fprintf(stderr, "\n");
    free(buf) ;
    buf = NULL ;
    buf = read_32bit_data_record(filename, &fd, dims, &ndim, &ndata) ;   // try to get next data record
  }

  fprintf(stderr, "SUCCESS : ncases = %d, quant = %f, nbits = %d\n", ncases, quant, nbits) ;
  return 0 ;

fail:
  fprintf(stderr, "\nFAIL : %s\n", msg) ;
  return 1 ;
}
