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
#include <stdlib.h>
#include <string.h>

#include <rmn/pipe_filters.h>
// all possible filters
#include <rmn/filter_all.h>

#include <rmn/misc_operators.h>

// maximum number of filter IDs
#define MAX_FILTERS         256
// maximum length of a filter chain
#define MAX_FILTER_CHAIN     32
// maximum length of a filter name (including the terminating null character)
#define MAX_NAME_LENGTH      32

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
  if(fn == NULL) {
    fprintf(stderr, "WARNING: undefined filter '%s', id = %d\n", name, id) ;
    return 1 ;
  }
fprintf(stderr, "filter_register(%3d) : '%32s' at %p\n", id, name, fn) ;
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
  pipe_filter_register(000, "dimensions",        pipe_filter_000) ;
  pipe_filter_register(001, "array properties",  pipe_filter_001) ;
  pipe_filter_register(100, "linear quantizer",  pipe_filter_100) ;
  pipe_filter_register(101, "lorenzo predictor", pipe_filter_101) ;
  pipe_filter_register(110, "simple packer",     pipe_filter_110) ;
  pipe_filter_register(252, "unknown",           pipe_filter_252) ;  // testing weak external support
  pipe_filter_register(253, "unknown",           pipe_filter_253) ;  // testing weak external support
  pipe_filter_register(254, "integer saxpy",     pipe_filter_254) ;
  pipe_filter_register(255, "diagnostics",       pipe_filter_255) ;
}

// decode dimensions encoded in w32, copy them into array_descriptor
// return number of dimensions, 0 in case of error
// ap   [OUT] : array descriptor
// w32   [IN] : encoded dimensions
int32_t decode_dimensions(array_descriptor *ap, const uint32_t w32[MAX_ARRAY_DIMENSIONS+1]){
  uint32_t i, ndims = w32[0] >> 29 ;       // top 3 bits
  uint32_t nb0 = (w32[0] >> 24) & 0x1F ;   // next 5 bits
  uint32_t w = w32[0] & 0xFFFFFF ;         // lower 24 bits

  if(ndims > 5) return 0 ;

  ap->ndims = ndims ;
  if(nb0 > 15){      // 24 or 32 bits needed for dimensions
    int offset = (nb0 > 24) ? 1 : 0 ;
    ap->nx[0] = (offset == 0) ? w32[0] & 0xFFFFFF : w32[1] ;
    for(i=1 ; i<ndims ; i++) ap->nx[i] = w32[i+offset] ;
  }else{             // 16, 12, 10, 8, 6 bits needed for dimensions
    switch (ndims) {
      case 1:        // one uint32_t
        ap->nx[0] = w ;
        break;
      case 2:        // one uint32_t if 12 bits or less, two if 16 bits
        if(nb0 < 12) { ap->nx[0] = w & 0xFFF  ; ap->nx[1] = w >> 12 ; }
        else         { ap->nx[0] = w & 0xFFFF ; ap->nx[1] = w32[1] ; }
        break;
      case 3:        // one uint32_t if 8 bits or less, two if 10/12/16 bits
        if(nb0 <  8) { ap->nx[0] = w & 0xFF ; ap->nx[1] = (w >> 8) & 0xFF ; ap->nx[2] = (w >> 16) & 0xFF ; }
        else         { ap->nx[0] = w & 0xFFFF ; ap->nx[1] = w32[1] & 0xFFFF ; ap->nx[2] = (w32[1] >> 16) & 0xFFFF ; }
        break;
      case 4:        // one uint32_t if 6 bits or less, two if 12 bits or less, three if 16 bits
        if(nb0 <  6)      { ap->nx[0] = w & 0x3F ; ap->nx[1] = (w >> 6) & 0x3F ; ap->nx[2] = (w >> 12) & 0x3F ; ap->nx[3] = (w >> 18) & 0x3F ; }
        else if(nb0 < 12) { ap->nx[0] = w & 0xFFF ;  ap->nx[1] = w >> 12 ; ap->nx[2] = w32[1] & 0xFFF ; ap->nx[3] = (w32[1] >> 12) & 0xFFF ; }
        else              { ap->nx[0] = w & 0xFFFF ; ap->nx[1] = w32[1] & 0xFFFF ; ap->nx[2] = (w32[1] >> 16) & 0xFFFF ; ap->nx[3] = w32[2] & 0xFFFF ; }
        break;
      case 5:        // two uint32_t if 10 bits or less, three if 12 or 16 bits
        if(nb0 < 10) { ap->nx[0] = w & 0x3FF ; ap->nx[1] = (w >> 10) & 0x3FF ; 
                       ap->nx[2] = w32[1] & 0x3FF ; ap->nx[3] = (w32[1] >> 10) & 0x3FF ; ap->nx[4] = (w32[1] >> 20) & 0x3FF ; }
        else         { ap->nx[0] = w & 0xFFFF ; ap->nx[1] = w32[1] & 0xFFFF ; ap->nx[2] = (w32[1] >> 16) & 0xFFFF ; 
                       ap->nx[3] = w32[2] & 0xFFFF ;ap->nx[4] = (w32[2] >> 16) & 0xFFFF ; }
        break;
      default:
        break;
    }
  }
  return ndims ;
}

