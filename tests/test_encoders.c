#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>

#include <rmn/timers.h>

// deliberate double inclusion
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
#include <rmn/tile_encoders.h>
#include <rmn/tile_encoders.h>
#include <rmn/ct_assert.h>
#include <rmn/ct_assert.h>

#include <rmn/tee_print.h>
#include <rmn/test_helpers.h>

#include <rmn/print_bitstream.h>

#define INDEX(col, lrow, row) ( (col) + (row)*(lrow) )

static int compare_tile(int32_t *ref, int32_t *tile, int ni, int lni, int nj){
  int i, j, ij, errors ;
  errors = 0 ;
  for(j=nj-1 ; j>=0 ; j--){
    for(i=0 ; i<ni ; i++){
      ij = INDEX(i, lni, j) ;
      if(ref[ij] != tile[ij]) errors++ ;
//       if(errors <= 64 && ref[ij] != tile[ij]){
//         TEE_FPRINTF(stderr,2, "i = %3d, j = %3d, expected %8.8x, got %8.8x\n", i, j, ref[ij], tile[ij]) ;
//       }
    }
  }
  TEE_FPRINTF(stderr,2, "errors = %d (%d items checked)\n", errors, ni*nj) ;
  return errors ;
}

static void print_tile(int32_t *tile, int ni, int lni, int nj, char *msg){
  int i, j, ij ;
  TEE_FPRINTF(stderr,2, "%s  : \n", msg) ;
  for(j=nj-1 ; j>=0 ; j--){
    TEE_FPRINTF(stderr,2, "%d :", j) ;
    for(i=0 ; i<ni ; i++){
      ij = INDEX(i, lni, j) ;
      TEE_FPRINTF(stderr,2, " (%2d)%8.8x", ij,tile[ij]) ;
    }
    TEE_FPRINTF(stderr,2, "\n") ;
  }
}

// GLOBAL scope compile time assertions
CT_ASSERT(8 == sizeof(tile_properties))
CT_ASSERT(8 == sizeof(tile_parms))
CT_ASSERT(2 == sizeof(tile_header))

#define NPTSI 16
#define NPTSLNI 17
#define NPTSJ 16

