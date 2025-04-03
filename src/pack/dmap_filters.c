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
static ssize_t dmap_filter_last(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream){
  (void) (a) ;
  (void) (bp) ;
  (void) (dpfl) ;
  (void) (stream) ;
  fprintf(stderr, "END of filter chain\n") ;
  return 0 ;
}

// insert END OF FILTER CHANI marker into bit stream
ssize_t dmap_filter_end(bitstream *stream){
  STREAM_PUT_NBITS(*stream, 255, 8) ;
  return 8 ;
}

// table containing " understandable" filter names
// 3 extra entries at end, for internal dummy filters
static char *dmap_filter_names[MAX_DP_FILTERS+3] = {
  "array dimensions and type",     // "filter_head",
  "integer scale + offset",        // "filter_001",
  "integer flag",                  // "filter_002",
  "float linear quantizer",        // "filter_003",
  "Lorenzo predictor",             // "filter_004",
  "integer wavelet transform",     // "filter_005",
  "bit stream encoder",            // "filter_006",
  "float scale + offset",          // "filter_007",
  "filter_010",
  "filter_011",
  "filter_012",
  "filter_013",
  "filter_014",
  "filter_015",
  "filter_016",
  "filter_017",
  "filter_020",
  "filter_021",
  "filter_022",
  "filter_023",
  "filter_024",
  "filter_025",
  "filter_026",
  "filter_027",
  "filter_030",
  "filter_031",
  "filter_032",
  "filter_033",
  "filter_034",
  "filter_035",
  "filter_036",
  "filter_037",
  "filter_none",    // dummy filter
  "filter_bad",     // dummy filter
  "filter_last"     // dummy filter
} ;

// table of filter addresses
// 3 extra entries at end, for internal dummy filters
static dmap_filter_ptr dmap_filter_table[MAX_DP_FILTERS+3] = {
  dmap_filter_head,
  dmap_filter_001,
  dmap_filter_002,
  dmap_filter_003,
  dmap_filter_004,
  dmap_filter_005,
  dmap_filter_006,
  dmap_filter_007,
  dmap_filter_010,
  dmap_filter_011,
  dmap_filter_012,
  dmap_filter_013,
  dmap_filter_014,
  dmap_filter_015,
  dmap_filter_016,
  dmap_filter_017,
  dmap_filter_020,
  dmap_filter_021,
  dmap_filter_022,
  dmap_filter_023,
  dmap_filter_024,
  dmap_filter_025,
  dmap_filter_026,
  dmap_filter_027,
  dmap_filter_030,
  dmap_filter_031,
  dmap_filter_032,
  dmap_filter_033,
  dmap_filter_034,
  dmap_filter_035,
  dmap_filter_036,
  dmap_filter_037,
  dmap_filter_none,    // dummy filter
  dmap_filter_bad,     // dummy filter
  dmap_filter_last     // dummy filter
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
  return (dmap_filter_table[ordinal] != NULL) ;
}

// ordinal [IN] : filter id
// return name of filter having this id
char *dmap_filter_name(int ordinal){
  if(ordinal < 0 || ordinal >= MAX_DP_FILTERS+3) return "filter_not_valid" ;
  if(dmap_filter_table[ordinal] == NULL) return "filter_not_defined" ;
  return dmap_filter_names[ordinal] ;
}

// ordinal [IN] : filter id
// return address of filter having this id
// if filter entry exists but is not defined, return dmap_filter_none
// if id is outside table bounds, return dmap_filter_bad
dmap_filter_ptr dmap_filter_get(int ordinal){
  if(ordinal < 0 || ordinal >= MAX_DP_FILTERS+3) return dmap_filter_bad ;
  if(dmap_filter_table[ordinal] == NULL)         return dmap_filter_none ;
  return dmap_filter_table[ordinal] ;
}

int dmap_filter_set(dmap_filter_ptr filter, int ordinal, int force){
  if(ordinal < 0 || ordinal >= MAX_DP_FILTERS) return -1 ;  // invalid filter ordinal
  if(dmap_filter_table[ordinal] == NULL){                  // filter not already defined
    dmap_filter_table[ordinal] = filter ;                  // set to filter
  }else{
    if(force == 0) return -2 ;                              // filter already defined
    dmap_filter_table[ordinal] = filter ;                  // override previous filter
  }
  return ordinal ;
}

// return address of next filter in list
dmap_filter_ptr dmap_filter_next(dmap_filter_list dpfl){
  if(*dpfl == NULL){
    return dmap_filter_last ;   // end of filter chain
//     return NULL ;
  }
  int next_filter = dpfl[000]->filter ;
  return dmap_filter_get(next_filter) ;
}
