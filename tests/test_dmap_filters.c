// Hopefully useful code for C
// Copyright (C) 2025  Recherche en Prevision Numerique
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
// Author:
//     M. Valin,   Recherche en Prevision Numerique, 2025
//
#include <stdio.h>

#include <rmn/timers.h>
#include <rmn/test_helpers.h>
#include <rmn/dmap_filters.h>

// end of section to be moved to dmap_filters.c

#define NI 95
#define NJ 65

int main(int argc, char **argv){
  dmap_filter_args_ptr dpfa[10] ;
  dmap_filter_list dpfl = &dpfa[0] ;
  dmap_filter_arg_000 arg_000a = { 0000, 1.0f, 2.0f } ;
  dmap_filter_arg_000 arg_000b = { 0000, 10.0f, 20.0f } ;
  dmap_filter_arg_001 arg_001a = { 0001, 5, 6 } ;
  dmap_filter_arg_002 arg_002a = { 0002, 0 } ;
  dmap_filter_arg_003 arg_000z = { 0003 } ;
  dmap_filter_arg_003 arg_000n = { 0177 } ;
  array_2d a2d ;
  block_properties bp2d ;
  bitstream *stream ;
  ssize_t status ;
  uint64_t freq ;
  double nano ;
  int i, j ;
  float z[NJ][NI] ;
  uint32_t buffer[NI*NJ*2] ;
  uint32_t unfilter ;
//   TIME_LOOP_DATA ;

// dummy code to avoid warnings
  if(argc > 1 && argv[1] == NULL) goto fail ;
  if(argc > 100) goto end ;

  freq = cycles_counter_freq() ;
  nano = 1000000000 ;
  nano /= freq ;

  start_of_test("C dmap_filter test");

  fprintf(stderr, "============================== base test ==============================\n") ;
  fprintf(stderr, "nano = %0.3f\n", nano) ;
  for(i=0 ; i<MAX_DP_FILTERS+10 ; i++){
    if(dmap_filter_exists(i)) fprintf(stderr, "filter %2d address : %16p, name = %s\n",
      i, (void *)dmap_filter_get(i), dmap_filter_name(i) ) ;
  }

  new_array(&a2d, (void *)&z, sizeof(float), float_data, NI, NJ) ;
  for(j=0 ; j<NJ ; j++){
    for(i=0 ; i<NI ; i++){
      z[j][i] = (i - (NI-1)*.5f) + (j - (NJ-1)*.5f) ;
    }
  }

  dpfl[0] = (dmap_filter_args_ptr)&arg_000a ;     // filter 000
  dpfl[1] = (dmap_filter_args_ptr)&arg_000b ;     // filter 000
  dpfl[2] = (dmap_filter_args_ptr)&arg_001a ;     // filter 001
  dpfl[3] = (dmap_filter_args_ptr)&arg_002a ;     // filter 002
  dpfl[4] = (dmap_filter_args_ptr)&arg_000z ;     // undefined filter 003
  dpfl[5] = (dmap_filter_args_ptr)&arg_000n ;     // invalid filter 127
  dpfl[6] = NULL ;                                // end of filter list
  dpfl[6] = NULL ;

  STREAM_CREATE(stream, buffer, sizeof(buffer), 0) ;
  STREAM_INSERT_BEGIN(*stream) ;
  fprintf(stderr, "filter test : available space in stream %ld bits\n", StreamAvailableSpace(stream)) ;
  status = dmap_filter_000((array_nd *)&a2d, &bp2d, dpfl, stream) ;   // activate filter chain
  STREAM_PUT_NBITS(*stream, 255, 8) ;
  fprintf(stderr, "filter test : status = %ld\n", status) ;
  STREAM_FLUSH(*stream) ;
  STREAM_INSERT_ALIGN32(*stream) ;
  STREAM_REWIND(*stream, 1) ;
  fprintf(stderr, "filter test : available data in stream %ld bits\n", StreamAvailableBits(stream)) ;
//   for(i=0 ; i<8 ; i++) fprintf(stderr, "%8.8x ", buffer[i]) ;
//   fprintf(stderr, "\n");
  STREAM_XTRACT_CHECK(*stream) ;
  STREAM_PEEK_NBITS(*stream, unfilter, 8) ;
  while(unfilter >=0 && unfilter < MAX_DP_FILTERS){
    fprintf(stderr, "filter test : reverse filter id = %d\n", unfilter) ;
    dmap_filter_ptr unfilter_ptr = dmap_filter_get(unfilter) ;
    status = (*unfilter_ptr)((array_nd *)&a2d, NULL, NULL, stream) ;
    STREAM_PEEK_NBITS(*stream, unfilter, 8) ;
    if(StreamAvailableBits(stream) < 8) break ;
  }
  STREAM_GET_NBITS(*stream, unfilter, 8) ;
  fprintf(stderr, "filter test : last id = %d\n", unfilter) ;
  STREAM_XTRACT_ALIGN32(*stream) ;
  fprintf(stderr, "filter test : available data in stream %ld bits\n", StreamAvailableBits(stream)) ;

end:
  fprintf(stderr, "SUCCESS\n") ;
  return 0 ;
fail:
  fprintf(stderr, "FAIL\n") ;
  return 1 ;
}
