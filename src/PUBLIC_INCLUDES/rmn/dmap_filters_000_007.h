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

// dmap filters 000 to 007

#pragma weak dmap_filter_000
dmap_filter  dmap_filter_000 ;
typedef struct{
  uint32_t filter ;  // filter number
  float offset ;
  float scale ;
} dmap_filter_arg_000 ;

#pragma weak dmap_filter_001
dmap_filter  dmap_filter_001 ;
typedef struct{
  uint32_t filter ;  // filter number
  int32_t offset ;
  int32_t scale ;
} dmap_filter_arg_001 ;

#pragma weak dmap_filter_002
dmap_filter  dmap_filter_002 ;
typedef struct{
  uint32_t filter ;  // filter number
  int32_t flag ;
} dmap_filter_arg_002 ;

#pragma weak dmap_filter_003
dmap_filter  dmap_filter_003 ;
typedef struct{
  uint32_t filter ;  // filter number
  uint8_t args[] ;
} dmap_filter_arg_003 ;

#pragma weak dmap_filter_004
dmap_filter  dmap_filter_004 ;
dmap_filter  dmap_filter_004 ;
typedef struct{
  uint32_t filter ;  // filter number
  uint8_t args[] ;
} dmap_filter_arg_004 ;

#pragma weak dmap_filter_005
dmap_filter  dmap_filter_005 ;
dmap_filter  dmap_filter_005 ;
typedef struct{
  uint32_t filter ;  // filter number
  uint8_t args[] ;
} dmap_filter_arg_005 ;

#pragma weak dmap_filter_006
dmap_filter  dmap_filter_006 ;
dmap_filter  dmap_filter_006 ;
typedef struct{
  uint32_t filter ;  // filter number
  uint8_t args[] ;
} dmap_filter_arg_006 ;

#pragma weak dmap_filter_007
dmap_filter  dmap_filter_007 ;
dmap_filter  dmap_filter_007 ;
typedef struct{
  uint32_t filter ;  // filter number
  uint8_t args[] ;
} dmap_filter_arg_007 ;
