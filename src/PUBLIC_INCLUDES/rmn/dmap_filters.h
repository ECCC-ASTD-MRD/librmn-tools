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
#if ! defined(MAX_DP_FILTERS)
#define MAX_DP_FILTERS      32
// max number of 32 bit arguments, excluding filter number (1st item in struct)
#define MAX_ARG_NUM         8
#define MAX_FILTER_ARG_SIZE ((MAX_ARG_NUM+1) * sizeof(uint32_t))

// end of filter chain marker
#define FILTER_CHAIN_END 0377

#include <rmn/cpp_extras.h>
// #include <rmn/data_map.h>
#include <rmn/array_nd.h>
#include <rmn/be_stream.h>
#include <rmn/bitstream.h>

// the following 3 macros will eventually come from rmn/common_stream.h
#if ! defined(STREAM_BITS_BHW)
#define STREAM_BITS_BHW(v, nbits) { uint32_t c = 2 + 8 + ((v >> 8) ? 8 : 0) + ((v >> 16) ? 8 : 0) + ((v >> 24) ? 8 : 0) ; nbits = c ; }
#endif
#if ! defined(STREAM_GET_BHW)
#define STREAM_GET_BHW(s, v, nbits) { uint32_t c ; \
                                      STREAM_GET_NBITS(s, c , 2) ; \
                                      uint32_t TbItS = (1 + c) << 3 ; \
                                      STREAM_GET_NBITS(s, v , TbItS) ; \
                                      nbits = TbItS + 2 ; \
                                    }
#endif
#if ! defined(STREAM_PUT_BHW)
// store v into stream, set nbits to 10/18/26/34 according to number of bits needed
#define STREAM_PUT_BHW(s, v, nbits) { uint32_t c = ((v >> 8) ? 1 : 0) + ((v >> 16) ? 1 : 0) + ((v >> 24) ? 1 : 0) ; \
                                      uint32_t TbItS = (1 + c) << 3 ; \
                                      STREAM_PUT_NBITS(s, c , 2) ; \
                                      STREAM_PUT_NBITS(s, v , TbItS) ; \
                                      nbits = TbItS + 2 ; \
                                    }
#endif

// allocate an argument list with room for at most nmax arguments
typedef struct{
  uint32_t filter ;  // filter number
  uint32_t nargs ;   // number of arguments
  union{
    double    d ;    // 64 bit double
    void     *p ;    // address
    int64_t   l ;    // 64 bit signed integer
    uint64_t lu ;    // 64 bit unsigned integer
    int32_t   i ;    // 32 bit signed integer
    uint32_t  u ;    // 32 bit unsigned integer
    float     f ;    // 32 bit float
  } args[] ;         // arguments ( [0] .. [nargs-1] )
} dmapfilter_args ;  // processing function argument list

// forward filter control structure template
// generic data pipe filter arguments
typedef struct{
  uint32_t filter ;  // filter number
  uint8_t args[] ;
} dmap_filter_args ;

// pointer to data pipe filter arguments
typedef dmap_filter_args *dmap_filter_args_ptr ;

// list of pointers to dmap_filter_args structures (NULL TERMINATED)
typedef  dmap_filter_args_ptr *dmap_filter_list ;

typedef enum { DMAP_FILTER, DMAP_RESTORE, DMAP_ENCODE, DMAP_DECODE, DMAP_PRINT } dmap_command ;
// generic data pipe filter function
typedef ssize_t dmap_filter(array_nd *, block_properties *, dmap_filter_list, bitstream *, dmap_command) ;
// generic pointer to data pipe filter function
typedef dmap_filter *dmap_filter_ptr ;

// data pipe filter parameter encoder
ssize_t dmap_encode_parameters(dmap_filter_list, bitstream *) ;

// data pipe filter parameter decoder
dmap_filter_list dmap_decode_parameters(bitstream *) ;

// print filter parameters
int32_t dmap_print_parameters(dmap_filter_list dpfl) ;

#include <rmn/dmap_filters_000_007.h>
#include <rmn/dmap_filters_010_017.h>
#include <rmn/dmap_filters_020_027.h>
#include <rmn/dmap_filters_030_037.h>

// list of pointers to dmapfilter_args structures
// typedef struct{
//   uint32_t nfilters ;
//   dmapfilter_args *filters[] ;
// } dmapfilter_list ;

// bit stream encoding format for filter metadata
// first element of metadata for ALL filters (MUST be present and be the FIRST element)
// id    : filter ID (000 -> 255)
// flags : local flags for this filter (0 -> 15)
// size  : size of the struct in 32 bit units (excluding prolog) (0 -> 30)
// meta0 : used for size or metadata. if size == 31 , size = meta0 (0 -> 65535)
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define DMAPFILTER_PROLOG uint16_t id:7, flags:4, size:5 ; uint16_t meta0 ;
#endif

// ============ preliminary, subject to changes ============
// bit stream encoding of a datamap block :
// number of filters (8 bits)
// metadata for filter 1 (n x 32 bits)
// .....
// metadata for filter n (last filter) (n x 32 bits)
// encoded data stream

// a float block would use a sequence like
// quantization filter | prediction filter | encoding filter

// processed subarray will be stored into memory described by zmap table entries mem[index] and size[index]
// typedef int (*dmapfilter_ptr)(zmap *map, int index, array_nd *array, dmapfilter_args *args) ;   // pointer to processing function


// static inline int dmapfilter_invalid(dmapfilter_args *args, uint32_t expected){
//   return (args->filter != expected) ;
// }

int dmap_filter_set(dmap_filter_ptr filter, int ordinal, const char *name, size_t arg_size, int force);
int dmap_filter_exists(int ordinal);

dmap_filter_ptr dmap_filter_get(int ordinal);
const char *dmap_filter_name(int ordinal);
size_t dmap_filter_argsize(int ordinal);

// ssize_t dmap_filter_bad(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream);
// ssize_t dmap_filter_none(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream);
// ssize_t dmap_filter_end(bitstream *stream);
dmap_filter_ptr dmap_filter_next(dmap_filter_list dpfl);
int dmap_filter_valid(dmap_filter_list dpfl, uint32_t id);
int dmap_filter_is_last(dmap_filter_list dpfl);

ssize_t dmap_filter_fwd(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream);
ssize_t dmap_filter_inv(array_nd *a, bitstream *stream);

int32_t dmap_filter_put_array_info(array_nd *a, bitstream *stream);
int32_t dmap_filter_get_array_info(array_nd *a, bitstream *stream, int allocate);

int dmap_strict_mode(int mode);
int dmap_debug_mode(int mode);

#endif
