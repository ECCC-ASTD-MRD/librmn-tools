//
// Copyright (C) 2023  Environnement Canada
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
//     M. Valin,   Recherche en Prevision Numerique, 2023
//

#if ! defined(PIPE_FORWARD)

#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>

#include <rmn/ieee_quantize.h>
#include <rmn/tools_types.h>
#include <rmn/word_stream.h>
#include <rmn/ct_assert.h>

typedef uint64_t pack_flags ;
#define PIPE_FORWARD     1
#define PIPE_REVERSE     2
#define PIPE_INPLACE     4
#define PIPE_REALLOC     8     /* realloc() can be used for buffer */
#define PIPE_NOFREE     16     /* buffer cannot be "freed" */
#define PIPE_VALIDATE   32
#define PIPE_FWDSIZE    64

#define ARRAY_DESCRIPTOR_MAXDIMS 5

typedef struct{
  uint32_t ndims ;
  uint32_t nx[ARRAY_DESCRIPTOR_MAXDIMS] ;
  uint32_t esize ;
} array_dimensions ;
array_dimensions array_dimensions_null = { .ndims = 0, .esize = 0, .nx = {[0 ... ARRAY_DESCRIPTOR_MAXDIMS-1] = 1 } } ;

typedef struct{
  void *data ;
  union{
    uint32_t dims[W32_SIZEOF(array_dimensions)] ;
    array_dimensions adim ;
  };
} array_descriptor ;
array_descriptor array_null = { .data = NULL, .dims = {[0 ... ARRAY_DESCRIPTOR_MAXDIMS] = 0 } } ;

#define PIPE_DATA_NDIMS(dims)  ((dims)[0])

static int array_data_values(array_descriptor *ad){
  int nval = 1, i, ndims = ad->adim.ndims ;
  if(ndims < 0 || ndims > ARRAY_DESCRIPTOR_MAXDIMS) return -1 ;
  for(i=0 ; i<ndims ; i++) nval *= ad->adim.nx[i] ;
  return nval ;
}

static int array_data_size(array_descriptor *ad){
  return array_data_values(ad) * ad->adim.esize ;
}

typedef struct{
  size_t used ;            // number of bytes used in buffer
  size_t max_size ;        // buffer maximum size in bytes
  void *buffer ;           // pointer to storage for buffer
  uint32_t flags ;         // usage flags for buffer
} pipe_buffer ;
pipe_buffer pipe_buffer_null = { .used = 0, .max_size = 0, .buffer = NULL, .flags = 0 } ;

static size_t pipe_buffer_bytes_used(pipe_buffer *p){
  return p->used / sizeof(uint32_t) ;
}

static size_t pipe_buffer_bytes_free(pipe_buffer *p){
  return (p->max_size - p->used) / sizeof(uint32_t) ;
}

static size_t pipe_buffer_words_used(pipe_buffer *p){
  return p->used / sizeof(uint32_t) ;
}
static size_t pipe_buffer_words_free(pipe_buffer *p){
  return (p->max_size - p->used) / sizeof(uint32_t) ;
}

static void *pipe_buffer_data(pipe_buffer *p){
  return p->buffer ;
}

// translate dims[] into number of values
static int filter_data_values(array_dimensions *ad){
  int nval = 0 ;
  int i ;
  int ndims = ad->ndims ;
  if(ndims > 5) goto end ;
  nval = 1 ;
  for(i=0 ; i<ndims ; i++){
    nval *= ad->nx[i] ;
  }
end:
  return nval ;
}
#define FILTER_CONCAT2(str1,str2) str1 ## str2
#define FILTER_CONCAT3(str1,str2,str3) str1 ## str2 ## str3
#define FILTER_TYPE(id) FILTER_CONCAT2(filter_ , id)
#define FILTER_FUNCTION(id) FILTER_CONCAT2(pipe_filter_ , id)
#define FILTER_NULL(id) FILTER_CONCAT3(filter_ , id , _null)
#define FILTER_BASE(fid) .size = W32_SIZEOF(FILTER_TYPE(fid)), .id = fid, .flags = 0

// first element of metadata for ALL filters
// id    : filter ID
// size  : size of the struct in 32 bit units (includes 32 bit prolog)
// used  : used space in 32 bit units
// flags : local flags for this filter
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define FILTER_PROLOG uint32_t id:8, flags:2, size:22
#endif
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define FILTER_PROLOG uint32_t size:22, flags:2, id:8
#endif

// check that size of filter struct is a multiple of 32 bits
#define FILTER_SIZE_OK(name) (W32_SIZEOF(name)*sizeof(uint32_t) == sizeof(name))

typedef struct{             // generic filter with metadata
  FILTER_PROLOG ;           // used for meta_out in forward mode
  uint32_t meta[] ;        // and meta_in in reverse mode
} filter_meta ;             // type used by the filter API

// set filter metadata to null values (only keep id and size as they are)
static inline void filter_reset(filter_meta *m){
  int i ;
  m->flags = 0 ;                                        // set flags to 0
  for(i=0 ; i< (m->size - 1) ; i++) m->meta[i] = 0 ;    // set metadata array to 0
}

