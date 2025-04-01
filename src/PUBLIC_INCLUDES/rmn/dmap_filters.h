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

#if ! defined(MAX_DP_FILTERS)
#define MAX_DP_FILTERS 32

#include <rmn/cpp_extras.h>
#include <rmn/data_map.h>
#include <rmn/array_nd.h>
#include <rmn/be_stream.h>
#include <rmn/bitstream.h>

// allocate an argument list with room for at most nmax arguments
typedef struct{
  uint32_t filter ;  // filter number
  uint32_t nargs ;   // number of arguments
  union{
    double    d ;    // 64 bit double
    void     *p ;    // address
    int64_t   l ;    // 64 bit signed integer
    uint64_t lu ;    // 64 bit unsigned integer
    int32_t   i ;    // 32 bit signed integer
    uint32_t  u ;    // 32 bit unsigned integer
    float     f ;    // 32 bit float
  } args[] ;         // arguments ( [0] .. [nargs-1] )
} dmapfilter_args ;  // processing function argument list

// forward filter control structure template
// generic data pipe filter arguments
typedef struct{
  uint32_t filter ;  // filter number
  uint8_t args[] ;
} dmap_filter_args ;

// pointer to data pipe filter arguments
typedef dmap_filter_args *dmap_filter_args_ptr ;

// list of pointers to dmap_filter_args structures (NULL TERMINATED)
typedef  dmap_filter_args_ptr *dmap_filter_list ;

// generic data pipe filter function
typedef ssize_t dmap_filter(array_nd *, block_properties *, dmap_filter_list, bitstream *) ;
// generic pointer to data pipe filter function
typedef dmap_filter *dmap_filter_ptr ;

#include <rmn/dmap_filters_000_007.h>

#pragma weak dmap_filter_010
dmap_filter  dmap_filter_010 ;
#pragma weak dmap_filter_011
dmap_filter  dmap_filter_011 ;
#pragma weak dmap_filter_012
dmap_filter  dmap_filter_012 ;
#pragma weak dmap_filter_013
dmap_filter  dmap_filter_013 ;
#pragma weak dmap_filter_014
dmap_filter  dmap_filter_014 ;
#pragma weak dmap_filter_015
dmap_filter  dmap_filter_015 ;
#pragma weak dmap_filter_016
dmap_filter  dmap_filter_016 ;
#pragma weak dmap_filter_017
dmap_filter  dmap_filter_017 ;

#pragma weak dmap_filter_020
dmap_filter  dmap_filter_020 ;
#pragma weak dmap_filter_021
dmap_filter  dmap_filter_021 ;
#pragma weak dmap_filter_022
dmap_filter  dmap_filter_022 ;
#pragma weak dmap_filter_023
dmap_filter  dmap_filter_023 ;
#pragma weak dmap_filter_024
dmap_filter  dmap_filter_024 ;
#pragma weak dmap_filter_025
dmap_filter  dmap_filter_025 ;
#pragma weak dmap_filter_026
dmap_filter  dmap_filter_026 ;
#pragma weak dmap_filter_027
dmap_filter  dmap_filter_027 ;

#pragma weak dmap_filter_030
dmap_filter  dmap_filter_030 ;
#pragma weak dmap_filter_031
dmap_filter  dmap_filter_031 ;
#pragma weak dmap_filter_032
dmap_filter  dmap_filter_032 ;
#pragma weak dmap_filter_033
dmap_filter  dmap_filter_033 ;
#pragma weak dmap_filter_034
dmap_filter  dmap_filter_034 ;
#pragma weak dmap_filter_035
dmap_filter  dmap_filter_035 ;
#pragma weak dmap_filter_036
dmap_filter  dmap_filter_036 ;
#pragma weak dmap_filter_037
dmap_filter  dmap_filter_037 ;

// list of pointers to dmapfilter_args structures
typedef struct{
  uint32_t nfilters ;
  dmapfilter_args *filters[] ;
} dmapfilter_list ;

// bit stream encoding format for filter metadata
// first element of metadata for ALL filters (MUST be present and be the FIRST element)
// id    : filter ID (000 -> 255)
// flags : local flags for this filter (0 -> 15)
// size  : size of the struct in 32 bit units (excluding prolog) (0 -> 30)
// meta0 : used for size or metadata. if size == 31 , size = meta0 (0 -> 65535)
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define DMAPFILTER_PROLOG uint16_t id:7, flags:4, size:5 ; uint16_t meta0 ;
#endif
// #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
// #define DMAPFILTER_PROLOG uint32_t meta0:16, size:5, flags:4, id:7
// #endif

// generic datamap filter metadata type, used for bit stream encoding
// datamap filter metadata encoding used for
// metadata output in forward mode and metadata input in reverse mode
typedef struct{         // generic type used by the filter API
  DMAPFILTER_PROLOG
  uint32_t m32[] ;
} dmapfilter_meta ;

typedef struct{         // 16 bit metadata
  DMAPFILTER_PROLOG
  uint16_t m16[] ;
} dmapfilter_m16 ;

static const dmapfilter_meta dmeta_000 = { .id = 0, .flags = 0, .size = sizeof(dmapfilter_meta), .meta0 = 0 } ;

// bit stream encoding of a datamap block :
// number of filters (8 bits)
// metadata for filter 1 (n x 32 bits)
// .....
// metadata for filter n (last filter) (n x 32 bits)
// encoded data stream

// a float block would use a sequence like
// quantization filter | prediction filter | encoding filter

// processed subarray will be stored into memory described by zmap table entries mem[index] and size[index]
// typedef int (*dmapfilter_ptr)(zmap *map, int index, array_nd *array, dmapfilter_args *args) ;   // pointer to processing function


// static inline int dmapfilter_invalid(dmapfilter_args *args, uint32_t expected){
//   return (args->filter != expected) ;
// }

dmap_filter_ptr dmap_filter_get(int ordinal);
int dmap_filter_set(dmap_filter_ptr filter, int ordinal, int force);
int dmap_filter_exists(int ordinal);
char *dmap_filter_name(int ordinal);

// ssize_t dmap_filter_bad(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream);
// ssize_t dmap_filter_none(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream);
dmap_filter_ptr dmap_filter_next(dmap_filter_list dpfl);
int dmap_filter_valid(dmap_filter_list dpfl, uint32_t id);


#endif
