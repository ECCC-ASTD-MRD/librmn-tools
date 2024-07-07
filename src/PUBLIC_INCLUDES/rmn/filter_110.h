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

#if ! defined(FILTER_110)
#define FILTER_110

// basic macros and definitions
#include <rmn/filter_base.h>
#include <rmn/pipe_filters.h>

// ----------------- id = 110 simpler packer -----------------
typedef struct{
  FILTER_PROLOG ;
} FILTER_TYPE(110) ;
static FILTER_TYPE(110) FILTER_NULL(110) = {FILTER_BASE(110) } ;
CT_ASSERT_(FILTER_SIZE_OK(FILTER_TYPE(110)))
pipe_filter FILTER_FUNCTION(110) ;

#endif
