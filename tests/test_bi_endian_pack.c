//
// Copyright (C) 2022  Environnement Canada
//
// This is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation,
// version 2.1 of the License.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// Author:
//     M. Valin,   Recherche en Prevision Numerique, 2022
//
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>

#include <rmn/timers.h>
#include <rmn/bi_endian_pack.h>
#include <rmn/tee_print.h>
#include <rmn/test_helpers.h>

#if !defined(NPTS)
#define NPTS 4096
#define NPTS2 12
// #define NPTS 524289
// #define NPTS 9
#endif

// #define NTIMES 100
#define NTIMES 1

static int32_t verify_restore(void *f1, void *f2, int n, uint32_t mask){
  uint32_t *w1 = (uint32_t *) f1 ;
  uint32_t *w2 = (uint32_t *) f2 ;
  int errors = 0 ;
  int i ;
  for(i=0 ; i<n ; i++){
    if((w1[i] & mask) != (w2[i] & mask)) errors++ ;
  }
  return errors ;
}

static void print_stream_params(bitstream s, char *msg, char *expected_mode){
  TEE_FPRINTF(stderr,2, "%s: filled = %d(%d), free= %d, first/in/out = %p/%p/%p [%d], insert/xtract = %d/%d \n",
    msg, StreamAvailableBits(&s), StreamStrictAvailableBits(&s), StreamAvailableSpace(&s), 
    s.first, s.out, s.in, s.in-s.out, s.insert, s.xtract ) ;
  if(expected_mode){
    TEE_FPRINTF(stderr,2, "Stream mode = %s(%d) (%s expected)\n", StreamMode(s), StreamModeCode(s), expected_mode) ;
    if(strcmp(StreamMode(s), expected_mode) != 0) { 
      TEE_FPRINTF(stderr,2, "Bad mode, exiting\n") ;
      exit(1) ;
    }
  }
}

