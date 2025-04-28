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
  fprintf(stderr, "dmap_filter_fwd(HEAD) : inserted %d bits\n", nbits) ;

  // call next filter in list
  dmap_filter_ptr next_filter = dmap_filter_next(dpfl) ;
  status = (*next_filter)(a, bp, dpfl, &s) ;
  if(status < 0) goto fail ;
//   fprintf(stderr, "dmap_filter_fwd(MID) : nbits in stream = %ld\n", nbits+status) ;

end:
  // put end of filter chain data marker at the end of the bit stream
  STREAM_PUT_NBITS(s, FILTER_CHAIN_END, 8) ;
  nbits += 8 ;
  *stream = s ;   // SAVE stream changes
  status += nbits ;
  fprintf(stderr, "dmap_filter_fwd(TAIL) : inserted %d bits, bits in stream = %ld\n", nbits, status) ;
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
#include <rmn/quantizers.h>
#include <rmn/ieee_functions.h>
#include <rmn/move_blocks.h>
#include <rmn/misc_operators.h>

// 32 bit float linear quantizer
#define FILTER_ID 003
#define FILTER_NAME CONCAT2(dmap_filter_,FILTER_ID)
#define FILTER_ARGS CONCAT2(dmap_filter_arg_,FILTER_ID)
// dpfl == NULL && bp == NULL indicates reverse filter call
// for the reverse filter, the metadata from the bit stream provides the necessary information
// the filter may modify the contents of the array described by a and of the data properties bp
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
  if(type != float_data) goto fail ;             // data type MUST BE FLOAT
  if(ndim != 2) goto fail ;                      // only 2D is supported at this time
//
// filter processing code goes here  (FWD)
//
  int32_t nbits = arg->nbits ;
  iuf32_t maxerr ; maxerr.f = arg->maxerr ;      // largest absolute error desired
  iuf32_t offset ; offset.i = arg->offset ;      // discretization offset
  if(maxerr.f < 0) goto fail ;                   // maxerr MUST BE >= 0
  if(nbits    < 0) goto fail ;                   // nbits MUST BE >= 0
  if(nbits == 0 && maxerr.f == 0) goto fail ;    // BOTH cannot be 0

  if(! data_kind_valid(bp->kind)){               // if the data properties are not valid
    analyze_data32_block((void *)array, a->dim[0].gnn, a->dim[0].gnn, a->dim[1].gnn, bp) ;
    adjust_block_properties(bp, float_data) ;
    fprintf(stderr, "filter %3.3o, computing data properties\n", FILTER_ID) ;
  }

  int32_t q_exp, nvalues ;
  float maxabs, quantum, ovq ;
  maxabs = FLOAT_MAX_ABS(*bp) ;                  // largest absolute value in array

  q_exp = fp2q_exp(maxabs, maxerr.f, nbits) ;
  quantum = fp32_pow2(q_exp) ;                   // discretization quantum, first power of 2 >= maxerr
  ovq = fp32_pow2( -q_exp ) ;                    // discretization mutiplier, inverse of quantum

  nvalues = a->dim[0].gnn * a->dim[1].gnn ;      // number of values in array
  fprintf(stderr, "filter %3.3o, maxerr = %f(%d), quantum = %f, ovq = %f, nbits = %d, offset = %d\n",
                  FILTER_ID, arg->maxerr,q_exp-1, quantum, ovq, arg->nbits, arg->offset) ;
  fprintf(stderr, "filter %3.3o, array[%d,%d](%d)\n", FILTER_ID, a->dim[0].gnn, a->dim[1].gnn, nvalues) ;
  if(offset.u == 0x7FFFFFFF){                    // use quantized value of minimum value as offset
    float minval = FLOAT_MIN_VALUE(*bp) ;        // minimum value
    offset.i = fp2q_lin_(minval, ovq) ;          // quantized value of minimum value
    fprintf(stderr, "filter %3.3o, setting offset to %d (%f)\n", FILTER_ID, offset.i, minval) ;
  }

//   block_properties bp_in, bp_out ;
//   int nvalues_in = analyze_data32_block((void *) array, a->dim[0].gnn, a->dim[0].gnn, a->dim[1].gnn, &bp_in) ;
//   if(nvalues_in != a->dim[0].gnn *a->dim[1].gnn){
//     fprintf(stderr, "ERROR: expecting %d values, got %d\n", a->dim[0].gnn *a->dim[1].gnn, nvalues_in) ;
//     goto fail ;
//   }
//   adjust_block_properties(&bp_in, float_data);
//   print_float_props(bp_in);
// ==================== call linear fp32 quantizer ====================
  fp2q_lin((float *)array, (int32_t *)array, nvalues, ovq, offset.i);
  a->type = int_data ;

