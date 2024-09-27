//
// Copyright (C) 2024  Environnement Canada
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
//     M. Valin,   Recherche en Prevision Numerique, 2024
//
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <rmn/test_helpers.h>

#define WITH_TIMING

#if defined(WITH_TIMING)
#include <rmn/timers.h>
#else
#define TIME_LOOP_DATA ;
#define TIME_LOOP_EZ(niter, npts, code) ;
  char *timer_msg = "" ;
  float NaNoSeC = 0.0f ;
#endif

int32_t float_encode_4x4(float *f, int lni, int nbits, uint32_t *stream);
void float_decode_4x4(float *f, int nbits, uint32_t *stream);

#define NTIMES 10000

static float array[32], decoded[32], ratio[32], error[32] ;

#define LNI 1027
#define LNJ 1025
static float f[LNJ][LNI], r[LNJ][LNI] ;
static uint32_t virt[LNJ*LNI] ;

int32_t array_decode_by_4x4(int lni, int ni, int nj, float r[nj][lni], uint32_t *stream, int nbits){
  int i, j, lng ;
  int32_t header ;
  float localf[4][4] ;
  lng = 0 ;
  for(j=0 ; j<nj-3 ; j+=4){
    for(i=0 ; i<ni-3 ; i+=4){
      float_decode_4x4(&(localf[0]), nbits, stream) ;
      int ii, jj ;
      for(jj=0 ; jj<4 ; jj++){
        for(ii=0 ; ii<4 ; ii++){
          r[j+jj][i+ii] = localf[jj][ii] ;
        }
      }
      stream += (nbits+1) / 2 ; lng += (nbits+1) / 2 ;
    }
  }
  return lng ;
}

int32_t array_encode_by_4x4(int lni, int ni, int nj, float f[nj][lni], uint32_t *stream, int nbits){
  int i, j, lng ;
  int32_t header ;
  lng = 0 ;
  for(j=0 ; j<nj-3 ; j+=4){
    for(i=0 ; i<ni-3 ; i+=4){
      header = float_encode_4x4(&(f[j][i]), lni, nbits, stream) ;
      stream += (nbits+1) / 2 ; lng += (nbits+1) / 2 ;
    }
  }
  return lng ;
}

