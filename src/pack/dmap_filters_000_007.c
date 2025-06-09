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
#include <rmn/ieee_functions.h>
#include <rmn/move_blocks.h>
#include <rmn/misc_operators.h>

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
//   block_properties lbp ;
// 
//   if(bp == NULL) { bp = &lbp ; lbp.kind = bad_data ; }
  if(dpfl == NULL) goto reverse ;                // call to reverse filter

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

  uint32_t filter ;
reverse:
  STREAM_GET_NBITS(s, filter, 8) ;
  status = 8 ;
//   fprintf(stderr, "reverse filter %3.3o, id = %d\n", FILTER_ID, filter) ;
  if(filter != FILTER_ID) goto fail ;                     // wrong id, MUST be FILTER_ID (0)

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
#define FILTER_NAME CAT(dmap_filter_,FILTER_ID)
#define FILTER_ARGS CAT(dmap_filter_arg_,FILTER_ID)
ssize_t FILTER_NAME(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream){
  uint32_t me = FILTER_ID ;
  if(a == NULL || stream == NULL) goto fail ;    // no array or no stream
  void *array = array_address(a) ;               // get array address, dimension(s), and type
  int ndim = a->ndim, type = a->type ;
  ssize_t status = 0 ;
  bitstream s = *stream ;                        // local copy of stream control structure
//   block_properties lbp ;
// 
//   if(bp == NULL) { bp = &lbp ; lbp.kind = bad_data ; }
  if(dpfl == NULL) goto reverse ;                // call to reverse filter
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

  uint32_t filter ;
reverse:
  STREAM_GET_NBITS(s, filter, 8) ;
//   fprintf(stderr, "reverse filter %3.3o, id = %d\n", FILTER_ID, filter) ;
  if(filter != FILTER_ID) goto fail ;                     // wrong id, MUST be FILTER_ID
  uint32_t t1, t2 ;
  STREAM_GET_NBITS(s, t1, 32) ;
  STREAM_GET_NBITS(s, t2, 32) ;
  fprintf(stderr, "reverse filter %3.3o, id = %d, args = %8.8x, %8.8x\n", FILTER_ID, filter, t1, t2) ;
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
#include <rmn/quantizers.h>

// 32 bit float pseudo log quantizer
#define FILTER_ID 002
#define FILTER_NAME CAT(dmap_filter_,FILTER_ID)
#define FILTER_ARGS CAT(dmap_filter_arg_,FILTER_ID)
ssize_t FILTER_NAME(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream){
  uint32_t me = FILTER_ID ;
  if(a == NULL || stream == NULL) goto fail ;    // no array or no stream
  void *array = array_address(a) ;               // get array address, dimension(s), and type
  int ndim = a->ndim, type = a->type ;
  ssize_t status = 0 ;
  bitstream s = *stream ;                        // local copy of stream control structure
//   block_properties lbp ;
// 
//   if(bp == NULL) { bp = &lbp ; lbp.kind = bad_data ; }
  if(dpfl == NULL) goto reverse ;                // call to reverse filter
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

  uint32_t filter ;
reverse:
  STREAM_GET_NBITS(s, filter, 8) ;
  status = 8 ;                                         // 8 bits extracted so far
//   fprintf(stderr, "reverse filter %3.3o, id = %d\n", FILTER_ID, filter) ;
  if(filter != FILTER_ID) goto fail ;                  // wrong id, MUST be FILTER_ID
  uint32_t t ;
  STREAM_GET_NBITS(s, t, 8) ;
  fprintf(stderr, "reverse filter %3.3o, id = %d, t = %d\n", FILTER_ID, filter, t) ;
  status += 8 ;

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

// 32 bit float quantizer
#define FILTER_ID 003
#define FILTER_NAME CAT(dmap_filter_,FILTER_ID)
#define FILTER_ARGS CAT(dmap_filter_arg_,FILTER_ID)
// dpfl == NULL means a reverse filter call
// for the reverse filter, the data from the bit stream provides the necessary information
// the filter may modify the contents of the array described by a and of the data properties bp
// in filter mode, bp == NULL if no properties information is available
// the filter list MUST BE NULL TERMINATED
ssize_t FILTER_NAME(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream){
  char *errmsg = "" ;
  uint32_t me = FILTER_ID ;
  if(a == NULL || stream == NULL) goto fail ;    // no array or no stream
  void *array = array_address(a) ;               // get array address, dimension(s), and type
  int ndim = a->ndim, type = a->type ;
  ssize_t status = 0 ;
  bitstream s = *stream ;                        // local copy of stream control structure
  block_properties lbp ;

  if(dpfl == NULL ) goto reverse ;               // this is a call to the reverse filter
  if(! dmap_filter_valid(dpfl,me)) goto fail ;   // not the right filter or NULL pointer
  FILTER_ARGS *arg = (FILTER_ARGS *)(*dpfl) ;    // get parameters for this filter
//
  if(type != float_data) goto fail ;             // data type MUST BE FLOAT
  if(ndim != 2) goto fail ;                      // only 2D is supported at this time
//
// filter processing code
//
  int32_t nbits = arg->nbits ;                   // max number of bits to be used for quantization
  if(nbits    < 0) goto fail ;                   // nbits MUST BE >= 0
  float maxerr = arg->maxerr ;                   // largest absolute/relative error desired
  if(maxerr < 0) goto fail ;                     // maxerr MUST BE >= 0
  float maxsig = arg->maxsig ;
  if(maxsig < 0) goto fail ;
  int32_t offset = arg->offset ;                 // discretization offset (ox7FFFFFFF means minimum quantized value)
  if(nbits == 0 && maxerr == 0) goto fail ;      // cannot be BOTH 0
  int32_t mode = arg->mode ;

  int32_t q_exp, nvalues, e_base ;
  nvalues = a->dim[0].gnn * a->dim[1].gnn ;      // number of values in array

#if 0
  if(bp == NULL) { bp = &lbp ; lbp.kind = bad_data ; }
  if(! data_kind_valid(bp->kind)){               // if the data properties are not valid
    analyze_data32_block((void *)array, a->dim[0].gnn, a->dim[0].gnn, a->dim[1].gnn, bp) ;
    adjust_block_properties(bp, float_data) ;    // adjust properties for float data
    fprintf(stderr, "filter %3.3o, computing data properties\n", FILTER_ID) ;
  }
  float maxabs, quantum, ovq ;
  maxabs = FLOAT_MAX_ABS(*bp) ;                  // largest absolute value in array

  // compute power of 2 to use for quantum given max error and nbits
  // discretization quantum, first power of 2 >= 2.0 * maxerr
  quantum = fp2q_quantum(maxabs, maxerr, nbits) ;
  q_exp = fp32_exp(quantum) ;
  ovq = fp32_pow2( -q_exp ) ;                    // discretization mutiplier, 1.0 / quantum

  fprintf(stderr, "filter %3.3o, maxerr = %f(%d), quantum = %f, ovq = %f, nbits = %d, offset = %d\n",
                  FILTER_ID, arg->maxerr,q_exp-1, quantum, ovq, arg->nbits, arg->offset) ;
  fprintf(stderr, "filter %3.3o, array[%d,%d](%d)\n", FILTER_ID, a->dim[0].gnn, a->dim[1].gnn, nvalues) ;
  if(offset == 0x7FFFFFFF){                      // use quantized value of minimum value as offset
    float minval = FLOAT_MIN_VALUE(*bp) ;        // minimum value
    offset = fp2q_lin_(minval, ovq) ;            // quantized value of minimum value in array
    fprintf(stderr, "filter %3.3o, setting offset to %d (%f)\n", FILTER_ID, offset, minval) ;
    a->type = uint_data ;                        // mark data as unsigned integer data
  }else{
    a->type = int_data ;                         // mark data as signed integer data
  }
#endif
// ==================== call fp32 quantizer ====================
//   e_base = fp2q_lin((float *)array, (int32_t *)array, nvalues, quantum, offset) ;
  a->type = (offset == 0x7FFFFFFF) ? uint_data : int_data ;
  e_base = fp2q_n((float *)array, (int32_t *)array, nvalues, bp, maxerr, maxsig, nbits, &offset, mode) ;

  dpfl++ ;                                       // call next filter if there is one
  dmap_filter_ptr next_filter = dmap_filter_next(dpfl) ;
  status = (*next_filter)(a, NULL, dpfl, &s) ;   // bloc properties are non longer valid
  if(status < 0) goto fail ;
//
// insert into bitstream the appropriate data for the reverse filter (PUT)
//
  int inserted = 0 ;
  STREAM_PUT_NBITS(s, FILTER_ID, 8) ;
  inserted += 8 ;                                 // 8 bits inserted so far
  STREAM_PUT_NBITS(s, (mode & 0x3), 2) ;
  inserted += 2 ;
  if(mode == FP_QUANTIZE_LOG){
    STREAM_PUT_NBITS(s, (nbits & 0x3F), 6) ;      // 0 -> 23 (only useful for pseudo log quantizer)
    inserted += 6 ;
  }
  q_exp = e_base & 0xFF ;                         // biased exponent from restoring
  STREAM_PUT_NBITS(s, q_exp, 8) ;
  inserted += 8 ;
  if(offset != 0){
    uint32_t zigzag = to_zigzag_32(offset) ;
    nbits = BitsNeeded_u32(zigzag) ;
    nbits = (nbits < 2) ? 2 : nbits ;             // nbits for offset SHALL NOT BE < 2
    STREAM_PUT_NBITS(s, (nbits-1), 5) ;
    inserted += 5 ;
    STREAM_PUT_NBITS(s, zigzag, nbits) ;
    inserted += nbits ;
  }else{
    nbits = 0 ;
    STREAM_PUT_NBITS(s, nbits, 5) ;               // nbits == 0 (decoded as 1) means offset is not used
    inserted += 5 ;
  }
//
  STREAM_INSERT_PUSH(s) ;
  status += inserted ;
  fprintf(stderr, "filter %3.3o : inserted %d bits, quantum = %f, exp = %d, offset = %d, mode = %d\n",
          FILTER_ID, inserted, fp32_pow2(e_base), q_exp, offset, mode) ;

end:
  *stream = s ;   // success, SAVE stream changes
  return status ;

fail:
  fprintf(stderr, "%s filter %3.3o ERROR : %s\n", (dpfl == NULL) ? "reverse" : "forward", FILTER_ID, errmsg) ;
  return -1 ;     // failure, DO NOT SAVE stream changes

  uint32_t filter, nbitsd ;
reverse:
// get from bitstream the appropriate information for the reverse filter (GET)
  STREAM_GET_NBITS(s, filter, 8) ;
  status = 8 ;                                         // 8 bits extracted so far
  if(filter != FILTER_ID) goto fail ;                  // wrong id, MUST be FILTER_ID
  STREAM_GET_NBITS(s, mode, 2) ;
  status += 2 ;
  if(mode == FP_QUANTIZE_LOG){                         // 0 -> 23 (only useful for pseudo log quantizer)
    STREAM_GET_NBITS(s, nbits, 6) ;
    status += 6 ;
  }
  STREAM_GET_NBITS(s, q_exp,  8) ;                     // quantum exponent (biased)
  STREAM_GET_NBITS(s, nbitsd, 5) ;                     // number of bits for offset
  status += 13 ;                                       // bits extracted so far
  if(nbitsd > 0){                                      // nbitsd == 0 means no offset
    uint32_t zigzag ;
    nbitsd++ ;
    STREAM_GET_NBITS(s, zigzag, nbitsd) ;              // zigzag encoding of offfset
    offset = from_zigzag_32(zigzag) ;                  // restore to signed integer
    status += nbitsd ;
  }else{
    offset = 0 ;
  }
  e_base = q_exp | (nbits << 8) ;
//   quantum = fp32_pow2(e_base) ;                        // float quantum
//   maxerr.u = (q_exp << 23) ; quantum = maxerr.f ;
  fprintf(stderr, "reverse filter %3.3o, id = %d, quantum = %f, nbits = %d, offset = %d, mode = %d\n",
          FILTER_ID, filter, fp32_pow2(e_base), nbits, offset, mode) ;
//
// inverse filter processing code goes here  (INV)
//
  nvalues = a->dim[0].gnn * a->dim[1].gnn ;
  fprintf(stderr, "reverse filter %3.3o, array[%d,%d](%d)\n", FILTER_ID, a->dim[0].gnn, a->dim[1].gnn, nvalues) ;
// ==================== call fp32 de-quantizer ====================
//   q2fp_lin((float *)array, (int32_t *)array, nvalues, e_base, offset) ;
//   status = q2fp_n((float *)array, (int32_t *)array, nvalues, e_base, nbits, offset, mode) ;
  status = q2fp_n((float *)array, (int32_t *)array, nvalues, e_base, offset, mode) ;
  if(status != 0) goto fail ;
  a->type = float_data ;                               // mark data as float data

  ssize_t status2 = dmap_filter_inv(a, &s) ;           // call next inverse filter
  if(status2 < 0) goto fail ; else status += status2 ;
  *stream = s ;                                        // SAVE stream changes
  return status ;
}
#undef FILTER_NAME
#undef FILTER_ARGS
#undef FILTER_ID

// ======================================= filter 004 =======================================
// Lorenzo predictor
#include <rmn/lorenzo.h>
#define FILTER_ID 004
#define FILTER_NAME CAT(dmap_filter_,FILTER_ID)
#define FILTER_ARGS CAT(dmap_filter_arg_,FILTER_ID)
// dpfl == NULL means a reverse filter call
// for the reverse filter, the metadata from the bit stream provides the necessary information
// the filter may modify the contents of the array described by a
// in filter mode, bp == NULL if no properties information is available
// the filter list MUST BE NULL TERMINATED
ssize_t FILTER_NAME(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream){
  char *errmsg = "" ;
  uint32_t me = FILTER_ID ;
  if(a == NULL || stream == NULL) goto fail ;    // no array or no stream
  void *array = array_address(a) ;               // get array address, dimension(s), and type
  int ndim = a->ndim, type = a->type ;
  ssize_t status = 0 ;
  bitstream s = *stream ;                        // local copy of stream control structure
//   block_properties lbp ;
// 
//   if(bp == NULL) { bp = &lbp ; lbp.kind = bad_data ; }
  if(dpfl == NULL) goto reverse ;                // call to reverse filter
  if(! dmap_filter_valid(dpfl,me)) goto fail ;   // not the right filter or NULL pointer
  FILTER_ARGS *arg = (FILTER_ARGS *)(*dpfl) ;    // get parameters for this filter
//
// check a->type and a->ndim
  if(type != int_data && type != uint_data) goto fail ;
  if(ndim != 2) goto fail ;
//
// filter processing code goes here
//
  // call Lorenzo predictor in place
  LorenzoPredictInplace((int32_t *)array, a->dim[0].gnn, a->dim[0].gnn, a->dim[1].gnn) ;
  a->type = int_data ;
  bp = NULL ;                                  // not used
  dpfl++ ;                                     // call next filter if there is one
  dmap_filter_ptr next_filter = dmap_filter_next(dpfl) ;
  status = (*next_filter)(a, bp, dpfl, &s) ;
  if(status < 0) goto fail ;
//
// insert into bitstream the appropriate data for the reverse filter
//
  STREAM_PUT_NBITS(s, FILTER_ID, 8) ;
  STREAM_INSERT_PUSH(s) ;
  status += 8 ;                                 // 8 bits inserted
end:
  *stream = s ;   // SAVE stream changes
  return status ;

fail:
  fprintf(stderr, "%s filter %3.3o ERROR : %s\n", (dpfl == NULL) ? "reverse" : "forward", FILTER_ID, errmsg) ;
  return -1 ;     // DO NOT SAVE stream changes

  uint32_t filter ;
reverse:
  STREAM_GET_NBITS(s, filter, 8) ;
  status = 8 ;                                         // 8 bits extracted so far
  fprintf(stderr, "reverse filter %3.3o, id = %d\n", FILTER_ID, filter) ;
  if(filter != FILTER_ID) goto fail ;                     // wrong id, MUST be FILTER_ID
//
// inverse filter processing code goes here
//
  errmsg = "expecting type == int_data" ;
  if(type != int_data) goto fail ;
  errmsg = "expecting 2 D array" ;
  if(ndim != 2)        goto fail ;
  // call Lorezo inverse predictor
  LorenzoUnpredictInplace((int32_t *)array, a->dim[0].gnn, a->dim[0].gnn, a->dim[1].gnn) ;
  a->type = int_data ;
  ssize_t status2 = dmap_filter_inv(a, &s) ;     // call next reverse filter
  if(status2 < 0) goto fail ; else status += status2 ;
  *stream = s ;   // SAVE stream changes
  return status ;
}
#undef FILTER_NAME
#undef FILTER_ARGS
#undef FILTER_ID

// ======================================= filter 005 =======================================
// integer wavelet transform
#define FILTER_ID 005
#define FILTER_NAME CAT(dmap_filter_,FILTER_ID)
#define FILTER_ARGS CAT(dmap_filter_arg_,FILTER_ID)
// dpfl == NULL means a reverse filter call
// for the reverse filter, the metadata from the bit stream provides the necessary information
// the filter may modify the contents of the array described by a
// in filter mode, bp == NULL if no properties information is available
// the filter list MUST BE NULL TERMINATED
ssize_t FILTER_NAME(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream){
  char *errmsg = "" ;
  uint32_t me = FILTER_ID ;
  if(a == NULL || stream == NULL) goto fail ;    // no array or no stream
  void *array = array_address(a) ;               // get array address, dimension(s), and type
  int ndim = a->ndim, type = a->type ;
  ssize_t status = 0 ;
  bitstream s = *stream ;                        // local copy of stream control structure
//   block_properties lbp ;
// 
//   if(bp == NULL) { bp = &lbp ; lbp.kind = bad_data ; }
  if(dpfl == NULL) goto reverse ;                // call to reverse filter
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
  STREAM_PUT_NBITS(s, FILTER_ID, 8) ;
  STREAM_INSERT_PUSH(s) ;
  status += 8 ;                                 // 8 bits inserted
end:
  *stream = s ;   // SAVE stream changes
  return status ;

fail:
  fprintf(stderr, "%s filter %3.3o ERROR : %s\n", (dpfl == NULL) ? "reverse" : "forward", FILTER_ID, errmsg) ;
  return -1 ;     // DO NOT SAVE stream changes

  uint32_t filter ;
reverse:
  STREAM_GET_NBITS(s, filter, 8) ;
//   fprintf(stderr, "reverse filter %3.3o, id = %d\n", FILTER_ID, filter) ;
  if(filter != FILTER_ID) goto fail ;                     // wrong id, MUST be FILTER_ID
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
#define FILTER_NAME CAT(dmap_filter_,FILTER_ID)
#define FILTER_ARGS CAT(dmap_filter_arg_,FILTER_ID)
// dpfl == NULL means a reverse filter call
// for the reverse filter, the metadata from the bit stream provides the necessary information
// the filter may modify the contents of the array described by a
// in filter mode, bp == NULL if no properties information is available
// the filter list MUST BE NULL TERMINATED
// this filter MUST BE THE LAST active filter in the chain as it encodes its data
ssize_t FILTER_NAME(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream){
  char *errmsg = "" ;
  uint32_t me = FILTER_ID ;
  if(a == NULL || stream == NULL) goto fail ;    // no array or no stream
  void *array = array_address(a) ;               // get array address, dimension(s), and type
  int ndim = a->ndim, type = a->type ;
  ssize_t status = 0 ;
  bitstream s = *stream ;                        // local copy of stream control structure
//   block_properties lbp ;
// 
//   if(bp == NULL) { bp = &lbp ; lbp.kind = bad_data ; }
  if(dpfl == NULL) goto reverse ;                // call to reverse filter
  if(! dmap_filter_valid(dpfl,me)) goto fail ;   // not the right filter or NULL pointer
  FILTER_ARGS *arg = (FILTER_ARGS *)(*dpfl) ;    // get parameters for this filter

// filter processing code goes here
  if(! dmap_filter_is_last(dpfl)){
    fprintf(stderr, "ERROR : filter 006 MUST BE THE LAST FILTER\n") ;
    goto fail ;
  }

  // data must be integer (signed or unsigned)
  if(type != int_data && type != uint_data) goto fail ;

  // mode ==  -1       : raw encoding using size from array descriptor
  // 0   <= mode <  65 : raw encoding using mode bits ( 0 - 64 )
  // 100 <= mode < 165 : force zigzag encoding using mode - 100 bits ( 0 - 64 )
  // mode >=  200      : tile encoding with tile size mode - 100 (nbits computed independently for each tile)
  // nbits == 0        : auto adjust number of bits and zigzag flag to data values
  // nbits > 0         : force value of nbits (at own risk)
  // mode 0 and 100 both set nbits to 0 (automatically compute necessary nbits)
  int32_t mode = arg->mode, nbits = 0, zigzag = 0 ;
  if(mode ==  -1) nbits = a->esize * 8 ;
  if(mode >=   0) { nbits = mode       ; zigzag = 0 ; }
  if(mode >= 100) { nbits = mode - 100 ; zigzag = 1 ; }
  if(mode >= 200) goto fail ;                     // not supported yet

  if(nbits > 32) goto fail ;                     // not supported yet
  fprintf(stderr, "filter 006(E) : available space in stream %ld bits, mode = %d, nbits = %d\n", StreamAvailableSpace(&s), mode, nbits) ;
//
  fprintf(stderr, "filter 006 saving %d array elements\n", array_dimension(a)) ;
//
  dpfl++ ;                                     // call next filter if there is one
  dmap_filter_ptr next_filter = dmap_filter_next(dpfl) ;
  status = (*next_filter)(a, bp, dpfl, &s) ;
  if(status < 0) goto fail ;
  fprintf(stderr, "filter 006(M) : status = %ld, available space in stream %ld bits\n", status, StreamAvailableSpace(&s)) ;
//
// insert into bitstream the appropriate data for the reverse filter
//
  uint32_t header ;
  uint32_t *z = (uint32_t *) array ;
  int32_t i, nelem = array_dimension(a) ;
// end:
  if(mode > 200){                // tile encoding not implemented yet

  }else{                         // constant nbits, 1 block, non tiling
    if(nbits == 0){              // find number of bits to use and appropriate value fot zigzag flag
      block_properties bp0 ;
      if(nelem != analyze_data32_block((void *)array, nelem, nelem, 1, &bp0)) goto fail ;
      if(bp0.mins.i < 0 && type == int_data){
        zigzag = 1 ;             // negative values are present, must use zigzag encoding
      }else{                     // negative values are not present, no need for zigzag
        zigzag = 0 ;             // max value will be used to determine nb of bits needed
        nbits = BitsNeeded_u32(bp0.maxu.u) ;
      }
      fprintf(stderr, "filter 006(W) : max = %d\n", bp0.maxu.u) ;
    }
    uint32_t umax = 0 ;
    if(zigzag == 1){       // if zigzag encoding is selected, use absolute max value to set nbits
      umax = v_to_zigzag_32_inplace((int32_t *)array, nelem) ;    // transform into zigzag form
      nbits = BitsNeeded_u32(umax) ;
    }
    header = 0b00110000 ;
    STREAM_PUT_NBITS(s, FILTER_ID, 8) ;
    STREAM_PUT_NBITS(s, header   , 8) ;
    STREAM_PUT_NBITS(s, nbits-1  , 7) ;
    STREAM_PUT_NBITS(s, zigzag   , 1) ;
    status += 24 ;
    for(i=0 ; i<nelem ; i++) { STREAM_PUT_NBITS(s, z[i], nbits) ; status += nbits ; } ;
    fprintf(stderr, "filter 006(X) : mode = %d, zigzag = %d, umax = %d, nbits = %d\n", mode, zigzag, umax, nbits) ;
    STREAM_INSERT_PUSH(s) ;
  }
  *stream = s ;   // SAVE stream changes
  fprintf(stderr, "filter 006(X) : available space in stream %ld bits\n", StreamAvailableSpace(stream)) ;
  return status ;

fail:
  fprintf(stderr, "%s filter %3.3o ERROR : %s\n", (dpfl == NULL) ? "reverse" : "forward", FILTER_ID, errmsg) ;
  return -1 ;     // DO NOT SAVE stream changes

// decode bit stream encoded by forward filter
  int32_t *sz = (int32_t *) array ;
  uint32_t filter ;
reverse:
  STREAM_GET_NBITS(s, filter, 8) ;
  if(filter != FILTER_ID) goto fail ;                   // wrong id, MUST be FILTER_ID

  STREAM_GET_NBITS(s, header, 8) ;
  if(header == 0b00110000){                              // nbits per value, no tiling
    STREAM_GET_NBITS(s, nbits , 7) ; nbits++ ;           // nbits -1 was encoded
    STREAM_GET_NBITS(s, zigzag, 1) ;                     // zigzag mode
    status = 24 ;
    fprintf(stderr, "reverse filter %3.3o, id = %d, header = 0x%2.2x, nbits = %d\n", FILTER_ID, filter, header, nbits) ;
    if(nbits > 32)           goto fail ;                 // not supported yet
    nelem = array_dimension(a) ;
    z = (uint32_t *) array ;
    int32_t max ;
    fprintf(stderr, "reverse filter %3.3o restoring %d array elements\n", FILTER_ID, nelem) ;
    for(i=0 ; i<nelem ; i++) { STREAM_GET_NBITS(s, z[i], nbits) ; status += nbits ; } ;
    if(zigzag != 0){                                     // restore from zigzag
      max = v_from_zigzag_32_inplace((int32_t *)array, nelem) ;
    }
    fprintf(stderr, "reverse filter %3.3o, max = %d, zigzag = %d\n", FILTER_ID, max, zigzag) ;
  }else{
    goto fail ;
  }
  a->type = int_data ;

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
#define FILTER_NAME CAT(dmap_filter_,FILTER_ID)
#define FILTER_ARGS CAT(dmap_filter_arg_,FILTER_ID)
ssize_t FILTER_NAME(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream){
  uint32_t me = FILTER_ID ;
  if(a == NULL || stream == NULL) goto fail ;    // no array or no stream
  void *array = array_address(a) ;               // get array address, dimension(s), and type
  int ndim = a->ndim, type = a->type ;
  ssize_t status = 0 ;
  bitstream s = *stream ;                        // local copy of stream control structure
//   block_properties lbp ;
// 
//   if(bp == NULL) { bp = &lbp ; lbp.kind = bad_data ; }
  if(dpfl == NULL) goto reverse ;                // call to reverse filter
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

  uint32_t filter ;
reverse:
  STREAM_GET_NBITS(s, filter, 8) ;
//   fprintf(stderr, "reverse filter %3.3o, id = %d\n", FILTER_ID, filter) ;
  if(filter != FILTER_ID) goto fail ;                     // wrong id, MUST be FILTER_ID
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
