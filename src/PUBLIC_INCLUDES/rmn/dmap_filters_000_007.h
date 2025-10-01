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

#if ! defined(DMAP_FILTER_001)

// filter 000 is a special case, it MUST be present
// there is no argument struct associated with it
dmap_filter  dmap_filter_fwd ;

// filter 001, float demo/scaling filter
#pragma weak dmap_filter_001
dmap_filter  dmap_filter_001 ;
typedef struct{
  uint32_t filter ;  // filter number
  float offset ;
  float scale ;
} dmap_filter_arg_001 ;
#define DMAP_FILTER_001(...) (dmap_filter_arg_001) { 001 , __VA_ARGS__ }
#define DMAP_SAXPY_001(...) (dmap_filter_arg_001) { 001 , __VA_ARGS__ }

// filter 002, deprecated, will be replaced by filter 003
#pragma weak dmap_filter_002
dmap_filter  dmap_filter_002 ;
typedef struct{
  uint32_t filter ;  // filter number
  int32_t flag ;
} dmap_filter_arg_002 ;
#define DMAP_FILTER_002(...) (dmap_filter_arg_002) { 002 , __VA_ARGS__ }
#define DMAP_FP_LOG_QUANTIZE(...) (dmap_filter_arg_002) { 002 , __VA_ARGS__ }

// filter 003, float -> integer quantizer
#pragma weak dmap_filter_003
dmap_filter  dmap_filter_003 ;
typedef struct{
  uint32_t filter ;  // filter number
  float    maxerr ;  // maximum absolute or relative error
  float    maxsig ;  // values below maxsig are considered NOT SIGNIFICANT
  int32_t  nbits ;   // maximum number of bits for quantized values (1 -> 24)
  int32_t  offset ;  // use this offset if non zero (use minimum quantized value if 0x7FFFFFFF)
  int32_t  mode ;    // quantization mode 0 : linear, 1 : pseudo log
} dmap_filter_arg_003 ;
typedef dmap_filter_arg_003 dmap_fp_quantize ;
#define DMAP_FILTER_003(...) (dmap_filter_arg_003) { 003 , __VA_ARGS__ }
#define DMAP_FP_QUANTIZE(...) (dmap_filter_arg_003) { 003 , __VA_ARGS__ }

// filter 004, Lorenzo predictor for signed integers
#pragma weak dmap_filter_004
dmap_filter  dmap_filter_004 ;
typedef struct{
  uint32_t filter ;  // filter number, 004
} dmap_filter_arg_004 ;
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
typedef dmap_filter_arg_005 dmap_wavelet_arg ;
#define DMAP_FILTER_005(...) (dmap_filter_arg_005) { 005 , __VA_ARGS__ }
#define DMAP_WAVELET(...) (dmap_filter_arg_005) { 005 , __VA_ARGS__ }

// filter 006, integer data encoding, MUST BE the last filter in the filter list
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
  int32_t mode ;
  int  options ;   // options for tile encoding (unused in other modes)
} dmap_filter_arg_006 ;
typedef dmap_filter_arg_006 dmap_encode_arg ;
#define DMAP_FILTER_006(...) (dmap_filter_arg_006) { 006 , __VA_ARGS__ }
#define DMAP_ENCODE(...) (dmap_filter_arg_006) { 006 , __VA_ARGS__ }

// filter 007, all in one filter for floats, quantize/decimate/predict/encode
#pragma weak dmap_filter_007
dmap_filter  dmap_filter_007 ;
typedef struct{
  uint32_t filter ;    // filter number
  float    maxerr ;    // maximum absolute or relative error
  float    zero ;      // maximum significant value (only for log quantizing)
  int32_t  nbits ;     // number of significant bits
  int32_t  qmode ;     // quantization mode (linear, pseudo log, fake integer, ...)
  int32_t  decimate ;  // decimation factor (0 or 1 : NONE, even, no more than 2 for new)
  int32_t  predict ;   // predictor (None, Lorenzo, Wavelet, ...)
  int32_t  tsize ;     // encoding tile size
  int32_t  options ;   // tile encoding options
} dmap_filter_arg_007 ;
#define DMAP_FILTER_007(...) (dmap_filter_arg_007) { 007 , __VA_ARGS__ }
#define DMAP_FP_BLOCK(...) (dmap_filter_arg_007) { 007 , __VA_ARGS__ }

#endif
