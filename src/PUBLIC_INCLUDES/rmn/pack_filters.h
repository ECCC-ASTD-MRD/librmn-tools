//
// Copyright (C) 2023  Environnement Canada
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
//     M. Valin,   Recherche en Prevision Numerique, 2023
//

#include <stdint.h>
#include <unistd.h>

#include <rmn/ieee_quantize.h>
#include <rmn/tools_types.h>
#include <rmn/ct_assert.h>

typedef uint64_t pack_flags ;
#define FFLAG_FORWARD     1
#define FFLAG_REVERSE     2
#define FFLAG_INPLACE     4
#define FFLAG_REALLOC     8
#define FFLAG_NOFREE     16
#define FFLAG_VALIDATE   32

#define PIPE_DATA_08     (1 << 16)
#define PIPE_DATA_16     (2 << 16)
#define PIPE_DATA_32     (4 << 16)
#define PIPE_DATA_64     (8 << 16)
#define PIPE_DATA_SIZE(dims) ( (((dims)[0] >> 16) == 0) ? 4 : ((dims)[0] >> 16) )
#define PIPE_DATA_NDIMS(dims)  ((dims)[0] & 0x7)

// #define FFLAG_NI(x) (((x) & 0xFFF) << 12)
// #define FPACK_NI(f) (((f) >> 12) & 0xFFF)
// 
// #define FFLAG_NJ(x) (((x) & 0xFFF))
// #define FPACK_NJ(f) (((f)      ) & 0xFFF)
// 
// #define FFLAG_ID(x) (((x) & 0x0FF) << 24)
// #define FPACK_ID(f) (((f) >> 24) & 0x0FF)

typedef struct{
  size_t used ;            // number of bytes used in buffer
  size_t max_size ;        // buffer maximum size in bytes
  void *buffer ;           // pointer to storage for buffer
  uint32_t flags ;         // usage flags for buffer
} pipe_buffer ;

// typedef struct{
//   uint32_t id    : 8 ,     // filter ID (for consistency checks)
//            nmeta :24 ;     // number of values in meta[]
//   uint32_t resv  : 8 ,     // reserved, should be 0
//            size  :24 ;     // max size of meta[]
//   AnyType32  meta[]  ;     // any 32 bit type, likely .f, .i, .u
// } pack_meta ;
// CT_ASSERT(sizeof(pack_meta) == sizeof(uint64_t))

// first element of metadata for ALL filters
// id    : filter ID
// size  : max size allowed in 32 bit units
// used  : used space in 32 bit units
// flags : local flags for this filter
#define FILTER_PROLOG uint32_t id:8, size:8, used:8, flags:8 ;

// check that size of filter struct is a multiple of 32 bits
#define FILTER_SIZE_OK(name) (sizeof(filter_001)/sizeof(uint32_t)*sizeof(uint32_t) == sizeof(filter_001))

typedef struct{             // generic filter metadata
  FILTER_PROLOG ;
  AnyType32 meta[] ;
} filter_meta ;

typedef struct{
  int nfilters ;
  filter_meta *meta[] ;
} filter_list ;

typedef struct{             // id = 001, linear quantizer
  FILTER_PROLOG ;
  float    ref ;
  uint32_t nbits : 5 ;
} filter_001 ;
static filter_001 filter_001_null = {.size = sizeof(filter_001)/sizeof(uint32_t), .id = 1, .used = 0, .ref = 0.0f, .nbits = 0 } ;
CT_ASSERT(FILTER_SIZE_OK(filter_001))

typedef struct{             // id = 254, scale and offset test filter
  FILTER_PROLOG ;
  int32_t factor ;
  int32_t offset ;
} filter_254 ;
static filter_254 filter_254_null = {.size = sizeof(filter_254)/sizeof(uint32_t), .id = 254, .used = 0, .factor = 1, .offset = 0 } ;
CT_ASSERT(FILTER_SIZE_OK(filter_254))

#define BASE_META_SIZE (sizeof(pack_meta)/sizeof(uint32_t))
// to allocate space for NM meta elements : uint32_t xxx[BASE_META_SIZE + NM]
//                                          yyy = malloc((sizeof(uint32_t) * (BASE_META_SIZE + NM))
// then use (pack_meta) xxx or (pack_meta *) yyy as argument to filters

// flags     [IN] : flags passed to filter
// dims      [IN] : dimensions for array in buffer. (some filters will consider data as 1D anyway)
//                  dims[0] : number of dimensions + DATA_08|DATA_16|DATA_32|DATA_64
//                  dims[1->dims[0]] : dimensions
// buffer    [IN] : input data to filter
//                  buffer->used      : number of valid bytes in buffer->buffer
//                  buffer->max_size  : available storage size in buffer->buffer
//          [OUT] : output from filter, same address as before if operation done in place
//                  buffer->used     : number of valid bytes in buffer->buffer
//                  buffer->max_size : available storage size in buffer->buffer
// meta_in   [IN] : parameters passed to filter
// meta_out [OUT] : (REVERSE direction) ignored
//          [OUT] : (FORWARD direction) parameters that must be passed for the REVERSE call
// the function returns the size of its data output in bytes
typedef ssize_t pipe_filter(uint32_t flags, int *dims, filter_meta *meta_in, pipe_buffer *buffer, filter_meta *meta_out) ;
typedef pipe_filter *pipe_filter_pointer ;

pipe_filter pipe_filter_001 ;
pipe_filter pipe_filter_254 ;
