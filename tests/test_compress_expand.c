#include <stdio.h>
#include <stdlib.h>

// double include done on purpose (test protection against double inclusion)
#include <rmn/compress_expand.h>
#include <rmn/compress_expand.h>
#include <rmn/timers.h>

#if defined(__AVX512F__)
void BitReverseArray_avx512(uint32_t *what, int n){
  int i ;
  for(i = 0 ; i < n-4 ; i += 4) BitReverse_128_avx512(what+i) ;
  for( ; i<n ; i++) what[i] = BitReverse_32(what[i]) ;
}
#endif

#if defined(__AVX2__)
void BitReverseArray_avx2(uint32_t *what, int n){
  int i ;
  for(i = 0 ; i < n-4 ; i += 4) BitReverse_128_avx2(what+i) ;
  for( ; i<n ; i++) what[i] = BitReverse_32(what[i]) ;
}
#endif

void BitReverseArray_c(uint32_t *what, int n){
  int i ;
  for(i = 0 ; i < n-4 ; i += 4) BitReverse_128_c(what+i) ;
  for( ; i<n ; i++) what[i] = BitReverse_32(what[i]) ;
}

void BitReverseArray(uint32_t *what, int n){
  int i ;
  for(i = 0 ; i < n-4 ; i += 4) BitReverse_128(what+i) ;
  for( ; i<n ; i++) what[i] = BitReverse_32(what[i]) ;
}

int compare_values(void *what, void *ref, int n){
  int i, errors = 0 ;
  uint32_t *v1 = (uint32_t *) what, *v2 = (uint32_t *) ref ;
  for(i=0 ; i<n ; i++) if(v1[i] != v2[i]) errors++ ;
  return errors ;
}

void test_reversal(char *str){
  uint32_t src[4] = {0x00000001, 0x00000600, 0x000f0000, 0x09000000 } ;
  uint32_t ref[4] = {0x80000000, 0x00600000, 0x0000f000, 0x00000090 } ;
  uint32_t s1[4], s2[4], s3[4] ;
  int npts = atoi(str) ;
  uint32_t tmp[npts] ;
  int i, errors ;
  TIME_LOOP_DATA

  fprintf(stderr, "bit reversal test with %d values\n", npts) ;

  for(i=0 ; i<4 ; i++) { s1[i] = s2[i] = s3[i] = src[i] ; }

  fprintf(stderr, "reference test with 4 values\n") ;

  for(i=0 ; i<4 ; i++) s1[i] = BitReverse_32(s1[i]) ; errors = compare_values(s1, ref, 4) ;
  if(errors > 0){
    for(i=0 ; i<4 ; i++)
      fprintf(stderr, "src = %8.8x, rev = %8.8x, ref = %8.8x\n", src[i], BitReverse_32(src[i]), ref[i]);
    exit(1) ;
  }
  fprintf(stderr, "scalar version SUCCESS\n");

#if defined(__AVX512F__)
  BitReverse_128_avx512(s3) ;
  errors = compare_values(s3, ref, 4) ;
  BitReverse_128_avx512(s3) ;
  errors += compare_values(s3, src, 4) ;
  if(errors) exit(1) ;
  fprintf(stderr, "AVX512 version SUCCESS\n");
#endif
#if defined(__AVX2__)
  BitReverse_128_avx2(s3) ;
  errors = compare_values(s3, ref, 4) ;
  BitReverse_128_avx2(s3) ;
  errors += compare_values(s3, src, 4) ;
  if(errors) exit(1) ;
  fprintf(stderr, "AVX2 version SUCCESS\n");
#endif
  BitReverse_128_c(s3) ;
  errors = compare_values(s3, ref, 4) ;
  BitReverse_128_c(s3) ;
  errors += compare_values(s3, src, 4) ;
  if(errors) exit(1) ;
  fprintf(stderr, "plain C version SUCCESS\n");

  BitReverse_128(s2) ;
  errors = compare_values(s2, ref, 4) ;
  if(errors > 0){
    for(i=0 ; i<4 ; i++)
      fprintf(stderr, "src = %8.8x, rev = %8.8x, ref = %8.8x\n", src[i], s2[i], ref[i]);
    exit(1) ;
  }
  BitReverse_128(s2) ;
  errors = compare_values(s2, src, 4) ;
  if(errors > 0){
    for(i=0 ; i<4 ; i++)
      fprintf(stderr, "src = %8.8x, rev = %8.8x, ref = %8.8x\n", src[i], s2[i], src[i]);
    exit(1) ;
  }
  fprintf(stderr, "generic version SUCCESS\n");

  BitReverseArray(tmp, npts) ;
  TIME_LOOP_EZ(1000, npts, BitReverseArray(tmp, npts) ;) ;
  fprintf(stderr, "BitReverseArray   : %s\n", timer_msg);

#if defined(__AVX512F__)
//   BitReverseArray_avx512(tmp, npts) ;
  BitReverseArray_avx512(tmp, npts) ;
  TIME_LOOP_EZ(1000, npts, BitReverseArray_avx512(tmp, npts) ;) ;
  fprintf(stderr, "BitReverse_avx512 : %s\n", timer_msg);
#endif

#if defined(__AVX2__)
  BitReverseArray_avx2(tmp, npts) ;
  BitReverseArray_avx2(tmp, npts) ;
  TIME_LOOP_EZ(1000, npts, BitReverseArray_avx2(tmp, npts) ;) ;
  fprintf(stderr, "BitReverse_avx2   : %s\n", timer_msg);
#endif

  BitReverseArray_c(tmp, npts) ;
  BitReverseArray_c(tmp, npts) ;
  TIME_LOOP_EZ(1000, npts, BitReverseArray_c(tmp, npts) ;) ;
  fprintf(stderr, "BitReverse_c      : %s\n", timer_msg);
}

