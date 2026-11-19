// Hopefully useful functions for C and FORTRAN
// Copyright (C) 2026  Environnement Canada
//
// This code is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation,
// version 2.1 of the License.
//
// This code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// #define USE_C_CODE
#if defined(__x86_64__) && ! defined(USE_C_CODE)
#include <immintrin.h>
#else
#undef __AVX2__
#undef __AVX512F__
#endif

#include <rmn/fst_mask.h>

#define STATIC static

// build 8 bit mask from 8 values (1 where s[x] == value, 0 elsewhere
// s     [IN] : source array
// value [IN] : special value
// return 8 bit mask (LittleEndian style) in lower 8 bits of 32 bit integer
// LSB is associated with s[0]
STATIC  uint32_t mask32_eq_w08(uint32_t *s, uint32_t value){
#if defined(__AVX2__)
  __m256i s8 = _mm256_loadu_si256 ((__m256i const *)s) ;      // 8 values slice
  __m256i v8 = _mm256_set1_epi32(value) ;                     // special value
  __m256i t8 = _mm256_cmpeq_epi32(s8, v8) ;                   // s[] == value
  int32_t m8 = _mm256_movemask_ps((__m256)t8) ;               // 8 bits mask
#else
  uint32_t m8 = 0 ;
  for(int i=0 ; i<8 ; i++){                                   // 1 bit at a time
    m8 |= ( (s[i] == value) ? (1 << i) : 0 ) ;
  }
#endif
  return m8 ;
}

#if defined(WITH_FLOAT_MASKS)
// build 8 bit mask from 8 float values (1 where s[x] > value, 0 elsewhere
STATIC  uint32_t mask32_gt_f08(float *s, float value){
#if defined(__AVX2__)
  __m256 s8 = _mm256_loadu_ps (s) ;                           // 8 values slice
  __m256 v8 = _mm256_set1_ps(value) ;                         // special value
  __m256 t8 = _mm256_cmp_ps(s8, v8, _CMP_GT_OQ) ;             // s[] > value
  int32_t m8 = _mm256_movemask_ps(t8) ;                       // 8 bits mask
#else
  uint32_t m8 = 0 ;
  for(int i=0 ; i<8 ; i++){                                   // 1 bit at a time
    m8 |= ( (s[i] > value) ? (1 << i) : 0 ) ;
  }
#endif
  return m8 ;
}
// build 8 bit mask from 8 float values (1 where s[x] > value, 0 elsewhere
STATIC  uint32_t mask32_lt_f08(float *s, float value){
#if defined(__AVX2__)
  __m256 s8 = _mm256_loadu_ps (s) ;                           // 8 values slice
  __m256 v8 = _mm256_set1_ps(value) ;                         // special value
  __m256 t8 = _mm256_cmp_ps(v8, s8, _CMP_GT_OQ) ;             // s[] < value (value > s[])
  int32_t m8 = _mm256_movemask_ps(t8) ;                       // 8 bits mask
#else
  uint32_t m8 = 0 ;
  for(int i=0 ; i<8 ; i++){                                   // 1 bit at a time
    m8 |= ( (s[i] > value) ? (1 << i) : 0 ) ;
  }
#endif
  return m8 ;
}
#endif

// for now only used on AVX512 capable CPUs
#if defined(__AVX512F__)
// build 16 bit mask from 16 values (1 where s[x] == value, 0 elsewhere
// s     [IN] : source array
// value [IN] : special value
// return 16 bit mask (LittleEndian style) in lower 16 bits of 32 bit integer
// LSB is associated with s[0]
STATIC uint32_t mask32_eq_w16(uint32_t *s, uint32_t value){
  uint32_t m16 ;
#if defined(__AVX512F__)
  __mmask16 m0 ;
  __m512i v0 = _mm512_loadu_epi32(s) ;      // 16 values slice
  __m512i vc = _mm512_set1_epi32(value) ;   // special value
  m0 = _mm512_cmpeq_epi32_mask(v0, vc) ;    // 16 bits mask ( s[] == value )
  m16 = m0 ;
#else
  int32_t m0 = mask32_eq_w08(s     , value) ;  // 8 values slice
  int32_t m1 = mask32_eq_w08(s +  8, value) ;  // 8 values slice
  m16 = ( m0 | (m1 << 8) ) ;
#endif
  return m16 ;
}
#endif

#if defined(WITH_FLOAT_MASKS)
// build 16 bit mask from 16 float values (1 where s[x] > value, 0 elsewhere
STATIC uint32_t mask32_gt_f16(float *s, float value){
  uint32_t m16 ;
#if defined(__AVX512F__)
  __mmask16 m0 ;
  __m512 v0 = _mm512_loadu_ps(s) ;          // 16 values slice
  __m512 vc = _mm512_set1_ps(value) ;       // special value
  m0 = _mm512_cmplt_ps_mask(vc, v0) ;       // 16 bits mask s[] > value (value < s[] )
  m16 = m0 ;
#else
  int32_t m0 = mask32_gt_f08(s     , value) ;  // 8 values slice
  int32_t m1 = mask32_gt_f08(s +  8, value) ;  // 8 values slice
  m16 = ( m0 | (m1 << 8) ) ;
#endif
  return m16 ;
}
// build 16 bit mask from 16 float values (1 where s[x] < value, 0 elsewhere
STATIC uint32_t mask32_lt_f16(float *s, float value){
  uint32_t m16 ;
#if defined(__AVX512F__)
  __mmask16 m0 ;
  __m512 v0 = _mm512_loadu_ps(s) ;          // 16 values slice
  __m512 vc = _mm512_set1_ps(value) ;       // special value
  m0 = _mm512_cmplt_ps_mask(v0, vc) ;       // 16 bits mask ( s[] < value )
  m16 = m0 ;
#else
  int32_t m0 = mask32_lt_f08(s     , value) ;  // 8 values slice
  int32_t m1 = mask32_lt_f08(s +  8, value) ;  // 8 values slice
  m16 = ( m0 | (m1 << 8) ) ;
#endif
  return m16 ;
}
#endif

