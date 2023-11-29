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
#include <rmn/bits.h>
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
  int32_t available = StreamAvailableBits(&s) ;
  int32_t strict_available = StreamStrictAvailableBits(&s) ;
  int32_t space_available = StreamAvailableSpace(&s) ;
  available = (available < 0) ? 0 : available ;
  strict_available = (strict_available < 0) ? 0 : strict_available ;
  TEE_FPRINTF(stderr,2, "%s: filled = %d(%d), free= %d, first/in/out/limit = %p/%ld/%ld/%ld [%ld], insert/xtract = %d/%d",
    msg, available, strict_available, space_available, 
    s.first, s.in-s.first, s.out-s.first, s.limit-s.first, s.in-s.out, s.insert, s.xtract ) ;
  TEE_FPRINTF(stderr,2, ", full/part/user = %d/%d/%d", s.full, s.part, s.user) ;
  if(expected_mode){
    TEE_FPRINTF(stderr,2, ", Mode = %s(%d) (%s expected)", StreamMode(s), StreamModeCode(s), expected_mode) ;
    if(strcmp(StreamMode(s), expected_mode) != 0) { 
      TEE_FPRINTF(stderr,2, "\nBad mode, exiting\n") ;
      exit(1) ;
    }
  }
  TEE_FPRINTF(stderr,2, "\n") ;
}

