#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <rmn/fst_mask.h>
#include <rmn/fst_mask.h>
#include <rmn/timers.h>

#define NI 32
#define NJ 16
#define NINJ (NI*NJ)

int main(int argc, char **argv){
  (void) (argc) ;
  (void) (argv) ;
  TIME_LOOP_DATA ;
  fst_rle_code r ;
  int32_t count = 0, lastcount ;
  uint32_t totbits = 0 ;
  ssize_t navail = 0, ssize = 0 ;
  bitstream stream = NULL_BITSTREAM ;

//   fprintf(stderr,"fst98 mask test\n");

  int test = 0, test_1 = 0, test_n = 0 ;
  if(argc > 1){ test_1 = test_n = atoi(argv[1]) ; }
  if(argc > 2){ test_n = atoi(argv[2]) ; }

  for(test = test_1 ; test <= test_n ; test++){
    switch (test) {
      case 0:{
        fprintf(stderr,"===== single repeat count =====\n");
        totbits = 0 ;
        fprintf(stderr,"max repeat count = %d\n", RLE_MAX_COUNT) ;
        for(uint32_t byte = 0 ; byte < 256 ; byte += 127){
          if(byte > 128) byte = 0xFF ;
          uint32_t nbits = 0 ;
          for(count=1 ; count<4096 ; count++){
            r = fst_rle_token(byte, count) ;
            if(r.bits != nbits){
              fprintf(stderr,"r.bits != nbits : count %4d, byte %2.2x, code %6.1x, nbits %2d, left %4d\n",
                      count, r.byte, r.code, r.bits, r.left);
              nbits = r.bits ;
            }
            if((count - r.left) > RLE_MAX_COUNT){
              fprintf(stderr,"ERROR : count = %d, residual %d is too small (<%d)\n",
                      count, r.left, count - RLE_MAX_COUNT) ;
              exit(6) ;
            }
            lastcount = count ;
          }
          count = lastcount ;     // last used count in loop
          fprintf(stderr,"                  count %4d, byte %2.2x, code %6.1x, nbits %2d, left %4d\n",
                  count, r.byte, r.code, r.bits, r.left);
          if((count - r.left) > RLE_MAX_COUNT){
            fprintf(stderr,"ERROR : count = %d, residual %d is too small (<%d)\n",
                    count, r.left, count - RLE_MAX_COUNT) ;
            exit(6) ;
          }
        }
        fprintf(stderr, "SUCCESS\n\n");
        break ;
      }
      case 1:{
        fprintf(stderr,"===== RLE stream : write  / read back =====\n");
        navail = 0, ssize = 0 ;
        stream = NULL_BITSTREAM ;
        InitStream(&stream, NULL, 1024l*1024l, BIT_FULL_INIT|BIT_INSERT|BIT_XTRACT|SET_BIG_ENDIAN) ;
        ssize = STREAM_BITS_EMPTY(stream) ;
        fprintf(stderr, "test_rle (after create) : size of stream = %ld bits", ssize) ;
        navail = StreamAvailableBits(&stream) ;
        fprintf(stderr, ", bits available in stream = %ld\n", navail) ;

        // write counts into stream
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
        fprintf(stderr, "test_rle (after rewind) : bits available = %ld, totbits = %d", navail, totbits) ;
        fprintf(stderr, ", space left = %ld", STREAM_BITS_EMPTY(stream)) ;
        fprintf(stderr, ", stream size = %ld\n", navail + STREAM_BITS_EMPTY(stream)) ;
        if(ssize != navail + STREAM_BITS_EMPTY(stream)) exit(2) ;

        STREAM_REWRITE(stream, 1) ;
        fprintf(stderr, "\n");
        navail = StreamAvailableBits(&stream) ;
        fprintf(stderr, "test_rle (after rewrite) : bits available = %ld", navail) ;
        fprintf(stderr, ", space left = %ld", STREAM_BITS_EMPTY(stream)) ;
        fprintf(stderr, ", stream size = %ld\n", navail + STREAM_BITS_EMPTY(stream)) ;

        // write again into stream
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
        // read back stream
        for(uint32_t byte = 0 ; byte < 256 ; byte += 127){
          if(byte > 127) byte = 0xFF ;
          fprintf(stderr, "read_rle : byte = %2.2x\n", byte);
          for(count=1 ; count<275 ; count++){
            uint32_t kount = ((byte == 0x00) || (byte == 0xFF)) ? count : 1 ;
            r = get_rle_token(&stream) ;
            if(r.byte != byte || r.count != kount){
              fprintf(stderr, "r.code = %4.4x\n", r.code) ;
              fprintf(stderr, "count = %d, byte = %2.2x, got r.count = %d, r.byte = %2.2x\n",
                      kount, byte, r.count, r.byte) ;
              exit(3) ;
            }
          }
        }
        r = get_rle_token(&stream) ;    // get terminator token
        fprintf(stderr, "END TOKEN = %2.2x, count = %d\n", r.byte, r.count) ;
        if( ((r.byte != 0) && (r.byte != 0xFF)) || (r.count != 0) ){
          fprintf(stderr, "r.code = %4.4x\n", r.code) ;
          fprintf(stderr, "got r.count = %d, r.byte = %2.2x\n", r.count, r.byte) ;
          exit(4) ;
        }
        fprintf(stderr, "\nSUCCESS\n");
        break ;
      }
      case 2:{
        fprintf(stderr,"===== test mask creation =====\n");
        uint32_t demo[] = { 0x76543210, 0xfedcba98, 0x76543210 } ;
        print_mask32(stderr, "test 2 : ", demo, 64+5) ;

        uint32_t array[130], mask[1+sizeof(array)/sizeof(uint32_t)/32] ;
        for(uint64_t i=0 ; i<sizeof(array)/sizeof(uint32_t) ; i++) array[i] = 1 + i ;
        for(uint64_t i=0 ; i<sizeof(array)/sizeof(uint32_t) ; i+=8) array[i] = 0 ;
        for(uint64_t i=7 ; i<sizeof(array)/sizeof(uint32_t) ; i+=8) array[i] = 0 ;
        fprintf(stderr, "array size %ld words, mask size %ld bits\n", sizeof(array)/sizeof(uint32_t), sizeof(mask)*8) ;
        build_mask32(mask, array, 0, sizeof(array)/sizeof(uint32_t), 0);
        print_mask32(stderr, "mask 1 :", mask, sizeof(array)/sizeof(uint32_t)) ;
        fprintf(stderr, "\nSUCCESS\n");
        break ;
      }
      case 3:{
        int32_t orig[NJ][NI], packed[NINJ], restored[NJ][NI], mask[1 + NINJ/32], count, svalue = 0, expected ;
        for(int j=0 ; j<NJ ; j++){
          for(int i=0 ; i<NI ; i++){
            orig[j][i] = 1 + i*j ;
          }
        }
        expected = NI*NJ ;
        for(int i=0 ; i<NI ; i++){ orig[0][i] = orig[NJ-1][i] = svalue ; }  // top and bottom rows
        expected -= (2*NI) ;
        for(int j=0 ; j<NJ ; j++){ orig[j][0] = orig[j][NI-1] = svalue ; }  // first and last column
        expected -= (2*(NJ-2)) ;
        for(int j=NJ/4 ; j<3*NJ/4 ; j++){
          for(int i=NI/4 ; i<3*NI/4 ; i++){                                 // centered square
             orig[j][i] = svalue ;
             expected-- ;
          }
        }
        count = build_mask32((void *)mask, (void *)orig, svalue, NINJ, MASK_EQ | INVERT_MASK) ;
        fprintf(stderr, "build count = %d\n", count) ;
        print_mask32_2D(stderr, "row : ", (void *)mask, NI, NJ);
        count = compress_store((void *)packed, (void *)mask, (void *)orig, NINJ) ;
        fprintf(stderr, "store count = %d, expected = %d\n", count, expected) ;
        memset(restored, 0xFF, sizeof(restored)) ;
        count = expand_load((void *)restored, (void *)mask, (void *)packed, svalue, NINJ) ;
        fprintf(stderr, "load count = %d, expected = %d\n", count, expected) ;
        int32_t errors = 0 ;
        for(int j=0 ; j<NJ ; j++){
          for(int i=0 ; i<NI ; i++){
            if(orig[j][i] != restored[j][i]) errors++ ;
          }
        }
        fprintf(stderr, "restore errors = %d\n", errors) ;
        TIME_LOOP_EZ(100, NINJ, count = build_mask32((void *)mask, (void *)orig, svalue, NINJ, MASK_EQ | INVERT_MASK) ) ;
        fprintf(stderr, "build_mask32       : %s\n",timer_msg);
        TIME_LOOP_EZ(100, NINJ, count = compress_store((void *)packed, (void *)mask, (void *)orig, NINJ) ) ;
        fprintf(stderr, "compress_store     : %s\n",timer_msg);
        TIME_LOOP_EZ(100, NINJ, count = expand_load((void *)restored, (void *)mask, (void *)packed, svalue, NINJ) ) ;
        fprintf(stderr, "expand_load        : %s\n",timer_msg);
        break ;
      }
      case 4:{
        break ;
      }
      case 5:{
        break ;
      }
      default:{
        fprintf(stderr, "non existent mask test %d\n", test);
        break ;
      }
    }    // switch
  }    // for
}    // main