// build 32 bit mask from 32 values (1 where s[x] == value, 0 elsewhere
// s     [IN] : source array
// value [IN] : special value
// return 32 bit mask (LittleEndian style) in 32 bit integer
// LSB is associated with s[0]
STATIC uint32_t mask32_eq_w32(uint32_t *s, uint32_t value){
#if defined(__AVX512F__)
  // 16 value slices if AVX512 is available
  int32_t m0 = mask32_eq_w16(s     , value) ;
  int32_t m1 = mask32_eq_w16(s + 16, value) ;
  return ( m0 | (m1 << 16) ) ;
#else
  // 8 value slices otherwise
  int32_t m0 = mask32_eq_w08(s     , value) ;
  int32_t m1 = mask32_eq_w08(s +  8, value) ;
  int32_t m2 = mask32_eq_w08(s + 16, value) ;
  int32_t m3 = mask32_eq_w08(s + 24, value) ;
  return ( m0 | (m1 << 8) | (m2 << 16) | (m3 << 24) ) ;
#endif
}

#if defined(WITH_FLOAT_MASKS)
// build 32 bit mask from 32 float values (1 where s[x] > value, 0 elsewhere
STATIC uint32_t mask32_gt_f32(float *s, float value){
#if defined(__AVX512F__)
  // 16 value slices if AVX512 is available
  int32_t m0 = mask32_gt_f16(s     , value) ;
  int32_t m1 = mask32_gt_f16(s + 16, value) ;
  return ( m0 | (m1 << 16) ) ;
#else
  // 8 value slices otherwise
  int32_t m0 = mask32_gt_f08(s     , value) ;
  int32_t m1 = mask32_gt_f08(s +  8, value) ;
  int32_t m2 = mask32_gt_f08(s + 16, value) ;
  int32_t m3 = mask32_gt_f08(s + 24, value) ;
  return ( m0 | (m1 << 8) | (m2 << 16) | (m3 << 24) ) ;
#endif
}
// build 32 bit mask from 32 float values (1 where s[x] < value, 0 elsewhere
STATIC uint32_t mask32_lt_f32(float *s, float value){
#if defined(__AVX512F__)
  // 16 value slices if AVX512 is available
  int32_t m0 = mask32_lt_f16(s     , value) ;
  int32_t m1 = mask32_lt_f16(s + 16, value) ;
  return ( m0 | (m1 << 16) ) ;
#else
  // 8 value slices otherwise
  int32_t m0 = mask32_lt_f08(s     , value) ;
  int32_t m1 = mask32_lt_f08(s +  8, value) ;
  int32_t m2 = mask32_lt_f08(s + 16, value) ;
  int32_t m3 = mask32_lt_f08(s + 24, value) ;
  return ( m0 | (m1 << 8) | (m2 << 16) | (m3 << 24) ) ;
#endif
}
// build 32 bit mask from 32 float values (1 where s[x] >= value, 0 elsewhere
STATIC uint32_t mask32_ge_f32(float *s, float value){
  return (~ mask32_lt_f32(s, value)) ;   // >= ==> NOT <
}
// build 32 bit mask from 32 float values (1 where s[x] <= value, 0 elsewhere
STATIC uint32_t mask32_le_f32(float *s, float value){
  return (~ mask32_gt_f32(s, value)) ;   // <= ==> NOT >
}
#endif

// build mask array from n values (1 where s[x] == value, 0 elsewhere
// where s[0..n-1] == value, set bit i in mask to 1
// m    [OUT] : result 32 bit mask array
// s     [IN] : source array
// value [IN] : special value
// n     [IN] : number of values in source array
// opt   [IN] : options (see fst_mask.h)
// return number of values
// LSB is associated with s[0]
int32_t mask32_eq(uint32_t * restrict m, uint32_t * restrict s, uint32_t value, int n, int opt){
  (void) (opt) ;
  uint32_t negate = (opt & INVERT_MASK) ? (~0) : 0 ;
  int32_t n0 = 0 ;
  while(n > 31){                    // blocks of 32 values
    *m = mask32_eq_w32(s, value) ^ negate ;
    n0 += _mm_popcnt_u32(*m) ;
    s += 32 ; m++ ; n -= 32 ;
  }
  if(n > 0){                        // up to 31 values left
    uint32_t mask = 0 ;
    for(int i=0 ; i<31 ; i++){
      if(i == n) break ;
      if(s[i] == value){
        mask |= (1 << i) ;
      }
    }
    *m = mask ^ negate ;
    n0 += _mm_popcnt_u32(*m) ;
  }
  return n0 ;
}

// ======================================= compress and mask =======================================

// where s[0..31] != value, copy s[i] into destination array d
// d    [OUT] : destination array
// m    [OUT] : result 32 bit mask
// s     [IN] : source array
// value [IN] : special value
// return number of values copied from s into d, set m[0] to 32 bit mask value
STATIC int32_t compress_mask32_w32(uint32_t * restrict d, uint32_t * restrict m, uint32_t * restrict s, uint32_t value){
  uint32_t mask = mask32_eq_w32(s, value), count = 0 ;  // build 32 bit mask
  *m = mask ;
  if(mask == 0xFFFFFFFF) return 0 ;

  for(int i=0 ; i<32 ; i++){
    if( (mask & (1 << i)) == 0 ) { *d = s[i] ; d++ ; count++ ; }
  }
  return count ;
}
// special case of above if n < 32 (n == 32 O.K.)
STATIC int32_t compress_mask32_w31(uint32_t * restrict d, uint32_t * restrict m, uint32_t * restrict s, uint32_t value, int n){
  if(n == 0) return 0 ;                                  // nothing to do
  uint32_t mask = mask32_eq_w32(s, value), count = 0 ;  // build <= 32 bit mask
  *m = mask ;
  if( mask == (0xFFFFFFFF >> (32-n)) ) return 0 ;        // all relevant values == value

  for(int i=0 ; i<32 ; i++){
    if(i == n) break ;
    if( (mask & (1 << i)) == 0 ) { *d = s[i] ; d++ ; count++ ; }
  }
  return count ;
}

// where s[0..n-1] != value, copy s[i] into destination array d
// d    [OUT] : destination array
// m    [OUT] : result 32 bit mask array
// s     [IN] : source array
// value [IN] : special value
// n     [IN] : number of values in source array
// return number of values copied from s into d
// this function should work "in place" as destination array is at worst the same length as the source array
int32_t compress_mask32_w(uint32_t * restrict d, uint32_t * restrict m, uint32_t * restrict s, uint32_t value, int n, int opt){
  (void) (opt) ;
  int32_t total = 0 ;
  while(n > 31){                    // blocks of 32 values
    int32_t count = compress_mask32_w32(d, m, s, value) ;
    s += 32 ; d += count ; m++ ; n -= 32 ; total += count ;
  }
  if(n > 0){
    total += compress_mask32_w31(d, m, s, value, n) ;
  }
  return total ;
}

