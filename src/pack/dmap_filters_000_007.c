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

// ======================================= filter 000 =======================================
// head of forward filter chain, calls dmap_filter_000 with command = DMAP_FILTER
// the filter template is not used for this special filter
ssize_t dmap_filter_fwd(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream){
  return dmap_filter_000(a, bp, dpfl, stream, DMAP_FILTER) ;
}
#define FILTER_ID 000
#define FILTER_NAME CAT(dmap_filter_,FILTER_ID)
#define FILTER_ARGS CAT(dmap_filter_arg_,FILTER_ID)
// special filter used to get/put array dimensions and type information (found in dmap_filters.c)
// this filter writes into bit stream BEFORE calling the filter chain and AFTER calling said chain
// this filter expects NO ARGUMENT from the filter list
// this filter MUST be called first
// in restore mode, it makes sure that the target array has the correct configuration
// for data type and dimensions
// TODO: allocate memory for the target array if necessary
// this filter will be the first to be called in restore mode (get/check rank and dimensions)
ssize_t FILTER_NAME(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream, dmap_command command){
  uint32_t self = FILTER_ID ;
  if(a == NULL || stream == NULL) goto fail ;    // no array or no stream
  void *array = array_address(a) ;               // get array address, dimension(s), and type
  ssize_t status = 0 ;
  bitstream s = *stream ;                        // local copy of stream control structure
  int32_t nbits ;

  if(command == DMAP_ENCODE)  return 0 ;         // irrelevant
  if(command == DMAP_DECODE)  return 0 ;         // irrelevant
  if(command == DMAP_RESTORE) goto restore ;
  if(command == DMAP_PRINT)   return 0 ;         // irrelevant

// ========== forward filter ==========
  STREAM_PUT_NBITS(s, self, 8) ; nbits = 8 ;     // insert own filter id into stream
  // insert array description information into the bit stream
  nbits += dmap_filter_put_array_info(a, &s) ;

  // call next filter in list
  // DO NOT USE dpfl++, dmap_filter_fwd is a filter implicitely at the head of the list
  dmap_filter_ptr next_filter = dmap_filter_next(dpfl) ;
  status = (*next_filter)(a, bp, dpfl, &s, command) ;
  if(status < 0) goto fail ;

  // insert the FILTER_CHAIN_END marker into the bit stream
  STREAM_PUT_NBITS(s, FILTER_CHAIN_END, 8) ;
  nbits += 8 ;
  STREAM_INSERT_ALIGN32(s) ;        // align stream to a 32 bit boundary
  STREAM_FLUSH(s) ;                 // flush stream data
  status += nbits ;

// successful end
end :
  *stream = s ;   // SAVE stream changes
  return status ;

// miserable failure
fail:
  return -1 ;     // DO NOT SAVE stream changes

restore:
  STREAM_GET_NBITS(s, self, 8) ;
  status = 8 ;
  if(self != FILTER_ID) goto fail ;                     // wrong id, MUST be FILTER_ID (0)

  // get array description from stream
  int32_t temp = dmap_filter_get_array_info(a, &s, 1) ;
  if(temp < 0) goto fail ;
  status += temp ;
  array_set_empty(a) ;                                  // mark array as having no valid data

  ssize_t status2 = dmap_filter_inv(a, &s) ;            // call first inverse filter
  if(status2 < 0) goto fail ; else status += status2 ;
  goto end ;
}
#undef FILTER_ID

// ======================================= filter 001 =======================================
// integer/float offset and scale (NO OP for now)
#define FILTER_ID 001
#define FILTER_NAME CAT(dmap_filter_,FILTER_ID)
#define FILTER_ARGS CAT(dmap_filter_arg_,FILTER_ID)
ssize_t FILTER_NAME(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream, dmap_command command){
  ssize_t status = 0, status2 ;
  uint32_t self, rank, type ;
  char *errmsg = "" ;
  FILTER_ARGS *arg ;
  void *array ;
  bitstream s ;

// ==================== local variable declarations ====================
  int i, lni, lnj ;
  float *f ;
// ========================================================================
  if(command != DMAP_RESTORE){                       // DMAP_RESTORE does not use a parameter list
    errmsg = "dmap filter list is NULL" ;
    if(dpfl == NULL) goto fail ;
    arg = (FILTER_ARGS *) dpfl[0] ;                  // get parameters for this filter
    errmsg = "invalid/inconsistent filter ID" ;
    if(! dmap_filter_valid(dpfl,FILTER_ID)) goto fail ;   // not the expected filter ID
  }
  if(command != DMAP_PRINT){                         // DMAP_PRINT does not use the bit stream
    errmsg = "no stream" ;
    if(stream == NULL) goto fail ;                   // no bit stream
    s = *stream ;                                    // local copy of stream control structure
  }

  if(command == DMAP_ENCODE) goto encode ;
  if(command == DMAP_DECODE) goto decode ;
  if(command == DMAP_PRINT)  goto print ;
  if(command == DMAP_RESTORE || command == DMAP_FILTER){
    errmsg = "no array" ;
    if(a == NULL) goto fail ;
    array = array_address(a) ;                     // get array address, dimension(s), and type
    if(array == NULL) goto fail ;
    rank = a->rank ;
    type = a->type ;
    // check type and rank as/if needed
    errmsg = "wrong type (not int/float) or rank(not 2)" ;
    if((type != float_data && type != int_data) || rank != 2) goto fail ;
    errmsg = "int type not supported yet" ;
    if(type == int_data) goto fail ;
    if(command == DMAP_RESTORE) goto restore ;
    goto forward ;
  }else{      // not DMAP_ENCODE, DMAP_DECODE, DMAP_PRINT, DMAP_RESTORE, DMAP_FILTER
    errmsg = "invalid command" ;
    goto fail ;
  }
// forward filter
forward :
// ====================  filter processing code  (FWD) ====================
// local code transforming array before calling next filter
  lni = a->dim[0].gnn ; lnj = a->dim[1].gnn ; f = (float *)array ;
  for(i = 0 ; i < lni * lnj ; i++){
    f[i] = (f[i] * arg->scale) + arg->offset ;
  }
// ========================================================================
  dpfl++ ;                              // call next filter
  dmap_filter_ptr next_filter = dmap_filter_next(dpfl) ;
  status = (*next_filter)(a, bp, dpfl, &s, command) ;
  errmsg = "filter chain failed" ;
  if(status < 0) goto fail ;

  STREAM_PUT_NBITS(s, FILTER_ID, 8) ; status += 8 ;
// ====================  filter processing code  (FWD) ====================
// local code inserting proper data into bit stream
  uint32_t *tmp1 = (uint32_t *) &(arg->offset) ;
  STREAM_PUT_NBITS(s, *tmp1, 32) ; status += 32 ;
  uint32_t *tmp2 = (uint32_t *) &(arg->scale) ;
  STREAM_PUT_NBITS(s, *tmp2, 32) ; status += 32 ;
  STREAM_INSERT_PUSH(s) ;
// ========================================================================

// successful end
end:
  *stream = s ;   // success, SAVE stream changes
  return status ;

// miserable failure
fail:
  fprintf(stderr, "filter %3.3o ERROR : %s\n", FILTER_ID, errmsg) ;
  return -1 ;     // failure, DO NOT SAVE stream changes

// inverse of forward filter
restore:
// get the appropriate information for the restore filter from bitstream
  STREAM_GET_NBITS(s, self, 8) ; status = 8 ;          // 8 bits extracted so far
  errmsg = "inconsistent filter ID" ;
  if(self != FILTER_ID) goto fail ;                    // wrong id, MUST be FILTER_ID

// ====================  restore (INV) ====================
// local code to restore from bit stream
  uint32_t t1, t2 ;
  STREAM_GET_NBITS(s, t1, 32) ; status += 32 ;  // offset
  STREAM_GET_NBITS(s, t2, 32) ; status += 32 ;  // scale
  float *ft1 = (float *)&t1, *ft2 = (float *)&t2 ;
  fprintf(stderr, "restore filter %3.3o, id = %d, args = %10E, %10E\n", FILTER_ID, self, *ft1, *ft2) ;
// inverse array processing code
  lni = a->dim[0].gnn ; lnj = a->dim[1].gnn ; f = (float *)array ;
  for(i = 0 ; i < lni * lnj ; i++){
    f[i] = (f[i] - ft1[0]) / ft2[0]  ;
  }
// ========================================================================

  status2 = dmap_filter_inv(a, &s) ;           // call next inverse filter
  errmsg = "restore filter chain failed" ;
  if(status2 < 0) goto fail ;
  status += status2 ;
  goto end ;

// encode filter parameters from *dpfl[0] into bit stream
encode:
  status = 0 ;
  s = *stream ;                        // local copy of stream control structure
  fprintf(stderr, "encode parameters : filter = %3.3o", arg->filter) ;
  arg = (FILTER_ARGS *) dpfl[0] ;    // parameters for this filter
  STREAM_PUT_NBITS(s, arg->filter , 8) ; status = 8 ;
// ========================================================================
  STREAM_PUT_NBITS(s, arg->iscale , 32) ; status += 32 ;
  STREAM_PUT_NBITS(s, arg->ioffset, 32) ; status += 32 ;
  fprintf(stderr, "(%3.3o), scale = %8.8x, offset = %8.8x, status = %ld\n", self, arg->iscale, arg->ioffset, status) ;
// ========================================================================
  goto end ;

// decode filter parameters from bit stream, copy into *dpfl[0]
decode:
  status = 0 ;
  s = *stream ;                        // local copy of stream control structure
  fprintf(stderr, "decode parameters, filter = %3.3o", arg->filter) ;
  STREAM_GET_NBITS(s, self, 8) ;
  errmsg = "decode parameters : self != FILTER_ID" ;
  if(self != FILTER_ID) goto fail ;                     // wrong id, MUST be FILTER_ID
  status = sizeof(FILTER_ARGS) ;
  arg = (FILTER_ARGS *) dpfl[0] ;                       // parameters for this filter
  arg->filter  = self ;
  status = sizeof(FILTER_ARGS) ;
// ========================================================================
  int32_t w32 ;
  STREAM_GET_NBITS(s, w32 , 32) ;                       // get scale
  arg->iscale  = w32 ;
  STREAM_GET_NBITS(s, w32, 32) ;                        // get offset
  arg->ioffset = w32 ;
  fprintf(stderr, "(%3.3o), scale = %8.8x, offset = %8.8x, status = %ld\n", self, arg->iscale, arg->ioffset, status) ;
// ========================================================================
  goto end ;

// print filter parameters
print:
  arg = (FILTER_ARGS *) dpfl[0] ;                       // parameters for this filter
// ========================================================================
  fprintf(stderr, "[%3.3o] SAXPY, scale = %8.8x, offset = %8.8x\n", arg->filter, arg->iscale, arg->ioffset) ;
// ========================================================================
  return 0 ;
}
#undef FILTER_NAME
#undef FILTER_ARGS
#undef FILTER_ID

