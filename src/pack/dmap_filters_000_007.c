//
// Copyright (C) 2025  Environnement Canada
//
// This is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation,
// version 2.1 of the License.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details .
//
// Author:
//     M. Valin,   Recherche en Prevision Numerique, 2025
//

#include <rmn/dmap_filters.h>

#define FILTER_ID 000
#define FILTER_NAME CONCAT2(dmap_filter_,FILTER_ID)
#define FILTER_ARGS CONCAT2(dmap_filter_arg_,FILTER_ID)
ssize_t FILTER_NAME(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream){
  uint32_t me = FILTER_ID ;
  if(a == NULL || stream == NULL) goto fail ;    // no array or no stream
  void *array = array_address(a) ;               // get array address, dimension(s), and type
  int ndim = a->ndim, type = a->type ;
  ssize_t status = 0 ;
  bitstream s ;                                  // local copy of stream control structure
  if(dpfl == NULL && bp == NULL) goto reverse ;  // call to reverse filter
  if(! dmap_filter_valid(dpfl,me)) goto fail ;   // not the right filter or NULL pointer
  FILTER_ARGS *arg = (FILTER_ARGS *)(*dpfl) ;    // get parameters for this filter

// check a->type and a->ndim
  if(type != float_data || ndim != 2) goto fail ;
// filter processing code goes here
  int i, j, lni = a->dim[0].gnn, lnj = a->dim[1].gnn ;
  float *f = (float *)array ;
  fprintf(stderr, "filter 000, offset =%f, scale = %f, array[%d:%d]", arg->offset, arg->scale, lni, lnj) ;
  fprintf(stderr, ", LL = %f, UR = %f, ", f[0] , f[lni*lnj-1]) ;
  for(j=0 ; j<lnj ; j++){
    for(i=0 ; i<lni ; i++){
      f[i + j*lni] = (f[i + j*lni] * arg->scale) + arg->offset ;
    }
  }
  fprintf(stderr, ", LL = %f, UR = %f\n", f[0] , f[lni*lnj-1]) ;
  fprintf(stderr, "filter 000(E) : available space in stream %ld bits\n", StreamAvailableSpace(&s)) ;

  dpfl++ ;                              // call next filter if there is one
  dmap_filter_ptr next_filter = dmap_filter_next(dpfl) ;
  status = (*next_filter)(a, bp, dpfl, stream) ;
  s = *stream ;
  fprintf(stderr, "filter 000(M) : available space in stream %ld bits\n", StreamAvailableSpace(&s)) ;
  if(status < 0) goto fail ;

  STREAM_PUT_NBITS(s, FILTER_ID, 8) ;
  uint32_t *tmp1 = (uint32_t *) &(arg->offset) ;
  STREAM_PUT_NBITS(s, *tmp1, 32) ;
  uint32_t *tmp2 = (uint32_t *) &(arg->scale) ;
  STREAM_PUT_NBITS(s, *tmp2, 32) ;
  status += 72 ;

end:
  *stream = s ;   // SAVE stream changes
  fprintf(stderr, "filter 000(X) : available space in stream %ld bits\n", StreamAvailableSpace(stream)) ;
  return status ;

fail:
  return -1 ;     // DO NOT SAVE stream changes

reverse:
  s = *stream ;
  uint32_t w32 ;
  STREAM_GET_NBITS(s, w32, 8) ;
  fprintf(stderr, "reverse filter 000, id = %d\n", w32) ;
  STREAM_GET_NBITS(s, w32, 32) ;
  STREAM_GET_NBITS(s, w32, 32) ;
  *stream = s ;
  return status ;
}
#undef FILTER_NAME
#undef FILTER_ARGS
#undef FILTER_ID