void print_mask32_2D(FILE *f, char *header, uint32_t *m, int ni, int nj){
  uint8_t mask[nj][ni] ;
  int col = 0, row = 0, ninj = ni*nj ;
  while(ninj > 0){
    for(int i=0 ; i<32 && i<ninj ; i++) {
      mask[row][col] = ((m[0] >> i) & 1) ;
      col++ ;
      if(col == ni){row++ ; col = 0; }
    }
    m++ ; ninj -= 32 ;
  }
  for(int j=nj-1 ; j>=0 ; j--){
    fprintf(f, "%s%3d ", header, j) ;
    for(int i=0 ; i<ni ; i++) { fprintf(f, "%1d", mask[j][i] ? 1 : 0) ; }
    fprintf(f, "\n") ;
  }
}

// ======================================= print_mask =======================================

// print mask in big endian style (LSB left)
void print_mask32(FILE *f, char *header, uint32_t *m, int n){
  fprintf(f, "%s0123456789ABCDEF0123456789ABCDEF\n", header) ;
  char bits[32] ;
  while(n > 0){
    fprintf(f, "%s", header) ;
    for(int i=0 ; i<32 ; i++){ bits[i] = ' ' ; }
    for(int i=0 ; i<32 && i<n ; i++) { bits[31-i] = ((m[0] >> i) & 1) ? '1' : '0' ; }
    for(int i=0 ; i<32        ; i++) { fprintf(f, "%c", bits[31-i]) ; } ;
    fprintf(f, " %8.8x\n", m[0]) ;
    n -= 32 ; m++ ;
  }
}

// ======================================= compress_store =======================================

// ZEN4 has a very slow _mm512_mask_compressstoreu_epi32, use _mm512_maskz_compress_epi32
// this should also be O.K. performance wise for Intel CPUs
#define ZEN4_SAFE

// this function may store up to 16 extra 32 bit words in dst in ZEN4_SAFE mode
// but dst should be large enough anyway as all 16 words could have to be stored
// copy from src[i] into dst[] where corresponding mask bit is 1
// dst  [OUT] : output array
// mask  [IN] : mask array
// src   [IN] : source array
// return number of useful values stored into dst
// mask bits are LittleEndian style, bit 0 is LSB
static int32_t compress_store_32(uint32_t * restrict dst, uint32_t mask, uint32_t * restrict src) {
//   if(mask == 0) return 0 ;
#if defined(__AVX512F__)
  __mmask16 m0 = mask & 0xFFFF, m1 = (mask >> 16) ;
  __m512i v0, v1 ;
  v0 = _mm512_loadu_epi32 (src) ;
  v1 = _mm512_loadu_epi32 (src + 16) ;
#if defined(ZEN4_SAFE)
  v0 = _mm512_maskz_compress_epi32(m0, v0) ;             // much faster on ZEN4, O.K. on Intel
  _mm512_storeu_epi32 (dst, v0) ;                        // may store up to 16 extra elements in dst
#else
  _mm512_mask_compressstoreu_epi32 (dst, m0, v0) ;       // very slow on ZEN4, O.K. on Intel
#endif
  dst += _mm_popcnt_u32(m0) ;
#if defined(ZEN4_SAFE)
  v1 = _mm512_maskz_compress_epi32(m1, v1) ;             // much faster on ZEN4, O.K. on Intel
  _mm512_storeu_epi32 (dst, v1) ;                        // may store up to 16 extra elements in dst
#else
  _mm512_mask_compressstoreu_epi32 (dst, m1, v1) ;       // very slow on ZEN4, O.K. on Intel
#endif
  return _mm_popcnt_u32(mask) ;
#else
//   if(mask == 0xFFFFFFFF) { for(int i=0 ; i<32 ; i++) { dst[i] = src[i] ; } ; return 32 ; } ;
  uint32_t* dst0 = dst ;
  for(int i=0 ; i<32 ; i++){
    if(mask & (1 << i)) *dst++ = src[i] ;
  }
  return (dst - dst0) ;
#endif
}
// copy from src[i] into dst[] where mask bit is 1
// variant of above if n <= 32
// dst  [OUT] : output array
// mask  [IN] : mask array
// src   [IN] : source array
// return number of useful values stored into dst
// mask bits are LittleEndian style, bit 0 is LSB
static int32_t compress_store_31(uint32_t* restrict dst, uint32_t mask, uint32_t* restrict src, int n) {
  if(n < 0 || n > 32) return -1 ;
  uint32_t* dst0 = dst ;
  for(int i=0 ; i<32 ; i++){
    if(i == n) break ;
    if(mask & (1 << i)) *dst++ = src[i] ;
  }
  return (dst - dst0) ;
}

// copy from src[i] into dst[] where mask bit is 1
// dst  [OUT] : output array
// mask  [IN] : mask array
// src   [IN] : source array
// n     [IN] : number of values in src
// return number of useful values stored into dst
// mask bits are LittleEndian style, bit 0 is LSB
int32_t compress_store(uint32_t* restrict dst, uint32_t *mask, uint32_t* restrict src, int n) {
  uint32_t* dst0 = dst ;
  while(n > 31){
    dst += compress_store_32(dst, mask[0], src) ;
    src += 32 ; mask++ ; n -=32 ;
  }
  if(n > 0){
    dst += compress_store_31(dst, mask[0], src, n) ;
  }
  return (dst - dst0) ;
}

// ======================================= expand_load =======================================

