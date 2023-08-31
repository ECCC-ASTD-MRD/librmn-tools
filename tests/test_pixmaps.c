/* Hopefully useful functions for C and FORTRAN
 * Copyright (C) 2021-2023  Recherche en Prevision Numerique
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation,
 * version 2.1 of the License.
 *
 * This is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

// double include deliberate for test purposes
#include <rmn/pixmaps.h>
#include <rmn/pixmaps.h>
#include <rmn/timers.h>
#include <rmn/bits.h>

#define NPTS 4095
#define EVERY 31

#define STUFF(what) { uint32_t t = 1 ; if(free < 32) { *in = (acc >> (32-free)) ; t = *in ; in++ ; free += 32 ; } ; acc <<= bits ; acc |= what ; free -= bits ; what = t ; }
#define FLUSH { if(free < 32) { *in = acc >> (32-free) ; in++ ; free += 32 ; } ; if(free < 64) *in = (acc << (free - 32)) ; }

#define XTRACT(what) { if(avail < 32) { uint64_t t = *out++ ; acc |= (t << (32 - avail)) ; avail += 32 ; } ; what = acc >> (64 - bits) ; acc <<= bits ; avail -= bits ; }

// 2 bits per element, packing done 16 values at a time to vectorize
void pixmap_be_02(uint32_t *src, int n, rmn_pixmap *s){
  uint32_t i, i0, r1, r2, n15 = n & 0xF ;
  uint32_t mask = RMASK31(2) ;
  int bits = 2*16 ;               // 16 x 2 bit slices
  int32_t sh[16] ;
  uint64_t  acc  = 0 ;
  int32_t   free = 64 ;
  uint32_t *in  = s->data ;
  int zero = 0, all1 = 0 ;

  for(i=0 ; i<16 ; i++) sh[i] = 2 * (15 - i) ;

  for(i0=0 ; i0<n-31 ; i0+=32){
    r1 = r2 = 0 ;
    for(i=0 ; i<16 ; i++){
      r1 |= ((src[i   ] & mask) << sh[i]) ;
      r2 |= ((src[i+16] & mask) << sh[i]) ;
    }
    src += 32 ;
    STUFF(r1) ; STUFF(r2) ;
    zero = zero + (r1 == 0) ? 1 : 0 + (r2 == 0) ? 1 : 0 ;
    all1 = all1 + (r1 == ~0u) ? 1 : 0 + (r2 == ~0u) ? 1 : 0 ;
  }
  r1 = 0 ;
  for(     ; i0<n-15 ; i0+=16){
    for(i=0 ; i<16 ; i++){
      r1 |= ((src[i   ] & mask) << sh[i]) ;
    }
    src += 16 ;
    STUFF(r1) ;
    zero = zero + (r1 == 0) ? 1 : 0 ;
    all1 = all1 + (r1 == ~0u) ? 1 : 0 ;
  }
  if(i0 < n){
    r1 = 0 ;
    for(i=0 ; i<n15 ; i++){
      r1 |= ((src[i   ] & mask) << sh[i]) ;
    }
    STUFF(r1) ;
  }
  FLUSH ;
  s->bits = 2 ;
  s->elem = 2 * n ;
  s->zero = zero ;
  s->all1 = all1 ;
}
void pixmap_restore_be_02(uint32_t *dst, int n, rmn_pixmap *s){
  uint32_t mask = RMASK31(2) ;
  uint32_t *out  = s->data ;
  uint64_t  acc = 0 ;
  int32_t   avail = 0 ;
  int bits = 2*16 ;         // extraction done 8 packed values at a time
  int i0, i ;
  int32_t sh[16] ;
  uint32_t s1, s2, n15 = n & 0xF ;

  for(i=0 ; i<16 ; i++) sh[i] = 2 * (15 - i) ;

  for(i0=0 ; i0<n-31 ; i0+=32){
    XTRACT(s1) ; XTRACT(s2) ;
// fprintf(stderr, "< %8.8x %8.8x n= %d\n", s1, s2, n);
    for(i=0 ; i<16 ; i++){
      dst[i   ] = mask & (s1 >> sh[i]) ;
      dst[i+16] = mask & (s2 >> sh[i]) ;
    }
    dst += 32 ;
  }
  for(     ; i0<n-15 ; i0+=16){
    XTRACT(s1) ;
    for(i=0 ; i<16 ; i++){
      dst[i   ] = mask & (s1 >> sh[i]) ;
    }
    dst += 16 ;
  }
  XTRACT(s1) ;
  for(i=0 ; i<n15 ; i++){
    dst[i   ] = mask & (s1 >> sh[i]) ;
  }
}

// 3 or 4 bits per element, packing done 8 values at a time to vectorize
void pixmap_be_34(uint32_t *src, int nbits, int n, rmn_pixmap *s){
  uint32_t i, i0, t, r1, r2, r3, r4, n7 = n & 0x7 ;
  int32_t sh[8] ;
  uint32_t mask = RMASK31(nbits) ;
  uint64_t  acc  = 0 ;
  int32_t   free = 64 ;
  uint32_t *in  = (uint32_t *)s->data ;
  int bits = nbits*8 ;         // insertion done 8 packed values at a time
  int zero = 0, all1 = 0 ;

  for(i=0 ; i<8 ; i++) sh[i] = nbits * (7 - i) ;

  for(i0=0 ; i0<n-31 ; i0+=32){        // 4 x 8 slices = 32 packed values
    r1 = r2 = r3 = r4 = 0 ;
    for(i=0 ; i<8 ; i++){
      r1 |= ((src[i   ] & mask) << sh[i]) ;
      r2 |= ((src[i+ 8] & mask) << sh[i]) ;
      r3 |= ((src[i+16] & mask) << sh[i]) ;
      r4 |= ((src[i+24] & mask) << sh[i]) ;
    }
    src += 32 ;
    STUFF(r1) ; STUFF(r2) ; STUFF(r3) ; STUFF(r4) ;
    zero = zero + (r1 == 0) ? 1 : 0 + (r2 == 0) ? 1 : 0 + (r3 == 0) ? 1 : 0 + (r4 == 0) ? 1 : 0 ;
    all1 = all1 + (r1 == ~0u) ? 1 : 0 + (r2 == ~0u) ? 1 : 0 + (r3 == ~0u) ? 1 : 0 + (r4 == ~0u) ? 1 : 0 ;
  }
  for(     ; i0<n-7 ; i0+=8){          // 1x 8 slices
    r1 = 0 ;
    for(i=0 ; i<8 ; i++){
      r1 |= ((src[i  ] & mask) << sh[i]) ;
    }
    src += 8 ;
    STUFF(r1) ;
    zero = zero + (r1 == 0) ? 1 : 0 ;
  }
  r1 = 0 ;
  for(i=0 ; i<n7 ; i++){
    r1 |= ((src[i  ] & mask) << sh[i]) ;
  }
  STUFF(r1) ;
  FLUSH ;
  s->bits = nbits ;
  s->elem = nbits * n ;
  s->zero = zero ;
  s->all1 = all1 ;
}

void pixmap_restore_be_34(uint32_t *dst, int nbits, int n, rmn_pixmap *s){
  uint32_t mask = RMASK31(nbits) ;
  uint32_t *out  = s->data ;
  uint64_t  acc ;
  int32_t   avail ;
  int bits = nbits*8 ;         // extraction done 8 packed values at a time
  int i0, i ;
  int32_t sh[8] ;
  uint32_t s1, s2, s3, s4, n7 = n & 0x7 ;

  acc = out[0] ; acc <<= 32 ; out++ ; avail = 32 ;
  for(i=0 ; i<8 ; i++) sh[i] = nbits * (7 - i) ;

  for(i0=0 ; i0<n-31 ; i0+=32){
    XTRACT(s1) ; XTRACT(s2) ; XTRACT(s3) ; XTRACT(s4) ;  // get 4 slices
    for(i=0 ; i<8 ; i++){                                // extract 8 elements from each slice
      dst[i   ] = mask & (s1 >> sh[i]) ;
      dst[i+ 8] = mask & (s2 >> sh[i]) ;
      dst[i+16] = mask & (s3 >> sh[i]) ;
      dst[i+24] = mask & (s4 >> sh[i]) ;
    }
    dst += 32 ;
  }
  for(     ; i0<n-7 ; i0+=8){          // 1x 8 slices
    XTRACT(s1) ;
    for(i=0 ; i<8 ; i++){
      dst[i   ] = mask & (s1 >> sh[i]) ;
    }
    dst += 8 ;
  }
  XTRACT(s1) ;
  for(i=0 ; i<n7 ; i++){
    dst[i   ] = mask & (s1 >> sh[i]) ;
  }
}

// 5 to 8 bits per element, packing done 4 values at a time to vectorize
void pixmap_be_58(uint32_t *src, int nbits, int n, rmn_pixmap *s){
  uint32_t i, i0, t, r1, r2, r3, r4, n3 = n & 0x3 ;
  int32_t sh[4] ;
  uint32_t mask  = RMASK31(nbits) ;
  uint64_t  acc  = 0 ;
  int32_t   free = 64 ;
  uint32_t *in  = (uint32_t *)s->data ;
  int bits = nbits*4 ;                        // insertion done 4 packed values at a time
  int zero = 0, all1 = 0 ;

  sh[3] = 0 ; sh[2] = nbits ; sh[1] = nbits + nbits ; sh[0] = nbits + nbits + nbits ; 
  for(i0=0 ; i0<n-15 ; i0+=16){
    r1 = r2 = r3 = r4 = 0 ;
    for(i=0 ; i<4 ; i++){                     // 4 x 4 slices = 16 packed values
      r1 |= ((src[i   ] & mask) << sh[i]) ;
      r2 |= ((src[i+ 4] & mask) << sh[i]) ;
      r3 |= ((src[i+ 8] & mask) << sh[i]) ;
      r4 |= ((src[i+12] & mask) << sh[i]) ;
    }
    src += 16 ;
    STUFF(r1) ; STUFF(r2) ; STUFF(r3) ; STUFF(r4) ;
    zero = zero + (r1 == 0) ? 1 : 0 + (r2 == 0) ? 1 : 0 + (r3 == 0) ? 1 : 0 + (r4 == 0) ? 1 : 0 ;
    all1 = all1 + (r1 == ~0u) ? 1 : 0 + (r2 == ~0u) ? 1 : 0 + (r3 == ~0u) ? 1 : 0 + (r4 == ~0u) ? 1 : 0 ;
  }
  for(     ; i0<n-3 ; i0+=4){
    r1 = 0 ;
    for(i=0 ; i<4 ; i++){
      r1 |= ((src[i  ] & mask) << sh[i]) ;
    }
    src += 4 ;
    STUFF(r1) ;
    zero = zero + (r1 == 0) ? 1 : 0 ;
    all1 = all1 + (r1 == ~0u) ? 1 : 0 ;
  }
  r1 = 0 ;
  for(i=0 ; i<n3 ; i++){
    r1 |= ((src[i  ] & mask) << sh[i]) ;
  }
  STUFF(r1) ;
  FLUSH ;
  s->bits = nbits ;
  s->elem = nbits * n ;
  s->zero = zero ;
  s->all1 = all1 ;
}

void pixmap_restore_be_58(uint32_t *dst, int nbits, int n, rmn_pixmap *s){
  uint32_t mask = RMASK31(nbits) ;
  uint32_t *out  = s->data ;
  uint64_t  acc ;
  int32_t   avail ;
  int bits = nbits*4 ;         // extraction done 4 packed values at a time
  int i0, i ;
  int32_t sh[4] ;
  uint32_t s1, s2, s3, s4, n3 = n & 0x3 ;

  acc = 0 ; avail = 0 ;
  sh[3] = 0 ; sh[2] = nbits ; sh[1] = nbits + nbits ; sh[0] = nbits + nbits + nbits ;

  for(i0=0 ; i0<n-15 ; i0+=16){
    XTRACT(s1) ; XTRACT(s2) ; XTRACT(s3) ; XTRACT(s4) ;  // get 4 slices
    for(i=0 ; i<4 ; i++){                                // extract 4 elements from each slice
      dst[i   ] = mask & (s1 >> sh[i]) ;
      dst[i+ 4] = mask & (s2 >> sh[i]) ;
      dst[i+ 8] = mask & (s3 >> sh[i]) ;
      dst[i+12] = mask & (s4 >> sh[i]) ;
    }
    dst += 16 ;
  }
  for(     ; i0<n-3 ; i0+=4){
    XTRACT(s1) ;                                         // get 1 slice at a time
    for(i=0 ; i<4 ; i++){                                // extract 4 elements from each slice
      dst[i   ] = mask & (s1 >> sh[i]) ;
    }
    dst += 4 ;
  }
  XTRACT(s1) ;
  for(i=0 ; i<n3 ; i++){
    dst[i   ] = mask & (s1 >> sh[i]) ;
  }
}

void print_encode_mode(char *msg, int mode){
  int full_83_1 = (mode&RLE_FULL_1) && ((mode&RLE_123_1) == 0) ;
  int full_83_0 = (mode&RLE_FULL_0) && ((mode&RLE_123_0) == 0) ;
  fprintf(stderr, "%s : mode = %s %s%s (0) %s %s%s (1)\n", 
          msg, 
          mode&RLE_FULL_0 ? "FULL" : "SIMPLE" , mode&RLE_123_0 ? "12/3" : "", full_83_0 ? "8/3" : "",
          mode&RLE_FULL_1 ? "FULL" : "SIMPLE" , mode&RLE_123_1 ? "12/3" : "", full_83_1 ? "8/3" : "") ;
}

int main(int argc, char **argv){
  int32_t iarray[NPTS] ;
  uint32_t uarray[NPTS], urestored[NPTS] ;
  float farray[NPTS] ;
  int32_t restored[NPTS] ;
  rmn_pixmap *pixmap = NULL, *pixmap2, *pixmap3, *pixmapp, *pixmapd, *pixmap8 ;
  rmn_pixmap *rle, *rle2 ;
  int i0, i, nbits, errors, mode, npts ;
  int32_t special = 999999 ;
  uint32_t mmask = 0u, mask ;
  TIME_LOOP_DATA ;
  uint64_t overhead = get_cycles_overhead() ;
  uint32_t umulti[NPTS] ;    // for multi bit per element pixmap tests

  fprintf(stderr, "timing cycles overhead = %ld cycles\n", overhead) ;

  for(i=0 ; i<NPTS ; i++) umulti[i] = i ;
  for(i=0 ; i<NPTS ; i++) iarray[i] = i - NPTS/2 ;
  for(i=0 ; i<NPTS ; i++) farray[i] = i - NPTS/2 ;
  for(i=0 ; i<NPTS ; i++) uarray[i] = 0x80000000u + i - NPTS/2 ;

  pixmap8 = pixmap_create(NPTS, 8) ;   // create an empty pixmap for up to NPTS 8 bit elements
  npts = NPTS ;

  nbits = 2 ;
  errors = 0 ;
  mask = RMASK31(2) ;
  for(i=0 ; i<NPTS*2/32 ; i++) pixmap8->data[i] = ~0 ;
  pixmap_be_02(umulti, npts, pixmap8) ;
  for(i=0 ; i<npts ; i++) urestored[i] = 0xFFFFFFFFu ;
  pixmap_restore_be_02(urestored, npts, pixmap8) ;
  for(i=0 ; i<npts ; i++) if(urestored[i] != (umulti[i] & mask)) { errors++ ; if(errors < 8) fprintf(stderr, "error at position %d %2.2x (%2.2x) \n", i, urestored[i],(umulti[i] & mask) ) ;}
  if(errors > 0) fprintf(stderr,"pixmap_02 : nbits = %d, npts = %d, errors = %d\n", nbits, npts, errors) ;
  if(errors > 0) exit(1) ;
  TIME_LOOP_EZ(1000, NPTS, pixmap_be_02((uint32_t *)umulti, NPTS, pixmap8)) ;
  fprintf(stderr, "pixmap_be_02(2)          : %s\n",timer_msg);
  TIME_LOOP_EZ(1000, npts, pixmap_restore_be_02(urestored, npts, pixmap8)) ;
  fprintf(stderr, "pixmap_restore_be_02(2)  : %s\n", timer_msg);

  for(nbits = 3 ; nbits < 5 ; nbits++){
    errors = 0 ;
    mask = RMASK31(nbits) ;
    pixmap_be_34(umulti, nbits, npts, pixmap8) ;
    for(i=0 ; i<npts ; i++) urestored[i] = 0xFFFFFFFFu ;
    pixmap_restore_be_34(urestored, nbits, npts, pixmap8) ;
    for(i=0 ; i<npts ; i++) if(urestored[i] != (umulti[i] & mask)) { errors++ ; if(errors < 8) fprintf(stderr, "error at position %d %2.2x (%2.2x) \n", i, urestored[i],(umulti[i] & mask) ) ;}
    if(errors > 0) fprintf(stderr,"pixmap_34 : nbits = %d, npts = %d, errors = %d\n", nbits, npts, errors) ;
    if(errors > 0) exit(1) ;
    TIME_LOOP_EZ(1000, npts, pixmap_be_34((uint32_t *)umulti, nbits, npts, pixmap8)) ;
    fprintf(stderr, "pixmap_be_34(%d)          : %s\n", nbits, timer_msg);
    TIME_LOOP_EZ(1000, npts, pixmap_restore_be_34(urestored, nbits, npts, pixmap8)) ;
    fprintf(stderr, "pixmap_restore_be_34(%d)  : %s\n", nbits, timer_msg);
  }

  for(nbits = 5 ; nbits < 9 ; nbits++){
    errors = 0 ;
    mask = RMASK31(nbits) ;
    pixmap_be_58(umulti, nbits, npts, pixmap8) ;
    for(i=0 ; i<npts ; i++) urestored[i] = 0xFFFFFFFFu ;
    pixmap_restore_be_58(urestored, nbits, npts, pixmap8) ;
    for(i=0 ; i<npts ; i++) if(urestored[i] != (umulti[i] & mask)) errors++ ;
    if(errors > 0) fprintf(stderr,"pixmap_58 : nbits = %d, npts = %d, errors = %d\n", nbits, npts, errors) ;
    if(errors > 0) exit(1) ;
    TIME_LOOP_EZ(1000, npts, pixmap_be_58((uint32_t *)umulti, nbits, npts, pixmap8)) ;
    fprintf(stderr, "pixmap_be_58(%d)          : %s\n", nbits, timer_msg);
    TIME_LOOP_EZ(1000, npts, pixmap_restore_be_58(urestored, nbits, npts, pixmap8)) ;
    fprintf(stderr, "pixmap_restore_be_58(%d)  : %s\n", nbits, timer_msg);
  }

// return 0 ;
  pixmapp = pixmap_create(NPTS, 1) ;   // create an empty pixmap for up to NPTS 1 bit elements

  pixmapp = pixmap_be_fp_01(farray, pixmapp, 31.5f, 0x7, NPTS, OP_SIGNED_GT) ;
  fprintf(stderr, "RAW  > 31.5 pixmap    (float) :\n");
  for(i0=0 ; i0<NPTS/32-15 ; i0+=16){
    for(i=0 ; i<16 ; i++) fprintf(stderr, "%8.8x ", pixmapp->data[i0+i]) ;
    fprintf(stderr, "\n");
  }

  mode = pixmap_encode_hint_01(pixmapp) ;
  print_encode_mode("pixmap_encode_hint_01", mode) ;

  pixmapp = pixmap_be_fp_01(farray, pixmapp, 31.5f, 0x7, NPTS, OP_SIGNED_LT) ;
  fprintf(stderr, "RAW  < 31.5 pixmap    (float) :\n");
  for(i0=0 ; i0<NPTS/32-15 ; i0+=16){
    for(i=0 ; i<16 ; i++) fprintf(stderr, "%8.8x ", pixmapp->data[i0+i]) ;
    fprintf(stderr, "\n");
  }

  pixmapp = pixmap_be_int_01(iarray, pixmapp, 0, 0x7, NPTS, OP_SIGNED_GT) ;
  fprintf(stderr, "RAW  > 0 pixmap   (signed) :\n");
  for(i0=0 ; i0<NPTS/32-15 ; i0+=16){
    for(i=0 ; i<16 ; i++) fprintf(stderr, "%8.8x ", pixmapp->data[i0+i]) ;
    fprintf(stderr, "\n");
  }

  pixmapp = pixmap_be_int_01(uarray, pixmapp, uarray[NPTS/2-32], 0, NPTS, OP_UNSIGNED_GT) ;
  fprintf(stderr, "RAW  > %8.8x pixmap (unsigned) :\n", uarray[NPTS/2-32]);
  for(i0=0 ; i0<NPTS/32-15 ; i0+=16){
    for(i=0 ; i<16 ; i++) fprintf(stderr, "%8.8x ", pixmapp->data[i0+i]) ;
    fprintf(stderr, "\n");
  }

  pixmapp = pixmap_be_int_01(iarray, pixmapp, 0, 0x7, NPTS, OP_SIGNED_LT) ;
  fprintf(stderr, "RAW  < 0 pixmap   (signed) :\n");
  for(i0=0 ; i0<NPTS/32-15 ; i0+=16){
    for(i=0 ; i<16 ; i++) fprintf(stderr, "%8.8x ", pixmapp->data[i0+i]) ;
    fprintf(stderr, "\n");
  }

  pixmapp = pixmap_be_int_01(uarray, pixmapp, uarray[NPTS/2-32], 0, NPTS, OP_UNSIGNED_LT) ;
  fprintf(stderr, "RAW  < %8.8x pixmap (unsigned) :\n", uarray[NPTS/2-32]);
  for(i0=0 ; i0<NPTS/32-15 ; i0+=16){
    for(i=0 ; i<16 ; i++) fprintf(stderr, "%8.8x ", pixmapp->data[i0+i]) ;
    fprintf(stderr, "\n");
  }

  pixmapp = pixmap_be_int_01(iarray, pixmapp, -7, 0x7, NPTS, 0) ;
  fprintf(stderr, "RAW == -7/0x7 pixmap :\n");
  for(i0=0 ; i0<NPTS/32-15 ; i0+=16){
    for(i=0 ; i<16 ; i++) fprintf(stderr, "%8.8x ", pixmapp->data[i0+i]) ;
    fprintf(stderr, "\n");
  }

  // add special value and mask to the fray
  for(i=EVERY+1 ; i<NPTS ; i+=EVERY) iarray[i] = special ;
  for(i=NPTS-3 ; i<NPTS ; i++) iarray[i] = special ;
  iarray[0] = special ;
  for(i=0 ; i<NPTS ; i++) if(iarray[i] == special) fprintf(stderr, "%d ",i) ;
  fprintf(stderr, "\n\n");

  pixmap = pixmap_be_eq_01(iarray, NULL, special, mmask, NPTS) ;
  pixmap2 = pixmap_create(NPTS, 1) ;
  fprintf(stderr, "RAW == %8.8x/%8.8x pixmap :\n", special, mmask);
  for(i0=0 ; i0<NPTS/32 ; i0+=16){
    for(i=0 ; i<16 ; i++) fprintf(stderr, "%8.8x ", pixmap->data[i0+i]) ;
    fprintf(stderr, "\n");
  }

  mode = pixmap_encode_hint_01(pixmap) ;
  print_encode_mode("pixmap_encode_hint_01", mode) ;

// RLE encode bit map
  rle2 = pixmap_encode_be_01(pixmap, NULL, RLE_DEFAULT) ;                       // encoding
  rle  = pixmap_encode_be_01(pixmap, pixmap, RLE_DEFAULT) ;   // in-place encoding
  fprintf(stderr, "RLE encoded pixmap :%d RLE bits , encoding %d RAW bits, max size = %d\n", rle->nrle, rle->elem, 32*rle->size);
  for(i=0 ; i<(rle->nrle+31)/32 ; i++) fprintf(stderr, "%8.8x ",rle->data[i]) ;
  fprintf(stderr, "\n");
  errors = 0 ;
  for(i=0 ; i<(rle->nrle+31)/32 ; i++) if(pixmap->data[i] != rle2->data[i]) errors++ ;
  fprintf(stderr, "in-place encoding errors = %d (%d|%d elements)\n", errors, (rle->nrle+31)/32, (pixmap->nrle+31)/32) ;

// try to restore from RLE coded pixmap
  for(i=0 ; i<NPTS ; i++) restored[i] = iarray[i] ;
  for(i=EVERY+1 ; i<NPTS ; i+=EVERY) restored[i] = -special ;
  for(i=NPTS-3 ; i<NPTS ; i++) restored[i] = -special ;
  restored[0] = -special ;
  int nrestore = pixmap_restore_be_01(restored, pixmap, special, NPTS) ;
  fprintf(stderr, "restore from RLE encoding : nrestore = %d\n", nrestore) ;
  errors = 0 ;
  for(i=0 ; i<NPTS ; i++){
    if(iarray[i] != restored[i]) {
      if(errors < 10) fprintf(stderr, "i = %d, expected %d, got %d\n", i, iarray[i], restored[i]);
      errors++ ;
    }
  }
  fprintf(stderr, "RLE pixmap restore errors = %d\n", errors) ;

// decode RLE encoded pixmap
  fprintf(stderr, "pixmap2 O.K. for up to %d bits\n", 32 * pixmap2->size) ;
  for(i=0 ; i<pixmap2->size ; i++) pixmap2->data[i] = 0xFFFFFFFFu ;
  pixmap3 = pixmap_decode_be_01(pixmap2, rle2) ;
  fprintf(stderr, "decoded RLE pixmap :\n");
  for(i0=0 ; i0<NPTS/32 ; i0+=16){
    for(i=0 ; i<16 ; i++) fprintf(stderr, "%8.8x ", pixmap3->data[i0+i]) ;
    fprintf(stderr, "\n");
  }

// restore from bit map
  for(i=0 ; i<NPTS ; i++) restored[i] = iarray[i] ;
  for(i=EVERY+1 ; i<NPTS ; i+=EVERY) restored[i] = -special ;
  for(i=NPTS-3 ; i<NPTS ; i++) restored[i] = -special ;
  restored[0] = -special ;
  pixmap_restore_be_01(restored, pixmap3, special, NPTS) ;
  errors = 0 ;
  for(i=0 ; i<NPTS ; i++){
    if(iarray[i] != restored[i]) {
      if(errors < 10) fprintf(stderr, "i = %d, expected %d, got %d\n", i, iarray[i], restored[i]);
      errors++ ;
    }
  }
  fprintf(stderr, "pixmap restore errors = %d\n", errors) ;

//   TIME_LOOP_EZ(1000, NPTS, pixmap_be_eq_01(iarray, pixmap, special, mmask, NPTS)) ;
//   fprintf(stderr, "pixmap_be_eq_01 : %s\n",timer_msg);

  TIME_LOOP_EZ(1000, NPTS, pixmap_be_int_01(iarray, pixmap, iarray[NPTS/2], mmask, NPTS, OP_SIGNED_GT)) ;
  fprintf(stderr, "pixmap_be_int_01 : %s\n",timer_msg);

  TIME_LOOP_EZ(1000, NPTS, pixmap_be_fp_01(farray, pixmap, farray[NPTS/2], mmask, NPTS, OP_SIGNED_GT)) ;
  fprintf(stderr, "pixmap_be_fp_01  : %s\n",timer_msg);

  for(i=0 ; i<NPTS ; i++) iarray[i] = i - NPTS/2 ;
  for(i=1 ; i<NPTS ; i+=(31)) iarray[i] = special ;
  pixmapd = pixmap_be_eq_01(iarray, NULL, special, mmask, NPTS) ;

  TIME_LOOP_EZ(1000, NPTS, pixmap_be_eq_01(iarray, pixmapd, special, mmask, NPTS)) ;
  fprintf(stderr, "pixmap_be_eq_01  : %s\n",timer_msg);

  TIME_LOOP_EZ(1000, NPTS, mode = pixmap_encode_hint_01(pixmapd)) ;
  fprintf(stderr, "encode_hint_01   : %s\n",timer_msg);

  TIME_LOOP_EZ(1000, NPTS, rle = pixmap_encode_be_01(pixmapd, rle, RLE_DEFAULT)) ;
  fprintf(stderr, "encode_be_01     : %s\n",timer_msg);

  TIME_LOOP_EZ(1000, NPTS, pixmap3 = pixmap_decode_be_01(pixmapd, rle)) ;
  fprintf(stderr, "decode_be_01     : %s\n",timer_msg);

  TIME_LOOP_EZ(1000, NPTS, pixmap_restore_be_01(restored, pixmapd, special, NPTS)) ;
  fprintf(stderr, "restore_be_01    : %s\n",timer_msg);

  TIME_LOOP_EZ(1000, NPTS, pixmap_be_34((uint32_t *)umulti, 3, NPTS, pixmap8)) ;
  fprintf(stderr, "pixmap_be_34(3)  : %s\n",timer_msg);

  TIME_LOOP_EZ(1000, NPTS, pixmap_be_34((uint32_t *)umulti, 4, NPTS, pixmap8)) ;
  fprintf(stderr, "pixmap_be_34(4)  : %s\n",timer_msg);
}
