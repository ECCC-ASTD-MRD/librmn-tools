#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>

#include <rmn/timers.h>
#include <rmn/tile_encoders.h>
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

int main(int argc, char **argv){
  uint64_t freq ;
  double nano ;
  int32_t tile0[64] ;
  int32_t tile1[64] ;
  int32_t tile2[64] ;
  uint32_t packed[65] ;
  int i, j, ij, ni, nj ;
  bitstream stream ;
  int32_t nbtot ;

  freq = cycles_counter_freq() ;
  nano = 1000000000 ;
  nano /= freq ;

  start_of_test("tile1 encoder test C");

  for(i=0 ; i<64 ; i++){
    packed[i] = 0xFFFFFFFFu ;
    tile2[i]  = 0x0F0F0F0F ;
  }
  for(j=7 ; j>=0 ; j--){
    for(i=0 ; i<8 ; i++){
      ij = INDEX(i, 8, j) ;
      tile1[ij] = (i << 4) + j ;
      tile1[ij] |= ((j & 1) << 11) ;
    }
  }
  for(i=0 ; i<64 ; i++) tile1[i] = -tile1[i] ;
  print_tile(tile1, 8, 8, 8, "original tile") ;

  BeStreamInit(&stream, packed, sizeof(packed), 0) ;  // force read-write stream mode
  print_stream_params(stream, "Init Stream", "ReadWrite") ;
  print_stream_data(stream, "stream contents") ;

  nbtot = encode_ints(tile1, 8, 8, &stream) ;

  TEE_FPRINTF(stderr,2, "nbtot = %d\n", nbtot) ;
  print_stream_params(stream, "encoded Stream", "ReadWrite") ;
  print_stream_data(stream, "stream contents") ;

  nbtot = decode_ints(tile2, &ni, &nj, &stream) ;
  TEE_FPRINTF(stderr,2, "ni = %d, nj = %d\n", ni, nj) ;
  print_tile(tile2, ni, ni, nj, "restored tile") ;

  return 0 ;
}
