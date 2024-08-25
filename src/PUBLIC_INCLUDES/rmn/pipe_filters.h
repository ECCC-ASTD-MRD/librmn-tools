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

// #define PIPE_DATA_SIGNED    1
// #define PIPE_DATA_UNSIGNED  2
// #define PIPE_DATA_FP        3
// #define PIPE_DATA_DOUBLE    4
typedef enum { UNKNOWN, PIPE_DATA_SIGNED, PIPE_DATA_UNSIGNED, PIPE_DATA_FP, PIPE_DATA_DOUBLE } pd_type ;
static char *ptype[] = {"unknown", "int", "uint", "float", "double"} ;

#define MAX_ARRAY_DIMENSIONS 5
typedef union{
  uint32_t u ;
  int32_t  i ;
  float    f ;
} p_iuf;

#define PROP_VERSION      1

// #define PROP_MIN_MAX      1
typedef enum { NONE, PROP_MIN_MAX } pr_kind ;

#define MAX_ARRAY_PROPERTIES 8
// N.B. arrays are stored Fortran style, 1st dimension varying first
typedef struct{
  void     *data ;
  uint32_t  version ;  // version marker
  uint32_t  ndims ;    // number of dimensions
  uint32_t  esize ;    // array element size (1/2/4)
  uint32_t  etype ;    // element type (signed/unsigned/float/double/...)
  uint32_t  nprop ;    // number of used properties (prop[])
  uint32_t  ptype ;    // property type (allows to know what is in prop[n])
  uint32_t  fid ;      // id of last filter that modified properties
  uint32_t  spare ;
  uint32_t  tilex ;    // tiling block size along 1st dimension
  uint32_t  tiley ;    // tiling block size along 2nd dimension
  uint32_t  nx[MAX_ARRAY_DIMENSIONS] ;    // array dimensions
  uint32_t  n0[2] ;    // tile start along 1st and 2nd dimension
  p_iuf     prop[MAX_ARRAY_PROPERTIES] ;  // array properties (e.g. min value, max value, ...)
  uint32_t *extra ;    // normally NULL, pointer to extended information
} array_descriptor ;
// CT_ASSERT_(sizeof(array_descriptor) == 8 + 8 + 4 + MAX_ARRAY_DIMENSIONS*4 + 2*4 + MAX_ARRAY_PROPERTIES * sizeof(p_iuf) + 8)

// static array_descriptor array_descriptor_null = {.data = NULL, .version = 0, .nx = {[0 ... MAX_ARRAY_DIMENSIONS-1] = 0 } } ;
static array_descriptor array_descriptor_null = {.data = NULL, .version = 0 } ;
// static array_descriptor array_descriptor_base = {.data = NULL, .version = PROP_VERSION, .nx = {[0 ... MAX_ARRAY_DIMENSIONS-1] = 1 } } ;
static array_descriptor array_descriptor_base = {.data = NULL, .version = PROP_VERSION } ;

static inline int array_data_values(array_descriptor *ap){
  int nval = 1, i, ndims = ap->ndims ;
  if(ndims < 0 || ndims > MAX_ARRAY_DIMENSIONS) return -1 ;
  for(i=0 ; i<ndims ; i++) nval *= ap->nx[i] ;
  return nval ;
}

static inline int array_data_size(array_descriptor *ap){
  return array_data_values(ap) * ap->esize ;
}

// NOTE : may need a flag to indicate it can be freed/realloced/just_replaced_with_another_value
typedef   union{
  struct{
    uint32_t u32 ;
  } ;
  struct{
    uint32_t no_realloc  :1,    // the buffer address may be "realloc(ed)"
             can_realloc :1,    // NO realloc permitted
             no_replace  :1,    // the buffer address may NOT be replaced
             can_replace :1,    // the buffer address may be replaced (no call to free() )
             no_free     :1,    // free() must NOT be called
             can_free    :1,    // if no longer useful, free() may be called to free the memory used by buffer
             owner       :1,    // the pipe buffer "owns" the address (it allocated it)
             spare       :25 ;
  } ;
  }p_b_flags ;
CT_ASSERT_(sizeof(p_b_flags) == sizeof(uint32_t))

typedef struct{
  void *buffer ;           // pointer to storage for buffer
  size_t used ;            // number of bytes used in buffer
  size_t max_size ;        // buffer maximum size in bytes
  p_b_flags flags ;
} pipe_buffer ;
static pipe_buffer pipe_buffer_null = { .used = 0, .max_size = 0, .buffer = NULL, .flags.u32 = 0 } ;

