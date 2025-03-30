// Hopefully useful code for C
// Copyright (C) 2024  Recherche en Prevision Numerique
//
// This code is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation,
// version 2.1 of the License.
//
// This code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// Author:
//     M. Valin,   Recherche en Prevision Numerique, 2024
//
// used by block movers and array_nd
//
#if ! defined(malloc_fn_args)

#include <stdint.h>

// expected data type codes
typedef enum {
  bad_data    = 0,     // invalid
  int_data    = 1,     // 32 bit signed integers
  uint_data   = 2,     // 32 bit unsigned integers
  float_data  = 3,     // 32 bit floats
  raw_data    = 4,     // any 32 bit items (block_properties likely to be meaningless)
  large_data  = 5,     // items use a multiple of 32 bits (block_properties are meaningless)
  any_data    = 6,     // unknown or unspecified
  long_data   = 7,     // 64 bit signed integers
  ulong_data  = 8,     // 64 bit unsigned integers
  double_data = 9      // 64 bit doubles
} data_kind ;

// generic 64 bit container
typedef union{
  double    d ;    // double
  int64_t   l ;    // long long signed integer
  uint64_t lu ;    // long long unsigned integer
  void     *p ;    // address
  int32_t   i ;    // signed integer
  uint32_t  u ;    // unsigned integer
  float     f ;    // float
} iuf64_t ;

// generic 32 bit container
typedef union{
  int32_t  i ;    // signed integer
  uint32_t u ;    // unsigned integer
  float    f ;    // float
} iuf32_t ;

// generic argument list
typedef struct{
  uint32_t maxargs ;    // max number of arguments
  uint32_t nargs ;      // number of arguments
  iuf64_t  args[] ;     // arguments ( [0] .. [nargs-1] )
} function_args ;       // function argument list

// allocate a generic argument list with room for at most nmax arguments
#define malloc_fn_args(arglist, nmax) { arglist = (function_args *) malloc(sizeof(function_args) + nmax * sizeof(iuf64_t)) ; arglist->maxargs = nmax ; }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static const char *printable_type[10] = { "INVALID", "INT_32", "UINT_32", "FLOAT", "RAW_32", "LARGE", "UNKNOWN" "INT_64", "UINT_64", "DOUBLE" } ;
#pragma GCC diagnostic pop

#endif
