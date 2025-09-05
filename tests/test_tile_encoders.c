//
// Copyright (C) 2025  Environnement Canada
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
//     M. Valin,   Recherche en Prevision Numerique, 2025
//
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>

#include <rmn/test_helpers.h>
#include <rmn/move_blocks.h>

// deliberate double inclusion
#include <rmn/timers.h>
#include <rmn/timers.h>
#include <rmn/tile_encoders.h>
#include <rmn/tile_encoders.h>
#include <rmn/cpp_extras.h>
#include <rmn/cpp_extras.h>

#include <rmn/be_stream.h>

#define NPTI 8
#define NPTJ 8
#define NPT (NPTI*NPTJ)

void print_tile(int32_t *tile_in, int nval, char *msg){
  int i ;
  fprintf(stderr,"%s", msg) ;
  for(i=0 ; i<nval ; i++) fprintf(stderr," %d", tile_in[i]) ;
  fprintf(stderr,"\n") ;
}

void move_block(int32_t *block_in, int32_t *block_out, int lnix, int ni, int nj, int tile_size){
  int i0, lni, j0, lnj, nval = tile_size*tile_size ;
  int32_t tile[nval] ;
  int lnis = lnix, lnid = lnix ;

  for(j0=0 ; j0<nj ; j0+=tile_size){
    lnj = ((j0+tile_size) > nj) ? (nj - j0) : tile_size ;
    int32_t *src = block_in ;
    int32_t *dst = block_out ;
    for(i0=0 ; i0<ni ; i0+=tile_size){
      lni = ((i0+tile_size) > ni) ? (ni - i0) : tile_size ;
      move_w32_block(src, lnis, tile, tile_size, lni, lnj, NULL) ;   // get tile from block_in
      move_w32_block(tile, tile_size, dst, lnid, lni, lnj, NULL) ;   // put tile into block_out
      src += tile_size ;
      dst += tile_size ;
    }
    block_in  += (tile_size*lnis) ;
    block_out += (tile_size*lnid) ;
  }
}

