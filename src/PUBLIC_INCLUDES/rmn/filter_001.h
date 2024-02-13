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

#if ! defined(FILTER_001)
#define FILTER_001

// basic macros and definitions
#include <rmn/filter_base.h>
#include <rmn/pipe_filters.h>

// ----------------- id = 001 array properties -----------------
// no specific input metadata is expected, PROLOG only
typedef struct{
  FILTER_PROLOG ;
} FILTER_TYPE(001) ;
static FILTER_TYPE(001) FILTER_NULL(001) = {FILTER_BASE(001) } ;
CT_ASSERT(FILTER_SIZE_OK(FILTER_TYPE(001)))
pipe_filter FILTER_FUNCTION(001) ;  // dummy filter (dimension encoding)

#endif
