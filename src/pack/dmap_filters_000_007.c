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
#include <rmn/ieee_extras.h>
#include <rmn/move_blocks.h>
#include <rmn/misc_operators.h>
#include <rmn/tile_encoders.h>
#include <rmn/dwt_i_lgt53.h>
#include <rmn/fp_qlin.h>
#include <rmn/fp_qflog.h>

#define FILTER_ID 000
// ======================================= filter 000 =======================================
// special filter used to get/put array dimensions and type information (found in dmap_filters.c)
// this filter writes into bit stream BEFORE calling the filter chain and AFTER calling said chain
// this filter expects NO ARGUMENT from the filter list
// this filter MUST be called first
// in reverse mode, it makes sure that the target array has the correct configuration
// for data type and dimensions
// TODO: allocate memory for the target array if necessary
// this filter will be the first to be called in reverse mode (get/check rank and dimensions)
ssize_t dmap_filter_fwd(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream){
  uint32_t self = FILTER_ID ;
  if(a == NULL || stream == NULL) goto fail ;    // no array or no stream
  void *array = array_address(a) ;               // get array address, dimension(s), and type
  ssize_t status = 0 ;
  bitstream s = *stream ;                        // local copy of stream control structure
  int32_t nbits ;

  if(dpfl == NULL) goto reverse ;                // call to reverse filter

  STREAM_PUT_NBITS(s, self, 8) ; nbits = 8 ;     // insert own filter id into stream
  // insert array description information into the bit stream
  nbits += dmap_filter_put_array_info(a, &s) ;
//   fprintf(stderr, "dmap_filter_fwd(HEAD) : inserted %d bits\n", nbits) ;

  // call next filter in list
  // DO NOT USE dpfl++, dmap_filter_fwd is a filter implicitely in the list
  dmap_filter_ptr next_filter = dmap_filter_next(dpfl) ;
  status = (*next_filter)(a, bp, dpfl, &s) ;
  if(status < 0) goto fail ;
//   fprintf(stderr, "dmap_filter_fwd(MID) : nbits in stream = %ld\n", nbits+status) ;

  // insert the FILTER_CHAIN_END marker into the bit stream
  STREAM_PUT_NBITS(s, FILTER_CHAIN_END, 8) ;
  nbits += 8 ;
  STREAM_INSERT_ALIGN32(s) ;        // align stream to a 32 bit boundary
  STREAM_FLUSH(s) ;                 // flush stream data

  *stream = s ;   // SAVE stream changes
  status += nbits ;
//   fprintf(stderr, "dmap_filter_fwd(TAIL) : inserted %d bits, bits in stream = %ld\n", nbits, status) ;
  return status ;

fail:
  return -1 ;     // DO NOT SAVE stream changes

reverse:
  STREAM_GET_NBITS(s, self, 8) ;
  status = 8 ;
//   fprintf(stderr, "in reverse filter %3.3o, id = %d\n", FILTER_ID, self) ;
  if(self != FILTER_ID) goto fail ;                     // wrong id, MUST be FILTER_ID (0)

  // get array description from stream
  int32_t temp = dmap_filter_get_array_info(a, &s, 1) ;
  if(temp < 0) goto fail ;
  status += temp ;
  array_set_empty(a) ;                                  // mark array as having no valid data

  ssize_t status2 = dmap_filter_inv(a, &s) ;            // call next inverse filter
  if(status2 < 0) goto fail ; else status += status2 ;
  *stream = s ;   // SAVE stream changes
  return status ;
}
#undef FILTER_ID