// dst  [OUT] : output array
// mask  [IN] : mask array
// src   [IN] : source array
// n     [IN] : number of values in src
// return number of values copied from src
// mask bits are LittleEndian style, bit 0 is LSB
static int32_t expand_load_32(uint32_t* restrict dst, uint32_t mask, uint32_t* restrict src, uint32_t value) {
#if defined(__AVX512F__)
  __m512i v0, v1, vc ;
  __mmask16 m0 = mask & 0xFFFF, m1 = mask >> 16 ;
  v0 = _mm512_maskz_expandloadu_epi32 (m0, src) ;
  vc = _mm512_set1_epi32(value) ;
  v1 = _mm512_mask_mov_epi32 (vc, m0, v0) ;
  _mm512_storeu_epi32 (dst, v1) ;
  src += _mm_popcnt_u32(m0) ;
  dst += 16 ;
  v0 = _mm512_maskz_expandloadu_epi32 (m1, src) ;
  v1 = _mm512_mask_mov_epi32 (vc, m1, v0) ;
  _mm512_storeu_epi32 (dst, v1) ;
  return _mm_popcnt_u32(mask) ;
#else
  uint32_t* src0 = src ;
  for(int i=0 ; i<32 ; i++){
    if(mask & (1 << i)){
      dst[i] = *src ; src++ ;
    }else{
      dst[i] = value ;
    }
  }
  return (src - src0) ;
#endif
}
// variant of above where n <= 32
static int32_t expand_load_31(uint32_t* restrict dst, uint32_t mask, uint32_t* restrict src, uint32_t value, int n) {
  if(n < 0 || n > 32) return -1 ;
  if(n == 0) return 0 ;             // nothing to do
  uint32_t* src0 = src ;
  for(int i=0 ; i<32 ; i++){
    if(i == n) break ;
    if(mask & (1 << i)){
      dst[i] = *src ; src++ ;
    }else{
      dst[i] = value ;
    }
  }
  return (src - src0) ;
}

int32_t expand_load(uint32_t* restrict dst, uint32_t *mask, uint32_t* restrict src, uint32_t value, int n) {
  uint32_t* src0 = src ;
  while(n > 31){
    src += expand_load_32(dst, mask[0], src, value) ;
    dst += 32 ;
    mask++ ;
    n -=32 ;
  }
  if(n > 0){
    src += expand_load_31(dst, mask[0], src, value, n) ;
  }
  return (src - src0) ;
}

// ======================================= RLE encoders =======================================

static int32_t micro  = 0, small  = 0, large  = 0, xlarge  = 0, arb  = 0 ;
static int32_t micro_ = 0, small_ = 0, large_ = 0, xlarge_ = 0 ;
static int32_t token_no = 0 ;

// #define DEBUG_STATS(CODE)  CODE
#define DEBUG_STATS(CODE)

void rle_stats(char *msg){
  if(token_no == 0) return ;
  fprintf(stderr, "%s : counts = %6d %6d %6d %6d %6d, pop = %6d %6d %6d %6d, total = %d, tokens = %d\n",
          msg, micro, small, large, xlarge, arb, micro_, small_, large_, xlarge_, micro_+small_+large_+xlarge_+arb, token_no) ;
  micro  = small  = large  = xlarge   = arb  = 0 ;
  micro_ = small_ = large_ = xlarge_  = 0 ;
  token_no = 0 ;
}

// partial encoding of a single byte repeat count
// byte  [IN] : byte to be repeated
// count [IN] : repeat count (up to 
// return byte/encoded token/length in bits of encoded token/residual repeat count
// in a rle_token struct
// in case of error, all members of the returned struc will be 0
static rle_token _set_rle8_token_(uint8_t byte, uint32_t count){
  rle_token r = (rle_token) { 0, {0}, 0, 0} ;   // NULL TOKEN
  if(count == 0)     goto end ;       // ERROR : zero count
  if(count > 0xFFFF) goto end ;       // ERROR : count cannot use more than 16 bits
  r.byte = byte ;
  if(byte != 0x00 && byte != 0xFF){   // byte not eligible for repeat count
    r.code = byte ;                   // 0bbyyttee : 0 bit followed by byte
    r.bits = 9 ;                      // token needs 9 bits
    r.left = count - 1 ;              // count must be 1 for byte that cannot be repeated
DEBUG_STATS(arb++ ;)
    goto end ;
  }
// codes 000000000b or 011111111 will never be returned (used for special functions)
// 1010b or 1011b would be the returned code
  if(count < RLE_SMALL_OFFSET){           //  4 bit micro RLE count (1 - 2)
    r.code = ((byte == 0) ? RLE_MICRO_0 : RLE_MICRO_1) + (count - RLE_MICRO_OFFSET) ;
    r.bits = 4 ;
DEBUG_STATS(micro++ ; micro_ += count ;)
  }else if(count < RLE_LARGE_OFFSET){     //  8 bit small RLE count (3 - 18)
    r.code = ((byte == 0) ? RLE_SMALL_0 : RLE_SMALL_1) + (count - RLE_SMALL_OFFSET) ;
    r.bits = 8 ;
DEBUG_STATS(small++ ; small_ += count ;)
  }else if(count <= RLE_MAX_COUNT){       // 12 bit large RLE count (19 - 274)
    r.code = ((byte == 0) ? RLE_LARGE_0 : RLE_LARGE_1) + (count - RLE_LARGE_OFFSET) ;
    r.bits = 12 ;
DEBUG_STATS(large++ ; large_ += count ;)
  }else{                                  // > RLE_MAX_COUNT,  12 bit large RLE count (RLE_MAX_COUNT)
    r.code = ((byte == 0) ? RLE_LARGE_0 : RLE_LARGE_1) + (RLE_MAX_COUNT - RLE_LARGE_OFFSET) ;
    r.bits = 12 ;
    r.left = count - (RLE_MAX_COUNT) ;    // we have leftovers
DEBUG_STATS(xlarge++ ; xlarge_ += RLE_MAX_COUNT ;)
  }
end:
  return r ;
}
// partial encoding of a single nibble (4 bits) repeat count
static rle_token _set_rle4_token_(uint8_t byte, uint32_t count){
  rle_token r = (rle_token) { 0, {0}, 0, 0} ;   // NULL TOKEN
  if(count == 0)     goto end ;       // ERROR : zero count
  if(count > 0xFFFF) goto end ;       // ERROR : count cannot use more than 16 bits
  r.byte = byte ;
  if(byte != 0x0 && byte != 0xF){     // byte not eligible for repeat count
    r.code = byte ;                   // 0nibl : 0 bit followed by nibble
    r.bits = 5 ;                      // token needs 5 bits
    r.left = count - 1 ;              // count must be 1 for byte that cannot be repeated
DEBUG_STATS(arb++ ;)
    goto end ;
  }
// codes 000000000b or 011111111 will never be returned (used for special functions)
// 1010b or 1011b would be the returned code
  if(count < RLE_SMALL_OFFSET){           //  4 bit micro RLE count (1 - 2)
    r.code = ((byte == 0) ? RLE_MICRO_0 : RLE_MICRO_1) + (count - RLE_MICRO_OFFSET) ;
    r.bits = 4 ;
DEBUG_STATS(micro++ ; micro_ += count ;)
  }else if(count < RLE_LARGE_OFFSET){     //  8 bit small RLE count (3 - 18)
    r.code = ((byte == 0) ? RLE_SMALL_0 : RLE_SMALL_1) + (count - RLE_SMALL_OFFSET) ;
    r.bits = 8 ;
DEBUG_STATS(small++ ; small_ += count ;)
  }else if(count <= RLE_MAX_COUNT){       // 12 bit large RLE count (19 - 274)
    r.code = ((byte == 0) ? RLE_LARGE_0 : RLE_LARGE_1) + (count - RLE_LARGE_OFFSET) ;
    r.bits = 12 ;
DEBUG_STATS(large++ ; large_ += count ;)
  }else{                                  // > RLE_MAX_COUNT,  12 bit large RLE count (RLE_MAX_COUNT)
    r.code = ((byte == 0) ? RLE_LARGE_0 : RLE_LARGE_1) + (RLE_MAX_COUNT - RLE_LARGE_OFFSET) ;
    r.bits = 12 ;
    r.left = count - (RLE_MAX_COUNT) ;    // we have leftovers
DEBUG_STATS(xlarge++ ; xlarge_ += RLE_MAX_COUNT ;)
  }
end:
  return r ;
}

