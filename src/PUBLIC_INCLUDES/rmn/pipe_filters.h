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

// #include <rmn/ieee_quantize.h>
// #include <rmn/tools_types.h>
#include <rmn/word_stream.h>
#include <rmn/ct_assert.h>
#include <rmn/filter_base.h>

// typedef uint64_t pack_flags ;
#define PIPE_FORWARD     1
#define PIPE_REVERSE     2
#define PIPE_INPLACE     4
#define PIPE_REALLOC     8     /* realloc() can be used for buffer */
#define PIPE_NOFREE     16     /* buffer cannot be "freed" */
#define PIPE_VALIDATE   32
#define PIPE_FWDSIZE    64

#define PIPE_DATA_SIGNED    1
#define PIPE_DATA_UNSIGNED  2
#define PIPE_DATA_FP        4

#define MAX_ARRAY_DIMENSIONS 5
typedef union{
  uint32_t u ;
  int32_t  i ;
  float    f ;
} i_u_f;

#define PROP_VERSION      1
#define PROP_MIN_MAX      1

#define ARRAY_PROPERTIES_SIZE (MAX_ARRAY_DIMENSIONS + MAX_ARRAY_PROPERTIES + 3 + 2)
#define MAX_ARRAY_PROPERTIES 8
// N.B. arrays are stored Fortran style, 1st dimension varying first
typedef struct{
  uint32_t  version:8,  // structure version
            ndims:8,    // number of dimensions
            esize:8,    // array element size (1/2/4)
            etype:8,    // element type (signed/unsigned/float)
            nprop:8,    // number of used properties (prop[])
            ptype:8,    // property type (allows to know what is in prop[n])
            fid:8,      // id of last filter that modified properties
            spare:8,
            tilex:16,   // tiling block size along 1st dimension
            tiley:16;   // tiling block size along 2nd dimension
  uint32_t nx[MAX_ARRAY_DIMENSIONS] ;
  i_u_f    prop[MAX_ARRAY_PROPERTIES] ;
  uint32_t *extra ;    // normally NULL, pointer to extended information
} array_properties ;
CT_ASSERT(ARRAY_PROPERTIES_SIZE == W32_SIZEOF(array_properties))

static array_properties array_properties_null = 
       { .version = 1, .ndims = 0, .esize = 0, .etype = 0, .nprop = 0, .nx = {[0 ... MAX_ARRAY_DIMENSIONS-1] = 1 } } ;

typedef struct{
  void *data ;
  union{
    uint32_t props[W32_SIZEOF(array_properties)] ;
    array_properties ap ;
  };
} array_descriptor ;
static array_descriptor array_null = { .data = NULL, .props = {[0 ... ARRAY_PROPERTIES_SIZE-1] = 0 } } ;

static int array_data_values(array_descriptor *ap){
  int nval = 1, i, ndims = ap->ap.ndims ;
  if(ndims < 0 || ndims > MAX_ARRAY_DIMENSIONS) return -1 ;
  for(i=0 ; i<ndims ; i++) nval *= ap->ap.nx[i] ;
  return nval ;
}

static int array_data_size(array_descriptor *ap){
  return array_data_values(ap) * ap->ap.esize ;
}

typedef struct{
  void *buffer ;           // pointer to storage for buffer
  size_t used ;            // number of bytes used in buffer
  size_t max_size ;        // buffer maximum size in bytes
  uint32_t flags ;         // usage flags for buffer
} pipe_buffer ;
static pipe_buffer pipe_buffer_null = { .used = 0, .max_size = 0, .buffer = NULL, .flags = 0 } ;

static size_t pipe_buffer_bytes_used(pipe_buffer *p){
  return p->used ;
}

static size_t pipe_buffer_bytes_free(pipe_buffer *p){
  return (p->max_size - p->used) ;
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

// translate array dimensions into number of values
// ap  [IN] : pointer to array dimensions struct
static int filter_data_values(const array_properties *ap){
  int nval = 0 ;
  int i ;
  int ndims = ap->ndims ;
  if(ndims > 5) goto end ;
  nval = 1 ;
  for(i=0 ; i<ndims ; i++){
    nval *= ap->nx[i] ;
  }
end:
  return nval ;
}

// translate array dimensions into number of bytes
// ap  [IN] : pointer to array dimensions struct
static int filter_data_bytes(const array_properties *ap){
  return filter_data_values(ap) * ap->esize ;
}

// set filter metadata to null values (only keep id and size unchanged)
// m [IN] : pointer to generic metadata struct
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
    uint32_t v[MAX_ARRAY_DIMENSIONS+2] ;
  } filter_dim ;

#define BASE_META_SIZE (W32_SIZEOF(filter_meta))
#define ALLOC_META(nmeta) (filter_meta *)malloc( sizeof(filter_meta) + (nmeta) * sizeof(uint32_t) )
// to allocate space for NM meta elements : uint32_t xxx[BASE_META_SIZE + NM]
//                                          yyy = malloc((sizeof(uint32_t) * (BASE_META_SIZE + NM))
// then use (filter_meta) xxx or (filter_meta *) yyy as argument to filters

// flags     [IN] : flags passed to filter (controls filter behaviour)
// dims      [IN] : dimensions and data element size of array in buffer.
//                  (some filters will consider data as 1D anyway)
//                  not used with PIPE_VALIDATE
//          [OUT] : some filters may need to alter the data dimensions
//                  the inbound dimensions must then be stored in meta_out for the reverse filter
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

typedef ssize_t pipe_filter(uint32_t flags, array_properties *ap, const filter_meta *meta_in, pipe_buffer *buffer, wordstream *meta_out) ;
typedef pipe_filter *pipe_filter_pointer ;

// encode/decode array dimensions <-> filter struct (expected to contain ONLY dimension information)
int32_t filter_dimensions_encode(const array_properties *ap, filter_meta *fm);
int32_t filter_dimensions_decode(array_properties *ap, const filter_meta *fm);

ssize_t pipe_filter_validate(filter_meta *meta_in);
int pipe_filter_is_defined(int id);
int pipe_filter_register(int id, char *name, pipe_filter_pointer fn);
void pipe_filters_init(void);
pipe_filter_pointer pipe_filter_address(int id);
char *pipe_filter_name(int id);

ssize_t run_pipe_filters(int flags, array_descriptor *data_in, const filter_list list, wordstream *stream);
ssize_t tiled_fwd_pipe_filters(int flags, array_descriptor *data_in, const filter_list list, wordstream *stream);

#endif // ! defined(PIPE_FORWARD)
