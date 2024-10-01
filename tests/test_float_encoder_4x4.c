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
#include <rmn/ct_assert.h>

#define WITH_TIMING

#if defined(WITH_TIMING)
#include <rmn/timers.h>
#else
#define TIME_LOOP_DATA ;
#define TIME_LOOP_EZ(niter, npts, code) ;
  char *timer_msg = "" ;
  float NaNoSeC = 0.0f ;
#endif

// variable length array
typedef float float_vla[] ;
// pointer to variable lengtp array
#define FLOAT_VLAP (float_vla *)
// #define FLOAT_VLAP (float(*)[])
// #define FLOAT_VLAP (vla *)

uint16_t *float_encode_4x4(float *f, int lni, int nbits, uint16_t *stream, uint32_t ni, uint32_t nj, uint32_t *head);
uint16_t *float_decode_4x4(float *f, int nbits, void *stream, uint32_t *head);

#define NTIMES 10000

static float array[32], decoded[32], ratio[32], error[32] ;

#define LNI 1027
#define LNJ 1025
static float f[LNJ][LNI], r[LNJ][LNI] ;
static uint16_t virt[LNJ*LNI*2] ;

typedef struct{
  uint32_t vrs1:     8 ,            // versioning indicator
           spare1:   8 ,
           spare2:   8 ,
           spare3:   8 ,
           mode:    12 ,            // packing mode (s/e/m, normalized mantissa, ...)
           nbits:    6 ,            // number of bits per item in packed 4x4 block (0 -> 31)
           bsize:    6 ,            // size of packed 4x4 blocks (including optional block header) in 16 bit units
           ni0:      4 ,            // useful dimension along i of last block in row (0 -> 4)
           nj0:      4 ;            // useful dimension along j of blocks in last(top) row (0 -> 4)
} vprop_2d ;
CT_ASSERT(sizeof(vprop_2d) == 2 * sizeof(uint32_t), "wrong size of struct vprop_2d")

// return index of first point of a block along a dimension
static uint32_t block_2_indx(uint32_t block){
  return block * 4 ;
}
// return block number containing index along a dimension
static uint32_t indx_2_block(uint32_t indx){
  return indx / 4 ;
}

typedef struct{
  vprop_2d desc ;             // virtual array properties
  uint32_t gni, gnj ;         // original float array dimensions
  uint16_t data[] ;           // packed data, bsize * gni * gni * 16 bits
}virtual_float_2d ;

// gni    [IN] : row storage length of virtual array
// lni    [IN] : row storage length of array r
// ni     [IN] : number of values to get along i
// nj     [IN] : number of values to get along j
// r     [OUT] : array to receive extracted section of original virtual array r[nj][lni]
// ix0    [IN] : offset along i in virtual array
// jx0    [IN] : offset along j in virtual array
// stream [IN] : virtual array stream
// nbits  [IN] : number of bits per packed value (virtual array)
int32_t array_section_by_4x4(uint32_t gni, uint32_t lni, uint32_t ni, uint32_t nj, float r[nj][lni], uint32_t ix0, uint32_t jx0, uint16_t *stream0, uint32_t nbits){
  uint32_t ixn = ix0 + ni -1, jxn = jx0 + nj - 1, nbi = (gni+3)/4 ;
  uint32_t bi0 = ix0/4, bin = 1+ixn/4 , bj0 = jx0/4, bjn = 1+jxn/4 ;
  uint32_t bi, bj ;
  float localf[4][4] ;
  uint32_t bsize = nbits + 1 ;
  int full, fullj ;
fprintf(stderr, "blocks[%d:%d,%d:%d]\n", bi0, bin-1, bj0, bjn-1);
  for(bj=bj0 ; bj<bjn ; bj++){
    int j0 = bj*4 ;
    fullj = (j0 >= jx0) & (j0+3 <= jxn) ;
    int jl = (j0  >    jx0) ? j0  : jx0 ;        // max(j0,jx0)    row target range
    int jh = (jxn < j0+4  ) ? jxn : j0+4 ;       // min(j0+4,jxn)
    for(bi=bi0 ; bi<bin ; bi++){
      int i0 = bi*4 ;
      uint16_t *stream = stream0 + (bi + (bj*nbi)) * bsize ;
      int il = (i0  >    ix0) ? i0  : ix0 ;        // max(i0,ix0)  column target range
      int ih = (ixn < i0+4  ) ? ixn : i0+4 ;       // min(i0+4,ixn)
      full = fullj & (i0 >= ix0) & (i0+3 <= ixn) ;
// fprintf(stderr, "i0 = %2d, j0 = %2d, restoring[%2d:%2d,%2d:%2d], stream - stream0 = %4ld, %s\n",
//         i0, j0, ix0, ixn, jx0, jxn, stream - stream0, full ? "full" : "part") ;
      float_decode_4x4(&(localf[0][0]), nbits, stream, NULL) ;  // decode 4x4 block to local array
      int i, j ;
fprintf(stderr, "[irange:jrange] = [%d:%d][%d:%d] block[%d,%d]\n", il-i0, ih-i0, jl-j0, jh-j0, bi, bj);
      if(full){                                                 // full block, copy all into result
        for(j=0 ; j<4 ; j++) for(i=0 ; i<4 ; i++) r[j0+j-jx0][i0+i-ix0] = localf[j][i] ;
      }else{
        for(j=jl-j0 ; j<=jh-j0 ; j++){                  // copy target range
          for(i=il-i0 ; i<=ih-i0 ; i++){
            r[j0+j-jx0][i0+i-ix0] = localf[j][i] ;
          }
        }
//         for(j=0 ; j<4 ; j++){                                   // only copy relevant part into result
//           if( j0+j < jx0 || j0+j > jxn ) continue ;             // row not in target range
//           for(i=0 ; i<4 ; i++){
//             if(i0+i < ix0 || i0+i > ixn ) continue ;            // column not in target range
//             r[j0+j-jx0][i0+i-ix0] = localf[j][i] ;
//           }
//         }
      }
    }
  }
  return 0 ;
}

