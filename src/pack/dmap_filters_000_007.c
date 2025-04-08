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
// ======================================= filter 000 =======================================
// special filter used to get/put array dimensions and type information (found in dmap_filters.c)
// this filter writes into bit stream BEFORE calling the filter chain and AFTER calling said chain
// this filter expects NO ARGUMENT from the filter list
// this filter MUST be called first
// in reverse mode, it makes sure that the target array has the correct configuration
// for data type and dimensions
// TODO: allocate memory for the target array if necessary
ssize_t dmap_filter_fwd(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream){
  uint32_t me = FILTER_ID ;
  if(a == NULL || stream == NULL) goto fail ;    // no array or no stream
  void *array = array_address(a) ;               // get array address, dimension(s), and type
  int ndim = a->ndim, type = a->type ;
  ssize_t status = 0 ;
  bitstream s = *stream ;                        // local copy of stream control structure
  if(dpfl == NULL && bp == NULL) goto reverse ;  // call to reverse filter

  // put array information at the start of the bit stream
  int32_t i, nbits, dsize ;
  nbits = 8 ;
  STREAM_PUT_NBITS(s, me,      8) ;              // dummy filter id
  nbits += dmap_filter_put_array_info(a, &s) ;

  // call next filter in list
  dmap_filter_ptr next_filter = dmap_filter_next(dpfl) ;
  status = (*next_filter)(a, bp, dpfl, &s) ;
  if(status < 0) goto fail ;

end:
  // put end of filter chain data marker at the end of the bit stream
  STREAM_PUT_NBITS(s, FILTER_CHAIN_END, 8) ;
  nbits += 8 ;
  *stream = s ;   // SAVE stream changes
  status += nbits ;
  return status ;

fail:
  return -1 ;     // DO NOT SAVE stream changes

  uint32_t w32 ;
reverse:
  STREAM_GET_NBITS(s, w32, 8) ;
  status = 8 ;
//   fprintf(stderr, "reverse filter %3.3o, id = %d\n", FILTER_ID, w32) ;
  if(w32 != FILTER_ID) goto fail ;                     // wrong id, MUST be FILTER_ID (0)

  // get array dimensions and type from stream (check that it matches a)
  int32_t temp = dmap_filter_get_array_info(a, &s, 1) ;
  if(temp < 0) goto fail ;
  status += temp ;
  fprintf(stderr, "filter_head(OUT), type = %s, ndim = %d, [", printable_type[type], ndim) ;
  for(i=0 ; i<ndim ; i++){ fprintf(stderr, " %d", a->dim[i].gnn) ; }
  fprintf(stderr, "]\n");

  ssize_t status2 = dmap_filter_inv(a, &s) ;     // call next inverse filter
  if(status2 < 0) goto fail ; else status += status2 ;
  *stream = s ;   // SAVE stream changes
  return status ;
}
#undef FILTER_ID

