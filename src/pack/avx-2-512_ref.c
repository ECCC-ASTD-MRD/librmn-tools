
#include <immintrin.h>
#include <stdint.h>
#include <stdio.h>

void *ExpandLoad32(void *what, uint32_t mask, void *compressed, uint32_t ref){
  uint32_t *dest = (uint32_t *) what ;
  uint32_t *w32 = (uint32_t *) compressed ;
#if defined(__AVX512F__)
  __m512i vr0 = _mm512_set1_epi32(ref) ;
  __m512i vd0 = _mm512_mask_expandloadu_epi32 (vr0, mask & 0xFFFF, w32) ;
  _mm512_mask_storeu_epi32 (dest, 0xFFFF, vd0) ;
  w32 +=  _mm_popcnt_u32(mask & 0xFFFF) ;
  __m512i vd1 = _mm512_mask_expandloadu_epi32 (vr0, mask >> 16, w32) ;
  _mm512_mask_storeu_epi32 (dest+16, 0xFFFF, vd1) ;
  w32 += _mm_popcnt_u32(mask >> 16) ;
#else
  int i ;
  for(i = 0 ; i < 32 ; i++, mask >>=1){
    *dest = *w32 ;
    w32 += (mask & 1) ;
  }
#endif
  return w32 ;
}

void *ExpandLoad31(void *what, int nw32, uint32_t mask, void *compressed, uint32_t ref){
  uint32_t *dest = (uint32_t *) what ;
  uint32_t *w32 = (uint32_t *) compressed ;
  int i ;
  if(nw32 > 31) return NULL ;
  for(i = 0 ; i < nw32 ; i++, mask >>=1){
    *dest = (mask & 1) ? *w32 : ref ;
    w32 += (mask & 1) ;
  }
  return w32 ;
}

void *ExpandLoad(void *what, int nw32, void *mk, void *compressed, const void *value){
  uint32_t *dest = (uint32_t *) what ;
  uint32_t *mask = (uint32_t *) mk ;
  uint32_t *w32 = (uint32_t *) compressed ;
  uint32_t *vref = (uint32_t *) value ;
  uint32_t ref = *vref ;
  int i0 ;
  for(i0 = 0 ; i0 < nw32 - 31 ; i0 += 32){
    w32 = ExpandLoad32(dest+i0, *mask, w32, ref) ;
    mask++ ;
  }
  w32 = ExpandLoad31(dest + i0, nw32 - i0, *mask, w32, ref) ;
  return w32 ;
}

void *CompressStore32(void *what, uint32_t mask, void *compressed){
  uint32_t *w32 = (uint32_t *) what ;
  uint32_t *dest = (uint32_t *) compressed ;
#if defined(__AVX512F__)
  __m512i vd0 = _mm512_loadu_epi32(w32) ;
  __m512i vd1 = _mm512_loadu_epi32(w32+16) ;
  _mm512_mask_compressstoreu_epi32 (dest, mask & 0xFFFF, vd0) ;
  dest += _mm_popcnt_u32(mask & 0xFFFF) ;
  _mm512_mask_compressstoreu_epi32 (dest, mask >> 16,  vd1) ;
  dest += _mm_popcnt_u32(mask >> 16) ;
#else
  int i ;
  for(i = 0 ; i < 32 ; i++, mask >>=1){
    *dest = w32[i] ;
    dest += (mask & 1) ;
  }
#endif
return dest ;
}

void *CompressStore31(void *what, int nw32, uint32_t mask, void *compressed){
  uint32_t *w32 = (uint32_t *) what ;
  uint32_t *dest = (uint32_t *) compressed ;
  int i ;
  if(nw32 > 31) return NULL ;
  for(i = 0 ; i < nw32 ; i++, mask >>=1){
    *dest = w32[i] ;
    dest += (mask & 1) ;
  }
  return dest ;
}

void *CompressStore(void *what, int nw32, void *mk, void *compressed){
  int i0 ;
  uint32_t *w32 = (uint32_t *) what ;
  uint32_t *mask = (uint32_t *) mk ;
  uint32_t *dest = (uint32_t *) compressed ;
  for(i0 = 0 ; i0 < nw32 - 31 ; i0 += 32){
    dest = CompressStore32(w32 + i0, *mask, dest) ;
    mask++ ;
  }
  dest = CompressStore31(w32 + i0, nw32 - i0, *mask, dest) ;
//   uint32_t bits = *mask ;
//   for(i=i0 ; i<nw32 ; i++, bits >>= 1){
//     *dest = w32[i] ;
//     dest += (bits & 1) ;
//   }
  return dest ;
}

