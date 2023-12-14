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
#include <string.h>

#include <rmn/pack_filters.h>

#define MAX_FILTERS     256
#define MAX_FILTER_NAME  16

typedef struct{
  pipe_filter_pointer fn ;
  char name[MAX_FILTER_NAME] ;
} filter_table_element ;

static filter_table_element filter_table[MAX_FILTERS] ;

// translate dims[] into number of values
static int buffer_dimension(int *dims){
  int nval = 0 ;
  int i ;
  int ndims = PIPE_DATA_NDIMS(dims) ;
  if((ndims > 5) || (ndims < 0)) goto end ;
  nval = 1 ;
  for(i=1 ; i<ndims ; i++){
    nval *= dims[i] ;
  }
end:
  return nval ;
}

// test filter id = 254, scale data and add offset
// with FFLAG_VALIDATE, the only needed arguments are flags and meta_in, all other arguments should be NULL
ssize_t pipe_filter_254(uint32_t flags, int *dims, filter_meta *meta_in, pipe_buffer *buf, filter_meta *meta_out){
  int nval, i, errors ;
  ssize_t nbytes ;
  int32_t *data = (int32_t *) buf->buffer ;

  if(meta_in->id != 254)       goto error ;   // wrong ID
  if(flags & FFLAG_VALIDATE){                 // validate input flags for eventual call with FFLAG_FORWARD mode

    if(meta_in->size < 2)       errors++ ;
    if(meta_in->used != 2)      errors++ ;
    if(meta_in->meta[0].i == 0 && meta_in->meta[1].i == 0) errors++ ;
    nbytes = -errors ;
    goto end ;

  }else if(flags & FFLAG_REVERSE){            // inverse filter

    if(meta_in->size < 2)       goto error ;  // not enough space in meta_in
    if(meta_in->used != 2)      goto error ;  // wrong metadata values count
    nval = buffer_dimension(dims) ;
    nbytes = nval * sizeof(uint32_t) ;
    for(i=0 ; i<nval ; i++) data[i] = (data[i] - meta_in->meta[1].i) / meta_in->meta[0].i ;

  }else if(flags & FFLAG_FORWARD){             // forward filter

    if(meta_in->used != 2)       goto error ;  // wrong length
    if(meta_out->size < 2)       goto error ;  // too small
    nval = buffer_dimension(dims) ;
    for(i=0 ; i<nval ; i++) data[i] = data[i] * meta_in->meta[0].i + meta_in->meta[1].i ;
    meta_out->id    = 254 ;
    meta_out->used = 2 ;
    meta_out->meta[0].u = meta_in->meta[0].i ;
    meta_out->meta[1].u = meta_in->meta[1].i ;
    nbytes = nval * sizeof(uint32_t) ;

  }else{
    goto error ; // ERROR, filter is called neither in the forward nor in the reverse direction
  }
end:
  return nbytes ;
error :
  return (nbytes = -1) ;
}

// test filter id = 253, delta filter
ssize_t pipe_filter_253(uint32_t flags, int *dims, filter_meta *meta_in, pipe_buffer *buf, filter_meta *meta_out){
  ssize_t nbytes ;
error :
  return (nbytes = -1) ;
}

#if 0
ssize_t pack_filter_253(pack_flags flags, pack_meta *meta_in, pipe_buffer *buf_in, pack_meta *meta_out, pipe_buffer *buf_out){
  int errors = 0 ;
  if(FPACK_ID(flags) != 254)     goto error ;  // bad ID
  if(flags & FFLAG_REVERSE){                   // inverse filter
    if(meta_in->id != 253)       goto error ;  // bad ID
    if(meta_out->size < 2)       goto error ;  // not enough space in meta_out
    if(meta_out->id != 253)      errors++ ;
    if(meta_out->nmeta != 1)     errors++ ;
    if(meta_out->meta[0].u != 3) errors++ ;
    if(errors > 0)               goto error ;
  }else if(flags & FFLAG_FORWARD){             // forward filter
    if(meta_out->size < 2)       goto error ;  // not enough space in meta_out
    meta_out->id    = 253 ;
    meta_out->nmeta = 1 ;
    meta_out->meta[0].u = 3 ;
  }else{
    goto error ; // ERROR, filter is called neither in the forward nor in the reverse direction
  }
  return 0 ;
error :
  return -1 ;
}

