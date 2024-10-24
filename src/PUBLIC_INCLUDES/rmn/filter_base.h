//
// Copyright (C) 2024  Environnement Canada
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
//     M. Valin,   Recherche en Prevision Numerique, 2024
//
// base macros for pipe filters
//
#if ! defined(FILTER_NULL)

#include <stdint.h>

// N.B. ID MUST BE a 3 character number e.g. 002 013 000 099 234, <= 255
#define FILTER_CONCAT2(str1,str2) str1 ## str2
#define FILTER_CONCAT3(str1,str2,str3) str1 ## str2 ## str3
// create filter ID type structs as typedefs
#define FILTER_TYPE(ID) FILTER_CONCAT2(filter_ , ID)
// create filter function name for filter type ID
#define FILTER_FUNCTION(ID) FILTER_CONCAT2(pipe_filter_ , ID)
// create filter null initializer name for filter type ID
#define FILTER_NULL(ID) FILTER_CONCAT3(filter_ , ID , _null)
// create contents of null initializer for filter type ID
// FILTER_CONCAT2(1,fid)-1000 avoids octal confusion problem in cases like fid = 009
#define FILTER_BASE(fid) .size = W32_SIZEOF(FILTER_TYPE(fid)), .id = (FILTER_CONCAT2(1,fid)-1000), .flags = 0

#if 0
// for a typical filter nnn

// filter_nnn.h file :

#include <rmn/filter_base.h>
#include <rmn/pipe_filters.h>
typedef struct{
  FILTER_PROLOG ;
//   specific filter contents
} FILTER_TYPE(nnn) ;
static FILTER_TYPE(nnn) FILTER_NULL(nnn) = {FILTER_BASE(nnn) } ;
pipe_filter FILTER_FUNCTION(nnn) ;

// filter_nnn.c file :

#include <rmn/filter_base.h>
#include <rmn/pipe_filters.h>

#define ID nnn
ssize_t FILTER_FUNCTION(ID)(uint32_t flags, array_descriptor *ap, const filter_meta *meta_in, pipe_buffer *buf, wordstream *stream_out){
// appropriate code
}
#endif

// first element of metadata for ALL filters (MUST be present and be the FIRST element)
// id    : filter ID (000 to 255)
// flags : local flags for this filter
// size  : size of the struct in 32 bit units (includes 32 bit prolog)
// meta0 : used for size or metadata (if size == 63 , size = meta0)
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define FILTER_PROLOG uint32_t id:8, flags:2, size:6, meta0:16
#endif
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define FILTER_PROLOG uint32_t meta0:16, size:6, flags:2, id:8
#endif

// check that size of filter struct is a multiple of 32 bits
#define FILTER_SIZE_OK(name) (W32_SIZEOF(name)*sizeof(uint32_t) == sizeof(name))

// generic filter metadata type, used for filter interface
typedef struct{             // generic type used with 32 bit based metadata,
  FILTER_PROLOG ;           // used for meta_out in forward mode
  uint32_t meta[] ;         // and meta_in in reverse mode.
} filter_meta_32 ;          // generic type used by the filter API

typedef struct{             // generic type used with 16 bit based metadata
  FILTER_PROLOG ;
  uint16_t meta[] ;
} filter_meta_16 ;

typedef struct{             // generic type used with byte based metadata
  FILTER_PROLOG ;
  uint8_t meta[] ;
} filter_meta_08 ;

typedef filter_meta_32 filter_meta ;  // default metadata

#endif
