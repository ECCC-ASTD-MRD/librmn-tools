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

#if ! defined(MAX_DMAPFILTERS)
#define MAX_DMAPFILTERS 128

#include <rmn/data_map.h>
#include <rmn/array_nd.h>

// code in pack/dmapfilter_nnn.c
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
typedef int (*dmapfilter_ptr)(zmap *map, int index, array_nd *array, dmapfilter_args *args) ;   // pointer to processing function


static inline int dmapfilter_invalid(dmapfilter_args *args, uint32_t expected){
  return (args->filter != expected) ;
}

#endif
