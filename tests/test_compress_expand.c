#include <stdio.h>
#include <stdlib.h>

// double include done on purpose (test protection against double inclusion)
#include <rmn/compress_expand.h>
#include <rmn/compress_expand.h>
#include <rmn/timers.h>
#include <rmn/c_record_io.h>

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
  int npts = atoi(str), nc ;
  int32_t expanded[npts], compressed[npts], restored[npts], plugged[npts], fill2[npts], plug2[npts] ;
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
    fprintf(stderr, "========== C (le) ==========\n") ;
    for(i=0 ; i<32 ; i++) { compressed[i] = npts + 1 ; restored[i] = -1 ; fill2[i] = -expanded[i] ; }
    dle = CompressStore_32_c_le(expanded, compressed, lmasks[0]) ;
    fprintf(stderr, "mask = %8.8x, ", lmasks[0]) ; nc = dle - compressed ;
    fprintf(stderr, "popbe = %d, pople = %d, points left = %d\n", popbe, pople, nc) ;
    for(i=0 ; i<32 ; i++) fprintf(stderr, "%3d ", expanded[i]) ; fprintf(stderr, " - orig\n") ;
    for(i=0 ; i<32 ; i++) { plugged[i] = -1 ; plug2[i] = 999 ; }
    MaskedFill_le(expanded, plugged, lmasks[0], 99, 32) ;
    MaskedMerge_le(expanded, plug2, lmasks[0], fill2, 32) ;
    for(i=0 ; i<32 ; i++) fprintf(stderr, "%3d ", plugged[i]) ; fprintf(stderr, " - fill\n") ;
    for(i=0 ; i<32 ; i++) fprintf(stderr, "%3d ", fill2[i]) ; fprintf(stderr, " - filler\n") ;
    for(i=0 ; i<32 ; i++) fprintf(stderr, "%3d ", plug2[i]) ; fprintf(stderr, " - after merge\n") ;
    for(i=0 ; i<nc ; i++) fprintf(stderr, "%3d ", compressed[i]) ; fprintf(stderr, " - compressed\n") ;
    for(i=0 ; i<32 ; i++) { restored[i] = 100+i ; }
    for(i=0 ; i<32 ; i++) fprintf(stderr, "%3d ", restored[i]) ; fprintf(stderr, " - before replace/fill\n") ;
    sle = ExpandReplace_32_c_le(compressed, restored, lmasks[0]) ; nc = sle - compressed ;
    fprintf(stderr, "mask = %8.8x, ", lmasks[0]) ;
    fprintf(stderr, "popbe = %d, pople = %d, points read = %d\n", popbe, pople, nc) ;
    for(i=0 ; i<32 ; i++) fprintf(stderr, "%3d ", restored[i]) ; fprintf(stderr, " - after replace\n") ;
    for(i=0 ; i<32 ; i++) { restored[i] = -1 ; }
    sle = ExpandFill_32_c_le(compressed, restored, lmasks[0], 66) ;
    for(i=0 ; i<32 ; i++) fprintf(stderr, "%3d ", restored[i]) ; fprintf(stderr, " - after fill\n") ;
return ;
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
#if defined(__AVX512F___)
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
#if defined(__AVX2___)
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
#endif
  }
}

int32_t rep_encode(int32_t what, int32_t rep){
  int32_t nbits = 0 ;
  int32_t bi = 3 ;
  while(rep > 0){
    if(rep & 1) {
      nbits = nbits + bi ;
    }
    rep >>= 1 ;
    bi++ ;
  }
  return nbits ;
}

