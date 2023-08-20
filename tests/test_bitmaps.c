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

#include <rmn/bitmaps.h>
#include <rmn/bitmaps.h>
#include <rmn/timers.h>

#define NPTS 512
#define EVERY 127

int main(int argc, char **argv){
  int32_t array[NPTS] ;
  int32_t restored[NPTS] ;
  rmn_bitmap *bitmap = NULL ;
  int i0, i, errors ;
  int32_t special = 999999 ;
  uint32_t mmask = 0u ;
  TIME_LOOP_DATA ;
  stream32 stream ;

  for(i=0 ; i<NPTS ; i++) array[i] = i - NPTS/2 ;
  for(i=EVERY ; i<NPTS ; i+=EVERY) array[i] = special ;
  for(i=NPTS-3 ; i<NPTS ; i++) array[i] = special ;
  array[0] = special ;
  for(i=0 ; i<NPTS ; i++) if(array[i] == special) fprintf(stderr, "%d ",i) ;
  fprintf(stderr, "\n\n");

// build bit map
  bitmap = bitmask_be_01(array, NULL, special, mmask, NPTS) ;
  for(i0=0 ; i0<NPTS/32-15 ; i0+=16){
    for(i=0 ; i<16 ; i++) fprintf(stderr, "%8.8x ", bitmap->bm[i0+i]) ;
    fprintf(stderr, "\n");
  }
  bitmap_encode_be_01(bitmap, NULL) ;

// decode bit map
  for(i=0 ; i<NPTS ; i++) restored[i] = array[i] ;
  for(i=EVERY ; i<NPTS ; i+=EVERY) restored[i] = -special ;
  bitmask_restore_be_01(restored, bitmap, special, NPTS) ;
  errors = 0 ;
  for(i=0 ; i<NPTS ; i++){
    if(array[i] != restored[i]) {
      if(errors < 10) fprintf(stderr, "i = %d, expected %d, got %d\n", i, array[i], restored[i]);
      errors++ ;
    }
  }
  fprintf(stderr, "bitmap restore errors = %d\n", errors) ;

  TIME_LOOP_EZ(1000, NPTS, bitmask_be_01(array, bitmap, special, mmask, NPTS)) ;
  fprintf(stderr, "bitmask_be_01    : %s\n",timer_msg);

  TIME_LOOP_EZ(1000, NPTS, bitmask_restore_be_01(restored, bitmap, special, NPTS)) ;
  fprintf(stderr, "bitmask_be_01    : %s\n",timer_msg);
}
