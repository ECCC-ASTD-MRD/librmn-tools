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

#if ! defined(DMAP_FILTERS_000_007)
#define DMAP_FILTERS_000_007

// filter 000 is a special case, it MUST be present
// there is no argument struct associated with it
dmap_filter  dmap_filter_fwd ;

#pragma weak dmap_filter_001
dmap_filter  dmap_filter_001 ;
typedef struct{
  uint32_t filter ;  // filter number
  int32_t offset ;
  int32_t scale ;
} dmap_filter_arg_001 ;
#define DMAP_FILTER_001(...) (dmap_filter_arg_001) { 001 __VA_OPT__(,) __VA_ARGS__ }

#pragma weak dmap_filter_002
dmap_filter  dmap_filter_002 ;
typedef struct{
  uint32_t filter ;  // filter number
  int32_t flag ;
} dmap_filter_arg_002 ;
#define DMAP_FILTER_002(...) (dmap_filter_arg_002) { 002 __VA_OPT__(,) __VA_ARGS__ }
#define DMAP_FP_LOG_QUANTIZE(...) (dmap_filter_arg_002) { 002 __VA_OPT__(,) __VA_ARGS__ }

#pragma weak dmap_filter_003
dmap_filter  dmap_filter_003 ;
typedef struct{
  uint32_t filter ;  // filter number
  float    maxerr ;  // maximum absolute error
  uint32_t nbits ;   // maximum number of bits for quantized values (1 -> 24)
  int32_t  offset ;  // use this offset if non zero (use minimum quantized value if 0x7FFFFFFF)
} dmap_filter_arg_003 ;
typedef dmap_filter_arg_003 dmap_fp_linear_quantize ;
#define DMAP_FILTER_003(...) (dmap_filter_arg_003) { 003 __VA_OPT__(,) __VA_ARGS__ }
#define DMAP_FP_LINEAR_QUANTIZE(...) (dmap_filter_arg_003) { 003 __VA_OPT__(,) __VA_ARGS__ }

#pragma weak dmap_filter_004
dmap_filter  dmap_filter_004 ;
typedef struct{
  uint32_t filter ;  // filter number
} dmap_filter_arg_004 ;
typedef dmap_filter_arg_004 dmap_lorenzo_arg ;
#define DMAP_FILTER_004(...) (dmap_filter_arg_004) { 004 __VA_OPT__(,) __VA_ARGS__ }
#define DMAP_LORENZO(...) (dmap_filter_arg_004) { 004 __VA_OPT__(,) __VA_ARGS__ }

#pragma weak dmap_filter_005
dmap_filter  dmap_filter_005 ;
typedef struct{
  uint32_t filter ;  // filter number
  uint32_t levels ;
} dmap_filter_arg_005 ;
typedef dmap_filter_arg_005 dmap_wavelet_arg ;
#define DMAP_FILTER_005(...) (dmap_filter_arg_005) { 005 __VA_OPT__(,) __VA_ARGS__ }
#define DMAP_WAVELET(...) (dmap_filter_arg_005) { 005 __VA_OPT__(,) __VA_ARGS__ }

#pragma weak dmap_filter_006
dmap_filter  dmap_filter_006 ;
typedef struct{
  uint32_t filter ;  // filter number
  // mode >  200 : tile encoding with tile size mode - 200
  // mode >  100 : zigzag encoding using mode - 100 bits ( 101 -> 164 )
  // mode >    0 : raw encoding using mode bits ( 1 -> 64 )
  // mode ==   0 : raw encoding using size from array descriptor
  int32_t mode ;
} dmap_filter_arg_006 ;
typedef dmap_filter_arg_006 dmap_encode_arg ;
#define DMAP_FILTER_006(...) (dmap_filter_arg_006) { 006 __VA_OPT__(,) __VA_ARGS__ }
#define DMAP_ENCODE(...) (dmap_filter_arg_006) { 006 __VA_OPT__(,) __VA_ARGS__ }

#pragma weak dmap_filter_007
dmap_filter  dmap_filter_007 ;
typedef struct{
  uint32_t filter ;  // filter number
  float offset ;
  float scale ;
} dmap_filter_arg_007 ;
#define DMAP_FILTER_007(...) (dmap_filter_arg_007) { 007 __VA_OPT__(,) __VA_ARGS__ }

#endif
