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

#define NPTS 2047
#define EVERY 31

#define STUFF(what) { if(free < 32) { *in = acc > (32-free) ; in++ ; free += 32 ; } ; acc <<= bits ; acc |= what ; free -= bits ; }

// 2 bits per element, packing done 16 values at a time to vectorize
void pixmap_be_02(uint32_t *src, int nbits, int n, rmn_pixmap *s){
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
   }
   for(     ; i0<n-7 ; i0+=8){          // 1x 8 slices
     r1 = 0 ;
     for(i=0 ; i<8 ; i++){
       r1 |= ((src[i  ] & mask) << sh[i]) ;
     }
     src += 8 ;
     STUFF(r1) ;
   }
   r1 = 0 ;
   for(i=0 ; i<n7 ; i++){
     r1 |= ((src[i  ] & mask) << sh[i]) ;
   }
   STUFF(r1) ;
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
   }
   for(     ; i0<n-3 ; i0+=4){
     r1 = 0 ;
     for(i=0 ; i<4 ; i++){
       r1 |= ((src[i  ] & mask) << sh[i]) ;
     }
     src += 4 ;
     STUFF(r1) ;
   }
   r1 = 0 ;
   for(i=0 ; i<n3 ; i++){
     r1 |= ((src[i  ] & mask) << sh[i]) ;
   }
   STUFF(r1) ;
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
  uint32_t uarray[NPTS] ;
  float farray[NPTS] ;
  int32_t restored[NPTS] ;
  rmn_pixmap *pixmap = NULL, *pixmap2, *pixmap3, *pixmapp, *pixmapd, *pixmapx ;
  rmn_pixmap *rle, *rle2 ;
  int i0, i, errors, mode ;
  int32_t special = 999999 ;
  uint32_t mmask = 0u ;
  TIME_LOOP_DATA ;
  uint64_t overhead = get_cycles_overhead() ;

  fprintf(stderr, "timing cycles overhead = %ld cycles\n", overhead) ;

  for(i=0 ; i<NPTS ; i++) iarray[i] = i - NPTS/2 ;
  for(i=0 ; i<NPTS ; i++) farray[i] = i - NPTS/2 ;
  for(i=0 ; i<NPTS ; i++) uarray[i] = 0x80000000u + i - NPTS/2 ;

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

  pixmapx = pixmap_create(NPTS, 8) ;
  TIME_LOOP_EZ(1000, NPTS, pixmap_be_34((uint32_t *)iarray, 3, NPTS, pixmapx)) ;
  fprintf(stderr, "pixmap_be_34(3)  : %s\n",timer_msg);

  TIME_LOOP_EZ(1000, NPTS, pixmap_be_34((uint32_t *)iarray, 4, NPTS, pixmapx)) ;
  fprintf(stderr, "pixmap_be_34(4)  : %s\n",timer_msg);

  TIME_LOOP_EZ(1000, NPTS, pixmap_be_58((uint32_t *)iarray, 5, NPTS, pixmapx)) ;
  fprintf(stderr, "pixmap_be_58(5)  : %s\n",timer_msg);

  TIME_LOOP_EZ(1000, NPTS, pixmap_be_58((uint32_t *)iarray, 6, NPTS, pixmapx)) ;
  fprintf(stderr, "pixmap_be_58(6)  : %s\n",timer_msg);

  TIME_LOOP_EZ(1000, NPTS, pixmap_be_58((uint32_t *)iarray, 7, NPTS, pixmapx)) ;
  fprintf(stderr, "pixmap_be_58(7)  : %s\n",timer_msg);

  TIME_LOOP_EZ(1000, NPTS, pixmap_be_58((uint32_t *)iarray, 8, NPTS, pixmapx)) ;
  fprintf(stderr, "pixmap_be_58(8)  : %s\n",timer_msg);
}
