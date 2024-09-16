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
void float_decode_4x4(float *f, int nbits, uint32_t *stream, int header);

int main(){
  float array[32], decoded[32], ratio[32], error[32] ;
  uint32_t *iarray = (uint32_t *) &(array[0]) ;
  uint32_t *idecoded = (uint32_t *) &(decoded[0]) ;
  uint32_t stream[64] ;
  int32_t header = 0 ;
  float epsilon = 0.012345678f ;
  int i ;
  int nbits = 15 ;
  uint64_t t0, t1 ;

  for(i=0 ; i<16 ; i++) array[i] = 1.01f + epsilon + i / 15.97f ;
  array[15] *= 8.0f ;
  for(i=0 ; i<16 ; i++) array[i] = -array[i] ;        // all negative numbers
//   for(i=0 ; i<16 ; i+=2) array[i] = -array[i] ;       // introduce some negative numbers
  header = float_encode_4x4(array, 4, nbits, stream) ;
//   for(i=0 ; i<16 ; i++) fprintf(stderr, "%9.8x ", stream[i]) ;
//   fprintf(stderr, "\n") ;
  fprintf(stderr, "emin = %d, ", header >> 8) ;
  int ebits = header & 7 ;
  fprintf(stderr, "ebits = %d, ", ebits) ;
  int sbits = (header >> 3) & 1 ;
  fprintf(stderr, "sbits = %d, ", sbits) ;
  if(sbits == 0) fprintf(stderr, "sign = %d, ", (header >> 4) & 1) ;
  fprintf(stderr, "mbits = %d, ", nbits - ebits - sbits) ;
  fprintf(stderr, "nbits = %d\n", nbits) ;
  float_decode_4x4(decoded, nbits, stream, header) ;
//   fprintf(stderr, "original   :");
//   for(i=0 ; i<16 ; i++) fprintf(stderr, "%9.8x ", iarray[i]) ;
//   fprintf(stderr, "\n") ;
  fprintf(stderr, "original   :");
  for(i=0 ; i<16 ; i++) fprintf(stderr, "%9f ", array[i]) ;
  fprintf(stderr, "\n") ;
//   fprintf(stderr, "decoded    :");
//   for(i=0 ; i<16 ; i++) fprintf(stderr, "%9.8x ", idecoded[i]) ;
//   fprintf(stderr, "\n") ;
  fprintf(stderr, "decoded    :");
  for(i=0 ; i<16 ; i++) fprintf(stderr, "%9f ", decoded[i]) ;
  fprintf(stderr, "\n") ;
//   fprintf(stderr, "original   :");
//   for(i=0 ; i<16 ; i++) fprintf(stderr, "%9f ", array[i]) ;
//   fprintf(stderr, "\n") ;
  for(i=0 ; i<16 ; i++) error[i] = decoded[i]-array[i] ;
  fprintf(stderr, "error      :");
  for(i=0 ; i<16 ; i++) fprintf(stderr, "%9f ", error[i]) ;
  fprintf(stderr, "\n") ;
  for(i=0 ; i<16 ; i++) ratio[i] = ((decoded[i]-array[i]) != 0.0f) ? array[i]/(decoded[i]-array[i]) : 1000000.0f ;
  for(i=0 ; i<16 ; i++) ratio[i] = (ratio[i] < 0) ? -ratio[i] : ratio[i] ;
  fprintf(stderr, "orig/error :");
  for(i=0 ; i<16 ; i++) fprintf(stderr, "%9.0f ", ratio[i]) ;
  fprintf(stderr, "\n") ;

  t0 = elapsed_cycles() ;
  for(i=0 ; i<50000000 ; i++){
    header = float_encode_4x4(array, 4, nbits, stream) ;
    iarray[0] ^= (i & 0xFFFF) ;
  }
  t1 = elapsed_cycles() ;
  float nano = cycles_to_ns(t1 - t0) ;
  fprintf(stderr, "ns/value = %5.2f\n", nano / i / 16) ;
}
