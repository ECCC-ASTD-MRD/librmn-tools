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

// filter specific include file template

#if ! defined(FILTER_255)
#define FILTER_255 2

// basic macros and definitions
#include <rmn/filter_base.h>
#include <rmn/pipe_filters.h>

// ----------------- id = 255 filter template -----------------
typedef struct{
  FILTER_PROLOG ;
  uint32_t opt[FILTER_255] ;
} FILTER_TYPE(255) ;
static FILTER_TYPE(255) FILTER_NULL(255) = {FILTER_BASE(255) } ;
CT_ASSERT(FILTER_SIZE_OK(FILTER_TYPE(255)))
pipe_filter FILTER_FUNCTION(255) ;  // dummy filter (dimension encoding)

#endif