// ======================================= filter 001 =======================================
// integer/float offset and scale
#define FILTER_ID 001
#define FILTER_NAME CAT(dmap_filter_,FILTER_ID)
#define FILTER_ARGS CAT(dmap_filter_arg_,FILTER_ID)
ssize_t FILTER_NAME(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream){
  uint32_t self = FILTER_ID ;
  if(a == NULL || stream == NULL) goto fail ;    // no array or no stream
  void *array = array_address(a) ;               // get array address, dimension(s), and type
  int rank = a->rank, type = a->type ;
  ssize_t status = 0 ;
  bitstream s = *stream ;                        // local copy of stream control structure
//   block_properties lbp ;
// 
//   if(bp == NULL) { bp = &lbp ; lbp.kind = bad_data ; }
  if(dpfl == NULL) goto reverse ;                // call to reverse filter
  if(! dmap_filter_valid(dpfl,self)) goto fail ;   // not the right filter or NULL pointer
  FILTER_ARGS *arg = (FILTER_ARGS *)(*dpfl) ;    // get parameters for this filter
// check a->type and a->rank
  if(type != float_data || rank != 2) goto fail ;
// filter processing code goes here
  int i, j, lni = a->dim[0].gnn, lnj = a->dim[1].gnn ;
  float *f = (float *)array ;
  fprintf(stderr, "filter 001, offset =%f, scale = %f, array[%d:%d]", arg->offset, arg->scale, lni, lnj) ;
  fprintf(stderr, ", LL = %f, UR = %f, ", f[0] , f[lni*lnj-1]) ;
  for(j=0 ; j<lnj ; j++){
    for(i=0 ; i<lni ; i++){
      f[i + j*lni] = (f[i + j*lni] * arg->scale) + arg->offset ;
    }
  }
//   fprintf(stderr, ", LL = %f, UR = %f\n", f[0] , f[lni*lnj-1]) ;

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

// end:
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
  uint32_t self = FILTER_ID ;
  if(a == NULL || stream == NULL) goto fail ;    // no array or no stream
  void *array = array_address(a) ;               // get array address, dimension(s), and type
//   int rank = a->rank, type = a->type ;
  ssize_t status = 0 ;
  bitstream s = *stream ;                        // local copy of stream control structure
//   block_properties lbp ;
// 
//   if(bp == NULL) { bp = &lbp ; lbp.kind = bad_data ; }
  if(dpfl == NULL) goto reverse ;                // call to reverse filter
  if(! dmap_filter_valid(dpfl,self)) goto fail ;   // not the right filter or NULL pointer
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

// end:
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
#include <rmn/fp_qflog.h>

// 32 bit float quantizer
#define FILTER_ID 003
#define FILTER_NAME CAT(dmap_filter_,FILTER_ID)
#define FILTER_ARGS CAT(dmap_filter_arg_,FILTER_ID)
// dpfl == NULL : reverse filter call (no list needed)
// for the reverse filter, the bit stream provides the necessary information
// the filter may modify the contents of the array described by a and of the data properties bp
// in filter mode, bp == NULL if no properties information is available
// the filter list MUST BE NULL TERMINATED
ssize_t FILTER_NAME(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream){
  char *errmsg = "" ;
  uint32_t self = FILTER_ID, filter ;
  errmsg = "a == NULL || stream == NULL" ;
  if(a == NULL || stream == NULL) goto fail ;    // no array or no stream
  void *array = array_address(a) ;               // get array address, dimension(s), and type
  int32_t nvalues, rank = a->rank, type = a->type, offset = 0,  e_base = 0 ;
  ssize_t status = 0 ;
  bitstream s = *stream ;                        // local copy of stream control structure

  errmsg = "rank != 2" ;
  if(rank != 2) goto fail ;                      // only 2D is supported at this time
  nvalues = a->dim[0].gnn * a->dim[1].gnn ;      // number of values in array

  if(dpfl == NULL ) goto reverse ;               // this is a call to the reverse filter

  errmsg = "invalid filter" ;
  if(! dmap_filter_valid(dpfl,self)) goto fail ;   // not the right filter or NULL pointer

  errmsg = "type != float_data" ;
  if(type != float_data) goto fail ;             // data type MUST BE FLOAT

  FILTER_ARGS *arg = (FILTER_ARGS *)(*dpfl) ;    // get parameters for this filter
//
// filter processing code
//
  int32_t mode = arg->mode ;                     // get mode
  int32_t nbits = arg->nbits ;                   // max number of bits to be used for quantization
  errmsg = "nbits < 0" ;
  if(nbits    < 0) goto fail ;                   // nbits MUST BE >= 0
  float maxerr = arg->maxerr ;                   // largest absolute/relative error desired
  errmsg = "maxerr < 0" ;
  if(maxerr < 0) goto fail ;                     // maxerr MUST BE >= 0
  errmsg = "maxerr and nbits both 0" ;
  if(nbits == 0 && maxerr == 0) goto fail ;      // cannot be BOTH 0

  float minabs = arg->minabs ;

  if(mode == FP_2_INT){       // linear quantizer
    offset = arg->offset ;                       // discretization offset
// fprintf(stderr,"nvalues = %d, maxerr = %f, nbits = %d, offset = %8.8x, bp = %p\n", nvalues, maxerr, nbits, offset, bp) ;
    e_base = fp_to_qlin((float *)array, (int32_t *)array, nvalues, maxerr, nbits, &offset, bp) ;
    if(e_base <= 0) goto fail ;
    a->type = (offset == 0x7FFFFFFF) ? uint_data : int_data ;
  }else if(mode == FP_2_FAKELOG){
    errmsg = "minabs < 0" ;
    if(minabs < 0) goto fail ;
    errmsg = "mode == FP_2_FAKELOG, unsupported" ;
    goto fail ;        // fake log quantizer not supported yet
  }else{
    errmsg = "invalid mode" ;
    goto fail ;        // invalid mode
  }

  dpfl++ ;                                       // call next filter if there is one
  dmap_filter_ptr next_filter = dmap_filter_next(dpfl) ;
  status = (*next_filter)(a, NULL, dpfl, &s) ;   // block properties are no longer valid
  if(status < 0) goto fail ;
//
// insert into bitstream the appropriate data for the reverse filter (PUT)
//
  int inserted = 0 ;
  STREAM_PUT_NBITS(s, FILTER_ID, 8) ;
  inserted += 8 ;                                 // 8 bits inserted so far
  STREAM_PUT_NBITS(s, (mode & 0x3), 2) ;
  inserted += 2 ;
  if(mode == FP_2_INT){
    STREAM_PUT_NBITS(s, e_base, 8) ;
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
  }else if(mode == FP_2_FAKELOG){
    STREAM_PUT_NBITS(s, (nbits & 0x3F), 6) ;        // 0 -> 23 (only useful for fake log quantizer)
    inserted += 6 ;
  }
  STREAM_INSERT_PUSH(s) ;
  status += inserted ;
//   fprintf(stderr, "filter %3.3o : inserted %d bits, quantum = %f, exp = %d, offset = %d, mode = %d\n",
//           FILTER_ID, inserted, fp32_pow2(e_base-127), e_base, offset, mode) ;

// end:
  *stream = s ;   // success, SAVE stream changes
  return status ;

fail:
  fprintf(stderr, "%s filter %3.3o ERROR : %s\n", (dpfl == NULL) ? "reverse" : "forward", FILTER_ID, errmsg) ;
  return -1 ;     // failure, DO NOT SAVE stream changes

reverse:
  errmsg = "reverse filter : data type MUST BE integer" ;
  if(type != int_data && type != uint_data) goto fail ;             // data type MUST BE INTEGER
// get the appropriate information for the reverse filter from bitstream (GET)
  filter = 0xFFFFu ;
  STREAM_GET_NBITS(s, filter, 8) ;
  status = 8 ;                                         // 8 bits extracted so far
  errmsg = "bad filter ID" ;
  if(filter != FILTER_ID) goto fail ;                  // wrong id, MUST be FILTER_ID
  STREAM_GET_NBITS(s, mode, 2) ;
  status += 2 ;
  if(mode == FP_2_INT){
    uint32_t ue_base ;
    STREAM_GET_NBITS(s, ue_base, 8) ; e_base = ue_base ;
    status += 8 ;
    STREAM_GET_NBITS(s, nbits, 5) ; nbits++ ;
    status += 5 ;
    offset = 0 ;
    if(nbits > 1){
      uint32_t zigzag ;
      STREAM_GET_NBITS(s, zigzag, nbits) ;             // zigzag encoding of offfset
      status += nbits ;
      offset = from_zigzag_32(zigzag) ;                // restore offset to signed integer
    }
    qflin_to_fp((float *)array, (int32_t *)array, nvalues, e_base, offset) ;
  }else if(mode == FP_2_FAKELOG){                      // 0 -> 23 (only useful for pseudo log quantizer)
    STREAM_GET_NBITS(s, nbits, 6) ;
    status += 6 ;
  }
//   fprintf(stderr, "reverse filter %3.3o, array[%d,%d](%d), offset = %d\n",
//                   FILTER_ID, a->dim[0].gnn, a->dim[1].gnn, nvalues, offset) ;
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
// dpfl == NULL : reverse filter call (no list needed)
// for the reverse filter, the bit stream provides the necessary information
// the filter may modify the contents of the array described by a
// in filter mode, bp == NULL if no properties information is available
// the filter list MUST BE NULL TERMINATED
ssize_t FILTER_NAME(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream){
  char *errmsg = "" ;
  uint32_t self = FILTER_ID ;
  errmsg = "no array" ;
  if(a == NULL) goto fail ;                      // no array
  errmsg = "no stream" ;
  if(stream == NULL) goto fail ;                 // no stream
  void *array = array_address(a) ;               // get array address, dimension(s), and type
  int rank = a->rank, type = a->type ;
  ssize_t status = 0 ;
  bitstream s = *stream ;                        // local copy of stream control structure

  if(dpfl == NULL) goto reverse ;                // reverse filter

  errmsg = "invalid/inconsistent filter ID" ;
  if(! dmap_filter_valid(dpfl,self)) goto fail ;   // not the right filter or NULL pointer
  // there are no specific parameters for this filter
  errmsg = "expecting int_data or uint_data" ;
  if(type != int_data && type != uint_data) goto fail ;    // integers only
  errmsg = "expecting 2D array" ;
  if(rank != 2) goto fail ;                                // 2 D only
  // call Lorenzo predictor in place
  LorenzoPredictInplace((int32_t *)array, a->dim[0].gnn, a->dim[0].gnn, a->dim[1].gnn) ;
//   fprintf(stderr, "filter %3.3o(X) Lorenzo prediction performed \n", FILTER_ID) ;
  a->type = int_data ;
  bp = NULL ;                                    // not used
  dpfl++ ;                                       // call next filter if there is one
  dmap_filter_ptr next_filter = dmap_filter_next(dpfl) ;
  status = (*next_filter)(a, bp, dpfl, &s) ;
  if(status < 0) goto fail ;
//
// insert the appropriate data into bitstream for the reverse filter
//
  STREAM_PUT_NBITS(s, FILTER_ID, 8) ;            // Lorenzo marker
  STREAM_INSERT_PUSH(s) ;
  status += 8 ;                                  // 8 bits inserted

  *stream = s ;   // SAVE stream changes
  return status ;

fail:
  fprintf(stderr, "%s filter %3.3o ERROR : %s\n", (dpfl == NULL) ? "reverse" : "forward", FILTER_ID, errmsg) ;
  return -1 ;     // DO NOT SAVE stream changes

reverse:
  errmsg = "inconsistent filter ID" ;
  uint32_t filter ;
  STREAM_GET_NBITS(s, filter, 8) ;
  status = 8 ;                                         // 8 bits extracted so far
//   fprintf(stderr, "reverse filter %3.3o, id = %d\n", FILTER_ID, filter) ;
  if(filter != FILTER_ID) goto fail ;                     // wrong id, MUST be FILTER_ID
  errmsg = "expecting int_data" ;
  if(type != int_data) goto fail ;
  errmsg = "expecting 2D array" ;
  if(rank != 2)        goto fail ;
  // call Lorenzo inverse predictor in place
  LorenzoUnpredictInplace((int32_t *)array, a->dim[0].gnn, a->dim[0].gnn, a->dim[1].gnn) ;
// fprintf(stderr, "filter %3.3o(E) Lorenzo restore performed [%d x %d]\n", FILTER_ID, a->dim[0].gnn, a->dim[1].gnn) ;
  a->type = int_data ;

  ssize_t status2 = dmap_filter_inv(a, &s) ;     // call next reverse filter
  if(status2 < 0) goto fail ; else status += status2 ;
// fprintf(stderr, "filter %3.3o(END) Lorenzo post  [%d x %d]\n", FILTER_ID, a->dim[0].gnn, a->dim[1].gnn) ;
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
// dpfl == NULL : reverse filter call (no list needed)
// for the reverse filter, the bit stream provides the necessary information
// the filter may modify the contents of the array described by a
// in filter mode, bp == NULL if no properties information is available
// the filter list MUST BE NULL TERMINATED
ssize_t FILTER_NAME(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream){
  char *errmsg = "" ;
  uint32_t self = FILTER_ID ;
  errmsg = "no array" ;
  if(a == NULL) goto fail ;                      // no array
  errmsg = "no stream" ;
  if(stream == NULL) goto fail ;                 // no stream
  void *array = array_address(a) ;               // get array address, dimension(s), and type
  int rank = a->rank, type = a->type, ni, nj, levels ;
  ssize_t status = 0 ;
  bitstream s = *stream ;                        // local copy of stream control structure

  if(dpfl == NULL) goto reverse ;                // call to reverse filter

  errmsg = "invalid/inconsistent filter ID" ;
  if(! dmap_filter_valid(dpfl,self)) goto fail ;   // not the right filter or NULL pointer

  errmsg = "expecting int_data or uint_data" ;
  if(type != int_data && type != uint_data) goto fail ;    // integers only
  errmsg = "expecting 2D array" ;
  if(rank != 2) goto fail ;                                // 2 D only

  FILTER_ARGS *arg = (FILTER_ARGS *)(*dpfl) ;    // get parameters for this filter
  levels = arg->levels ;
  errmsg = "0 <= levels <= 4 NOT TRUE" ;
  if(levels < 0 || levels > 4) goto fail ;
  ni = a->dim[0].gnn ;
  nj = a->dim[1].gnn ;
  // call forward integer wavelet transform
//   fprintf(stderr, "filter %3.3o %d levels %d x %d forward wavelet transform\n", FILTER_ID, levels, ni, nj) ;
  if(levels > 0) fwd_2d_lgt53_n(array, ni, ni, nj, levels);
  a->type = int_data ;
  bp = NULL ;                                    // not used
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
  STREAM_PUT_NBITS(s, levels, 8) ;
  status += 8 ;

end:
  *stream = s ;   // SAVE stream changes
  return status ;

fail:
  fprintf(stderr, "%s filter %3.3o ERROR : %s\n", (dpfl == NULL) ? "reverse" : "forward", FILTER_ID, errmsg) ;
  return -1 ;     // DO NOT SAVE stream changes

  uint32_t filter ;
reverse:
  STREAM_GET_NBITS(s, filter, 8) ;
  status = 8 ;
//   fprintf(stderr, "reverse filter %3.3o, id = %d\n", FILTER_ID, filter) ;
  if(filter != FILTER_ID) goto fail ;                     // wrong id, MUST be FILTER_ID

  STREAM_GET_NBITS(s, levels, 8) ;
  status += 8 ;
  errmsg = "expecting int_data" ;
  if(type != int_data) goto fail ;
  errmsg = "expecting 2D array" ;
  if(rank != 2)        goto fail ;
  ni = a->dim[0].gnn ;
  nj = a->dim[1].gnn ;

//   fprintf(stderr, "filter %3.3o %d levels %d x %d inverse wavelet transform\n", FILTER_ID, levels, ni, nj) ;
  if(levels > 0) inv_2d_lgt53_n(array, ni, ni, nj, levels);
  a->type = int_data ;

  ssize_t status2 = dmap_filter_inv(a, &s) ;     // call next inverse filter
  if(status2 < 0) goto fail ; else status += status2 ;
  goto end ;
//   *stream = s ;   // SAVE stream changes
//   return status ;
}
#undef FILTER_NAME
#undef FILTER_ARGS
#undef FILTER_ID

// ======================================= filter 006 =======================================
// dpfl == NULL : reverse filter call (no list needed)
// for the reverse filter, the bit stream provides the necessary information
// bit stream encoder
#define FILTER_ID 006
#define FILTER_NAME CAT(dmap_filter_,FILTER_ID)
#define FILTER_ARGS CAT(dmap_filter_arg_,FILTER_ID)
// dpfl == NULL : reverse filter call (no list needed, required data will be in bit stream)
// the filter may modify the contents of the array described by argument a
// in forward filter mode, bp == NULL if no properties information is available
// bp is irrelevant in reverse filter mode
// the filter list MUST BE NULL TERMINATED
//
// this filter MUST BE THE LAST active filter in the chain as it encodes its data
ssize_t FILTER_NAME(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream){
  char *errmsg = "" ;
  uint32_t self = FILTER_ID, zigzag  ;
  ssize_t status = 0, status_0 ;
  int32_t ni, nj, mode, nbits, tnbits, tile, bhw, nelem, i ;

  errmsg = "no array" ;
  if(a == NULL) goto fail ;                      // no array
  void *array = array_address(a) ;               // get array address, dimension(s), and type
  uint32_t rank = a->rank, type = a->type ;

  errmsg = "no stream" ;
  if(stream == NULL) goto fail ;                 // no stream
  bitstream s = *stream ;                        // local copy of stream control structure
//   fprintf(stderr, "filter 006(I) : available space = %ld bits, available bits = %ld bits\n", StreamAvailableSpace(&s), StreamAvailableBits(stream)) ;

  if(dpfl == NULL) goto reverse ;                // reverse filter call
// ================================ forward filter ================================
  errmsg = "invalid filter" ;
  if(! dmap_filter_valid(dpfl,self)) goto fail ; // not the right filter or NULL pointer
  FILTER_ARGS *arg = (FILTER_ARGS *)(*dpfl) ;    // get parameters for this filter

  if(! dmap_filter_is_last(dpfl)){
    errmsg = "filter 006 MUST BE THE LAST FILTER" ;
    goto fail ;
  }

  errmsg = "data must be integer (signed or unsigned)" ;
  if(type != int_data && type != uint_data) goto fail ;

  // mode ==  -1       : raw encoding using size from array descriptor
  // 0   <= mode <  65 : raw encoding using mode bits ( 0 - 64 )
  // mode == 98        : zigzag encoding, nbits auto adjusted
  // mode == 99        : BHW encoding (2 bit length code, followed by 8/16/24/32 bits of data), nbits irrelevant
  // mode >=  100      : tile encoding with tile size mode - 100 (nbits/zigzag computed independently for each tile)
  // nbits == 0        : auto adjust number of bits and zigzag flag according to data values
  // nbits > 0         : force value of nbits (at own risk)
  // modes 0 and 100 both set nbits to 0 (automatically compute necessary nbits)
  mode = arg->mode, zigzag = 0, nbits = 32, tile = 0, bhw = 0 ;
  if     (mode ==  -1) nbits = a->esize * 8 ;                              // nbits from data element size
  else if(mode >= 100) { tile  = mode - 100 ; if(tile < 8) tile = 8 ; }    // tile mode, nbits/zigzag/bhw are irrelevant
  else if(mode ==  99) { bhw = 1 ; }                                       // BHW mode, nbits/zigzag are irrelevant
  else if(mode ==  98) { zigzag = 1 ; }                                    // zigzag mode forces to compute nbits
  else if(mode >=   0) { nbits = mode ; }

  errmsg="nbits is too large" ; if(nbits > 32) goto fail ;                 // not supported yet
//   fprintf(stderr, "filter 006(E) : available space = %ld bits, mode = %d, nbits = %d, bhw = %d", StreamAvailableSpace(&s), mode, nbits, bhw) ;
//   fprintf(stderr, ", saving %d array elements\n", array_dimension(a)) ;
//
  dmap_filter_ptr next_filter = dmap_filter_next(++dpfl) ;       // call next filter (filter chain terminator for thi particular filter)
  status_0 = (*next_filter)(a, bp, dpfl, &s) ;                   // should not fail
  errmsg="tail filter failure" ;  if(status_0 < 0) goto fail ;
//   fprintf(stderr, "filter 006(M) : status = %ld, available space in stream = %ld bits", status, StreamAvailableSpace(&s)) ;
//   fprintf(stderr, ", available bits = %ld bits", StreamAvailableBits(&s)) ;
//   fprintf(stderr, ", insert reverse FILTER_ID = %3.3o\n", FILTER_ID) ;
//
// insert the appropriate data for the reverse filter into bitstream
//
  uint32_t header, *z = (uint32_t *) array ;
  ssize_t available = StreamAvailableSpace(&s) ;

  STREAM_PUT_NBITS(s, FILTER_ID, 8) ;          // reverse filter ID (same as self)
  status = 8 ;

  errmsg="encoder only supports 1D or 2D arrays" ;
  if(rank > 2) goto fail ;
  STREAM_PUT_NBITS(s, rank, 3) ; status += 3 ;

  ni = a->dim[0].gnn, nj = 1 ;                 // 1D setup
  // store dimensions into stream
  STREAM_PUT_BHW(s, ni, tnbits) ; status += tnbits ;
  if(rank == 2){ 
    nj = a->dim[1].gnn ;
    STREAM_PUT_BHW(s, nj, tnbits) ; status += tnbits ;
  }
  nelem = ni * nj ;

  if(mode >= 100){                             // tile encoding
    int32_t *block = (int32_t *) array ;
    ssize_t encoded, needed ;
    errmsg="tile mode needs 2D array" ;
    if(rank != 2) goto fail ;
    header = 0b00110011 ;
    errmsg="tile mode, not enough space to encode data" ;
    // dry-run of encoder to check that there is enough space to encode data
    needed = encode_block(&s, block, ni, ni, nj, tile, arg->options | ENCODE_DRY_RUN) ;
    if(available < needed) goto fail ;         // not enough space
    STREAM_PUT_NBITS(s, header   , 8) ;        // (indicator for tile encoding mode)
    status += 8 ;
    STREAM_PUT_BHW(s, tile, tnbits) ;          // tile size
    status += tnbits ;
    encoded = encode_block(&s, block, ni, ni, nj, tile, arg->options) ;
    status += encoded ;
// fprintf(stderr, "filter 006(X), tile size = %d, tnbits = %d", tile, tnbits) ;
// fprintf(stderr, ", needed = %ld, encoded = %ld\n", needed, encoded) ;
    STREAM_PUT_NBITS(s, header   , 8) ;        // NULL tile (flag for last tile )
    status += 8 ;

  }else if(bhw == 1){                        // "BHW" encoding
    errmsg="BHW mode, not enough space to encode data" ;
    if(available < 24 + nelem*34) goto fail ;  // not enough space for worst case
    header = 0b00110000 ;                      // constant nbits/zigzag/bhw
    STREAM_PUT_NBITS(s, header   , 8) ;        // 8 bit header
    STREAM_PUT_NBITS(s, 31       , 6) ;        // nbits is set to 32 for BHW encoding
    STREAM_PUT_NBITS(s, 2        , 2) ;        // BHW flag
    status += 16 ;
    for(i=0 ; i<nelem ; i++) { STREAM_PUT_BHW(s, z[i], tnbits) ; status += tnbits ; } ;
// fprintf(stderr, "filter 006(X), after BHW encoding %ld bits\n", status) ;

  }else{                                       // constant nbits, 1 block, no tiling, maybe zigzag
    if(nbits == 0){                            // find number of bits to use and value of zigzag flag
      block_properties bp0 ;
      if(nelem != analyze_data32_block((void *)array, nelem, nelem, 1, &bp0)) goto fail ;
      if(bp0.mins.i < 0 && type == int_data){
        zigzag = 1 ;             // negative values are present, will use zigzag encoding
      }else{                     // negative values are not present, no need for zigzag
        zigzag = 0 ;             // max value will be used to determine nb of bits needed
        nbits = BitsNeeded_u32(bp0.maxu.u) ;
      }
      fprintf(stderr, "filter 006(XW) : max = %8.8x, nbits = %d, zigzag = %d\n", bp0.maxu.u, nbits, zigzag) ;
    }
    uint32_t umax = 0 ;
    if(zigzag == 1){       // if zigzag encoding is selected, use absolute max value to (re)set nbits
      umax = v_to_zigzag_32_inplace((int32_t *)array, nelem) ;    // transform array into zigzag form
      nbits = BitsNeeded_u32(umax) ;
    }
    nbits = (nbits < 1) ? 1 : nbits ;     // nbits cannot be 0
    errmsg="constant nbits, not enough space to encode data" ;
    if(available < 24 + nelem*nbits) goto fail ;   // not enough space to encode data into stream
    header = 0b00110000 ;                 // constant nbits/zigzag/bhw
    STREAM_PUT_NBITS(s, header   , 8) ;   // 8 bit header
    STREAM_PUT_NBITS(s, nbits-1  , 6) ;   // nbits
    STREAM_PUT_NBITS(s, zigzag   , 2) ;   // zigzag flag ( 0 or 1 )
    status += 16 ;
//     fprintf(stderr, "filter 006(X) : mode = %d, zigzag = %d, umax = %d, nbits = %d, items = %d\n", mode, zigzag, umax, nbits, nelem) ;

    for(i=0 ; i<nelem ; i++) { STREAM_PUT_NBITS(s, z[i], nbits) ; status += nbits ; } ;   // inject data into stream
    STREAM_INSERT_PUSH(s) ;   // push all data into stream
  }
  *stream = s ;   // SAVE stream changes
//   fprintf(stderr, "filter 006(X) : available space in stream %ld bits\n", StreamAvailableSpace(stream)) ;
// fprintf(stderr, "filter 006(X) : inserted %ld bits\n", status);
  return status + status_0 ;    // return number of bits produced

// ================================ miserable failure ================================
// used by forward and reverse filter
fail:
  fprintf(stderr, "%s filter %3.3o ERROR : %s\n", (dpfl == NULL) ? "reverse" : "forward", FILTER_ID, errmsg) ;
  return -1 ;     // DO NOT SAVE stream changes

// ================================ reverse filter ================================
// decode bit stream encoded by forward filter
reverse:
  STREAM_GET_NBITS(s, self, 8) ; status = 8 ;            // filter ID from stream
  if(self != FILTER_ID) goto fail ;                      // wrong id, MUST be FILTER_ID

  STREAM_GET_NBITS(s, rank, 3) ; status += 3 ;           // get rank from stream
  errmsg="decoding rank mismatch" ;
  if(rank != a->rank) goto fail ;
  errmsg="decoder only supports 1D or 2D" ;
  if(rank > 2) goto fail ;
  errmsg = "REVERSE  filter 006 : input array should be empty" ;
  if( ! array_no_data(a) ) goto fail ;                  // array should not contain valid data

  // get dimensions from stream, reshape array descriptor a-> (STREAM_GET_BHW), remember input array dimensions
  int32_t ni_in, nj_in ;                                 // input array dimensions
  STREAM_GET_BHW(s, ni, tnbits) ;  status += tnbits ;
  ni_in = a->dim[0].gnn ;                                // remember first dimension
  if(rank == 1){
    nj = nj_in = 1 ;
    reshape_array((array_1d *)a, a->esize, a->type, ni) ;
  }else{
    STREAM_GET_BHW(s, nj, tnbits) ; status += tnbits ;
    reshape_array((array_2d *)a, a->esize, a->type, ni, nj) ;
    nj_in = a->dim[1].gnn ;                              // remember second dimension
  }
//   fprintf(stderr, "rank = %d, ni = %d, expected %d, nj = %d, expected = %d, status = %ld\n", rank, ni, a->dim[0].gnn, nj, (rank>1) ? a->dim[1].gnn : 1, status) ;

  STREAM_GET_NBITS(s, header, 8) ; status += 8 ;         // 8 bit header
  if(header == 0b00110011){                              // tile encoding
    errmsg="tile decoder needs 2D array" ;
    if(rank != 2) goto fail ;
    int32_t *block = (int32_t *) array ;
    STREAM_GET_BHW(s, tile, tnbits) ;                    // get tile size
    status += tnbits ;
// fprintf(stderr, "REVERSE  filter 006 : tilesize = %d, tnbits = %d\n", tile, tnbits) ;
// fprintf(stderr, "REVERSE  filter 006 : decoding ni = %d, nj = %d", ni, nj);
    ssize_t decoded = decode_block(&s, block, ni, ni, nj, tile) ;
    status += decoded ;
// fprintf(stderr, ", TILE decoded %ld bits\n", decoded) ;
    STREAM_GET_NBITS(s, header , 8) ;                    // check end of tiles flag
    if(header != 0b00110011) goto fail ;
    status += 8 ;

  }else if(header == 0b00110000){                        // nbits per value, RAW/zigzag/BHW
    STREAM_GET_NBITS(s, nbits , 6) ; nbits++ ;           // nbits - 1 was encoded into stream
    STREAM_GET_NBITS(s, zigzag, 2) ;                     // RAW/zigzag/BHW mode
    status += 8 ;
//     fprintf(stderr, "reverse filter %3.3o, id = %d, header = 0x%2.2x, nbits = %d, zigzag = %d\n", FILTER_ID, self, header, nbits, zigzag) ;
    if(nbits > 32)           goto fail ;                 // not supported yet
    nelem = ni * nj ;
    z = (uint32_t *) array ;
//     fprintf(stderr, "reverse filter %3.3o restoring %d array elements\n", FILTER_ID, nelem) ;
    if(zigzag == 2){                                     // BHW mode, nbits MUST be 32 (but will be ignored)
      if(nbits != 32) goto fail ;
      for(i=0 ; i<nelem ; i++) { STREAM_GET_BHW(s, z[i], tnbits) ; status += tnbits ; } ;
// fprintf(stderr, "BHW decoded %ld bits\n", status) ;
    }else{                                               // constant nbits, maybe zigzag
      int32_t max = 0 ;                                  // make sure that max is always initialized
      for(i=0 ; i<nelem ; i++) { STREAM_GET_NBITS(s, z[i], nbits) ; status += nbits ; } ;
      if(zigzag != 0) max = v_from_zigzag_32_inplace((int32_t *)array, nelem) ;
      if(max == 0) fprintf(stderr, "reverse filter %3.3o, max = %d, zigzag = %d\n", FILTER_ID, max, zigzag) ;
    }

  }else{                                                 // invalid header value
    goto fail ;
  }
  a->type = int_data ;                                   // output data type is signed integers
  array_set_used(a) ;
// fprintf(stderr, "REVERSE  filter 006(X) : extracted %ld bits\n", status) ;
  ssize_t status2 = dmap_filter_inv(a, &s) ;             // call next inverse filter
  if(status2 < 0) goto fail ; else status += status2 ;

  if( ! array_has_data(a) ) goto fail ;                  // array should be filled
  ni = a->dim[0].gnn ; nj = 1 ;                          // and final shape should be as expected
  if(a->rank == 2) nj = a->dim[1].gnn ;
  errmsg = "final array dimensions not as expected" ;
  if(ni != ni_in || nj != nj_in || rank != a->rank) goto fail ;
  *stream = s ;      // SAVE stream changes
  return status ;    // return number of bits consumed
}
#undef FILTER_NAME
#undef FILTER_ARGS
#undef FILTER_ID


// ======================================= filter 007 =======================================
// compound filter (quantize/decimate/predict/encode)
#define FILTER_ID 007
#define FILTER_NAME CAT(dmap_filter_,FILTER_ID)
#define FILTER_ARGS CAT(dmap_filter_arg_,FILTER_ID)
ssize_t FILTER_NAME(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream){
  uint32_t self = FILTER_ID ;
  if(a == NULL || stream == NULL) goto fail ;    // no array or no stream
  void *array = array_address(a) ;               // get array address, dimension(s), and type
  int rank = a->rank, type = a->type ;
  ssize_t status = 0 ;
  bitstream s = *stream ;                        // local copy of stream control structure
//   block_properties lbp ;
// 
//   if(bp == NULL) { bp = &lbp ; lbp.kind = bad_data ; }
  if(dpfl == NULL) goto reverse ;                // call to reverse filter
  if(! dmap_filter_valid(dpfl,self)) goto fail ;   // not the right filter or NULL pointer
//   FILTER_ARGS *arg = (FILTER_ARGS *)(*dpfl) ;    // get parameters for this filter

// check a->type and a->rank
  if(type != float_data || rank != 2) goto fail ;
// filter processing code goes here
//   int i, j, lni = a->dim[0].gnn, lnj = a->dim[1].gnn ;
//   float *f = (float *)array ;
//   fprintf(stderr, "filter 007, offset =%f, scale = %f, array[%d:%d]", arg->offset, arg->scale, lni, lnj) ;
//   fprintf(stderr, ", LL = %f, UR = %f, ", f[0] , f[lni*lnj-1]) ;
//   for(j=0 ; j<lnj ; j++){
//     for(i=0 ; i<lni ; i++){
//       f[i + j*lni] = (f[i + j*lni] * arg->scale) + arg->offset ;
//     }
//   }
//   fprintf(stderr, ", LL = %f, UR = %f\n", f[0] , f[lni*lnj-1]) ;
//   fprintf(stderr, "filter 007(E) : available space in stream %ld bits\n", StreamAvailableSpace(&s)) ;

  dpfl++ ;                              // call next filter if there is one
  dmap_filter_ptr next_filter = dmap_filter_next(dpfl) ;
  status = (*next_filter)(a, bp, dpfl, &s) ;
  if(status < 0) goto fail ;
  fprintf(stderr, "filter 007(M) : status = %ld, available space in stream %ld bits\n", status, StreamAvailableSpace(stream)) ;

//   STREAM_PUT_NBITS(s, FILTER_ID, 8) ;
//   uint32_t *tmp1 = (uint32_t *) &(arg->offset) ;
//   STREAM_PUT_NBITS(s, *tmp1, 32) ;
//   uint32_t *tmp2 = (uint32_t *) &(arg->scale) ;
//   STREAM_PUT_NBITS(s, *tmp2, 32) ;
//   STREAM_INSERT_PUSH(s) ;
//   fprintf(stderr, "filter 007(W) %3.3o, %8.8x , %8.8x\n", FILTER_ID, *tmp1, *tmp2) ;
//   status += 72 ;
  status += 8 ;

// end:
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
//   if(filter != FILTER_ID) goto fail ;                     // wrong id, MUST be FILTER_ID
//   uint32_t t1, t2 ;
//   STREAM_GET_NBITS(s, t1, 32) ;
//   STREAM_GET_NBITS(s, t2, 32) ;
//   fprintf(stderr, "filter 007(R) %3.3o, %8.8x , %8.8x\n", FILTER_ID, t1, t2) ;
//   status = 72 ;
  status = 8 ;

  ssize_t status2 = dmap_filter_inv(a, &s) ;     // call next inverse filter
  if(status2 < 0) goto fail ; else status += status2 ;
  *stream = s ;
  return status ;
}
#undef FILTER_NAME
#undef FILTER_ARGS
#undef FILTER_ID