// partial encoding of a single byte repeat count
// byte  [IN] : byte to be repeated
// count [IN] : repeat count (up to 
// return byte/encoded token/length in bits of encoded token/residual repeat count
// in a rle_token struct
// in case of error, all members of the returned struc will be 0
rle_token set_rle8_token(uint8_t byte, uint32_t count){
  return _set_rle8_token_(byte, count) ;
}
rle_token set_rle4_token(uint8_t byte, uint32_t count){
  return _set_rle4_token_(byte, count) ;
}
// partial encoding of a single byte repeat count
// byte      [IN] : byte to be repeated
// count     [IN] : repeat count (MUST BE <= 0xFF)
// stream [INOUT] : bit stream to receive encoded data
// returns the total number of bits used for encoding
static int32_t _put_rle8_count_(uint8_t byte, uint32_t count, bitstream *stream){
  int32_t totbits = 0 ;
  while(count > 0){
    rle_token r = _set_rle8_token_(byte, count) ;
    uint32_t code = r.code ;
    STREAM_PUT_NBITS(*stream, code, r.bits) ;
DEBUG_STATS(token_no++ ;)
    count = r.left ;
    totbits += r.bits ;
  }
  return totbits ;
}
static int32_t _put_rle4_count_(uint8_t byte, uint32_t count, bitstream *stream){
  int32_t totbits = 0 ;
  while(count > 0){
    rle_token r = _set_rle4_token_(byte, count) ;
    uint32_t code = r.code ;
    STREAM_PUT_NBITS(*stream, code, r.bits) ;
DEBUG_STATS(token_no++ ;)
    count = r.left ;
    totbits += r.bits ;
  }
  return totbits ;
}

// encode a single byte repeat count
// byte      [IN] : byte to be repeated
// count     [IN] : repeat count
// stream [INOUT] : bit stream to receive encoded data
// returns the total number of bits used for encoding
// if count == 0, put end of sequence code into stream
int32_t put_rle8_count(uint8_t byte, uint32_t count, bitstream *stream){
  int32_t totbits = 0 ;

  if(count == 0){
    byte =  byte ? 0xFF : 0 ;             // end of sequence byte can only be 0x00 or 0xFF
    STREAM_PUT_NBITS(*stream, byte, 9) ;  // end of sequence code (0 bit followed by 8 bit byte)
    return 9 ;
  }

  while(count > RLE_MAX_COUNT){
    totbits += _put_rle8_count_(byte, RLE_MAX_COUNT, stream) ;
    count -= RLE_MAX_COUNT ;
  }
  if(count > 0) totbits += _put_rle8_count_(byte, count, stream) ;

  return totbits ;
}
int32_t put_rle4_count(uint8_t byte, uint32_t count, bitstream *stream){
  int32_t totbits = 0 ;

  if(count == 0){
    byte =  byte ? 0xF : 0 ;             // end of sequence byte can only be 0x00 or 0xFF
    STREAM_PUT_NBITS(*stream, byte, 5) ;  // end of sequence code (0 bit followed by 8 bit byte)
    return 5 ;
  }

  while(count > RLE_MAX_COUNT){
    totbits += _put_rle4_count_(byte, RLE_MAX_COUNT, stream) ;
    count -= RLE_MAX_COUNT ;
  }
  if(count > 0) totbits += _put_rle4_count_(byte, count, stream) ;

  return totbits ;
}

// encode a mask byte array
// mask      [IN] : mask byte array
// n         [IN] : number of mask bits
// stream [INOUT] : bit stream to receive encoded data
// returns the total number of bits used for encoding
// add end of sequence code into stream
int32_t fst_rle8_encode(uint8_t * restrict mask, int32_t n, bitstream * restrict stream){
  uint32_t old = 0xFFFF ;   // impossible previous byte
  uint32_t count = 0 ;      // no count
  uint32_t totbits = 0 ;
  n = (n+7) / 8 ;
  while(n > 0){
    uint32_t current = mask[0] ;
    if(current == old){     // same as previous byte, bump count
      count++ ;
    }else{
      if(count > 0){        // emit count
        totbits += put_rle8_count(old, count, stream) ;
      }
      old = current ;       // set previous byte to current byte
      count = 1 ;           // reset count
    }
    n-- ;
    mask++ ;                // point to next mask byte
  }
  if(count > 0) totbits += put_rle8_count(old, count, stream) ;
  TERMINATE_RLE8_STREAM(stream) ;
  return totbits ;
}
int32_t fst_rle4_encode(uint8_t * restrict mask, int32_t n, bitstream * restrict stream){
  uint32_t old = 0xFFFF ;   // impossible previous byte
  uint32_t count = 0 ;      // no count
  uint32_t totbits = 0, current ;
  n = (n+3) / 4 ;           // number of nibbles
//   while(n > 0){
  for(int i=0 ; i<n ; i++){
    if(i&1){
      current = mask[0] >> 4 ;     // get upper nibble
      mask++ ;                     // point to next mask byte
    }else{
      current = mask[0] & 0xF ;    // get lower nibble
    }
    if(current == old){     // same as previous byte, bump count
      count++ ;
    }else{
      if(count > 0){        // emit count
        totbits += put_rle4_count(old, count, stream) ;
      }
      old = current ;       // set previous byte to current byte
      count = 1 ;           // reset count
    }
  }
  if(count > 0) totbits += put_rle4_count(old, count, stream) ;
  TERMINATE_RLE4_STREAM(stream) ;
  return totbits ;
}

