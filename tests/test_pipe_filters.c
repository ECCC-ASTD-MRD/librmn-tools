// Copyright (C) 2023-2024  Environnement Canada
//
// This is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation,
// version 2.1 of the License.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// Author:
//     M. Valin,   Recherche en Prevision Numerique, 2023-2024
//

#include <stdio.h>
#include <stdlib.h>

// double inclusion of include files is deliberate to test against double inclusion
// #include <rmn/tee_print.h>
#include <rmn/test_helpers.h>
#include <rmn/filter_000.h>
#include <rmn/filter_000.h>
#include <rmn/filter_001.h>
#include <rmn/filter_001.h>
#include <rmn/filter_110.h>
#include <rmn/filter_110.h>
#include <rmn/filter_254.h>
#include <rmn/filter_254.h>
#include <rmn/filter_255.h>

#define NPTSI 3
#define NPTSJ 4

int test_0(char *msg){
  fprintf(stderr, "============================ register pipe filters  ============================\n");
  pipe_filters_init() ;                                   // initialize filter table
}

int test_1(char *msg){
  filter_254 filter1 = filter_254_null ;
  filter_254 filter2 = filter_254_null ;
  filter_254 filter3 = filter_254_null ;
  filter_255 filter4 = filter_255_null ;
  filter4.flags = 1 ; filter4.opt[0] = 0xBEBEFADA ; filter4.opt[1] = 0xDEADBEEF ;
  filter_list filters = { (filter_meta *) &filter1, 
                          (filter_meta *) &filter2, 
                          (filter_meta *) &filter3, 
                          (filter_meta *) &filter4, 
                          NULL } ;
  int32_t data_ref[NPTSJ*NPTSI] ;
  int32_t data_i[NPTSJ*NPTSI] ;
//   int32_t data_o[NPTSJ*NPTSI] ;
  int i, j, k, errors ;
  ssize_t nmeta, nmetao ;
  wordstream stream_out ;
  array_descriptor adi = array_descriptor_null ; //, ado = array_null ;
  array_descriptor ad1, ad2 ;
  filter_dim fdim ;
  uint32_t fsize ;
  // syntax test for 009 (possible octal confusion)
//   typedef struct{
//     FILTER_PROLOG ;
//     array_properties ap ;
//   } FILTER_TYPE(009) ;
//   static FILTER_TYPE(009) FILTER_NULL(009) = {FILTER_BASE(009) } ;
//   filter_009 dummy = filter_009_null ;

  ws32_create(&stream_out, NULL, 4096, 0, WS32_CAN_REALLOC) ;

  fprintf(stderr, "============================ dimension encoding / decoding ============================\n");
  // test dimension encoding / decoding
  uint32_t listdim[7] = { 63, 255, 1023, 4095, 65535, 16777215, 16777216 } ;
  for(j=0 ; j<7 ; j++){
    for(i=1 ; i<=MAX_ARRAY_DIMENSIONS ; i++){
      ad1 = ad2 = array_descriptor_null ;
      ad1.ndims = i ;
      for(k=0 ; k<ad1.ndims ; k++) ad1.nx[k] = listdim[j] - k ;
      fsize = filter_dimensions_encode(&ad1, (filter_meta *)(&fdim)) ;
      filter_dimensions_decode(&ad2, (filter_meta *)(&fdim)) ;
      errors = 0 ;
      for(k=0 ; k<ad1.ndims ; k++) {
        if(ad2.nx[k] != ad1.nx[k]){
          errors ++ ;
          fprintf(stderr, "ndims = %d, fsize = %d, dimension %d, expecting %9d, got %9d\n", ad1.ndims, fsize, k+1, ad1.nx[k], ad2.nx[k]) ;
        }
      }

      fprintf(stderr, "encoded dimensions: fsize = %1d, flags = %d ", fsize, fdim.flags) ;
      fprintf(stderr, ", ndim = %1d", ad1.ndims) ;
      for(k=0 ; k<ad1.ndims ; k++) fprintf(stderr, ",%2d", ad1.nx[k]) ;
      fprintf(stderr, ", errors = %d\n", errors);
      if(errors) exit(1) ;
    }
    fprintf(stderr, "\n") ;
  }
  fprintf(stderr, "SUCCESS\n") ;

  fprintf(stderr, "============================ register pipe filters  ============================\n");
  pipe_filters_init() ;                                   // initialize filter table
  i = pipe_filter_register(254, "demo254", pipe_filter_254) ;  // change name of filter 254
  fprintf(stderr, "registered demo254 status = %d, name = '%s', address = %p\n", i, pipe_filter_name(254), pipe_filter_address(254)) ;
  i = pipe_filter_register(255, "demo255", pipe_filter_255) ;  // change name of filter 255
  fprintf(stderr, "registered demo255 status = %d, name = '%s', address = %p\n", i, pipe_filter_name(255), pipe_filter_address(255)) ;

  fprintf(stderr, "============================ pipe filters (in place)  ============================\n");

  WS32_RESET(stream_out) ;                     // set stream in and out indexes to beginning of buffer
  filter1.factor = 2 ; filter1.offset = 100 ;
  filter2.factor = 3 ; filter2.offset = 1000 ;
  filter3.factor = 1 ; filter3.offset = 0 ;    // essentially a copy

  for(i = 0 ; i < NPTSJ*NPTSI ; i++) data_ref[i] = i ;
  for(i = 0 ; i < NPTSJ*NPTSI ; i++) data_i[i] = data_ref[i] ;
  fprintf(stderr, "input    : ") ;
  for(i = 0 ; i < NPTSJ*NPTSI ; i++) fprintf(stderr, "%6d ", data_ref[i]) ; fprintf(stderr, "\n") ;

  // metadata will be in stream_out, "filtered" data will be in data_i[]
  adi = (array_descriptor) { .esize = 4, .ndims = 2, .data = data_i, .nx[0] = NPTSI, .nx[1] = NPTSJ } ;
  nmeta = run_pipe_filters(PIPE_FORWARD|PIPE_INPLACE, &adi, filters, &stream_out) ;
  fprintf(stderr, "forward filters metadata length = %ld\n", nmeta);
  fprintf(stderr, "filtered : ") ;
  for(i = 0 ; i < NPTSJ*NPTSI ; i++) fprintf(stderr, "%6d ", data_i[i]) ; fprintf(stderr, "\n") ;

  fprintf(stderr, " %d words in output stream\nmetadata : ", WS32_FILLED(stream_out)) ;
  for(i = 0 ; i < nmeta ; i++) fprintf(stderr, "%8.8x ", stream_out.buf[i]) ;
  fprintf(stderr, "\ndata_i   : ");
  for(i = 0 ; i < NPTSJ*NPTSI ; i++) fprintf(stderr, "%6d ", data_i[i]) ; fprintf(stderr, "\n\n") ;
//   ssize_t run_pipe_filters(int flags, int *dims, void *data_i, filter_list list, pipe_buffer *buffer);

  WS32_REREAD(stream_out) ;
  // description of what is expected to come out of the reverse filter chain
//   ado = (array_descriptor) { .esize = 4, .ndims = 2, .data = data_o, .nx[0] = NPTSI, .nx[1] = NPTSJ } ;
//   for(i = 0 ; i < NPTSJ*NPTSI ; i++) data_o[i] = stream_out.buf[i+8] ;
  nmetao = run_pipe_filters(PIPE_REVERSE|PIPE_INPLACE, &adi, filters, &stream_out) ;
  fprintf(stderr, "reverse filters metadata read = %ld\n", nmetao);
  if(nmetao != nmeta){
    fprintf(stderr, "ERROR: metadata length mismatch, expected %ld, got %ld\n", nmeta, nmetao) ;
    exit(1) ;
  }
  fprintf(stderr, "restored : ") ;
  for(i = 0 ; i < NPTSJ*NPTSI ; i++) fprintf(stderr, "%6d ", data_i[i]) ; // fprintf(stderr, "\n") ;
  errors = 0 ;
  for(i=0 ; i<NPTSJ*NPTSI ; i++) errors += (data_i[i] != data_ref[i]) ;
  fprintf(stderr,", errors = %d\n", errors) ;
  if(errors) exit(1) ;
  fprintf(stderr, "SUCCESS\n") ;

  fprintf(stderr, "============================ pipe filters (not in place)  ============================\n");

  WS32_RESET(stream_out) ;                     // set stream in and out indexes to beginning of buffer
  filter1.factor = 2 ; filter1.offset = 100 ;
  filter2.factor = 3 ; filter2.offset = 1000 ;

  for(i = 0 ; i < NPTSJ*NPTSI ; i++) data_ref[i] = i ;
  for(i = 0 ; i < NPTSJ*NPTSI ; i++) data_i[i] = data_ref[i] ;
  fprintf(stderr, "input    : ") ;
  for(i = 0 ; i < NPTSJ*NPTSI ; i++) fprintf(stderr, "%6d ", data_ref[i]) ; fprintf(stderr, "\n") ;

  // metadata will be in stream_out, "filtered" data will be in stream_put, after metadata
  adi = (array_descriptor) { .esize = 4, .ndims = 2, .data = data_i, .nx[0] = NPTSI, .nx[1] = NPTSJ } ;
  nmeta = run_pipe_filters(PIPE_FORWARD, &adi, filters, &stream_out) ;
  fprintf(stderr, "forward filters metadata length = %ld\n", nmeta);
  fprintf(stderr, "filtered : ") ;
  for(i = 0 ; i < NPTSJ*NPTSI ; i++) fprintf(stderr, "%6d ", stream_out.buf[i+nmeta]) ; fprintf(stderr, "\n") ;

  fprintf(stderr, " %d words in output stream\nmetadata : ", WS32_FILLED(stream_out)) ;
  for(i = 0 ; i < nmeta ; i++) fprintf(stderr, "%8.8x ", stream_out.buf[i]) ;
  fprintf(stderr, "\ndata_i   : ");
  for(i = 0 ; i < NPTSJ*NPTSI ; i++) fprintf(stderr, "%6d ", data_i[i]) ; fprintf(stderr, "\n\n") ;
//   ssize_t run_pipe_filters(int flags, int *dims, void *data_i, filter_list list, pipe_buffer *buffer);

  WS32_REREAD(stream_out) ;
  // description of what is expected to come out of the reverse filter chain
//   ado = (array_descriptor) { .esize = 4, .ndims = 2, .data = data_o, .nx[0] = NPTSI, .nx[1] = NPTSJ } ;
//   for(i = 0 ; i < NPTSJ*NPTSI ; i++) data_o[i] = stream_out.buf[i+8] ;
  nmetao = run_pipe_filters(PIPE_REVERSE, &adi, filters, &stream_out) ;
  fprintf(stderr, "reverse filters metadata read = %ld\n", nmetao);
  if(nmetao != nmeta){
    fprintf(stderr, "ERROR: metadata length mismatch, expected %ld, got %ld\n", nmeta, nmetao) ;
    exit(1) ;
  }
  fprintf(stderr, "restored : ") ;
  for(i = 0 ; i < NPTSJ*NPTSI ; i++) fprintf(stderr, "%6d ", data_i[i]) ; // fprintf(stderr, "\n") ;
  errors = 0 ;
  for(i=0 ; i<NPTSJ*NPTSI ; i++) errors += (data_i[i] != data_ref[i]) ;
  fprintf(stderr,", errors = %d\n", errors) ;
  if(errors) exit(1) ;
  fprintf(stderr, "============================ %s : SUCCESS ============================\n\n", msg) ;
  return 0 ;
}

