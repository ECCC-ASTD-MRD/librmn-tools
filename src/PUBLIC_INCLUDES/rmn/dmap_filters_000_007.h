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

#if ! defined(DMAP_FILTER_000)

#include <rmn/ct_assert.h>

// filter 000 is a special case, it MUST be present
// dmap_filter_arg_000 is a generic structure with a flexible array
// there is no argument struct associated with it
dmap_filter  dmap_filter_000 ;
typedef struct{
  uint32_t filter ;  // filter number
  uint32_t arg[] ;
} dmap_filter_arg_000 ;
CT_ASSERT(sizeof(dmap_filter_arg_000) <= MAX_FILTER_ARG_SIZE, "sizeof(dmap_filter_arg_000) too large") ;
#define DMAP_FILTER_000(...) (dmap_filter_arg_000) { .filter = 001 }

// filter 001, integer/float demo/saxpy filter
// upper 9 bits used to differentiate floats from ints (all 1s or all 0s for ints)
#pragma weak dmap_filter_001
dmap_filter  dmap_filter_001 ;
typedef struct{
  uint32_t filter ;  // filter number, 001
  union{ float offset ; int32_t ioffset ; } ;
  union{ float  scale ; int32_t  iscale ; } ;
} dmap_filter_arg_001 ;
CT_ASSERT(sizeof(dmap_filter_arg_001) <= MAX_FILTER_ARG_SIZE, "sizeof(dmap_filter_arg_001) too large") ;
typedef dmap_filter_arg_001 dmap_saxpy_arg ;
#define DMAP_FILTER_001(...) (dmap_filter_arg_001) { .filter = 001 , __VA_ARGS__ }
#define DMAP_SAXPY(...) (dmap_filter_arg_001) { 001 , __VA_ARGS__ }

// filter 002, NO-OP for now
#pragma weak dmap_filter_002
dmap_filter  dmap_filter_002 ;
typedef struct{
  uint32_t filter ;  // filter number, 002
  uint32_t flag ;
} dmap_filter_arg_002 ;
CT_ASSERT(sizeof(dmap_filter_arg_002) <= MAX_FILTER_ARG_SIZE, "sizeof(dmap_filter_arg_002) too large") ;
typedef dmap_filter_arg_002 dmap_no_op2 ;
#define DMAP_FILTER_002(...) (dmap_filter_arg_002) { 002 , __VA_ARGS__ }
#define DMAP_NO_OP2(...) (dmap_filter_arg_002) { 002 , __VA_ARGS__ }
// #define DMAP_FP_LOG_QUANTIZE(...) (dmap_filter_arg_002) { 002 , __VA_ARGS__ }

// filter 003, float -> integer quantizers
#pragma weak dmap_filter_003
dmap_filter  dmap_filter_003 ;
typedef struct{
  uint32_t  filter ;  // filter number, 003
  int32_t   mode ;    // quantization mode 0 : linear, 1 : fake integers, 2 : float block
  uint32_t  nbits ;   // maximum number of significant bits kept in quantized values (1 -> 24)
  union{ float abserr ; float relerr ; float maxerr ; } ; // maximum absolute or relative error
  int32_t  offset ;   // use this offset if non zero (use minimum quantized value if 0x7FFFFFFF) (linear quantizer)
  float    minabs ;   // values below minabs are considered NOT SIGNIFICANT (not for linear quantizer)
  float    zval ;     // |value| < minabs gets replaced with zval (not for linear quantizer)
} dmap_filter_arg_003 ;
CT_ASSERT(sizeof(dmap_filter_arg_003) <= MAX_FILTER_ARG_SIZE, "sizeof(dmap_filter_arg_003) too large") ;
typedef dmap_filter_arg_003 dmap_fp_quantize ;
#define DMAP_FILTER_003(...) (dmap_filter_arg_003) { 003 , __VA_ARGS__ }
#define DMAP_FP_QUANTIZE(...) (dmap_filter_arg_003) { 003 , __VA_ARGS__ }