void test_compress_store(char *str){
  int npts = atoi(str) ;
  int32_t expanded[npts], compressed[npts], restored[npts] ;
  uint32_t bmasks[npts], lmasks[npts] ;
  int i;
fprintf(stderr, "compress_store test with %d elements\n", npts) ;
  for(i=0 ; i<npts ; i++){
    expanded[i] = i + 1 ;
    compressed[i] = npts + 1 ;
    restored[i] = -1 ;
    bmasks[i] = 0x01234567 ;
    lmasks[i] = 0xE6A2C480 ;
  }
  int32_t *d0, *d1, *d2, *dbe, *dle, *sbe, *sle ;
  uint32_t popbe = popcnt_32(bmasks[0]) ;
  uint32_t pople = popcnt_32(lmasks[0]) ;
  if(npts >= 32){
#if defined(__AVX512F__)
    fprintf(stderr, "========== AVX512 (le) ==========\n") ;
    dle = CompressStore_32_avx512_le(expanded, compressed, lmasks[0]) ;
    fprintf(stderr, "mask = %8.8x, ", lmasks[0]) ;
    fprintf(stderr, "popbe = %d, pople = %d, points left = %ld\n", popbe, pople, dle - compressed) ;
    for(i=0 ; i<32 ; i++) fprintf(stderr, "%3d ", compressed[i]) ; fprintf(stderr, "\n") ;
    fprintf(stderr, "mask = %8.8x, ", lmasks[0]) ;
    sle = ExpandReplace_32_avx512_le(compressed, restored, lmasks[0]) ;
    fprintf(stderr, "popbe = %d, pople = %d, points read = %ld\n", popbe, pople, sle - compressed) ;
    for(i=0 ; i<32 ; i++) fprintf(stderr, "%3d ", restored[i]) ; fprintf(stderr, "\n") ;
    sle = ExpandFill_32_avx512_le(compressed, restored, lmasks[0], 99) ;
    for(i=0 ; i<32 ; i++) fprintf(stderr, "%3d ", restored[i]) ; fprintf(stderr, "\n") ;
#endif
    fprintf(stderr, "========== AVX2 (le) ==========\n") ;
    for(i=0 ; i<32 ; i++) { compressed[i] = npts + 1 ; }
    dle = CompressStore_32_avx2_le(expanded, compressed, lmasks[0]) ;
    fprintf(stderr, "mask = %8.8x, ", lmasks[0]) ;
    fprintf(stderr, "popbe = %d, pople = %d, points left = %ld\n", popbe, pople, dle - compressed) ;
    for(i=0 ; i<32 ; i++) fprintf(stderr, "%3d ", compressed[i]) ; fprintf(stderr, "\n") ;
    for(i=0 ; i<32 ; i++) { restored[i] = -1 ; }
    sle = ExpandReplace_32_avx2_le(compressed, restored, lmasks[0]) ;
    fprintf(stderr, "mask = %8.8x, ", lmasks[0]) ;
    fprintf(stderr, "popbe = %d, pople = %d, points read = %ld\n", popbe, pople, sle - compressed) ;
    for(i=0 ; i<32 ; i++) fprintf(stderr, "%3d ", restored[i]) ; fprintf(stderr, "\n") ;
    for(i=0 ; i<32 ; i++) { restored[i] = -1 ; }
    sle = ExpandFill_32_avx2_le(compressed, restored, lmasks[0], 88) ;
    for(i=0 ; i<32 ; i++) fprintf(stderr, "%3d ", restored[i]) ; fprintf(stderr, "\n") ;

    fprintf(stderr, "========== AVX2 (be) ==========\n") ;
    for(i=0 ; i<32 ; i++) { compressed[i] = npts + 1 ; }
    dle = CompressStore_32_avx2_be(expanded, compressed, bmasks[0]) ;
    fprintf(stderr, "mask = %8.8x, ", bmasks[0]) ;
    fprintf(stderr, "popbe = %d, pople = %d, points left = %ld\n", popbe, pople, dle - compressed) ;
    for(i=0 ; i<32 ; i++) fprintf(stderr, "%3d ", compressed[i]) ; fprintf(stderr, "\n") ;
    for(i=0 ; i<32 ; i++) { restored[i] = -1 ; }
    sle = ExpandReplace_32_avx2_be(compressed, restored, bmasks[0]) ;
    fprintf(stderr, "mask = %8.8x, ", bmasks[0]) ;
    fprintf(stderr, "popbe = %d, pople = %d, points read = %ld\n", popbe, pople, sle - compressed) ;
    for(i=0 ; i<32 ; i++) fprintf(stderr, "%3d ", restored[i]) ; fprintf(stderr, "\n") ;
    for(i=0 ; i<32 ; i++) { restored[i] = -1 ; }
    sle = ExpandFill_32_avx2_be(compressed, restored, bmasks[0], 77) ;
    for(i=0 ; i<32 ; i++) fprintf(stderr, "%3d ", restored[i]) ; fprintf(stderr, "\n") ;

    fprintf(stderr, "========== C (le) ==========\n") ;
    for(i=0 ; i<32 ; i++) { compressed[i] = npts + 1 ; restored[i] = -1 ; }
    dle = CompressStore_32_c_le(expanded, compressed, lmasks[0]) ;
    fprintf(stderr, "mask = %8.8x, ", lmasks[0]) ;
    fprintf(stderr, "popbe = %d, pople = %d, points left = %ld\n", popbe, pople, dle - compressed) ;
    for(i=0 ; i<32 ; i++) fprintf(stderr, "%3d ", compressed[i]) ; fprintf(stderr, "\n") ;
    for(i=0 ; i<32 ; i++) { restored[i] = -1 ; }
    sle = ExpandReplace_32_c_le(compressed, restored, lmasks[0]) ;
    fprintf(stderr, "mask = %8.8x, ", lmasks[0]) ;
    fprintf(stderr, "popbe = %d, pople = %d, points read = %ld\n", popbe, pople, sle - compressed) ;
    for(i=0 ; i<32 ; i++) fprintf(stderr, "%3d ", restored[i]) ; fprintf(stderr, "\n") ;
    for(i=0 ; i<32 ; i++) { restored[i] = -1 ; }
    sle = ExpandFill_32_c_le(compressed, restored, lmasks[0], 66) ;
    for(i=0 ; i<32 ; i++) fprintf(stderr, "%3d ", restored[i]) ; fprintf(stderr, "\n") ;

    fprintf(stderr, "========== C (be) ==========\n") ;
    for(i=0 ; i<32 ; i++) { compressed[i] = npts + 1 ; restored[i] = -1 ; }
    dle = CompressStore_32_c_be(expanded, compressed, bmasks[0]) ;
    fprintf(stderr, "mask = %8.8x, ", bmasks[0]) ;
    fprintf(stderr, "popbe = %d, pople = %d, points left = %ld\n", popbe, pople, dle - compressed) ;
    for(i=0 ; i<32 ; i++) fprintf(stderr, "%3d ", compressed[i]) ; fprintf(stderr, "\n") ;
    for(i=0 ; i<32 ; i++) { restored[i] = -1 ; }
    sle = ExpandReplace_32_c_be(compressed, restored, bmasks[0]) ;
    fprintf(stderr, "mask = %8.8x, ", bmasks[0]) ;
    fprintf(stderr, "popbe = %d, pople = %d, points read = %ld\n", popbe, pople, sle - compressed) ;
    for(i=0 ; i<32 ; i++) fprintf(stderr, "%3d ", restored[i]) ; fprintf(stderr, "\n") ;
    for(i=0 ; i<32 ; i++) { restored[i] = -1 ; }
    sle = ExpandFill_32_c_be(compressed, restored, bmasks[0], 55) ;
    for(i=0 ; i<32 ; i++) fprintf(stderr, "%3d ", restored[i]) ; fprintf(stderr, "\n") ;
  }
}