// mask   [OUT] : 1s where what[i] != reference value, 0s otherwise
static int32_t CompressSpecialValue(void *what, int nw32, const void *value, void *mask, void *compressed){
  int32_t nval = 0, i, i0, j ;
  uint32_t *w32 = (uint32_t *) what ;
  uint32_t *vref = (uint32_t *) value ;
  uint32_t *mk = (uint32_t *) mask ;
  uint32_t *dest = (uint32_t *) compressed ;
  uint32_t mask0, bits ;
  uint32_t ref = *vref ;

#if defined(__AVX512F__)
  __m512i vr0 = _mm512_set1_epi32(ref) ;
// fprintf(stderr, "AVX512 code : 2 subslices of 16 values\n");
#elif defined(__AVX2__)
  __m256i vr0 = _mm256_set1_epi32(ref) ;
// fprintf(stderr, "AVX2 code : 4 subslices of 8 values\n");
#else
// fprintf(stderr, "plain C code : 1 slice of 32 values\n");
#endif

  for(i0 = 0 ; i0 < nw32 - 31 ; i0 += 32){   // 32 values per slice
#if defined(__AVX512F__)
//   AVX512 code : 2 subslices of 16 values
  __m512i vd0, vd1 ;
  __mmask16 mk0, mk1 ;
  uint32_t pop0, pop1 ;
  vd0 = _mm512_loadu_epi32(w32 + i0) ;
  vd1 = _mm512_loadu_epi32(w32 + i0 + 16) ;
  mk0 = _mm512_mask_cmp_epu32_mask (0xFFFF, vr0, vd0, _MM_CMPINT_NE) ;
  _mm512_mask_compressstoreu_epi32 (dest, mk0, vd0) ;          // store non special values
  pop0 = _mm_popcnt_u32(mk0) ;
  dest += pop0 ; nval += pop0 ;
  mk1 = _mm512_mask_cmp_epu32_mask (0xFFFF, vr0, vd1, _MM_CMPINT_NE) ;
  _mm512_mask_compressstoreu_epi32 (dest, mk1, vd1) ;          // store non special values
  pop1 = _mm_popcnt_u32(mk1) ;
  dest += pop1 ; nval += pop1 ;
  mask0 = mk1 ;
  mask0 = (mask0 << 16) | mk0 ;
#elif defined(__AVX2__)
//   AVX2 code : 4 subslices of 8 values, probably better to use the plain C code, because of the compressed store
  __m256i vd0, vd1, vd2, vd3, vs0, vc0, vc1, vc2, vc3 ;
  uint32_t pop ;
  vd0 = _mm256_loadu_si256((__m256i *)(w32 + i0     )) ;
  vd1 = _mm256_loadu_si256((__m256i *)(w32 + i0 +  8)) ;
  vd2 = _mm256_loadu_si256((__m256i *)(w32 + i0 + 16)) ;
  vd3 = _mm256_loadu_si256((__m256i *)(w32 + i0 + 24)) ;
  vc3 = _mm256_cmpeq_epi32(vd3, vr0) ; mask0  = _mm256_movemask_ps((__m256) vc3) ; mask0 <<= 8 ;
  vc2 = _mm256_cmpeq_epi32(vd2, vr0) ; mask0 |= _mm256_movemask_ps((__m256) vc2) ; mask0 <<= 8 ;
  vc1 = _mm256_cmpeq_epi32(vd1, vr0) ; mask0 |= _mm256_movemask_ps((__m256) vc1) ; mask0 <<= 8 ;
  vc0 = _mm256_cmpeq_epi32(vd0, vr0) ; mask0 |= _mm256_movemask_ps((__m256) vc0) ; mask0 = (~mask0) ;
  pop = _mm_popcnt_u32(mask0) ;
  nval += pop ;
  bits = mask0 ;
  for(j=0 ; j<32 ; j++, bits >>= 1){
    *dest = w32[i0 + j] ;
    dest += (bits & 1) ;
  }
#else
//   plain C code
  bits = 1 ; mask0 = 0 ;
  for(i = 0 ; i < 32 ; i++){
    int onoff = (w32[i+i0] != ref) ? 1 : 0 ;
    mask0 |= (onoff << i) ;
    *dest = w32[i+i0] ;
    dest += onoff ;
    nval += onoff ;
  }
#endif

  *mk = mask0 ; mk++ ;
  }
  bits = 1 ; mask0 = 0 ;
  for(i = i0 ; i < nw32 ; i++){
    if(w32[i] != ref){
      mask0 |= bits ;
      *dest = w32[i] ; dest++ ;
      nval++ ;
    }
    bits <<= 1 ;
  }
  *mk = mask0 ;
  return nval ;
}

#if defined(TEST)
#include <stdio.h>

void print_int_values(int *values, int nvalues, char *fmt){
  int i ;
  for(i=0 ; i<nvalues ; i++) fprintf(stderr, fmt, values[i]) ;
}

#define NDATA 69
int main(int argc, char **argv){
  int values[NDATA], i, nval, ref = 99 ;
  int cval[NDATA] ;
  int bmap[NDATA] ;

  for(i=0 ; i<NDATA ; i++) { values[i] = i ; cval[i] = -1 ; }
  for(i=0 ; i<NDATA ; i+=8) values[i] = ref ;

  nval = CompressSpecialValue(values, NDATA, &ref, bmap, cval) ;
  fprintf(stderr, "ndata = %d, nval = %d\n", NDATA, nval) ;
  print_int_values(bmap, (NDATA+31)/32, "%8.8x ") ; fprintf(stderr, "\n");
  print_int_values(cval, nval, "%3d") ; fprintf(stderr, "\n");
}
#endif