// ======================================= filter 002 =======================================
// #include <rmn/quantizers.h>

// (NO OP with a flag for now)
#define FILTER_ID 002
#define FILTER_NAME CAT(dmap_filter_,FILTER_ID)
#define FILTER_ARGS CAT(dmap_filter_arg_,FILTER_ID)
ssize_t FILTER_NAME(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream, dmap_command command){
  ssize_t status = 0, status2 ;
  uint32_t self, rank, type ;
  char *errmsg = "" ;
  FILTER_ARGS *arg ;
  void *array ;
  bitstream s ;

// ==================== local variable declarations ====================
// local code
// ========================================================================
  if(command != DMAP_RESTORE){                       // DMAP_RESTORE does not use a parameter list
    errmsg = "dmap filter list is NULL" ;
    if(dpfl == NULL) goto fail ;
    arg = (FILTER_ARGS *) dpfl[0] ;                  // get parameters for this filter
    errmsg = "invalid/inconsistent filter ID" ;
    if(! dmap_filter_valid(dpfl,FILTER_ID)) goto fail ;   // not the expected filter ID
  }
  if(command != DMAP_PRINT){                         // DMAP_PRINT does not use the bit stream
    errmsg = "no stream" ;
    if(stream == NULL) goto fail ;                   // no bit stream
    s = *stream ;                                    // local copy of stream control structure
  }

  if(command == DMAP_ENCODE) goto encode ;
  if(command == DMAP_DECODE) goto decode ;
  if(command == DMAP_PRINT)  goto print ;
  if(command == DMAP_RESTORE || command == DMAP_FILTER){
    errmsg = "no array" ;
    if(a == NULL) goto fail ;
    array = array_address(a) ;                     // get array address, dimension(s), and type
    if(array == NULL) goto fail ;
    rank = a->rank ;
    type = a->type ;
    // check type and rank as/if needed
    // local code
    if(command == DMAP_RESTORE) goto restore ;
    goto forward ;
  }else{      // not DMAP_ENCODE, DMAP_DECODE, DMAP_PRINT, DMAP_RESTORE, DMAP_FILTER
    errmsg = "invalid command" ;
    goto fail ;
  }
// forward filter
forward :
// ====================  filter processing code  (FWD) ====================
// local code transforming array before calling next filter
// ========================================================================
  dpfl++ ;                              // call next filter
  dmap_filter_ptr next_filter = dmap_filter_next(dpfl) ;
  status = (*next_filter)(a, bp, dpfl, &s, command) ;
  errmsg = "filter chain failed" ;
  if(status < 0) goto fail ;

  STREAM_PUT_NBITS(s, FILTER_ID, 8) ; status += 8 ;
// ====================  filter processing code  (FWD) ====================
// local code inserting proper data into bit stream
  uint32_t *tmp1 = (uint32_t *) &(arg->flag) ;
  STREAM_PUT_NBITS(s, *tmp1, 8) ; status += 8 ;
  STREAM_INSERT_PUSH(s) ;
// ========================================================================

// successful end
end:
  *stream = s ;   // success, SAVE stream changes
  return status ;

// miserable failure
fail:
  fprintf(stderr, "filter %3.3o ERROR : %s\n", FILTER_ID, errmsg) ;
  return -1 ;     // failure, DO NOT SAVE stream changes

// inverse of forward filter
restore:
// get the appropriate information for the restore filter from bitstream
  STREAM_GET_NBITS(s, self, 8) ; status = 8 ;          // 8 bits extracted so far
  errmsg = "inconsistent filter ID" ;
  if(self != FILTER_ID) goto fail ;                    // wrong id, MUST be FILTER_ID

// ====================  restore (INV) ====================
// local code to restore from bit stream
  uint32_t t ;
  STREAM_GET_NBITS(s, t, 8) ; status += 8 ;
  fprintf(stderr, "restore filter %3.3o, id = %d, t = %d\n", FILTER_ID, self, t) ;
// inverse array processing code
// ========================================================================

  status2 = dmap_filter_inv(a, &s) ;           // call next inverse filter
  errmsg = "restore filter chain failed" ;
  if(status2 < 0) goto fail ;
  status += status2 ;
  goto end ;

// encode filter parameters from *dpfl[0] into bit stream
encode:
  status = 0 ;
  s = *stream ;                        // local copy of stream control structure
  fprintf(stderr, "encode parameters : filter = %3.3o", arg->filter) ;
  arg = (FILTER_ARGS *) dpfl[0] ;    // parameters for this filter
  STREAM_PUT_NBITS(s, arg->filter , 8) ; status = 8 ;
// ========================================================================
  fprintf(stderr, "(%3.3o), status = %ld\n", arg->filter, status) ;
  STREAM_PUT_NBITS(s, arg->flag, 16) ; status += 16 ;
  fprintf(stderr, ", flag = %4.4x, status = %ld\n", arg->flag, status) ;
// ========================================================================
  goto end ;

// decode filter parameters from bit stream, copy into *dpfl[0]
decode:
  status = 0 ;
  s = *stream ;                        // local copy of stream control structure
  fprintf(stderr, "decode parameters, filter = %3.3o", arg->filter) ;
  STREAM_GET_NBITS(s, self, 8) ;
  errmsg = "decode parameters : self != FILTER_ID" ;
  if(self != FILTER_ID) goto fail ;                     // wrong id, MUST be FILTER_ID
  status = sizeof(FILTER_ARGS) ;
  arg = (FILTER_ARGS *) dpfl[0] ;                       // parameters for this filter
  arg->filter  = self ;
  status = sizeof(FILTER_ARGS) ;
// ========================================================================
  uint32_t w32 ;
  STREAM_GET_NBITS(s, w32, 16) ;
  arg->flag = w32 ;
  fprintf(stderr, "(%3.3o), flag = %4.4x, status = %ld\n", arg->filter, arg->flag, status) ;
// ========================================================================
  goto end ;

// print filter parameters
print:
  arg = (FILTER_ARGS *) dpfl[0] ;                       // parameters for this filter
// ========================================================================
  fprintf(stderr, "[%3.3o] NO-OP filter, flag = %4.4x\n", arg->filter, arg->flag) ;
// ========================================================================
  return 0 ;
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
// dpfl == NULL : restore filter call (no list needed)
// for the restore filter, the bit stream provides the necessary information
// the filter may modify the contents of the array described by a and of the data properties bp
// in filter mode, bp == NULL if no properties information is available
// the filter list MUST BE NULL TERMINATED
#if 1
ssize_t FILTER_NAME(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream, dmap_command command){
  ssize_t status = 0, status2 ;
  uint32_t self, rank, type ;
  char *errmsg = "" ;
  FILTER_ARGS *arg ;
  void *array ;
  bitstream s ;

// ==================== local variable declarations ====================
  union{ float f32 ; int32_t i32 ; uint32_t u32 ; } x32 ;
  int32_t mode, nvalues, nbits, e_base, offset ;
  float maxerr, minabs ;
// ========================================================================
  if(command != DMAP_RESTORE){                       // DMAP_RESTORE does not use a parameter list
    errmsg = "dmap filter list is NULL" ;
    if(dpfl == NULL) goto fail ;
    arg = (FILTER_ARGS *) dpfl[0] ;                  // get parameters for this filter
    errmsg = "invalid/inconsistent filter ID" ;
    if(! dmap_filter_valid(dpfl,FILTER_ID)) goto fail ;   // not the expected filter ID
  }
  if(command != DMAP_PRINT){                         // DMAP_PRINT does not use the bit stream
    errmsg = "no stream" ;
    if(stream == NULL) goto fail ;                   // no bit stream
    s = *stream ;                                    // local copy of stream control structure
  }

  if(command == DMAP_ENCODE) goto encode ;
  if(command == DMAP_DECODE) goto decode ;
  if(command == DMAP_PRINT)  goto print ;
  if(command == DMAP_RESTORE || command == DMAP_FILTER){
    errmsg = "no array" ;
    if(a == NULL) goto fail ;
    array = array_address(a) ;                     // get array address, dimension(s), and type
    if(array == NULL) goto fail ;
    rank = a->rank ;
    type = a->type ;
    // check type and rank as/if needed
    errmsg = "rank != 2" ;
    if(rank != 2) goto fail ;                      // only 2D is supported at this time
    if(command == DMAP_RESTORE) goto restore ;
    goto forward ;
  }else{      // not DMAP_ENCODE, DMAP_DECODE, DMAP_PRINT, DMAP_RESTORE, DMAP_FILTER
    errmsg = "invalid command" ;
    goto fail ;
  }
// forward filter
forward :
// ====================  filter processing code  (FWD) ====================
  errmsg = "type != float_data" ;
  if(type != float_data) goto fail ;             // data type MUST BE FLOAT
  mode = arg->mode ;                     // get mode
  nbits = arg->nbits ;                   // max number of bits to be used for quantization
  offset = 0 ;
  e_base = 0 ;
  nvalues = a->dim[0].gnn * a->dim[1].gnn ;      // number of values in array
  errmsg = "nbits < 0" ;
  if(nbits    < 0) goto fail ;                   // nbits MUST BE >= 0
  maxerr = arg->maxerr ;                   // largest absolute/relative error desired
  errmsg = "maxerr < 0" ;
  if(maxerr < 0) goto fail ;                     // maxerr MUST BE >= 0
  errmsg = "maxerr and nbits both 0" ;
  if(nbits == 0 && maxerr == 0) goto fail ;      // cannot be BOTH 0

  minabs = arg->minabs ;

  if(mode == FP_2_INT){       // linear quantizer
    offset = arg->offset ;                       // discretization offset
// fprintf(stderr,"nvalues = %d, maxerr = %f, nbits = %d, offset = %8.8x, bp = %p\n", nvalues, maxerr, nbits, offset, bp) ;
    e_base = fp_to_qlin((float *)array, (int32_t *)array, nvalues, maxerr, nbits, &offset, bp) ;
    if(e_base <= 0) goto fail ;
    a->type = (offset == 0x7FFFFFFF) ? uint_data : int_data ;
  }else if(mode == FP_2_FAKELOG){
    errmsg = "minabs < 0" ;
    if(minabs < 0) goto fail ;
    errmsg = "mode == FP_2_FAKELOG, not supported yet" ;
    goto fail ;        // fake log quantizer not supported yet
  }else{
    errmsg = "invalid mode" ;
    goto fail ;        // invalid mode
  }
// ========================================================================
  dpfl++ ;                              // call next filter
  dmap_filter_ptr next_filter = dmap_filter_next(dpfl) ;
  status = (*next_filter)(a, bp, dpfl, &s, command) ;
  errmsg = "filter chain failed" ;
  if(status < 0) goto fail ;

  STREAM_PUT_NBITS(s, FILTER_ID, 8) ; status += 8 ;
// ====================  filter processing code  (FWD) ====================
// local code inserting proper data into bit stream
  int inserted ;

  STREAM_PUT_NBITS(s, (mode & 0x3), 2) ; inserted = 2 ;  // 2 bits inserted so far
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
  fprintf(stderr, "filter %3.3o : inserted %d bits, quantum = %f, exp = %d, offset = %d, mode = %d, nvalues = %d\n",
          FILTER_ID, inserted+8, fp32_pow2(e_base-127), e_base, offset, mode, nvalues) ;
// ========================================================================

// successful end
end:
  *stream = s ;   // success, SAVE stream changes
  return status ;

// miserable failure
fail:
  fprintf(stderr, "filter %3.3o ERROR : %s\n", FILTER_ID, errmsg) ;
  return -1 ;     // failure, DO NOT SAVE stream changes

// inverse of forward filter
restore:
// get the appropriate information for the restore filter from bitstream
  STREAM_GET_NBITS(s, self, 8) ; status = 8 ;          // 8 bits extracted so far
  errmsg = "inconsistent filter ID" ;
  if(self != FILTER_ID) goto fail ;                    // wrong id, MUST be FILTER_ID
  errmsg = "restore filter : data type MUST BE integer" ;
  if(type != int_data && type != uint_data) goto fail ;             // data type MUST BE INTEGER
// get the appropriate information for the restore filter from bitstream (GET)
  STREAM_GET_NBITS(s, mode, 2) ; status += 2 ;
  nvalues = a->dim[0].gnn * a->dim[1].gnn ;      // number of values in array
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
  fprintf(stderr, "restore filter %3.3o, array[%d,%d](%d)(%dD), offset = %d, extracted %ld bits\n",
                  FILTER_ID, a->dim[0].gnn, a->dim[1].gnn, nvalues, a->rank, offset, status) ;
  a->type = float_data ;                               // mark data as float data

// ====================  restore (INV) ====================
// local code to restore from bit stream
// inverse array processing code
// ========================================================================

  status2 = dmap_filter_inv(a, &s) ;           // call next inverse filter
  errmsg = "restore filter chain failed" ;
  if(status2 < 0) goto fail ;
  status += status2 ;
  goto end ;

// encode filter parameters from *dpfl[0] into bit stream
encode:
  status = 0 ;
  s = *stream ;                        // local copy of stream control structure
  fprintf(stderr, "encode parameters : filter = %3.3o", arg->filter) ;
  arg = (FILTER_ARGS *) dpfl[0] ;    // parameters for this filter
  STREAM_PUT_NBITS(s, arg->filter , 8) ; status = 8 ;
// ========================================================================
                          STREAM_PUT_NBITS(s, arg->mode  , 12) ; status += 12 ;
                          STREAM_PUT_NBITS(s, arg->nbits , 12) ; status += 12 ;
  x32.f32 = arg->abserr ; STREAM_PUT_NBITS(s, x32.u32, 32)     ; status += 32 ;
                          STREAM_PUT_NBITS(s, arg->offset, 32) ; status += 32 ;
  x32.f32 = arg->minabs ; STREAM_PUT_NBITS(s, x32.u32, 32)     ; status += 32 ;
  x32.f32 = arg->zval   ; STREAM_PUT_NBITS(s, x32.u32, 32)     ; status += 32 ;
  fprintf(stderr, "(%3.3o), mode = %d, nbits = %d, err = %10E, offset = %8.8x, minabs = %10E, zval = %10E, status = %ld\n",
                    arg->filter, arg->mode, arg->nbits, arg->maxerr, arg->offset, arg->minabs, arg->zval,status) ;
// ========================================================================
  goto end ;

// decode filter parameters from bit stream, copy into *dpfl[0]
decode:
  status = 0 ;
  s = *stream ;                        // local copy of stream control structure
  fprintf(stderr, "decode parameters, filter = %3.3o", arg->filter) ;
  STREAM_GET_NBITS(s, self, 8) ;
  errmsg = "decode parameters : self != FILTER_ID" ;
  if(self != FILTER_ID) goto fail ;                     // wrong id, MUST be FILTER_ID
  status = sizeof(FILTER_ARGS) ;
  arg = (FILTER_ARGS *) dpfl[0] ;                       // parameters for this filter
  arg->filter  = self ;
  status = sizeof(FILTER_ARGS) ;
// ========================================================================
  uint32_t u32 ; int32_t i32 ;
  STREAM_GET_NBITS(s, i32, 12)     ; arg->mode   = i32 ;
  STREAM_GET_NBITS(s, u32, 12)     ; arg->nbits  = u32 ;
  STREAM_GET_NBITS(s, x32.u32, 32) ; arg->abserr = x32.f32 ;
  STREAM_GET_NBITS(s, i32, 32)     ; arg->offset = i32 ;
  STREAM_GET_NBITS(s, x32.u32, 32) ; arg->minabs = x32.f32 ;
  STREAM_GET_NBITS(s, x32.u32, 32) ; arg->zval   = x32.f32 ;
  status = sizeof(FILTER_ARGS) ;
  fprintf(stderr, "(%3.3o), mode = %d, nbits = %d, err = %10E, offset = %8.8x, minabs = %10E, zval = %10E, status = %ld\n",
                   self, arg->mode, arg->nbits, arg->maxerr, arg->offset, arg->minabs, arg->zval, status) ;
// ========================================================================
  goto end ;

// print filter parameters
print:
  arg = (FILTER_ARGS *) dpfl[0] ;                       // parameters for this filter
// ========================================================================
  fprintf(stderr, "[%3.3o] Float Quantizer, mode = %d, nbits = %d, err = %10E, offset = %8.8x, minabs = %10E, zval = %10E\n",
                  arg->filter, arg->mode, arg->nbits, arg->maxerr, arg->offset, arg->minabs, arg->zval) ;
// ========================================================================
  return 0 ;
}
#else
ssize_t FILTER_NAME(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream, dmap_command command){
  char *errmsg = "" ;
  uint32_t self = FILTER_ID ;
  union{ float f32 ; int32_t i32 ; uint32_t u32 ; } x32 ;
  FILTER_ARGS *arg ;

  errmsg = "dmap filter list is NULL" ;
  if(command != DMAP_RESTORE && dpfl == NULL) goto fail ;

  if(command == DMAP_ENCODE) goto encode ;
  if(command == DMAP_DECODE) goto decode ;
  if(command == DMAP_PRINT)  goto print ;

  errmsg = "a == NULL || stream == NULL" ;
  if(a == NULL || stream == NULL) goto fail ;    // no array or no stream

  void *array = array_address(a) ;               // get array address, dimension(s), and type
  int32_t nvalues, rank = a->rank, type = a->type, offset = 0,  e_base = 0 ;
  ssize_t status = 0 ;
  bitstream s = *stream ;                        // local copy of stream control structure

  errmsg = "rank != 2" ;
  if(rank != 2) goto fail ;                      // only 2D is supported at this time
  nvalues = a->dim[0].gnn * a->dim[1].gnn ;      // number of values in array

  if(command == DMAP_RESTORE) goto restore ;     // this is a call to the restore filter
//   if(dpfl == NULL ) goto restore ;

  errmsg = "invalid filter" ;
  if(! dmap_filter_valid(dpfl,self)) goto fail ;   // not the right filter or NULL pointer

  errmsg = "type != float_data" ;
  if(type != float_data) goto fail ;             // data type MUST BE FLOAT

  arg = (FILTER_ARGS *)(*dpfl) ;                 // get parameters for this filter
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
    errmsg = "mode == FP_2_FAKELOG, not supported yet" ;
    goto fail ;        // fake log quantizer not supported yet
  }else{
    errmsg = "invalid mode" ;
    goto fail ;        // invalid mode
  }

  dpfl++ ;                                       // call next filter if there is one
  dmap_filter_ptr next_filter = dmap_filter_next(dpfl) ;
  status = (*next_filter)(a, NULL, dpfl, &s, command) ;   // block properties are no longer valid
  if(status < 0) goto fail ;
//
// insert into bitstream the appropriate data for the restore filter (PUT)
//
  int inserted = 0 ;
  STREAM_PUT_NBITS(s, FILTER_ID, 8) ; status += 8 ;
//   inserted += 8 ;                                 // 8 bits inserted so far
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
  fprintf(stderr, "filter %3.3o : inserted %d bits, quantum = %f, exp = %d, offset = %d, mode = %d\n",
          FILTER_ID, inserted, fp32_pow2(e_base-127), e_base, offset, mode) ;

// successful end
end:
  *stream = s ;   // success, SAVE stream changes
  return status ;

// miserable failure
fail:
  fprintf(stderr, "filter %3.3o ERROR : %s\n", FILTER_ID, errmsg) ;
  return -1 ;     // failure, DO NOT SAVE stream changes

// restore original data using forward filter result
restore:
  errmsg = "restore filter : data type MUST BE integer" ;
  if(type != int_data && type != uint_data) goto fail ;             // data type MUST BE INTEGER
// get the appropriate information for the restore filter from bitstream (GET)
  STREAM_GET_NBITS(s, self, 8) ;
  status = 8 ;                                         // 8 bits extracted so far
  errmsg = "inconsistent filter ID" ;
  if(self != FILTER_ID) goto fail ;                  // wrong id, MUST be FILTER_ID
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
  fprintf(stderr, "restore filter %3.3o, array[%d,%d](%d), offset = %d, status = %ld\n",
                  FILTER_ID, a->dim[0].gnn, a->dim[1].gnn, nvalues, offset, status) ;
  a->type = float_data ;                               // mark data as float data

  ssize_t status2 = dmap_filter_inv(a, &s) ;           // call next inverse filter
  errmsg = "restore filter chain failed" ;
  if(status2 < 0) goto fail ;
fprintf(stderr, "restore filter %3.3o, status2 = %ld\n", FILTER_ID, status2) ;
  status += status2 ;
  goto end ;                                           // success

// encode filter parameters into bit stream
encode:
  errmsg = "no stream" ;
  if(stream == NULL) goto fail ;
  errmsg = "invalid filter" ;
  if(self != FILTER_ID) goto fail ;                    // wrong id, MUST be FILTER_ID
  s = *stream ;
  fprintf(stderr, "encode parameters, filter = %d", self) ;
  STREAM_PUT_NBITS(s, self, 8) ; status = 8 ;
  arg = (FILTER_ARGS *)(*dpfl) ;    // parameters for this filter
                          STREAM_PUT_NBITS(s, arg->mode  , 12) ; status += 12 ;
                          STREAM_PUT_NBITS(s, arg->nbits , 12) ; status += 12 ;
  x32.f32 = arg->abserr ; STREAM_PUT_NBITS(s, x32.u32, 32)     ; status += 32 ;
                          STREAM_PUT_NBITS(s, arg->offset, 32) ; status += 32 ;
  x32.f32 = arg->minabs ; STREAM_PUT_NBITS(s, x32.u32, 32)     ; status += 32 ;
  x32.f32 = arg->zval   ; STREAM_PUT_NBITS(s, x32.u32, 32)     ; status += 32 ;
  fprintf(stderr, ", mode = %d, nbits = %d, err = %10E, offset = %8.8x, minabs = %10E, zval = %10E, status = %ld\n",
                   arg->mode, arg->nbits, arg->maxerr, arg->offset, arg->minabs, arg->zval,status) ;
  goto end ;                                           // success

// recover filter parameters from bit stream
decode:
  errmsg = "no stream" ;
  if(stream == NULL) goto fail ;
  s = *stream ;
  fprintf(stderr, "decode parameters") ;
  STREAM_GET_NBITS(s, self, 8) ;
  errmsg = "invalid filter" ;
  if(self != FILTER_ID) goto fail ;
  FILTER_ARGS *argp = (FILTER_ARGS *) dpfl[0] ;         // parameters for this filter
  argp->filter  = self ;
  uint32_t u32 ; int32_t i32 ;
  STREAM_GET_NBITS(s, i32, 12)     ; argp->mode   = i32 ;
  STREAM_GET_NBITS(s, u32, 12)     ; argp->nbits  = u32 ;
  STREAM_GET_NBITS(s, x32.u32, 32) ; argp->abserr = x32.f32 ;
  STREAM_GET_NBITS(s, i32, 32)     ; argp->offset = i32 ;
  STREAM_GET_NBITS(s, x32.u32, 32) ; argp->minabs = x32.f32 ;
  STREAM_GET_NBITS(s, x32.u32, 32) ; argp->zval   = x32.f32 ;
  status = sizeof(FILTER_ARGS) ;
  fprintf(stderr, ", filter = %d, mode = %d, nbits = %d, err = %10E, offset = %8.8x, minabs = %10E, zval = %10E, status = %ld\n",
                   self, argp->mode, argp->nbits, argp->maxerr, argp->offset, argp->minabs, argp->zval, status) ;
  goto end ;                                           // success

print:
  errmsg = "invalid filter" ;
  if(! dmap_filter_valid(dpfl,self)) goto fail ;   // not the right filter or NULL pointer
  FILTER_ARGS *arg0 = (FILTER_ARGS *)(*dpfl) ;      // get parameters for this filter
  fprintf(stderr, "[%3.3o] Float Quantizer, mode = %d, nbits = %d, err = %10E, offset = %8.8x, minabs = %10E, zval = %10E\n",
                  arg0->filter, arg0->mode, arg0->nbits, arg0->maxerr, arg0->offset, arg0->minabs, arg0->zval) ;
  return 0 ;
}
#endif
#undef FILTER_NAME
#undef FILTER_ARGS
#undef FILTER_ID

