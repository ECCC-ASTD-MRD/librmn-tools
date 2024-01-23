// Copyright (C) 2024  Environnement Canada
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
//     M. Valin,   Recherche en Prevision Numerique, 2024
//
#include <stdio.h>
#include <stdint.h>

#include <rmn/word_stream.h>
#include <rmn/test_helpers.h>
//
#define NDATA   8192
#define CHUNK0  512
//
static int array_compare(int32_t *a1, int32_t *a2, int n){
  int ndif = 0, i ;
  for(i=0 ; i<n ; i++) if(a1[i] != a2[i]) ndif++ ;
    return ndif ;
}
//
int main(int argc, char **argv){
  wordstream stream0, stream1, stream2 ;
  int32_t data_in[NDATA], data_out[NDATA] ;
  int32_t *local_buf, *buf00, *buf01, *buf10, *buf11, *buf20, *buf21, *buf2 ;
  int32_t lbuf0 = NDATA/2, lbuf2 = NDATA/4 ;
  int i, status0, status1, status2 ;
  int size0, size1, size2 ;

  start_of_test("C word stream test") ;

  for(i=0 ; i<NDATA ; i++) data_in[i] = i - NDATA/2 ;

  buf00 = ws32_create(&stream0, NULL, NDATA/2, 0, 0) ;        // internally malloc(ed) buffer, last 2 arguments are irrelevant
  size0 = WS32_SIZE(stream0) ;
  fprintf(stderr, "stream0 buffer at %16.16p[%6d], mem = %16.16p\n", buf00, size0, NULL) ;
  status0 = ws32_insert(&stream0, data_in, CHUNK0) ;
  fprintf(stderr, "stream0 insertion of %d words, status = %5d, out/in = %5d/%5d, status = %d\n", 
          CHUNK0, status0, WS32_OUT(stream0), WS32_IN(stream0), status0) ;
  for(i=0 ; i<NDATA ; i++) data_out[i] = 999999 ;
  status0 = ws32_xtract(&stream0, data_out, CHUNK0) ;
  fprintf(stderr, "stream0 extraction of %d words, status = %5d, out/in = %5d/%5d, status = %d, errors = %d\n", 
          CHUNK0, status0, WS32_OUT(stream0), WS32_IN(stream0), status0, array_compare(data_in, data_out, CHUNK0)) ;
  fprintf(stderr, "\n") ;

  local_buf = (uint32_t *) malloc(sizeof(uint32_t) * lbuf0) ;
  buf10 = ws32_create(&stream1, local_buf, lbuf0, 0, WS32_CAN_REALLOC) ;   // realloc(atable) local buffer
  size1 = WS32_SIZE(stream1) ;
  fprintf(stderr, "stream1 buffer at %16.16p[%6d], mem = %16.16p\n", buf10, size1, local_buf) ;
  status1 = ws32_insert(&stream1, data_in, CHUNK0) ;
  fprintf(stderr, "stream1 insertion of %d words, status = %5d, out/in = %5d/%5d, status = %d\n", 
          CHUNK0, status1, WS32_OUT(stream1), WS32_IN(stream1), status1) ;
  for(i=0 ; i<NDATA ; i++) data_out[i] = 999999 ;
  status1 = ws32_xtract(&stream1, data_out, CHUNK0) ;
  fprintf(stderr, "stream1 extraction of %d words, status = %5d, out/in = %5d/%5d, status = %d, errors = %d\n", 
          CHUNK0, status1, WS32_OUT(stream1), WS32_IN(stream1), status1, array_compare(data_in, data_out, CHUNK0)) ;
  fprintf(stderr, "\n") ;

  buf2 = (uint32_t *) malloc(sizeof(uint32_t) * lbuf2) ;
  buf20 = ws32_create(&stream2, buf2, lbuf2, 0, 0) ;   // non realloc(atable) local buffer
  size2 = WS32_SIZE(stream2) ;
  fprintf(stderr, "stream2 buffer at %16.16p[%6d], mem = %16.16p\n", buf20, size2, buf2) ;
  status2 = ws32_insert(&stream2, data_in, CHUNK0) ;
  fprintf(stderr, "stream2 insertion of %d words, status = %5d, out/in = %5d/%5d, status = %d\n", 
          CHUNK0, status2, WS32_OUT(stream2), WS32_IN(stream2), status2) ;
  for(i=0 ; i<NDATA ; i++) data_out[i] = 999999 ;
  status2 = ws32_xtract(&stream2, data_out, CHUNK0) ;
  fprintf(stderr, "stream2 extraction of %d words, status = %5d, out/in = %5d/%5d, status = %d, errors = %d\n", 
          CHUNK0, status2, WS32_OUT(stream2), WS32_IN(stream2), status2, array_compare(data_in, data_out, CHUNK0)) ;
  fprintf(stderr, "\n") ;

  status0 = ws32_resize(&stream0, NDATA+1) ;
  buf01 = WS32_BUFFER(stream0) ;
  size0 = WS32_SIZE(stream0) ;
  fprintf(stderr, "stream0 new buffer at %16.16p[%6d], old was at %16.16p, %s\n", buf01, size0, buf00, status0 ? "FAIL" : "SUCCESS") ;

  status1 = ws32_resize(&stream1, NDATA+1) ;
  buf11 = WS32_BUFFER(stream1) ;
  size1 = WS32_SIZE(stream1) ;
  fprintf(stderr, "stream1 new buffer at %16.16p[%6d], old was at %16.16p, %s\n", buf11, size1, buf10, status1 ? "FAIL" : "SUCCESS") ;

  status2 = ws32_resize(&stream2, NDATA+1) ;
  buf21 = WS32_BUFFER(stream2) ;
  size2 = WS32_SIZE(stream2) ;
  fprintf(stderr, "stream2 new buffer at %16.16p[%6d], old was at %16.16p, %s\n", buf21, size2, buf20, buf20 == buf21 ? "SUCCESS" : "FAIL") ;

  fprintf(stderr, "status 0/1/2 = %3d/%3d/%3d\n", status0, status1, status2) ;

  return 0 ;
}