#define NI  15
#define NJ  13

int test_2(char *msg){
  filter_255 filter1 = filter_255_null ;
  filter1.flags = 1 ; filter1.opt[0] = 0xBEBEFADA ; filter1.opt[1] = 0xDEADBEEF ;
  filter_list filters = { (filter_meta *) &filter1, 
                          NULL } ;
  uint32_t array_in[NJ][NI] ;
  uint32_t array_out[NJ][NI] ;
  uint32_t *map;
  int i, j ;
  wordstream stream_2 ;
  ssize_t nbytes ;
  array_descriptor adi = array_descriptor_null , ado = array_descriptor_null ;

  // create word stream
  ws32_create(&stream_2, NULL, 4096, 0, WS32_CAN_REALLOC) ;

  fprintf(stderr, "============================ register pipe filters  ============================\n");
  pipe_filters_init() ;                                   // initialize filter table
  i = pipe_filter_register(255, "diag255", pipe_filter_255) ;  // change name of filter 255
  fprintf(stderr, "registered diag255 status = %d, name = '%s', address = %p\n", i, pipe_filter_name(255), pipe_filter_address(255)) ;

  for(j=0 ; j<NJ ; j++){
    for(i=0 ; i<NI ; i++){
      array_in[j][i] = i*1000 + j ;
      array_out[j][i] = -1 ;
    }
  }
  fprintf(stderr, "\n============================ forward ============================\n") ;
  adi.data = array_in ;
  adi.tilex = adi.tiley = 8 ;
  adi.ndims = 2 ; adi.nx[0] = NI ; adi.nx[1] = NJ ;
  adi.esize = 4 ; adi.etype = PIPE_DATA_UNSIGNED ;
  nbytes = tiled_fwd_pipe_filters(0, &adi, filters, &stream_2) ;
  fprintf(stderr, "bytes added = %ld\n", nbytes) ;
  WS32_REREAD(stream_2) ;
  map = WS32_BUFFER_OUT(stream_2) ;
  uint32_t map2 = map[2] & 0xFFFF, map3 = map[3] & 0xFFFF ;
  uint32_t nblki = (map[0]+map[2]-1)/map[2] , nblkj = (map[1]+map[3]-1)/map[3] ;
  fprintf(stderr, "array [%d x %d], blocks [%d x %d], data map [%d,%d]", map[0], map[1], map2, map3, nblki, nblkj) ;
  map += 4 ;
  for(i=0 ; i<nblkj ; i++) fprintf(stderr, "%10d", map[i]) ; // fprintf(stderr, "\n") ;
  map += nblkj ;
  for(i=0 ; i<nblki*nblkj ; i++) fprintf(stderr, "%10d", map[i]) ; fprintf(stderr, "\n") ;

  fprintf(stderr, "\n============================ reverse ============================\n") ;
  ado = adi ;
  ado.data = array_out ;
  nbytes = tiled_rev_pipe_filters(0, &ado, &stream_2) ;

  fprintf(stderr, "============================ %s : SUCCESS ============================\n\n", msg) ;
  return 0 ;
}