#define NPTS 1025

int main(int argc, char **argv){
  TIME_LOOP_DATA ;
  uint64_t overhead = get_cycles_overhead() ;
  uint32_t uarray[NPTS], bmasks[NPTS], lmasks[NPTS], ucomp0[NPTS], ucomp1[NPTS], ucomp2[NPTS] ;
  int32_t urest0[NPTS], urest1[NPTS] ;
  int i ;
  uint32_t *d0, *d1, *d2 ;
  uint32_t fill = 0xFF00u, pop0, pop1, pop2, errors ;

  fprintf(stderr, "timing cycles overhead = %ld cycles\n", overhead) ;
  for(i=0 ; i<NPTS ; i++) {
    uarray[i] = i+1 ;
    bmasks[i] = 0x01234567 ;
    lmasks[i] = 0xE6A2C480 ;
  }

  if(argc == 3 && atoi(argv[1]) == 1){
    test_compress_store(argv[2]) ;
    return 0 ;
  }
  if(argc == 3 && atoi(argv[1]) == 2){
    test_reversal(argv[2]) ;
    return 0 ;
  }

  for(i=0 ; i<NPTS ; i++) ucomp0[i] =  ucomp2[i] =0xF0F0F0F0u ;
  d0 = CompressStore_c_be(uarray, ucomp0, bmasks, NPTS) ;
  d2 = CompressStore_c_le(uarray, ucomp2, lmasks, NPTS) ;
  pop0 = d0-ucomp0 ;
  pop2 = d2-ucomp2 ;
  errors = (pop0 != pop2) ? 1 : 0 ;
  errors += compare_values(ucomp0, ucomp2, pop0) ;
//   for(i=0 ; i<pop0 ; i++) errors += ((ucomp0[i] != ucomp2[i]) ? 1 : 0 ) ;
  fprintf(stderr, "items after compress C (BE/LE) = %d, %d\n", pop0, pop2) ;
  fprintf(stderr, "BE(%d) vs LE(%d) discrepancies = %d\n", pop0, pop2, errors) ;
  if(errors) exit(1) ;

  for(i=0 ; i<NPTS ; i++) ucomp1[i] =  ucomp2[i] =0xF0F0F0F0u ;
  d1 = CompressStore_be(uarray, ucomp1, bmasks, NPTS) ;
  d2 = CompressStore_le(uarray, ucomp2, lmasks, NPTS) ;
  pop1 = d1-ucomp1 ;
  pop2 = d2-ucomp2 ;
  errors =   (pop0 != pop1) ? 1 : 0 ;
  errors += ((pop0 != pop2) ? 1 : 0) ;
  errors += compare_values(ucomp0, ucomp1, pop0) ;
  errors += compare_values(ucomp0, ucomp2, pop0) ;
//   for(i=0 ; i<pop0 ; i++) errors += ((ucomp0[i] != ucomp1[i]) ? 1 : 0 ) ;
//   for(i=0 ; i<pop0 ; i++) errors += ((ucomp0[i] != ucomp2[i]) ? 1 : 0 ) ;
  fprintf(stderr, "items after compress generic (BE/LE) = %d, %d\n", pop1, pop2) ;
  fprintf(stderr, "BE(%d) + LE(%d) discrepancies = %d\n", pop1, pop2, errors) ;
  if(errors) exit(1) ;

#if defined(__x86_64__) && defined(__AVX2__)
  for(i=0 ; i<NPTS ; i++) ucomp1[i] =  ucomp2[i] =0xF0F0F0F0u ;
  d1 = CompressStore_avx2_be(uarray, ucomp1, bmasks, NPTS) ;
  d2 = CompressStore_avx2_le(uarray, ucomp2, lmasks, NPTS) ;
  pop1 = d1-ucomp1 ;
  pop2 = d2-ucomp2 ;
  errors =   (pop0 != pop1) ? 1 : 0 ;
  errors += ((pop0 != pop2) ? 1 : 0) ;
  errors += compare_values(ucomp0, ucomp1, pop0) ;
  errors += compare_values(ucomp0, ucomp2, pop0) ;
//   for(i=0 ; i<pop0 ; i++) errors += ((ucomp0[i] != ucomp1[i]) ? 1 : 0 ) ;
//   for(i=0 ; i<pop0 ; i++) errors += ((ucomp0[i] != ucomp2[i]) ? 1 : 0 ) ;
  fprintf(stderr, "items after compress sse (BE/LE) = %d, %d\n", pop1, pop2) ;
  fprintf(stderr, "BE(%d) + LE(%d) discrepancies = %d\n", pop1, pop2, errors) ;
  if(errors) exit(1) ;
#endif

#if defined(__x86_64__) && defined(__AVX512F__)
  for(i=0 ; i<NPTS ; i++) ucomp2[i] = 0xF0F0F0F0u ;   // ucomp1 left as before for now
//   d1 = CompressStore_avx512_be(uarray, ucomp1, bmasks, NPTS) ;
  d2 = CompressStore_avx512_le(uarray, ucomp2, lmasks, NPTS) ;
//   pop1 = d1-ucomp1 ;
  pop2 = d2-ucomp2 ;  pop1 = pop2 ;  // for now, pop1 forced to pop2
  fprintf(stderr, "items after compress AVX512 (BE/LE) = %d, %d\n", pop1, pop2) ;
  errors =   (pop0 != pop1) ? 1 : 0 ;
  errors += ((pop0 != pop2) ? 1 : 0) ;
  for(i=0 ; i<pop0 ; i++) errors += ((ucomp0[i] != ucomp1[i]) ? 1 : 0 ) ;
  for(i=0 ; i<pop0 ; i++) errors += ((ucomp0[i] != ucomp2[i]) ? 1 : 0 ) ;
  fprintf(stderr, "BE(%d) + LE(%d) discrepancies = %d\n", pop1, pop2, errors) ;
  if(errors) exit(1) ;
#endif
  fprintf(stderr, "\n");

  fill = 99 ;

  for(i=0 ; i<NPTS ; i++) urest0[i] = -1 ;
  ExpandFill_32_c_be(ucomp0, urest0, bmasks[0], fill) ;
  for(i=0 ; i<NPTS ; i++) urest1[i] = uarray[i] ;
//   MaskReplace_32_c_be(urest1, ~bmasks[0], 88) ;
  for(i=0 ; i<32 ; i++) fprintf(stderr, "%3d ", urest1[i]) ; fprintf(stderr, "\n") ;
  fprintf(stderr, "after ExpandFill_32_c_be, mask = %8.8x, fill = %d\n", bmasks[0], fill) ;
  for(i=0 ; i<32 ; i++) fprintf(stderr, "%3d ", uarray[i]) ; fprintf(stderr, "\n") ;
  for(i=0 ; i<32 ; i++) fprintf(stderr, "%3d ", (bmasks[0] >> (31-i)) & 1 ) ; fprintf(stderr, "\n") ;
  for(i=0 ; i<popcnt_32(bmasks[0]) ; i++) fprintf(stderr, "%3d ", ucomp0[i]) ; fprintf(stderr, "\n") ;
  for(i=0 ; i<32 ; i++) fprintf(stderr, "%3d ", urest0[i]) ; fprintf(stderr, "\n") ;
//   for(i=16; i<32 ; i++) fprintf(stderr, "%8.8x ", urest0[i]) ; fprintf(stderr, "\n") ;
// return 0 ;
#if defined(__x86_64__) && defined(__AVX2__)
  for(i=0 ; i<NPTS ; i++) urest1[i] = 0xFFFFFFFFu ;
  ExpandFill_32_avx2_be(ucomp0, urest1, bmasks[0], fill) ;
  fprintf(stderr, "after ExpandFill_32_sse\n") ;
  for(i=0 ; i<16 ; i++) fprintf(stderr, "%8.8x ", urest1[i]) ; fprintf(stderr, "\n") ;
  for(i=16; i<32 ; i++) fprintf(stderr, "%8.8x ", urest1[i]) ; fprintf(stderr, "\n") ;

  errors = 0 ;
  for(i=0 ; i<32 ; i++) errors += ((urest0[i] != urest1[i]) ? 1 : 0 ) ;
  fprintf(stderr, "AVX2 vs C expand discrepancies = %d\n", errors) ;
  if(errors) exit(1) ;
#endif
// return 0 ;
  for(i=0 ; i<NPTS ; i++) urest0[i] = 0xFF000000u + i + 1 ;
  ExpandFill_be(ucomp0, urest0, bmasks, NPTS, NULL) ;
  fprintf(stderr, "after replace\n") ;
  for(i=0 ; i<16 ; i++) fprintf(stderr, "%8.8x ", urest0[i]) ; fprintf(stderr, "\n") ;
  for(i=16; i<32 ; i++) fprintf(stderr, "%8.8x ", urest0[i]) ; fprintf(stderr, "\n") ;

#if defined(__x86_64__) && defined(__AVX2__)
  for(i=0 ; i<NPTS ; i++) urest1[i] = 0xFF000000u + i + 1 ;
  ExpandFill_be(ucomp0, urest1, bmasks, NPTS, NULL) ;
  fprintf(stderr, "after replace_sse\n") ;
  for(i=0 ; i<16 ; i++) fprintf(stderr, "%8.8x ", urest1[i]) ; fprintf(stderr, "\n") ;
  for(i=16; i<32 ; i++) fprintf(stderr, "%8.8x ", urest1[i]) ; fprintf(stderr, "\n") ;

  errors = 0 ;
  for(i=0 ; i<NPTS ; i++) errors += ((urest0[i] != urest1[i]) ? 1 : 0 ) ;
  fprintf(stderr, "AVX2 vs C replace discrepancies = %d\n", errors) ;
  if(errors) exit(1) ;
#endif

  TIME_LOOP_EZ(1000, NPTS, CompressStore_c_be(uarray, ucomp0, bmasks, NPTS)) ;
  fprintf(stderr, "compress_c     : %s\n", timer_msg);

  TIME_LOOP_EZ(1000, NPTS, CompressStore_be(uarray, ucomp0, bmasks, NPTS)) ;
  fprintf(stderr, "compress       : %s\n", timer_msg);

#if defined(__x86_64__) && defined(__AVX2__)
  TIME_LOOP_EZ(1000, NPTS, CompressStore_avx2_be(uarray, ucomp0, bmasks, NPTS)) ;
  fprintf(stderr, "compress_sse   : %s\n", timer_msg);
#endif

#if defined(__x86_64__) && defined(__AVX2__)
  TIME_LOOP_EZ(1000, NPTS, CompressStore_le(uarray, ucomp2, bmasks, NPTS)) ;
  fprintf(stderr, "compress_AVX2: %s\n", timer_msg);
#endif

  TIME_LOOP_EZ(1000, NPTS, ExpandFill_be(uarray, ucomp0, bmasks, NPTS, &fill)) ;
  fprintf(stderr, "expand_fill    : %s\n", timer_msg);

#if defined(__x86_64__) && defined(__AVX2__)
  TIME_LOOP_EZ(1000, NPTS, ExpandFill_avx2_be(uarray, ucomp0, bmasks, NPTS, &fill)) ;
  fprintf(stderr, "expand_fill_sse: %s\n", timer_msg);
#endif

  TIME_LOOP_EZ(1000, NPTS, ExpandFill_be(uarray, ucomp0, bmasks, NPTS, NULL)) ;
  fprintf(stderr, "replace        : %s\n", timer_msg);

#if defined(__x86_64__) && defined(__AVX2__)
  TIME_LOOP_EZ(1000, NPTS, ExpandFill_avx2_be(uarray, ucomp0, bmasks, NPTS, NULL)) ;
  fprintf(stderr, "replace_sse    : %s\n", timer_msg);
#endif
}