int main(int argc, char **argv){
  uint32_t *iarray = (uint32_t *) &(array[0]) ;
  uint32_t stream[64] ;
  int32_t header = 0 ;
  float epsilon = 0.012345678f, scale = 2.99f, xrand, xmax, xmin, bias = 0.0f ;
  int i, j ;
  int nbits = 15 ;
  uint64_t t0, t1, u0, u1 ;

  srandom( (uint64_t)(&stream) & 0xFFFFFFFFu ) ;
  srand( (uint64_t)(&stream) & 0xFFFFFFFFu ) ;
  if(argc > 1) srand(atoi(argv[1])) ;

  xmax = -1.0E+35 ;
  xmin = 1.0E+35 ;
  for(j=0 ; j<LNJ ; j++){
    for(i=0 ; i<LNI ; i++){
      xrand = (random() & 0xFFF) / 4096.0f ;     // 0.0 -> 1.0
      f[j][i] = 1.0001f + xrand * scale ;           // 1.0 -> 1.0 + scale
      xmax = (f[j][i] > xmax) ? f[j][i] : xmax ;
      xmin = (f[j][i] < xmin) ? f[j][i] : xmin ;
      r[j][i] = 0.0f ;
    }
  }
  fprintf(stderr, "reference array : min = %10.5f, max = %10.5f\n\n", xmin, xmax) ;

  for(i=0 ; i<16 ; i++) array[i] = 1.01f + epsilon + i / 15.97f ;
  array[15] *= 8.0f ;
  for(i=0 ; i<16 ; i++) array[i] = -array[i] ;        // all negative numbers
//   for(i=0 ; i<16 ; i+=2) array[i] = -array[i] ;       // introduce some negative numbers

  header = float_encode_4x4(array, 4, nbits, stream) ;

  fprintf(stderr, "emin = %d, ", header >> 8) ;
  int ebits = header & 7 ;
  fprintf(stderr, "ebits = %d, ", ebits) ;
  int sbits = (header >> 3) & 1 ;
  fprintf(stderr, "sbits = %d, ", sbits) ;
  if(sbits == 0) fprintf(stderr, "sign = %d, ", (header >> 4) & 1) ;
  fprintf(stderr, "mbits = %d, ", nbits - ebits - sbits) ;
  fprintf(stderr, "nbits = %d\n", nbits) ;

  float_decode_4x4(decoded, nbits, stream) ;

  fprintf(stderr, "original   :");
  for(i=0 ; i<16 ; i++) fprintf(stderr, "%9f ", array[i]) ;
  fprintf(stderr, "\n") ;
  fprintf(stderr, "stream     :");
  for(i=0 ; i<12 ; i++) fprintf(stderr, "%9.8x ", stream[i]) ;
  fprintf(stderr, "\n") ;
  fprintf(stderr, "\n") ;
  fprintf(stderr, "decoded    :");
  for(i=0 ; i<16 ; i++) fprintf(stderr, "%9f ", decoded[i]) ;
  fprintf(stderr, "\n") ;

  for(i=0 ; i<16 ; i++) { error[i] = decoded[i]-array[i] ; bias += error[i] ; }
  bias = bias / 16 ;
  fprintf(stderr, "bias = %f\n", bias) ;
  fprintf(stderr, "error      :");
  for(i=0 ; i<16 ; i++) fprintf(stderr, "%9f ", error[i]) ;
  fprintf(stderr, "\n") ;
  for(i=0 ; i<16 ; i++) ratio[i] = ((decoded[i]-array[i]) != 0.0f) ? array[i]/(decoded[i]-array[i]) : 1000000.0f ;
  for(i=0 ; i<16 ; i++) ratio[i] = (ratio[i] < 0) ? -ratio[i] : ratio[i] ;
  fprintf(stderr, "orig/error :");
  for(i=0 ; i<16 ; i++) fprintf(stderr, "%9.0f ", ratio[i]) ;
  fprintf(stderr, "\n") ;

  u0 = elapsed_us() ;
  t0 = elapsed_cycles() ;
  for(i=0 ; i<NTIMES ; i++){
    header = float_encode_4x4(array, 4, nbits, stream) ;
//     iarray[0] ^= (0x1) ;    // fool the optimizer to prevent loop modification
  }
  t1 = elapsed_cycles() ;
  u1 = elapsed_us() ;

  fprintf(stderr, "encoding [%d times] : %ld cycles = %ld microseconds, ", NTIMES, t1-t0, u1-u0) ;
  float nano = cycles_to_ns(t1 - t0) ;
  float nano2 = 1000.0f * (u1 - u0) ;
  fprintf(stderr, "ns/value = %5.2f, ", nano / NTIMES / 16) ;
  fprintf(stderr, " %5.2f\n", nano2 / NTIMES / 16) ;
  fprintf(stderr, "\n") ;

  u0 = elapsed_us() ;
  t0 = elapsed_cycles() ;
  for(i=0 ; i<NTIMES ; i++){
    float_decode_4x4(decoded, nbits, stream) ;
  }
  t1 = elapsed_cycles() ;
  u1 = elapsed_us() ;

  fprintf(stderr, "stream     :");
  for(i=0 ; i<12 ; i++) fprintf(stderr, "%9.8x ", stream[i]) ;
  fprintf(stderr, "\n") ;
  fprintf(stderr, "decoded    :");
  for(i=0 ; i<16 ; i++) fprintf(stderr, "%9f ", decoded[i]) ;
  fprintf(stderr, "\n") ;
  for(i=0 ; i<16 ; i++) error[i] = decoded[i]-array[i] ;
  fprintf(stderr, "error      :");
  for(i=0 ; i<16 ; i++) fprintf(stderr, "%9f ", error[i]) ;
  fprintf(stderr, "\n") ;
  for(i=0 ; i<16 ; i++) ratio[i] = ((decoded[i]-array[i]) != 0.0f) ? array[i]/(decoded[i]-array[i]) : 1000000.0f ;
  for(i=0 ; i<16 ; i++) ratio[i] = (ratio[i] < 0) ? -ratio[i] : ratio[i] ;
  fprintf(stderr, "orig/error :");
  for(i=0 ; i<16 ; i++) fprintf(stderr, "%9.0f ", ratio[i]) ;
  fprintf(stderr, "\n") ;

  fprintf(stderr, "decoding [%d times] : %ld cycles = %ld microseconds, ", NTIMES, t1-t0, u1-u0) ;
  nano = cycles_to_ns(t1 - t0) ;
  nano2 = 1000.0f * (u1 - u0) ;
  fprintf(stderr, "ns/value = %5.2f, ", nano / NTIMES / 16) ;
  fprintf(stderr, " %5.2f\n", nano2 / NTIMES / 16) ;
  fprintf(stderr, "\n") ;

  int ni = 4, nj = 4, lng ;
  float err, rat, ratiomin = 999999.0f , ratiomax = 0.0f ;

  nbits = 15 ;
  lng = 0 ;
  array_encode_by_4x4(LNI, ni, nj, &(f[0][0]), &(virt[0]), nbits) ;  // prime the memory pump
  array_decode_by_4x4(LNI, ni, nj, &(r[0][0]), &(virt[0]), nbits) ;
//   array_encode_by_4x4(LNI, ni, nj, array, &(virt[0]), nbits) ;  // prime the memory pump
//   array_decode_by_4x4(LNI, ni, nj, &(r[0][0]), &(virt[0]), nbits) ;
  for(j=0 ; j<nj ; j++){
    for(i=0 ; i<ni ; i++){
      fprintf(stderr, "%9f ", f[j][i]) ;
//       fprintf(stderr, "%9f ", array[4*j+i]) ;
    }
  }
  fprintf(stderr, "\n") ;
  fprintf(stderr, "\n") ;
  for(j=0 ; j<nj ; j++){
    for(i=0 ; i<ni ; i++){
      fprintf(stderr, "%9f ", r[j][i]) ;
    }
  }
  fprintf(stderr, "\n") ;

  ni = 1024 ;
  nj = 1024 ;
  nbits = 15 ;
  t0 = elapsed_cycles() ;
  lng += array_encode_by_4x4(LNI, ni, nj, &(f[0][0]), &(virt[0]), nbits) ;
  t1 = elapsed_cycles() ;
  fprintf(stderr, "encoding %d x %d [%d]: %ld cycles ", ni, nj, lng, t1-t0) ;
  nano = cycles_to_ns(t1 - t0) ;
  fprintf(stderr, "ns/value = %5.2f\n", nano / (ni*nj)) ;

  lng = 0 ;
  t0 = elapsed_cycles() ;
  lng += array_decode_by_4x4(LNI, ni, nj, &(r[0][0]), &(virt[0]), nbits) ;
  t1 = elapsed_cycles() ;
  fprintf(stderr, "decoding %d x %d [%d]: %ld cycles ", ni, nj, lng, t1-t0) ;
  nano = cycles_to_ns(t1 - t0) ;
  fprintf(stderr, "ns/value = %5.2f\n", nano / (ni*nj)) ;

  for(j=0 ; j<nj ; j++){
    for(i=0 ; i<ni ; i++){
      err = r[j][i] - f[j][i] ;
      bias += err ;
      err = (err < 0) ? -err : err ;
      rat = f[j][i] / ((err == 0.0f) ? .0001f : err) ;
      ratiomin = (rat < ratiomin) ? rat : ratiomin ;
      ratiomax = (rat > ratiomax) ? rat : ratiomax ;
    }
  }
  fprintf(stderr, "bias = %f\n", bias/(ni*nj)) ;
  fprintf(stderr, "min error ratio = %8.0f, max error ratio = %8.0f\n", ratiomin, ratiomax) ;
}