#define FILTER_ID 001
#define FILTER_NAME CONCAT2(dmap_filter_,FILTER_ID)
#define FILTER_ARGS CONCAT2(dmap_filter_arg_,FILTER_ID)
ssize_t FILTER_NAME(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream){
  uint32_t me = FILTER_ID ;
  if(a == NULL || stream == NULL) goto fail ;    // no array or no stream
  void *array = array_address(a) ;               // get array address, dimension(s), and type
  int ndim = a->ndim, type = a->type ;
  ssize_t status = 0 ;
  bitstream s ;                                  // local copy of stream control structure
  if(dpfl == NULL && bp == NULL) goto reverse ;  // call to reverse filter
  if(! dmap_filter_valid(dpfl,me)) goto fail ;   // not the right filter or NULL pointer
  FILTER_ARGS *arg = (FILTER_ARGS *)(*dpfl) ;    // get parameters for this filter

  fprintf(stderr, "filter 001, offset =%d, scale = %d\n", arg->offset, arg->scale) ;
  fprintf(stderr, "filter 001(E) : available space in stream %ld bits\n", StreamAvailableSpace(&s)) ;

  dpfl++ ;                              // call next filter if there is one
  dmap_filter_ptr next_filter = dmap_filter_next(dpfl) ;
  status = (*next_filter)(a, bp, dpfl, stream) ;
  s = *stream ;
  fprintf(stderr, "filter 001(M) : available space in stream %ld bits\n", StreamAvailableSpace(&s)) ;
  if(status < 0) goto fail ;

  STREAM_PUT_NBITS(s, FILTER_ID, 8) ;
  uint32_t *tmp1 = (uint32_t *) &(arg->offset) ;
  STREAM_PUT_NBITS(s, *tmp1, 32) ;
  uint32_t *tmp2 = (uint32_t *) &(arg->scale) ;
  STREAM_PUT_NBITS(s, *tmp2, 32) ;
  status += 72 ;

end:
  *stream = s ;   // SAVE stream changes
  fprintf(stderr, "filter 001(X) : available space in stream %ld bits\n", StreamAvailableSpace(stream)) ;
  return status ;

fail:
  return -1 ;     // DO NOT SAVE stream changes

reverse:
  s = *stream ;
  uint32_t w32 ;
  STREAM_GET_NBITS(s, w32, 8) ;
  fprintf(stderr, "reverse filter 001, id = %d\n", w32) ;
  STREAM_GET_NBITS(s, w32, 32) ;
  STREAM_GET_NBITS(s, w32, 32) ;
  *stream = s ;
  return status ;
}
#undef FILTER_NAME
#undef FILTER_ARGS
#undef FILTER_ID

#define FILTER_ID 002
#define FILTER_NAME CONCAT2(dmap_filter_,FILTER_ID)
#define FILTER_ARGS CONCAT2(dmap_filter_arg_,FILTER_ID)
ssize_t FILTER_NAME(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream){
  uint32_t me = FILTER_ID ;
  if(a == NULL || stream == NULL) goto fail ;    // no array or no stream
  void *array = array_address(a) ;               // get array address, dimension(s), and type
  int ndim = a->ndim, type = a->type ;
  ssize_t status = 0 ;
  bitstream s ;                                  // local copy of stream control structure
  if(dpfl == NULL && bp == NULL) goto reverse ;  // call to reverse filter
  if(! dmap_filter_valid(dpfl,me)) goto fail ;   // not the right filter or NULL pointer
  FILTER_ARGS *arg = (FILTER_ARGS *)(*dpfl) ;    // get parameters for this filter

  fprintf(stderr, "filter 002, flag = %d\n", arg->flag) ;
  fprintf(stderr, "filter 002(E) : available space in stream %ld bits\n", StreamAvailableSpace(&s)) ;

  dpfl++ ;                              // call next filter if there is one
  dmap_filter_ptr next_filter = dmap_filter_next(dpfl) ;
  status = (*next_filter)(a, bp, dpfl, stream) ;
  s = *stream ;
  fprintf(stderr, "filter 002(M) : available space in stream %ld bits\n", StreamAvailableSpace(&s)) ;
  if(status < 0) goto fail ;

  STREAM_PUT_NBITS(s, FILTER_ID, 8) ;
  uint32_t *tmp1 = (uint32_t *) &(arg->flag) ;
  STREAM_PUT_NBITS(s, *tmp1, 8) ;
  STREAM_INSERT_PUSH(s) ;
  status += 16 ;

end:
  *stream = s ;   // SAVE stream changes
  fprintf(stderr, "filter 002(X) : available space in stream %ld bits\n", StreamAvailableSpace(stream)) ;
  return status ;

fail:
  return -1 ;     // DO NOT SAVE stream changes

reverse:
  s = *stream ;
  uint32_t w32 ;
  STREAM_GET_NBITS(s, w32, 8) ;
  fprintf(stderr, "reverse filter 001, id = %d\n", w32) ;
  STREAM_GET_NBITS(s, w32, 8) ;
  *stream = s ;
  return status ;
}
#undef FILTER_NAME
#undef FILTER_ARGS
#undef FILTER_ID