// ======================================= filter 004 =======================================
// Lorenzo predictor
#include <rmn/lorenzo.h>
#define FILTER_ID 004
#define FILTER_NAME CAT(dmap_filter_,FILTER_ID)
#define FILTER_ARGS CAT(dmap_filter_arg_,FILTER_ID)
// dpfl == NULL : restore filter call (no list needed)
// for the restore filter, the bit stream provides the necessary information
// the filter may modify the contents of the array described by a
// in filter mode, bp == NULL if no properties information is available
// the filter list MUST BE NULL TERMINATED
ssize_t FILTER_NAME(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream, dmap_command command){
  ssize_t status = 0, status2 ;
  uint32_t self, rank, type ;
  char *errmsg = "" ;
  FILTER_ARGS *arg ;
  void *array ;
  bitstream s ;

// ==================== local variable declarations ====================
// local code
// ========================================================================
  if(command != DMAP_RESTORE){                       // DMAP_RESTORE does not use a parameter list
    errmsg = "dmap filter list is NULL" ;
    if(dpfl == NULL) goto fail ;
    arg = (FILTER_ARGS *) dpfl[0] ;                  // get parameters for this filter
    errmsg = "invalid/inconsistent filter ID" ;
    if(! dmap_filter_valid(dpfl,FILTER_ID)) goto fail ;   // not the expected filter ID
  }
  if(command != DMAP_PRINT){                         // DMAP_PRINT does not use the bit stream
    errmsg = "no stream" ;
    if(stream == NULL) goto fail ;                   // no bit stream
    s = *stream ;                                    // local copy of stream control structure
  }

  if(command == DMAP_ENCODE) goto encode ;
  if(command == DMAP_DECODE) goto decode ;
  if(command == DMAP_PRINT)  goto print ;
  if(command == DMAP_RESTORE || command == DMAP_FILTER){
    errmsg = "no array" ;
    if(a == NULL) goto fail ;
    array = array_address(a) ;                     // get array address, dimension(s), and type
    if(array == NULL) goto fail ;
    rank = a->rank ;
    type = a->type ;
    // check type and rank as/if needed
    errmsg = "expecting int_data or uint_data" ;
    if(type != int_data && type != uint_data) goto fail ;    // integers only
    errmsg = "expecting 2D array" ;
    if(rank != 2) goto fail ;                                // 2 D only
    if(command == DMAP_RESTORE) goto restore ;
    goto forward ;
  }else{      // not DMAP_ENCODE, DMAP_DECODE, DMAP_PRINT, DMAP_RESTORE, DMAP_FILTER
    errmsg = "invalid command" ;
    goto fail ;
  }
// forward filter
forward :
// ====================  filter processing code  (FWD) ====================
// local code transforming array before calling next filter
  // call Lorenzo predictor in place
  LorenzoPredictInplace((int32_t *)array, a->dim[0].gnn, a->dim[0].gnn, a->dim[1].gnn) ;
  a->type = int_data ;
  bp = NULL ;                                    // not used
// ========================================================================
  dpfl++ ;                              // call next filter
  dmap_filter_ptr next_filter = dmap_filter_next(dpfl) ;
  status = (*next_filter)(a, bp, dpfl, &s, command) ;
  errmsg = "filter chain failed" ;
  if(status < 0) goto fail ;

  STREAM_PUT_NBITS(s, FILTER_ID, 8) ; status += 8 ;
// ====================  filter processing code  (FWD) ====================
// local code inserting proper data into bit stream
// ========================================================================

// successful end
end:
  *stream = s ;   // success, SAVE stream changes
  return status ;

// miserable failure
fail:
  fprintf(stderr, "filter %3.3o ERROR : %s\n", FILTER_ID, errmsg) ;
  return -1 ;     // failure, DO NOT SAVE stream changes

// inverse of forward filter
restore:
// get the appropriate information for the restore filter from bitstream
  STREAM_GET_NBITS(s, self, 8) ; status = 8 ;          // 8 bits extracted so far
  errmsg = "inconsistent filter ID" ;
  if(self != FILTER_ID) goto fail ;                    // wrong id, MUST be FILTER_ID

// ====================  restore (INV) ====================
// local code to restore from bit stream
// inverse array processing code
  errmsg = "expecting signed integer data" ;
  if(type != int_data) goto fail ;
  // call Lorenzo inverse predictor in place
  LorenzoUnpredictInplace((int32_t *)array, a->dim[0].gnn, a->dim[0].gnn, a->dim[1].gnn) ;
  a->type = int_data ;
// ========================================================================

  status2 = dmap_filter_inv(a, &s) ;           // call next inverse filter
  errmsg = "restore filter chain failed" ;
  if(status2 < 0) goto fail ;
  status += status2 ;
  goto end ;

// encode filter parameters from *dpfl[0] into bit stream
encode:
  status = 0 ;
  s = *stream ;                        // local copy of stream control structure
  fprintf(stderr, "encode parameters : filter = %3.3o", arg->filter) ;
  arg = (FILTER_ARGS *) dpfl[0] ;    // parameters for this filter
  STREAM_PUT_NBITS(s, arg->filter , 8) ; status = 8 ;
// ========================================================================
// local code
  fprintf(stderr, "(%3.3o), status = %ld\n", arg->filter, status) ;
// ========================================================================
  goto end ;

// decode filter parameters from bit stream, copy into *dpfl[0]
decode:
  status = 0 ;
  s = *stream ;                        // local copy of stream control structure
  fprintf(stderr, "decode parameters, filter = %3.3o", arg->filter) ;
  STREAM_GET_NBITS(s, self, 8) ;
  errmsg = "decode parameters : self != FILTER_ID" ;
  if(self != FILTER_ID) goto fail ;                     // wrong id, MUST be FILTER_ID
  status = sizeof(FILTER_ARGS) ;
  arg = (FILTER_ARGS *) dpfl[0] ;                       // parameters for this filter
  arg->filter  = self ;
  status = sizeof(FILTER_ARGS) ;
// ========================================================================
  fprintf(stderr, "(%3.3o), status = %ld\n", self, status) ;
// local code
// ========================================================================
  goto end ;

// print filter parameters
print:
  errmsg = "invalid filter" ;
  if(! dmap_filter_valid(dpfl,self)) goto fail ;   // not the right filter or NULL pointer
  arg = (FILTER_ARGS *) dpfl[0] ;                       // parameters for this filter
// ========================================================================
  fprintf(stderr, "[%3.3o] Lorenzo predictor\n", arg->filter) ;
// local code
// ========================================================================
  return 0 ;
}
#undef FILTER_NAME
#undef FILTER_ARGS
#undef FILTER_ID