static inline size_t pipe_buffer_bytes_used(pipe_buffer *p){
  return p->used ;
}

static inline size_t pipe_buffer_bytes_free(pipe_buffer *p){
  return (p->max_size - p->used) ;
}

static inline size_t pipe_buffer_words_used(pipe_buffer *p){
  return p->used / sizeof(uint32_t) ;
}
static inline size_t pipe_buffer_words_free(pipe_buffer *p){
  return (p->max_size - p->used) / sizeof(uint32_t) ;
}

static inline void *pipe_buffer_data(pipe_buffer *p){
  return p->buffer ;
}

// translate array dimensions into number of values
// ap  [IN] : pointer to array dimensions struct
static inline int filter_data_values(const array_descriptor *ap){
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
static inline int filter_data_bytes(const array_descriptor *ap){
  return filter_data_values(ap) * ap->esize ;
}

typedef filter_meta *filter_list[] ;  // list of input metadata pointers for a filter pipe (NULL terminated list)

// set filter metadata to null values (only keep id and size unchanged)
// m [IN] : pointer to generic metadata struct
static inline void filter_reset(filter_meta *m){
  int i ;
  m->flags = 0 ;                                        // set flags to 0
  for(i=0 ; i< (m->size - 1) ; i++) m->meta[i] = 0 ;    // set metadata array to 0
}

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
// ad        [IN] : dimensions, data element size, extra properties, ...  of array in buffer.
//                  (some filters will consider data as 1D anyway)
//                  not used with PIPE_VALIDATE
//                  the data field IS NOT USED ( NULL is a recommended value)
//          [OUT] : some filters may need to alter the data dimensions
//                  the inbound dimensions that will be used by the reverse filter must then be stored in meta_out
// buffer    [IN] : input data to filter (used by PIPE_FORWARD and PIPE_REVERSE)
//                  buffer->used      : number of valid bytes in buffer->buffer
//                  buffer->max_size  : available storage size in buffer->buffer
//          [OUT] : output from filter, same address as before if operation can be performed in place
//                  buffer->used     : number of valid bytes in buffer->buffer
//                  buffer->max_size : available storage size in buffer->buffer
// meta_in   [IN] : parameters passed to filter
// meta_out [OUT] :
//         PIPE_VALIDATE : not used
//         PIPE_FWDSIZE  : meta_out->size = needed size of meta_out for FORWARD call
//         PIPE_REVERSE  : not used
//         PIPE_FORWARD  : future meta_in for the REVERSE call
// the function returns
//         PIPE_VALIDATE : needed size for meta_out(32 bit units), negative value in case of error(s)
//         PIPE_FWDSIZE  : "worst case" size of filter output, negative value in case of error(s)
//         PIPE_FORWARD  : size of data output in bytes, negative value in case of error(s)
//         PIPE_REVERSE  : size of data output in bytes, negative value in case of error(s)

typedef ssize_t pipe_filter(uint32_t flags, array_descriptor *ap, const filter_meta *meta_in, pipe_buffer *buffer, wordstream *meta_out) ;
typedef pipe_filter *pipe_filter_pointer ;

// encode/decode array dimensions <-> unsigned integer array
int32_t decode_dimensions(array_descriptor *ap, const uint32_t w32[MAX_ARRAY_DIMENSIONS+1]);
int32_t encode_dimensions(const array_descriptor *ap, uint32_t w32[MAX_ARRAY_DIMENSIONS+1]);
// encode/decode array dimensions <-> filter struct (expected to contain ONLY dimension information)
int32_t filter_dimensions_encode(const array_descriptor *ap, filter_meta *fm);
int32_t filter_dimensions_decode(array_descriptor *ap, const filter_meta *fm);

ssize_t pipe_filter_validate(filter_meta *meta_in);
int pipe_filter_is_defined(int id);
int pipe_filter_register(int id, char *name, pipe_filter_pointer fn);
void pipe_filters_init(void);
pipe_filter_pointer pipe_filter_address(int id);
char *pipe_filter_name(int id);

ssize_t run_pipe_filters(int flags, array_descriptor *data_in, const filter_list list, wordstream *stream);
ssize_t tiled_fwd_pipe_filters(int flags, array_descriptor *data_in, const filter_list list, wordstream *stream);
ssize_t tiled_rev_pipe_filters(int flags, array_descriptor *data_out, wordstream *stream);

// #include <rmn/filter_all.h>

#endif // ! defined(PIPE_FORWARD)