// encode dimensions from array_descriptor into unsigned integer array w32 (max MAX_ARRAY_DIMENSIONS+1 uint32_t elements)
// return number of uint32_t needed (0 in case of error)
// used by filters that alter incoming dimensions (saved for the reverse filter)
// ap   [IN] : array descriptor (the function uses only dimensions and number of dimensions)
// w32 [OUT] : unsigned integer array (up to MAX_ARRAY_DIMENSIONS+1 elements are used)
int32_t encode_dimensions(const array_descriptor *ap, uint32_t w32[MAX_ARRAY_DIMENSIONS+1]){
  uint32_t i, ndims = ap->ndims ;
  uint32_t maxdim = ap->nx[0], nb0, nw = 0, w0 ;

  // w32[0] = ndims:3, nb0:5, data:24
  if(ndims > MAX_ARRAY_DIMENSIONS) return 0 ;

  for(i=1 ; i<ndims ; i++) maxdim = (ap->nx[i] > maxdim) ? ap->nx[i] : maxdim ;  // largest dimension
  if(maxdim < 64)            nb0 =  5 ;
  else if(maxdim <      256) nb0 =  7 ;
  else if(maxdim <     1024) nb0 =  9 ;
  else if(maxdim <     4096) nb0 = 11 ;
  else if(maxdim <    65536) nb0 = 15 ;
  else if(maxdim < 16777216) nb0 = 23 ;
  else                       nb0 = 31 ;
  w0 = (ndims << 29) | ((nb0-1) << 24) ;
  w32[0] = 0 ;
  if(nb0 > 15){      // 24 or 32 bits needed for dimensions
    int offset = (nb0 > 24) ? 1 : 0 ;
    for(i=0 ; i<ndims ; i++) w32[i+offset] = ap->nx[i] ;
    nw = offset + ndims ;
  }else{             // 16, 12, 10, 8, 6 bits needed for dimensions
    switch (ndims) {
      case 1:        // one uint32_t
        w32[0] = ap->nx[0] ; nw = 1 ;
        break;
      case 2:        // one uint32_t if 12 bits or less (24), two if 16 bits (16, 16)
        if(nb0 < 12) { w32[0] = ( ap->nx[0] | (ap->nx[1] << 12) ) ; nw = 1 ; }
        else         { w32[0] = ap->nx[0] ; w32[1]  = ap->nx[1]   ; nw = 2 ; }
        break;
      case 3:        // one uint32_t if 8 bits or less (24) , two if 10/12/16 bits (16, 32)
        if(nb0 <  8) { w32[0] = ( ap->nx[0] | (ap->nx[1] << 8) | (ap->nx[2] << 16) ) ; nw = 1 ; }  // 3 x 8 per uint32_t
        else         { w32[0] = ap->nx[0] ; w32[1]  = ap->nx[1] | (ap->nx[2] << 16) ; nw = 2 ; }   // 1, 2 x 16 per uint32_t
        break;
      case 4:        // one uint32_t if 6 bits or less (24), two if 12 bits or less (24, 24), 3 if 16 bits (16, 32, 16)
        if(nb0 <  6)      { w32[0] = ( ap->nx[0] | (ap->nx[1] << 6) | (ap->nx[2] << 12) | (ap->nx[3] << 18) ) ;  nw = 1 ; }   // 4 x 6 per uint32_t
        else if(nb0 < 12) { w32[0] = ( ap->nx[0] | (ap->nx[1] << 12) ) ; w32[1] = ap->nx[2] | (ap->nx[3] << 12) ; nw = 2 ; }   // 2,2 x 12 per uint32_t
        else              { w32[0] = ap->nx[0] ; w32[1] = ap->nx[1] | (ap->nx[2] << 16) ; w32[2] = ap->nx[3] ; nw = 3 ; }   // 1,2,1 x 16 per uint32_t
        break;
      case 5:        // two uint32_t if 10 bits or less (20, 30), three if 12 or 16 bits (16, 32, 32)
        if(nb0 < 10) { w32[0] = ( ap->nx[0] | (ap->nx[1] << 10) ) ; w32[1] = ap->nx[2] | (ap->nx[3] << 10) | (ap->nx[4] << 20) ; nw = 2 ; }  // 2,3 x 10 per uint32_t
        else         { w32[0] = ap->nx[0] ; w32[1] = ap->nx[1] | (ap->nx[2] << 16) ; w32[2] = ap->nx[3] | (ap->nx[4] << 16) ; nw = 3 ; }  // 1,2,2 x 16 per uint32_t
        break;
      default:
        break;
    }
  }
  w32[0] |= w0 ;
  return nw ;
}