// ======================================= filter 005 =======================================
// integer wavelet transform
#define FILTER_ID 005
#define FILTER_NAME CAT(dmap_filter_,FILTER_ID)
#define FILTER_ARGS CAT(dmap_filter_arg_,FILTER_ID)
// dpfl == NULL : restore filter call (no list needed)
// for the restore filter, the bit stream provides the necessary information
// the filter may modify the contents of the array described by a
// in filter mode, bp == NULL if no properties information is available
// the filter list MUST BE NULL TERMINATED
ssize_t FILTER_NAME(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream, dmap_command command){
  ssize_t status = 0, status2 ;
  uint32_t self, rank, type ;
  char *errmsg = "" ;
  FILTER_ARGS *arg ;
  void *array ;
  bitstream s ;

// ==================== local variable declarations ====================
  int ni, nj, levels ;
// ========================================================================
  if(command != DMAP_RESTORE){                       // DMAP_RESTORE does not use a parameter list
    errmsg = "dmap filter list is NULL" ;
    if(dpfl == NULL) goto fail ;
    arg = (FILTER_ARGS *) dpfl[0] ;                  // get parameters for this filter
    errmsg = "invalid/inconsistent filter ID" ;
    if(! dmap_filter_valid(dpfl,FILTER_ID)) goto fail ;   // not the expected filter ID
  }
  if(command != DMAP_PRINT){                         // DMAP_PRINT does not use the bit stream
    errmsg = "no stream" ;
    if(stream == NULL) goto fail ;                   // no bit stream
    s = *stream ;                                    // local copy of stream control structure
  }

  if(command == DMAP_ENCODE) goto encode ;
  if(command == DMAP_DECODE) goto decode ;
  if(command == DMAP_PRINT)  goto print ;
  if(command == DMAP_RESTORE || command == DMAP_FILTER){
    errmsg = "no array" ;
    if(a == NULL) goto fail ;
    array = array_address(a) ;                     // get array address, dimension(s), and type
    if(array == NULL) goto fail ;
    rank = a->rank ;
    type = a->type ;
    // check type and rank as/if needed
    errmsg = "expecting int_data or uint_data" ;
    if(type != int_data && type != uint_data) goto fail ;    // integers only
    errmsg = "expecting 2D array" ;
    if(rank != 2) goto fail ;                      // 2 D only
    if(command == DMAP_RESTORE) goto restore ;
    goto forward ;
  }else{      // not DMAP_ENCODE, DMAP_DECODE, DMAP_PRINT, DMAP_RESTORE, DMAP_FILTER
    errmsg = "invalid command" ;
    goto fail ;
  }
// forward filter
forward :
// ====================  filter processing code  (FWD) ====================
// local code transforming array before calling next filter
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
// ========================================================================
  dpfl++ ;                              // call next filter
  dmap_filter_ptr next_filter = dmap_filter_next(dpfl) ;
  status = (*next_filter)(a, bp, dpfl, &s, command) ;
  errmsg = "filter chain failed" ;
  if(status < 0) goto fail ;

  STREAM_PUT_NBITS(s, FILTER_ID, 8) ; status += 8 ;
// ====================  filter processing code  (FWD) ====================
// local code inserting proper data into bit stream
  STREAM_PUT_NBITS(s, levels, 8) ; status += 8 ;
  STREAM_INSERT_PUSH(s) ;
// ========================================================================

// successful end
end:
  *stream = s ;   // success, SAVE stream changes
  return status ;

// miserable failure
fail:
  fprintf(stderr, "filter %3.3o ERROR : %s\n", FILTER_ID, errmsg) ;
  return -1 ;     // failure, DO NOT SAVE stream changes

// inverse of forward filter
restore:
// get the appropriate information for the restore filter from bitstream
  STREAM_GET_NBITS(s, self, 8) ; status = 8 ;          // 8 bits extracted so far
  errmsg = "inconsistent filter ID" ;
  if(self != FILTER_ID) goto fail ;                    // wrong id, MUST be FILTER_ID

// ====================  restore (INV) ====================
// local code to restore from bit stream
  STREAM_GET_NBITS(s, levels, 8) ; status += 8 ;
// inverse array processing code
  errmsg = "expecting int_data" ;
  if(type != int_data) goto fail ;
  ni = a->dim[0].gnn ;
  nj = a->dim[1].gnn ;

//   fprintf(stderr, "filter %3.3o %d levels %d x %d inverse wavelet transform\n", FILTER_ID, levels, ni, nj) ;
  if(levels > 0) inv_2d_lgt53_n(array, ni, ni, nj, levels);
  a->type = int_data ;
// ========================================================================

  status2 = dmap_filter_inv(a, &s) ;           // call next inverse filter
  errmsg = "restore filter chain failed" ;
  if(status2 < 0) goto fail ;
  status += status2 ;
  goto end ;

// encode filter parameters from *dpfl[0] into bit stream
encode:
  status = 0 ;
  s = *stream ;                        // local copy of stream control structure
  fprintf(stderr, "encode parameters : filter = %3.3o", arg->filter) ;
  arg = (FILTER_ARGS *) dpfl[0] ;    // parameters for this filter
  STREAM_PUT_NBITS(s, arg->filter , 8) ; status = 8 ;
// ========================================================================
  STREAM_PUT_NBITS(s, arg->levels, 8) ; status += 8 ;
  fprintf(stderr, "(%3.3o), levels = %d, status = %ld\n", arg->filter, arg->levels, status) ;
// ========================================================================
  goto end ;

// decode filter parameters from bit stream, copy into *dpfl[0]
decode:
  status = 0 ;
  s = *stream ;                        // local copy of stream control structure
  fprintf(stderr, "decode parameters, filter = %3.3o", arg->filter) ;
  STREAM_GET_NBITS(s, self, 8) ;
  errmsg = "decode parameters : self != FILTER_ID" ;
  if(self != FILTER_ID) goto fail ;                     // wrong id, MUST be FILTER_ID
  status = sizeof(FILTER_ARGS) ;
  arg = (FILTER_ARGS *) dpfl[0] ;                       // parameters for this filter
  arg->filter  = self ;
  status = sizeof(FILTER_ARGS) ;
// ========================================================================
  uint32_t w32 ;
  STREAM_GET_NBITS(s, w32, 8) ;
  arg->levels = w32 ;
  status = sizeof(FILTER_ARGS) ;
  fprintf(stderr, "(%3.3o), levels = %d, status = %ld\n", self, arg->levels, status) ;
// ========================================================================
  goto end ;

// print filter parameters
print:
  arg = (FILTER_ARGS *) dpfl[0] ;                       // parameters for this filter
// ========================================================================
  fprintf(stderr, "[%3.3o] Integer Wavelet, levels = %d\n", arg->filter, arg->levels) ;
// local code
// ========================================================================
  return 0 ;
}
#undef FILTER_NAME
#undef FILTER_ARGS
#undef FILTER_ID