// syntax and functional test for the bi endian pack/unpack macros and functions
int main(int argc, char **argv){
  uint32_t unpacked[NPTS], packedle[NPTS], packedbe[NPTS], restored[NPTS] ;
  int32_t unpacked_signed[NPTS], signed_restored[NPTS] ;
  bitstream ple, pbe ;
  int i, nbits, errorsle, errorsbe, errorsles, errorsbes ;
  uint32_t mask ;
  uint64_t tmin, tmax, tavg, freq ;
  double nano ;
//   uint64_t t[NTIMES] ;
  char buf[1024] ;
  size_t bufsiz = sizeof(buf) ;
  uint32_t peek_u, peek_u2 ;
  int32_t peek_i, peek_i2 ;

  start_of_test("test_bi_endian_pack C");
  freq = cycles_counter_freq() ;
  nano = 1000000000 ;
  nano /= freq ;
//   for(i=0 ; i<NTIMES ; i++) t[i] = 0 ;

  for(i=0 ; i<NPTS ; i++) unpacked[i] = i + 16 ;
  for(i=0 ; i<NPTS   ; i+=2) unpacked_signed[i] = -unpacked[i] ;
  for(i=1 ; i<NPTS-1 ; i+=2) unpacked_signed[i] =  unpacked[i] ;
  TEE_FPRINTF(stderr,2, "original(u)  : ") ;
  for(i=0 ; i<NPTS2 ; i++) TEE_FPRINTF(stderr,2, "%8.8x ", unpacked[i]); TEE_FPRINTF(stderr,2, "\n") ;
  TEE_FPRINTF(stderr,2, "original(s)  : ") ;
  for(i=0 ; i<NPTS2 ; i++) TEE_FPRINTF(stderr,2, "%8.8x ", unpacked_signed[i]); TEE_FPRINTF(stderr,2, "\n") ;
  TEE_FPRINTF(stderr,2, "\n") ;

// ========================== functional and syntax test ==========================
  nbits = 12 ;
  TEE_FPRINTF(stderr,2, "packing %d items, using %d bits : %d bits total\n", NPTS, nbits, nbits*NPTS) ;
  // insert in Little Endian mode (unsigned)
  for(i=0 ; i<NPTS ; i++) packedle[i] = 0xFFFFFFFFu ; 
  LeStreamInit(&ple, packedle, sizeof(packedle), 0) ;  // force read-write stream mode
  print_stream_params(ple, "Init Le Stream", "ReadWrite") ;

  LE64_STREAM_INSERT_BEGIN(ple) ;                      // this should force insert (write) mode
  print_stream_params(ple, "LE64_STREAM_INSERT_BEGIN", "ReadWrite") ;

  LeStreamInsert(&ple, unpacked, nbits, NPTS) ;
  print_stream_params(ple, "Insert into Le Stream", "ReadWrite") ;
  TEE_FPRINTF(stderr,2, "packedle %2d bits : ", nbits) ; for(i=7 ; i>=0 ; i--) TEE_FPRINTF(stderr,2, "%8.8x ", packedle[i]); TEE_FPRINTF(stderr,2, "\n") ;
  // rewind and extract in Little Endian mode (unsigned)
  if(0) LeStreamInit(&ple, packedle, sizeof(packedle), BIT_XTRACT_MODE) ;  // syntax check, no code execution
  LeStreamRewind(&ple) ;                               // this should force extract (read) mode
  print_stream_params(ple, "Rewind Le Stream", "ReadWrite") ;

  LE64_PEEK_NBITS(ple.acc_x, ple.xtract, peek_u, nbits) ;  // explicit peek macro (unsigned)
  LE64_STREAM_PEEK_NBITS(ple, peek_u2, nbits) ;            // stream peek macro (unsigned)
  TEE_FPRINTF(stderr,2, "peekle(u) = %8.8x %8.8x\n", peek_u, peek_u2) ;
  for(i=0 ; i<NPTS ; i++) restored[i] = 0xFFFFFFFFu ;
  LeStreamXtract(&ple, restored, nbits, NPTS) ;
  TEE_FPRINTF(stderr,2, "restoredle %2d bits: ", nbits) ; for(i=0 ; i<NPTS2 ; i++) TEE_FPRINTF(stderr,2, "%8.8x ", restored[i]); TEE_FPRINTF(stderr,2, "\n") ;

  // insert in Big Endian mode (unsigned)
  BeStreamInit(&pbe, packedbe, sizeof(packedbe), BIT_INSERT_MODE) ;
  print_stream_params(pbe, "Be stream in write mode", "Write") ;

  BE64_STREAM_INSERT_BEGIN(pbe) ;
  for(i=0 ; i<NPTS ; i++) packedbe[i] = 0xFFFFFFFFu ; 
  BeStreamInsert(&pbe, unpacked, nbits, -NPTS) ;
  TEE_FPRINTF(stderr,2, "packedbe %2d bits: ", nbits) ; for(i=0 ; i<NPTS2 ; i++) TEE_FPRINTF(stderr,2, "%8.8x ", packedbe[i]); TEE_FPRINTF(stderr,2, "\n") ;
  // rewind and extract in Big Endian mode (unsigned)
  TEE_FPRINTF(stderr,2, "Be stream in BIT_XTRACT_MODE\n") ;
  BeStreamInit(&pbe, packedbe, sizeof(packedbe), BIT_XTRACT_MODE) ;  // not necessary, syntax check
  BeStreamRewind(&pbe) ;                               // this should force extract (read) mode
  print_stream_params(pbe, "Be stream in BIT_XTRACT_MODE", "Read") ;

  BE64_PEEK_NBITS(pbe.acc_x, pbe.xtract, peek_u, nbits) ;  // explicit peek macro (unsigned)
  BE64_STREAM_PEEK_NBITS(pbe, peek_u2, nbits) ;            // stream peek macro (unsigned)
  TEE_FPRINTF(stderr,2, "peekbe(u) = %8.8x %8.8x\n", peek_u, peek_u2) ;
  for(i=0 ; i<NPTS ; i++) restored[i] = 0xFFFFFFFFu ;
  BeStreamXtract(&pbe, restored, nbits, NPTS) ;
  TEE_FPRINTF(stderr,2, "restoredbe %2d bits: ", nbits) ; for(i=0 ; i<NPTS2 ; i++) TEE_FPRINTF(stderr,2, "%8.8x ", restored[i]); TEE_FPRINTF(stderr,2, "\n") ;
  TEE_FPRINTF(stderr,2, "\n") ;

  // insert in Little Endian mode (signed)
  LeStreamInit(&ple, packedle, sizeof(packedle), BIT_INSERT_MODE) ;
  LeStreamInsert(&ple, (void *) unpacked_signed, nbits, -NPTS) ;
  TEE_FPRINTF(stderr,2, "packedle %2d bits: ", nbits) ; for(i=7 ; i>=0 ; i--) TEE_FPRINTF(stderr,2, "%8.8x ", packedle[i]); TEE_FPRINTF(stderr,2, "\n") ;
  // rewind and extract in Little Endian mode (signed)
  if(0) LeStreamInit(&ple, packedle, sizeof(packedle), BIT_XTRACT_MODE) ;  // not necessary, syntax check
  LeStreamRewind(&ple) ;                               // this should force extract (read) mode
  LE64_PEEK_NBITS((int64_t) ple.acc_x, ple.xtract, peek_i, nbits) ;  // explicit peek macro (signed)
  LE64_STREAM_PEEK_NBITS_S(ple, peek_i2, nbits) ;                    // stream peek macro (signed)
  TEE_FPRINTF(stderr,2, "peekle(s) = %8.8x %8.8x\n", peek_i, peek_i2) ;
  for(i=0 ; i<NPTS ; i++) signed_restored[i] = 0xFFFFFFFFu ;
  LeStreamXtractSigned(&ple, signed_restored, nbits, NPTS) ;
  TEE_FPRINTF(stderr,2, "restoredle %2d bits: ", nbits) ; for(i=0 ; i<NPTS2 ; i++) TEE_FPRINTF(stderr,2, "%8.8x ", signed_restored[i]); TEE_FPRINTF(stderr,2, "\n") ;

  // insert in Big Endian mode (signed)
  BeStreamInit(&pbe, packedbe, sizeof(packedbe), BIT_INSERT_MODE) ;
  BeStreamInsert(&pbe, (void *) unpacked_signed, nbits, -NPTS) ;
  TEE_FPRINTF(stderr,2, "packedbe %2d bits: ", nbits) ; for(i=0 ; i<NPTS2 ; i++) TEE_FPRINTF(stderr,2, "%8.8x ", packedbe[i]); TEE_FPRINTF(stderr,2, "\n") ;
  // rewind and extract in Big Endian mode (signed)
  if(0) BeStreamInit(&pbe, packedbe, sizeof(packedbe), BIT_XTRACT_MODE) ;  // not necessary, syntax check
  BeStreamRewind(&pbe) ;                               // this should force extract (read) mode
  BE64_PEEK_NBITS((int64_t) pbe.acc_x, pbe.xtract, peek_i, nbits) ;  // explicit peek macro (signed)
  BE64_STREAM_PEEK_NBITS_S(pbe, peek_i2, nbits) ;                    // stream peek macro (signed)
  TEE_FPRINTF(stderr,2, "peekbe(s) = %8.8x %8.8x\n", peek_i, peek_i2) ;
  for(i=0 ; i<NPTS ; i++) signed_restored[i] = 0xFFFFFFFFu ;
  BeStreamXtractSigned(&pbe, signed_restored, nbits, NPTS) ;
  TEE_FPRINTF(stderr,2, "restoredbe %2d bits: ", nbits) ; for(i=0 ; i<NPTS2 ; i++) TEE_FPRINTF(stderr,2, "%8.8x ", signed_restored[i]); TEE_FPRINTF(stderr,2, "\n") ;
  TEE_FPRINTF(stderr,2, "\n") ;

// ==================================================== timing tests ====================================================
  TEE_FPRINTF(stderr,2, "%6d items,               insert                            extract (unsigned)                       extract (signed)\n", NPTS) ;
  for(nbits = 1 ; nbits <= 32 ; nbits += 1){
    mask = RMask(nbits) ;
    for(i=0 ; i<NPTS ; i++)    unpacked[i] = (i + 15) ;
    for(i=0 ; i<NPTS   ; i+=2) unpacked_signed[i] = -(((unpacked[i]) & mask) >> 1) ;
    for(i=1 ; i<NPTS-1 ; i+=2) unpacked_signed[i] =  (((unpacked[i]) & mask) >> 1) ;
    TEE_FPRINTF(stderr,2, "nbits = %2d", nbits) ;

//  time little endian insertion
    TIME_LOOP(tmin, tmax, tavg, NTIMES, NPTS, buf, bufsiz, \
    LeStreamInit(&ple, packedle, sizeof(packedle), BIT_INSERT_MODE) ; LeStreamInsert(&ple, unpacked, nbits, -NPTS) ) ;
    TEE_FPRINTF(stderr,2, ", %6.2f ns/pt (le)", tavg*nano/NPTS);

//  time big endian insertion
    TIME_LOOP(tmin, tmax, tavg, NTIMES, NPTS, buf, bufsiz, \
    BeStreamInit(&pbe, packedbe, sizeof(packedbe), BIT_INSERT_MODE) ; BeStreamInsert(&pbe, unpacked, nbits, -NPTS) ) ;
    TEE_FPRINTF(stderr,2, ", %6.2f ns/pt (be)", tavg*nano/NPTS);

//  time little endian unsigned extraction + error check
    for(i=0 ; i<NPTS ; i++) restored[i] = 0xFFFFFFFFu ;
    LeStreamInit(&ple, packedle, sizeof(packedle), BIT_XTRACT_MODE) ;
    LeStreamRewind(&ple) ;
    LeStreamXtract(&ple, restored, nbits, NPTS) ;
    mask = RMask(nbits) ;
    errorsle = verify_restore(restored, unpacked, NPTS, mask) ;
    TIME_LOOP(tmin, tmax, tavg, NTIMES, NPTS, buf, bufsiz, \
    LeStreamInit(&ple, packedle, sizeof(packedle), BIT_XTRACT_MODE) ; LeStreamXtract(&ple, restored, nbits, NPTS) ) ;
    TEE_FPRINTF(stderr,2, ", = %6.2f ns/pt (le)", tavg*nano/NPTS);

//  time big endian unsigned extraction + error check
    for(i=0 ; i<NPTS ; i++) restored[i] = 0xFFFFFFFFu ;
    BeStreamInit(&pbe, packedbe, sizeof(packedbe), BIT_XTRACT_MODE) ;
    BeStreamRewind(&pbe) ;
    BeStreamXtract(&pbe, restored, nbits, NPTS) ;
    mask = RMask(nbits) ;
    errorsbe = verify_restore(restored, unpacked, NPTS, mask) ;
    TIME_LOOP(tmin, tmax, tavg, NTIMES, NPTS, buf, bufsiz, \
    BeStreamInit(&pbe, packedbe, sizeof(packedbe), BIT_XTRACT_MODE) ; BeStreamXtract(&pbe, restored, nbits, NPTS) ) ;
    TEE_FPRINTF(stderr,2, ", %6.2f ns/pt (be)", tavg*nano/NPTS);

//  time little endian signed extraction + error check
    for(i=0 ; i<NPTS ; i++) signed_restored[i] = 0xFFFFFFFFu ;
    LeStreamInit(&ple, packedle, sizeof(packedle), BIT_INSERT_MODE) ;
    LeStreamInsert(&ple, (void *) unpacked_signed, nbits, -NPTS) ;
    LeStreamInit(&ple, packedle, sizeof(packedle), BIT_XTRACT_MODE) ;
    LeStreamXtractSigned(&ple, signed_restored, nbits, NPTS) ;
    errorsles = verify_restore(signed_restored, unpacked_signed, NPTS, mask) ;
    TIME_LOOP(tmin, tmax, tavg, NTIMES, NPTS, buf, bufsiz, \
    LeStreamInit(&ple, packedle, sizeof(packedle), BIT_XTRACT_MODE) ; LeStreamXtractSigned(&ple, signed_restored, nbits, NPTS) ) ;
    TEE_FPRINTF(stderr,2, ", %6.2f ns/pt (les)", tavg*nano/NPTS);

//  time big endian signed extraction + error check
    for(i=0 ; i<NPTS ; i++) signed_restored[i] = 0xFFFFFFFFu ;
    BeStreamInit(&pbe, packedbe, sizeof(packedbe), BIT_INSERT_MODE) ;
    BeStreamInsert(&pbe, (void *) unpacked_signed, nbits, -NPTS) ;
    BeStreamInit(&pbe, packedbe, sizeof(packedbe), BIT_XTRACT_MODE) ;
    BeStreamXtractSigned(&pbe, signed_restored, nbits, NPTS) ;
    errorsbes = verify_restore(signed_restored, unpacked_signed, NPTS, mask) ;
    TIME_LOOP(tmin, tmax, tavg, NTIMES, NPTS, buf, bufsiz, \
    BeStreamInit(&pbe, packedbe, sizeof(packedbe), BIT_XTRACT_MODE) ; BeStreamXtractSigned(&pbe, signed_restored, nbits, NPTS) ) ;
    TEE_FPRINTF(stderr,2, ", %6.2f ns/pt (bes)", tavg*nano/NPTS);
//
    TEE_FPRINTF(stderr,2, " (%d/%d/%d/%d errors\n)", errorsle, errorsbe, errorsles, errorsbes);
    if(errorsle + errorsbe + errorsles + errorsbes > 0) exit(1) ;
//
  }

  if(tmax == 0) TEE_FPRINTF(stderr,2, "tmax == 0, should not happen\n") ;
}
