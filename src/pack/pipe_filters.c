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
#include <stdio.h>
#include <string.h>

#include <rmn/pipe_filters.h>

// maximum number of filter IDs
#define MAX_FILTERS         256
// maximum length of a filter chain
#define MAX_FILTER_CHAIN     32
// maximum length of a filter name (including the terminating null character)
#define MAX_NAME_LENGTH      16

typedef struct{
  pipe_filter_pointer fn ;       // address of filter function
  char name[MAX_NAME_LENGTH] ;   // filter function name
} filter_table_element ;

// table containing the known filters
// filter_table[nn] : entry for filer with ID = nn
static filter_table_element filter_table[MAX_FILTERS] ;

// validate meta_in, return 0 if valid, negative number if errors
// meta_in [IN] : pointer to filter metadata to validate
// function returns -1 in case of error, non negative value from filter if O.K.
ssize_t pipe_filter_validate(filter_meta *meta_in){
  uint32_t id = meta_in->id ;
  if(id > (MAX_FILTERS -1))       goto error ;      // invalid ID
  if(filter_table[id].fn == NULL) goto error ;      // undefined filter ID
  return (*filter_table[id].fn)(PIPE_VALIDATE, NULL, meta_in, NULL, NULL) ;  // validation call to filter
error :
  return -1 ;
}

// id [IN] : filter id
// return adddress of filter with this ID (NULL if ID invalid or filter not defined)
pipe_filter_pointer pipe_filter_address(int id){
  if(id > MAX_FILTERS-1) return NULL ;     // invalid id
  return filter_table[id].fn ;
}

// id [IN] : filter id
// return name of filter with this ID (NULL if ID invalid or filter not defined)
char *pipe_filter_name(int id){
  if(id > MAX_FILTERS-1) return NULL ;     // invalid id
  return filter_table[id].name ;
}

// id [IN] : id to check for validity
// return 0 if filter with ID is defined, -1 if invalid ID, 1 if filter is not defined
int pipe_filter_is_defined(int id){
  if(id > MAX_FILTERS-1) return -1 ;               // invalid id
  return (filter_table[id].fn == NULL) ? 1 : 0 ;   // 0 if defined, 1 if not
}

// register a pipe filter
// id     [IN] : id for this pipe filter (0 < ID < MAX_FILTERS)
// name   [IN] : name of pipe filter (ta most MAX_NAME_LENGTH characters)
// fn     [IN] : address of pipe filter function
// return 0 if successful, -1 otherwise
int pipe_filter_register(int id, char *name, pipe_filter_pointer fn){
fprintf(stderr, "filter_register : '%s' at %p\n", name, fn) ;
  if(id >= MAX_FILTERS || id <= 0) return -1 ;                             // bad id
  if(filter_table[id].fn != 0 && filter_table[id].fn != fn) return -1 ;    // id already in use for another function
  filter_table[id].fn = fn ;                                               // filter function address
  strncpy(filter_table[id].name, name, sizeof(filter_table[id].name)-1) ;  // copy first chars of name
  filter_table[id].name[sizeof(filter_table[id].name)-1] = '\0' ;          // make sure it is null terminated
  return 0 ;
}

// check validity of the list of pipe filters (and that arguments to filters are acceptable)
// list [IN] : NULL terminated array of pointers to filter input metadata
// return number of valid filters is successful, negative index of first filter in error otherwise
int filter_list_valid(const filter_list list){
  int i ;

  for(i=0 ; list[i] != NULL ; i++){                    // loop until NULL terminator
    if(pipe_filter_validate(list[i]) != 0) return -(i+1) ;  // error at filter i
  }
  return i - 1 ;                                       // number of valid filters
}

// initialize table with known filters
void pipe_filters_init(void){
  pipe_filter_register(254, "test254", pipe_filter_254) ;
  pipe_filter_register(100, "lin_quantizer", pipe_filter_100) ;
  pipe_filter_register(000, "dummy filter", pipe_filter_000) ;
//   pack_filter_register(254, "test253", pack_filter_253) ;
//   pack_filter_register(001, "linear1", pack_filter_001) ;
}