void test_bit_mask_field(char *filename){
  int32_t dims[7], ndims, ndata, fd = 0, i, j, nzeros, nones, old = 2, rep = 0, totbits = 0, deficit = 0 ;
  char name[5] ;
  float *fdata, fmin, fmax ;
  int32_t *idata, *bitmask, ncomp ;
  uint32_t zero = 0 ;
  uint64_t t0 ;

  for(j=0 ; j<1 ; j++){
    ndims = 0 ;
    void *data = read_32bit_data_record_named(filename, &fd, dims, &ndims, &ndata, name) ;
fprintf( stderr, "read record from '%s', %d dimensions, name = '%s', ndata = %d, (%d x %d)\n",
          filename, ndims, name, ndata, dims[0], dims[1]) ;
    idata = (int32_t *) data ;
    fdata = (float *) data ;
    fmin = fmax = fdata[0] ;
    for(i=0 ; i<ndata ; i++){
      fmin = (fdata[i] < fmin) ? fdata[i] : fmin ;
      fmax = (fdata[i] > fmax) ? fdata[i] : fmax ;
    }
fprintf( stderr, "min = %f, max = %f\n", fmin, fmax) ;
    nzeros = nones = 0 ;
    fdata[ndata-1] = fdata[ndata-9] = 10.0f ;  // make sure the last 2 bytes contain at least 1 bit set
    for(i=0 ; i<ndata ; i++){
      idata[i] = (fdata[i] > 0) ? 1 : 0 ;
      if(old == idata[i]){
        rep++ ;
      }else{
        int32_t nbits = rep_encode(old, rep >> 3) ;
  // fprintf(stderr,"%1d %d ", old, nbits) ;
        old = idata[i] ;
        totbits += nbits ;
        rep &= 7 ;
        if(rep){
          totbits += 9 ;
          rep = rep - 8 ;
        }
      }
      nzeros += ((idata[i] == 0) ? 1 : 0) ;
      nones += ((idata[i] == 1) ? 1 : 0) ;
    }
fprintf( stderr, "\n");
fprintf( stderr, "number of 1s = %d, number of 0s = %d, total = %d, totbits = %d\n", nones, nzeros, nones + nzeros, totbits) ;
    uint32_t the_bits[ndata] , aec_compressed[ndata] ;
#if defined(__x86_64__) && defined(__AVX512F__)
    for(i=0 ; i<ndata ; i++) the_bits[i] = 0 ;
    nones = nzeros = 0 ;
    t0 = elapsed_cycles_fast() ;
    nones = MaskGreater_avx512_le(idata , ndata, &zero, 1, the_bits, 0) ;
    t0 = elapsed_cycles_fast() - t0 ;
fprintf( stderr, "number of 1s (avx512) = %d, cycles = %ld\n", nones, t0) ;
    t0 = elapsed_cycles_fast() ;
    nzeros = MaskGreater_avx512_le(idata , ndata, &zero, 1, the_bits, 1) ;
    t0 = elapsed_cycles_fast() - t0 ;
fprintf( stderr, "number of 0s (avx512) = %d, cycles = %ld\n", nzeros, t0) ;
#endif
#if defined(__x86_64__) && defined(__AVX2__)
    for(i=0 ; i<ndata ; i++) the_bits[i] = 0 ;
    nones = nzeros = 0 ;
    t0 = elapsed_cycles_fast() ;
    nones = MaskGreater_avx2_le(idata , ndata, &zero, 1, the_bits, 0) ;
    t0 = elapsed_cycles_fast() - t0 ;
fprintf( stderr, "number of 1s (avx2)   = %d, cycles = %ld\n", nones, t0) ;
    t0 = elapsed_cycles_fast() ;
    nzeros = MaskGreater_avx2_le(idata , ndata, &zero, 1, the_bits, 1) ;
    t0 = elapsed_cycles_fast() - t0 ;
fprintf( stderr, "number of 0s (avx2)   = %d, cycles = %ld\n", nzeros, t0) ;
#endif
    for(i=0 ; i<ndata ; i++) the_bits[i] = 0 ;
    nones = nzeros = 0 ;
//     nones = MaskGreater_avx2_le(idata , ndata, &zero, 1, the_bits, 0) ;
// fprintf( stderr, "number of 1s (avx2)   = %d\n", nones) ;
    for(i=0 ; i<ndata ; i++) the_bits[i] = 0 ;
    t0 = elapsed_cycles_fast() ;
    nones = MaskGreater_c_le(idata , ndata, &zero, 1, the_bits, 0) ;
    t0 = elapsed_cycles_fast() - t0 ;
fprintf( stderr, "number of 1s (c)      = %d, cycles = %ld\n", nones, t0) ;
    t0 = elapsed_cycles_fast() ;
    nzeros = MaskGreater_c_le(idata , ndata, &zero, 1, the_bits, 1) ;
    t0 = elapsed_cycles_fast() - t0 ;
fprintf( stderr, "number of 0s (c)      = %d, cycles = %ld\n", nzeros, t0) ;

return ;
// AecEncodeUnsigned(void *source, int32_t source_length, void *dest, int32_t dest_length, int bits_per_sample)
    ncomp = AecEncodeUnsigned(the_bits, (ndata+7)/8, aec_compressed, ndata, 8) ;
fprintf( stderr, "bytes = %d, compressed bytes = %d\n", (ndata+7)/8, ncomp) ;
// AecDecodeUnsigned(void *source, int32_t source_length, void *dest, int32_t dest_length, int bits_per_sample)
    uint32_t restored[ndata] ;
    ncomp = AecDecodeUnsigned(aec_compressed, ndata, restored, ndata, 8) ;
fprintf( stderr, "decompressed bytes = %d\n", ncomp) ;
    int errors = 0 ;
    for(i=0 ; i<(ndata+31)/32 ; i++){
      if(restored[i] != the_bits[i]){
        errors++ ;
//         fprintf( stderr, "error at %d\n", i) ;
//         exit(1) ;
      }
    }
    fprintf( stderr, "decompression errors = %d, %d words, %d points\n", errors, (ndata+31)/32, ((ndata+31)/32)*32) ;
  }
}