// encode dimensions into a filter_meta struct
// ap    [IN] : pointer to array dimension struct (see pipe_filters.h)
// fm   [OUT] : pointer to filter (id = 0) metadata with encoded dimensions
int32_t filter_dimensions_encode(const array_descriptor *ap, filter_meta *fm){
  uint32_t esize = ap->esize ;

  fm->id = 0 ;                     // set filter id to 0
  fm->flags = 0 ;                  // encode esize into flags
  if(esize == 2) fm->flags = 1 ;   // 16 bits
  if(esize == 4) fm->flags = 2 ;   // 32 bits
  if(esize == 8) fm->flags = 3 ;   // 64 bits
  fm->size = 1 + encode_dimensions(ap, fm->meta) ;
  return fm->size ;
#if 0
  for(i=1 ; i<ndims ; i++) maxdim = (ap->nx[i] > maxdim) ? ap->nx[i] : maxdim ;  // largest dimension
  if(maxdim < 256) {                     // 8 bits per value (ndims + 1 values)
    fm->size = 1 + ((ndims+1+3) >> 2) ;  // size will be 2 or 3
    fm->flags = 1 ;
    fm->meta[0] = (ap->nx[0] << 24) | (ap->nx[1] << 16) | (ap->nx[2] << 8) | esize << 4 | ndims ;
    if(ndims > 3) fm->meta[1] = (ap->nx[3] << 24) | (ap->nx[4] << 16) ;
  }else if(maxdim < 65536){              // 16 bits per value (ndims + 1 values)
    fm->size = 1 + ((ndims+1+1) >> 1) ;  // size will be 2, 3, or 4
    fm->flags = 2 ;
    fm->meta[0] = (ap->nx[0] << 16) | esize << 4 | ndims ;
    if(ndims > 1) fm->meta[1] = (ap->nx[1] << 16) | ap->nx[2] ;
    if(ndims > 3) fm->meta[2] = (ap->nx[3] << 16) | ap->nx[4] ;
  }else{                                 // 32 bits per value
    fm->size = 1 + ndims + 1 ;           // size will be 3, 4, 5, 6, or 7
    fm->flags = 3 ;
    fm->meta[0] =  esize << 4 | ndims ;
    for(i=0 ; i<ndims ; i++) fm->meta[i+1] = ap->nx[i] ;
  }
// fprintf(stderr,"filter_dimensions_encode ndims %d flags %d", ndims, fm->flags);
// for(i=0 ; i<fm->size-1 ; i++) fprintf(stderr," %8.8x", fm->meta[i]); fprintf(stderr,"\n") ;
  return fm->size ;
#endif
}

