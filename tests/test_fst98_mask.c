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

  uint32_t totbits = 0 ;
  for(uint32_t byte = 0 ; byte < 256 ; byte += 127){
    if(byte > 128) byte = 0xFF ;
    uint32_t nbits = 0 ;
    for(count=1 ; count<4096 ; count++){
      r = fst_rle_token(byte, count) ;
      if(r.bits != nbits){
        nbits = r.bits ;
        fprintf(stderr,"count %4d, byte %2.2x, code %6.1x, nbits %2d, left %4d\n", count, r.byte, r.code, r.bits, r.left);
      }
    }
    fprintf(stderr,"count %4d, byte %2.2x, code %6.1x, nbits %2d, left %4d\n", count, r.byte, r.code, r.bits, r.left);
    fprintf(stderr,"\n") ;
  }

  ssize_t navail = 0, ssize = 0 ;
  bitstream stream = NULL_BITSTREAM ;
  InitStream(&stream, NULL, 1024l*1024l, BIT_FULL_INIT|BIT_INSERT|BIT_XTRACT|SET_BIG_ENDIAN) ;
  ssize = STREAM_BITS_EMPTY(stream) ;
  fprintf(stderr, "test_rle : size of stream = %ld bits", ssize) ;
  navail = StreamAvailableBits(&stream) ;
  fprintf(stderr, ", bits available in stream = %ld\n", navail) ;

  totbits = 0 ;
  for(uint32_t byte = 0 ; byte < 256 ; byte += 127){
    if(byte > 127) byte = 0xFF ;
    fprintf(stderr, "test_rle : byte = %2.2x\n", byte);
    for(count=0 ; count<=80*1024 ; count+=1024){
      if(count == 0) continue ;  // skip count == 0
      uint32_t kount = ((byte == 0x00) || (byte == 0xFF)) ? count : count/1024 ;
      totbits += put_rle_count(byte, kount, &stream) ;
      navail = StreamAvailableBits(&stream) ;
      if(navail != totbits){
        fprintf(stderr, "test_rle : bits available in stream = %ld, totbits = %d\n", navail, totbits) ;
        exit(1) ;
      }
    }
  }
  STREAM_REWIND(stream, 1) ;
  navail = StreamAvailableBits(&stream) ;
  fprintf(stderr, "test_rle : bits available = %ld, totbits = %d", navail, totbits) ;
  fprintf(stderr, ", space left = %ld", STREAM_BITS_EMPTY(stream)) ;
  fprintf(stderr, ", stream size = %ld\n", navail + STREAM_BITS_EMPTY(stream)) ;
  if(ssize != navail + STREAM_BITS_EMPTY(stream)) exit(2) ;

  STREAM_REWRITE(stream, 1) ;
  fprintf(stderr, "\n");
  navail = StreamAvailableBits(&stream) ;
  fprintf(stderr, "test_rle (after rewrite) : bits available = %ld", navail) ;
  fprintf(stderr, ", space left = %ld", STREAM_BITS_EMPTY(stream)) ;
  fprintf(stderr, ", stream size = %ld\n", navail + STREAM_BITS_EMPTY(stream)) ;

  fprintf(stderr, "\n");
  for(uint32_t byte = 0 ; byte < 256 ; byte += 127){
    if(byte > 127) byte = 0xFF ;
    fprintf(stderr, "write_rle : byte = %2.2x\n", byte);
    for(count=1 ; count<275 ; count++){
      uint32_t kount = ((byte == 0x00) || (byte == 0xFF)) ? count : 1 ;
      put_rle_count(byte, kount, &stream) ;
    }
  }
  CLOSE_RLE_STREAM(&stream) ;

  fprintf(stderr, "\n");
  STREAM_REWIND(stream, 1) ;
  for(uint32_t byte = 0 ; byte < 256 ; byte += 127){
    if(byte > 127) byte = 0xFF ;
    fprintf(stderr, "read_rle : byte = %2.2x\n", byte);
    for(count=1 ; count<275 ; count++){
      uint32_t kount = ((byte == 0x00) || (byte == 0xFF)) ? count : 1 ;
      r = get_rle_token(&stream) ;
      if(r.byte != byte || r.count != kount){
        fprintf(stderr, "r.code = %4.4x\n", r.code) ;
        fprintf(stderr, "count = %d, byte = %2.2x, got r.count = %d, r.byte = %2.2x\n", kount, byte, r.count, r.byte) ;
        exit(3) ;
      }
    }
  }
  r = get_rle_token(&stream) ;
  fprintf(stderr, "END TOKEN = %2x, count = %d\n", r.byte, r.count) ;
  if( ((r.byte != 0) && (r.byte != 0xFF)) || (r.count != 0) ){
    fprintf(stderr, "r.code = %4.4x\n", r.code) ;
    fprintf(stderr, "got r.count = %d, r.byte = %2.2x\n", r.count, r.byte) ;
    exit(4) ;
  }
  fprintf(stderr, "\nSUCCESS\n");
}