// ======================================= filter 006 =======================================
// for the restore filter, the bit stream provides the necessary information
// bit stream encoder
#define FILTER_ID 006
#define FILTER_NAME CAT(dmap_filter_,FILTER_ID)
#define FILTER_ARGS CAT(dmap_filter_arg_,FILTER_ID)
// dpfl == NULL : restore filter call (no list needed, required data will be in bit stream)
// the filter may modify the contents of the array described by argument a
// in forward filter mode, bp == NULL if no properties information is available
// bp is irrelevant in restore filter mode
// the filter list MUST BE NULL TERMINATED
//
// this filter MUST BE THE LAST active filter in the chain as it encodes its data
#if 1
ssize_t FILTER_NAME(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream, dmap_command command){
  ssize_t status = 0, status2 ;
  uint32_t self, rank, type ;
  char *errmsg = "" ;
  FILTER_ARGS *arg ;
  void *array ;
  bitstream s ;

// ==================== local variable declarations ====================
  int32_t ni, nj, mode, nbits, tnbits, tile, bhw, nelem, i ;
  uint32_t zigzag ;
// ========================================================================
  if(command != DMAP_RESTORE){                       // DMAP_RESTORE does not use a parameter list
    errmsg = "dmap filter list is NULL" ;
    if(dpfl == NULL) goto fail ;
    arg = (FILTER_ARGS *) dpfl[0] ;                  // get parameters for this filter
    errmsg = "invalid/inconsistent filter ID" ;
    if(! dmap_filter_valid(dpfl,FILTER_ID)) goto fail ;   // not the expected filter ID
  }
  if(command != DMAP_PRINT){                         // DMAP_PRINT does not use the bit stream
    errmsg = "no stream" ;
    if(stream == NULL) goto fail ;                   // no bit stream
    s = *stream ;                                    // local copy of stream control structure
  }

  if(command == DMAP_ENCODE) goto encode ;
  if(command == DMAP_DECODE) goto decode ;
  if(command == DMAP_PRINT)  goto print ;
  if(command == DMAP_RESTORE || command == DMAP_FILTER){
    errmsg = "no array" ;
    if(a == NULL) goto fail ;
    array = array_address(a) ;                     // get array address, dimension(s), and type
    if(array == NULL) goto fail ;
    rank = a->rank ;
    type = a->type ;
    // check type and rank as/if needed
    // local code
    if(command == DMAP_RESTORE) goto restore ;
    goto forward ;
  }else{      // not DMAP_ENCODE, DMAP_DECODE, DMAP_PRINT, DMAP_RESTORE, DMAP_FILTER
    errmsg = "invalid command" ;
    goto fail ;
  }
// forward filter
forward :
// ====================  filter processing code  (FWD) ====================
  if(! dmap_filter_is_last(dpfl)){
    errmsg = "filter 006 MUST BE THE LAST FILTER" ;
    goto fail ;
  }

  errmsg = "data must be integer (signed or unsigned)" ;
  if(type != int_data && type != uint_data) goto fail ;

  errmsg="encoder only supports 1D or 2D arrays" ;
  if(rank > 2) goto fail ;
// local code transforming array before calling next filter

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
// ========================================================================
  dpfl++ ;                              // call next filter
  dmap_filter_ptr next_filter = dmap_filter_next(dpfl) ;
  status = (*next_filter)(a, bp, dpfl, &s, command) ;
  errmsg = "filter chain failed" ;
  if(status < 0) goto fail ;

  STREAM_PUT_NBITS(s, FILTER_ID, 8) ; status += 8 ;
// ====================  filter processing code  (FWD) ====================
//
  uint32_t header, *z = (uint32_t *) array ;
  ssize_t available = StreamAvailableSpace(&s) ;

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
  status = status ;
// ========================================================================

// successful end
end:
  *stream = s ;   // success, SAVE stream changes
  return status ;

// miserable failure
fail:
  fprintf(stderr, "filter %3.3o ERROR : %s\n", FILTER_ID, errmsg) ;
  return -1 ;     // failure, DO NOT SAVE stream changes

// inverse of forward filter
restore:
// get the appropriate information for the restore filter from bitstream
  STREAM_GET_NBITS(s, self, 8) ; status = 8 ;          // 8 bits extracted so far
  errmsg = "inconsistent filter ID" ;
  if(self != FILTER_ID) goto fail ;                    // wrong id, MUST be FILTER_ID

// ====================  restore (INV) ====================
// local code to restore from bit stream
// inverse array processing code
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
//     fprintf(stderr, "restore filter %3.3o, id = %d, header = 0x%2.2x, nbits = %d, zigzag = %d\n", FILTER_ID, self, header, nbits, zigzag) ;
    if(nbits > 32)           goto fail ;                 // not supported yet
    nelem = ni * nj ;
    z = (uint32_t *) array ;
//     fprintf(stderr, "restore filter %3.3o restoring %d array elements\n", FILTER_ID, nelem) ;
    if(zigzag == 2){                                     // BHW mode, nbits MUST be 32 (but will be ignored)
      if(nbits != 32) goto fail ;
      for(i=0 ; i<nelem ; i++) { STREAM_GET_BHW(s, z[i], tnbits) ; status += tnbits ; } ;
// fprintf(stderr, "BHW decoded %ld bits\n", status) ;
    }else{                                               // constant nbits, maybe zigzag
      int32_t max = 0 ;                                  // make sure that max is always initialized
      for(i=0 ; i<nelem ; i++) { STREAM_GET_NBITS(s, z[i], nbits) ; status += nbits ; } ;
      if(zigzag != 0) max = v_from_zigzag_32_inplace((int32_t *)array, nelem) ;
      if(max == 0) fprintf(stderr, "restore filter %3.3o, max = %d, zigzag = %d\n", FILTER_ID, max, zigzag) ;
    }

  }else{                                                 // invalid header value
    goto fail ;
  }
  a->type = int_data ;                                   // output data type is signed integers
  array_set_used(a) ;