// decode dimensions from a filter_meta struct
// ap   [OUT] : pointer to array dimension struct (see pipe_filters.h)
// fm    [IN] : pointer to filter (id = 0) metadata with encoded dimensions
int32_t filter_dimensions_decode(array_descriptor *ap, const filter_meta *fm){
  uint32_t ndims ;
  uint32_t elist[4] = { 1, 2, 4, 8 } ;

  if(fm->id != 0) return 0 ;         // ERROR, filter id MUST be 0
  *ap = array_descriptor_base ;
  ap->esize = elist[fm->flags] ;     // get esize from flags
  ndims = decode_dimensions(ap, fm->meta) ;
  return ndims ;
#if 0
  if(fm->id != 0) return 0 ;      // ERROR, filter id MUST be 0
  *ap = array_descriptor_base ;   // set version number to software version
  ndims = fm->meta[0] & 0xF ;
  esize = (fm->meta[0] >> 4) & 0xF ;
//   fprintf(stderr,"filter_dimensions_decode ndims %d flags %d", ndims, fm->flags);
//   for(i=0 ; i<fm->size-1 ; i++) fprintf(stderr," %8.8x", fm->meta[i]); fprintf(stderr,"\n") ;
  if(fm->flags == 1){
    ap->nx[0] = fm->meta[0] >> 24 ; ap->nx[1] = (fm->meta[0] >> 16) & 0xFF ; ap->nx[2] = (fm->meta[0] >> 8) & 0xFF ; 
    if(ndims > 3) { ap->nx[3] = fm->meta[1] >> 24 ; ap->nx[4] = (fm->meta[1] >> 16) & 0xFF ; }
  }else if(fm->flags == 2){
    ap->nx[0] = fm->meta[0] >> 16 ;
    if(ndims > 1) {ap->nx[1] = fm->meta[1] >> 16 ; ap->nx[2] = fm->meta[1] & 0xFFFF ; } ;
    if(ndims > 3) {ap->nx[3] = fm->meta[2] >> 16 ; ap->nx[4] = fm->meta[2] & 0xFFFF ; } ;
  }else if(fm->flags == 3){
    for(i=0 ; i<ndims ; i++) ap->nx[i] = fm->meta[i+1] ;
  }
  ap->ndims = ndims ;
  ap->esize = esize ;
  for(i=ndims ; i<MAX_ARRAY_DIMENSIONS ; i++) ap->nx[i] = 1 ;
  return fm->size ;
#endif
}

