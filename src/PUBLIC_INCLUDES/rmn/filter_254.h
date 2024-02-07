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

// filter 254 specific include file

// ----------------- id = 254, scale and offset filter -----------------

#if ! defined(FILTER_254)
#define FILTER_254

#include <rmn/filter_base.h>

typedef struct{
  FILTER_PROLOG ;
  int32_t factor ;
  int32_t offset ;
} FILTER_TYPE(254) ;
static FILTER_TYPE(254) filter_254_null = {FILTER_BASE(254), .factor = 0, .offset = 0 } ;
CT_ASSERT(FILTER_SIZE_OK(FILTER_TYPE(254)))

#endif