// fprintf(stderr, "REVERSE  filter 006(X) : extracted %ld bits\n", status) ;
// ========================================================================

  status2 = dmap_filter_inv(a, &s) ;           // call next inverse filter
  errmsg = "restore filter chain failed" ;
  if(status2 < 0) goto fail ;
  status += status2 ;
// ========================================================================
  errmsg = "array should contain data" ;
  if( ! array_has_data(a) ) goto fail ;                  // array should be filled
  ni = a->dim[0].gnn ; nj = 1 ;                          // and final shape should be as expected
  if(a->rank == 2) nj = a->dim[1].gnn ;
  errmsg = "final array dimensions not as expected" ;
  if(ni != ni_in || nj != nj_in || rank != a->rank) goto fail ;
// ========================================================================
  goto end ;

// encode filter parameters from *dpfl[0] into bit stream
encode:
  status = 0 ;
  s = *stream ;                        // local copy of stream control structure
  fprintf(stderr, "encode parameters : filter = %3.3o", arg->filter) ;
  arg = (FILTER_ARGS *) dpfl[0] ;    // parameters for this filter
  STREAM_PUT_NBITS(s, arg->filter , 8) ; status = 8 ;
// ========================================================================
  STREAM_PUT_NBITS(s, arg->mode   , 8) ; status += 8 ;
  STREAM_PUT_NBITS(s, arg->options, 8) ; status += 8 ;
  fprintf(stderr, "(%3.3o), mode = %d, options = %x, status = %ld\n", arg->filter, arg->mode, arg->options, status) ;