// ======================================= filter 001 =======================================
#define FILTER_ID 001
#define FILTER_NAME CONCAT2(dmap_filter_,FILTER_ID)
#define FILTER_ARGS CONCAT2(dmap_filter_arg_,FILTER_ID)
ssize_t FILTER_NAME(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream){
  uint32_t me = FILTER_ID ;
  if(a == NULL || stream == NULL) goto fail ;    // no array or no stream
  void *array = array_address(a) ;               // get array address, dimension(s), and type
  int ndim = a->ndim, type = a->type ;
  ssize_t status = 0 ;
  bitstream s = *stream ;                        // local copy of stream control structure
  if(dpfl == NULL && bp == NULL) goto reverse ;  // call to reverse filter
  if(! dmap_filter_valid(dpfl,me)) goto fail ;   // not the right filter or NULL pointer
  FILTER_ARGS *arg = (FILTER_ARGS *)(*dpfl) ;    // get parameters for this filter

  fprintf(stderr, "filter 001, offset =%d, scale = %d\n", arg->offset, arg->scale) ;
  fprintf(stderr, "filter 001(E) : available space in stream %ld bits\n", StreamAvailableSpace(&s)) ;

  dpfl++ ;                              // call next filter if there is one
  dmap_filter_ptr next_filter = dmap_filter_next(dpfl) ;
  status = (*next_filter)(a, bp, dpfl, &s) ;
  if(status < 0) goto fail ;
  fprintf(stderr, "filter 001(M) : status = %ld, available space in stream %ld bits\n", status, StreamAvailableSpace(&s)) ;

  STREAM_PUT_NBITS(s, FILTER_ID, 8) ;
  uint32_t *tmp1 = (uint32_t *) &(arg->offset) ;
  STREAM_PUT_NBITS(s, *tmp1, 32) ;
  uint32_t *tmp2 = (uint32_t *) &(arg->scale) ;
  STREAM_PUT_NBITS(s, *tmp2, 32) ;
  STREAM_INSERT_PUSH(s) ;
  fprintf(stderr, "filter 001(W) %3.3o, %8.8x , %8.8x\n", FILTER_ID, *tmp1, *tmp2) ;
  status += 72 ;

end:
  *stream = s ;   // SAVE stream changes
  fprintf(stderr, "filter 001(X) : available space in stream %ld bits\n", StreamAvailableSpace(stream)) ;
  return status ;

fail:
  return -1 ;     // DO NOT SAVE stream changes

  uint32_t w32 ;
reverse:
  STREAM_GET_NBITS(s, w32, 8) ;
//   fprintf(stderr, "reverse filter %3.3o, id = %d\n", FILTER_ID, w32) ;
  if(w32 != FILTER_ID) goto fail ;                     // wrong id, MUST be FILTER_ID
  uint32_t t1, t2 ;
  STREAM_GET_NBITS(s, t1, 32) ;
  STREAM_GET_NBITS(s, t2, 32) ;
  fprintf(stderr, "reverse filter %3.3o, id = %d, args = %8.8x, %8.8x\n", FILTER_ID, w32, t1, t2) ;
  status = 72 ;

  ssize_t status2 = dmap_filter_inv(a, &s) ;     // call next inverse filter
  if(status2 < 0) goto fail ; else status += status2 ;
  *stream = s ;   // SAVE stream changes
  return status ;
}
#undef FILTER_NAME
#undef FILTER_ARGS
#undef FILTER_ID

// ======================================= filter 002 =======================================
#define FILTER_ID 002
#define FILTER_NAME CONCAT2(dmap_filter_,FILTER_ID)
#define FILTER_ARGS CONCAT2(dmap_filter_arg_,FILTER_ID)
ssize_t FILTER_NAME(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream){
  uint32_t me = FILTER_ID ;
  if(a == NULL || stream == NULL) goto fail ;    // no array or no stream
  void *array = array_address(a) ;               // get array address, dimension(s), and type
  int ndim = a->ndim, type = a->type ;
  ssize_t status = 0 ;
  bitstream s = *stream ;                        // local copy of stream control structure
  if(dpfl == NULL && bp == NULL) goto reverse ;  // call to reverse filter
  if(! dmap_filter_valid(dpfl,me)) goto fail ;   // not the right filter or NULL pointer
  FILTER_ARGS *arg = (FILTER_ARGS *)(*dpfl) ;    // get parameters for this filter

  fprintf(stderr, "filter 002, flag = %d\n", arg->flag) ;
  fprintf(stderr, "filter 002(E) : available space in stream %ld bits\n", StreamAvailableSpace(&s)) ;

  dpfl++ ;                              // call next filter if there is one
  dmap_filter_ptr next_filter = dmap_filter_next(dpfl) ;
  status = (*next_filter)(a, bp, dpfl, &s) ;
  if(status < 0) goto fail ;
  fprintf(stderr, "filter 002(M) : status = %ld, available space in stream %ld bits\n", status, StreamAvailableSpace(&s)) ;

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

  uint32_t w32 ;
reverse:
  STREAM_GET_NBITS(s, w32, 8) ;
//   fprintf(stderr, "reverse filter %3.3o, id = %d\n", FILTER_ID, w32) ;
  if(w32 != FILTER_ID) goto fail ;                     // wrong id, MUST be FILTER_ID
  uint32_t t ;
  STREAM_GET_NBITS(s, t, 8) ;
  fprintf(stderr, "reverse filter %3.3o, id = %d, t = %d\n", FILTER_ID, w32, t) ;
  status = 16 ;

  ssize_t status2 = dmap_filter_inv(a, &s) ;     // call next inverse filter
  if(status2 < 0) goto fail ; else status += status2 ;
  *stream = s ;   // SAVE stream changes
  return status ;
}
#undef FILTER_NAME
#undef FILTER_ARGS
#undef FILTER_ID