//   int nvalues_out = analyze_data32_block((void *) array, a->dim[0].gnn, a->dim[0].gnn, a->dim[1].gnn, &bp_in) ;
//   if(nvalues_out != a->dim[0].gnn *a->dim[1].gnn){
//     fprintf(stderr, "ERROR: expecting %d values, got %d\n", a->dim[0].gnn *a->dim[1].gnn, nvalues_out) ;
//     goto fail ;
//   }
//   adjust_block_properties(&bp_in, int_data);
//   print_int_props(bp_in);
//   fprintf(stderr, "filter %3.3o, values = %d %d %d\n", FILTER_ID, a->dim[0].gnn *a->dim[1].gnn, nvalues_in, nvalues_out) ;
//
  dpfl++ ;                                     // call next filter if there is one
  dmap_filter_ptr next_filter = dmap_filter_next(dpfl) ;
  status = (*next_filter)(a, bp, dpfl, &s) ;
  if(status < 0) goto fail ;
//
// insert into bitstream the appropriate information for the reverse filter (PUT)
//
  int inserted = 0 ;
  STREAM_PUT_NBITS(s, FILTER_ID, 8) ;
  inserted += 8 ;                                 // 8 bits inserted so far
  q_exp = fp32_exp(quantum) + 127 ;               // biased exponent from quantum
  STREAM_PUT_NBITS(s, q_exp, 8) ;
  inserted += 8 ;
  if(offset.i != 0){
    uint32_t zigzag = to_zigzag_32(offset.i) ;
    nbits = BitsNeeded_u32(zigzag) ;
    nbits = (nbits < 2) ? 2 : nbits ;             // nbits for offset SHALL NOT BE < 2
    STREAM_PUT_NBITS(s, (nbits-1), 5) ;
    inserted += 5 ;
    STREAM_PUT_NBITS(s, zigzag, nbits) ;
    inserted += nbits ;
  }else{
    nbits = 0 ;
    STREAM_PUT_NBITS(s, nbits, 5) ;
    inserted += 5 ;
  }
//
  STREAM_INSERT_PUSH(s) ;
  status += inserted ;
  fprintf(stderr, "filter %3.3o : inserted %d bits, quantum = %f, exp = %d\n", FILTER_ID, inserted, quantum, q_exp) ;
end:
  *stream = s ;   // success, SAVE stream changes
  return status ;

fail:
  return -1 ;     // failure, DO NOT SAVE stream changes

  uint32_t w32 ;
reverse:
// get from bitstream the appropriate information for the reverse filter (GET)
  status = 8 ;                                         // 8 bits extracted so far
  STREAM_GET_NBITS(s, w32, 8) ;
  if(w32 != FILTER_ID) goto fail ;                     // wrong id, MUST be FILTER_ID
  STREAM_GET_NBITS(s, q_exp, 8) ;
  STREAM_GET_NBITS(s, nbits, 5) ; nbits++ ;
  status += 13 ;
  if(nbits > 1){                                       // nbits == 0 means no offset
    uint32_t zigzag ;
    STREAM_GET_NBITS(s, zigzag, nbits) ;
    offset.i = from_zigzag_32(zigzag) ;
    status += nbits ;
  }else{
    offset.i = 0 ;
  }
  maxerr.u = (q_exp << 23) ; quantum = maxerr.f ;
  fprintf(stderr, "reverse filter %3.3o, id = %d, quantum = %f, nbits = %d, offset = %d\n", FILTER_ID, w32, quantum, nbits, offset.i) ;
//
// inverse filter processing code goes here  (INV)
//
  nvalues = a->dim[0].gnn * a->dim[1].gnn ;
  fprintf(stderr, "reverse filter %3.3o, array[%d,%d](%d)\n", FILTER_ID, a->dim[0].gnn, a->dim[1].gnn, nvalues) ;
//   nvalues_in = analyze_data32_block((void *) array, a->dim[0].gnn, a->dim[0].gnn, a->dim[1].gnn, &bp_in) ;
//   if(nvalues != nvalues_in){
//     fprintf(stderr, "ERROR: expecting %d values in, got %d\n", a->dim[0].gnn *a->dim[1].gnn, nvalues_in) ;
//     goto fail ;
//   }
//   adjust_block_properties(&bp_in, int_data);
//   print_int_props(bp_in);
// ==================== call linear fp32 de-quantizer code ====================
  q2fp_lin((float *)array, (int32_t *)array, nvalues, quantum, offset.i) ;
  a->type = float_data ;