typedef union{
  uint32_t u ;
  int32_t  i ;
} ui ;
typedef struct{
  union{
    uint32_t ul ;
    int32_t  il ;
  } ;
  union{
    uint32_t uh ;
    int32_t  ih ;
  } ;
}word_2 ;
typedef struct{
  ui l ;
  ui h ;
}words_2 ;

#define NIZ 11
#define NJZ 9
void test_interleave(char *msg){
  uint32_t int1 = interleave_16_32(0x1234, 0x0000) ;
  uint32_t int2 = interleave_16_32(0x0000, 0x1234) ;
  uint32_t int3 = interleave_16_32(0x1111, 0x1111) ;
  fprintf(stderr, "%8.8x %8.8x %8.8x\n", int1, int2, int3) ;
  uint32_t z[NJZ][NIZ] ;
  uint64_t k64, k32, t0 ;

  words2 t ;
  int32_t i, j, k, errors = 0 ;
//   for(k=0 ; k<16 ; k++){
//     t = deinterleave_32_16(k) ;
//     fprintf(stderr, "%3d %3d %3d\n", k, t.l, t.h) ;
//   }
  errors = 0 ;
  for(j=NJZ-1 ; j>=0 ; j--){
    for(i=0 ; i<NIZ-1 ; i++){
      z[j][i] = interleave_16_32((uint32_t) i, (uint32_t) j) ;
      t = deinterleave_32_16(z[j][i]) ;
      fprintf(stderr, "%3d ", z[j][i]) ;
      if(t.l != i || t.h != j) errors++ ;
    }
    fprintf(stderr, "\n");
  }
  fprintf(stderr, "deinterleaving errors = %d\n", errors) ;

  errors = 0 ;
  t0 = elapsed_cycles_fast() ;
  for(j=0 ; j<65536 ; j+=3){
    for(i=0 ; i<65536 ; i+=3){
      k32 = interleave_16_32_c((uint32_t) i, (uint32_t) j) ;
      t   = deinterleave_32_16_bmi2(k32) ;
      if(t.l != i || t.h != j) errors++ ;
    }
  }
  t0 = elapsed_cycles_fast() - t0 ;
  fprintf(stderr, "full range C-bmi2-32 deinterleaving errors = %d, time = %6.2f Gcyles\n", errors, t0*1.0E-9) ;

  errors = 0 ;
  t0 = elapsed_cycles_fast() ;
  for(j=0 ; j<65536 ; j+=3){
    for(i=0 ; i<65536 ; i+=3){
      k32 = interleave_16_32((uint32_t) i, (uint32_t) j) ;
      t   = deinterleave_32_16_c(k32) ;
      if(t.l != i || t.h != j) errors++ ;
    }
  }
  t0 = elapsed_cycles_fast() - t0 ;
  fprintf(stderr, "full range bmi2-32-c deinterleaving errors = %d, time = %6.2f Gcyles\n", errors, t0*1.0E-9) ;

#if defined(__BMI2__)
  errors = 0 ;
  t0 = elapsed_cycles_fast() ;
  for(j=0 ; j<65536 ; j+=3){
    for(i=0 ; i<65536 ; i+=3){
      k64 = interleave_32_64_bmi2((uint32_t) i, (uint32_t) j) ;
      t   = deinterleave_64_32_bmi2(k64) ;
      if(t.l != i || t.h != j) errors++ ;
    }
  }
  t0 = elapsed_cycles_fast() - t0 ;
  fprintf(stderr, "full range BMI2-64   deinterleaving errors = %d, time = %6.2f Gcyles\n", errors, t0*1.0E-9) ;

  errors = 0 ;
  t0 = elapsed_cycles_fast() ;
  for(j=0 ; j<65536 ; j+=3){
    for(i=0 ; i<65536 ; i+=3){
      k32 = interleave_16_32((uint32_t) i, (uint32_t) j) ;
      t   = deinterleave_32_16(k32) ;
      if(t.l != i || t.h != j) errors++ ;
    }
  }
  t0 = elapsed_cycles_fast() - t0 ;
  fprintf(stderr, "full range BMI2-32   deinterleaving errors = %d, time = %6.2f Gcyles\n", errors, t0*1.0E-9) ;
#endif
}
void test_rle(char *msg){
  uint32_t i, j, nbits ;
  int32_t nbytes, count ;
  char cbuf[33] ;
  bitstream *bstream ;
  size_t bsize = 4096 ;
  uint8_t bytes[4096] ;

  bstream = StreamCreate(bsize, BIT_INSERT | BIT_XTRACT | SET_BIG_ENDIAN) ;
  j = 1 ;
  for(i=0 ; i<8 ; i++, j*=16){
    BinaryToString(&j, cbuf, 32) ;
    fprintf(stderr, "%10u = 0B%s 0X%8.8x\n", j, cbuf, j) ;
  }
  nbits = 0 ;
  for(i=1 ; i<9 ; i*=2){
    uint8_t byte = 0 ;
    uint32_t repcount = i ;
    nbits += RleEncodeByte(byte, repcount, bstream) ;
  }
  nbits += RleEncodeByte(127, 3, bstream) ;
  for(i=1 ; i<9 ; i*=2){
    uint8_t byte = 0xFFu ;
    uint32_t repcount = i ;
    nbits += RleEncodeByte(byte, repcount, bstream) ;
  }
  fprintf(stderr, "nbits total    = %4d\n", nbits) ;
//   fprintf(stderr, "bits in stream = %4ld\n", STREAM_BITS_USED(*bstream)) ;
  StreamFlush(bstream) ;
  StreamRewind(bstream, 1) ;
BinaryToString(bstream->first, cbuf, 32) ;
fprintf(stderr, "0b%s\n", cbuf) ;
BinaryToString((bstream->first) + 1, cbuf, 32) ;
fprintf(stderr, "0b%s\n", cbuf) ;
  fprintf(stderr, "bits in stream = %4ld\n", StreamAvailableBits(bstream)) ;
  nbytes = 0 ;
  for(i=0 ; i<9 ; i++){
    count = RleDecodeByte(bytes + nbytes, bstream) ;
    nbytes += count ;
//     fprintf(stderr, "bits in stream = %4ld\n", StreamAvailableBits(bstream)) ;
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
  if(argc == 3 && atoi(argv[1]) == 3){
    test_bit_mask_field(argv[2]) ;
    return 0 ;
  }
  if(argc == 3 && atoi(argv[1]) == 4){
    test_interleave(argv[2]) ;
    return 0 ;
  }
  if(argc == 3 && atoi(argv[1]) == 5){
    test_rle(argv[2]) ;
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
