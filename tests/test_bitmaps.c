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

// double include intended for test purposes
#include <rmn/bitmaps.h>
#include <rmn/bitmaps.h>
#include <rmn/timers.h>

#define NPTS 511
#define EVERY 31

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
  rmn_bitmap *bitmap = NULL, *bitmap2, *bitmap3, *bitmapp, *bitmapd ;
  rmn_bitmap *rle, *rle2 ;
  int i0, i, errors, mode ;
  int32_t special = 999999 ;
  uint32_t mmask = 0u ;
  TIME_LOOP_DATA ;

  for(i=0 ; i<NPTS ; i++) iarray[i] = i - NPTS/2 ;
  for(i=0 ; i<NPTS ; i++) farray[i] = i - NPTS/2 ;
  for(i=0 ; i<NPTS ; i++) uarray[i] = 0x80000000u + i - NPTS/2 ;

  bitmapp = bitmap_create(NPTS) ;   // create an empty bitmap for up to NPTS elements

  bitmapp = bitmap_be_fp_01(farray, bitmapp, 31.5f, 0x7, NPTS, OP_SIGNED_GT) ;
  fprintf(stderr, "RAW  > 31.5 bitmap    (float) :\n");
  for(i0=0 ; i0<NPTS/32-15 ; i0+=16){
    for(i=0 ; i<16 ; i++) fprintf(stderr, "%8.8x ", bitmapp->data[i0+i]) ;
    fprintf(stderr, "\n");
  }

  mode = bitmap_encode_hint_01(bitmapp) ;
  print_encode_mode("bitmap_encode_hint_01", mode) ;

  bitmapp = bitmap_be_fp_01(farray, bitmapp, 31.5f, 0x7, NPTS, OP_SIGNED_LT) ;
  fprintf(stderr, "RAW  < 31.5 bitmap    (float) :\n");
  for(i0=0 ; i0<NPTS/32-15 ; i0+=16){
    for(i=0 ; i<16 ; i++) fprintf(stderr, "%8.8x ", bitmapp->data[i0+i]) ;
    fprintf(stderr, "\n");
  }

  bitmapp = bitmap_be_int_01(iarray, bitmapp, 0, 0x7, NPTS, OP_SIGNED_GT) ;
  fprintf(stderr, "RAW  > 0 bitmap   (signed) :\n");
  for(i0=0 ; i0<NPTS/32-15 ; i0+=16){
    for(i=0 ; i<16 ; i++) fprintf(stderr, "%8.8x ", bitmapp->data[i0+i]) ;
    fprintf(stderr, "\n");
  }

  bitmapp = bitmap_be_int_01(uarray, bitmapp, uarray[NPTS/2-32], 0, NPTS, OP_UNSIGNED_GT) ;
  fprintf(stderr, "RAW  > %8.8x bitmap (unsigned) :\n", uarray[NPTS/2-32]);
  for(i0=0 ; i0<NPTS/32-15 ; i0+=16){
    for(i=0 ; i<16 ; i++) fprintf(stderr, "%8.8x ", bitmapp->data[i0+i]) ;
    fprintf(stderr, "\n");
  }

  bitmapp = bitmap_be_int_01(iarray, bitmapp, 0, 0x7, NPTS, OP_SIGNED_LT) ;
  fprintf(stderr, "RAW  < 0 bitmap   (signed) :\n");
  for(i0=0 ; i0<NPTS/32-15 ; i0+=16){
    for(i=0 ; i<16 ; i++) fprintf(stderr, "%8.8x ", bitmapp->data[i0+i]) ;
    fprintf(stderr, "\n");
  }

  bitmapp = bitmap_be_int_01(uarray, bitmapp, uarray[NPTS/2-32], 0, NPTS, OP_UNSIGNED_LT) ;
  fprintf(stderr, "RAW  < %8.8x bitmap (unsigned) :\n", uarray[NPTS/2-32]);
  for(i0=0 ; i0<NPTS/32-15 ; i0+=16){
    for(i=0 ; i<16 ; i++) fprintf(stderr, "%8.8x ", bitmapp->data[i0+i]) ;
    fprintf(stderr, "\n");
  }

  bitmapp = bitmap_be_int_01(iarray, bitmapp, -7, 0x7, NPTS, 0) ;
  fprintf(stderr, "RAW == -7/0x7 bitmap :\n");
  for(i0=0 ; i0<NPTS/32-15 ; i0+=16){
    for(i=0 ; i<16 ; i++) fprintf(stderr, "%8.8x ", bitmapp->data[i0+i]) ;
    fprintf(stderr, "\n");
  }

  // add special value and mask to the fray
  for(i=EVERY+1 ; i<NPTS ; i+=EVERY) iarray[i] = special ;
  for(i=NPTS-3 ; i<NPTS ; i++) iarray[i] = special ;
  iarray[0] = special ;
  for(i=0 ; i<NPTS ; i++) if(iarray[i] == special) fprintf(stderr, "%d ",i) ;
  fprintf(stderr, "\n\n");

  bitmap = bitmap_be_eq_01(iarray, NULL, special, mmask, NPTS) ;
  bitmap2 = bitmap_create(NPTS) ;
  fprintf(stderr, "RAW == %8.8x/%8.8x bitmap :\n", special, mmask);
  for(i0=0 ; i0<NPTS/32 ; i0+=16){
    for(i=0 ; i<16 ; i++) fprintf(stderr, "%8.8x ", bitmap->data[i0+i]) ;
    fprintf(stderr, "\n");
  }

  mode = bitmap_encode_hint_01(bitmap) ;
  print_encode_mode("bitmap_encode_hint_01", mode) ;

