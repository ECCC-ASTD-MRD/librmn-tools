#include <stdio.h>
#include <stdlib.h>

// double include done on purpose (test protection against double inclusion)
#include <rmn/compress_expand.h>
#include <rmn/compress_expand.h>
#include <rmn/timers.h>

#define NPTS 1023

int main(int argc, char **argv){
  TIME_LOOP_DATA ;
  uint64_t overhead = get_cycles_overhead() ;
  uint32_t uarray[NPTS], masks[NPTS], ucomp0[NPTS], urest0[NPTS], ucomp1[NPTS], urest1[NPTS] ;
  int i ;
  uint32_t *d0, *d1 ;
  uint32_t fill = 0xFF00u, pop0, pop1, errors ;

  fprintf(stderr, "timing cycles overhead = %ld cycles\n", overhead) ;
  for(i=0 ; i<NPTS ; i++) {
    uarray[i] = i+1 ;
    masks[i] = 0x01234567 ;
  }
  for(i=1 ; i<NPTS ; i+=2) masks[i] = 0x89ABCDEF ;

  for(i=0 ; i<NPTS ; i++) ucomp0[i] = 0xF0F0F0F0u ;
  d0 = stream_compress_32(uarray, ucomp0, masks, NPTS) ;
  pop0 = d0-ucomp0 ;
  fprintf(stderr, "items after compress = %d\n", pop0) ;
  for(i=0 ; i<16 ; i++) fprintf(stderr, "%8.8x ", ucomp0[i]) ; fprintf(stderr, "\n") ;
//   for(i=16 ; i<32 ; i++) fprintf(stderr, "%8.8x ", ucomp0[i]) ; fprintf(stderr, "\n") ;
//   for(i=32 ; i<48 ; i++) fprintf(stderr, "%8.8x ", ucomp0[i]) ; fprintf(stderr, "\n") ;

#if defined(__x86_64__) && defined(__SSE2__)
  for(i=0 ; i<NPTS ; i++) ucomp1[i] = 0xF0F0F0F0u ;
  d1 = stream_compress_32_sse(uarray, ucomp1, masks, NPTS) ;
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
  stream_expand_32(ucomp0, urest0, masks, NPTS, &fill) ;
  fprintf(stderr, "after expand\n") ;
  for(i=0 ; i<16 ; i++) fprintf(stderr, "%8.8x ", urest0[i]) ; fprintf(stderr, "\n") ;
  for(i=16; i<32 ; i++) fprintf(stderr, "%8.8x ", urest0[i]) ; fprintf(stderr, "\n") ;

#if defined(__x86_64__) && defined(__SSE2__)
  for(i=0 ; i<NPTS ; i++) urest1[i] = 0xFFFFFFFFu ;
  stream_expand_32_sse(ucomp0, urest1, masks, NPTS, &fill) ;
  fprintf(stderr, "after expand_sse\n") ;
  for(i=0 ; i<16 ; i++) fprintf(stderr, "%8.8x ", urest1[i]) ; fprintf(stderr, "\n") ;
  for(i=16; i<32 ; i++) fprintf(stderr, "%8.8x ", urest1[i]) ; fprintf(stderr, "\n") ;

  errors = 0 ;
  for(i=0 ; i<NPTS ; i++) errors += ((urest0[i] != urest1[i]) ? 1 : 0 ) ;
  fprintf(stderr, "SSE vs C expand discrepancies = %d\n", errors) ;
  if(errors) exit(1) ;
#endif

  for(i=0 ; i<NPTS ; i++) urest0[i] = 0xFF000000u + i + 1 ;
  stream_expand_32(ucomp0, urest0, masks, NPTS, NULL) ;
  fprintf(stderr, "after replace\n") ;
  for(i=0 ; i<16 ; i++) fprintf(stderr, "%8.8x ", urest0[i]) ; fprintf(stderr, "\n") ;
  for(i=16; i<32 ; i++) fprintf(stderr, "%8.8x ", urest0[i]) ; fprintf(stderr, "\n") ;

#if defined(__x86_64__) && defined(__SSE2__)
  for(i=0 ; i<NPTS ; i++) urest1[i] = 0xFF000000u + i + 1 ;
  stream_expand_32_sse(ucomp0, urest1, masks, NPTS, NULL) ;
  fprintf(stderr, "after replace_sse\n") ;
  for(i=0 ; i<16 ; i++) fprintf(stderr, "%8.8x ", urest1[i]) ; fprintf(stderr, "\n") ;
  for(i=16; i<32 ; i++) fprintf(stderr, "%8.8x ", urest1[i]) ; fprintf(stderr, "\n") ;

  errors = 0 ;
  for(i=0 ; i<NPTS ; i++) errors += ((urest0[i] != urest1[i]) ? 1 : 0 ) ;
  fprintf(stderr, "SSE vs C replace discrepancies = %d\n", errors) ;
  if(errors) exit(1) ;
#endif

  TIME_LOOP_EZ(1000, NPTS, stream_compress_32(uarray, ucomp0, masks, NPTS)) ;
  fprintf(stderr, "compress       : %s\n", timer_msg);

#if defined(__x86_64__) && defined(__SSE2__)
  TIME_LOOP_EZ(1000, NPTS, stream_compress_32_sse(uarray, ucomp0, masks, NPTS)) ;
  fprintf(stderr, "compress_sse   : %s\n", timer_msg);
#endif

  TIME_LOOP_EZ(1000, NPTS, stream_expand_32(uarray, ucomp0, masks, NPTS, &fill)) ;
  fprintf(stderr, "expand_fill    : %s\n", timer_msg);

#if defined(__x86_64__) && defined(__SSE2__)
  TIME_LOOP_EZ(1000, NPTS, stream_expand_32_sse(uarray, ucomp0, masks, NPTS, &fill)) ;
  fprintf(stderr, "expand_fill_sse: %s\n", timer_msg);
#endif

  TIME_LOOP_EZ(1000, NPTS, stream_expand_32(uarray, ucomp0, masks, NPTS, NULL)) ;
  fprintf(stderr, "replace        : %s\n", timer_msg);

#if defined(__x86_64__) && defined(__SSE2__)
  TIME_LOOP_EZ(1000, NPTS, stream_expand_32_sse(uarray, ucomp0, masks, NPTS, NULL)) ;
  fprintf(stderr, "replace_sse    : %s\n", timer_msg);
#endif
}