// LINEAR_QUANTIZER
ssize_t pack_filter_001(pack_flags flags, pack_meta *meta_in, pipe_buffer *buf_in, pack_meta *meta_out, pipe_buffer *buf_out){
  float ref ;
  uint32_t nbits, qtype ;
  q_desc qdesc ;
  uint32_t nval ;
  ssize_t status = 0 ;

  if(FPACK_ID(flags) != 001) goto error ;
  if(meta_in->id != 001)      goto error ;
  nval = (FPACK_NI(flags) + 1) * (FPACK_NJ(flags) + 1) ;
  if(flags & FFLAG_REVERSE){                // inverse filter, restore quantized data

    if(meta_in->nmeta != 2)   goto error ;    // need 64 bits of metadata information
    qdesc.u = ((uint64_t)meta_in->meta[0].u << 32) | meta_in->meta[0].u ;   // we can now use qdesc.f (q_rules)
    // we can now call the quantizer inverse operation (restore)
    // call un-quantizer(buf, nval, q_encode desc, buf)
    // the operation will work in place
    meta_out->meta[0].u = (qdesc.u >> 32) ;           // using q_decode (optional)
    meta_out->meta[1].u = (qdesc.u & 0xFFFFFFFFu) ;

  }else if(flags & FFLAG_FORWARD){                              // forward filter, quantize data using rules from meta_in

    if(meta_in->nmeta != 3)        goto error ;
    ref   = meta_in->meta[0].f ;      // get reference value
    nbits = meta_in->meta[1].u ;      // get nbits
    qtype = meta_in->meta[2].u ;      // get quantizer type
    if(nbits == 0 && ref == 0.0f) goto error ;    // cannot be both set to 0
    if(qtype > 2)                 goto error ;    // invalid quantizer type
    if(meta_out->nmeta < 2)       goto error ;    // not enough space in meta_out
    meta_out->id = 001 ;
    // the operation will work in place
    // call quantizer(*buf, nval, q_rules rules, *buf, limits_w32 *limits, NULL)
    // get qdesc from the quantizer qdesc.q / qdesc.u
    meta_out->meta[0].u = (qdesc.u >> 32) ;
    meta_out->meta[1].u = (qdesc.u & 0xFFFFFFFFu) ;

  }else{
    goto error ; // ERROR, filter is called neither in the forward nor in the reverse direction
  }
end :
  return status ;
error :
  return -1 ;
}
#endif

// validate meta_in, return 0 if valid, negative number if errors
// meta_in [IN] : filter input metadata to validate
ssize_t filter_validate(filter_meta *meta_in){
  uint32_t id = meta_in->id ;
  if(id > (MAX_FILTERS -1))       goto error ;      // invalid ID
  if(filter_table[id].fn == NULL) goto error ;      // undefined filter ID
  return (*filter_table[id].fn)(FFLAG_VALIDATE, NULL, meta_in, NULL, NULL) ;
error :
  return -1 ;
}

// id [IN] : id to check for validity
// return adddress if filter with this ID (NULL if ID invalid or filter not defined)
pipe_filter_pointer filter_address(int id){
  if(id > MAX_FILTERS-1) return NULL ;     // invalid id
  return filter_table[id].fn ;
}

// id [IN] : id to check for validity
// return 0 if filter with ID is defined, -1 if invalid ID, 1 if filter is not defined
int filter_is_defined(int id){
  if(id > MAX_FILTERS-1) return -1 ;               // invalid id
  return (filter_table[id].fn == NULL) ? 1 : 0 ;   // 0 if defined, 1 if not
}

// register a pipe filter
// id     [IN] : id for this pipe filter (0 < ID < MAX_FILTERS)
// name   [IN] : name of pipe filter (ta most MAX_FILTER_NAME characters)
// fn     [IN] : address of pipr filter function
int filter_register(int id, char *name, pipe_filter_pointer fn){
  if(id >= MAX_FILTERS || id <= 0) return -1 ;                             // bad id
  if(filter_table[id].fn != 0 && filter_table[id].fn != fn) return -1 ;    // id already in use for another function
  filter_table[id].fn = fn ;                                               // filter function address
  strncpy(filter_table[id].name, name, sizeof(filter_table[id].name)-1) ;  // copy first chars of name
  filter_table[id].name[sizeof(filter_table[id].name)-1] = '\0' ;          // make sure it is null terminated
}

// check validity of the list of pipe filters (and that arguments to filters are acceptable)
// list [IN] : list contains the number of filters and the pointers to filter input metadata
int filter_list_valid(filter_list *list){
  int i ;

  for(i=0 ; i<list->nfilters ; i++){
    if(filter_validate(list->meta[i]) != 0) return -(i+1) ;  // error at filter i
  }
  return list->nfilters ;
}

void pipe_filter_init(void){
  filter_register(254, "test254", pipe_filter_254) ;
//   pack_filter_register(254, "test253", pack_filter_253) ;
//   pack_filter_register(001, "linear1", pack_filter_001) ;
}

// run a filter cascade on input data
// flags   [IN] : flags for the cascade
// dims    [IN] : dimensions of input data (input data is assumed to be 32 bit)
// list    [IN] : list of filters to be run
// buffer [OUT] : cascade result
ssize_t run_pipe_filters(int flags, int *dims, void *data, filter_list *list, pipe_buffer *buffer){
  int i ;
  int dsize = PIPE_DATA_SIZE(dims) ;
  for(i=0 ; i<list->nfilters ; i++){
  }
}