// filter 004, Lorenzo predictor for signed integers (no specific arguments)
#pragma weak dmap_filter_004
dmap_filter  dmap_filter_004 ;
typedef struct{
  uint32_t filter ;  // filter number, 004
//   uint32_t dummy ;   // because of __VA_ARGS__
} dmap_filter_arg_004 ;
CT_ASSERT(sizeof(dmap_filter_arg_004) <= MAX_FILTER_ARG_SIZE, "sizeof(dmap_filter_arg_004) too large") ;
typedef dmap_filter_arg_004 dmap_lorenzo_arg ;
#define DMAP_FILTER_004(...) (dmap_filter_arg_004) { 004 , __VA_ARGS__ }
#define DMAP_LORENZO(...) (dmap_filter_arg_004) { 004 , __VA_ARGS__ }

// filter 005, signed integer wavelet filter
#pragma weak dmap_filter_005
dmap_filter  dmap_filter_005 ;
typedef struct{
  uint32_t filter ;  // filter number, 005
  uint32_t levels ;
} dmap_filter_arg_005 ;
CT_ASSERT(sizeof(dmap_filter_arg_005) <= MAX_FILTER_ARG_SIZE, "sizeof(dmap_filter_arg_005) too large") ;
typedef dmap_filter_arg_005 dmap_wavelet_arg ;
#define DMAP_FILTER_005(...) (dmap_filter_arg_005) { 005 , __VA_ARGS__ }
#define DMAP_WAVELET(...) (dmap_filter_arg_005) { 005 , __VA_ARGS__ }

// filter 006, integer data encoding, MUST BE the LAST filter in the filter list
#pragma weak dmap_filter_006
dmap_filter  dmap_filter_006 ;
typedef struct{
  uint32_t filter ;  // filter number, 006
  // mode >=  100   : tile encoding with tile size mode - 100 (nbits/zigzag computed independently for each tile)
  // mode ==   99   : BHW encoding (2 bit length code, followed by 8/16/24/32 bits of data), nbits irrelevant
  // mode ==   98   : zigzag encoding, nbits auto adjusted
  // 0 <= mode < 65 : raw encoding using mode bits ( 0 - 64 )
  // mode ==  -1    : raw encoding using size from array descriptor
  // modes 0 and 100 both set nbits to 0 (automatically compute necessary nbits)
  int32_t  mode ;
  uint32_t options ;   // options for tile encoding (unused in other modes)
} dmap_filter_arg_006 ;
CT_ASSERT(sizeof(dmap_filter_arg_006) <= MAX_FILTER_ARG_SIZE, "sizeof(dmap_filter_arg_006) too large") ;
typedef dmap_filter_arg_006 dmap_encode_arg ;
#define DMAP_FILTER_006(...) (dmap_filter_arg_006) { 006 , __VA_ARGS__ }
#define DMAP_ENCODE(...) (dmap_filter_arg_006) { 006 , __VA_ARGS__ }

// filter 007, future all in one filter for floats, quantize/decimate/predict/encode
// NO OP for now
#pragma weak dmap_filter_007
dmap_filter  dmap_filter_007 ;
typedef struct{
  uint32_t filter ;  // filter number, 007
//   float    maxerr ;    // maximum absolute or relative error
//   float    zero ;      // maximum significant value (only for log quantizing)
//   int32_t  nbits ;     // number of significant bits
//   int32_t  qmode ;     // quantization mode (linear, pseudo log, fake integer, ...)
//   int32_t  decimate ;  // decimation factor (0 or 1 : NONE, even, no more than 2 for new)
//   int32_t  predict ;   // predictor (None, Lorenzo, Wavelet, ...)
//   int32_t  tsize ;     // encoding tile size
//   int32_t  options ;   // tile encoding options
} dmap_filter_arg_007 ;
CT_ASSERT(sizeof(dmap_filter_arg_007) <= MAX_FILTER_ARG_SIZE, "sizeof(dmap_filter_arg_007) too large") ;
typedef dmap_filter_arg_007 dmap_no_op7 ;
#define DMAP_FILTER_007(...) (dmap_filter_arg_007) { 007 , __VA_ARGS__ }
#define DMAP_NO_OP7(...) (dmap_filter_arg_007) { 007 , __VA_ARGS__ }

#endif