int test_3(char *msg){
  array_descriptor adi = array_descriptor_null, ado ;
  uint32_t w32[6], nw32 ;
  uint32_t dimref[7] = { 63, 255, 1023, 4095, 65535, 16777215, 16777216 } ;
  int i, j, k, errors ;

  fprintf(stderr, "============================ encode/decode dimensions  ============================\n");
  for(i=1 ; i<=5 ; i++){
    adi.ndims = i ;
    for(k=0 ; k<7 ; k++){
      for(j=0 ; j<adi.ndims ; j++) {
        adi.nx[j] = dimref[k] - adi.ndims + j + 1 ;
      }
      nw32 = encode_dimensions(&adi, w32) ;
      fprintf(stderr, "nw32 = %d, ndims = %d, dimmax = %9d", nw32, adi.ndims, adi.nx[adi.ndims-1]) ;
      for(j=0 ; j<5 ; j++) ado.nx[j] = 0 ;
      nw32 = decode_dimensions(&ado, w32) ;
      errors = 0 ;
      for(j=0 ; j<adi.ndims ; j++) {
        if(adi.nx[j] != ado.nx[j]) {
          errors++ ;
          fprintf(stderr, ", dimension %d, expecting %9d, got %9d", j, adi.nx[j], ado.nx[j]) ;
        }
      }
      fprintf(stderr, ", errors = %d", errors) ;
      fprintf(stderr, "\n");
    }
  }
  fprintf(stderr, "============================ %s : SUCCESS ============================\n\n", msg) ;
  return 0 ;
}