static int iter = 1 ;

int32_t rle1_count(mask_stream *s, uint32_t mode){
  uint32_t totbits = 0 ;
  mode = mode ? 0xFFFFFFFF : 0 ;
// fprintf(stderr,"[rle1_count %2d] < mode = %8.8x ", iter, mode);
  while(1){
    if(s->avail == 0){
      if(s->out >= s->limit) break ;   // end of mask stream
      s->token = *(s->out) ;
      (s->out)++ ;
      s->avail = 32 ;
    }
    uint32_t target = s->token ^ mode ;
    uint32_t count = (target == 0) ? s->avail : __builtin_ctz(target) ;
// fprintf(stderr,"%d ", count);
    s->avail = s->avail - count ;
    s->token >>= count ;
    totbits += count ;
    if(count == 0 || s->avail > 0) break ;
  }
end:
// fprintf(stderr,">[%d]\n", totbits);
  return totbits ;
}

static void printBits(size_t const size, void const * const ptr)
{
  uint32_t *b = (uint32_t*) ptr;
  uint32_t c = *b ;
  for (int i = size-1; i >= 0; i--)  fprintf(stderr, "%c", (c & (1 << i)) ? '1' : '0') ;
}

static uint32_t fst_rle1_put(uint32_t count, bitstream * restrict stream){
  uint32_t bits = 32 - __builtin_clz((count+1)/2) ;
  uint32_t token1 = ( ~(0xFFFFFFFF << (bits-1)) ) << 1 ;
  uint32_t offset = ~(0xFFFFFFFF << bits) ;
  uint32_t token2 = count - offset ;
  fprintf(stderr,"fst_rle1_put : count = %2d, bits = %2d, offset = %3d ", count, bits * 2, offset) ;
  uint32_t token3 = (token1 << bits) | token2 ;
  printBits(32, &token3) ;
  fprintf(stderr," : ") ;
  printBits(bits, &token1) ;
  fprintf(stderr,"|") ;
  printBits(bits, &token2) ;
  fprintf(stderr,"\n") ;
  STREAM_PUT_NBITS(*stream, token1, bits) ;
  STREAM_PUT_NBITS(*stream, token2, bits) ;
  return bits*2 ;
}

int32_t fst_rle1_encode(uint32_t * restrict mask, int32_t n, bitstream * restrict stream){
  uint32_t totbits = 0, count = 1, avail = 0, token, putbits = 1 ; ;
  n = (n + (8 * sizeof(int32_t)) - 1) / (8 * sizeof(int32_t)) ;
  uint32_t *limit = mask + n, *out = mask ;
  uint32_t target = mask[0] & 1 ;
  STREAM_PUT_NBITS(*stream, target, 1) ;
  uint32_t mode = target ? 0xFFFFFFFF : 0 ;

iter = 0 ;
  while(count > 0){
    count = 0 ;
    while(1){
      if(avail == 0){
        if(out >= limit) break ;
        token = *out ;
        out++ ;
        avail = 32 ;
      }
      target = token ^ mode ;
      uint32_t count0 = (target == 0) ? avail : __builtin_ctz(target) ;
      avail -= count0 ;
      token >>= count0 ;
      count += count0 ;
      if(count0 == 0 || avail > 0) break ;
    }
//     count = rle1_count(&mstream, target) ;
    totbits += count ;
    if(count > 0) putbits += fst_rle1_put(count, stream) ;
iter++ ;
// fprintf(stderr,"fst_rle1_encode[%2d] : mode = %8.8x, count = %3d, totbits = %d\n", iter, mode, count, totbits) ;
//     target = target ^ 1 ;      // invert target bit
    mode = mode ^ 0xFFFFFFFF ;    // invert target
  }
fprintf(stderr,"fst_rle1_encode : putbits = %d\n", putbits) ;
  return totbits ;
}

// ======================================= RLE decoders =======================================