// ========================================================================
  goto end ;

// decode filter parameters from bit stream, copy into *dpfl[0]
decode:
  status = 0 ;
  s = *stream ;                        // local copy of stream control structure
  fprintf(stderr, "decode parameters, filter = %3.3o", arg->filter) ;
  STREAM_GET_NBITS(s, self, 8) ;
  errmsg = "decode parameters : self != FILTER_ID" ;
  if(self != FILTER_ID) goto fail ;                     // wrong id, MUST be FILTER_ID
  status = sizeof(FILTER_ARGS) ;
  arg = (FILTER_ARGS *) dpfl[0] ;                       // parameters for this filter
  arg->filter  = self ;
  status = sizeof(FILTER_ARGS) ;
// ========================================================================
  int32_t w32 ;
  STREAM_GET_NBITS(s, w32, 8) ;
  arg->mode = w32 ;
  STREAM_GET_NBITS(s, w32, 8) ;
  arg->options = w32 ;
  fprintf(stderr, "(%3.3o), mode = %d, options = %x, status = %ld\n", arg->filter, arg->mode, arg->options, status) ;
// ========================================================================
  goto end ;

// print filter parameters
print:
  arg = (FILTER_ARGS *) dpfl[0] ;                       // parameters for this filter
// ========================================================================
  fprintf(stderr, "[%3.3o] Integer Encoder, mode = %d, options = %x\n", arg->filter, arg->mode, arg->options) ;
// ========================================================================
  return 0 ;
}
#else
ssize_t FILTER_NAME(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream, dmap_command command){
  ssize_t status = 0 ;
  uint32_t self = FILTER_ID, rank, type ;
  char *errmsg = "" ;
  FILTER_ARGS *arg ;
  void *array ;
  bitstream s ;                        // local copy of stream control structure

  // specific declarations
  int32_t ni, nj, mode, nbits, tnbits, tile, bhw, nelem, i ;
  uint32_t zigzag ;
  ssize_t status2 ;

  if(command != DMAP_RESTORE){                   // DMAP_RESTORE does not use a parameter list
    errmsg = "dmap filter list is NULL" ;
    if(dpfl == NULL) goto fail ;
    arg = (FILTER_ARGS *) dpfl[0] ;              // get parameters for this filter
  }

  if(command == DMAP_ENCODE) goto encode ;
  if(command == DMAP_DECODE) goto decode ;
  if(command == DMAP_PRINT)  goto print ;

  errmsg = "no array" ; if(a == NULL) goto fail ;
  array = array_address(a) ;                     // get array address, dimension(s), and type
  rank = a->rank ;
  type = a->type ;

  errmsg = "no stream" ; if(stream == NULL) goto fail ;
  s = *stream ;
//   fprintf(stderr, "filter 006(I) : available space = %ld bits, available bits = %ld bits\n", StreamAvailableSpace(&s), StreamAvailableBits(stream)) ;

  if(command == DMAP_RESTORE) goto restore ;     // this is a call to the restore filter
// ================================ forward filter ================================
  errmsg = "invalid filter" ;
  if(! dmap_filter_valid(dpfl,self)) goto fail ; // not the right filter or NULL pointer
  arg = (FILTER_ARGS *) dpfl[0] ;                // get parameters for this filter

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
  dmap_filter_ptr next_filter = dmap_filter_next(++dpfl) ;       // call next filter (filter chain terminator for this particular filter)
  status = (*next_filter)(a, bp, dpfl, &s, command) ;                   // should not fail
  errmsg="tail filter failure" ;  if(status < 0) goto fail ;
//   fprintf(stderr, "filter 006(M) : status = %ld, available space in stream = %ld bits", status, StreamAvailableSpace(&s)) ;
//   fprintf(stderr, ", available bits = %ld bits", StreamAvailableBits(&s)) ;
//   fprintf(stderr, ", insert restore FILTER_ID = %3.3o\n", FILTER_ID) ;
//
// insert the appropriate data for the restore filter into bitstream
//
  uint32_t header, *z = (uint32_t *) array ;
  ssize_t available = StreamAvailableSpace(&s) ;

  STREAM_PUT_NBITS(s, FILTER_ID, 8) ;          // restore filter ID (same as self)
  status += 8 ;

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
  status = status ;

// successsful end
end:
  *stream = s ;   // SAVE stream changes
//   fprintf(stderr, "filter 006(X) : available space in stream %ld bits\n", StreamAvailableSpace(stream)) ;
// fprintf(stderr, "filter 006(X) : inserted %ld bits\n", status);
  return status ;    // return number of bits produced/decoded

// miserable failure
fail:
  fprintf(stderr, "filter %3.3o ERROR : %s\n", FILTER_ID, errmsg) ;
  return -1 ;     // failure, DO NOT SAVE stream changes

// ================================ restore filter ================================
// decode bit stream encoded by forward filter
restore:
  STREAM_GET_NBITS(s, self, 8) ; status = 8 ;            // filter ID from stream
  errmsg = "inconsistent filter ID" ;
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
//     fprintf(stderr, "restore filter %3.3o, id = %d, header = 0x%2.2x, nbits = %d, zigzag = %d\n", FILTER_ID, self, header, nbits, zigzag) ;
    if(nbits > 32)           goto fail ;                 // not supported yet
    nelem = ni * nj ;
    z = (uint32_t *) array ;
//     fprintf(stderr, "restore filter %3.3o restoring %d array elements\n", FILTER_ID, nelem) ;
    if(zigzag == 2){                                     // BHW mode, nbits MUST be 32 (but will be ignored)
      if(nbits != 32) goto fail ;
      for(i=0 ; i<nelem ; i++) { STREAM_GET_BHW(s, z[i], tnbits) ; status += tnbits ; } ;
// fprintf(stderr, "BHW decoded %ld bits\n", status) ;
    }else{                                               // constant nbits, maybe zigzag
      int32_t max = 0 ;                                  // make sure that max is always initialized
      for(i=0 ; i<nelem ; i++) { STREAM_GET_NBITS(s, z[i], nbits) ; status += nbits ; } ;
      if(zigzag != 0) max = v_from_zigzag_32_inplace((int32_t *)array, nelem) ;
      if(max == 0) fprintf(stderr, "restore filter %3.3o, max = %d, zigzag = %d\n", FILTER_ID, max, zigzag) ;
    }

  }else{                                                 // invalid header value
    goto fail ;
  }
  a->type = int_data ;                                   // output data type is signed integers
  array_set_used(a) ;
