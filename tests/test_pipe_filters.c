// Copyright (C) 2023  Environnement Canada
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
//     M. Valin,   Recherche en Prevision Numerique, 2023
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
  int32_t data[NPTSJ*NPTSI] ;
  int i ;
  int dims[] = { 2, NPTSI, NPTSJ } ;
  ssize_t psize ;
  pipe_buffer pbuf = pipe_buffer_null ;

  start_of_test("C pipe filters test") ;
  pipe_filter_init() ;                                    // initialize filter table
  i = filter_register(254, "demo254", pipe_filter_254) ;  // change name of filter 254
  fprintf(stderr, "filter_register demo254 status = %d, name = '%s', address = %p\n", i, pipe_filter_name(254), pipe_filter_address(254)) ;

  filter1.used = 2 ; filter1.factor = 2 ; filter1.offset = 100 ;
  filter2.used = 2 ; filter2.factor = 2 ; filter2.offset = 1000 ;

  for(i = 0 ; i < NPTSJ*NPTSI ; i++) data[i] = i ;
  for(i = 0 ; i < NPTSJ*NPTSI ; i++) fprintf(stderr, "%6d ", data[i]) ; fprintf(stderr, "\n") ;

  fprintf(stderr, "data address = %p\n", data) ;
  psize = run_pipe_filters(PIPE_FORWARD, dims, data, filters, &pbuf) ;
  fprintf(stderr, "psize = %ld\n", psize);
  for(i = 0 ; i < NPTSJ*NPTSI ; i++) fprintf(stderr, "%6d ", data[i]) ; fprintf(stderr, "\n") ;
//   ssize_t run_pipe_filters(int flags, int *dims, void *data, filter_list list, pipe_buffer *buffer);
}
