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

// filter 100 specific include file

// ----------------- id = 100, linear quantizer -----------------

#if ! defined(FILTER_100)
#define FILTER_100

#include <rmn/filter_base.h>

typedef struct{
  FILTER_PROLOG ;
  float    ref ;
  uint32_t nbits : 5 ;
} FILTER_TYPE(100) ;
static FILTER_TYPE(100) filter_100_null = {FILTER_BASE(100), .ref = 0.0f, .nbits = 0 } ;
CT_ASSERT(FILTER_SIZE_OK(FILTER_TYPE(100)))

#endif