// ======================================= filter 003 =======================================
#define FILTER_ID 003
#define FILTER_NAME CONCAT2(dmap_filter_,FILTER_ID)
#define FILTER_ARGS CONCAT2(dmap_filter_arg_,FILTER_ID)
// dpfl == NULL && bp == NULL indicates reverse filter call
// for the reverse filter, the metadata from the bit stream provides the necessary information
// the filter may modify the contents of the array described by a
// in filter mode, bp == NULL if no properties information is available
// the filter list MUST BE NULL TERMINATED
ssize_t FILTER_NAME(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream){
  uint32_t me = FILTER_ID ;
  if(a == NULL || stream == NULL) goto fail ;    // no array or no stream
  void *array = array_address(a) ;               // get array address, dimension(s), and type
  int ndim = a->ndim, type = a->type ;
  ssize_t status = 0 ;
  bitstream s = *stream ;                        // local copy of stream control structure
  if(dpfl == NULL && bp == NULL) goto reverse ;  // call to reverse filter
  if(! dmap_filter_valid(dpfl,me)) goto fail ;   // not the right filter or NULL pointer
  FILTER_ARGS *arg = (FILTER_ARGS *)(*dpfl) ;    // get parameters for this filter

//
// check a->type and a->ndim
//
// filter processing code goes here
//
  dpfl++ ;                                     // call next filter if there is one
  dmap_filter_ptr next_filter = dmap_filter_next(dpfl) ;
  status = (*next_filter)(a, bp, dpfl, &s) ;
  if(status < 0) goto fail ;
//
// insert into bitstream the appropriate metadata for the reverse filter
//
end:
  *stream = s ;   // SAVE stream changes
  return status ;

fail:
  return -1 ;     // DO NOT SAVE stream changes

  uint32_t w32 ;
reverse:
  STREAM_GET_NBITS(s, w32, 8) ;
//   fprintf(stderr, "reverse filter %3.3o, id = %d\n", FILTER_ID, w32) ;
  if(w32 != FILTER_ID) goto fail ;                     // wrong id, MUST be FILTER_ID
//
// inverse filter processing code goes here
//

  ssize_t status2 = dmap_filter_inv(a, &s) ;     // call next inverse filter
  if(status2 < 0) goto fail ; else status += status2 ;
  *stream = s ;   // SAVE stream changes
  return status ;
}
#undef FILTER_NAME
#undef FILTER_ARGS
#undef FILTER_ID