// decode a single byte repeat count
// stream [INOUT] : bit stream containing encoded data
// return rle_token struct
// if an end of sequence code is detected, a count of 0 is returned
rle_token get_rle8_token(bitstream *stream){
  rle_token r = (rle_token) { 0, 0, 0, 0} ;
  uint32_t bits ;
  STREAM_PEEK_NBITS(*stream, bits, 4) ;
  if(0 == (bits >> 3)){                   // byte transmitted "as is" with no repeat count
    STREAM_FAST_GET_NBITS(*stream, bits, 9) ;
    if(bits == 0){
      DEBUG_STATS( fprintf(stderr, "END marker detected\n"); )
      goto end ;
    }
//     if(bits == 0xFF) bits = 0 ;        // 000000000 or 011111111 are equivalent
    r.bits = 9 ;
    r.code = r.byte = bits ;
    r.count = (bits == RLE_END) ? 0 : 1 ;    // count is set to 0 for terminating code
DEBUG_STATS(arb++ ;)
  }else{
    r.byte = (bits & 0x4) ? 0xFF : 0x00 ;  // 11xx is a repeat count for 0xFF, 10xx is a repeat count for 0x00
    bits &= 0x7 ;
    uint32_t token = 0 ;
    switch(bits){
      case 0b010:    // 1010b RLE_MICRO_0
      case 0b011:    // 1011b RLE_MICRO_0
      case 0b100:    // 1100b RLE_MICRO_1
      case 0b101:    // 1101b RLE_MICRO_1
        STREAM_FAST_GET_NBITS(*stream, token, 4) ;
        r.count = 1 + (token & 1) ;
        r.bits = 4 ;
DEBUG_STATS(micro++ ; micro_ += r.count ;)
        break ;
      case 0b001:    // 1001b RLE_SMALL_0
      case 0b110:    // 1110b RLE_SMALL_1
        STREAM_FAST_GET_NBITS(*stream, token, 8) ;
        r.count = 3 + (token & 0xF) ;
        r.bits = 8 ;
DEBUG_STATS(small++ ; small_ += r.count ;)
        break ;
      case 0b000:    // 1000b RLE_LARGE_0
      case 0b111:    // 1111b RLE_LARGE_1
        STREAM_FAST_GET_NBITS(*stream, token, 12) ;
        r.count = 19 + (token & 0xFF) ;
        r.bits = 12 ;
DEBUG_STATS(large++ ; large_ += r.count ;)
        break ;
    }
    r.code = token ;
  }
end:
  return r ;
}
rle_token get_rle4_token(bitstream *stream){
  rle_token r = (rle_token) { 0, 0, 0, 0} ;
  uint32_t bits ;
  STREAM_PEEK_NBITS(*stream, bits, 4) ;
  if(0 == (bits >> 3)){                   // byte transmitted "as is" with no repeat count
    STREAM_FAST_GET_NBITS(*stream, bits, 5) ;
    if(bits == 0){
      DEBUG_STATS( fprintf(stderr, "END marker detected\n"); )
      goto end ;
    }
//     if(bits == 0xFF) bits = 0 ;        // 000000000 or 011111111 are equivalent
    r.bits = 5 ;
    r.code = r.byte = bits ;
    r.count = (bits == RLE_END) ? 0 : 1 ;    // count is set to 0 for terminating code
DEBUG_STATS(arb++ ;)
  }else{
    r.byte = (bits & 0x4) ? 0xF : 0x0 ;    // 11xx is a repeat count for 0xF, 10xx is a repeat count for 0x0
    bits &= 0x7 ;
    uint32_t token = 0 ;
    switch(bits){
      case 0b010:    // 1010b RLE_MICRO_0
      case 0b011:    // 1011b RLE_MICRO_0
      case 0b100:    // 1100b RLE_MICRO_1
      case 0b101:    // 1101b RLE_MICRO_1
        STREAM_FAST_GET_NBITS(*stream, token, 4) ;
        r.count = 1 + (token & 1) ;
        r.bits = 4 ;
DEBUG_STATS(micro++ ; micro_ += r.count ;)
        break ;
      case 0b001:    // 1001b RLE_SMALL_0
      case 0b110:    // 1110b RLE_SMALL_1
        STREAM_FAST_GET_NBITS(*stream, token, 8) ;
        r.count = 3 + (token & 0xF) ;
        r.bits = 8 ;
DEBUG_STATS(small++ ; small_ += r.count ;)
        break ;
      case 0b000:    // 1000b RLE_LARGE_0
      case 0b111:    // 1111b RLE_LARGE_1
        STREAM_FAST_GET_NBITS(*stream, token, 12) ;
        r.count = 19 + (token & 0xFF) ;
        r.bits = 12 ;
DEBUG_STATS(large++ ; large_ += r.count ;)
        break ;
    }
    r.code = token ;
  }
end:
DEBUG_STATS(token_no++ ;)
  return r ;
}

// decode RLE stream
// mask     [OUT] : decoded mask array
// nmax      [IN] : max number of mask bytes
// stream [INOUT] : bit stream representing RLE encoded mask array
// return number of bytes decoded (-1 in case of error)
int32_t fst_rle8_decode(uint8_t * restrict mask, int32_t nmax, bitstream * restrict stream){
  bitstream stream_save = *stream ;          // save stream state
  int32_t count = 0, totbits = 0, nrep ;

  rle_token code = get_rle8_token(stream) ;   // get first token
  totbits = code.bits ;
  // loop until END marker is detected
  while(code.count > 0){                     // count is 0 for END marker
    nrep = code.count ;                      // repeat count for this token
    count += nrep ;                          // bump total count
    if(count > nmax) goto fail ;             // error, excess count
    for(int i=0 ; i<nrep ; i++) {            // copy byte into mask according to repeat count
      mask[i] = code.byte ;
    }
    mask += nrep ;                           // bump mask pointer
    code = get_rle8_token(stream) ;           // get next token
    totbits += code.bits ;
  }
// fprintf(stderr, "fst_rle8_decode : totbits = %d, code.count = %d\n", totbits, code.count) ;
  return count ;                             // total byte count

fail:
// fprintf(stderr, "fst_rle8_decode : totbits = %d, nmax = %d, count = %d, nrep = %d\n", totbits, nmax, count, nrep) ;
  *stream = stream_save ;                    // restore stream state in case of error
  return -1 ;
}
int32_t fst_rle4_decode(uint8_t * restrict mask, int32_t nmax, bitstream * restrict stream){
  bitstream stream_save = *stream ;          // save stream state
  int32_t count = 0, totbits = 0, nrep, low_nibble = 1 ;

  nmax *= 2 ;                                // covert into nibbles (4 bits)
  rle_token code = get_rle4_token(stream) ;  // get first token
  totbits = code.bits ;
  // loop until END marker is detected
  while(code.count > 0){                     // count is 0 for END marker
    nrep = code.count ;                      // repeat count for this token
    count += nrep ;                          // bump total count
    if(count > nmax) goto fail ;             // error, excess count
    for(int i=0 ; i<nrep ; i++) {            // copy byte into mask according to repeat count
      if(low_nibble){
        mask[0] = code.byte & 0xF ;          // set mask value to lower nibble
      }else{
        mask[0] |= ((code.byte & 0xF) << 4) ;       // add upper nibble to previous lower nibble
        mask++ ;
      }
      low_nibble = 1 - low_nibble ;                      // alternate low/high nibble
    }
    code = get_rle4_token(stream) ;          // get next token
    totbits += code.bits ;
  }
// fprintf(stderr, "fst_rle4_decode : totbits = %d, code.count = %d\n", totbits, code.count) ;
  return count ;                             // total byte count

fail:
// fprintf(stderr, "fst_rle4_decode : totbits = %d, nmax = %d, count = %d, nrep = %d\n", totbits, nmax, count, nrep) ;
  *stream = stream_save ;                    // restore stream state in case of error
  return -1 ;
}

