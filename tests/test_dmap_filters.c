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
#include <string.h>

#include <rmn/timers.h>
#include <rmn/test_helpers.h>
#include <rmn/dmap_filters.h>
#include <rmn/move_blocks.h>
#include <rmn/split_dimension.h>
#include <rmn/quantizers.h>
#include <rmn/fp_qlin.h>
#include <rmn/fp_qflog.h>
#include <rmn/eval_diff.h>

// end of section to be moved to dmap_filters.c

#define NI 95
#define NJ 65

// will be transferred to dmap_filters.c when finished
// static void dmap_filter_array(array_nd *a, dmap_filter_list dpfl, bitstream *stream){
//   (void) (a) ;
//   (void) (dpfl) ;
//   (void) (stream) ;
// }

int main(int argc, char **argv){
  bitstream *estream = NULL ;
  bitstream *stream = NULL ;
  bitstream *str000 = NULL ;
  char *errmsg = "" ;

// non intuitive order of labels to get rid of warnings about skipping initialization code when jumping to end or fail
  goto process ;

end:
  fprintf(stderr, "SUCCESS\n") ;
  return 0 ;

fail:
  if(estream) fprintf(stderr, "filter test : available data in stream %ld bits\n", StreamAvailableBits(estream)) ;
  if(estream) fprintf(stderr, "filter test : available space in stream %ld bits\n", StreamAvailableSpace(estream)) ;
  fprintf(stderr, "FAIL : %s\n", errmsg) ;
  return 1 ;

process:
  if(argc > 1 && argv[1] == NULL) goto fail ;       // dummy code to avoid warnings

  dmap_filter_args_ptr dpfa[16] ;
  dmap_filter_list dpfl = &dpfa[0] ;
  dmap_filter_arg_001 arg_001a = DMAP_SAXPY( .iscale = 1,    .ioffset = 2   ) ;
//   arg_001a = DMAP_SAXPY( {1},  {2}) ;
  dmap_filter_arg_001 arg_001b = DMAP_SAXPY( .scale  = 1.0f, .offset  = 0.0f) ;
  arg_001b = DMAP_SAXPY({10.0f}, {20.0f}) ;
  dmap_encode_arg arg_006a = DMAP_FILTER_006(100, 0) ;
  dmap_filter_arg_002 arg_002a = { 0002, 127 } ;
  dmap_fp_quantize arg_003a ; // = { 0003, .25f, 12, 0, FP_QUANTIZE_LIN } ;
//   dmap_filter_arg_036 arg_036z = DMAP_FILTER_036() ;
  array_2d a2d, b2d, g2d, f2d, i2d, o2d, r2d ;
  array_1d i1d, o1d, r1d ;
//   block_properties bp2d ;
  ssize_t status = 0 ;
  uint64_t freq ;
  double nano ;
  int i, j, debug_mode, strict_mode, errors ;
  float    zi[NJ][NI], zo[NJ][NI], zr[NJ][NI] ;    // input, output, reference (float)
  float    fi[NJ][NI], fo[NJ][NI], fr[NJ][NI] ;    // input, output, reference (float)
  uint32_t ui[NJ][NI], uo[NJ][NI], ur[NJ][NI] ;    // input, output, reference (unsigned int)
  uint32_t buffer[NI*NJ*2] ;
  ssize_t tot_status ;
  int test_no ;
//   TIME_LOOP_DATA ;

  freq = cycles_counter_freq() ;
  nano = 1.0E+9 ;
  nano /= freq ;
  if(nano == 0.0) fprintf(stderr, "nano == 0 !!\n") ;

  start_of_test("C dmap_filter test");

  fprintf(stderr, "============================== base test ==============================\n") ;
//   fprintf(stderr, "nano = %0.3f\n", nano) ;
//   for(i=0 ; i<MAX_DP_FILTERS+10 ; i++){              // list registered filters
//     if(dmap_filter_exists(i)) fprintf(stderr, "filter %2d address : %16p, name = %s\n",
//       i, (void *)dmap_filter_get(i), dmap_filter_name(i) ) ;
//   }
//   fprintf(stderr, "\n");

  for(j=0 ; j<NJ ; j++){
    for(i=0 ; i<NI ; i++){
      fr[j][i]  = (i - (NI-1)*.5f) + (j - (NJ-1)*.5f) ;   // float reference
      ur[j][i] = ((i + 1) << 8) | (j + 1) ;               // unsigned integer reference
      zr[j][i] = ur[j][i] ;
      fi[j][i]  = (i - (NI-1)*.5f) + (j - (NJ-1)*.5f) ;
//       fr[j][i]  = fi[j][i] ;
      fo[j][i]  = 999999.0f ;
    }
  }

  debug_mode  = dmap_debug_mode(1)  ; dmap_debug_mode(debug_mode) ;
  strict_mode = dmap_strict_mode(0) ; dmap_strict_mode(strict_mode) ;

  STREAM_CREATE(str000, buffer, sizeof(buffer), 0) ;   // create stream for reading and writing

  new_array(&r2d, (void *)&ur, sizeof(int), int_data,   NI, NJ) ;  // integer source data
  new_array(&r1d, (void *)&ur, sizeof(int), int_data,   NI*NJ) ;   // 1D version of above
  new_array(&i2d, (void *)&ui, sizeof(int), int_data,   NI, NJ) ;  // used as input data
  new_array(&i1d, (void *)&ui, sizeof(int), int_data,   NI*NJ) ;   // 1D version of above
  new_array(&o2d, (void *)&uo, sizeof(int), int_data,   NI, NJ) ;  // used as output data
  new_array(&o1d, (void *)&uo, sizeof(int), int_data,   1) ;   // 1D version of above
  new_array(&f2d, (void *)&zr, sizeof(int), float_data, NI, NJ) ;  // float source data
  new_array(&a2d, (void *)&zi, sizeof(float), float_data, NI, NJ) ;
  new_array(&g2d, (void *)&zo, sizeof(float), float_data, NI, NJ) ;
  new_array(&b2d, (void *)&zo, sizeof(float), raw_data,   NI, NJ) ;

  char *test_nam0[4] = { "LIN 00", "LINo16", "LINo00", "FLOG16" } ;
  for(test_no = 0 ; test_no < 4 ; test_no++){
//   for(test_no = 0 ; test_no < 1 ; test_no++){
    fprintf(stderr, "============================== float quantize test %d start ==============================\n", test_no) ;
    float abs_err = 07.5f ;
    STREAM_INIT(str000, NULL, 0, 0) ;               // full RW stream reset (keep buffer, size, and mode)
    dmap_fp_quantize arg_003 = DMAP_FP_QUANTIZE(.mode = -1) ;
    dmap_lorenzo_arg arg_004 = DMAP_LORENZO() ;
    dmap_no_op7      arg_007 = DMAP_NO_OP7() ;
    dmap_wavelet_arg arg_005 = DMAP_WAVELET(1) ;
    dmap_saxpy_arg   arg_001 = DMAP_SAXPY({0.0f}, {1.0f}) ;
    dmap_no_op2      arg_002 = DMAP_NO_OP2(123) ;
//     dmap_encode_arg  arg_006 = DMAP_ENCODE(.mode= 32, .options=0) ; // filter 006, raw, 32 bits per item
    dmap_encode_arg  arg_006 = DMAP_ENCODE(.mode= 104, .options=0) ; // filter 006, tile encoding

    switch(test_no){
      case 0 :
        arg_003 = DMAP_FP_QUANTIZE(.mode = FP_2_INT,  .offset = 0,           .nbits =  0, .maxerr = abs_err) ;
        break ;
      case 1 :
        arg_003 = DMAP_FP_QUANTIZE(.mode = FP_2_INT,  .offset = 0x7FFFFFFF,  .nbits = 16, .maxerr = abs_err) ;
        break ;
      case 2 :
        arg_003 = DMAP_FP_QUANTIZE(.mode = FP_2_INT,  .offset = 0x7FFFFFFF,  .nbits =  0, .maxerr = abs_err) ;
        break ;
      case 3 :
        arg_003 = DMAP_FP_QUANTIZE(.mode = FP_2_FLOG, .offset = 0,           .nbits = 16, .maxerr = abs_err) ;
        break ;
      default : goto fail ;
    }
    dpfl[0] = (dmap_filter_args_ptr)&arg_001 ;    // neutral saxpy
    dpfl[1] = (dmap_filter_args_ptr)&arg_003 ;    // linear quantizer
    dpfl[2] = (dmap_filter_args_ptr)&arg_004 ;    // Lorenzo predictor
    dpfl[3] = (dmap_filter_args_ptr)&arg_007 ;    // pass through
    dpfl[4] = (dmap_filter_args_ptr)&arg_005 ;    // 0 level wavelet
    dpfl[5] = (dmap_filter_args_ptr)&arg_002 ;    // pass through with flag
    dpfl[6] = (dmap_filter_args_ptr)&arg_006 ;    // tile encoder
    dpfl[7] = (dmap_filter_args_ptr)&arg_007 ;    // pass through
    dpfl[7] = FILTER_LIST_END ;                   // comment to test error return propagation
    dpfl[8] = FILTER_BLOCK_END ;
    dmap_print_parameters(dpfl) ;

    fprintf(stderr, "filter test : available space in str000 %ld bits, available bits = %ld bits\n", StreamAvailableSpace(str000), StreamAvailableBits(str000)) ;
    if((NI * NJ) != copy_array_data(&f2d, &a2d)) goto fail ;
    a2d.type = float_data ;

    status = dmap_filter_enc((array_nd *)&a2d, NULL, dpfl, str000) ;      // forward filter
    estream = str000 ;
    errmsg = "forward filter failed" ;
    if(status < 0){
      fprintf(stderr, "filter test : status = %4.4lo, error 0%2.2lo in filter 0%2.2lo\n", -status, (-status) & 077, (-status) >> 6) ;
      goto fail ;
    }
    fprintf(stderr, "filter test : '%s' , bits inserted = %ld\n\n", test_nam0[test_no], status) ;

    STREAM_REWIND(*str000, 1) ;
    fprintf(stderr, "filter test : available space in str000 %ld bits, available bits = %ld bits\n", StreamAvailableSpace(str000), StreamAvailableBits(str000)) ;

    set_array_value(&g2d, 0x0F, ARRAY_BYTES) ;                            // set output to nonsense
    array_set_empty(&g2d) ;
    tot_status = dmap_filter_dec((array_nd *)&g2d, str000) ;              // inverse filter
    errmsg = "inverse filter failed" ;
    if(tot_status < 0){
      fprintf(stderr, "filter test : status = %5.5lo\n", -tot_status) ;
      goto fail ;
    }
    errmsg = "encode/decode bit count mismatch" ;
    if(status != tot_status){ fprintf(stderr, "encoded = %ld, decoded = %ld\n", status, tot_status) ; }
    if(status != tot_status) goto fail ;

    int32_t notsame = array_compare_2D(NI, NJ, (void *)zr, (void *)zo, 0) ;
    errors = array_compare_float_2D(NI, NJ, (void *)zr, (void *)zo, abs_err) ;
    fprintf(stderr, "filter test : '%s' ,  bits extracted = %ld, nvalues = %d, errors = %d, not same = %d\n",
                    test_nam0[test_no], tot_status, NI*NJ, errors, notsame) ;
    errmsg = "data restore failed" ;
    if(errors > 0) goto fail ;
    fprintf(stderr, "filter test : available space in str000 %ld bits, available bits = %ld bits\n", StreamAvailableSpace(str000), StreamAvailableBits(str000)) ;
    STREAM_XTRACT_ALIGN32(*str000) ;
    fprintf(stderr, "filter test : available space in str000 %ld bits, available bits = %ld bits\n", StreamAvailableSpace(str000), StreamAvailableBits(str000)) ;

    fprintf(stderr, "============================== float quantize test %d end ==============================\n", test_no) ;
  }

if(argc < 1000) goto end ;     // suppress unreachable code warning
  char *test_nam1[6] = { "RAW-15", "RAW-24", "RAW-32", "ZIGZAG", "BHW   ", "TILE  " } ;
  for(test_no = 0 ; test_no < 6 ; test_no++){
    fprintf(stderr, "============================== 2D integer encode test %d start ==============================\n", test_no) ;
    STREAM_INIT(str000, NULL, 0, 0) ;               // full RW stream reset (keep buffer, size, and mode)
    dmap_lorenzo_arg arg_004 = DMAP_LORENZO() ;
    dmap_encode_arg  arg_006 ;
    // 15 and 24 bit raw encoding will not work with wavelet transform (transform is deactivated for tests 0 and 1)
    dmap_wavelet_arg arg_005 = DMAP_WAVELET(.levels = ((test_no < 2) ? 0 : 3)) ;    // 3 level (or 0 level) transform
    switch(test_no){
      case 0  : arg_006 = DMAP_ENCODE(.mode= 15, .options=0) ; break ;    // filter 006, raw, 15 bits per item
      case 1  : arg_006 = DMAP_ENCODE(.mode= 24, .options=0) ; break ;    // filter 006, raw, 24 bits per item
      case 2  : arg_006 = DMAP_ENCODE(.mode= 32, .options=0) ; break ;    // filter 006, raw, 32 bits per item
      case 3  : arg_006 = DMAP_ENCODE(.mode= 98, .options=0) ; break ;    // filter 006, zigzag, up to 32 bits per item
      case 4  : arg_006 = DMAP_ENCODE(.mode= 99, .options=0) ; break ;    // filter 006, BHW, auto bits per item
      case 5  : arg_006 = DMAP_ENCODE(.mode=104, .options=0) ; break ;    // filter 006, tile encoding
      default : goto fail ;                                   // invalid test number
    }
    dpfl[0] = (dmap_filter_args_ptr)&arg_005 ;      // wavelet transform
    dpfl[1] = (dmap_filter_args_ptr)&arg_004 ;      // Lorenzo predictor
    dpfl[2] = (dmap_filter_args_ptr)&arg_006 ;      // integer encoder
    dpfl[3] = FILTER_LIST_END ;                     // end of filter list

    fprintf(stderr, "filter test : available space in str000 %ld bits, available bits = %ld bits\n", StreamAvailableSpace(str000), StreamAvailableBits(str000)) ;

    if((NI * NJ) != copy_array_data(&r2d, &i2d)) goto fail ;              // set input data (copy ur into ui), check sizes
    i2d.type = int_data ;

    array_set_used(&i2d) ;
    status = dmap_filter_enc((array_nd *)&i2d, NULL, dpfl, str000) ;      // forward filter
    estream = str000 ;
    errmsg = "forward filter failed" ;
    if(status < 0) goto fail ;
    fprintf(stderr, "filter test : '%s' , bits inserted = %ld\n\n", test_nam1[test_no], status) ;

    STREAM_REWIND(*str000, 1) ;
    fprintf(stderr, "filter test : available space in str000 %ld bits, available bits = %ld bits\n", StreamAvailableSpace(str000), StreamAvailableBits(str000)) ;

    set_array_value(&o2d, 0x0F, ARRAY_BYTES) ;                            // set output to nonsense
    array_set_empty(&o2d) ;
    tot_status = dmap_filter_dec((array_nd *)&o2d, str000) ;              // inverse filter
    errmsg = "inverse filter failed" ;
    if(tot_status < 0) goto fail ;
//     fprintf(stderr, "filter test : '%s' ,  bits extracted = %ld\n", test_nam1[test_no], tot_status) ;
    errmsg = "encode/decode bit count mismatch" ;
    if(status != tot_status) goto fail ;

    errors = array_compare_2D(NI, NJ, (void *)ur, (void *)uo, 1) ;
    fprintf(stderr, "filter test : '%s' ,  bits extracted = %ld", test_nam1[test_no], tot_status) ;
    fprintf(stderr, ", errors = %d\n", errors) ;
    errmsg = "data restore failed" ;
    if(errors > 0) goto fail ;
    fprintf(stderr, "filter test : available space in str000 %ld bits, available bits = %ld bits\n", StreamAvailableSpace(str000), StreamAvailableBits(str000)) ;
    STREAM_XTRACT_ALIGN32(*str000) ;
    fprintf(stderr, "filter test : available space in str000 %ld bits, available bits = %ld bits\n", StreamAvailableSpace(str000), StreamAvailableBits(str000)) ;

    fprintf(stderr, "============================== 2D integer encode test %d end ==============================\n", test_no) ;

  }
//   if(argc < 1000) goto end ;     // suppress unreachable code warning
  char *test_nam2[4] = { "RAW-32", "ZIGZAG", "BHW   ", "TILE  " } ;
  for(test_no = 0 ; test_no < 4 ; test_no++){
    fprintf(stderr, "============================== 1D integer encode test %d start ==============================\n", test_no) ;
    array_2d o1d = array_2d_invalid ;
    array_2d *o1d_p = &o1d ;
    STREAM_INIT(str000, NULL, 0, 0) ;               // full RW stream reset (keep buffer, size, and mode)
    dmap_encode_arg  arg_006 ;
    switch(test_no){
      case 0  : arg_006 = DMAP_ENCODE(.mode=  32, .options=0) ; break ;    // filter 006, raw, 32 bits per item
      case 1  : arg_006 = DMAP_ENCODE(.mode=  98, .options=0) ; break ;    // filter 006, zigzag, up to 32 bits per item
      case 2  : arg_006 = DMAP_ENCODE(.mode=  99, .options=0) ; break ;    // filter 006, BHW, auto bits per item
      case 3  : arg_006 = DMAP_ENCODE(.mode= 100, .options=0) ; break ;    // filter 006, tile encoding
      default : goto fail ;                                   // invalid test number
    }
    dpfl[0] = (dmap_filter_args_ptr)&arg_006 ;
    dpfl[1] = FILTER_LIST_END ;                               // end of filter list

    fprintf(stderr, "filter test : available space in str000 %ld bits, available bits = %ld bits\n", StreamAvailableSpace(str000), StreamAvailableBits(str000)) ;

    if((NI * NJ) != copy_array_data(&r1d, &i1d)) goto fail ;              // set input data (copy ur into ui), check sizes
    i1d.type = int_data ;

    status = dmap_filter_enc((array_nd *)&i1d, NULL, dpfl, str000) ;      // forward filter

    estream = str000 ;
    errmsg = "forward filter failed" ;
    if(status < 0) goto fail ;
    fprintf(stderr, "filter test : '%s' , bits inserted = %ld\n\n", test_nam2[test_no], status) ;

    STREAM_REWIND(*str000, 1) ;
    fprintf(stderr, "filter test : available space in str000 %ld bits, available bits = %ld bits\n", StreamAvailableSpace(str000), StreamAvailableBits(str000)) ;

    o1d = array_2d_null ;
    errmsg = "set_array_value returned a non 0 value" ;
    if(set_array_value(&o1d, 0x0F, ARRAY_BYTES) != 0) goto fail ;
    array_set_empty(&o1d) ;
    fprintf(stderr, "rank of o1d : syntactic = %d/%d/%d, effective = %d, data at %p\n",
                    ARRAY_ALLOC_RANK(o1d), ARRAY_ALLOC_RANK(*o1d_p), o1d.ndim, ARRAY_RANK(o1d), ARRAY_DATA(o1d)) ;
    tot_status = dmap_filter_dec((array_nd *)&o1d, str000) ;              // inverse filter
    fprintf(stderr, "rank of o1d : syntactic = %d/%d/%d, effective = %d, data at %p->%p, flags = %d\n",
                    ARRAY_ALLOC_RANK( o1d ), ARRAY_ALLOC_RANK( *(array_nd *)o1d_p ), o1d.ndim,
                    ARRAY_RANK( *o1d_p ), ARRAY_DATA( *o1d_p ), ARRAY_LIMIT( *o1d_p ), o1d.flags) ;
    if(tot_status < 0){
      fprintf(stderr, "status = %5.5lo\n", -tot_status) ;
      errmsg = "inverse filter failed" ;
      goto fail ;
    }
    errmsg = "encode/decode bit count mismatch" ;
    if(status != tot_status) goto fail ;

//     errors = array_compare_2D(NI*NJ, 1, (void *)ur, (void *)uo, 1) ;
    errors = array_compare_2D(NI*NJ, 1, (void *)ur, (void *)o1d.data, 1) ;
    int32_t fstatus = free_array(&o1d) ;
    fprintf(stderr, "fstatus = %d (expecting 1)", fstatus) ;
    if(fstatus < 0){
      errmsg = "free failed" ;
      goto fail ;
    }
    fstatus = free_array(&o1d) ;
    fprintf(stderr, ", fstatus = %d (expecting -1)\n", fstatus) ;

    fprintf(stderr, "filter test : '%s' ,  bits extracted = %ld, errors = %d\n", test_nam2[test_no], tot_status, errors) ;
    errmsg = "data restore failed" ;
    if(errors > 0) goto fail ;
    fprintf(stderr, "filter test : available space in str000 %ld bits, available bits = %ld bits\n", StreamAvailableSpace(str000), StreamAvailableBits(str000)) ;
    STREAM_XTRACT_ALIGN32(*str000) ;
    fprintf(stderr, "filter test : available space in str000 %ld bits, available bits = %ld bits\n", StreamAvailableSpace(str000), StreamAvailableBits(str000)) ;

    fprintf(stderr, "============================== 1D integer encode test %d end ==============================\n", test_no) ;
  }
//   if(argc < 1000) goto end ;     // suppress unreachable code warning
  fprintf(stderr, "============================== encode/decode/print filter arguments ==============================\n") ;
  errmsg = "" ;
  dmap_fp_quantize arg_003  = DMAP_FP_QUANTIZE(.mode = FP_2_INT, .offset = 0x7FFFFFFF,  .nbits = 16, .maxerr = 7.5f, .zval = .0001f, .minabs = .0002f) ;
  dmap_lorenzo_arg arg_004a = DMAP_LORENZO() ;
  dmap_wavelet_arg arg_005a = DMAP_WAVELET(1) ;
  dmap_no_op7 arg_007       = DMAP_NO_OP7() ;
  dmap_filter_arg_036 arg_036 = (dmap_filter_arg_036) { .filter =  036 } ;
  dmap_filter_args arg_177    = (dmap_filter_args   ) { .filter = 0177 } ;
  dpfl[0] = (dmap_filter_args_ptr) &arg_001a ;     // filter 001
  dpfl[1] = (dmap_filter_args_ptr) &arg_001b ;     // filter 001
  dpfl[2] = (dmap_filter_args_ptr) &arg_003 ;      // filter 003
  dpfl[3] = (dmap_filter_args_ptr) &arg_002a ;     // filter 002
  dpfl[4] = (dmap_filter_args_ptr) &arg_004a ;     // filter 004
  dpfl[5] = (dmap_filter_args_ptr) &arg_005a ;     // filter 005
  dpfl[6] = (dmap_filter_args_ptr) &arg_036 ;      // undefined filter 036 (30)
  dpfl[7] = (dmap_filter_args_ptr) &arg_177 ;      // invalid filter 0177 (127)
  dpfl[8] = (dmap_filter_args_ptr) &arg_007 ;      // filter 003
  dpfl[9] = (dmap_filter_args_ptr) &arg_006a ;     // filter 006
  dpfl[10] = FILTER_LIST_END ;                     // end of filter list

  dmap_print_parameters(dpfl) ;
  STREAM_INIT(str000, NULL, 0, 0) ;               // full RW stream reset (keep buffer, size, and mode)
  dmap_encode_parameters(dpfl, str000) ;
  STREAM_REWIND(*str000, 1) ;
  dmap_filter_list dpfl2 ;
  dpfl2 = dmap_decode_parameters(str000) ;
  errmsg = "dmap_decode_parameters returned NULL" ;
  if(dpfl2 == NULL) goto fail ;
  dmap_print_parameters(dpfl2) ;
  free(dpfl2) ;

if(argc < 1000) goto end ;     // suppress unreachable code warning

  arg_003a = DMAP_FP_QUANTIZE( .maxerr = .25f, .minabs = 0.0f, .nbits = 12, .offset = 0x7FFFFFFF, .mode = FP_QUANTIZE_LIN ) ;
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
  dpfl[3] = FILTER_LIST_END ;                    // end of filter list
// goto array_test ;
  STREAM_CREATE(stream, buffer, sizeof(buffer), 0) ;
  STREAM_INSERT_BEGIN(*stream) ;
  fprintf(stderr, "filter test : available space in stream %ld bits\n", StreamAvailableSpace(stream)) ;

  debug_mode = dmap_debug_mode(1) ;
  strict_mode = dmap_strict_mode(1) ;
//   bp2d = NULL_BLOCK_PROPERTIES ; // data properties are not valid
//   status = dmap_filter_enc((array_nd *)&a2d, &bp2d, dpfl, stream) ;   // activate forward filter chain
  // no data properties available, pass NULL
  status = dmap_filter_enc((array_nd *)&a2d, NULL, dpfl, stream) ;   // activate forward filter chain
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
  tot_status = dmap_filter_dec((array_nd *)&b2d, stream) ;
  fprintf(stderr, "filter test : new b2d address = %p\n", (void *)b2d.data) ;
  fprintf(stderr, "filter test : bits extracted = %ld\n", tot_status) ;
  fprintf(stderr, "filter test : available data in stream %ld bits\n", StreamAvailableBits(stream)) ;
  STREAM_XTRACT_ALIGN32(*stream) ;        // align to a 32 bit boundary
  fprintf(stderr, "filter test : available data in stream %ld bits\n", StreamAvailableBits(stream)) ;

  errors = NI*NJ ;
//   float *pz = (float *) a2d.data ;
  float *pz = (float *) fr ;
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
  if(errors > 0) fprintf(stderr, "%f %f %f %f\n", fi[0][0], fi[NJ-1][NI-1], fo[0][0], fo[NJ-1][NI-1]) ;
  fprintf(stderr, "filter test : %d differences between fo and fi (%d values), max = %f\n", errors, NI*NJ, maxdiff) ;
  if(errors > 0) goto fail ;
  fprintf(stderr, "SUCCESS\n") ;
if(argc < 1000) goto end ;     // suppress unreachable code warning
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
  dmap_filter_arg_003 quantize = DMAP_FILTER_003( .mode = 0, .maxerr = .25f, .nbits = 12) ;
  dmap_filter_arg_006 encode   = DMAP_FILTER_006( .mode = 24 ) ;

  dpfl[0] = (dmap_filter_args_ptr)&quantize ;     // filter 003, linear float quantizer
  dpfl[1] = (dmap_filter_args_ptr)&encode ;       // filter 006, bit encoder
  dpfl[1] = FILTER_LIST_END ;                     // end of filter list

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
    index_range jrange = block_limits(j0, jaxis) ;
    int jsize = jrange.ixn - jrange.ix0 + 1 ;
    for(i0=0 ; i0<iaxis.nbk ; i0++){
      //  get the float block to process
      index_range irange = block_limits(i0, iaxis) ;
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
      status = dmap_filter_enc((array_nd *)&b2d, &bp, dpfl, stream) ;   // activate forward filter chain
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