// ======================================= filter 004 =======================================
#define FILTER_ID 004
#define FILTER_NAME CONCAT2(dmap_filter_,FILTER_ID)
#define FILTER_ARGS CONCAT2(dmap_filter_arg_,FILTER_ID)
// dpfl == NULL && bp == NULL indicates reverse filter call
// for the reverse filter, the metadata from the bit stream provides the necessary information
// the filter may modify the contents of the array described by a
// in filter mode, bp == NULL if no properties information is available
// the filter list MUST BE NULL TERMINATED
ssize_t FILTER_NAME(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream){
  uint32_t me = FILTER_ID ;
  if(a == NULL || stream == NULL) goto fail ;    // no array or no stream
  void *array = array_address(a) ;               // get array address, dimension(s), and type
  int ndim = a->ndim, type = a->type ;
  ssize_t status = 0 ;
  bitstream s = *stream ;                        // local copy of stream control structure
  if(dpfl == NULL && bp == NULL) goto reverse ;  // call to reverse filter
  if(! dmap_filter_valid(dpfl,me)) goto fail ;   // not the right filter or NULL pointer
  FILTER_ARGS *arg = (FILTER_ARGS *)(*dpfl) ;    // get parameters for this filter

//
// check a->type and a->ndim
//
// filter processing code goes here
//
  dpfl++ ;                                     // call next filter if there is one
  dmap_filter_ptr next_filter = dmap_filter_next(dpfl) ;
  status = (*next_filter)(a, bp, dpfl, &s) ;
  if(status < 0) goto fail ;
//
// insert into bitstream the appropriate metadata for the reverse filter
//
end:
  *stream = s ;   // SAVE stream changes
  return status ;

fail:
  return -1 ;     // DO NOT SAVE stream changes

  uint32_t w32 ;
reverse:
  STREAM_GET_NBITS(s, w32, 8) ;
//   fprintf(stderr, "reverse filter %3.3o, id = %d\n", FILTER_ID, w32) ;
  if(w32 != FILTER_ID) goto fail ;                     // wrong id, MUST be FILTER_ID
//
// inverse filter processing code goes here
//

  ssize_t status2 = dmap_filter_inv(a, &s) ;     // call next inverse filter
  if(status2 < 0) goto fail ; else status += status2 ;
  *stream = s ;   // SAVE stream changes
  return status ;
}
#undef FILTER_NAME
#undef FILTER_ARGS
#undef FILTER_ID

// ======================================= filter 005 =======================================
#define FILTER_ID 005
#define FILTER_NAME CONCAT2(dmap_filter_,FILTER_ID)
#define FILTER_ARGS CONCAT2(dmap_filter_arg_,FILTER_ID)
// dpfl == NULL && bp == NULL indicates reverse filter call
// for the reverse filter, the metadata from the bit stream provides the necessary information
// the filter may modify the contents of the array described by a
// in filter mode, bp == NULL if no properties information is available
// the filter list MUST BE NULL TERMINATED
ssize_t FILTER_NAME(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream){
  uint32_t me = FILTER_ID ;
  if(a == NULL || stream == NULL) goto fail ;    // no array or no stream
  void *array = array_address(a) ;               // get array address, dimension(s), and type
  int ndim = a->ndim, type = a->type ;
  ssize_t status = 0 ;
  bitstream s = *stream ;                        // local copy of stream control structure
  if(dpfl == NULL && bp == NULL) goto reverse ;  // call to reverse filter
  if(! dmap_filter_valid(dpfl,me)) goto fail ;   // not the right filter or NULL pointer
  FILTER_ARGS *arg = (FILTER_ARGS *)(*dpfl) ;    // get parameters for this filter

//
// check a->type and a->ndim
//
// filter processing code goes here
//
  dpfl++ ;                                     // call next filter if there is one
  dmap_filter_ptr next_filter = dmap_filter_next(dpfl) ;
  status = (*next_filter)(a, bp, dpfl, &s) ;
  if(status < 0) goto fail ;
//
// insert into bitstream the appropriate metadata for the reverse filter
//
end:
  *stream = s ;   // SAVE stream changes
  return status ;

fail:
  return -1 ;     // DO NOT SAVE stream changes

  uint32_t w32 ;
reverse:
  STREAM_GET_NBITS(s, w32, 8) ;
//   fprintf(stderr, "reverse filter %3.3o, id = %d\n", FILTER_ID, w32) ;
  if(w32 != FILTER_ID) goto fail ;                     // wrong id, MUST be FILTER_ID
//
// inverse filter processing code goes here
//

  ssize_t status2 = dmap_filter_inv(a, &s) ;     // call next inverse filter
  if(status2 < 0) goto fail ; else status += status2 ;
  *stream = s ;   // SAVE stream changes
  return status ;
}
#undef FILTER_NAME
#undef FILTER_ARGS
#undef FILTER_ID