// encode dimensions into a filter_meta struct
// ad    [IN] : pointer to array dimension struct (see pipe_filters.h)
// fm   [OUT] : pointer to filter (id = 0) metadata with encoded dimensions
int32_t filter_dimensions_encode(const array_dimensions *ad, filter_meta *fm){
  uint32_t i, ndims = ad->ndims, esize = ad->esize ;
  uint32_t maxdim = ad->nx[0] ;

  fm->id = 0 ;
  for(i=1 ; i<ndims ; i++) maxdim = (ad->nx[i] > maxdim) ? ad->nx[i] : maxdim ;  // largest dimension
  if(maxdim < 256) {                     // 8 bits per value (ndims + 1 values)
    fm->size = 1 + ((ndims+1+3) >> 2) ;  // size will be 2 or 3
    fm->flags = 1 ;
    fm->meta[0] = (ad->nx[0] << 24) | (ad->nx[1] << 16) | (ad->nx[2] << 8) | esize << 4 | ndims ;
    if(ndims > 3) fm->meta[1] = (ad->nx[3] << 24) | (ad->nx[4] << 16) ;
  }else if(maxdim < 65536){              // 16 bits per value (ndims + 1 values)
    fm->size = 1 + ((ndims+1+1) >> 1) ;  // size will be 2, 3, or 4
    fm->flags = 2 ;
    fm->meta[0] = (ad->nx[0] << 16) | esize << 4 | ndims ;
    if(ndims > 1) fm->meta[1] = (ad->nx[1] << 16) | ad->nx[2] ;
    if(ndims > 3) fm->meta[2] = (ad->nx[3] << 16) | ad->nx[4] ;
  }else{                                 // 32 bits per value
    fm->size = 1 + ndims + 1 ;           // size will be 3, 4, 5, 6, or 7
    fm->flags = 3 ;
    fm->meta[0] =  esize << 4 | ndims ;
    for(i=0 ; i<ndims ; i++) fm->meta[i+1] = ad->nx[i] ;
  }
// fprintf(stderr,"filter_dimensions_encode ndims %d flags %d", ndims, fm->flags);
// for(i=0 ; i<fm->size-1 ; i++) fprintf(stderr," %8.8x", fm->meta[i]); fprintf(stderr,"\n") ;
  return fm->size ;
}

// decode dimensions from a filter_meta struct
// ad   [OUT] : pointer to array dimension struct (see pipe_filters.h)
// fm    [IN] : pointer to filter (id = 0) metadata with encoded dimensions
int32_t filter_dimensions_decode(array_dimensions *ad, const filter_meta *fm){
  uint32_t i, ndims, esize ;

  if(fm->id != 0) return 0 ;      // ERROR, filter id MUST be 0
  *ad = array_dimensions_null ;
  ndims = fm->meta[0] & 0xF ;
  esize = (fm->meta[0] >> 4) & 0xF ;
//   fprintf(stderr,"filter_dimensions_decode ndims %d flags %d", ndims, fm->flags);
//   for(i=0 ; i<fm->size-1 ; i++) fprintf(stderr," %8.8x", fm->meta[i]); fprintf(stderr,"\n") ;
  if(fm->flags == 1){
    ad->nx[0] = fm->meta[0] >> 24 ; ad->nx[1] = (fm->meta[0] >> 16) & 0xFF ; ad->nx[2] = (fm->meta[0] >> 8) & 0xFF ; 
    if(ndims > 3) { ad->nx[3] = fm->meta[1] >> 24 ; ad->nx[4] = (fm->meta[1] >> 16) & 0xFF ; }
  }else if(fm->flags == 2){
    ad->nx[0] = fm->meta[0] >> 16 ;
    if(ndims > 1) {ad->nx[1] = fm->meta[1] >> 16 ; ad->nx[2] = fm->meta[1] & 0xFFFF ; } ;
    if(ndims > 3) {ad->nx[3] = fm->meta[2] >> 16 ; ad->nx[4] = fm->meta[2] & 0xFFFF ; } ;
  }else if(fm->flags == 3){
    for(i=0 ; i<ndims ; i++) ad->nx[i] = fm->meta[i+1] ;
  }
  ad->ndims = ndims ;
  ad->esize = esize ;
  for(i=ndims ; i<ARRAY_DESCRIPTOR_MAXDIMS ; i++) ad->nx[i] = 1 ;
  return fm->size ;
}

