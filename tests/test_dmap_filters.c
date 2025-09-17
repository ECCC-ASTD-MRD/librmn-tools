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
#include <rmn/move_blocks.h>
#include <rmn/split_dimension.h>
#include <rmn/quantizers.h>

// end of section to be moved to dmap_filters.c

#define NI 95
#define NJ 65

int main(int argc, char **argv){
  bitstream *stream = NULL ;
  bitstream *str000 = NULL ;
  bitstream *str001 = NULL ;
  bitstream *str002 = NULL ;
  bitstream *str003 = NULL ;

// on intuitive order to get rid of warnings about skipping initialization code
  goto code ;

end:
  fprintf(stderr, "SUCCESS\n") ;
  return 0 ;

fail:
  fprintf(stderr, "filter test : available data in stream %ld bits\n", StreamAvailableBits(stream)) ;
  fprintf(stderr, "FAIL\n") ;
  return 1 ;

code:
  if(argc > 500) goto fail ;

  dmap_filter_args_ptr dpfa[10] ;
  dmap_filter_list dpfl = &dpfa[0] ;
  dmap_filter_arg_001 arg_001a = { 0001, 1.0f, 2.0f } ;
  dmap_filter_arg_001 arg_001b = { 0001, 10.0f, 20.0f } ;
//   dmap_filter_arg_001 arg_001a = { 0001, 5, 6 } ;
  dmap_encode_arg arg_006a = { 0006,  32 } ;    // raw encoding, 32 bits per item
  dmap_encode_arg arg_006b = { 0006,  99 } ;    // BHW encoding, 8/16/24/32 bits per item
  dmap_encode_arg arg_006c = { 0006, 132 } ;    // raw encoding, 32 bits per item, zigzag
  dmap_encode_arg arg_006d = { 0006, 208 } ;    // tile encoding, 8 x 8 tiles
  dmap_filter_arg_002 arg_002a = { 0002, 0 } ;
  dmap_fp_quantize arg_003a ; // = { 0003, .25f, 12, 0, FP_QUANTIZE_LIN } ;
  dmap_filter_arg_036 arg_036z = { 0036 } ;
  dmap_filter_arg_036 arg_177n = { 0177 } ;
  array_2d a2d, b2d, c2d, d2d ;
//   block_properties bp2d ;
  ssize_t status ;
  uint64_t freq ;
  double nano ;
  int i, j, debug_mode, strict_mode, errors ;
  float z[NJ][NI] ;
  int zi[NJ][NI] ;
  int zo[NJ*2][NI] ;
  float r[NJ][NI] ;
  float Z[NJ][NI] ;
  uint32_t buffer[NI*NJ*2] ;
  ssize_t tot_status ;
//   TIME_LOOP_DATA ;

// dummy code to avoid warnings
  if(argc > 1 && argv[1] == NULL) goto fail ;
  if(argc > 100) goto end ;

  freq = cycles_counter_freq() ;
  nano = 1000000000 ;
  nano /= freq ;

  start_of_test("C dmap_filter test");

  fprintf(stderr, "============================== base test ==============================\n") ;
//   fprintf(stderr, "nano = %0.3f\n", nano) ;
//   for(i=0 ; i<MAX_DP_FILTERS+10 ; i++){
//     if(dmap_filter_exists(i)) fprintf(stderr, "filter %2d address : %16p, name = %s\n",
//       i, (void *)dmap_filter_get(i), dmap_filter_name(i) ) ;
//   }
//   fprintf(stderr, "\n");

  new_array(&a2d, (void *)&z, sizeof(float), float_data, NI, NJ) ;
  new_array(&b2d, (void *)&r, sizeof(float), raw_data,   NI, NJ) ;
  for(j=0 ; j<NJ ; j++){
    for(i=0 ; i<NI ; i++){
      z[j][i]  = (i - (NI-1)*.5f) + (j - (NJ-1)*.5f) ;
      Z[j][i]  = z[j][i] ;
      r[j][i]  = 999999.0f ;
      zi[j][i] = (i << 8) | (j) ;
      zo[j][i] = 0x0F0F0F0F ;
    }
  }

//   debug_mode  = dmap_debug_mode(1)  ; dmap_debug_mode(debug_mode) ;
//   strict_mode = dmap_strict_mode(0) ; dmap_strict_mode(strict_mode) ;

  new_array(&c2d, (void *)&zi, sizeof(int), int_data, NI, NJ) ;
  new_array(&d2d, (void *)&zo, sizeof(int), int_data, NI, NJ) ;
  STREAM_CREATE(str000, buffer, sizeof(buffer), 0) ;

  dpfl[0] = (dmap_filter_args_ptr)&arg_006a ;     // filter 006, raw 32 bit encoding
  dpfl[1] = NULL ;                                // end of filter list
  STREAM_INSERT_BEGIN(*str000) ;
  fprintf(stderr, "filter test : available space in str000 %ld bits\n", StreamAvailableSpace(str000)) ;
  status = dmap_filter_fwd((array_nd *)&c2d, NULL, dpfl, str000) ;
  if(status < 0) goto fail ;
  fprintf(stderr, "filter test : bits inserted = %ld\n\n", status) ;

  STREAM_REWIND(*str000, 1) ;
  fprintf(stderr, "filter test : available bits in str000 = %ld bits\n", StreamAvailableBits(str000)) ;
for(i=0 ; i<8 ; i++) fprintf(stderr, "%8.8x ", buffer[i]) ;
fprintf(stderr, " BIT-STREAM\n");
  c2d.data = NULL ;
  tot_status = dmap_filter_inv((array_nd *)&d2d, str000) ;
  errors = 0 ;
  for(j=0 ; j<NJ ; j++){
    for(i=0 ; i<NI ; i++){
      if(zi[j][i] != zo[j][i]){
        if(errors < 5) fprintf(stderr, "i=%d , j=%d, expecting %8.8x, got %8.8x\n", i, j, zi[j][i], zo[j][i]) ;
        errors ++ ;
      }
    }
  }
  fprintf(stderr, "filter test : errors = %d\n", errors) ;

  goto end ;

  dpfl[0] = (dmap_filter_args_ptr)&arg_001a ;     // filter 001
  dpfl[1] = (dmap_filter_args_ptr)&arg_001b ;     // filter 001
  dpfl[2] = (dmap_filter_args_ptr)&arg_006a ;     // filter 006
  dpfl[3] = (dmap_filter_args_ptr)&arg_002a ;     // filter 002
  dpfl[4] = (dmap_filter_args_ptr)&arg_036z ;     // undefined filter 036
  dpfl[5] = (dmap_filter_args_ptr)&arg_177n ;     // invalid filter 127
  dpfl[6] = NULL ;                                // end of filter list
  arg_003a = DMAP_FP_QUANTIZE( .maxerr = .25f, .maxsig = 0.0f, .nbits = 12, .offset = 0x7FFFFFFF, .mode = FP_QUANTIZE_LIN ) ;
//   arg_003a = DMAP_FP_QUANTIZE( .maxerr = .25f, .nbits = 12, .offset = 0, .mode = FP_QUANTIZE_LIN ) ;
//   arg_003a = DMAP_FP_QUANTIZE( .maxerr = .00000025f, .nbits = 8, .offset = 0x7FFFFFFF, .mode = FP_QUANTIZE_LIN ) ;
//   arg_003a = DMAP_FP_QUANTIZE( .maxerr = .00000025f, .nbits = 8, .offset = 0, .mode = FP_QUANTIZE_LIN ) ;
  dpfl[0] = (dmap_filter_args_ptr)&arg_003a ;     // filter 003, linear float quantizer
//   dmap_encode_arg arg_006 = DMAP_ENCODE( .mode = 116 ) ;
  dmap_lorenzo_arg arg_004 = DMAP_LORENZO() ;
  dmap_wavelet_arg arg_005 = DMAP_WAVELET(.levels = 2 ) ;
  dmap_encode_arg arg_006 = DMAP_ENCODE( .mode = 0 ) ;
  dpfl[1] = (dmap_filter_args_ptr)&arg_005 ;     // integer wavelet
  dpfl[1] = (dmap_filter_args_ptr)&arg_004 ;     // Lorenzo predictor
  dpfl[2] = (dmap_filter_args_ptr)&arg_006 ;     // 32 bit raw encoding
  dpfl[3] = NULL ;                               // end of filter list
// goto array_test ;
  STREAM_CREATE(stream, buffer, sizeof(buffer), 0) ;
  STREAM_INSERT_BEGIN(*stream) ;
  fprintf(stderr, "filter test : available space in stream %ld bits\n", StreamAvailableSpace(stream)) ;

  debug_mode = dmap_debug_mode(1) ;
  strict_mode = dmap_strict_mode(1) ;
//   bp2d = NULL_BLOCK_PROPERTIES ; // data properties are not valid
//   status = dmap_filter_fwd((array_nd *)&a2d, &bp2d, dpfl, stream) ;   // activate forward filter chain
  // no data properties available, pass NULL
  status = dmap_filter_fwd((array_nd *)&a2d, NULL, dpfl, stream) ;   // activate forward filter chain
  if(status < 0) goto fail ;
  debug_mode = dmap_debug_mode(1)   ; dmap_debug_mode(debug_mode) ;
  strict_mode = dmap_strict_mode(0) ; dmap_strict_mode(strict_mode) ;

  fprintf(stderr, "filter test : bits inserted = %ld\n", status) ;
  STREAM_FLUSH(*stream) ;
  STREAM_INSERT_ALIGN32(*stream) ;        // align to a 32 bit boundary
  STREAM_REWIND(*stream, 1) ;
  fprintf(stderr, "filter test : available data in stream %ld bits\n", StreamAvailableBits(stream)) ;

  for(i=0 ; i<12 ; i++) fprintf(stderr, "%8.8x ", buffer[i]) ;
  fprintf(stderr, "\n");

  fprintf(stderr, "filter test : old b2d address = %p\n", (void *)b2d.data) ;
  b2d.data = NULL ;
  tot_status = dmap_filter_inv((array_nd *)&b2d, stream) ;
  fprintf(stderr, "filter test : new b2d address = %p\n", (void *)b2d.data) ;
  fprintf(stderr, "filter test : bits extracted = %ld\n", tot_status) ;
  fprintf(stderr, "filter test : available data in stream %ld bits\n", StreamAvailableBits(stream)) ;
  STREAM_XTRACT_ALIGN32(*stream) ;        // align to a 32 bit boundary
  fprintf(stderr, "filter test : available data in stream %ld bits\n", StreamAvailableBits(stream)) ;

  errors = NI*NJ ;
//   float *pz = (float *) a2d.data ;
  float *pz = (float *) Z ;
  float *pr = (float *) b2d.data ;
  block_properties bp_out, bp_in ;
  analyze_data32_block((void *) pz, NI, NI, NJ, &bp_in)  ; adjust_block_properties(&bp_in,  float_data) ;
  print_float_props(bp_in) ;
  analyze_data32_block((void *) pr, NI, NI, NJ, &bp_out) ; adjust_block_properties(&bp_out, float_data) ;
  print_float_props(bp_out);
  float maxdiff = 0.0f ;
  for(i=0 ; i<NI*NJ ; i++){
    float err = pz[i] - pr[i] ;
    err = (err < 0) ? -err : err ;
    maxdiff = (err > maxdiff) ? err : maxdiff ;
    if(pz[i] == pr[i]) errors-- ;
  }
  if(errors > 0) fprintf(stderr, "%f %f %f %f\n", z[0][0], z[NJ-1][NI-1], r[0][0], r[NJ-1][NI-1]) ;
  fprintf(stderr, "filter test : %d differences between r and z (%d values), max = %f\n", errors, NI*NJ, maxdiff) ;
  if(errors > 0) goto fail ;
  fprintf(stderr, "SUCCESS\n") ;
goto end ;
// array_test:
  fprintf(stderr, "============================== array test ==============================\n") ;
#undef NI
#undef NJ
#define NI 194
#define NJ 62
#define BLOCKSIZE 64
  float z2[NJ][NI] ; // , r2[NJ][NI] ;   // 3 tiles horizontally, 1 tile vertically
  uint32_t buf2[NI*NJ*2] ;         // enough space for stream packing
//   bitstream *stream2 = NULL ;
  dmap_filter_arg_003 quantize = DMAP_FILTER_003( .maxerr = .25f, .nbits = 12) ;
  dmap_filter_arg_006 encode   = DMAP_FILTER_006( .mode = 24 ) ;

  dpfl[0] = (dmap_filter_args_ptr)&quantize ;     // filter 003, linear float quantizer
  dpfl[1] = (dmap_filter_args_ptr)&encode ;       // filter 006, bit encoder
  dpfl[1] = NULL ;                                // end of filter list

  STREAM_CREATE(stream, buf2, sizeof(buf2), 0) ;
  STREAM_INSERT_BEGIN(*stream) ;

  for(j=0 ; j<NJ ; j++){
    for(i=0 ; i<NI ; i++){
      z2[j][i] = j * ( i - (NI-1)*.5f ) ;
//       r2[j][i] = 999999.0 ;
    }
  }
  int i0, j0 ;
  array_2d z2d ;    // GLOBAL array
  new_array(&z2d, (void *)&z2, sizeof(float), float_data, NI, NJ) ;

  array_axis iaxis, jaxis ;
  iaxis = split_axis(NI, BLOCKSIZE) ;
  jaxis = split_axis(NJ, BLOCKSIZE) ;
  for(j0 = 0 ; j0<jaxis.nbk ; j0++){
    index_range jrange = block_limits(jaxis, j0) ;
    int jsize = jrange.ixn - jrange.ix0 + 1 ;
    for(i0=0 ; i0<iaxis.nbk ; i0++){
      //  get the float block to process
      index_range irange = block_limits(iaxis, i0) ;
      int isize = irange.ixn - irange.ix0 + 1 ;
      fprintf(stderr, "block[%d,%d] limits = [%3d:%3d,%3d:%3d] (%3dx%3d)\n", i0, j0, irange.ix0, irange.ixn, jrange.ix0, jrange.ixn, isize, jsize) ;
      if(2 != set_array_lbounds(&z2d, irange.ix0, irange.ixn, jrange.ix0, jrange.ixn)) goto fail ;

      float block[jsize][isize] ;     // storage for BLOCK
      array_2d b2d ;                  // LOCAL BLOCK
      new_array(&b2d, (void *)&block, sizeof(float), float_data, isize, jsize) ;
      block_properties bp ;
      float *src = (float *)subarray_address(&z2d) ; // address within z2d
      if(isize*jsize != move_w32_block(src, NI, block, isize, isize, jsize, &bp)) goto fail ;   // copy from GLOBAL to LOCAL BLOCK
      b2d.type = float_data ;                                                                   // LOCAL contains floats

      float xmin, xmax ;
      xmin = xmax = z2[jrange.ix0][irange.ix0] ;
      for(j=jrange.ix0 ; j<=jrange.ixn ; j++){      // explicit check for min and max
        for(i=irange.ix0 ; i<=irange.ixn ; i++){
          xmin = (z2[j][i] < xmin) ? z2[j][i] : xmin ;
          xmax = (z2[j][i] > xmax) ? z2[j][i] : xmax ;
        }
      }
      print_float_props(bp) ;
      if(xmin != FLOAT_MIN_VALUE(bp) || xmax != FLOAT_MAX_VALUE(bp)) {
        fprintf(stderr, "expected : min = %f, max = %f, found : min = %f, max = %f\n", xmin, xmax, FLOAT_MIN_VALUE(bp), FLOAT_MAX_VALUE(bp)) ;
        goto fail ;
      }
      // process subarray
      status = dmap_filter_fwd((array_nd *)&b2d, &bp, dpfl, stream) ;   // activate forward filter chain
      if(status < 0) goto fail ;
      debug_mode = dmap_debug_mode(1)   ; dmap_debug_mode(debug_mode) ;

      fprintf(stderr, "filter test : bits inserted = %ld\n", status) ;
      STREAM_FLUSH(*stream) ;
      STREAM_INSERT_ALIGN32(*stream) ;        // align to a 32 bit boundary
      fprintf(stderr, "filter test : available data in stream %ld bits\n\n", StreamAvailableBits(stream)) ;
// break ;
    }
  }

  goto end ;
}