// ssize_t pipe_filter(uint32_t flags, array_descriptor *ap, filter_meta *meta_in, pipe_buffer *buffer, wordstream *meta_out)
// run a filter cascade on input data
// flags       [IN] : flags for the cascade
// data_in     [IN] : input data (forward mode)
//            [OUT] : output data (reverse mode)
// list        [IN] : list of filters to be run (forward mode only, ignored in reverse mode)
// stream     [OUT] : cascade result in forward mode
//             [IN] : input to reverse filter cascade in reverse mode
// return length in 32 bit words of added metadata (forward mode), length of read metadata (reverse mode)
ssize_t run_pipe_filters(int flags, array_descriptor *data_in, const filter_list list, wordstream *stream){
  int i ;
  int esize = data_in->esize ;
  array_descriptor dim = *data_in ;
  array_descriptor *pdim = &dim ;
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
      int32_t meta[MAX_ARRAY_DIMENSIONS+1] ;
    } meta_end = {.id = 0 , .size = 1, .flags = 0, .meta = {[0 ... MAX_ARRAY_DIMENSIONS - 1] = 0 } } ;

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
      int lsize = list[i]->size-1 ;
      fprintf(stderr, "applying forward filter no %d(%d)  %8.8x %8.8x\n", 
                       list[i]->id, lsize, (lsize > 0) ? list[i]->meta[0] : 0, (lsize > 1) ? list[i]->meta[1] : 0) ;
      if((*fn)(flags, pdim, list[i], &pbuf, stream) <= 0){
        fprintf(stderr, "run_pipe_filters : ERROR in forward filter\n") ;
        if(! in_place) {
          free(pbuf.buffer) ;
          status = -1 ;
          goto end ;
        }
      }
    }
    // ap last filter (id = 000) that will contain input dimensions to last filter
    filter_dimensions_encode((array_descriptor *)pdim, (filter_meta *)(&meta_end)) ;
    ws32_insert(stream, &meta_end, meta_end.size) ;                      // insert last filter metadata into wordstream
    status = WS32_IN(*stream) - pop_stream ;                             // length of added metadata

    if(! in_place)
      ws32_insert(stream, pbuf.buffer, pipe_buffer_words_used(&pbuf)) ;  // copy final data filter output into wordstream
  }else if(flags & PIPE_REVERSE){
    // build filter table and apply filters in reverse order
    int fnumber = 0 ;
    filter_meta *m_rev ;
    filter_meta *filters[MAX_FILTER_CHAIN] ;
    array_descriptor out_dims ;

    pop_stream = WS32_OUT(*stream) ;                // start of metadata part of the stream
    for(i=0 ; i<MAX_FILTER_CHAIN ; i++){            // get metadata 
      m_rev = (filter_meta *) ws32_data(stream) ;   // pointer to reverse filter metadata
      ws32_skip_out(stream, m_rev->size) ;          // skip filter metadata
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
    ws32_skip_out(stream, fdata) ;                  // skip data from stream

    for(i=fnumber ; i>0 ; i--){     // apply reverse filters in reverse order
      m_rev = filters[fnumber-1] ;
      fnumber-- ;
      pipe_filter_pointer fnr = pipe_filter_address(m_rev->id) ;
      int lsize = m_rev->size-1 ;
      fprintf(stderr, "applying reverse filter no %d(%d)  %8.8x %8.8x\n", 
                      m_rev->id, lsize, (lsize > 0) ? m_rev->meta[0] : 0, (lsize > 1) ? m_rev->meta[1] : 0) ;
      (*fnr) (flags, &out_dims, m_rev, &pbuf, NULL) ;  // call filter using run_pipe_filters flags
    }
    if(! in_place) memcpy(data_in->data, pbuf.buffer, pbuf.used) ;  // copy pipe buffer contents into user data
    // NOTE : may have to free pbuf.buffer under some circumstances
  }
end:
  return status ;
}

