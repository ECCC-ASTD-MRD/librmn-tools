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
#if ! defined(DMAP_FILTER_030)

#include <rmn/dmap_filters.h>

#pragma weak dmap_filter_030
dmap_filter  dmap_filter_030 ;
typedef struct{
  uint32_t filter ;  // filter number
  uint8_t args[] ;
} dmap_filter_arg_030 ;
#define DMAP_FILTER_030(...) (dmap_filter_arg_030) { 030 , __VA_ARGS__ }

#pragma weak dmap_filter_031
dmap_filter  dmap_filter_031 ;
typedef struct{
  uint32_t filter ;  // filter number
  uint8_t args[] ;
} dmap_filter_arg_031 ;
#define DMAP_FILTER_031(...) (dmap_filter_arg_031) { 031 , __VA_ARGS__ }

#pragma weak dmap_filter_032
dmap_filter  dmap_filter_032 ;
typedef struct{
  uint32_t filter ;  // filter number
  uint8_t args[] ;
} dmap_filter_arg_032 ;
#define DMAP_FILTER_032(...) (dmap_filter_arg_032) { 032 , __VA_ARGS__ }

#pragma weak dmap_filter_033
dmap_filter  dmap_filter_033 ;
typedef struct{
  uint32_t filter ;  // filter number
  uint8_t args[] ;
} dmap_filter_arg_033 ;
#define DMAP_FILTER_033(...) (dmap_filter_arg_033) { 033 , __VA_ARGS__ }

#pragma weak dmap_filter_034
dmap_filter  dmap_filter_034 ;
typedef struct{
  uint32_t filter ;  // filter number
  uint8_t args[] ;
} dmap_filter_arg_034 ;
#define DMAP_FILTER_034(...) (dmap_filter_arg_034) { 034 , __VA_ARGS__ }

#pragma weak dmap_filter_035
dmap_filter  dmap_filter_035 ;
typedef struct{
  uint32_t filter ;  // filter number
  uint8_t args[] ;
} dmap_filter_arg_035 ;
#define DMAP_FILTER_035(...) (dmap_filter_arg_035) { 035 , __VA_ARGS__ }

#pragma weak dmap_filter_036
dmap_filter  dmap_filter_036 ;
typedef struct{
  uint32_t filter ;  // filter number
  uint8_t args[] ;
} dmap_filter_arg_036 ;
#define DMAP_FILTER_036(...) (dmap_filter_arg_036) { 036 , __VA_ARGS__ }

#pragma weak dmap_filter_037
dmap_filter  dmap_filter_037 ;
typedef struct{
  uint32_t filter ;  // filter number
  uint8_t args[] ;
} dmap_filter_arg_037 ;
#define DMAP_FILTER_037(...) (dmap_filter_arg_037) { 037 , __VA_ARGS__ }

#endif