// ssize_t pipe_filter(uint32_t flags, array_dimensions *ad, filter_meta *meta_in, pipe_buffer *buffer, wordstream *meta_out)
// run a filter cascade on input data
// flags       [IN] : flags for the cascade
// ad          [IN] : dimensions of input data
// data        [IN] : input data (forward mode)
//            [OUT] : output data (reverse mode)
// list        [IN] : list of filters to be run
// stream     [OUT] : cascade result in forward mode
//             [IN] : input to reverse filter in reverse mode
ssize_t run_pipe_filters(int flags, array_descriptor *data_in, const filter_list list, wordstream *stream){
  int i ;
  int esize = data_in->adim.esize ;
  array_dimensions dim = data_in->adim ;
  array_dimensions *pdim = &dim ;
  pipe_buffer pbuf ;
  int32_t status = 0 ;
  uint32_t pop_stream ;
  int in_place = flags & PIPE_INPLACE, ndata, fdata ;

  if(flags & PIPE_FORWARD){
    ndata = array_data_values(data_in) ;
    ssize_t m_outsize ;
    filter_meta *m_out ;
    struct{
      FILTER_PROLOG ;
      int32_t meta[ARRAY_DESCRIPTOR_MAXDIMS] ;
    } meta_end = {.id = 0 , .size = 1, .flags = 0, .meta = {[0 ... ARRAY_DESCRIPTOR_MAXDIMS - 1] = 0 } } ;

    pop_stream = WS32_IN(*stream) ;                         // start of what will be metadata part of the stream
    pbuf.max_size = pbuf.used = ndata * esize ;             // input (and possibly output) data dimension
    if(in_place){
      pbuf.buffer = data_in->data ;                         // in place processing
    }else{
      pbuf.buffer = malloc(pbuf.max_size) ;                 // allocate local buffer
      memcpy(pbuf.buffer, data_in->data, pbuf.max_size) ;   // copy input data
    }
    for(i=0 ; list[i] != NULL ; i++){
      filter_meta *meta = list[i] ;
      int id = meta->id ;
      pipe_filter_pointer fn = pipe_filter_address(id) ;

      m_outsize = (*fn)(PIPE_VALIDATE, NULL, list[i], NULL, NULL) ;
      m_out = ALLOC_META(m_outsize) ;
      m_out->size = m_outsize ;
      if((*fn)(flags, pdim, list[i], &pbuf, stream) <= 0){
        fprintf(stderr, "run_pipe_filters : ERROR in forward filter\n") ;
        if(! in_place) {
          free(pbuf.buffer) ;
          status = -1 ;
          goto end ;
        }
      }
    }
    // ad last filter (id = 000) that will contain input dimensions to last filter
    filter_dimensions_encode((array_dimensions *)pdim, (filter_meta *)(&meta_end)) ;
    ws32_insert(stream, &meta_end, meta_end.size) ;                      // insert last filter metadata into wordstream
    status = WS32_IN(*stream) - pop_stream ;                             // length of added metadata

    if(! in_place)
      ws32_insert(stream, pbuf.buffer, pipe_buffer_words_used(&pbuf)) ;  // copy final data filter output into wordstream
  }else if(flags & PIPE_REVERSE){
    // build filter table and apply filters in reverse order
    int fnumber = 0 ;
    filter_meta *m_rev ;
    filter_meta *filters[MAX_FILTER_CHAIN] ;
    array_dimensions out_dims ;

    pop_stream = WS32_OUT(*stream) ;                // start of metadata part of the stream
    for(i=0 ; i<MAX_FILTER_CHAIN ; i++){            // get metadata 
      m_rev = (filter_meta *) ws32_data(stream) ;   // pointer to reverse filter metadata
//           fprintf(stderr, "filter id = %d, filter size = %d, filter flags = %d [ %8.8x %8.8x]\n", 
//                   m_rev->id, m_rev->size, m_rev->flags, m_rev->meta[0], m_rev->meta[1]) ;
      ws32_skip(stream, m_rev->size) ;              // skip filter metadata
      status = WS32_OUT(*stream) - pop_stream ;     // update length of skipped metadata
      if(m_rev->id == 0) break ;                    // last filter in filter chain
      filters[fnumber++] = m_rev ;                  // insert into filter list
    }
//         fprintf(stderr, "%d filters to apply in reverse order, status = %d\n", fnumber, status) ;
    filter_dimensions_decode(&out_dims, m_rev) ;
    ndata = array_data_values(data_in) ;            // number of data values in data_in
    fdata = filter_data_values(&out_dims) ;
    pbuf.buffer   = data_in->data ;    // point to data array
    pbuf.max_size = ndata * sizeof(uint32_t) ;
    pbuf.used     = fdata * sizeof(uint32_t) ;
    if(! in_place) {
      memcpy(data_in->data, ws32_data(stream), pbuf.used) ;  // copy stream data to data_in ;
    }

    for(i=fnumber ; i>0 ; i--){     // apply reverse filters in reverse order
      m_rev = filters[fnumber-1] ;
      fnumber-- ;
      pipe_filter_pointer fnr = pipe_filter_address(m_rev->id) ;
//       fprintf(stderr, "applying reverse filter no %d  %8.8x %8.8x\n", m_rev->id, m_rev->meta[0], m_rev->meta[1]) ;
      (*fnr) (flags, &out_dims, m_rev, &pbuf, NULL) ;  // call filter using run_pipe_filters flags
    }
    if(! in_place) memcpy(data_in->data, pbuf.buffer, pbuf.used) ;
  }
end:
  return status ;
}

