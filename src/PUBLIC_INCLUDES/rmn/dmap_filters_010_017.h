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
#if ! defined(DMAP_FILTER_010)

#include <rmn/dmap_filters.h>

#pragma weak dmap_filter_010
dmap_filter  dmap_filter_010 ;
typedef struct{
  uint32_t filter ;  // filter number
  uint8_t args[] ;
} dmap_filter_arg_010 ;
#define DMAP_FILTER_010(...) (dmap_filter_arg_010) { 010 , __VA_ARGS__ }

#pragma weak dmap_filter_011
dmap_filter  dmap_filter_011 ;
typedef struct{
  uint32_t filter ;  // filter number
  uint8_t args[] ;
} dmap_filter_arg_011 ;
#define DMAP_FILTER_011(...) (dmap_filter_arg_010) { 011 , __VA_ARGS__ }

#pragma weak dmap_filter_012
dmap_filter  dmap_filter_012 ;
typedef struct{
  uint32_t filter ;  // filter number
  uint8_t args[] ;
} dmap_filter_arg_012 ;
#define DMAP_FILTER_012(...) (dmap_filter_arg_010) { 012 , __VA_ARGS__ }

#pragma weak dmap_filter_013
dmap_filter  dmap_filter_013 ;
typedef struct{
  uint32_t filter ;  // filter number
  uint8_t args[] ;
} dmap_filter_arg_013 ;
#define DMAP_FILTER_013(...) (dmap_filter_arg_010) { 013 , __VA_ARGS__ }

#pragma weak dmap_filter_014
dmap_filter  dmap_filter_014 ;
typedef struct{
  uint32_t filter ;  // filter number
  uint8_t args[] ;
} dmap_filter_arg_014 ;
#define DMAP_FILTER_014(...) (dmap_filter_arg_010) { 014 , __VA_ARGS__ }

#pragma weak dmap_filter_015
dmap_filter  dmap_filter_015 ;
typedef struct{
  uint32_t filter ;  // filter number
  uint8_t args[] ;
} dmap_filter_arg_015 ;
#define DMAP_FILTER_015(...) (dmap_filter_arg_010) { 015 , __VA_ARGS__ }

#pragma weak dmap_filter_016
dmap_filter  dmap_filter_016 ;
typedef struct{
  uint32_t filter ;  // filter number
  uint8_t args[] ;
} dmap_filter_arg_016 ;
#define DMAP_FILTER_016(...) (dmap_filter_arg_010) { 016 , __VA_ARGS__ }

#pragma weak dmap_filter_017
dmap_filter  dmap_filter_017 ;
typedef struct{
  uint32_t filter ;  // filter number
  uint8_t args[] ;
} dmap_filter_arg_017 ;
#define DMAP_FILTER_017(...) (dmap_filter_arg_010) { 017 , __VA_ARGS__ }

#endif