// ======================================= filter 006 =======================================
// bit stream encoder
#define FILTER_ID 006
#define FILTER_NAME CONCAT2(dmap_filter_,FILTER_ID)
#define FILTER_ARGS CONCAT2(dmap_filter_arg_,FILTER_ID)
// dpfl == NULL && bp == NULL indicates reverse filter call
// for the reverse filter, the metadata from the bit stream provides the necessary information
// the filter may modify the contents of the array described by a
// in filter mode, bp == NULL if no properties information is available
// the filter list MUST BE NULL TERMINATED
ssize_t FILTER_NAME(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream){
  uint32_t me = FILTER_ID ;
  if(a == NULL || stream == NULL) goto fail ;    // no array or no stream
  void *array = array_address(a) ;               // get array address, dimension(s), and type
  int ndim = a->ndim, type = a->type ;
  ssize_t status = 0 ;
  bitstream s = *stream ;                        // local copy of stream control structure
  if(dpfl == NULL && bp == NULL) goto reverse ;  // call to reverse filter
  if(! dmap_filter_valid(dpfl,me)) goto fail ;   // not the right filter or NULL pointer
  FILTER_ARGS *arg = (FILTER_ARGS *)(*dpfl) ;    // get parameters for this filter

  // mode >    0 : raw encoding using mode bits ( 1 - 64 )
  // mode ==   0 : raw encoding using size from array descriptor
  // mode >  100 : tile encoding with tile size mode - 100
  int mode = arg->mode, nbits = 0 ;
  if(mode > 64) goto fail ;                      // unsupported for now
  if(mode < 65) nbits = mode ;
  if(nbits == 0) nbits = a->esize * 8 ;
  if(nbits > 32) goto fail ;                     // unsupported for now
  fprintf(stderr, "filter 006, mode = %d\n", arg->mode) ;
  fprintf(stderr, "filter 006(E) : available space in stream %ld bits\n", StreamAvailableSpace(&s)) ;
//
// check a->type and a->ndim
//
// filter processing code goes here
  int32_t i, nelem = array_dimension(a) ;
  uint32_t *z = (uint32_t *) array ;
  fprintf(stderr, "filter 006 saving %d array elements\n", nelem) ;
//
  dpfl++ ;                                     // call next filter if there is one
  dmap_filter_ptr next_filter = dmap_filter_next(dpfl) ;
  status = (*next_filter)(a, bp, dpfl, &s) ;
  if(status < 0) goto fail ;
  fprintf(stderr, "filter 006(M) : status = %ld, available space in stream %ld bits\n", status, StreamAvailableSpace(&s)) ;
//
// insert into bitstream the appropriate metadata for the reverse filter
//
  uint32_t header ;
end:
  header = 0b00110000 ;
  STREAM_PUT_NBITS(s, FILTER_ID, 8) ;
  STREAM_PUT_NBITS(s, header , 8) ;
  STREAM_PUT_NBITS(s, nbits-1, 8) ;
  for(i=0 ; i<nelem ; i++) { STREAM_PUT_NBITS(s, z[i], nbits) ; status += nbits ; } ;
  STREAM_INSERT_PUSH(s) ;
  status += 24 ;
  *stream = s ;   // SAVE stream changes
  fprintf(stderr, "filter 006(X) : available space in stream %ld bits\n", StreamAvailableSpace(stream)) ;
  return status ;

fail:
  fprintf(stderr, "filter 006 FAILED\n");
  return -1 ;     // DO NOT SAVE stream changes

  uint32_t w32 ;
reverse:
  STREAM_GET_NBITS(s, w32, 8) ;
//   fprintf(stderr, "reverse filter %3.3o, id = %d\n", FILTER_ID, w32) ;
  if(w32 != FILTER_ID) goto fail ;                     // wrong id, MUST be FILTER_ID
  STREAM_GET_NBITS(s, header, 8) ;
  STREAM_GET_NBITS(s, nbits, 8) ; nbits++ ;
  fprintf(stderr, "reverse filter %3.3o, id = %d, header = %2.2x\n", FILTER_ID, w32, header) ;
  if(w32 != FILTER_ID)     goto fail ;
  if(header != 0b00110000) goto fail ;
  if(nbits > 32)           goto fail ;                    // unsupported for now
  nelem = array_dimension(a) ;
  z = (uint32_t *) array ;
  fprintf(stderr, "filter 006 restoring %d array elements\n", nelem) ;
  status = 24 ;
  for(i=0 ; i<nelem ; i++) { STREAM_GET_NBITS(s, z[i], nbits) ; status += nbits ; } ;

  ssize_t status2 = dmap_filter_inv(a, &s) ;     // call next inverse filter
  if(status2 < 0) goto fail ; else status += status2 ;
  *stream = s ;   // SAVE stream changes
  return status ;
}
#undef FILTER_NAME
#undef FILTER_ARGS
#undef FILTER_ID