// RLE encode bit map
  rle2 = bitmap_encode_be_01(bitmap, NULL, RLE_DEFAULT) ;                       // encoding
  rle  = bitmap_encode_be_01(bitmap, bitmap, RLE_DEFAULT) ;   // in-place encoding
  fprintf(stderr, "RLE encoded bitmap :%d RLE bits , encoding %d RAW bits, max size = %d\n", rle->nrle, rle->elem, 32*rle->size);
  for(i=0 ; i<(rle->nrle+31)/32 ; i++) fprintf(stderr, "%8.8x ",rle->data[i]) ;
  fprintf(stderr, "\n");
  errors = 0 ;
  for(i=0 ; i<(rle->nrle+31)/32 ; i++) if(bitmap->data[i] != rle2->data[i]) errors++ ;
  fprintf(stderr, "in-place encoding errors = %d (%d|%d elements)\n", errors, (rle->nrle+31)/32, (bitmap->nrle+31)/32) ;

// try to restore from RLE coded bitmap
  for(i=0 ; i<NPTS ; i++) restored[i] = iarray[i] ;
  for(i=EVERY+1 ; i<NPTS ; i+=EVERY) restored[i] = -special ;
  for(i=NPTS-3 ; i<NPTS ; i++) restored[i] = -special ;
  restored[0] = -special ;
  int nrestore = bitmap_restore_be_01(restored, bitmap, special, NPTS) ;
  fprintf(stderr, "restore from RLE encoding : nrestore = %d\n", nrestore) ;
  errors = 0 ;
  for(i=0 ; i<NPTS ; i++){
    if(iarray[i] != restored[i]) {
      if(errors < 10) fprintf(stderr, "i = %d, expected %d, got %d\n", i, iarray[i], restored[i]);
      errors++ ;
    }
  }
  fprintf(stderr, "RLE bitmap restore errors = %d\n", errors) ;

// decode RLE encoded bitmap
  fprintf(stderr, "bitmap2 O.K. for up to %d bits\n", 32 * bitmap2->size) ;
  for(i=0 ; i<bitmap2->size ; i++) bitmap2->data[i] = 0xFFFFFFFFu ;
  bitmap3 = bitmap_decode_be_01(bitmap2, rle2) ;
  fprintf(stderr, "decoded RLE bitmap :\n");
  for(i0=0 ; i0<NPTS/32 ; i0+=16){
    for(i=0 ; i<16 ; i++) fprintf(stderr, "%8.8x ", bitmap3->data[i0+i]) ;
    fprintf(stderr, "\n");
  }

// restore from bit map
  for(i=0 ; i<NPTS ; i++) restored[i] = iarray[i] ;
  for(i=EVERY+1 ; i<NPTS ; i+=EVERY) restored[i] = -special ;
  for(i=NPTS-3 ; i<NPTS ; i++) restored[i] = -special ;
  restored[0] = -special ;
  bitmap_restore_be_01(restored, bitmap3, special, NPTS) ;
  errors = 0 ;
  for(i=0 ; i<NPTS ; i++){
    if(iarray[i] != restored[i]) {
      if(errors < 10) fprintf(stderr, "i = %d, expected %d, got %d\n", i, iarray[i], restored[i]);
      errors++ ;
    }
  }
  fprintf(stderr, "bitmap restore errors = %d\n", errors) ;

//   TIME_LOOP_EZ(1000, NPTS, bitmap_be_eq_01(iarray, bitmap, special, mmask, NPTS)) ;
//   fprintf(stderr, "bitmap_be_eq_01 : %s\n",timer_msg);

  TIME_LOOP_EZ(1000, NPTS, bitmap_be_int_01(iarray, bitmap, iarray[NPTS/2], mmask, NPTS, OP_SIGNED_GT)) ;
  fprintf(stderr, "bitmap_be_int_01 : %s\n",timer_msg);

  TIME_LOOP_EZ(1000, NPTS, bitmap_be_fp_01(farray, bitmap, farray[NPTS/2], mmask, NPTS, OP_SIGNED_GT)) ;
  fprintf(stderr, "bitmap_be_fp_01  : %s\n",timer_msg);

  for(i=0 ; i<NPTS ; i++) iarray[i] = i - NPTS/2 ;
  for(i=1 ; i<NPTS ; i+=(31)) iarray[i] = special ;
  bitmapd = bitmap_be_eq_01(iarray, NULL, special, mmask, NPTS) ;

  TIME_LOOP_EZ(1000, NPTS, bitmap_be_eq_01(iarray, bitmapd, special, mmask, NPTS)) ;
  fprintf(stderr, "bitmap_be_eq_01  : %s\n",timer_msg);

  TIME_LOOP_EZ(1000, NPTS, mode = bitmap_encode_hint_01(bitmapd)) ;
  fprintf(stderr, "encode_hint_01   : %s\n",timer_msg);

  TIME_LOOP_EZ(1000, NPTS, rle = bitmap_encode_be_01(bitmapd, rle, RLE_DEFAULT)) ;
  fprintf(stderr, "encode_be_01     : %s\n",timer_msg);

  TIME_LOOP_EZ(1000, NPTS, bitmap3 = bitmap_decode_be_01(bitmapd, rle)) ;
  fprintf(stderr, "decode_be_01     : %s\n",timer_msg);

  TIME_LOOP_EZ(1000, NPTS, bitmap_restore_be_01(restored, bitmapd, special, NPTS)) ;
  fprintf(stderr, "restore_be_01    : %s\n",timer_msg);
}