// typedef struct{             // input metadata for all the pipe filters to be run
//   int nfilters ;            // number of filters
//   filter_meta *meta[] ;     // pointers to the metadata for the filters
// } filter_list ;

typedef filter_meta *filter_list[] ;  // input metadata for all the pipe filters to be run

typedef  struct{   // max size for encoding dimensions
    FILTER_PROLOG ;
    uint32_t v[ARRAY_DESCRIPTOR_MAXDIMS+2] ;
  } filter_dim ;

// ----------------- id = 000, null filter (last filter) -----------------
typedef struct{
  FILTER_PROLOG ;
  array_dimensions adim ;
} FILTER_TYPE(000) ;
static FILTER_TYPE(000) FILTER_NULL(000) = {FILTER_BASE(000) } ;
CT_ASSERT(FILTER_SIZE_OK(FILTER_TYPE(000)))
// ----------------- id = 100, linear quantizer -----------------
typedef struct{
  FILTER_PROLOG ;
  float    ref ;
  uint32_t nbits : 5 ;
} FILTER_TYPE(100) ;
static FILTER_TYPE(100) filter_100_null = {FILTER_BASE(100), .ref = 0.0f, .nbits = 0 } ;
CT_ASSERT(FILTER_SIZE_OK(FILTER_TYPE(100)))
// ----------------- id = 254, scale and offset filter -----------------
typedef struct{
  FILTER_PROLOG ;
  int32_t factor ;
  int32_t offset ;
} FILTER_TYPE(254) ;
static FILTER_TYPE(254) filter_254_null = {FILTER_BASE(254), .factor = 0, .offset = 0 } ;
CT_ASSERT(FILTER_SIZE_OK(FILTER_TYPE(254)))
// ----------------- end of filter metadata definitions -----------------

#define BASE_META_SIZE (W32_SIZEOF(filter_meta))
#define ALLOC_META(nmeta) (filter_meta *)malloc( sizeof(filter_meta) + (nmeta) * sizeof(AnyType32) )
// to allocate space for NM meta elements : uint32_t xxx[BASE_META_SIZE + NM]
//                                          yyy = malloc((sizeof(uint32_t) * (BASE_META_SIZE + NM))
// then use (filter_meta) xxx or (filter_meta *) yyy as argument to filters

// flags     [IN] : flags passed to filter (controls filter behaviour)
// dims      [IN] : dimensions and data element size of array in buffer.
//                  (some filters will consider data as 1D anyway)
//                  not used with PIPE_VALIDATE
// buffer    [IN] : input data to filter (used used by PIPE_FORWARD and PIPE_REVERSE)
//                  buffer->used      : number of valid bytes in buffer->buffer
//                  buffer->max_size  : available storage size in buffer->buffer
//          [OUT] : output from filter, same address as before if operation can be performed in place
//                  buffer->used     : number of valid bytes in buffer->buffer
//                  buffer->max_size : available storage size in buffer->buffer
// meta_in   [IN] : parameters passed to filter
// meta_out [OUT]
//         PIPE_VALIDATE : not used
//         PIPE_FWDSIZE  : meta_out->size = needed size of meta_out for FORWARD call
//         PIPE_REVERSE  : not used
//         PIPE_FORWARD  : future meta_in for the REVERSE call
// the function returns
//         PIPE_VALIDATE : needed size for meta_out(32 bit units), negative value in case of error(s)
//         PIPE_FWDSIZE  : "worst case" size of filter output, negative value in case of error(s)
//         PIPE_FORWARD  : size of data output in bytes, negative value in case of error(s)
//         PIPE_REVERSE  : size of data output in bytes, negative value in case of error(s)

typedef ssize_t pipe_filter(uint32_t flags, array_dimensions *ad, filter_meta *meta_in, pipe_buffer *buffer, wordstream *meta_out) ;
typedef pipe_filter *pipe_filter_pointer ;

pipe_filter FILTER_FUNCTION(000) ;
pipe_filter FILTER_FUNCTION(001) ;
pipe_filter FILTER_FUNCTION(254) ;

// encode/decode array dimensions <-> filter struct (expected to contain ONLY dimension information)
int32_t filter_dimensions_encode(array_dimensions *ad, filter_meta *fm);
void filter_dimensions_decode(array_dimensions *ad, filter_meta *fm);

ssize_t pipe_filter_validate(filter_meta *meta_in);
int pipe_filter_is_defined(int id);
int pipe_filter_register(int id, char *name, pipe_filter_pointer fn);
void pipe_filters_init(void);
pipe_filter_pointer pipe_filter_address(int id);
char *pipe_filter_name(int id);

ssize_t run_pipe_filters(int flags, array_descriptor *data_in, filter_list list, wordstream *stream);
// ssize_t run_pipe_filters_(int flags, int *dims, void *data, filter_list list, wordstream *meta_out, pipe_buffer *buffer);

#endif // ! defined(PIPE_FORWARD)
