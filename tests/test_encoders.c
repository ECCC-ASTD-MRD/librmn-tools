#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>

#include <rmn/timers.h>

// deliberate double inclusion
#include <rmn/tile_encoders.h>
#include <rmn/tile_encoders.h>
#include <rmn/ct_assert.h>
#include <rmn/ct_assert.h>

#include <rmn/tee_print.h>
#include <rmn/test_helpers.h>

#define INDEX(col, lrow, row) ( (col) + (row)*(lrow) )

static void print_stream_data(bitstream s, char *msg){
  uint32_t *in = s.in ;
  uint32_t *first = s.first ;
  uint32_t *cur = first ;

  TEE_FPRINTF(stderr,2, "%s\n", msg) ;
  while(cur < in){
    TEE_FPRINTF(stderr,2, " %8.8x", *cur) ;
    cur++ ;
  }
  TEE_FPRINTF(stderr,2, ", accum = %16.16lx", s.acc_i << (64 - s.insert)) ;
  TEE_FPRINTF(stderr,2, ", guard = %8.8x\n", *cur) ;
}

static void print_stream_params(bitstream s, char *msg, char *expected_mode){
  TEE_FPRINTF(stderr,2, "%s: filled = %d(%d), free= %d, first/in/out = %p/%p/%p [%ld], insert/xtract = %d/%d \n",
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

static void compare_tile(int32_t *ref, int32_t *tile, int ni, int lni, int nj){
  int i, j, ij, errors ;
  errors = 0 ;
  for(j=nj-1 ; j>=0 ; j--){
    for(i=0 ; i<ni ; i++){
      ij = INDEX(i, lni, j) ;
      if(ref[ij] != tile[ij]) errors++ ;
    }
  }
  TEE_FPRINTF(stderr,2, "errors = %d (%d values)\n", errors, ni*nj) ;
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

int main(int argc, char **argv){
  uint64_t freq ;
  double nano ;
  int32_t tile0[64], tile1[64], tile2[64], tile3[64], tile4[64], tile5[64] ;
  uint32_t packed[65] ;
  int i, j, ij, ni, nj ;
  bitstream stream ;
  int32_t nbtot ;
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

  start_of_test("tile1 encoder test C");
  TEE_FPRINTF(stderr,2, "sizeof(tile_header) = %ld, expecting 2\n", sizeof(tile_header)) ;
  TEE_FPRINTF(stderr,2, "sizeof(tile_parms) = %ld, expecting 8\n", sizeof(tile_parms)) ;
  TEE_FPRINTF(stderr,2, "sizeof(tile_properties) = %ld, expecting 8\n", sizeof(tile_properties)) ;
  if((8 != sizeof(tile_properties)) || (8 != sizeof(tile_parms)) || (2 != sizeof(tile_header))) {
    TEE_FPRINTF(stderr,2, "ERROR, bd size for some tile properties structure\n") ;
    goto error ;
  }

  for(i=0 ; i<64 ; i++){
    packed[i] = 0xFFFFFFFFu ;
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
  nbtot = encode_tile_(temp, &stream, tp64) ;
  tp64 = encode_tile_properties_(tile1, 8, 8, 8, temp) ;
  nbtot = encode_tile_(temp, &stream, tp64) ;

  print_tile(tile2, 8, 8, 8, "original tile2") ;
  tp64 = encode_tile_properties(tile2, 8, 8, 8, temp) ;
  nbtot = encode_tile_(temp, &stream, tp64) ;
  tp64 = encode_tile_properties_(tile2, 8, 8, 8, temp) ;
  nbtot = encode_tile_(temp, &stream, tp64) ;

  print_tile(tile3, 8, 8, 8, "original tile3") ;
  tp64 = encode_tile_properties(tile3, 8, 8, 8, temp) ;
  nbtot = encode_tile_(temp, &stream, tp64) ;
  tp64 = encode_tile_properties_(tile3, 8, 8, 8, temp) ;
  nbtot = encode_tile_(temp, &stream, tp64) ;

  print_tile(tile4, 8, 8, 8, "original tile4") ;
  tp64 = encode_tile_properties(tile4, 8, 8, 8, temp) ;
  nbtot = encode_tile_(temp, &stream, tp64) ;
  tp64 = encode_tile_properties_(tile4, 8, 8, 8, temp) ;
  nbtot = encode_tile_(temp, &stream, tp64) ;

  print_tile(tile5, 8, 8, 8, "original tile5") ;
  tp64 = encode_tile_properties(tile5, 8, 8, 8, temp) ;
  nbtot = encode_tile_(temp, &stream, tp64) ;
  tp64 = encode_tile_properties_(tile5, 8, 8, 8, temp) ;
  nbtot = encode_tile_(temp, &stream, tp64) ;

  TIME_LOOP_EZ(1000, 640, tp64 = encode_tile_properties_10(tile5, 8, 8, 8, temp)) ;
  fprintf(stderr, "encode_tile_properties     : %s\n\n",timer_msg);

  TIME_LOOP_EZ(1000, 64, tp64 = encode_tile_properties_(tile5, 8, 8, 8, temp)) ;
  fprintf(stderr, "encode_tile_properties_    : %s\n\n",timer_msg);
#endif

//   print_tile(tile1, 8, 8, 8, "original tile1") ;
  print_tile(tile3, 8, 8, 8, "original tile3") ;

  BeStreamInit(&stream, packed, sizeof(packed), 0) ;  // force read-write stream mode
  print_stream_params(stream, "Init Stream", "ReadWrite") ;
  print_stream_data(stream, "stream contents") ;

//   nbtot = encode_tile_(tile1, 8, 8, 7, &stream, temp) ;
  nbtot = encode_tile_(tile3, 8, 8, 8, &stream, temp) ;

  TEE_FPRINTF(stderr,2, "nbtot = %d\n", nbtot) ;
  print_stream_params(stream, "encoded Stream", "ReadWrite") ;
  print_stream_data(stream, "stream contents") ;

  nbtot = decode_tile(tile0, &ni, 8, &nj, &stream) ;
  TEE_FPRINTF(stderr,2, "ni = %d, nj = %d\n", ni, nj) ;
  print_tile(tile0, ni, ni, nj, "restored tile") ;
//   compare_tile(tile0, tile1, ni, ni, nj) ;
  compare_tile(tile0, tile3, ni, ni, nj) ;

end:
  return 0 ;

error:
  return 1 ;
}