int main(int argc, char **argv){
  uint64_t freq ;
  double nano ;
  int32_t tile0[64], tile1[64], tile2[64], tile3[64], tile4[64], tile5[64] ;
  int32_t chunk_i[NPTSJ*NPTSLNI+64] ;
  int32_t chunk_o[NPTSJ*NPTSLNI+64] ;
  uint32_t packed0[NPTSJ*NPTSLNI+64] ;
  uint32_t packed1[NPTSJ*NPTSLNI+64] ;
  int i, j, ij, ni, nj, nij, errors ;
  bitstream stream0, stream1  ;
  int32_t nbtot ;
  size_t ncopy ;
  uint32_t temp[64] ;
  tile_properties tp ;
  uint64_t tp64 ;
  TIME_LOOP_DATA ;

  // LOCAL scope compile time assertions
CT_ASSERT(8 == sizeof(nano))
CT_ASSERT(8 == sizeof(freq))
CT_ASSERT(2 == sizeof(uint16_t))


  freq = cycles_counter_freq() ;
  nano = 1000000000 ;
  nano /= freq ;

  start_of_test("C tile encoder test");
  if((8 != sizeof(tile_properties)) || (8 != sizeof(tile_parms)) || (2 != sizeof(tile_header))) {
    TEE_FPRINTF(stderr,2, "ERROR, bd size for some tile properties structure\n") ;
    TEE_FPRINTF(stderr,2, "sizeof(tile_header) = %ld, expecting 2\n", sizeof(tile_header)) ;
    TEE_FPRINTF(stderr,2, "sizeof(tile_parms) = %ld, expecting 8\n", sizeof(tile_parms)) ;
    TEE_FPRINTF(stderr,2, "sizeof(tile_properties) = %ld, expecting 8\n", sizeof(tile_properties)) ;
    goto error ;
  }

  for(i=0 ; i<64 ; i++){
    packed0[i] = 0xFFFFFFFFu ;
    tile0[i]  = 0x0F0F0F0F ;
  }
  for(j=7 ; j>=0 ; j--){
    for(i=0 ; i<8 ; i++){
      ij = INDEX(i, 8, j) ;
      tile1[ij] = (i << 4) + j ;
      tile1[ij] |= ((j & 1) << 11) ;     // 0 or positive
      tile2[ij] = -( tile1[ij] + 1 ) ;   // all negative
      tile3[ij] = -tile1[ij] ;           // mixed sign
      tile4[ij] = tile1[ij] + (1<<12) ;  // positive with large offset
      tile5[ij] = tile4[ij] ;
    }
  }
  tile5[0] = 0x800 ;

#if 0
  print_tile(tile1, 8, 8, 8, "original tile1") ;
  tp64 = encode_tile_properties(tile1, 8, 8, 8, temp) ;
  nbtot = encode_tile_(temp, &stream0, tp64) ;
  tp64 = encode_tile_properties_(tile1, 8, 8, 8, temp) ;
  nbtot = encode_tile_(temp, &stream0, tp64) ;

  print_tile(tile2, 8, 8, 8, "original tile2") ;
  tp64 = encode_tile_properties(tile2, 8, 8, 8, temp) ;
  nbtot = encode_tile_(temp, &stream0, tp64) ;
  tp64 = encode_tile_properties_(tile2, 8, 8, 8, temp) ;
  nbtot = encode_tile_(temp, &stream0, tp64) ;

  print_tile(tile3, 8, 8, 8, "original tile3") ;
  tp64 = encode_tile_properties(tile3, 8, 8, 8, temp) ;
  nbtot = encode_tile_(temp, &stream0, tp64) ;
  tp64 = encode_tile_properties_(tile3, 8, 8, 8, temp) ;
  nbtot = encode_tile_(temp, &stream0, tp64) ;

  print_tile(tile4, 8, 8, 8, "original tile4") ;
  tp64 = encode_tile_properties(tile4, 8, 8, 8, temp) ;
  nbtot = encode_tile_(temp, &stream0, tp64) ;
  tp64 = encode_tile_properties_(tile4, 8, 8, 8, temp) ;
  nbtot = encode_tile_(temp, &stream0, tp64) ;

  print_tile(tile5, 8, 8, 8, "original tile5") ;
  tp64 = encode_tile_properties(tile5, 8, 8, 8, temp) ;
  nbtot = encode_tile_(temp, &stream0, tp64) ;
  tp64 = encode_tile_properties_(tile5, 8, 8, 8, temp) ;
  nbtot = encode_tile_(temp, &stream0, tp64) ;

  TIME_LOOP_EZ(1000, 640, tp64 = encode_tile_properties(tile5, 8, 8, 8, temp)) ;
  fprintf(stderr, "encode_tile_properties     : %s\n\n",timer_msg);

  TIME_LOOP_EZ(1000, 64, tp64 = encode_tile_properties_(tile5, 8, 8, 8, temp)) ;
  fprintf(stderr, "encode_tile_properties_    : %s\n\n",timer_msg);
#endif

  TEE_FPRINTF(stderr,2,"========== single tile encode / decode ==========\n");
//   tile3[0] = -2 ;
  print_tile(tile3, 8, 8, 8, "original tile (mixed signs)") ;
  TEE_FPRINTF(stderr,2,"\n");

  BeStreamInit(&stream0, packed0, sizeof(packed0), 0) ;  // force read-write stream0 mode
  print_stream_params(stream0, "Init stream0", "RW") ;
  print_stream_data(stream0, "stream0") ;

  nbtot = encode_tile(tile3, 8, 8, 8, &stream0, temp) ;  // 8 x 8 compact full tile
  TEE_FPRINTF(stderr,2, "bits after encoding = %d\n", nbtot) ;
  print_stream_params(stream0, "encoded stream0", "RW") ;
  print_stream_data(stream0, "stream0") ;

  for(i=0 ; i<64 ; i++) tile0[i] = 0x55555555 ;
  ni = nj = 8 ;
  nbtot = decode_tile(tile0, ni, 8, nj, &nij, &stream0) ;         // decode tile from stream0 into tile0
  TEE_FPRINTF(stderr,2, "ni = %d, nj = %d, nij = %d, ", ni, nj, nij) ;
  errors = compare_tile(tile0, tile3, ni, ni, nj) ;
  if(errors > 0) {
    print_tile(tile0, ni, ni, nj, "restored tile") ;
    goto error ;
  }else{
    TEE_FPRINTF(stderr,2, "stream0 -> tile0 : SUCCESS\n\n");
  }
// return 0 ;
  // no need to rewind stream0 in this case
  for(i=0 ; i< sizeof(packed1)/4 ; i++) packed1[i] = 0 ;
  ncopy=StreamDataCopy(&stream0, packed1, sizeof(packed1)) ;  // copy stream0 data into packed1
  TEE_FPRINTF(stderr,2,"copied %ld bits from stream0\n", ncopy);
  BeStreamInit(&stream1, packed1, sizeof(packed1), 0) ;       // initialize stream1 (RW) using packed1
  StreamSetFilledBits(&stream1, nbtot) ;                      // set available number of bits to nb of useful bits
  print_stream_params(stream1, "Init stream1", "RW") ;
  print_stream_data(stream1, "stream1") ;
  nbtot = decode_tile(tile0, ni, 8, nj, &nij, &stream1) ;         // decode tile from stream1 into tile0
  TEE_FPRINTF(stderr,2, "ni = %d, nj = %d, nij = %d, ", ni, nj, nij) ;
  errors = compare_tile(tile0, tile3, ni, ni, nj) ;
  if(errors > 0) {
    print_tile(tile0, ni, ni, nj, "restored tile") ;
    goto error ;
  }else{
    TEE_FPRINTF(stderr,2, "stream0 -> stream1 -> tile0 : SUCCESS\n\n");
  }
  TEE_FPRINTF(stderr,2,"\n");
// return 0 ;
  TEE_FPRINTF(stderr,2,"========== multi tile encode / decode ==========\n");
  for(j=0 ; j<NPTSJ ; j++){
    for(i=0 ; i<NPTSLNI ; i++){
      ij = INDEX(i, NPTSLNI, j) ;
      chunk_i[ij] = (i << 8) + j ;   // 16 bits max
      chunk_o[ij] = -1 ;
      if(i<8 && j<8) chunk_i[ij] = 0 ;             // lower left tile
      if(i>7 && j>7) chunk_i[ij] = 0x00001234 ;    // upper right tile
      if(i>7 && j>7) chunk_i[ij] = -chunk_i[ij] ;  // upper right tile
//       if(i>7 && j>7) chunk_i[ij] = 0xFFFF6543 ;    // upper right tile
      if(i>7 && i<10 && j<8) chunk_i[ij] = -7 ;    // first 2 columns of lower right tile
      if(i<2 && j>7) chunk_i[ij] = 0 ;             // first 2 columns of upper left tile
    }
  }
  TEE_FPRINTF(stderr,2,"Original data\n") ;
  for(j=NPTSJ-1 ; j>=0 ; j--){
    for(i=0 ; i<NPTSLNI ; i++){
      ij = INDEX(i, NPTSLNI, j) ;
      if(i==8 || i==16)TEE_FPRINTF(stderr,2,"  ");
      TEE_FPRINTF(stderr,2," %8.8x", chunk_i[ij]) ;
    }
    TEE_FPRINTF(stderr,2,"\n");
    if(j == 8) TEE_FPRINTF(stderr,2,"\n");
  }
  TEE_FPRINTF(stderr,2,"\n");

  BeStreamInit(&stream0, packed0, sizeof(packed0), 0) ;  // force read-write stream0 mode
  print_stream_params(stream0, "Init stream0", "RW") ;
  print_stream_data(stream0, "stream0") ;
  TEE_FPRINTF(stderr,2, "\n");

  TEE_FPRINTF(stderr,2,"========== encoding tiles ==========\n");
  nbtot = encode_as_tiles(chunk_i, NPTSI, NPTSLNI, NPTSJ, &stream0) ;
  TEE_FPRINTF(stderr,2, "needed %d bits (%4.1f/value)\n", nbtot, nbtot*1.0/(NPTSI*NPTSJ)) ;
  print_stream_params(stream0, "encoded_tiles stream0", "RW") ;
  TEE_FPRINTF(stderr,2, "\n") ;
// return 0 ;
  TEE_FPRINTF(stderr,2,"========== decoding from original stream ==========\n");

  nbtot = decode_as_tiles(chunk_o, NPTSI, NPTSLNI, NPTSJ, &stream0);

//   for(j=NPTSJ-1 ; j>=0 ; j--){
//     for(i=0 ; i<NPTSLNI ; i++){
//       ij = INDEX(i, NPTSLNI, j) ;
//       if(i==8 || i==16)TEE_FPRINTF(stderr,2,"  ");
//       TEE_FPRINTF(stderr,2," %8.8x", chunk_o[ij]^chunk_i[ij]) ;
//     }
//     TEE_FPRINTF(stderr,2,"\n");
//     if(j == 8) TEE_FPRINTF(stderr,2,"\n");
//   }
//   TEE_FPRINTF(stderr,2,"\n");
  compare_tile(chunk_i, chunk_o, NPTSI, NPTSLNI, NPTSJ) ;
// return 0 ;
  TEE_FPRINTF(stderr,2,"========== decoding from copy ==========\n");
  for(i=0 ; i< sizeof(packed1)/4 ; i++) packed1[i] = 0 ;
  print_stream_params(stream0, "Source stream0", "RW") ;
  ncopy = StreamDataCopy(&stream0, packed1, sizeof(packed1)) ;         // transfer stream0 buffer to packed1
  TEE_FPRINTF(stderr,2, "%ld(%ld) bits copied into stream1 buffer\n", ncopy, ((ncopy+31)/32)*32) ;
  BeStreamInit(&stream1, packed1, sizeof(packed1), 0) ;            // force read-write stream1 mode using packed1
  StreamSetFilledBits(&stream1, nbtot) ;
  print_stream_params(stream1, "Filled stream1", "RW") ;
  TEE_FPRINTF(stderr,2, "\n") ;
  nbtot = decode_as_tiles(chunk_o, NPTSI, NPTSLNI, NPTSJ, &stream1);

//   for(j=NPTSJ-1 ; j>=0 ; j--){
//     for(i=0 ; i<NPTSLNI ; i++){
//       ij = INDEX(i, NPTSLNI, j) ;
//       if(i==8 || i==16)TEE_FPRINTF(stderr,2,"  ");
//       TEE_FPRINTF(stderr,2," %8.8x", chunk_o[ij]) ;
//     }
//     TEE_FPRINTF(stderr,2,"\n");
//     if(j == 8) TEE_FPRINTF(stderr,2,"\n");
//   }
//   TEE_FPRINTF(stderr,2,"\n");

  errors = compare_tile(chunk_i, chunk_o, NPTSI, NPTSLNI, NPTSJ) ;
  if(errors == 0){
    TEE_FPRINTF(stderr,2, "compare chunk_i, chunk_o : SUCCESS\n") ;
  }

  TEE_FPRINTF(stderr,2,"========== tile properties/population test ==========\n");
  int32_t pop[4] ;
  int32_t ref[4] = { 2, 4, 8, 16 } ;

  tp.u64 = encode_tile_properties(tile1, 8, 8, 8, temp) ; // gather tile
  for(j=7 ; j>=0 ; j--){
    for(i=0 ; i<8 ; i++){
      ij = i + 8 * j ;
      TEE_FPRINTF(stderr,2," %8.8x", temp[ij]);
    }
    TEE_FPRINTF(stderr,2,"\n");
  }
  fprintf(stderr, "bshort = %d, nshort = %d, bits = %d, encd = %d\n", tp.t.bshort, tp.t.nshort, tp.t.h.nbts, tp.t.h.encd);
  ref[0] = 1 ; ref[2] = 1 << 6 ; ref[2] = 1 << 5 ; ref[3] = 1 << 7 ;
  fprintf(stderr, "ref        = %4d %4d %4d %4d\n", ref[0], ref[1], ref[2], ref[3]);
  tile_population(temp, 64, pop, ref) ;
  fprintf(stderr, "pop (<ref) = %4d %4d %4d %4d\n", pop[0], pop[1], pop[2], pop[3]);

  TEE_FPRINTF(stderr,2,"========== population timing test ==========\n");
//   TIME_LOOP_EZ(1000, 64, tp.u64 = encode_tile_properties(tile1, 8, 8, 8, temp)) ;
//   fprintf(stderr, "encode_tile_properties : %s, zero = %d, short = %d, bits = %d\n\n", timer_msg, tp.t.nzero, tp.t.nshort, tp.t.h.nbts);
//

  TIME_LOOP_EZ(1000, 64, tile_population(temp, 64, pop, ref)) ;
  fprintf(stderr, "tile_population_64     : %s, pop = %d %d %d %d\n", timer_msg, pop[0], pop[1], pop[2], pop[3]);

  TIME_LOOP_EZ(1000, 63, tile_population(temp, 63, pop, ref)) ;
  fprintf(stderr, "tile_population_63     : %s, pop = %d %d %d %d\n", timer_msg, pop[0], pop[1], pop[2], pop[3]);

  TIME_LOOP_EZ(1000, 56, tile_population(temp, 56, pop, ref)) ;
  fprintf(stderr, "tile_population_56     : %s, pop = %d %d %d %d\n", timer_msg, pop[0], pop[1], pop[2], pop[3]);

end:
  return 0 ;

error:
  return 1 ;
}
