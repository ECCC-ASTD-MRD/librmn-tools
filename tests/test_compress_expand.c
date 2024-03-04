#include <stdio.h>
#include <stdlib.h>

// double include done on purpose (test protection against double inclusion)
#include <rmn/compress_expand.h>
#include <rmn/compress_expand.h>
#include <rmn/timers.h>

#define NPTS 64

void test_compress_store(int npts){
  int32_t expanded[npts], compressed[npts], restored[npts] ;
  uint32_t bmasks[npts] ;
  int i;
fprintf(stderr, "compress_store test with %d elements\n", npts) ;
  for(i=0 ; i<npts ; i++){
    expanded[i] = i ;
    compressed[i] = npts + 1 ;
    restored[i] = -1 ;
  }
}

int main(int argc, char **argv){
  TIME_LOOP_DATA ;
  uint64_t overhead = get_cycles_overhead() ;
  uint32_t uarray[NPTS], bmasks[NPTS], lmasks[NPTS], ucomp0[NPTS], urest0[NPTS], ucomp1[NPTS], ucomp2[NPTS], urest1[NPTS] ;
  int i ;
  uint32_t *d0, *d1, *d2 ;
  uint32_t fill = 0xFF00u, pop0, pop1, pop2, errors ;

  fprintf(stderr, "timing cycles overhead = %ld cycles\n", overhead) ;
  for(i=0 ; i<NPTS ; i++) {
    uarray[i] = i+1 ;
    bmasks[i] = 0x01234567 ;
    lmasks[i] = 0xE6A2C480 ;
//     lmasks[i] = 0x76543210 ;
  }

  if(argc == 2){
    test_compress_store(atoi(argv[1])) ;
    return 0 ;
  }

//   for(i=1 ; i<NPTS ; i+=2) bmasks[i] = 0x89ABCDEF ;

  for(i=0 ; i<NPTS ; i++) ucomp0[i] = 0xF0F0F0F0u ;
  d0 = CompressStore_be(uarray, ucomp0, bmasks, NPTS) ;
  d2 = CompressStore_le(uarray, ucomp2, lmasks, NPTS) ;
  pop0 = d0-ucomp0 ;
  pop2 = d2-ucomp2 ;
  fprintf(stderr, "items after compress = %d, %d\n", pop0, pop2) ;
  for(i=0 ; i<24 ; i++) fprintf(stderr, "%8.8x ", ucomp0[i]) ; fprintf(stderr, "\n") ;
  for(i=0 ; i<24 ; i++) fprintf(stderr, "%8.8x ", ucomp2[i]) ; fprintf(stderr, "\n") ;
//   for(i=16 ; i<32 ; i++) fprintf(stderr, "%8.8x ", ucomp0[i]) ; fprintf(stderr, "\n") ;
//   for(i=32 ; i<48 ; i++) fprintf(stderr, "%8.8x ", ucomp0[i]) ; fprintf(stderr, "\n") ;
return 0 ;
#if defined(__x86_64__) && defined(__SSE2__)
  for(i=0 ; i<NPTS ; i++) ucomp1[i] = 0xF0F0F0F0u ;
  d1 = CompressStore_sse_be(uarray, ucomp1, bmasks, NPTS) ;
  pop1 = d1-ucomp1 ;
  fprintf(stderr, "items after compress sse = %d\n", pop1) ;
  for(i=0 ; i<16 ; i++) fprintf(stderr, "%8.8x ", ucomp1[i]) ; fprintf(stderr, "\n") ;
//   for(i=16 ; i<32 ; i++) fprintf(stderr, "%8.8x ", ucomp0[i]) ; fprintf(stderr, "\n") ;
//   for(i=32 ; i<48 ; i++) fprintf(stderr, "%8.8x ", ucomp0[i]) ; fprintf(stderr, "\n") ;

  errors = (pop0 != pop1) ? 1 : 0 ;
  for(i=0 ; i<pop0 ; i++) errors += ((ucomp0[i] != ucomp1[i]) ? 1 : 0 ) ;
  fprintf(stderr, "SSE(%d) vs C(%d) discrepancies = %d\n", pop1, pop0, errors) ;
  if(errors) exit(1) ;
#endif

  for(i=0 ; i<NPTS ; i++) urest0[i] = 0xFFFFFFFFu ;
  stream_expand_32(ucomp0, urest0, bmasks, NPTS, &fill) ;
  fprintf(stderr, "after expand\n") ;
  for(i=0 ; i<16 ; i++) fprintf(stderr, "%8.8x ", urest0[i]) ; fprintf(stderr, "\n") ;
  for(i=16; i<32 ; i++) fprintf(stderr, "%8.8x ", urest0[i]) ; fprintf(stderr, "\n") ;

#if defined(__x86_64__) && defined(__SSE2__)
  for(i=0 ; i<NPTS ; i++) urest1[i] = 0xFFFFFFFFu ;
  stream_expand_32_sse(ucomp0, urest1, bmasks, NPTS, &fill) ;
  fprintf(stderr, "after expand_sse\n") ;
  for(i=0 ; i<16 ; i++) fprintf(stderr, "%8.8x ", urest1[i]) ; fprintf(stderr, "\n") ;
  for(i=16; i<32 ; i++) fprintf(stderr, "%8.8x ", urest1[i]) ; fprintf(stderr, "\n") ;

  errors = 0 ;
  for(i=0 ; i<NPTS ; i++) errors += ((urest0[i] != urest1[i]) ? 1 : 0 ) ;
  fprintf(stderr, "SSE vs C expand discrepancies = %d\n", errors) ;
  if(errors) exit(1) ;
#endif

  for(i=0 ; i<NPTS ; i++) urest0[i] = 0xFF000000u + i + 1 ;
  stream_expand_32(ucomp0, urest0, bmasks, NPTS, NULL) ;
  fprintf(stderr, "after replace\n") ;
  for(i=0 ; i<16 ; i++) fprintf(stderr, "%8.8x ", urest0[i]) ; fprintf(stderr, "\n") ;
  for(i=16; i<32 ; i++) fprintf(stderr, "%8.8x ", urest0[i]) ; fprintf(stderr, "\n") ;

#if defined(__x86_64__) && defined(__SSE2__)
  for(i=0 ; i<NPTS ; i++) urest1[i] = 0xFF000000u + i + 1 ;
  stream_expand_32_sse(ucomp0, urest1, bmasks, NPTS, NULL) ;
  fprintf(stderr, "after replace_sse\n") ;
  for(i=0 ; i<16 ; i++) fprintf(stderr, "%8.8x ", urest1[i]) ; fprintf(stderr, "\n") ;
  for(i=16; i<32 ; i++) fprintf(stderr, "%8.8x ", urest1[i]) ; fprintf(stderr, "\n") ;

  errors = 0 ;
  for(i=0 ; i<NPTS ; i++) errors += ((urest0[i] != urest1[i]) ? 1 : 0 ) ;
  fprintf(stderr, "SSE vs C replace discrepancies = %d\n", errors) ;
  if(errors) exit(1) ;
#endif

  TIME_LOOP_EZ(1000, NPTS, CompressStore_c_be(uarray, ucomp0, bmasks, NPTS)) ;
  fprintf(stderr, "compress_c     : %s\n", timer_msg);

  TIME_LOOP_EZ(1000, NPTS, CompressStore_be(uarray, ucomp0, bmasks, NPTS)) ;
  fprintf(stderr, "compress       : %s\n", timer_msg);

#if defined(__x86_64__) && defined(__SSE2__)
  TIME_LOOP_EZ(1000, NPTS, CompressStore_sse_be(uarray, ucomp0, bmasks, NPTS)) ;
  fprintf(stderr, "compress_sse   : %s\n", timer_msg);
#endif

#if defined(__x86_64__) && defined(__SSE2__)
  TIME_LOOP_EZ(1000, NPTS, CompressStore_le(uarray, ucomp2, bmasks, NPTS)) ;
  fprintf(stderr, "compress_AVX512: %s\n", timer_msg);
#endif

  TIME_LOOP_EZ(1000, NPTS, stream_expand_32(uarray, ucomp0, bmasks, NPTS, &fill)) ;
  fprintf(stderr, "expand_fill    : %s\n", timer_msg);

#if defined(__x86_64__) && defined(__SSE2__)
  TIME_LOOP_EZ(1000, NPTS, stream_expand_32_sse(uarray, ucomp0, bmasks, NPTS, &fill)) ;
  fprintf(stderr, "expand_fill_sse: %s\n", timer_msg);
#endif

  TIME_LOOP_EZ(1000, NPTS, stream_expand_32(uarray, ucomp0, bmasks, NPTS, NULL)) ;
  fprintf(stderr, "replace        : %s\n", timer_msg);

#if defined(__x86_64__) && defined(__SSE2__)
  TIME_LOOP_EZ(1000, NPTS, stream_expand_32_sse(uarray, ucomp0, bmasks, NPTS, NULL)) ;
  fprintf(stderr, "replace_sse    : %s\n", timer_msg);
#endif
}
