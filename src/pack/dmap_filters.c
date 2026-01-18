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
//  filter chain quick description
//
//              fwd filter 0         filter 1           filter 2                 filter N
//   FWD------->+----------+       +----------+       +----------+             +----------+
//   (start)    |   PUT    |   +-->|          |   +-->|          |      +-..-->|          |
//              | (start)  |   |   |   FWD    |   |   |   FWD    |      |      |   FWD    |
//              |   call   |>--+   |   call   |>--+   |   call   |>....-+      |          |>-+------+
//   (end)      +==========+       +==========+       +==========+             +==========+  | LAST |
//   FWD<---+   |          |<--+   |          |<--+   |          |<-...-+      |          |<-+------+
//          |   |   PUT    |   |   |   PUT    |   |   |   PUT    |      |      |   PUT    |
//          +--<|  (end)   |   +--<|   call   |   +--<|   call   |      +-...-<|   call   |
//              +==========+       +==========+       +==========+             +==========+
//   INV------->+   GET    +   +-->|   GET    |   +-->|   GET    |      +-...->|   GET    |
//   (start)    |   INV    |   |   |   INV    |   |   |   INV    |      |      |   INV    |
//              |   call   |>--+   |   call   |>--+   |   call   |>....-+      |   call   |--> INV
//              +----------+       +----------+       +----------+             +----------+   (end)
//              inv filter 0
//    FWD  : apply forward filter
//    PUT  : write into bit stream
//    INV  : apply inverse filter
//    GET  : read from bit stream
//    LAST : specail last forward filter (turnaround)
//
// in forward (FWD) mode
//    the start filter (ID == 0)
//       writes array information into the bit stream
//       calls the next filter
//    the next filter is called 
//       after performing the filter action onto the array
//       before writing the information needed by the restore filter
// this makes it easy for the inverse filter chain to be applied in restore order
// in inverse (INV) mode
//    the start filter  (ID == 0)
//       reads array information from the bit stream
//       sets/checks the array
//       calls the filter chain
//    each filter in turn calls the next one until the end of the chain
//

#include <stdlib.h>
#include <rmn/dmap_filters.h>

