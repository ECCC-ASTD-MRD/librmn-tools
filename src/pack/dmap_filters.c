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
//       before writing the information needed by the reverse filter
// this makes it easy for the inverse filter chain to be applied in reverse order
// in inverse (INV) mode
//    the start filter  (ID == 0)
//       reads array information from the bit stream
//       sets/checks the array
//       calls the filter chain
//    each filter in turn calls the next one until the end of the chain
//
// ================================= generic template for filters =================================
//
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
// check a->type and a->ndim as needed
//
// filter processing code goes here  (FWD)
//
  dpfl++ ;                                     // call next filter if there is one
  dmap_filter_ptr next_filter = dmap_filter_next(dpfl) ;
  status = (*next_filter)(a, bp, dpfl, &s) ;
  if(status < 0) goto fail ;
//
// insert into bitstream the appropriate information for the reverse filter (PUT)
//
  STREAM_PUT_NBITS(s, FILTER_ID, 8) ;
  status += 8 ;     // 8 bits inserted so far
//
  STREAM_INSERT_PUSH(s) ;
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
//   fprintf(stderr, "reverse filter %3.3o, id = %d\n", FILTER_ID, w32) ;
  if(w32 != FILTER_ID) goto fail ;                     // wrong id, MUST be FILTER_ID
//
// inverse filter processing code goes here  (INV)
//
  ssize_t status2 = dmap_filter_inv(a, &s) ;           // call next inverse filter
  if(status2 < 0) goto fail ; else status += status2 ;
  *stream = s ;                                        // SAVE stream changes
  return status ;
}
#undef FILTER_NAME
#undef FILTER_ARGS
#undef FILTER_ID

#endif      // COMPILE_FILTER_TEMPLATE_NEVER_TRUE

#include <stdlib.h>
#include <rmn/dmap_filters.h>

static int strict_mode = 0 ;
static int debug_mode = 1 ;

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
static ssize_t dmap_filter_none(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream){
  (void) (a) ;
  (void) (bp) ;
  (void) (stream) ;
  if(strict_mode > 1) return -1 ;
  if(debug_mode) fprintf(stderr, "UNDEFINED FILTER (%d)\n", dpfl[0]->filter) ;
  dpfl++ ;                              // next filter
  dmap_filter_ptr next_filter = dmap_filter_next(dpfl) ;
  if(next_filter != NULL) return (*next_filter)(a, bp, dpfl, stream) ;
  return 0 ;
}

// used to process invalid filters, does not interrupt filter chain
// behaves like a null filter, MUST NEVER be called as an inverse filter
static ssize_t dmap_filter_bad(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream){
  (void) (a) ;
  (void) (bp) ;
  (void) (stream) ;
  if(strict_mode) return -1 ;
  if(debug_mode) fprintf(stderr, "INVALID FILTER (%d)\n", dpfl[0]->filter) ;
  dpfl++ ;                              // next filter
  dmap_filter_ptr next_filter = dmap_filter_next(dpfl) ;
  if(next_filter != NULL) return (*next_filter)(a, bp, dpfl, stream) ;
  return 0 ;
}

// this filter terminates the filter chain
// insert FILTER_CHAIN_END marker into bit stream
static ssize_t dmap_filter_last(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream){
  (void) (a) ;
  (void) (bp) ;
  (void) (dpfl) ;
  (void) (stream) ;
  if(debug_mode) fprintf(stderr, "END of filter chain\n") ;
  return 0 ;
}

typedef struct{
  dmap_filter_ptr ptr ;
  char *name ;
} filter_properties ;

