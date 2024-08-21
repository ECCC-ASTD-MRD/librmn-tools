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

#define FILTER_CONCAT2(str1,str2) str1 ## str2
#define FILTER_CONCAT3(str1,str2,str3) str1 ## str2 ## str3
#define FILTER_TYPE(id) FILTER_CONCAT2(filter_ , id)
#define FILTER_FUNCTION(id) FILTER_CONCAT2(pipe_filter_ , id)
#define FILTER_NULL(id) FILTER_CONCAT3(filter_ , id , _null)
// FILTER_CONCAT2(1,fid)-1000 avoids octal confusion problem in cases like fid = 009
#define FILTER_BASE(fid) .size = W32_SIZEOF(FILTER_TYPE(fid)), .id = (FILTER_CONCAT2(1,fid)-1000), .flags = 0

// first element of metadata for ALL filters
// id    : filter ID
// size  : size of the struct in 32 bit units (includes 32 bit prolog)
// meta0 : used for size or metadata_in_prolog (if size == 63 , size = meta0)
// flags : local flags for this filter
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define FILTER_PROLOG uint32_t id:8, flags:2, size:6, meta0:16
#endif
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define FILTER_PROLOG uint32_t meta0:16, size:6, flags:2, id:8
#endif

// check that size of filter struct is a multiple of 32 bits
#define FILTER_SIZE_OK(name) (W32_SIZEOF(name)*sizeof(uint32_t) == sizeof(name))

// generic filter metadata type, used for filter interface
typedef struct{             // generic filter with metadata
  FILTER_PROLOG ;           // used for meta_out in forward mode
  uint32_t meta[] ;         // and meta_in in reverse mode
} filter_meta ;             // type used by the filter API

#endif