// ================================= generic template for filters =================================
//
#undef COMPILE_FILTER_TEMPLATE
#define COMPILE_FILTER_TEMPLATE 1
#if defined(COMPILE_FILTER_TEMPLATE)
// filter template uses invalid ID 077
dmap_filter  dmap_filter_077 ;
typedef struct{
  uint32_t filter ;  // filter number
  uint8_t args[] ;
} dmap_filter_arg_077 ;
// FILTER_ID expected to be 000 -> 037 (Octal)
#define FILTER_ID 077
#define FILTER_NAME CAT(dmap_filter_,FILTER_ID)
#define FILTER_ARGS CAT(dmap_filter_arg_,FILTER_ID)
// for the restore filter, the data from the bit stream provides the necessary information
// the filter may modify the contents of the array described by a
// in filter mode, bp == NULL if no properties information is available
// the filter list MUST BE NULL TERMINATED
// a      [INOUT] : pointer to array descriptor, only used for DMAP_FILTER and DMAP_RESTORE command
// bp        [IN] : block properties, only used for DMAP_FILTER command
// dpfl      [IN] ; used for DMAP_FILTER, DMAP_ENCODE, DMAP_PRINT commands, ignored for DMAP_RESTORE command
//          [OUT] : for DMAP_DECODE command
// stream [INOUT] : ignored for DMAP_PRINT command, IN and OUT for all other commands
// command   [IN] : DMAP_FILTER | DMAP_RESTORE | DMAP_ENCODE | DMAP_DECODE | DMAP_PRINT
ssize_t FILTER_NAME(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream, dmap_command command){
  ssize_t status = 0, status2 = 0 ;
  uint32_t self/*, rank, type*/ ;
  char *errmsg = "" ;
  FILTER_ARGS *arg ;
  void *array ;
  bitstream s ;

// ==================== local variable declarations ====================
// local code
// ========================================================================
  if(command != DMAP_RESTORE){                       // DMAP_RESTORE does not use a parameter list
    errmsg = "\001dmap filter list is NULL" ;
    if(dpfl == NULL) goto fail ;
    arg = (FILTER_ARGS *) dpfl[0] ;                  // get parameters for this filter
    errmsg = "\002invalid/inconsistent filter ID" ;
    if(! dmap_filter_valid(dpfl,FILTER_ID)) goto fail ;   // not the expected filter ID
  }
  if(command != DMAP_PRINT){                         // DMAP_PRINT does not use the bit stream
    errmsg = "\003no stream" ;
    if(stream == NULL) goto fail ;                   // no bit stream
    s = *stream ;                                    // local copy of stream control structure
  }

  if(command == DMAP_ENCODE) goto encode ;
  if(command == DMAP_DECODE) goto decode ;
  if(command == DMAP_PRINT)  goto print ;
  if(command == DMAP_RESTORE || command == DMAP_FILTER){
    errmsg = "\004no array" ;
    if(a == NULL) goto fail ;
    array = array_address(a) ;                     // get array address, dimension(s), and type
    if(array == NULL) goto fail ;
//     rank = a->rank ;
//     type = a->type ;
    // check type and rank as/if needed
    // local code
    if(command == DMAP_RESTORE) goto restore ;
    goto forward ;
  }else{      // not DMAP_ENCODE, DMAP_DECODE, DMAP_PRINT, DMAP_RESTORE, DMAP_FILTER
    errmsg = "\005invalid command" ;
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
  errmsg = "\006filter chain failed" ;
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
  if(status < 0 || status2 < 0) dmap_filter_error(FILTER_ID, errmsg) ;
  if(status  < 0) return status ;
  if(status2 < 0) return status2 ;
  return dmap_filter_error(FILTER_ID, errmsg) ;     // DO NOT SAVE stream changes

// inverse of forward filter
restore:
// get the appropriate information for the restore filter from bitstream
  STREAM_GET_NBITS(s, self, 8) ; status = 8 ;          // 8 bits extracted so far
  errmsg = "\007inconsistent filter ID" ;
  if(self != FILTER_ID) goto fail ;                    // wrong id, MUST be FILTER_ID

// ====================  restore (INV) ====================
// local code to restore from bit stream
// inverse array processing code
// ========================================================================

  status2 = dmap_filter_inv(a, &s) ;           // call next inverse filter
  errmsg = "\010restore filter chain failed" ;
  if(status2 < 0) goto fail ;
  status += status2 ;
  goto end ;

// encode filter parameters from *dpfl[0] into bit stream
encode:
  status = 0 ;
  s = *stream ;                        // local copy of stream control structure
//   fprintf(stderr, "encode parameters : filter = %3.3o", arg->filter) ;
  arg = (FILTER_ARGS *) dpfl[0] ;    // parameters for this filter
  STREAM_PUT_NBITS(s, arg->filter , 8) ; status = 8 ;
// ========================================================================
//   fprintf(stderr, "(%3.3o), status = %ld\n", arg->filter, status) ;
// local code
// ========================================================================
  goto end ;

// decode filter parameters from bit stream, copy into *dpfl[0]
decode:
  status = 0 ;
  s = *stream ;                        // local copy of stream control structure
//   fprintf(stderr, "decode parameters, filter = %3.3o", arg->filter) ;
  STREAM_GET_NBITS(s, self, 8) ;
  errmsg = "\011decode parameters : self != FILTER_ID" ;
  if(self != FILTER_ID) goto fail ;                     // wrong id, MUST be FILTER_ID
  status = sizeof(FILTER_ARGS) ;
  arg = (FILTER_ARGS *) dpfl[0] ;                       // parameters for this filter
  arg->filter  = self ;
  status = sizeof(FILTER_ARGS) ;
// ========================================================================
//   fprintf(stderr, "(%3.3o), status = %ld\n", self, status) ;
// local code
// ========================================================================
  goto end ;

// print filter parameters
print:
  arg = (FILTER_ARGS *) dpfl[0] ;                       // parameters for this filter
// ========================================================================
//   fprintf(stderr, "[%3.3o] Demo Filter\n", self) ;
// local code
// ========================================================================
  return 0 ;
}
#undef FILTER_NAME
#undef FILTER_ARGS
#undef FILTER_ID

#endif      // COMPILE_FILTER_TEMPLATE_NEVER_TRUE
// ================================= end of filter template =================================

// workaround for a potential optimizer problem
// void do_nothing_with(void *what){
//   (void) (what) ;
//   return ;
// }

static int strict_mode = 0 ;
static int debug_mode = 0 ;

int dmap_strict_mode(int mode){
  int old_mode = strict_mode ;
  strict_mode = mode ;
  return old_mode ;
}

int dmap_debug_mode(int mode){
  int old_mode = debug_mode ;
  debug_mode = mode ;
  return old_mode ;
}

// used to process undefined filters, does not interrupt filter chain
// behaves like a null filter, MUST NEVER be called as an inverse filter
// if strict_mode is active, return ERROR
// a, bp, stream : passthrough filter arguments
// dpfl  [IN] : filter list
static ssize_t dmap_filter_none(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream, dmap_command command){
  (void) (a) ;
  (void) (bp) ;
  (void) (stream) ;
  if(strict_mode > 1) return -1 ;
  if(debug_mode) fprintf(stderr, "UNDEFINED FILTER (%d)\n", dpfl[0]->filter) ;
  dpfl++ ;                              // next filter
  dmap_filter_ptr next_filter = dmap_filter_next(dpfl) ;
  if(next_filter != NULL) return (*next_filter)(a, bp, dpfl, stream, command) ;
  return 0 ;
}

// used to process invalid filters, does not interrupt filter chain
// behaves like a null filter, MUST NEVER be called as an inverse filter
// if strict_mode is active,return  ERROR
// a, bp, stream : passthrough filter arguments
// dpfl  [IN] : filter list
static ssize_t dmap_filter_bad(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream, dmap_command command){
  (void) (a) ;
  (void) (bp) ;
  (void) (stream) ;
  if(strict_mode) return -1 ;
  if(debug_mode) fprintf(stderr, "INVALID FILTER (%d)\n", dpfl[0]->filter) ;
  dpfl++ ;                              // next filter
  dmap_filter_ptr next_filter = dmap_filter_next(dpfl) ;
  if(next_filter != NULL) return (*next_filter)(a, bp, dpfl, stream, command) ;
  return 0 ;
}

// this null filter terminates the filter chain (place holder)
// the FILTER_CHAIN_END marker will be inserted into the bit stream by dmap_filter_fwd (head of chain)
// a, bp, dpfl, stream : unused arguments, for compatibility with all other dmap filter arguments
static ssize_t dmap_filter_last(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream, dmap_command command){
  (void) (a) ;
  (void) (bp) ;
  (void) (dpfl) ;
  (void) (stream) ;
  (void) (command) ;
  if(debug_mode) fprintf(stderr, "END of filter chain\n") ;
  return 0 ;
}

typedef struct{
  dmap_filter_ptr ptr ;    // pointer to dmap filter function
  const char *name ;       // function description
  size_t sz ;              // argument list size
} filter_properties ;

// table of filter addresses and names
// 3 extra entries at end, for internal dummy filters
static filter_properties filters[MAX_DP_FILTERS+3] = {
  { dmap_filter_000,  "array dimensions and type"  , sizeof(dmap_filter_arg_000) } ,   // filter 000 is a special filter, always present, hidden
  { dmap_filter_001,  "int/fp offset and scale"    , sizeof(dmap_filter_arg_001) } ,   // test filter for now
  { dmap_filter_002,  "float pseudo log quantizer" , sizeof(dmap_filter_arg_002) } ,
  { dmap_filter_003,  "float quantizer"            , sizeof(dmap_filter_arg_003) } ,
  { dmap_filter_004,  "integer Lorenzo predictor"  , sizeof(dmap_filter_arg_004) } ,
  { dmap_filter_005,  "integer wavelet transform"  , sizeof(dmap_filter_arg_005) } ,
  { dmap_filter_006,  "bit stream encoder"         , sizeof(dmap_filter_arg_006) } ,
  { dmap_filter_007,  "compound filter"            , sizeof(dmap_filter_arg_007) } ,
  { dmap_filter_010,  "filter_010"                 , sizeof(dmap_filter_arg_000) } ,
  { dmap_filter_011,  "filter_011"                 , sizeof(dmap_filter_arg_000) } ,
  { dmap_filter_012,  "filter_012"                 , sizeof(dmap_filter_arg_000) } ,
  { dmap_filter_013,  "filter_013"                 , sizeof(dmap_filter_arg_000) } ,
  { dmap_filter_014,  "filter_014"                 , sizeof(dmap_filter_arg_000) } ,
  { dmap_filter_015,  "filter_015"                 , sizeof(dmap_filter_arg_000) } ,
  { dmap_filter_016,  "filter_016"                 , sizeof(dmap_filter_arg_000) } ,
  { dmap_filter_017,  "filter_017"                 , sizeof(dmap_filter_arg_000) } ,
  { dmap_filter_020,  "filter_020"                 , sizeof(dmap_filter_arg_000) } ,
  { dmap_filter_021,  "filter_021"                 , sizeof(dmap_filter_arg_000) } ,
  { dmap_filter_022,  "filter_022"                 , sizeof(dmap_filter_arg_000) } ,
  { dmap_filter_023,  "filter_023"                 , sizeof(dmap_filter_arg_000) } ,
  { dmap_filter_024,  "filter_024"                 , sizeof(dmap_filter_arg_000) } ,
  { dmap_filter_025,  "filter_025"                 , sizeof(dmap_filter_arg_000) } ,
  { dmap_filter_026,  "filter_026"                 , sizeof(dmap_filter_arg_000) } ,
  { dmap_filter_027,  "filter_027"                 , sizeof(dmap_filter_arg_000) } ,
  { dmap_filter_030,  "filter_030"                 , sizeof(dmap_filter_arg_000) } ,
  { dmap_filter_031,  "filter_031"                 , sizeof(dmap_filter_arg_000) } ,
  { dmap_filter_032,  "filter_032"                 , sizeof(dmap_filter_arg_000) } ,
  { dmap_filter_033,  "filter_033"                 , sizeof(dmap_filter_arg_000) } ,
  { dmap_filter_034,  "filter_034"                 , sizeof(dmap_filter_arg_000) } ,
  { dmap_filter_035,  "filter_035"                 , sizeof(dmap_filter_arg_000) } ,
  { dmap_filter_036,  "filter_036"                 , sizeof(dmap_filter_arg_000) } ,
  { dmap_filter_037,  "filter_037"                 , sizeof(dmap_filter_arg_000) } ,
  { dmap_filter_none, "filter_none"                , sizeof(dmap_filter_arg_000) } ,
  { dmap_filter_bad,  "filter_bad"                 , sizeof(dmap_filter_arg_000) } ,
  { dmap_filter_last, "filter_last"                , sizeof(dmap_filter_arg_000) }
} ;

// dpfl [IN] : pointer to argument list for a filter
// id   [IN] : id of requesting filter
// return 1 if argument list matches requested id, 0 if not
int dmap_filter_valid(dmap_filter_list dpfl, uint32_t id){
  if(dpfl == NULL) return 0 ;
  if(dpfl[0]->filter != id){           // 
    fprintf(stderr, "ERROR: bad filter reference, expecting %d, got %d\n", id, dpfl[0]->filter) ;
    return 0 ;
  }
  return 1 ;
}

// is this filter the last one in list ?
// dpfl [IN] : filter list
int dmap_filter_is_last(dmap_filter_list dpfl){
  if(dpfl == NULL) return 0 ;
  return (dpfl[1] == NULL) ;    // true if next list entry is NULL (no next filter)
}

// ordinal [IN] : filter id
// return expected argument size for this filter
size_t dmap_filter_argsize(int ordinal){
  if(ordinal < 0 || ordinal >= MAX_DP_FILTERS+3) return 0 ;
  return (filters[ordinal].sz) ;
}

// ordinal [IN] : filter id
// return 1 if filter entry exists and filter is defined
int dmap_filter_exists(int ordinal){
  if(ordinal < 0 || ordinal >= MAX_DP_FILTERS+3) return 0 ;
  return (filters[ordinal].ptr != NULL) ;
}

// ordinal [IN] : filter id
// return name of filter having this id
const char *dmap_filter_name(int ordinal){
  if(ordinal < 0 || ordinal >= MAX_DP_FILTERS+3) return "filter_not_valid" ;
  if(filters[ordinal].ptr == NULL) return "filter_not_defined" ;
  return filters[ordinal].name ;
}

// ordinal [IN] : filter id
// return address of filter having this id
// if filter entry exists but is not defined, return dmap_filter_none
// if id is outside table bounds, return dmap_filter_bad
dmap_filter_ptr dmap_filter_get(int ordinal){
  if(ordinal < 0 || ordinal >= MAX_DP_FILTERS+3) return dmap_filter_bad ;
  if(filters[ordinal].ptr == NULL)         return dmap_filter_none ;
  return filters[ordinal].ptr ;
}

// insert a filter address into the filter table at position ordinal
// filter  [IN] : address of dmap filter function
// ordinal [IN] : desired position in table
// name    [IN] : name of new filter (MUST BE STATIC)
// sz      [IN] : expected size of argument list
// force   [IN] : if force != 0, override table entry if it was non NULL
//                if force == 0, return error if table entry was non NULL
// return ordinal if O.K., negative error code in case of error
int dmap_filter_set(dmap_filter_ptr filter, int ordinal, const char *name, size_t sz, int force){
  if(ordinal < 0 || ordinal >= MAX_DP_FILTERS) return -1 ;  // invalid filter ordinal
  if(filters[ordinal].ptr == NULL){                  // filter not already defined
    filters[ordinal].ptr  = filter ;                 // set to filter
    filters[ordinal].sz   = sz ;                     // expected argument size
    filters[ordinal].name = name ;                   // filter name
  }else{
    if(force == 0) return -2 ;                       // filter already defined
    filters[ordinal].ptr  = filter ;                 // override previous filter
    filters[ordinal].sz   = sz ;                     // expected argument size
    filters[ordinal].name = name ;                   // filter name
  }
  return ordinal ;
}

// return address of next filter in list
// dpfl [IN] : filter list
dmap_filter_ptr dmap_filter_next(dmap_filter_list dpfl){
  if(*dpfl == NULL){
    return dmap_filter_last ;   // end of filter chain
  }
  int next_filter = dpfl[000]->filter ;
  return dmap_filter_get(next_filter) ;
}

// call next inverse dmap filter
// a      [INOUT] : pointer to array descriptor
// stream [INOUT] : pointer to bit stream
// return number of bits extracted from stream
ssize_t dmap_filter_inv(array_nd *a, bitstream *stream){
  uint32_t id ;
  ssize_t status ;

  STREAM_PEEK_NBITS(*stream, id, 8) ;                   // sniff next filter ID
  if(id == FILTER_CHAIN_END){                           // end of filter chain
    STREAM_GET_NBITS(*stream, id, 8) ;                  // get bits from stream
    status = 8 ;                                        // last filter, 8 bits processed
  }else{
    if(id >= MAX_DP_FILTERS) goto fail ;                // ERROR, invalid filter ID
    dmap_filter_ptr filter = filters[id].ptr ;          // get filter address for this ID
    if(filter == NULL) goto fail ;                      // ERROR, filter is not defined
    status = (*filter)(a, NULL, NULL, stream, DMAP_RESTORE) ;         // call selected inverse filter
  }
  return status ;

fail:
  return -1 ;
}

// get array dimension and type information from bit stream
// a         [IN] : array descriptor
// stream [INOUT] : bit stream
// allocate  [IN] : if nonzero, allocate spacefor data
// return number of bits extracted from bit stream (-1 if error)
// TODO : use BHW coding for dimensions et al ?
// TODO : collect rank, dimensions, type first, then check that array can be (re)configured properly
// TODO : add esize to the fray ?
// TODO : if allocate, allocate the data container, set dimensions (new_array_nd)
// TODO : otherwise check and potentially reallocate things
// get rank, type, element size, dimensions[rank] from bitstream
int32_t dmap_filter_get_array_info(array_nd *a, bitstream *stream, int allocate){
  char *msg = "" ;
  uint32_t rank, type, esize, nbits, i ;
  int32_t tbits, tdim[5] ;
  size_t mem_needed ;

  msg = "NULL array descriptor" ;
  if(a == NULL) goto fail ;
  msg = "NULL stream" ;
  if(stream == NULL) goto fail ;
  STREAM_GET_NBITS(*stream, rank, 3) ; nbits = 3 ;         // rank = number of dimensions (from stream)
  msg = "target max rank is too small" ;
  if(rank > a->ndim) goto fail ;

  STREAM_GET_NBITS(*stream, type, 8) ; nbits += 8 ;        // data type
  STREAM_GET_BHW(*stream, esize, tbits) ; nbits += tbits ; // element size
// fprintf(stderr, "get_array_info : rank = %d, type = %d, esize = %d, dimensions =", rank, type, esize) ;

  mem_needed = esize ;
  for(i = 0 ; i < 5 ; i++) tdim[i] = 1 ;                   // initialize all dimensions to 1
  for(i = 0 ; i < rank ; i++){                             // get dimensions from stream
    int32_t w32 ;
    STREAM_GET_BHW(*stream, w32, tbits) ;
    tdim[i] = w32 ;
    nbits += tbits ;
    mem_needed *= w32 ;
// fprintf(stderr, " %d", tdim[i]) ;
  }
// fprintf(stderr, "\n");
  if(a->type == any_data){                                 // set array description from stream data
// fprintf(stderr, "type set from %d to %d, esize set from %d to %d, rank set from %d to %d\n", a->type, type, a->esize, esize, a->rank, rank) ;
    a->type = type ;
    a->esize = esize ;
    a->rank = rank ;                                       // reset array rank
    for(i = 0 ; i < rank ; i++){                           // copy a->rank dimensions into array descriptor
      a->dim[i].gnn = tdim[i] ;
    } ;
  }
  if(allocate && a->data == NULL){
// fprintf(stderr, "calling fix_array\n");
    msg = "fix_array() failed" ;
    a->rank = rank ;                                       // reset array rank
    if(fix_array(a) == 0) goto fail ;
  }

  msg = "invalid rank" ;
  if(rank != a->rank) goto fail ;
  msg = "type mismatch" ;
  if(type != a->type) goto fail ;
  msg = "element size mismatch" ;
  if(esize != a->esize) goto fail ;
  msg = "dimension mismatch" ;
  for(i = 0 ; i < a->rank ; i++){
    if(a->dim[i].gnn != tdim[i]) goto fail ;
  }
  msg = "available memory too small" ;
  size_t mem_avail = a->limit - a->data ;                   // memory available in array
  if(mem_needed > mem_avail) goto fail ;
// fprintf(stderr, "dmap_filter_get_array_info : flags = %d\n", a->flags);

  return nbits ;
fail:
  fprintf(stderr, "dmap_filter_get_array_info : ERROR %s\n", msg);
  return -1 ;
}

// put array dimension and type information into bit stream
// a      [INOUT] : array descriptor
// stream [INOUT] : bit stream
// return number of bits inserted into bi stream
// put rank, type, element size, dimensions[rank] into bitstream
int32_t dmap_filter_put_array_info(array_nd *a, bitstream *stream){
  uint32_t rank = a->rank, type = a->type, esize = a->esize, nbits, i ;
  int32_t tbits ;
  STREAM_PUT_NBITS(*stream, rank, 3) ;    nbits = 3 ;      // number of dimensions
  STREAM_PUT_NBITS(*stream, type, 8) ;    nbits += 8 ;     // data type
  STREAM_PUT_BHW(*stream, esize, tbits) ; nbits += tbits ; // element size
// fprintf(stderr, "put_array_info : rank = %d, type = %d, esize = %d, dimensions =", rank, type, esize) ;
  for(i=0 ; i<rank ; i++){
    int32_t w32 = a->dim[i].gnn ;                          // dimension i
    STREAM_PUT_BHW(*stream, w32, tbits) ; nbits += tbits ;
// fprintf(stderr, " %d", w32) ;
  }
// fprintf(stderr, "\n");
  return nbits ;
}

// print filter list parameters
// return number of filters
int32_t dmap_print_parameters(dmap_filter_list dpfl){
  dmap_filter_args_ptr ptr = *dpfl ;
  int nfilters = 0 ;

  while(ptr != NULL){
    if(filters[ptr->filter].ptr == NULL && ptr->filter < MAX_DP_FILTERS){
      fprintf(stderr, "dmap_print_parameters : undefined filter = %d\n", ptr->filter) ;
    }else if(ptr->filter > MAX_DP_FILTERS){
      fprintf(stderr, "dmap_print_parameters : invalid filter = %d\n", ptr->filter) ;
    }else if(filters[ptr->filter].ptr == NULL){
      fprintf(stderr, "dmap_print_parameters : virtual filter = %d\n", ptr->filter) ;
    }else{
      nfilters++ ;
      dmap_filter_ptr filter = dmap_filter_get(ptr->filter) ;
      if((*filter)(NULL, NULL, dpfl, NULL, DMAP_PRINT) < 0) goto fail ;
    }
    dpfl++ ;
    ptr = *dpfl ;
  }

  return nfilters ;
fail:
  return -1 ;
}

// data pipe filter parameter encoder
// encode original filter list parameters into bit stream
// return number of bits written into stream
ssize_t dmap_encode_parameters(dmap_filter_list dpfl, bitstream *stream){
  dmap_filter_args_ptr ptr = *dpfl ;
  ssize_t status = 0 ;
  int nfilters = 0 ;
//   STREAM_PUT_NBITS(*stream, nfilters, 8) ;
//   fprintf(stderr, "dmap_encode_parameters : available %ld bits\n", StreamAvailableBits(stream)) ;
  while(ptr != NULL){
    if(ptr->filter > 0 && ptr->filter < MAX_DP_FILTERS+3){
      if(filters[ptr->filter].ptr == NULL || ptr->filter >= MAX_DP_FILTERS){
        fprintf(stderr, "dmap_encode_parameters : undefined filter = %d\n", ptr->filter) ;
      }else{
        nfilters++ ;
        dmap_filter_ptr filter = dmap_filter_get(ptr->filter) ;
        status += (*filter)(NULL, NULL, dpfl, stream, DMAP_ENCODE) ;
//         fprintf(stderr, "dmap_encode_parameters : defined filter = %d, status = %ld\n", ptr->filter, status) ;
      }
    }else{
      fprintf(stderr, "dmap_encode_parameters : invalid filter = %d\n", ptr->filter) ;
    }
    dpfl++ ;
    ptr = *dpfl ;
//     fprintf(stderr, "dmap_encode_parameters : status = %ld, available %ld bits\n", status, StreamAvailableBits(stream)) ;
  }
  STREAM_PUT_NBITS(*stream, 0xFF, 8) ; status += 8 ;
  STREAM_INSERT_ALIGN32(*stream) ;        // align to a 32 bit boundary
  STREAM_FLUSH(*stream) ;
  fprintf(stderr, "dmap_encode_parameters : status = %ld, available %ld bits, %d filters\n\n", status, StreamAvailableBits(stream), nfilters) ;
  return status ;
}

#define MAX_FILTERS MAX_DP_FILTERS
// data pipe filter parameter decoder
// decode original filter list parameters from bit stream
// dpfl     [OUT] : filter list
// nfilters  [IN] : size of filter list
// stream [INOUT] : stream to get filter parameters from
// return a properly set filter list
dmap_filter_list dmap_decode_parameters(bitstream *stream){
//   dmap_filter_args_ptr ptr = *dpfl ;
// TODO : allocate a large enough buffer for the filters and the filter list,
//        (pointers followed by data) populate it with DECODE calls
//        dpfl and nfilters probably not needed once done
  uint32_t filter_no = 0 ;
  int nf = 0 ;
  ssize_t status = 0 ;
  typedef struct{                              // accomodate large set of filters
    dmap_filter_args_ptr ptr[MAX_FILTERS] ;    // pointers into arg for filter arguments
    void          *nul ;                       // NULL pointer at end
    uint8_t       arg[MAX_FILTER_ARG_SIZE] ;   // max length argument data for filters
  } arg_list ;
  arg_list *list ;
  list = calloc(1, sizeof(arg_list)) ;
  dmap_filter_list dpfl = (dmap_filter_list) list ;
  uint8_t *ptr = (uint8_t *) (&(list->arg[0])) ;

  fprintf(stderr, "dmap_decode_parameters : available %ld bits, max filters = %d\n", StreamAvailableBits(stream), MAX_DP_FILTERS) ;
  STREAM_XTRACT_CHECK(*stream) ;
  STREAM_PEEK_NBITS(*stream, filter_no, 8) ;
  while(filter_no != 0xFF && nf < MAX_FILTERS){
    dmap_filter_ptr filter = dmap_filter_get(filter_no) ;
    // filters[filter_no] will provide the expected length of the argument list for this filter
    // or dmap_filter_argsize(filter_no)
    dpfl[0] = (dmap_filter_args_ptr) ptr ;
    dpfl[0]->filter = filter_no ;
    status = (*filter)(NULL, NULL, dpfl, stream, DMAP_DECODE) ;
    if(dpfl[0]->filter != filter_no) goto fail ;
    dpfl++ ;
    ptr += status ;
    nf++ ;
    STREAM_XTRACT_CHECK(*stream) ;
    STREAM_PEEK_NBITS(*stream, filter_no, 8) ;
  }
  STREAM_GET_NBITS(*stream, filter_no, 8) ; status += 8 ;
  fprintf(stderr, "dmap_decode_parameters : status = %ld, available %ld bits, nf = %d\n\n", status, StreamAvailableBits(stream), nf) ;

  return (dmap_filter_list) list ;   // return what can be used as a proper filter list

fail:
  fprintf(stderr, "dmap_decode_parameters : filter_no = %d, filter = %d\n", filter_no, dpfl[0]->filter) ;
  return NULL ;
}