int32_t array_decode_by_4x4(int lni, int ni, int nj, float r[nj][lni], uint16_t *stream, int nbits){
  int i0, j0, lng ;
//   int32_t header ;
  float localf[4][4] ;
  lng = 0 ;
  for(j0=0 ; j0<nj ; j0+=4){
    for(i0=0 ; i0<ni ; i0+=4){
      stream = float_decode_4x4(&(localf[0][0]), nbits, stream, NULL) ;  // decode 4x4 block to local array
      int i, j ;
      for(j=0 ; j<4 && j0+j<nj ; j++){                          // copy relevant part into result
        for(i=0 ; i<4 && i0+i<ni ; i++){
          r[j0+j][i0+i] = localf[j][i] ;
        }
      }
//       stream += (nbits+1) ;
      lng += (nbits+1) ;
    }
  }
  return lng ;
}

int32_t array_encode_by_4x4(int lni, int ni, int nj, float f[nj][lni], uint16_t *stream, int nbits){
  int i0, j0, lng ;
  int32_t header ;
  uint16_t *stream0 = stream ;
  lng = 0 ;
  for(j0=0 ; j0<nj ; j0+=4){
    for(i0=0 ; i0<ni ; i0+=4){
fprintf(stderr, "encoding at i0 = %d, j0 = %d, [%6ld] ", i0, j0, stream - stream0) ;
      stream = float_encode_4x4(&(f[j0][i0]), lni, nbits, stream, ni-i0, nj-j0, NULL) ;
fprintf(stderr, "encoded  at i0 = %d, j0 = %d, [%6ld]\n\n", i0, j0, stream - stream0) ;
//       if(header) stream += (nbits+1) / 2 ;
      lng += (nbits+1) / 2 ;
    }
  }
  return stream - stream0 ;
}

static float f55[5][5], r55[5][5] ;
// static float f75[7][5] ;
// static float f57[5][7] ;
static float f77[7][7], r77[7][7] ;
static float f11[11][11], r11[11][11] ;

