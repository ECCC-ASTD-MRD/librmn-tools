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
#include <rmn/pipe_filters.h>
#include <rmn/pipe_filters.h>
#include <rmn/tee_print.h>
#include <rmn/test_helpers.h>

#define NPTSI 3
#define NPTSJ 4

int main(int argc, char **argv){
  filter_254 filter1 = filter_254_null ;
  filter_254 filter2 = filter_254_null ;
  filter_list filters = { (filter_meta *) &filter1, (filter_meta *) &filter2, NULL } ;
  int32_t data_ref[NPTSJ*NPTSI] ;
  int32_t data_i[NPTSJ*NPTSI] ;
  int32_t data_o[NPTSJ*NPTSI] ;
  int i, j, k, errors, status, nmeta, nmetao ;
  int dims[] = { 2, NPTSI, NPTSJ } ;
  ssize_t psize ;
  wordstream stream_out ;
  array_descriptor adi = array_null, ado = array_null ;
  array_dimensions ad1, ad2 ;
  filter_dim fdim ;
  uint32_t fsize ;

  ws32_create(&stream_out, NULL, 4096, 0, WS32_CAN_REALLOC) ;

  start_of_test("C pipe filters test") ;

  fprintf(stderr, "============================ dimension encoding / decoding ============================\n");
  // test dimension encoding / decoding
  int factor = 1 ;
  for(j=0 ; j<3 ; j++){
    for(i=1 ; i<=ARRAY_DESCRIPTOR_MAXDIMS ; i++){
      ad1 = ad2 = array_dimensions_null ;
      ad1.ndims = i ;
      int j ;
      for(j=0 ; j<i ; j++) ad1.nx[j] = (9 - j) * factor ;
      fsize = filter_dimensions_encode(&ad1, (filter_meta *)(&fdim)) ;
      filter_dimensions_decode(&ad2, (filter_meta *)(&fdim)) ;
      errors = 0 ;
      for(k=0 ; k<ARRAY_DESCRIPTOR_MAXDIMS ; k++) errors += (ad2.nx[k] != ad1.nx[k]) ;

      fprintf(stderr, "encoded dimensions: fsize = %1d, flags = %d ", fsize, fdim.flags) ;
      fprintf(stderr, ", ndim = %1d", ad1.ndims) ;
      for(j=0 ; j<ad1.ndims ; j++) fprintf(stderr, ",%2d", ad1.nx[j]) ;
      fprintf(stderr, ", errors = %d\n", errors);
      if(errors) exit(1) ;
    }
    factor *= 256 ;
    fprintf(stderr, "\n") ;
  }
  fprintf(stderr, "SUCCESS\n") ;

  fprintf(stderr, "============================ pipe filters (in place)  ============================\n");
  pipe_filters_init() ;                                   // initialize filter table
  i = pipe_filter_register(254, "demo254", pipe_filter_254) ;  // change name of filter 254
  fprintf(stderr, "filter_register demo254 status = %d, name = '%s', address = %p\n", i, pipe_filter_name(254), pipe_filter_address(254)) ;

  filter1.factor = 2 ; filter1.offset = 100 ;
  filter2.factor = 3 ; filter2.offset = 1000 ;

  for(i = 0 ; i < NPTSJ*NPTSI ; i++) data_ref[i] = i ;
  for(i = 0 ; i < NPTSJ*NPTSI ; i++) data_i[i] = data_ref[i] ;
  fprintf(stderr, "input    : ") ;
  for(i = 0 ; i < NPTSJ*NPTSI ; i++) fprintf(stderr, "%6d ", data_ref[i]) ; fprintf(stderr, "\n") ;

  // metadata will be in stream_out, "filtered" data will be in data_i[]
  adi = (array_descriptor) { .adim.esize = 4, .adim.ndims = 2, .data = data_i, .adim.nx[0] = NPTSI, .adim.nx[1] = NPTSJ } ;
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
  ado = (array_descriptor) { .adim.esize = 4, .adim.ndims = 2, .data = data_o, .adim.nx[0] = NPTSI, .adim.nx[1] = NPTSJ } ;
  for(i = 0 ; i < NPTSJ*NPTSI ; i++) data_o[i] = stream_out.buf[i+8] ;
  nmetao = run_pipe_filters(PIPE_REVERSE|PIPE_INPLACE, &adi, filters, &stream_out) ;
  fprintf(stderr, "reverse filters metadata read = %ld\n", nmetao);
  if(nmetao != nmeta){
    fprintf(stderr, "ERROR: metadata length mismatch, expected %d, got %d\n", nmeta, nmetao) ;
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

  WS32_RESET(stream_out) ;                     // reset stream in and out indexes to beginning of buffer
  filter1.factor = 2 ; filter1.offset = 100 ;
  filter2.factor = 3 ; filter2.offset = 1000 ;

  for(i = 0 ; i < NPTSJ*NPTSI ; i++) data_ref[i] = i ;
  for(i = 0 ; i < NPTSJ*NPTSI ; i++) data_i[i] = data_ref[i] ;
  fprintf(stderr, "input    : ") ;
  for(i = 0 ; i < NPTSJ*NPTSI ; i++) fprintf(stderr, "%6d ", data_ref[i]) ; fprintf(stderr, "\n") ;

  // metadata will be in stream_out, "filtered" data will be in stream_put, after metadata
  adi = (array_descriptor) { .adim.esize = 4, .adim.ndims = 2, .data = data_i, .adim.nx[0] = NPTSI, .adim.nx[1] = NPTSJ } ;
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
  ado = (array_descriptor) { .adim.esize = 4, .adim.ndims = 2, .data = data_o, .adim.nx[0] = NPTSI, .adim.nx[1] = NPTSJ } ;
  for(i = 0 ; i < NPTSJ*NPTSI ; i++) data_o[i] = stream_out.buf[i+8] ;
  nmetao = run_pipe_filters(PIPE_REVERSE, &adi, filters, &stream_out) ;
  fprintf(stderr, "reverse filters metadata read = %ld\n", nmetao);
  if(nmetao != nmeta){
    fprintf(stderr, "ERROR: metadata length mismatch, expected %d, got %d\n", nmeta, nmetao) ;
    exit(1) ;
  }
  fprintf(stderr, "restored : ") ;
  for(i = 0 ; i < NPTSJ*NPTSI ; i++) fprintf(stderr, "%6d ", data_i[i]) ; // fprintf(stderr, "\n") ;
  errors = 0 ;
  for(i=0 ; i<NPTSJ*NPTSI ; i++) errors += (data_i[i] != data_ref[i]) ;
  fprintf(stderr,", errors = %d\n", errors) ;
  if(errors) exit(1) ;
  fprintf(stderr, "SUCCESS\n") ;
  return 0 ;
}