// ssize_t run_pipe_filters(int flags, array_descriptor *data_in, const filter_list list, wordstream *stream)
ssize_t tiled_fwd_pipe_filters(int flags, array_descriptor *data_in, const filter_list list, wordstream *stream){
  array_descriptor ad = *data_in ;            // import information from global array
  uint32_t blki  = ad.tilex ;              // tile size along 1st dimension
  uint32_t blkj  = ad.tiley ;              // tile size along 2nd dimension
  uint32_t *array = (uint32_t *) ad.data ;    // base address of global array
  uint32_t ndims = ad.ndims ;              // number of dimensions of global array
  ssize_t nbytes = 0 ;

  // if more than 2D,
  // do we want to tile using 1st dimension and product of other dimensions ?
  // or do we want a loop over other dimensions ?
  if(ndims == 2 && blki != 0 && blkj != 0){   // 2D data to be tiled for processing

    uint32_t tile[blki*blkj] ;
    int gni   = ad.nx[0] ;                 // 1st dimension of global array
    int gnj   = ad.nx[1] ;                 // 2nd dimension of global array
    int nblki = (gni + blki -1) / blki ;      // number of tiles along each dimension
    int nblkj = (gnj + blkj -1) / blkj ;
    // get current insertion address in stream (will be the starting address of data map)
    uint32_t *str0 = (uint32_t *) WS32_BUFFER_IN(*stream) ;
    WS32_INSERT1(*stream, gni)  ;
    WS32_INSERT1(*stream, gnj) ;
    WS32_INSERT1(*stream, (ad.etype << 16) | blki) ;
    WS32_INSERT1(*stream, (ad.esize << 16) | blkj) ;
    // gni, gnj, blki, blkj, row sizes ( rmap[nblkj] ), tile sizes ( tmap[nblki * nblkj] )
    uint32_t *rmap = str0 + 4, *tmap = rmap + nblkj ;
    // allocate (skip) space for data map in stream (nblkj rows + nblki * nblkj tiles)
    uint32_t mapsize = nblkj * (nblki + 1) ;
    if( ws32_skip_in(stream, mapsize) < 0) goto error ;

    int i0, j0, tni, tnj ;                           // start and dimension of tiles
    for(j0=0 ; j0<gnj ; j0+=blkj){                   // loop over tile rows
      uint32_t rowsize = 0 ;
      uint32_t *tile_base = array ;                  // lower left corner of first tile in current row
      tnj = (j0+blkj <= gnj) ? blkj : gnj-j0 ;       // tile dimension along j
      for(i0=0 ; i0 < gni ; i0+=blki){               // loop over row of tiles
        tni = (i0+blki <= gni) ? blki : gni-i0 ;     // tile dimension  along i
        ad.data = tile ;                             // point array descriptor to local tile
        ad.nx[0] = tni ; ad.nx[1] = tnj ;      // tile dimensions
        ad.n0[0] = i0  ; ad.n0[1] = j0 ;       // tile offset
        // collect local tile from global array ( array[i0:i0+tni-1,j0:j0+tnj-1] -> tile[tni,tnj] )
        if( get_word_block(tile_base, tile, tni, gni, tnj) < 0) goto error ;
        tile_base += tni ;    // lower left corner of next tile

        uint32_t istart = WS32_IN(*stream) ;         // get current position in stream
        // call filter chain with tile of dimensions (in-i0) , (jn-j0)
        if( run_pipe_filters(PIPE_FORWARD, &ad, list, stream) < 0) goto error ;
        // tile size = current position in stream - remembered position
        uint32_t tile_size = WS32_IN(*stream) - istart ;
fprintf(stderr,"tile_size = %ld\n", tile_size * sizeof(uint32_t)) ;
        *tmap = tile_size ; tmap ++ ;                // insert tile size in tile sizes table, bump pointer
        rowsize += tile_size ;                       // add tile size to size of the row of tiles
      }
      *rmap = rowsize ; rmap++ ;
      array += gni * blkj ;     // point to next row of blocks
    }
    // find number of bytes added to stream
    nbytes = ((uint32_t *) WS32_BUFFER_IN(*stream)) - str0 ;
    nbytes *= sizeof(uint32_t) ;
  }else{    // not 2D tiled data
  }
  return nbytes ;

error:
  return -1 ;
}

