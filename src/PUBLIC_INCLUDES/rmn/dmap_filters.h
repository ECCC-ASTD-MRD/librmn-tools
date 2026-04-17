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

// ID for end of filter chain marker
#define FILTER_CHAIN_END 0377

// array functions
#include <rmn/array_nd.h>
// use a Big Endian bit stream
#include <rmn/be_stream.h>
#include <rmn/bitstream.h>

// the following 3 macros should now come from rmn/common_stream.h
#if ! defined(STREAM_BITS_BHW)
#error STREAM_BITS_BHW should now come from librmn
#define STREAM_BITS_BHW(v, nbits) { uint32_t c = 2 + 8 + ((v >> 8) ? 8 : 0) + ((v >> 16) ? 8 : 0) + ((v >> 24) ? 8 : 0) ; nbits = c ; }
#endif
#if ! defined(STREAM_GET_BHW)
#error STREAM_GET_BHW should now come from librmn
#define STREAM_GET_BHW(s, v, nbits) { uint32_t c ; \
                                      STREAM_GET_NBITS(s, c , 2) ; \
                                      uint32_t TbItS = (1 + c) << 3 ; \
                                      STREAM_GET_NBITS(s, v , TbItS) ; \
                                      nbits = TbItS + 2 ; \
                                    }
#endif
#if ! defined(STREAM_PUT_BHW)
#error STREAM_PUT_BHW should now come from librmn
// store v into stream, set nbits to 10/18/26/34 according to number of bits needed
#define STREAM_PUT_BHW(s, v, nbits) { uint32_t c = ((v >> 8) ? 1 : 0) + ((v >> 16) ? 1 : 0) + ((v >> 24) ? 1 : 0) ; \
                                      uint32_t TbItS = (1 + c) << 3 ; \
                                      STREAM_PUT_NBITS(s, c , 2) ; \
                                      STREAM_PUT_NBITS(s, v , TbItS) ; \
                                      nbits = TbItS + 2 ; \
                                    }
#endif

// allocate an argument list with room for at most nmax arguments
// typedef struct{
//   uint32_t filter ;  // filter number
//   uint32_t nargs ;   // number of arguments
//   union{
//     double    d ;    // 64 bit double
//     void     *p ;    // address
//     int64_t   l ;    // 64 bit signed integer
//     uint64_t lu ;    // 64 bit unsigned integer
//     int32_t   i ;    // 32 bit signed integer
//     uint32_t  u ;    // 32 bit unsigned integer
//     float     f ;    // 32 bit float
//   } args[] ;         // arguments ( [0] .. [nargs-1] )
// } dmapfilter_args ;  // processing function argument list

// forward filter control structure template
// generic data map pipe filter arguments
typedef struct{
  uint32_t filter ;  // filter number
  uint8_t args[] ;
} dmap_filter_args ;

// pointer to data map pipe filter arguments
typedef dmap_filter_args *dmap_filter_args_ptr ;

// list of pointers to dmap_filter_args structures (NULL TERMINATED)
typedef  dmap_filter_args_ptr *dmap_filter_list ;

typedef enum { DMAP_FILTER, DMAP_RESTORE, DMAP_ENCODE, DMAP_DECODE, DMAP_PRINT } dmap_command ;
// generic data map pipe filter function
typedef ssize_t dmap_filter(array_nd *, block_properties *, dmap_filter_list, bitstream *, dmap_command) ;
// generic pointer to data map pipe filter function
typedef dmap_filter *dmap_filter_ptr ;

// data map pipe filter parameter encoder
ssize_t dmap_encode_parameters(dmap_filter_list, bitstream *) ;

// data map pipe filter parameter decoder
dmap_filter_list dmap_decode_parameters(bitstream *) ;

// print data map pipe filter parameters
int32_t dmap_print_parameters(dmap_filter_list dpfl) ;

#include <rmn/dmap_filters_000_007.h>
#include <rmn/dmap_filters_010_017.h>
#include <rmn/dmap_filters_020_027.h>
#include <rmn/dmap_filters_030_037.h>

// end of filter list
#define FILTER_LIST_END NULL

extern dmap_filter_arg_000 dmap_args_null ;
// last filter in block (tuples)
#define FILTER_BLOCK_END ((dmap_filter_args_ptr) &dmap_args_null)

// alternative bit stream encoding format for filter metadata
// first element of metadata for ALL filters (MUST be present and be the FIRST element)
// id    : filter ID (000 -> 255)
// flags : local flags for this filter (0 -> 15)
// size  : size of the struct in 32 bit units (excluding prolog) (0 -> 30)
// meta0 : used for size or metadata. if size == 31 , size = meta0 (0 -> 65535)
// #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
// #define DMAPFILTER_PROLOG uint16_t id:7, flags:4, size:5 ; uint16_t meta0 ;
// #endif

// ============ preliminary, subject to changes ============
// bit stream encoding of a datamap block :
// filter 000 marker
// array description
// stream from filter n (nn x 32 bits)
// .....
// stream from filter 1 (first filter) (n1 x 32 bits)
// end marker
//
// encoded data stream is written in decoding order

// a float block would use a filter list sequence like
// quantization filter | prediction filter | encoding filter

// static inline int dmapfilter_invalid(dmapfilter_args *args, uint32_t expected){
//   return (args->filter != expected) ;
// }

// handle error message text and error code for data map pipe filter
// id  : filter id
// msg : error message string
// if the first character of the message is > 0 and < 32, it is an error code
int dmap_filter_error(int id, char *msg);

// make a new data map pipe filter available
int dmap_filter_set(dmap_filter_ptr filter, int ordinal, const char *name, size_t arg_size, int force);

// check if dmap_filter_xxx exists (octal 000 -> 037)
int dmap_filter_exists(int ordinal);
// get address of dmap_filter_xxx
dmap_filter_ptr dmap_filter_get(int ordinal);
// get "name" of dmap_filter_xxx
const char *dmap_filter_name(int ordinal);
// get argument struct size for dmap_filter_xxx
size_t dmap_filter_argsize(int ordinal);

// internal functions, no longer needed in .h
// ssize_t dmap_filter_bad(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream);
// ssize_t dmap_filter_none(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream);
// ssize_t dmap_filter_end(bitstream *stream);

// get address of next filter function in list
dmap_filter_ptr dmap_filter_next(dmap_filter_list dpfl);
// is dpfl pointing to filter dmap_filter_xxx where xxx == id
int dmap_filter_valid(dmap_filter_list dpfl, uint32_t id);
// is this filter the last filter in the list
int dmap_filter_is_last(dmap_filter_list dpfl);

// forward chain of data map pipe filters (encode array into bitstream)
ssize_t dmap_filter_enc(array_nd *a, block_properties *bp, dmap_filter_list dpfl, bitstream *stream);
// restore chain of data map pipe filters (restore/decode array from bit stream)
ssize_t dmap_filter_dec(array_nd *a, bitstream *stream);     // full decode (8/16/32/64 bits + tuples)
ssize_t dmap_filter_inv(array_nd *a, bitstream *stream);     // single 32 bits block decode (next inverse filter)

// put array information into bit stream
int32_t dmap_filter_put_array_info(array_nd *a, bitstream *stream);
// retrieve array information from bit stream
int32_t dmap_filter_get_array_info(array_nd *a, bitstream *stream, int allocate);

// set "strict" processing mode
int dmap_strict_mode(int mode);
// set "debug" mode
int dmap_debug_mode(int mode);

#endif
