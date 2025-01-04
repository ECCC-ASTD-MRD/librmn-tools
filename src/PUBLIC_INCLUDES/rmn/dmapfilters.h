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

// processed subarray will be stored into memory described by zmap table entries mem[index] and size[index]
typedef int (*dmapfilter_ptr)(zmap *map, int index, array_nd *array, dmapfilter_args *args) ;   // pointer to processing function


static inline int dmapfilter_invalid(dmapfilter_args *args, uint32_t expected){
  return (args->filter != expected) ;
}

#endif