// pass 1 : build bit stream, read it
// pass 2 : rewind bit stream, read it again
// pass 3 : use data buffer to build a new stream, read it
uint32_t test_ez_macros(int nvalues){
  bitstream s = null_bitstream, s0, s1 ;

  // initialize stream for both insertion and extraction
  BeStreamInit(&s, NULL, nvalues * sizeof(uint32_t), BIT_INSERT | BIT_XTRACT) ;
  EZ_NEW_INSERT_VARS(s) ;        // declare insertion EZ variables and get values from bit stream
  int i ;
  uint32_t nbits, mask, w32 ;
  size_t available_bits ;

  // fill bit stream s
  nbits = 4 ; mask = 0xF ;
  print_stream_params(s, "s state before insertion", "RW") ;
  for(i=0 ; i<nvalues ; i++){
    if(i == mask){                                     // all bits are SET in i
      nbits += 1 ;                                     // increase nbits, adjust mask accordingly
      mask <<= 1 ;
      mask |= 0x1 ;
      BE64_EZ_INSERT_ALIGN ;                           // align bit stream to a 32 bit boundary
      EZ_SET_INSERT_VARS(s) ;                          // update bitstream struct data and metadata
      available_bits = StreamAvailableBits(&s) ;
      if( available_bits & 31l ) {                     // check 32 bit alignment
        fprintf(stderr, "misaligned data after INSERT_ALIGN\n") ;
        return 1 ;
      }
    }else{
      BE64_EZ_PUT_NBITS(i, nbits) ;                    // insert value i in bit stream using nbits bits
    }
  }
  BE64_EZ_INSERT_ALIGN ;                               // align bit stream to a 32 bit boundary after insertion
  BE64_EZ_INSERT_FINAL ;                               // flush residual data to bit stream
  EZ_SET_INSERT_VARS(s) ;                              // update bitstream struct data and metadata
  print_stream_params(s, "s state after insertion", "RW") ;
  size_t stream_bits = StreamStrictAvailableBits(&s) ;
  fprintf(stderr, "bits available in stream = %5ld\n\n", stream_bits) ;

  print_stream_params(s, "s state before extraction", "RW") ;
  fprintf(stderr, "bits available after init          = %5ld\n", StreamStrictAvailableBits(&s)) ;
  StreamSetFilledBits(&s, stream_bits) ;
  fprintf(stderr, "bits available after SetFilledBits = %5ld\n", StreamStrictAvailableBits(&s)) ;

  EZ_NEW_XTRACT_VARS(s) ;        // declare extraction EZ variables and get values from bit stream
  int pass = 1 ;

  // read bit stream s
read_again:
  fprintf(stderr, "\nread pass %d\n", pass) ;
  print_stream_params(s, "s state at start of read", "RW") ;
  nbits = 4 ; mask = 0xF ;
  BE64_EZ_XTRACT_BEGIN ;
  for(i=0 ; i<nvalues ; i++){
    if(i == mask){                                    // all bits are SET in i
      nbits +=1 ;                                     // increase nbits, adjust mask accordingly
      mask <<= 1 ;
      mask |= 0x1 ;
      BE64_EZ_XTRACT_ALIGN ;                          // skip to a 32 bit boundary in bit stream
      EZ_SET_XTRACT_VARS(s) ;                         // update bitstream struct data and metadata
      available_bits = StreamAvailableBits(&s) ;
      if( available_bits & 31l ) {                    // check 32 bit alignment
        fprintf(stderr, "misaligned data after XTRACT_ALIGN\n") ;
        return 1 ;
      }
    }else{
      BE64_EZ_GET_NBITS(w32, nbits) ;                 // insert value w32 (nbits wide) from bit stream
      if(i != w32){                                   // check that it is the expected value (i)
        fprintf(stderr, "expecting %3d, got %3d, xtract = %4d, accum = %16.16lx\n", i, w32, StReAm_xtract, StReAm_acc_x) ;
        return 1 ;                                    // error
      }
    }
  }
  BE64_EZ_XTRACT_ALIGN ;                              // skip to a 32 bit boundary in bit stream
  EZ_SET_XTRACT_VARS(s) ;                             // update bitstream struct data and metadata
  print_stream_params(s, "s state after read", "RW") ;

  if(pass == 1) {                                     // read s after rewinding s
    fprintf(stderr, "\n") ;
    pass++ ;
    StreamRewind(&s, 0) ;                             // rewind stream, do not force read mode
    EZ_GET_XTRACT_VARS(s) ;
    print_stream_params(s, "s state after rewind", "RW") ;
    goto read_again ;                                 // read stream again
  }

  fprintf(stderr, "\n") ;
  if(pass == 2) {                                     // save s to s0, build s1 using buffer from s0
    fprintf(stderr, "\n") ;
    pass++ ;                                          // set s to s1
    s0 = s ;                                          // copy s to s0 (save original s)
    StreamRewind(&s, 0) ;                            // rewind s, do not force read mode
    print_stream_params(s, "s state after rewind", "RW") ;
    // build s1 using buffer from s, set buffer size to size from s
    BeStreamInit(&s1, STREAM_BUFFER_ADDRESS(s0), STREAM_BUFFER_BYTES(s0), BIT_INSERT | BIT_XTRACT) ;
    print_stream_params(s1, "s1 state after Init", "RW") ;
    // set fill count using available bits count from s
    StreamSetFilledBits(&s1, StreamAvailableBits(&s0)) ;    // 
    print_stream_params(s1, "s1 state after SetFilledBits", "RW") ;
    s = s1 ;
    print_stream_params(s, "s state after copy from s1", "RW") ;
    EZ_GET_XTRACT_VARS(s) ;
    goto read_again ;                                 // read stream again
  }

  fprintf(stderr, "SUCCESS\n") ;
  return 0 ;   // success
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

  TEE_FPRINTF(stderr,2, "=============== EZ macros test ===============\n") ;
  if( test_ez_macros(4097) ) exit(1) ;
// return 0 ;
  TEE_FPRINTF(stderr,2, "=============== functional and syntax test ===============\n") ;

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
  print_stream_params(ple, "Init Le Stream", "RW") ;
  if(STREAM_IS_LITTLE_ENDIAN(ple)) {
    TEE_FPRINTF(stderr,2, "stream ple is little endian as expected\n") ;
  } else {
    exit(1) ;
  }

  LE64_STREAM_INSERT_BEGIN(ple) ;                      // this should force insert (write) mode
  print_stream_params(ple, "LE64_STREAM_INSERT_BEGIN", "RW") ;

  LeStreamInsert(&ple, unpacked, nbits, NPTS) ;
  print_stream_params(ple, "Insert into Le Stream", "RW") ;
  TEE_FPRINTF(stderr,2, "packedle %2d bits : ", nbits) ; for(i=7 ; i>=0 ; i--) TEE_FPRINTF(stderr,2, "%8.8x ", packedle[i]); TEE_FPRINTF(stderr,2, "\n") ;
  // rewind and extract in Little Endian mode (unsigned)
  if(0) LeStreamInit(&ple, packedle, sizeof(packedle), BIT_XTRACT) ;  // syntax check, no code execution
  LeStreamRewind(&ple, 1) ;                               // this should force extract (read) mode
  print_stream_params(ple, "Rewind Le Stream", "RW") ;

  LE64_PEEK_NBITS(ple.acc_x, ple.xtract, peek_u, nbits) ;  // explicit peek macro (unsigned)
  LE64_STREAM_PEEK_NBITS(ple, peek_u2, nbits) ;            // stream peek macro (unsigned)
  TEE_FPRINTF(stderr,2, "peekle(u) = %8.8x %8.8x\n", peek_u, peek_u2) ;
  for(i=0 ; i<NPTS ; i++) restored[i] = 0xFFFFFFFFu ;
  LeStreamXtract(&ple, restored, nbits, NPTS) ;
  TEE_FPRINTF(stderr,2, "restoredle %2d bits: ", nbits) ; for(i=0 ; i<NPTS2 ; i++) TEE_FPRINTF(stderr,2, "%8.8x ", restored[i]); TEE_FPRINTF(stderr,2, "\n") ;

  TEE_FPRINTF(stderr,2, "\n") ;
  // insert in Big Endian mode (unsigned)
  BeStreamInit(&pbe, packedbe, sizeof(packedbe), BIT_INSERT) ;
  if(STREAM_IS_BIG_ENDIAN(pbe)) {
    TEE_FPRINTF(stderr,2, "stream pbe is big endian as expected\n") ;
  } else {
    exit(1) ;
  }
  print_stream_params(pbe, "Be stream in write mode", "W") ;

  BE64_STREAM_INSERT_BEGIN(pbe) ;
  for(i=0 ; i<NPTS ; i++) packedbe[i] = 0xFFFFFFFFu ; 
  BeStreamInsert(&pbe, unpacked, nbits, -NPTS) ;
  TEE_FPRINTF(stderr,2, "packedbe %2d bits: ", nbits) ; for(i=0 ; i<NPTS2 ; i++) TEE_FPRINTF(stderr,2, "%8.8x ", packedbe[i]); TEE_FPRINTF(stderr,2, "\n") ;
  // rewind and extract in Big Endian mode (unsigned)
  TEE_FPRINTF(stderr,2, "Be stream in BIT_XTRACT_ONLY\n") ;
  BeStreamInit(&pbe, packedbe, sizeof(packedbe), BIT_XTRACT) ;  // not necessary, syntax check
  BeStreamRewind(&pbe, 1) ;                               // this should force extract (read) mode
  print_stream_params(pbe, "Be stream in BIT_XTRACT_ONLY", "R") ;

  BE64_PEEK_NBITS(pbe.acc_x, pbe.xtract, peek_u, nbits) ;  // explicit peek macro (unsigned)
  BE64_STREAM_PEEK_NBITS(pbe, peek_u2, nbits) ;            // stream peek macro (unsigned)
  TEE_FPRINTF(stderr,2, "peekbe(u) = %8.8x %8.8x\n", peek_u, peek_u2) ;
  for(i=0 ; i<NPTS ; i++) restored[i] = 0xFFFFFFFFu ;
  BeStreamXtract(&pbe, restored, nbits, NPTS) ;
  TEE_FPRINTF(stderr,2, "restoredbe %2d bits: ", nbits) ; for(i=0 ; i<NPTS2 ; i++) TEE_FPRINTF(stderr,2, "%8.8x ", restored[i]); TEE_FPRINTF(stderr,2, "\n") ;
  TEE_FPRINTF(stderr,2, "\n") ;

  // insert in Little Endian mode (signed)
  LeStreamInit(&ple, packedle, sizeof(packedle), BIT_INSERT) ;
  LeStreamInsert(&ple, (void *) unpacked_signed, nbits, -NPTS) ;
  TEE_FPRINTF(stderr,2, "packedle %2d bits: ", nbits) ; for(i=7 ; i>=0 ; i--) TEE_FPRINTF(stderr,2, "%8.8x ", packedle[i]); TEE_FPRINTF(stderr,2, "\n") ;
  // rewind and extract in Little Endian mode (signed)
  if(0) LeStreamInit(&ple, packedle, sizeof(packedle), BIT_XTRACT) ;  // not necessary, syntax check
  LeStreamRewind(&ple, 1) ;                               // this should force extract (read) mode
  LE64_PEEK_NBITS((int64_t) ple.acc_x, ple.xtract, peek_i, nbits) ;  // explicit peek macro (signed)
  LE64_STREAM_PEEK_NBITS_S(ple, peek_i2, nbits) ;                    // stream peek macro (signed)
  TEE_FPRINTF(stderr,2, "peekle(s) = %8.8x %8.8x\n", peek_i, peek_i2) ;
  for(i=0 ; i<NPTS ; i++) signed_restored[i] = 0xFFFFFFFFu ;
  LeStreamXtractSigned(&ple, signed_restored, nbits, NPTS) ;
  TEE_FPRINTF(stderr,2, "restoredle %2d bits: ", nbits) ; for(i=0 ; i<NPTS2 ; i++) TEE_FPRINTF(stderr,2, "%8.8x ", signed_restored[i]); TEE_FPRINTF(stderr,2, "\n") ;

  // insert in Big Endian mode (signed)
  BeStreamInit(&pbe, packedbe, sizeof(packedbe), BIT_INSERT) ;
  BeStreamInsert(&pbe, (void *) unpacked_signed, nbits, -NPTS) ;
  TEE_FPRINTF(stderr,2, "packedbe %2d bits: ", nbits) ; for(i=0 ; i<NPTS2 ; i++) TEE_FPRINTF(stderr,2, "%8.8x ", packedbe[i]); TEE_FPRINTF(stderr,2, "\n") ;
  // rewind and extract in Big Endian mode (signed)
  if(0) BeStreamInit(&pbe, packedbe, sizeof(packedbe), BIT_XTRACT) ;  // not necessary, syntax check
  BeStreamRewind(&pbe, 1) ;                               // this should force extract (read) mode
  BE64_PEEK_NBITS((int64_t) pbe.acc_x, pbe.xtract, peek_i, nbits) ;  // explicit peek macro (signed)
  BE64_STREAM_PEEK_NBITS_S(pbe, peek_i2, nbits) ;                    // stream peek macro (signed)
  TEE_FPRINTF(stderr,2, "peekbe(s) = %8.8x %8.8x\n", peek_i, peek_i2) ;
  for(i=0 ; i<NPTS ; i++) signed_restored[i] = 0xFFFFFFFFu ;
  BeStreamXtractSigned(&pbe, signed_restored, nbits, NPTS) ;
  TEE_FPRINTF(stderr,2, "restoredbe %2d bits: ", nbits) ; for(i=0 ; i<NPTS2 ; i++) TEE_FPRINTF(stderr,2, "%8.8x ", signed_restored[i]); TEE_FPRINTF(stderr,2, "\n") ;
  TEE_FPRINTF(stderr,2, "\n") ;

// ==================================================== control info tests ====================================================
  TEE_FPRINTF(stderr,2, "=============== control info tests ===============\n") ;
// use allocated bitstream structs
  bitstream *pple, *ppbe ;
  bitstream ple1, ple2 ;

  nbits = 8 ; mask = RMASK32(nbits) ;
  for(i=0 ; i<NPTS ; i++)  unpacked[i] = (i + 15) & mask ;

  pple = StreamCreate(sizeof(packedle)+8, 0) ;                      print_stream_params(*pple, "Create Le Stream (*pple)", "RW") ;
  LeStreamInit(pple, packedle, sizeof(packedle), BIT_INSERT) ;      print_stream_params(*pple, "Init Le Stream (*pple)", "W") ;
  if(pple->first != packedle)                                       TEE_FPRINTF(stderr,2, "pple->first != packedle, Success\n") ;
  LeStreamInsert(pple, unpacked, nbits, NPTS) ; /* with push */     print_stream_params(*pple, "after insert Le Stream (*pple)", "W") ;
  if(StreamAvailableBits(pple) != NPTS * nbits) exit(1) ;           TEE_FPRINTF(stderr,2, "StreamAvailableBits = %d, Success.\n", NPTS * nbits) ;
  LeStreamRewind(pple, 1) ;                                         print_stream_params(*pple, "after rewind Le Stream (*pple)", "RW") ;
  
  LeStreamXtract(pple, restored, nbits, NPTS) ;                     print_stream_params(*pple, "after xtract Le Stream (*pple)", "RW") ;
  if(StreamAvailableBits(pple) != 0) exit(1) ;                      TEE_FPRINTF(stderr,2, "StreamAvailableBits = %d, Success.\n", 0) ;

  TEE_FPRINTF(stderr,2, "\n") ;
  ppbe = StreamCreate(sizeof(packedbe)+8, 0) ;                      print_stream_params(*ppbe, "\nInit Be Stream (*ppbe)", "RW") ;
  LeStreamInit(ppbe, packedle, sizeof(packedle), BIT_INSERT) ;      print_stream_params(*ppbe, "Init Le Stream (*ppbe)", "W") ;
  if(ppbe->first != packedle)                                       TEE_FPRINTF(stderr,2, "ppbe->first != packedle, Success\n") ;
  LeStreamInsert(ppbe, unpacked, nbits, -NPTS) ; /* with flush */   print_stream_params(*ppbe, "after insert Le Stream (*ppbe)", "W") ;
  if(StreamAvailableBits(ppbe) != NPTS * nbits) exit(1) ;           TEE_FPRINTF(stderr,2, "StreamAvailableBits = %d, Success.\n", NPTS * nbits) ;
  LeStreamRewind(ppbe, 1) ;                                         print_stream_params(*ppbe, "after rewind Le Stream (*ppbe)", "RW") ;
  LeStreamXtract(ppbe, restored, nbits, NPTS) ;                     print_stream_params(*ppbe, "after xtract Le Stream (*ppbe)", "RW") ;
  if(StreamAvailableBits(ppbe) != 0) exit(1) ;                      TEE_FPRINTF(stderr,2, "StreamAvailableBits = %d, Success.\n", 0) ;

  StreamDup(&ple1, &ple) ;  TEE_FPRINTF(stderr,2, "\n") ;
  print_stream_params(ple,  "Duplicate Le Stream (ple)", "RW") ;
  print_stream_params(ple1, "Duplicate Le Stream (ple1)", "RW") ;
  StreamDup(&ple1, pple) ;
  print_stream_params(*pple,  "Duplicate Le Stream (*pple)", "RW") ;
  print_stream_params(ple1,   "Duplicate Le Stream (ple1)", "RW") ;
  LeStreamInit(&ple2, NULL, sizeof(packedle), BIT_INSERT) ;         print_stream_params(ple2, "Init Le Stream (ple2)", "W") ;

// ==================================================== timing tests ====================================================
  TEE_FPRINTF(stderr,2, "\n==================================================== timing tests ====================================================\n") ;
  TEE_FPRINTF(stderr,2, "\n%6d items,               insert                            extract (unsigned)                       extract (signed)\n", NPTS) ;
  for(nbits = 1 ; nbits <= 32 ; nbits += 1){
    mask = RMASK32(nbits) ;
    for(i=0 ; i<NPTS ; i++)    unpacked[i] = (i + 15) ;
    for(i=0 ; i<NPTS   ; i+=2) unpacked_signed[i] = -(((unpacked[i]) & mask) >> 1) ;
    for(i=1 ; i<NPTS-1 ; i+=2) unpacked_signed[i] =  (((unpacked[i]) & mask) >> 1) ;
    TEE_FPRINTF(stderr,2, "nbits = %2d", nbits) ;

//  time little endian insertion
    TIME_LOOP(tmin, tmax, tavg, NTIMES, NPTS, buf, bufsiz, \
    LeStreamInit(pple, packedle, sizeof(packedle), BIT_INSERT) ; LeStreamInsert(pple, unpacked, nbits, -NPTS) ) ;
    TEE_FPRINTF(stderr,2, ", %6.2f ns/pt (le)", tavg*nano/NPTS);

//  time big endian insertion
    TIME_LOOP(tmin, tmax, tavg, NTIMES, NPTS, buf, bufsiz, \
    BeStreamInit(ppbe, packedbe, sizeof(packedbe), BIT_INSERT) ; BeStreamInsert(ppbe, unpacked, nbits, -NPTS) ) ;
    TEE_FPRINTF(stderr,2, ", %6.2f ns/pt (be)", tavg*nano/NPTS);

//  time little endian unsigned extraction + error check
    for(i=0 ; i<NPTS ; i++) restored[i] = 0xFFFFFFFFu ;
//     LeStreamInit(pple, packedle, sizeof(packedle), BIT_XTRACT_ONLY) ;
    LeStreamRewind(pple, 1) ;
    LeStreamXtract(pple, restored, nbits, NPTS) ;
    mask = RMASK32(nbits) ;
    errorsle = verify_restore(restored, unpacked, NPTS, mask) ;
//     TIME_LOOP(tmin, tmax, tavg, NTIMES, NPTS, buf, bufsiz, \
//     LeStreamInit(pple, packedle, sizeof(packedle), BIT_XTRACT_ONLY) ; LeStreamXtract(pple, restored, nbits, NPTS) ) ;
    TIME_LOOP(tmin, tmax, tavg, NTIMES, NPTS, buf, bufsiz, \
    LeStreamRewind(pple, 1) ; LeStreamXtract(pple, restored, nbits, NPTS) ) ;
    TEE_FPRINTF(stderr,2, ", = %6.2f ns/pt (le)", tavg*nano/NPTS);

//  time big endian unsigned extraction + error check
    for(i=0 ; i<NPTS ; i++) restored[i] = 0xFFFFFFFFu ;
//     BeStreamInit(ppbe, packedbe, sizeof(packedbe), BIT_XTRACT_ONLY) ;
    BeStreamRewind(ppbe, 1) ;
    BeStreamXtract(ppbe, restored, nbits, NPTS) ;
    mask = RMASK32(nbits) ;
    errorsbe = verify_restore(restored, unpacked, NPTS, mask) ;
//     TIME_LOOP(tmin, tmax, tavg, NTIMES, NPTS, buf, bufsiz, \
//     BeStreamInit(ppbe, packedbe, sizeof(packedbe), BIT_XTRACT_ONLY) ; BeStreamXtract(ppbe, restored, nbits, NPTS) ) ;
    TIME_LOOP(tmin, tmax, tavg, NTIMES, NPTS, buf, bufsiz, \
    BeStreamRewind(ppbe, 1) ; BeStreamXtract(ppbe, restored, nbits, NPTS) ) ;
    TEE_FPRINTF(stderr,2, ", %6.2f ns/pt (be)", tavg*nano/NPTS);

//  time little endian signed extraction + error check
    for(i=0 ; i<NPTS ; i++) signed_restored[i] = 0xFFFFFFFFu ;
    LeStreamInit(pple, packedle, sizeof(packedle), BIT_INSERT) ;
    LeStreamInsert(pple, (void *) unpacked_signed, nbits, -NPTS) ;
//     LeStreamInit(pple, packedle, sizeof(packedle), BIT_XTRACT_ONLY) ;
    LeStreamRewind(pple, 1) ;
    LeStreamXtractSigned(pple, signed_restored, nbits, NPTS) ;
    errorsles = verify_restore(signed_restored, unpacked_signed, NPTS, mask) ;
//     TIME_LOOP(tmin, tmax, tavg, NTIMES, NPTS, buf, bufsiz, \
//     LeStreamInit(pple, packedle, sizeof(packedle), BIT_XTRACT_ONLY) ; LeStreamXtractSigned(pple, signed_restored, nbits, NPTS) ) ;
    TIME_LOOP(tmin, tmax, tavg, NTIMES, NPTS, buf, bufsiz, \
    LeStreamRewind(pple, 1) ; LeStreamXtractSigned(pple, signed_restored, nbits, NPTS) ) ;
    TEE_FPRINTF(stderr,2, ", %6.2f ns/pt (les)", tavg*nano/NPTS);

//  time big endian signed extraction + error check
    for(i=0 ; i<NPTS ; i++) signed_restored[i] = 0xFFFFFFFFu ;
    BeStreamInit(ppbe, packedbe, sizeof(packedbe), BIT_INSERT) ;
    BeStreamInsert(ppbe, (void *) unpacked_signed, nbits, -NPTS) ;
//     BeStreamInit(ppbe, packedbe, sizeof(packedbe), BIT_XTRACT_ONLY) ;
    BeStreamRewind(ppbe, 1) ;
    BeStreamXtractSigned(ppbe, signed_restored, nbits, NPTS) ;
    errorsbes = verify_restore(signed_restored, unpacked_signed, NPTS, mask) ;
//     TIME_LOOP(tmin, tmax, tavg, NTIMES, NPTS, buf, bufsiz, \
//     BeStreamInit(ppbe, packedbe, sizeof(packedbe), BIT_XTRACT_ONLY) ; BeStreamXtractSigned(ppbe, signed_restored, nbits, NPTS) ) ;
    TIME_LOOP(tmin, tmax, tavg, NTIMES, NPTS, buf, bufsiz, \
    BeStreamRewind(ppbe, 1) ; BeStreamXtractSigned(ppbe, signed_restored, nbits, NPTS) ) ;
    TEE_FPRINTF(stderr,2, ", %6.2f ns/pt (bes)", tavg*nano/NPTS);
//
    TEE_FPRINTF(stderr,2, " (%d/%d/%d/%d errors)\n", errorsle, errorsbe, errorsles, errorsbes);
    if(errorsle + errorsbe + errorsles + errorsbes > 0) exit(1) ;
//
  }
  TEE_FPRINTF(stderr,2, "\n") ;
  TEE_FPRINTF(stderr,2, "=============== StreamFree tests ===============\n") ;
  print_stream_params(*pple, "Le Stream (*pple)", "RW") ;
  print_stream_params(*pple, "Be Stream (*ppbe)", "RW") ;

  if(tmax == 0) TEE_FPRINTF(stderr,2, "tmax == 0, should not happen\n") ;

  TEE_FPRINTF(stderr,2, "\n") ;
  TEE_FPRINTF(stderr,2, "StreamFree(&ple)  : ") ; if(StreamFree(&ple))  TEE_FPRINTF(stderr,2, "Success\n") else { exit(1) ; }
  TEE_FPRINTF(stderr,2, "StreamFree(&pbe)  : ") ; if(StreamFree(&pbe))  TEE_FPRINTF(stderr,2, "Success\n") else { exit(1) ; }
  TEE_FPRINTF(stderr,2, "StreamFree(&ple1) : ") ; if(StreamFree(&ple1)) TEE_FPRINTF(stderr,2, "Success\n") else { exit(1) ; }
  TEE_FPRINTF(stderr,2, "StreamFree(&ple2) : ") ; if(StreamFree(&ple2)) { exit(1) ; } else TEE_FPRINTF(stderr,2, "Success\n") ;
  TEE_FPRINTF(stderr,2, "StreamFree(pple)  : ") ; if(StreamFree(pple))  { exit(1) ; } else TEE_FPRINTF(stderr,2, "Success\n") ;
  TEE_FPRINTF(stderr,2, "StreamFree(ppbe)  : ") ; if(StreamFree(ppbe))  { exit(1) ; } else TEE_FPRINTF(stderr,2, "Success\n") ;
}