// table of filter addresses and names
// 3 extra entries at end, for internal dummy filters
static filter_properties filters[MAX_DP_FILTERS+3] = {
  { dmap_filter_fwd,  "array dimensions and type"  } ,   // filter 000 is a special filter, always present, hidden
  { dmap_filter_001,  "integer scale + offset"     } ,   // test filter
  { dmap_filter_002,  "integer flag"               } ,   // test filter
  { dmap_filter_003,  "float linear quantizer"     } ,
  { dmap_filter_004,  "Lorenzo predictor"          } ,
  { dmap_filter_005,  "integer wavelet transform"  } ,
  { dmap_filter_006,  "bit stream encoder"         } ,
  { dmap_filter_007,  "float scale + offset"       } ,
  { dmap_filter_010,  "filter_010"                 } ,
  { dmap_filter_011,  "filter_011"                 } ,
  { dmap_filter_012,  "filter_012"                 } ,
  { dmap_filter_013,  "filter_013"                 } ,
  { dmap_filter_014,  "filter_014"                 } ,
  { dmap_filter_015,  "filter_015"                 } ,
  { dmap_filter_016,  "filter_016"                 } ,
  { dmap_filter_017,  "filter_017"                 } ,
  { dmap_filter_020,  "filter_020"                 } ,
  { dmap_filter_021,  "filter_021"                 } ,
  { dmap_filter_022,  "filter_022"                 } ,
  { dmap_filter_023,  "filter_023"                 } ,
  { dmap_filter_024,  "filter_024"                 } ,
  { dmap_filter_025,  "filter_025"                 } ,
  { dmap_filter_026,  "filter_026"                 } ,
  { dmap_filter_027,  "filter_027"                 } ,
  { dmap_filter_030,  "filter_030"                 } ,
  { dmap_filter_031,  "filter_031"                 } ,
  { dmap_filter_032,  "filter_032"                 } ,
  { dmap_filter_033,  "filter_033"                 } ,
  { dmap_filter_034,  "filter_034"                 } ,
  { dmap_filter_035,  "filter_035"                 } ,
  { dmap_filter_036,  "filter_036"                 } ,
  { dmap_filter_037,  "filter_037"                 } ,
  { dmap_filter_none, "filter_none"                } ,
  { dmap_filter_bad,  "filter_bad"                 } ,
  { dmap_filter_last, "filter_last"                }
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

// ordinal [IN] : filter id
// return 1 if filter entry exists and filter is defined
int dmap_filter_exists(int ordinal){
  if(ordinal < 0 || ordinal >= MAX_DP_FILTERS+3) return 0 ;
  return (filters[ordinal].ptr != NULL) ;
}

// ordinal [IN] : filter id
// return name of filter having this id
char *dmap_filter_name(int ordinal){
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

int dmap_filter_set(dmap_filter_ptr filter, int ordinal, int force){
  if(ordinal < 0 || ordinal >= MAX_DP_FILTERS) return -1 ;  // invalid filter ordinal
  if(filters[ordinal].ptr == NULL){                  // filter not already defined
    filters[ordinal].ptr = filter ;                  // set to filter
  }else{
    if(force == 0) return -2 ;                              // filter already defined
    filters[ordinal].ptr = filter ;                  // override previous filter
  }
  return ordinal ;
}

// return address of next filter in list
dmap_filter_ptr dmap_filter_next(dmap_filter_list dpfl){
  if(*dpfl == NULL){
    return dmap_filter_last ;   // end of filter chain
  }
  int next_filter = dpfl[000]->filter ;
  return dmap_filter_get(next_filter) ;
}

// call next inverse dmap filter
ssize_t dmap_filter_inv(array_nd *a, bitstream *stream){
  uint32_t id ;
  ssize_t status ;
  STREAM_PEEK_NBITS(*stream, id, 8) ;
fprintf(stderr, "inv_next : next reverse filter id = %d\n", id) ;
  if(id == FILTER_CHAIN_END){
    STREAM_GET_NBITS(*stream, id, 8) ;
    status = 8 ;
fprintf(stderr, "inv_next : reverse filter id = %d, status = %ld\n", id, status) ;
    return status ;                                          // last filter, 8 bits processed
  }
  if(id >= MAX_DP_FILTERS) return -1 ;                  // ERROR, invalid filter id
  dmap_filter_ptr filter = filters[id].ptr ;      // get filter address
  if(filter == NULL) return -1 ;                        // ERROR, filter is not defined
  status = (*filter)(a, NULL, NULL, stream) ;           // call selected inverse filter
fprintf(stderr, "inv_next : reverse filter id = %d, status = %ld\n", id, status) ;
  return status ;
}

// get array dimension and type information from bit stream
// a         [IN] : array descriptor
// stream [INOUT] : bit stream
// allocate  [IN] : if nonzero, allocate spacefor data
// return number of bits extracted from bit stream (-1 if error)
int32_t dmap_filter_get_array_info(array_nd *a, bitstream *stream, int allocate){
  int i, nbits, ndim, type, dsize, iw32, gnn ;
  size_t sz = 1 ;
  uint32_t w32 ;
  STREAM_GET_NBITS(*stream, ndim,  3) ;            // number of dimensions
  if(a->ndim == 0) a->ndim = ndim ;
  if(a->ndim != ndim) goto fail ;                  // number of dimensions mismatch
  STREAM_GET_NBITS(*stream, dsize, 5) ; dsize++ ;  // number of bits needed for dimensions - 1
  STREAM_GET_NBITS(*stream, type,  8) ;            // data type
  if(a->type == 0){
    a->type = type ;
    a->esize = size_of_type[type] ;
  }
  if(size_of_type[type] != size_of_type[a->type]) goto fail ;  // type size mismatch
  nbits = 16 ;
  for(i=0 ; i<ndim ; i++){
    STREAM_GET_NBITS(*stream, w32, dsize) ; gnn = w32 ;
    if(a->dim[i].gnn == 0) a->dim[i].gnn = gnn ;
    if(a->dim[i].gnn != gnn) goto fail ;
    nbits += dsize ;
    sz *= gnn ;
  }
  if(allocate && a->data == NULL){  // allocate storage, fix dimensions
fprintf(stderr, "dmap_filter_get_array_info : calling fix_array\n");
    if(fix_array(a) == 0) goto fail ;
//     a->data = malloc(sz * a->esize) ;
//     if(a->data == NULL) goto fail ;
//     a->limit = a->data + (sz * a->esize) ;
  }
fprintf(stderr, "dmap_filter_get_array_info : array size = %ld, esize = %d\n", sz, a->esize) ;
  return nbits ;

fail:
fprintf(stderr, "dmap_filter_get_array_info : ERROR\n");
  return -1 ;
}

// put array dimension and type information into bit stream
// a      [INOUT] : array descriptor
// stream [INOUT] : bit stream
// return number of bits inserted into bi stream
int32_t dmap_filter_put_array_info(array_nd *a, bitstream *stream){
  int32_t ndim = a->ndim, i, dimmax = a->dim[0].gnn, dsize = 8, type = a->type, nbits = 0 ;
  for(i=1 ; i<ndim ; i++){ dimmax = (a->dim[i].gnn > dimmax) ? a->dim[i].gnn : dimmax ; }
  for(i=0 ; i<ndim ; i++){
    if(dimmax >     0xFF) dsize = 12 ;          // will need 12 bits for dimensions
    if(dimmax >    0xFFF) dsize = 16 ;          // will need 16 bits for dimensions
    if(dimmax >   0xFFFF) dsize = 24 ;          // will need 24 bits for dimensions
    if(dimmax > 0xFFFFFF) dsize = 32 ;          // will need 32 bits for dimensions
  }
  STREAM_PUT_NBITS(*stream, ndim,    3) ;          // number of dimensions
  STREAM_PUT_NBITS(*stream, dsize-1, 5) ;          // number of bits needed for dimensions - 1
  STREAM_PUT_NBITS(*stream, type,    8) ;          // data type
  nbits += 16 ;
  fprintf(stderr, "filter_head(IN), type = %s, ndim = %d, [", printable_type[type], ndim) ;
  for(i=0 ; i<ndim ; i++){
    STREAM_PUT_NBITS(*stream, a->dim[i].gnn, dsize) ;
    nbits += dsize ;
    fprintf(stderr, " %d", a->dim[i].gnn) ;
  }
  fprintf(stderr, "], dsize = %d, nbits = %d\n", dsize, nbits) ;
  return nbits ;
}