int main(int argc, char **argv){
  uint64_t freq ;
  double nano ;
//   TIME_LOOP_DATA ;
  int i, j, status = 1, tilebits, tilebits2, totalbits, totalbits2, errors ;
  int32_t tile00[NPT], tile01[NPT], tile10[NPT], tile11[NPT] ;
  int32_t rest00[NPT], rest01[NPT], rest10[NPT], rest11[NPT] ;
  block_properties bp00, bp01, bp10, bp11 ;
  bitstream *ps ;

  // dummy code to avoid warnings
  if(argc > 1 && argv[1] == NULL) goto fail ;
  if(argc > 100) goto end ;

  // LOCAL scope compile time assertions
  CT_ASSERT_(8 == sizeof(nano))
  CT_ASSERT_(8 == sizeof(freq))
  CT_ASSERT_(2 == sizeof(uint16_t))

  freq = cycles_counter_freq() ;
  nano = 1000000000 ;
  nano /= freq ;

  start_of_test("C tile encoder test");

  if(argc > 1) goto bypass2 ;

  fprintf(stderr, "\n============================== mover test ==============================\n\n") ;
  for(i=0 ; i<NPT ; i++){
    tile00[i] = -123456 ;         // constant value
    tile01[i] = i * 64 ;          // all values >= 0, 0 -> 64*NPT
    tile10[i] = -tile01[i] ;      // all values <= 0
    tile11[i] = (i - 3) * 64  ;   // mixed signs
  }
  status = move_w32_block(tile00, NPTI, rest00, NPTI, NPTI, NPTJ, &bp00) ; // dummy move to force data analysis
  if(status != NPT){ status = 1 ; goto fail ; }
  /*adjust_block_properties(&bp00, int_data) ;*/ print_int_props(bp00);

  status = move_w32_block(tile01, NPTI, rest00, NPTI, NPTI, NPTJ, &bp01) ; // dummy move to force data analysis
  if(status != NPT){ status = 2 ; goto fail ; }
  /*adjust_block_properties(&bp01, int_data) ;*/ print_int_props(bp01);

  status = move_w32_block(tile10, NPTI, rest00, NPTI, NPTI, NPTJ, &bp10) ; // dummy move to force data analysis
  if(status != NPT){ status = 3 ; goto fail ; }
  /*adjust_block_properties(&bp10, int_data) ;*/ print_int_props(bp10);

  status = move_w32_block(tile11, NPTI, rest00, NPTI, NPTI, NPTJ, &bp11) ; // dummy move to force data analysis
  if(status != NPT){ status = 4 ; goto fail ; }
  /*adjust_block_properties(&bp11, int_data) ;*/ print_int_props(bp11);

  // create bit stream
  STREAM_CREATE(ps, NULL, sizeof(uint32_t)*NPT*8, BIT_FULL_INIT) ;
  if(ps->endian != PACK_ENDIAN){ status = 5 ; goto fail ; }
  if(StreamAvailableBits(ps) != 0){ status = 6 ; goto fail ; }
  if(StreamAvailableSpace(ps) != 8*sizeof(uint32_t)*NPT*8){ status = 7 ; goto fail ; }
  STREAM_INSERT_BEGIN(*ps) ;

  fprintf(stderr, "\n============================== tile encoding test ==============================\n\n") ;

  totalbits = 0 ; tilebits2 = 0 ;
//   tilebits = encode_tile(ps, tile00, NPT, &bp00) ;          // constant values
  tilebits2 = encode_block(ps, tile00, NPTI, NPTI, NPTJ, NPTI, ENCODE_DRY_RUN) ;
  tilebits  = encode_block(ps, tile00, NPTI, NPTI, NPTJ, NPTI, 0) ;
  totalbits += tilebits ;
  fprintf(stderr, "tilebits for tile00  = %d (%d)\n", tilebits, tilebits2) ;
  print_encode_stats(0) ; fprintf(stderr, "\n");

//   tilebits = encode_tile(ps, tile01, NPT, &bp01) ;          // all values >= 0, nbits <= 16
  tilebits2 = encode_block(ps, tile01, NPTI, NPTI, NPTJ, NPTI, ENCODE_DRY_RUN) ;
  tilebits  = encode_block(ps, tile01, NPTI, NPTI, NPTJ, NPTI, 0) ;
  totalbits += tilebits ;
  fprintf(stderr, "tilebits for tile01  = %d (%d)\n", tilebits, tilebits2) ;
  print_encode_stats(0) ; fprintf(stderr, "\n");

//   tilebits = encode_tile(ps, tile10, NPT, &bp10) ;          // all values <= 0, nbits <= 16
  tilebits2 = encode_block(ps, tile10, NPTI, NPTI, NPTJ, NPTI, ENCODE_DRY_RUN) ;
  tilebits  = encode_block(ps, tile10, NPTI, NPTI, NPTJ, NPTI, 0) ;
  totalbits += tilebits ;
  fprintf(stderr, "tilebits for tile10  = %d (%d)\n", tilebits, tilebits2) ;
  print_encode_stats(0) ; fprintf(stderr, "\n");

//   tilebits = encode_tile(ps, tile11, NPT, &bp11) ;         // mixed signs, nbits <= 16
  tilebits2 = encode_block(ps, tile11, NPTI, NPTI, NPTJ, NPTI, ENCODE_DRY_RUN) ;
  tilebits  = encode_block(ps, tile11, NPTI, NPTI, NPTJ, NPTI, 0) ;
  print_int_props(bp11) ;
  totalbits += tilebits ;
  fprintf(stderr, "tilebits for tile11  = %d (%d)\n", tilebits, tilebits2) ;
  print_encode_stats(0) ; fprintf(stderr, "\n");

// ========================  block properties pointer is now NULL ========================

  for(i=0 ; i<NPT ; i++) tile11[i] <<= 5 ;                 // mixed signs, nbits > 16
//   tilebits = encode_tile(ps, tile11, NPT, NULL) ;          // block properties are no longer correct
  tilebits2 = encode_block(ps, tile11, NPTI, NPTI, NPTJ, NPTI, ENCODE_DRY_RUN) ;
  tilebits  = encode_block(ps, tile11, NPTI, NPTI, NPTJ, NPTI, 0) ;
  totalbits += tilebits ;
  fprintf(stderr, "tilebits for tile11a = %d (%d)\n", tilebits, tilebits2) ;
  print_encode_stats(0) ; fprintf(stderr, "\n");

  for(i=0 ; i<NPT ; i++) tile11[i] = (i - 3) * 64  ;       // mixed signs, nbits <= 16
  for(i=NPT/4 ; i<3*NPT/4 ; i++) tile11[i] = 0 ;           // make short/long encoding usable
//   tilebits = encode_tile(ps, tile11, NPT, NULL) ;          // block properties are no longer correct
  tilebits2 = encode_block(ps, tile11, NPTI, NPTI, NPTJ, NPTI, ENCODE_DRY_RUN) ;
  tilebits  = encode_block(ps, tile11, NPTI, NPTI, NPTJ, NPTI, 0) ;
  totalbits += tilebits ;
  fprintf(stderr, "tilebits for tile11b = %d (%d)\n", tilebits, tilebits2) ;
  print_encode_stats(0) ; fprintf(stderr, "\n");

  for(i=0 ; i<NPT ; i++) tile11[i] <<= 5  ;                // mixed signs, nbits > 16
  for(i=NPT/4 ; i<3*NPT/4 ; i++) tile11[i] = 0 ;           // make short/long encoding usable
//   tilebits = encode_tile(ps, tile11, NPT, NULL) ;          // block properties are no longer correct
  tilebits2 = encode_block(ps, tile11, NPTI, NPTI, NPTJ, NPTI, ENCODE_DRY_RUN) ;
  tilebits  = encode_block(ps, tile11, NPTI, NPTI, NPTJ, NPTI, 0) ;
  totalbits += tilebits ;
  fprintf(stderr, "tilebits for tile11c = %d (%d)\n", tilebits, tilebits2) ;
  print_encode_stats(0) ; fprintf(stderr, "\n");

  for(i=0 ; i<NPT ; i++) tile11[i] = (i + 3) * 64 ;        // all > 0, nbits <= 16
  for(i=NPT/4 ; i<3*NPT/4 ; i++) tile11[i] = 0 ;           // make short/long encoding usable
  for(i=0 ; i<NPT ; i++) tile11[i] += 60000 ;              // force large offset
//   tilebits = encode_tile(ps, tile11, NPT, NULL) ;          // block properties are no longer correct
  tilebits2 = encode_block(ps, tile11, NPTI, NPTI, NPTJ, NPTI, ENCODE_DRY_RUN) ;
  tilebits  = encode_block(ps, tile11, NPTI, NPTI, NPTJ, NPTI, 0) ;
  totalbits += tilebits ;
  fprintf(stderr, "tilebits for tile11d = %d (%d)\n", tilebits, tilebits2) ;
  print_encode_stats(0) ; fprintf(stderr, "\n");

  for(i=0 ; i<NPT ; i++) tile11[i] = -(i + 3) * 64 ;       // all < 0, nbits <= 16
  for(i=NPT/4 ; i<3*NPT/4 ; i++) tile11[i] = 0 ;           // make short/long encoding usable
  for(i=0 ; i<NPT ; i++) tile11[i] -= 10000 ;              // force large offset
//   tilebits = encode_tile(ps, tile11, NPT, NULL) ;          // block properties are no longer correct
  tilebits2 = encode_block(ps, tile11, NPTI, NPTI, NPTJ, NPTI, ENCODE_DRY_RUN) ;
  tilebits  = encode_block(ps, tile11, NPTI, NPTI, NPTJ, NPTI, 0) ;
  totalbits += tilebits ;
  fprintf(stderr, "tilebits for tile11e = %d (%d)\n", tilebits, tilebits2) ;
  print_encode_stats(1) ; fprintf(stderr, "\n");
goto bypass1;
bypass1:
  STREAM_INSERT_FINALIZE(*ps) ;
  fprintf(stderr, "StreamAvailableBits = %ld, totalbits = %d, padding = %ld bits\n", StreamAvailableBits(ps), totalbits, StreamAvailableBits(ps)-totalbits) ;
  if(totalbits > StreamAvailableBits(ps)){
    fprintf(stderr, "expecting at least %d bits available from stream, found %ld\n", totalbits, StreamAvailableBits(ps)) ;
    status = 8 ;
    goto fail ;
  }

  fprintf(stderr, "\n============================== tile decoding test ==============================\n\n") ;

  STREAM_XTRACT_BEGIN(*ps) ;
  fprintf(stderr, "decoding constant tile\n");
//   tilebits = decode_tile(ps, rest00, NPT) ;
  tilebits = decode_block(ps, rest00, NPTI, NPTI, NPTJ, NPTI);
  errors = 0 ;
  for(i=0 ; i<NPT ; i++) if(rest00[i] != tile00[i]){
    errors++ ;
    if(errors < 4) fprintf(stderr, "[%2d] : expected %d, got %d\n", i, tile00[i], rest00[i]) ;
  }
  fprintf(stderr, "tilebits for tile00  = %d, errors = %d\n\n", tilebits, errors) ;
  if(errors > 0) goto fail ;

  fprintf(stderr, "decoding all >= 0 tile\n");
//   tilebits = decode_tile(ps, rest01, NPT) ;
  tilebits = decode_block(ps, rest01, NPTI, NPTI, NPTJ, NPTI);
  errors = 0 ;
  for(i=0 ; i<NPT ; i++) if(rest01[i] != tile01[i]){
    errors++ ;
    if(errors < 4) fprintf(stderr, "[%2d] : expected %d, got %d\n", i, tile01[i], rest01[i]) ;
  }
  fprintf(stderr, "tilebits for tile01  = %d, errors = %d\n\n", tilebits, errors) ;
  if(errors > 0) goto fail ;

  fprintf(stderr, "decoding all <= 0 tile\n");
//   tilebits = decode_tile(ps, rest10, NPT) ;
  tilebits = decode_block(ps, rest10, NPTI, NPTI, NPTJ, NPTI);
  errors = 0 ;
  for(i=0 ; i<NPT ; i++) if(rest10[i] != tile10[i]){
    errors++ ;
    if(errors < 4) fprintf(stderr, "[%2d] : expected %d, got %d\n", i, tile10[i], rest10[i]) ;
  }
  fprintf(stderr, "tilebits for tile10  = %d, errors = %d\n\n", tilebits, errors) ;
  if(errors > 0) goto fail ;

  fprintf(stderr, "decoding mixed signs tile11\n");
  for(i=0 ; i<NPT ; i++) tile11[i] = (i - 3) * 64  ;   // mixed signs
//   tilebits = decode_tile(ps, rest11, NPT) ;
  tilebits = decode_block(ps, rest11, NPTI, NPTI, NPTJ, NPTI);
  errors = 0 ;
  for(i=0 ; i<NPT ; i++) if(rest11[i] != tile11[i]){
    errors++ ;
    if(errors < 4) fprintf(stderr, "[%2d] : expected %d, got %d\n", i, tile11[i], rest11[i]) ;
  }
  fprintf(stderr, "tilebits for tile11  = %d, errors = %d\n\n", tilebits, errors) ;
  if(errors > 0) goto fail ;

  fprintf(stderr, "decoding mixed signs tile11a\n");
  for(i=0 ; i<NPT ; i++) tile11[i] <<= 5 ;                 // mixed signs, nbits > 16
//   tilebits = decode_tile(ps, rest11, NPT) ;
  tilebits = decode_block(ps, rest11, NPTI, NPTI, NPTJ, NPTI);
  errors = 0 ;
  for(i=0 ; i<NPT ; i++) if(rest11[i] != tile11[i]){
    errors++ ;
    if(errors < 4) fprintf(stderr, "[%2d] : expected %d, got %d\n", i, tile11[i], rest11[i]) ;
  }
  fprintf(stderr, "tilebits for tile11  = %d, errors = %d\n\n", tilebits, errors) ;
  if(errors > 0) goto fail ;

  fprintf(stderr, "decoding mixed signs tile11b\n");
  for(i=0 ; i<NPT ; i++) tile11[i] = (i - 3) * 64  ;       // mixed signs, nbits <= 16
  for(i=NPT/4 ; i<3*NPT/4 ; i++) tile11[i] = 0 ;           // make short/long encoding usable
//   tilebits = decode_tile(ps, rest11, NPT) ;
  tilebits = decode_block(ps, rest11, NPTI, NPTI, NPTJ, NPTI);
  errors = 0 ;
  for(i=0 ; i<NPT ; i++) if(rest11[i] != tile11[i]){
    errors++ ;
    if(errors < 4) fprintf(stderr, "[%2d] : expected %d, got %d\n", i, tile11[i], rest11[i]) ;
  }
  fprintf(stderr, "tilebits for tile11  = %d, errors = %d\n\n", tilebits, errors) ;
  if(errors > 0) goto fail ;

  fprintf(stderr, "decoding mixed signs tile11c\n");
  for(i=0 ; i<NPT ; i++) tile11[i] <<= 5  ;                // mixed signs, nbits > 16
  for(i=NPT/4 ; i<3*NPT/4 ; i++) tile11[i] = 0 ;           // make short/long encoding usable
//   tilebits = decode_tile(ps, rest11, NPT) ;
  tilebits = decode_block(ps, rest11, NPTI, NPTI, NPTJ, NPTI);
  errors = 0 ;
  for(i=0 ; i<NPT ; i++) if(rest11[i] != tile11[i]){
    errors++ ;
    if(errors < 4) fprintf(stderr, "[%2d] : expected %d, got %d\n", i, tile11[i], rest11[i]) ;
  }
  fprintf(stderr, "tilebits for tile11  = %d, errors = %d\n\n", tilebits, errors) ;
  if(errors > 0) goto fail ;

  fprintf(stderr, "decoding positive with offset tile11d\n");
  for(i=0 ; i<NPT ; i++) tile11[i] = (i + 3) * 64 ;        // all > 0, nbits <= 16
  for(i=NPT/4 ; i<3*NPT/4 ; i++) tile11[i] = 0 ;           // make short/long encoding usable
  for(i=0 ; i<NPT ; i++) tile11[i] += 60000 ;              // force offset
//   tilebits = decode_tile(ps, rest11, NPT) ;
  tilebits = decode_block(ps, rest11, NPTI, NPTI, NPTJ, NPTI);
  errors = 0 ;
  for(i=0 ; i<NPT ; i++) if(rest11[i] != tile11[i]){
    errors++ ;
    if(errors < 4) fprintf(stderr, "[%2d] : expected %d, got %d\n", i, tile11[i], rest11[i]) ;
  }
  fprintf(stderr, "tilebits for tile11  = %d, errors = %d\n\n", tilebits, errors) ;
  if(errors > 0) goto fail ;

  fprintf(stderr, "decoding negative with offset tile11e\n");
  for(i=0 ; i<NPT ; i++) tile11[i] = -(i + 3) * 64 ;       // all <= 0, nbits <= 16
  for(i=NPT/4 ; i<3*NPT/4 ; i++) tile11[i] = 0 ;           // make short/long encoding usable
  for(i=0 ; i<NPT ; i++) tile11[i] -= 10000 ;              // force offset
//   tilebits = decode_tile(ps, rest11, NPT) ;
  tilebits = decode_block(ps, rest11, NPTI, NPTI, NPTJ, NPTI);
  errors = 0 ;
  for(i=0 ; i<NPT ; i++) if(rest11[i] != tile11[i]){
    errors++ ;
    if(errors < 4) fprintf(stderr, "[%2d] : expected %d, got %d\n", i, tile11[i], rest11[i]) ;
  }
  fprintf(stderr, "tilebits for tile11  = %d, errors = %d\n\n", tilebits, errors) ;
  if(errors > 0) goto fail ;
  fprintf(stderr, "SUCCESS\n") ;

bypass2:
  fprintf(stderr, "\n============================== block/tiles encoding/decoding test ==============================\n\n") ;

#define BNI   1023
#define BNJ   1025
#define SHIFT 12
#define BNPT (BNI*BNJ)
#define BSZ 8

  bitstream *ps2 ;
  int32_t block_in[BNJ][BNI], block_out[BNJ][BNI] ;

  // build block_in to have blocks >0, <0, mixed signs, and some suitable for short/long encoding
  for(j=BNJ-1 ; j>=0 ; j--){
    for(i=0 ; i<BNI ; i++){
      block_in[j][i] = (i << SHIFT) + j ;
//       if((i+j) > (BNI+BNJ)/2) block_in[j][i] = -block_in[j][i] ;
      if((j&1) && (i&1)) block_in[j][i] = block_in[j][i] >> (2+SHIFT/2) ;
    }
  }
  move_block((void *)block_in, (void *)block_out, BNI, BNI, BNJ, 8);
  errors = 0 ;
  for(j=BNJ-1 ; j>=0 ; j--){
    for(i=0 ; i<BNI ; i++){
      if(block_out[j][i] != block_in[j][i]) errors++ ;
    }
  }
  fprintf(stderr, "move_block : %d errors\n", errors); 

  // create bit stream
  STREAM_CREATE(ps2, NULL, sizeof(uint32_t)*BNPT*8, BIT_FULL_INIT) ;
  if(ps2->endian != PACK_ENDIAN){ status = 105 ; goto fail ; }
  if(StreamAvailableBits(ps2) != 0){ status = 106 ; goto fail ; }
  if(StreamAvailableSpace(ps2) != 8*sizeof(uint32_t)*BNPT*8){ status = 107 ; goto fail ; }
  STREAM_INSERT_BEGIN(*ps2) ;
  uint64_t t0, t1, t2, t3, t4 ;
  // prime the cache
  totalbits2 = encode_block(NULL, (void *)block_in, BNI, BNI, BNJ, BSZ|1, ENCODE_DRY_RUN) ;
  totalbits2 = encode_block(NULL, (void *)block_in, BNI, BNI, BNJ, BSZ|1, ENCODE_DRY_RUN) ;
  totalbits2 = encode_block(NULL, (void *)block_in, BNI, BNI, BNJ, BSZ|1, ENCODE_DRY_RUN) ;
  // timing test
  t0 = elapsed_cycles() ;
  totalbits2 = encode_block(NULL, (void *)block_in, BNI, BNI, BNJ, BSZ|1, ENCODE_DRY_RUN) ;
  t1 = elapsed_cycles() ;
  totalbits  = encode_block(ps2, (void *)block_in, BNI, BNI, BNJ, BSZ|1, 0) ;
  t2 = elapsed_cycles() ;
  fprintf(stderr, "encode_block : totalbits = %d (%d)", totalbits, totalbits2) ;
  fprintf(stderr, ", encoding time per value (ns)  : dry run = %5.1f, full run = %5.1f\n", (t1-t0)*nano/(BNI*BNJ), (t2-t1)*nano/(BNI*BNJ)) ;
  print_encode_stats(0) ; fprintf(stderr, "\n");
  STREAM_INSERT_FINALIZE(*ps2) ;
  STREAM_XTRACT_BEGIN(*ps2) ;

  for(j=BNJ-1 ; j>=0 ; j--){
    for(i=0 ; i<BNI ; i++) block_out[j][i] = -1 ;
  }
  t3 = elapsed_cycles() ;
  totalbits = decode_block(ps2, (void *)block_out, BNI, BNI, BNJ, BSZ|1) ;
  t4 = elapsed_cycles() ;
  fprintf(stderr, "decode_block : totalbits = %d", totalbits) ;
  fprintf(stderr, ", decoding time per value (ns) = %5.1f\n", (t4-t3)*nano/(BNI*BNJ)) ;
  errors = 0 ;
  for(j=BNJ-1 ; j>=0 ; j--){
    for(i=0 ; i<BNI ; i++){
//       fprintf(stderr, "%c", (block_out[j][i] != block_in[j][i]) ? '#' : '-') ;
      if(block_out[j][i] != block_in[j][i]) errors++ ;
    }
//     fprintf(stderr, "\n");
  }
  fprintf(stderr, "decode_block : %d errors\n", errors);

#if defined(USE_AEC_COMPRESSION)
  int32_t aes_out[BNI*BNJ] ;
  int32_t AecEncodeUnsigned(void *source, int32_t source_length, void *dest, int32_t dest_length, int bits_per_sample) ;
  for(i=0 ; i<BNI*BNJ ; i++) aes_out[i] = 0xCAFEFADE ;
  t0 = elapsed_cycles() ;
  totalbits = 8 * AecEncodeUnsigned(block_in, BNI*BNJ, aes_out, BNI*BNJ*4, 32) ;
  t1 = elapsed_cycles() ;
  fprintf(stderr, "AecEncodeUnsigned : totalbits = %d, time/value = %5.1f ns\n", totalbits, (t1-t0)*nano/(BNI*BNJ)) ;
//   for(i=BNI*BNJ-1 ; i>0 ; i--) if(aes_out[i] != 0xCAFEFADE) break ;
//   fprintf(stderr, "AecEncodeUnsigned : i*32 = %d\n", i*32) ;

  int32_t AecDecodeUnsigned(void *source, int32_t source_length, void *dest, int32_t dest_length, int bits_per_sample);
  for(j=BNJ-1 ; j>=0 ; j--){
    for(i=0 ; i<BNI ; i++) block_out[j][i] = -1 ;
  }
  t3 = elapsed_cycles() ;
  totalbits = 8 * AecDecodeUnsigned(aes_out, totalbits, block_out, BNI*BNJ, 32) ;
  t4 = elapsed_cycles() ;
  errors = 0 ;
  for(j=BNJ-1 ; j>=0 ; j--){
    for(i=0 ; i<BNI ; i++){
      if(block_out[j][i] != block_in[j][i]) errors++ ;
    }
  }
  fprintf(stderr, "AecDecodeUnsigned : totalbits = %d, time/value = %5.1f ns, errors = %d\n", totalbits, (t4-t3)*nano/(BNI*BNJ), errors) ;
#endif

end:
  return 0 ;

fail:
  fprintf(stderr, "FAILED(%d)\n", status) ;
  return 1 ;
}