// fprintf(stderr, "REVERSE  filter 006(X) : extracted %ld bits\n", status) ;
  status2 = dmap_filter_inv(a, &s) ;                     // call next inverse filter
  if(status2 < 0) goto fail ; else status += status2 ;

  if( ! array_has_data(a) ) goto fail ;                  // array should be filled
  ni = a->dim[0].gnn ; nj = 1 ;                          // and final shape should be as expected
  if(a->rank == 2) nj = a->dim[1].gnn ;
  errmsg = "final array dimensions not as expected" ;
  if(ni != ni_in || nj != nj_in || rank != a->rank) goto fail ;

  goto end ;

encode:
  errmsg = "encode : self != FILTER_ID" ;
  if(self != FILTER_ID) goto fail ;                     // wrong id, MUST be FILTER_ID
  status = 0 ;
  s = *stream ;                        // local copy of stream control structure
  fprintf(stderr, "encode parameters, filter = %d", self) ;
  STREAM_PUT_NBITS(s, self, 8) ; status = 8 ;
  arg = (FILTER_ARGS *) dpfl[0] ;    // parameters for this filter
  STREAM_PUT_NBITS(s, arg->mode   , 8) ; status += 8 ;
  STREAM_PUT_NBITS(s, arg->options, 8) ; status += 8 ;
  fprintf(stderr, ", self = %8.8x, mode = %d, options = %x, status = %ld\n", self, arg->mode, arg->options, status) ;
  goto end ;

decode:
  errmsg = "decode : self != FILTER_ID" ;
  status = 0 ;
  s = *stream ;                        // local copy of stream control structure
  fprintf(stderr, "decode parameters, filter = %d", self) ;
  STREAM_GET_NBITS(s, self, 8) ;
  if(self != FILTER_ID) goto fail ;                     // wrong id, MUST be FILTER_ID
  status = sizeof(FILTER_ARGS) ;
  arg = (FILTER_ARGS *) dpfl[0] ;                       // parameters for this filter
  arg->filter  = self ;
  int32_t w32 ;
  STREAM_GET_NBITS(s, w32, 8) ;
  arg->mode = w32 ;
  STREAM_GET_NBITS(s, w32, 8) ;
  arg->options = w32 ;
  status = sizeof(FILTER_ARGS) ;
  fprintf(stderr, ", self = %8.8x, mode = %d, options = %x, status = %ld\n", self, arg->mode, arg->options, status) ;
  goto end ;

print:
  errmsg = "invalid filter" ;
  if(! dmap_filter_valid(dpfl,self)) goto fail ;   // not the right filter or NULL pointer
  arg = (FILTER_ARGS *)(*dpfl) ;                   // get parameters for this filter
  fprintf(stderr, "[%3.3o] Integer Encoder, mode = %d, options = %x\n", arg->filter, arg->mode, arg->options) ;
  return 0 ;
}
#endif
#undef FILTER_NAME
#undef FILTER_ARGS
#undef FILTER_ID


// ======================================= filter 007 =======================================
// NO-OP filter for now
#define FILTER_ID 007
#define FILTER_NAME CAT(dmap_filter_,FILTER_ID)
#define FILTER_ARGS CAT(dmap_filter_arg_,FILTER_ID)
ssize_t FILTER_NAME(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream, dmap_command command){
  ssize_t status = 0, status2 ;
  uint32_t self, rank, type ;
  char *errmsg = "" ;
  FILTER_ARGS *arg ;
  void *array ;
  bitstream s ;

// ==================== local variable declarations ====================
// local code
// ========================================================================
  if(command != DMAP_RESTORE){                       // DMAP_RESTORE does not use a parameter list
    errmsg = "dmap filter list is NULL" ;
    if(dpfl == NULL) goto fail ;
    arg = (FILTER_ARGS *) dpfl[0] ;                  // get parameters for this filter
    errmsg = "invalid/inconsistent filter ID" ;
    if(! dmap_filter_valid(dpfl,FILTER_ID)) goto fail ;   // not the expected filter ID
  }
  if(command != DMAP_PRINT){                         // DMAP_PRINT does not use the bit stream
    errmsg = "no stream" ;
    if(stream == NULL) goto fail ;                   // no bit stream
    s = *stream ;                                    // local copy of stream control structure
  }

  if(command == DMAP_ENCODE) goto encode ;
  if(command == DMAP_DECODE) goto decode ;
  if(command == DMAP_PRINT)  goto print ;
  if(command == DMAP_RESTORE || command == DMAP_FILTER){
    errmsg = "no array" ;
    if(a == NULL) goto fail ;
    array = array_address(a) ;                     // get array address, dimension(s), and type
    if(array == NULL) goto fail ;
    rank = a->rank ;
    type = a->type ;
    // check type and rank as/if needed
    errmsg = "invalid data type or rank" ;
    if(rank >5 || type == bad_data) goto fail ;
    if(command == DMAP_RESTORE) goto restore ;
    goto forward ;
  }else{      // not DMAP_ENCODE, DMAP_DECODE, DMAP_PRINT, DMAP_RESTORE, DMAP_FILTER
    errmsg = "invalid command" ;
    goto fail ;
  }
// forward filter
forward :
// ====================  filter processing code  (FWD) ====================
// local code transforming array before calling next filter
// ========================================================================
  dpfl++ ;                              // call next filter
  dmap_filter_ptr next_filter = dmap_filter_next(dpfl) ;
  status = (*next_filter)(a, bp, dpfl, &s, command) ;
  errmsg = "filter chain failed" ;
  if(status < 0) goto fail ;

  STREAM_PUT_NBITS(s, FILTER_ID, 8) ; status += 8 ;
// ====================  filter processing code  (FWD) ====================
// local code inserting proper data into bit stream
// ========================================================================

// successful end
end:
  *stream = s ;   // success, SAVE stream changes
  return status ;

// miserable failure
fail:
  fprintf(stderr, "filter %3.3o ERROR : %s\n", FILTER_ID, errmsg) ;
  return -1 ;     // failure, DO NOT SAVE stream changes

// inverse of forward filter
restore:
// get the appropriate information for the restore filter from bitstream
  STREAM_GET_NBITS(s, self, 8) ; status = 8 ;          // 8 bits extracted so far
  errmsg = "inconsistent filter ID" ;
  if(self != FILTER_ID) goto fail ;                    // wrong id, MUST be FILTER_ID

// ====================  restore (INV) ====================
// code to restore from bit stream
// inverse array processing code
// ========================================================================

  status2 = dmap_filter_inv(a, &s) ;           // call next inverse filter
  errmsg = "restore filter chain failed" ;
  if(status2 < 0) goto fail ;
  status += status2 ;
  goto end ;

// encode filter parameters from *dpfl[0] into bit stream
encode:
  status = 0 ;
  s = *stream ;                        // local copy of stream control structure
  fprintf(stderr, "encode parameters : filter = %3.3o", arg->filter) ;
  arg = (FILTER_ARGS *) dpfl[0] ;    // parameters for this filter
  STREAM_PUT_NBITS(s, arg->filter , 8) ; status = 8 ;
// ========================================================================
  fprintf(stderr, "(%3.3o), status = %ld\n", arg->filter, status) ;
// local code
// ========================================================================
  goto end ;

// decode filter parameters from bit stream, copy into *dpfl[0]
decode:
  status = 0 ;
  s = *stream ;                        // local copy of stream control structure
  fprintf(stderr, "decode parameters, filter = %3.3o", arg->filter) ;
  STREAM_GET_NBITS(s, self, 8) ;
  errmsg = "decode parameters : self != FILTER_ID" ;
  if(self != FILTER_ID) goto fail ;                     // wrong id, MUST be FILTER_ID
  status = sizeof(FILTER_ARGS) ;
  arg = (FILTER_ARGS *) dpfl[0] ;                       // parameters for this filter
  arg->filter  = self ;
  status = sizeof(FILTER_ARGS) ;
// ========================================================================
  fprintf(stderr, "(%3.3o), status = %ld\n", self, status) ;
// local code
// ========================================================================
  goto end ;

// print filter parameters
print:
  arg = (FILTER_ARGS *) dpfl[0] ;                       // parameters for this filter
// ========================================================================
  fprintf(stderr, "[%3.3o] Demo Filter\n", arg->filter) ;
// local code
// ========================================================================
  return 0 ;
}
#undef FILTER_NAME
#undef FILTER_ARGS
#undef FILTER_ID