int main(int argc, char **argv){
//   uint32_t *iarray = (uint32_t *) &(array[0]) ;
  uint16_t stream77[1024], stream11[1024], stream16[1024] ;
  uint32_t *stream = (uint32_t *) &(stream16[0]) ;
  uint32_t header = 0 ;
  float epsilon = 0.012345678f, scale = 2.99f, xrand, xmax, xmin, bias = 0.0f ;
  int i, j ;
  int nbits = 15 ;
  uint64_t t0, t1, u0, u1 ;

  if(cycles_overhead == 0) cycles_overhead = 1 ;    // get rid of annoying warning

  for(j=0 ; j<5 ; j++) for(i=0 ; i<5 ; i++) f55[j][i] = (i+0)*100.0f + (j+0)*1.0f + 10000.0f ;
  for(j=0 ; j<7 ; j++) for(i=0 ; i<7 ; i++) f77[j][i] = (i+0)*100.0f + (j+0)*1.0f + 10000.0f ;
  for(j=0 ; j<11; j++) for(i=0 ; i<11; i++) f11[j][i] = (i+0)*100.0f + (j+0)*1.0f + 10000.0f ;

//   int32_t lng55 = array_encode_by_4x4(5, 5, 5, FLOAT_VLAP &(f55[0][0]), &(stream16[0]), 23) ;
//   fprintf(stderr, "lng55 = %d\n\n", lng55) ;
  int32_t lng77 = array_encode_by_4x4(7, 7, 7, FLOAT_VLAP &(f77[0][0]), &(stream77[0]), nbits) ;
  fprintf(stderr, "lng77 = %d\n\n", lng77) ;
  int32_t lng11 = array_encode_by_4x4(11, 11, 11, FLOAT_VLAP &(f11[0][0]), &(stream11[0]), nbits) ;
  fprintf(stderr, "lng11 = %d\n\n", lng11) ;

  array_decode_by_4x4(7, 7, 7, FLOAT_VLAP &(r77[0][0]), stream77, nbits) ;
  fprintf(stderr, "7 x 7 r77 array\n");
  for(j=6 ; j>=0 ; j--){
    for(i=0 ; i<7 ; i++){
      fprintf(stderr, "%8.0f ", r77[j][i]) ;
    }
    fprintf(stderr, "\n");
  }
  fprintf(stderr, "\n");

  fprintf(stderr, "11 x 11 r11 array\n");
  array_decode_by_4x4(11, 11, 11, FLOAT_VLAP &(r11[0][0]), stream11, nbits) ;
  for(j=10 ; j>=0 ; j--){
    for(i=0 ; i<11 ; i++){
      fprintf(stderr, "%8.0f ", r11[j][i]) ;
    }
    fprintf(stderr, "\n");
  }
  fprintf(stderr, "\n");

  // get 5x5 section from 7x7 array
  fprintf(stderr, "r77[1:5,3:6] array\n");
  array_section_by_4x4(7, 5, 5, 4, FLOAT_VLAP &(r55[0][0]), 1, 3, stream77, nbits) ;
  for(j=3 ; j>=0 ; j--){
    for(i=0 ; i<5 ; i++){
      fprintf(stderr, "%8.0f ", r55[j][i]) ;
    }
    fprintf(stderr, "\n");
  }
  fprintf(stderr, "\n");

  // get 7x6 section from 11x11 array
  fprintf(stderr, "r11[3:9,4:8] array\n");
  array_section_by_4x4(11, 7, 7, 5, FLOAT_VLAP &(r77[0][0]), 3, 4, stream11, nbits) ;
  for(j=4 ; j>=0 ; j--){
    for(i=0 ; i<7 ; i++){
      fprintf(stderr, "%8.0f ", r77[j][i]) ;
    }
    fprintf(stderr, "\n");
  }
  fprintf(stderr, "\n");
return 0 ;
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

  float_encode_4x4(array, 4, nbits, (uint16_t *)stream, 4 ,4, &header) ;

  fprintf(stderr, "emin = %d, ", header >> 8) ;
  int ebits = header & 7 ;
  fprintf(stderr, "ebits = %d, ", ebits) ;
  int sbits = (header >> 3) & 1 ;
  fprintf(stderr, "sbits = %d, ", sbits) ;
  if(sbits == 0) fprintf(stderr, "sign = %d, ", (header >> 4) & 1) ;
  fprintf(stderr, "mbits = %d, ", nbits - ebits - sbits) ;
  fprintf(stderr, "nbits = %d\n", nbits) ;

  float_decode_4x4(decoded, nbits, stream, NULL) ;

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
    float_encode_4x4(array, 4, nbits, (uint16_t *)stream, 4, 4, NULL) ;  // encode complete 4x4 blocks
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
    float_decode_4x4(decoded, nbits, (uint16_t *)stream, NULL) ;
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
  array_encode_by_4x4(LNI, ni, nj, FLOAT_VLAP &(f[0][0]), &(virt[0]), nbits) ;  // prime the memory pump
  array_decode_by_4x4(LNI, ni, nj, (float(*)[]) &(r[0][0]), &(virt[0]), nbits) ;
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
  lng += array_encode_by_4x4(LNI, ni, nj, (float(*)[]) &(f[0][0]), &(virt[0]), nbits) ;
  t1 = elapsed_cycles() ;
  fprintf(stderr, "encoding %d x %d [%d]: %ld cycles ", ni, nj, lng, t1-t0) ;
  nano = cycles_to_ns(t1 - t0) ;
  fprintf(stderr, "ns/value = %5.2f\n", nano / (ni*nj)) ;

  lng = 0 ;
  t0 = elapsed_cycles() ;
  lng += array_decode_by_4x4(LNI, ni, nj, (float(*)[]) &(r[0][0]), &(virt[0]), nbits) ;
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