//   nvalues_out = analyze_data32_block((void *) array, a->dim[0].gnn, a->dim[0].gnn, a->dim[1].gnn, &bp_in) ;
//   if(nvalues != nvalues_out){
//     fprintf(stderr, "ERROR: expecting %d values out, got %d\n", a->dim[0].gnn *a->dim[1].gnn, nvalues_out) ;
//     goto fail ;
//   }
//   adjust_block_properties(&bp_in, float_data);
//   print_float_props(bp_in);

  ssize_t status2 = dmap_filter_inv(a, &s) ;           // call next inverse filter
  if(status2 < 0) goto fail ; else status += status2 ;
  *stream = s ;                                        // SAVE stream changes
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
// this filter MUST BE THE LAST active filter in the chain as it encodes its data
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

  // mode ==   0      : raw encoding using size from array descriptor
  // 0   < mode <  65 : raw encoding using mode bits ( 1 - 64 )
  // 100 < mode < 165 : zigzag encoding using mode - 100 bits ( 1 - 64 )
  // mode >  100      : tile encoding with tile size mode - 100
  int mode = arg->mode, nbits = 0, zigzag = 0 ;
  if(mode > 164) goto fail ;                     // unsupported for now
  if(mode < 65) { nbits = mode ; zigzag = 0 ; }
  if(mode > 100) { nbits = mode - 100 ; zigzag = 1 ; }
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
  int32_t *sz = (int32_t *) array ;
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
  if(mode < 165){
    int32_t *zz = (int32_t *)z, min = 0x7FFFFFFF, max = -min ;
    uint32_t umax = 0 ;
    for(i=0 ; i<nelem ; i++) { min = (sz[i] < min) ? sz[i] : min ; max = (sz[i] > max) ? sz[i] : max ; } ;
    if(zigzag != 0){
      for(i=0 ; i<nelem ; i++) { z[i] = to_zigzag_32(sz[i]) ; umax = (z[i] > umax) ? z[i] : umax ; } ;
      nbits = BitsNeeded_u32(umax) ;
    }
    header = 0b00110000 ;
    STREAM_PUT_NBITS(s, FILTER_ID, 8) ;
    STREAM_PUT_NBITS(s, header , 8) ;
    STREAM_PUT_NBITS(s, nbits-1, 8) ;
    STREAM_PUT_NBITS(s, zigzag, 1) ;
    status += 25 ;
    for(i=0 ; i<nelem ; i++) { STREAM_PUT_NBITS(s, z[i], nbits) ; status += nbits ; } ;
    fprintf(stderr, "filter 006(X) : mode = %d, min = %d, max = %d, zigzag = %d, umax = %d, nbits = %d\n", mode, min, max, zigzag, umax, nbits) ;
    STREAM_INSERT_PUSH(s) ;
  }
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
  STREAM_GET_NBITS(s, zigzag, 1) ;
  fprintf(stderr, "reverse filter %3.3o, id = %d, header = %2.2x, nbits = %d\n", FILTER_ID, w32, header, nbits) ;
  if(w32 != FILTER_ID)     goto fail ;
  if(header != 0b00110000) goto fail ;
  if(nbits > 32)           goto fail ;                    // unsupported for now
  nelem = array_dimension(a) ;
  z = (uint32_t *) array ;
  sz = (int32_t *) array ;
  fprintf(stderr, "reverse filter %3.3o restoring %d array elements\n", FILTER_ID, nelem) ;
  status = 24 ;
  for(i=0 ; i<nelem ; i++) { STREAM_GET_NBITS(s, z[i], nbits) ; status += nbits ; } ;
  if(zigzag != 0){
    for(i=0 ; i<nelem ; i++) { sz[i] = from_zigzag_32(z[i]) ; } ;
  }
  int32_t *zz = (int32_t *)z, min = 0x7FFFFFFF, max = -min ;
  for(i=0 ; i<nelem ; i++) { min = (zz[i] < min) ? zz[i] : min ; max = (zz[i] > max) ? zz[i] : max ; } ;
  fprintf(stderr, "reverse filter %3.3o min = %d, max = %d, zigzag = %d\n", FILTER_ID, min, max, zigzag) ;

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
