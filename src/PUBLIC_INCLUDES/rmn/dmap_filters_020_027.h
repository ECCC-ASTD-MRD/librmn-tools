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
#if ! defined(DMAP_FILTERS_020_027)
#define DMAP_FILTERS_020_027

#include <rmn/dmap_filters.h>

#pragma weak dmap_filter_020
dmap_filter  dmap_filter_020 ;
typedef struct{
  uint32_t filter ;  // filter number
  uint8_t args[] ;
} dmap_filter_arg_020 ;
#define DMAP_FILTER_020(...) (dmap_filter_arg_001) { 020 , __VA_ARGS__ }

#pragma weak dmap_filter_021
dmap_filter  dmap_filter_021 ;
typedef struct{
  uint32_t filter ;  // filter number
  uint8_t args[] ;
} dmap_filter_arg_021 ;
#define DMAP_FILTER_021(...) (dmap_filter_arg_001) { 021 , __VA_ARGS__ }

#pragma weak dmap_filter_022
dmap_filter  dmap_filter_022 ;
typedef struct{
  uint32_t filter ;  // filter number
  uint8_t args[] ;
} dmap_filter_arg_022 ;
#define DMAP_FILTER_022(...) (dmap_filter_arg_001) { 022 , __VA_ARGS__ }

#pragma weak dmap_filter_023
dmap_filter  dmap_filter_023 ;
typedef struct{
  uint32_t filter ;  // filter number
  uint8_t args[] ;
} dmap_filter_arg_023 ;
#define DMAP_FILTER_023(...) (dmap_filter_arg_001) { 023 , __VA_ARGS__ }

#pragma weak dmap_filter_024
dmap_filter  dmap_filter_024 ;
typedef struct{
  uint32_t filter ;  // filter number
  uint8_t args[] ;
} dmap_filter_arg_024 ;
#define DMAP_FILTER_024(...) (dmap_filter_arg_001) { 024 , __VA_ARGS__ }

#pragma weak dmap_filter_025
dmap_filter  dmap_filter_025 ;
typedef struct{
  uint32_t filter ;  // filter number
  uint8_t args[] ;
} dmap_filter_arg_025 ;
#define DMAP_FILTER_025(...) (dmap_filter_arg_001) { 025 , __VA_ARGS__ }

#pragma weak dmap_filter_026
dmap_filter  dmap_filter_026 ;
typedef struct{
  uint32_t filter ;  // filter number
  uint8_t args[] ;
} dmap_filter_arg_026 ;
#define DMAP_FILTER_026(...) (dmap_filter_arg_001) { 026 , __VA_ARGS__ }

#pragma weak dmap_filter_027
dmap_filter  dmap_filter_027 ;
typedef struct{
  uint32_t filter ;  // filter number
  uint8_t args[] ;
} dmap_filter_arg_027 ;
#define DMAP_FILTER_027(...) (dmap_filter_arg_001) { 027 , __VA_ARGS__ }

#endif