// ======================================= filter 007 =======================================
#define FILTER_ID 007
#define FILTER_NAME CONCAT2(dmap_filter_,FILTER_ID)
#define FILTER_ARGS CONCAT2(dmap_filter_arg_,FILTER_ID)
ssize_t FILTER_NAME(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream){
  uint32_t me = FILTER_ID ;
  if(a == NULL || stream == NULL) goto fail ;    // no array or no stream
  void *array = array_address(a) ;               // get array address, dimension(s), and type
  int ndim = a->ndim, type = a->type ;
  ssize_t status = 0 ;
  bitstream s = *stream ;                        // local copy of stream control structure
  if(dpfl == NULL && bp == NULL) goto reverse ;  // call to reverse filter
  if(! dmap_filter_valid(dpfl,me)) goto fail ;   // not the right filter or NULL pointer
  FILTER_ARGS *arg = (FILTER_ARGS *)(*dpfl) ;    // get parameters for this filter

// check a->type and a->ndim
  if(type != float_data || ndim != 2) goto fail ;
// filter processing code goes here
  int i, j, lni = a->dim[0].gnn, lnj = a->dim[1].gnn ;
  float *f = (float *)array ;
  fprintf(stderr, "filter 007, offset =%f, scale = %f, array[%d:%d]", arg->offset, arg->scale, lni, lnj) ;
  fprintf(stderr, ", LL = %f, UR = %f, ", f[0] , f[lni*lnj-1]) ;
  for(j=0 ; j<lnj ; j++){
    for(i=0 ; i<lni ; i++){
      f[i + j*lni] = (f[i + j*lni] * arg->scale) + arg->offset ;
    }
  }
  fprintf(stderr, ", LL = %f, UR = %f\n", f[0] , f[lni*lnj-1]) ;
  fprintf(stderr, "filter 007(E) : available space in stream %ld bits\n", StreamAvailableSpace(&s)) ;

  dpfl++ ;                              // call next filter if there is one
  dmap_filter_ptr next_filter = dmap_filter_next(dpfl) ;
  status = (*next_filter)(a, bp, dpfl, &s) ;
  if(status < 0) goto fail ;
  fprintf(stderr, "filter 007(M) : status = %ld, available space in stream %ld bits\n", status, StreamAvailableSpace(stream)) ;

  STREAM_PUT_NBITS(s, FILTER_ID, 8) ;
  uint32_t *tmp1 = (uint32_t *) &(arg->offset) ;
  STREAM_PUT_NBITS(s, *tmp1, 32) ;
  uint32_t *tmp2 = (uint32_t *) &(arg->scale) ;
  STREAM_PUT_NBITS(s, *tmp2, 32) ;
  STREAM_INSERT_PUSH(s) ;
  fprintf(stderr, "filter 007(W) %3.3o, %8.8x , %8.8x\n", FILTER_ID, *tmp1, *tmp2) ;
  status += 72 ;

end:
  *stream = s ;   // SAVE stream changes
  fprintf(stderr, "filter 007(X) : available space in stream %ld bits\n", StreamAvailableSpace(stream)) ;
  return status ;

fail:
  fprintf(stderr, "filter 007 FAILED\n");
  return -1 ;     // DO NOT SAVE stream changes

  uint32_t w32 ;
reverse:
  STREAM_GET_NBITS(s, w32, 8) ;
//   fprintf(stderr, "reverse filter %3.3o, id = %d\n", FILTER_ID, w32) ;
  if(w32 != FILTER_ID) goto fail ;                     // wrong id, MUST be FILTER_ID
  uint32_t t1, t2 ;
  STREAM_GET_NBITS(s, t1, 32) ;
  STREAM_GET_NBITS(s, t2, 32) ;
  fprintf(stderr, "filter 007(R) %3.3o, %8.8x , %8.8x\n", FILTER_ID, t1, t2) ;
  status = 72 ;

  ssize_t status2 = dmap_filter_inv(a, &s) ;     // call next inverse filter
  if(status2 < 0) goto fail ; else status += status2 ;
  *stream = s ;
  return status ;
}
#undef FILTER_NAME
#undef FILTER_ARGS
#undef FILTER_ID