int32_t fst_rle1_decode(uint32_t * restrict mask, int32_t nbits, bitstream * restrict stream){
  int32_t totbits = 0, maskbits = 0 ;
  uint32_t mode ;
  STREAM_GET_NBITS(*stream, mode, 1) ;
fprintf(stderr, "available = %d, first bit = %d\n", nbits, mode) ;
  while(totbits < nbits){
    uint32_t tbits, token, token1, token2 ;
    STREAM_PEEK_NBITS(*stream, token, 32) ;
    tbits = __builtin_clz(~token) + 1 ;
    STREAM_GET_NBITS(*stream, token1, tbits) ;
    STREAM_GET_NBITS(*stream, token2, tbits) ;
    uint32_t offset = ~(0xFFFFFFFF << tbits) ;
    token2 += offset ;
    totbits += (2 * tbits) ;
    maskbits += token2 ;
fprintf(stderr, "bits = %2d, count = %3d, mode = %d\n", tbits*2, token2, mode) ;
    mode ^= 1 ;
  }

  return maskbits ;
}

#if 0
// used to prevent compiler from calling memcpy
static uint32_t dummy = 0 ;
void set_useless_symbol(void){
  dummy++ ;
}
// specialized (n == 32) version
static int32_t load_expand_32(uint32_t * restrict d, uint32_t * restrict s, uint32_t plug, uint32_t mask){
  uint32_t *s0 = s ;        // start of source
  if(mask == 0         ){ for(uint32_t i=0 ; i<32 ; i++) { d[i] = s[i] ; return 32 ; } }
  if(mask == 0xFFFFFFFF){ for(uint32_t i=0 ; i<32 ; i++) { d[i] = plug ; return  0 ; } }
  for(int i=0 ; i<32 ; i++){
    if((mask >> i) & 1){
      d[i] = plug ;                  // plug destination
    }else{
      d[i] = *s ; s++ ;              // copy, then bump source pointer
    }
  }
  return (s - s0) ;
}
// expand using mask, up to 32 values
// d   [OUT] : destination array
// s    [IN] : source array
// mask [IN] : 32 bit control mask
// n    [IN] : number of valid bits in mask (up to 32)
// return number of values copied from source to destination
static int32_t load_expand_n(uint32_t * restrict d, uint32_t * restrict s, uint32_t plug, uint32_t mask, uint32_t n){
  if(n ==  0) return 0 ;     // nothing to do
  if(n == 32) return load_expand_32(d, s, plug, mask) ;

  uint32_t *s0 = s ;        // start of source
  n = (n > 32) ? 32 : n ;
  uint32_t emask = 0xFFFFFFFF >> (32 - n) ;

  if(mask == 0){                 // copy all values from source
    for(uint32_t i=0 ; i<32 ; i++) { if(i == n) break ; d[i] = s[i] + dummy ; }
    return n ;
  }
  if((mask & emask) == emask){   // plug all values
    for(uint32_t i=0 ; i<32 ; i++) { if(i == n) break ; d[i] = plug ; }
    return 0 ;
  }

  for(uint32_t i=0 ; i<32 ; i++){         // plug if mask bit is 1, copy otherwise
    if(i == n) break ;
    if((mask >> i) & 1){
      d[i] = plug ;                  // plug destination
    }else{
      d[i] = *s ; s++ ;              // copy, then bump source pointer
    }
  }
  return (s - s0) ;
}

// expand using mask
// d   [OUT] : destination array
// s    [IN] : source array
// mask [IN] : control mask array (32 bit items)
// n    [IN] : number of valid bits in mask array
// return number of values copied from source to destination
int32_t load_expand(uint32_t * restrict d, uint32_t * restrict s, uint32_t plug, uint32_t * restrict mask, uint32_t n){
  int32_t total = 0 ;
  while(n > 31){
    int32_t partial = load_expand_32(d, s, plug, *mask) ; // 32 values slice
    s += partial ; d += 32 ; mask++ ; n -= 32 ;
    total += partial ;
  }
  if(n > 0) total += load_expand_n(d, s, plug, *mask, n) ;          // remaining (< 32) values
  return total ;
}
int32_t store_compress_32(uint32_t * restrict d, uint32_t * restrict s, uint32_t mask){
  uint32_t *d0 = d ;        // start of destination
  if(mask == 0         ){ for(uint32_t i=0 ; i<32 ; i++) { d[i] = s[i] | dummy ; return 32 ; } }
  if(mask == 0xFFFFFFFF){ return  0 ; }
  for(int i=0 ; i<32 ; i++){     // skip if mask bit is 1, copy otherwise
    if((mask >> i) & 1){
      s++ ;                      // do not copy source
    }else{
      d[i] = *s ;                // copy
      s++ ;
    }
  }
  return (d - d0) ;
}

int32_t store_compress_n(uint32_t * restrict d, uint32_t * restrict s, uint32_t mask, uint32_t n){
  if(n == 32) return store_compress_32(d, s, mask) ;
  if(n ==  0) return 0 ;
  if(mask == 0){ for(uint32_t i=0 ; i<32 ; i++) { d[i] = s[i] | dummy ; return 32 ; } }

  uint32_t *d0 = d ;        // start of destination
  n = (n > 32) ? 32 : n ;
  uint32_t emask = 0xFFFFFFFF >> (32 - n) ;
  if((mask & emask) == emask){ return  0 ; }

  for(uint32_t i=0 ; i<32 ; i++){   // skip if mask bit is 1, copy otherwise
    if(i == n) break ;
    if((mask >> i) & 1){
      s++ ;                         // do not copy source
    }else{
      d[i] = *s ;                   // copy
      s++ ;
    }
  }
  return (d - d0) ;
}

// expand using mask
// d    [OUT] : destination array
// s     [IN] : source array
// mask [OUT] : control mask array (32 bit items)
// n     [IN] : number of values
// return number of values copied from source to destination
int32_t store_compress(uint32_t * restrict d, uint32_t * restrict s, uint32_t * restrict mask, uint32_t n){
  int32_t total = 0 ;
  while(n > 31){
    int32_t partial = store_compress_32(d, s, *mask) ;
    d += partial ; s += 32 ; mask++ ; n -= 32 ;
    total += partial ;
  }
  if(n > 0) total += store_compress_n(d, s, *mask, n) ;
  return total ;
}
#endif