// ssize_t run_pipe_filters(int flags, array_descriptor *data_in, const filter_list list, wordstream *stream)
// dimensions from array descriptor MUST match dimensions from stream or be 0
// tiling information from array descriptor MUST match information from stream or be 0
// if the data pointer in array descriptor is NULL, memory will be allocated
ssize_t tiled_rev_pipe_filters(int flags, array_descriptor *data_out, wordstream *stream){
  uint32_t *str0 = (uint32_t *) WS32_BUFFER_OUT(*stream) ;
  array_descriptor ad = *data_out ;          // import information from global array
  uint32_t ndims = ad.ndims ;             // number of dimensions of global array
  uint32_t gni   = ad.nx[0] ;             // array dimensions from array descriptor
  uint32_t gnj   = ad.nx[1] ;
  uint32_t sni   = str0[0] ;                 // array dimensions from stream
  uint32_t snj   = str0[1] ;
  if(gni == 0 && gnj == 0) { gni = sni ; gnj = snj ; }
  uint32_t blki  = ad.tilex ;             // tile dimensions from array descriptor
  uint32_t blkj  = ad.tiley ;
  uint32_t sblki = str0[2] & 0xFFFF ;        // tile dimensions from stream
  uint32_t sblkj = str0[3] & 0xFFFF ;
  if(blki == 0 && blkj == 0) { blki = sblki ; blkj = sblkj ; }
  uint32_t etype = ad.etype ;             // data type from array descriptor
  uint32_t esize = ad.esize ;
  uint32_t stype = str0[2] >> 16 ;           // data type and size from stream
  uint32_t ssize = str0[3] >> 16 ;
  if(etype == 0) etype = stype ;
  if(esize == 0) esize = ssize ;
  size_t msize = 0 ;                         // will be non zero only if array has been locally allocated
  uint32_t *array = (uint32_t *) ad.data ;   // base address of global array
  ssize_t nbytes = 0 ;
  uint32_t tile[blki*blkj] ;                 // local tile allocated on stack with largest tile dimensions
  int nblki = (gni + blki -1) / blki ;       // number of tiles along each dimension
  int nblkj = (gnj + blkj -1) / blkj ;
  int nskip ;
  uint32_t *rmap, *tmap ;

  if(blki == 0 || blkj == 0) goto error ;
  if(gni != sni || gnj != snj || blki != sblki || blkj != sblkj || etype != stype || esize != ssize) goto error ;

  if(array == NULL){                         // will need to allocate output array
    msize = (esize ? esize : 4) ;
    array = (uint32_t *)malloc(msize * gni * gnj) ;
  }

fprintf(stderr, "ndims = %d, [%d x %d], (%d,%d) tiles of dimension [%d x %d], data = %s_%d\n", 
        ndims, str0[0], str0[1], nblki, nblkj, sblki, sblkj, ptype[stype], ssize*8) ;
  if(ndims != 2) goto error ;
  if( gni != str0[0] || gnj != str0[1] ) goto error ;
  // nblki, nblkj, row sizes ( rmap[nblkj] ), followed by tile sizes ( tmap[nblki * nblkj] )
  nskip = ws32_skip_out(stream, 4) ;              // skip first 4 words
  if(nskip < 0) goto error ;
  rmap = WS32_BUFFER_OUT(*stream) ;
  nskip = ws32_skip_out(stream, nblkj) ;          // skip row sizes
  if(nskip < 0) goto error ;
  tmap = WS32_BUFFER_OUT(*stream) ;
  nskip = ws32_skip_out(stream, nblki*nblkj) ;    // skip tile sizes
  if(nskip < 0) goto error ;
fprintf(stderr, "data map : ") ;
int i ;
for(i=0 ; i<nblkj ; i++) fprintf(stderr, "%10d", rmap[i]) ;
for(i=0 ; i<nblki*nblkj ; i++) fprintf(stderr, "%10d", tmap[i]) ; fprintf(stderr, "\n") ;
fprintf(stderr, "size left in stream = %d\n", WS32_FILLED(*stream)) ;

  int i0, j0, tni, tnj, nadded ;                   // start and dimension of tiles
  array_descriptor ado ;                           // array descriptor for extraction
  ado.data = tile ;
  ado.tilex = blki ; ado.tiley = blkj ;
// int put_word_block(void *restrict f, void *restrict blk, int ni, int lni, int nj)
  for(j0=0 ; j0<gnj ; j0+=blkj){                   // loop over tile rows
    tnj = (j0+blkj <= gnj) ? blkj : gnj-j0 ;       // tile dimension along j
    for(i0=0 ; i0 < gni ; i0+=blki){               // loop over row of tiles
      tni = (i0+blki <= gni) ? blki : gni-i0 ;     // tile dimension  along i
      ado.ndims = 2 ;
      ado.nx[0] = tni ;
      ado.nx[1] = tnj ;
      nadded = run_pipe_filters(PIPE_REVERSE, &ado, NULL, stream) ;
      if(nadded < 0) goto error ;
fprintf(stderr, "size left in stream = %d\n", WS32_FILLED(*stream)) ;
//       nadded = put_word_block( tile_base, tile, tni, gni, tnj ) ;
    }
  }

  return nbytes ;

error:
  if(msize != 0) free(array) ;
  return -1 ;
}