#if defined(COMPILE_FILTER_TEMPLATE_NEVER_TRUE)

#define FILTER_ID xxx
#define FILTER_NAME CONCAT2(dmap_filter_,FILTER_ID)
#define FILTER_ARGS CONCAT2(dmap_filter_arg_,FILTER_ID)
// dpfl == NULL && bp == NULL indicates reverse filter call
// for the reverse filter, the metadata from the bit stream provides the necessary information
// the filter may modify the contents of the array described by a
// in filter mode, bp == NULL if no properties information is available
// the filter list MUST BE NULL TERMINATED
ssize_t FILTER_NAME(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream){
  uint32_t me = FILTER_ID ;
  if(a == NULL || stream == NULL) goto fail ;    // no array or no stream
  void *array = array_address(a) ;               // get array address, dimension(s), and type
  int ndim = a->ndim, type = a->type ;
  ssize_t status = 0 ;
  bitstream s = *stream ;                        // local copy of stream control structure
  if(dpfl == NULL && bp == NULL) goto reverse ;  // call to reverse filter
  if(! dmap_filter_valid(dpfl,me)) goto fail ;   // not the right filter or NULL pointer
  FILTER_ARGS *arg = (FILTER_ARGS *)(*dpfl) ;    // get parameters for this filter

//
// check a->type and a->ndim
//
// filter processing code goes here
//
  dpfl++ ;                                     // call next filter if there is one
  dmap_filter_ptr next_filter = dmap_filter_next(dpfl) ;
  status = (*next_filter)(a, bp, dpfl, &s) ;
  if(status < 0) goto fail ;
//
// insert into bitstream the appropriate metadata for the reverse filter
//
end:
  *stream = s ;   // SAVE stream changes
  return status ;

fail:
  return -1 ;     // DO NOT SAVE stream changes

  uint32_t w32 ;
reverse:
  STREAM_GET_NBITS(s, w32, 8) ;
//   fprintf(stderr, "reverse filter %3.3o, id = %d\n", FILTER_ID, w32) ;
  if(w32 != FILTER_ID) goto fail ;                     // wrong id, MUST be FILTER_ID
//
// inverse filter processing code goes here
//
  ssize_t status2 = dmap_filter_inv(a, &s) ;     // call next inverse filter
  if(status2 < 0) goto fail ; else status += status2 ;
  *stream = s ;   // SAVE stream changes
  return status ;
}
#undef FILTER_NAME
#undef FILTER_ARGS
#undef FILTER_ID

#endif
