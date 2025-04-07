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

#include <stdlib.h>
#include <rmn/dmap_filters.h>

// used to process undefined filters, does not interrupt filter chain
// behaves like a null filter, MUST NEVER be called as an inverse filter
static ssize_t dmap_filter_none(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream){
  (void) (a) ;
  (void) (bp) ;
  (void) (stream) ;
  fprintf(stderr, "UNDEFINED FILTER (%d)\n", dpfl[0]->filter) ;
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
  fprintf(stderr, "INVALID FILTER (%d)\n", dpfl[0]->filter) ;
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
//   (void) (stream) ;
  STREAM_PUT_NBITS(*stream, FILTER_CHAIN_END, 8) ;
  fprintf(stderr, "END of filter chain\n") ;
  return 8 ;
}

typedef struct{
  dmap_filter_ptr ptr ;
  char *name ;
} filter_properties ;

// table of filter addresses and names
// 3 extra entries at end, for internal dummy filters
static filter_properties filters[MAX_DP_FILTERS+3] = {
  { dmap_filter_fwd,   "array dimensions and type"  } ,
  { dmap_filter_001,   "integer scale + offset"     } ,
  { dmap_filter_002,   "integer flag"               } ,
  { dmap_filter_003,   "float linear quantizer"     } ,
  { dmap_filter_004,   "Lorenzo predictor"          } ,
  { dmap_filter_005,   "integer wavelet transform"  } ,
  { dmap_filter_006,   "bit stream encoder"         } ,
  { dmap_filter_007,   "float scale + offset"       } ,
  { dmap_filter_010,   "filter_010"                 } ,
  { dmap_filter_011,   "filter_011"                 } ,
  { dmap_filter_012,   "filter_012"                 } ,
  { dmap_filter_013,   "filter_013"                 } ,
  { dmap_filter_014,   "filter_014"                 } ,
  { dmap_filter_015,   "filter_015"                 } ,
  { dmap_filter_016,   "filter_016"                 } ,
  { dmap_filter_017,   "filter_017"                 } ,
  { dmap_filter_020,   "filter_020"                 } ,
  { dmap_filter_021,   "filter_021"                 } ,
  { dmap_filter_022,   "filter_022"                 } ,
  { dmap_filter_023,   "filter_023"                 } ,
  { dmap_filter_024,   "filter_024"                 } ,
  { dmap_filter_025,   "filter_025"                 } ,
  { dmap_filter_026,   "filter_026"                 } ,
  { dmap_filter_027,   "filter_027"                 } ,
  { dmap_filter_030,   "filter_030"                 } ,
  { dmap_filter_031,   "filter_031"                 } ,
  { dmap_filter_032,   "filter_032"                 } ,
  { dmap_filter_033,   "filter_033"                 } ,
  { dmap_filter_034,   "filter_034"                 } ,
  { dmap_filter_035,   "filter_035"                 } ,
  { dmap_filter_036,   "filter_036"                 } ,
  { dmap_filter_037,   "filter_037"                 } ,
  { dmap_filter_none,  "filter_none"                } ,
  { dmap_filter_bad,   "filter_bad"                 } ,
  { dmap_filter_last,  "filter_last"                }
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
  if(id == FILTER_CHAIN_END){
    STREAM_GET_NBITS(*stream, id, 8) ;
    status = 8 ;
// fprintf(stderr, "inv_next : reverse filter id = %d, status = %ld\n", id, status) ;
    return status ;                                          // last filter, 8 bits processed
  }
  if(id >= MAX_DP_FILTERS) return -1 ;                  // ERROR, invalid filter id
  dmap_filter_ptr filter = filters[id].ptr ;      // get filter address
  if(filter == NULL) return -1 ;                        // ERROR, filter is not defined
  status = (*filter)(a, NULL, NULL, stream) ;           // call selected inverse filter
// fprintf(stderr, "inv_next : reverse filter id = %d, status = %ld\n", id, status) ;
  return status ;
}
