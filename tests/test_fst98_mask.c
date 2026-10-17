#include <stdio.h>
#include <stdint.h>

#include <rmn/fst98_mask.h>
#include <rmn/fst98_mask.h>

int main(int argc, char **argv){
  (void) (argc) ;
  (void) (argv) ;
  fst_rle_code r ;
  uint32_t count = 0 ;
  fprintf(stderr,"fst98 mask test\n");
  for(uint32_t byte = 0 ; byte < 256 ; byte += 127){
    if(byte > 128) byte = 0xFF ;
    count = 290 + 274 ;
    while(count > 0){
      r = fst_rle_token(byte, count) ;
      fprintf(stderr,"count %4d, byte %2.2x, code %4.4x, nbits %2d, left %4d\n", count, r.byte, r.code, r.bits, r.left) ;
      count = r.left ;
      if((byte != 0) && (byte != 0xFF)) break ;
    }
  }
  fprintf(stderr,"\n");
  for(uint32_t byte = 0 ; byte < 256 ; byte += 127){
    if(byte > 128) byte = 0xFF ;
    r = fst_rle_token(byte, count = 270) ;
    fprintf(stderr,"count %4d, byte %2.2x, code %4.4x, nbits %2d, left %4d\n", count, r.byte, r.code, r.bits, r.left) ;
  }
  fprintf(stderr,"\n");
  for(uint32_t byte = 0 ; byte < 256 ; byte += 127){
    if(byte > 128) byte = 0xFF ;
    r = fst_rle_token(byte, count = 17) ;
    fprintf(stderr,"count %4d, byte %2.2x, code %4.4x, nbits %2d, left %4d\n", count, r.byte, r.code, r.bits, r.left) ;
  }
  fprintf(stderr,"\n");
  for(uint32_t byte = 0 ; byte < 256 ; byte += 127){
    if(byte > 128) byte = 0xFF ;
    r = fst_rle_token(byte, count = 2) ;
    fprintf(stderr,"count %4d, byte %2.2x, code %4.4x, nbits %2d, left %4d\n", count, r.byte, r.code, r.bits, r.left) ;
  }
  fprintf(stderr,"\n");
  for(uint32_t byte = 0 ; byte < 256 ; byte += 127){
    if(byte > 128) byte = 0xFF ;
    r = fst_rle_token(byte, count = 1) ;
    fprintf(stderr,"count %4d, byte %2.2x, code %4.4x, nbits %2d, left %4d\n", count, r.byte, r.code, r.bits, r.left) ;
  }
  bitstream stream = NULL_BITSTREAM ;
  InitStream(&stream, NULL, 10000l, BIT_FULL_INIT|BIT_INSERT|BIT_XTRACT|SET_BIG_ENDIAN) ;
  fprintf(stderr, "encode_decode_mask : size of stream = %ld words\n", STREAM_BITS_EMPTY(stream)/32) ;
  for(uint32_t byte = 0 ; byte < 256 ; byte += 127){
    if(byte > 127) byte = 0xFF ;
    count = 290 + 274 ;
    int32_t totbits = fst_rle_encode(byte, count, &stream) ;
    fprintf(stderr, "count %4d, byte %2.2x, totbits %4d\n", count, byte, totbits) ;
  }
  ssize_t navail = StreamAvailableBits(&stream) ;
  fprintf(stderr, "encode_decode_mask : bits available in stream = %ld\n", navail) ;
}