#undef NPTSI
#undef NPTSJ
#define NPTSI 15
#define NPTSJ 13
int test_4(char *msg){
  uint32_t fullsize[NPTSI*NPTSJ], reduced[NPTSI*NPTSJ] ;
  filter_001 filter0 = filter_001_null ;
  filter_110 filter1 = filter_110_null ;
  filter_255 filter2 = filter_255_null ;
  filter_list filters = {
                        (filter_meta *) &filter0,
                        (filter_meta *) &filter2,
                        (filter_meta *) &filter1,
                        (filter_meta *) &filter2,
                        NULL
                        } ;
  int i, npts = NPTSI * NPTSJ ;
  ssize_t nmeta ;

  fprintf(stderr, "============================ dimension reduction test  ============================\n");
  pipe_filters_init() ;                                   // initialize filter table

  for(i=0 ; i<npts ; i++) { fullsize[i] = i ; reduced[i] = 0 ; }
  // create word stream
  wordstream stream_0 ;
  void *ptr = ws32_create(&stream_0, NULL, npts, 0, WS32_CAN_REALLOC) ;
  fprintf(stderr, "word stream buffer address %p\n", ptr) ;
  if(ptr == NULL) exit(1) ;

  // metadata will be in stream_out, "filtered" data will be in data_i[]
  array_descriptor adi = { .esize = 4, .etype = PIPE_DATA_UNSIGNED, .ndims = 2, .data = fullsize, .nx[0] = NPTSI, .nx[1] = NPTSJ } ;
  filter2.flags = 1 ;
  nmeta = run_pipe_filters(PIPE_FORWARD|PIPE_INPLACE, &adi, filters, &stream_0) ;
  fprintf(stderr, "forward filters metadata length = %ld\n", nmeta);
}

int main(int argc, char **argv){
  int to_test = 0 ;

  start_of_test("C pipe filters test") ;

  if(argc == 1) return test_1((*argv)+1) ;
  while(argc > 1){
    argc-- ;
    argv++ ;
    char *msg = (*argv)+1 ;
    to_test = atoi(argv[0]) ;
    switch(to_test){
      case 0:
        test_0(msg) ;
        break;
      case 1:
        test_1(msg) ;
        break;
      case 2:
        test_2(msg) ;
        break;
      case 3:
        test_3(msg) ;
        break;
      case 4:
        test_4(msg) ;
        break;
      default:
        fprintf(stderr, "WARNING: unknown test %d\n", to_test) ;
    }
  }
  return 0 ;
}
