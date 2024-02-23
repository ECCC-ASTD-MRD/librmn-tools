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
  pipe_filter_register(000, "dimensions",       pipe_filter_000) ;
  pipe_filter_register(001, "array properties", pipe_filter_001) ;
  pipe_filter_register(100, "linear quantizer", pipe_filter_100) ;
  pipe_filter_register(252, "unknown",          pipe_filter_252) ;  // testing weak external support
  pipe_filter_register(253, "unknown",          pipe_filter_253) ;  // testing weak external support
  pipe_filter_register(254, "integer saxpy",    pipe_filter_254) ;
  pipe_filter_register(255, "diagnostics",      pipe_filter_255) ;
}

// encode dimensions into a filter_meta struct
// ap    [IN] : pointer to array dimension struct (see pipe_filters.h)
// fm   [OUT] : pointer to filter (id = 0) metadata with encoded dimensions
int32_t filter_dimensions_encode(const array_properties *ap, filter_meta *fm){
  uint32_t i, ndims = ap->ndims, esize = ap->esize ;
  uint32_t maxdim = ap->nx[0] ;

  fm->id = 0 ;
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
}

// decode dimensions from a filter_meta struct
// ap   [OUT] : pointer to array dimension struct (see pipe_filters.h)
// fm    [IN] : pointer to filter (id = 0) metadata with encoded dimensions
int32_t filter_dimensions_decode(array_properties *ap, const filter_meta *fm){
  uint32_t i, ndims, esize ;

  if(fm->id != 0) return 0 ;      // ERROR, filter id MUST be 0
  *ap = array_properties_null ;
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
}

// ssize_t pipe_filter(uint32_t flags, array_properties *ap, filter_meta *meta_in, pipe_buffer *buffer, wordstream *meta_out)
// run a filter cascade on input data
// flags       [IN] : flags for the cascade
// data_in     [IN] : input data (forward mode)
//            [OUT] : output data (reverse mode)
// list        [IN] : list of filters to be run (forward mode only, ignored in reverse mode)
// stream     [OUT] : cascade result in forward mode
//             [IN] : input to reverse filter cascade in reverse mode
// return length in 32 bit words of added metadata
ssize_t run_pipe_filters(int flags, array_descriptor *data_in, const filter_list list, wordstream *stream){
  int i ;
  int esize = data_in->ap.esize ;
  array_properties dim = data_in->ap ;
  array_properties *pdim = &dim ;
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
      int32_t meta[MAX_ARRAY_DIMENSIONS] ;
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
    filter_dimensions_encode((array_properties *)pdim, (filter_meta *)(&meta_end)) ;
    ws32_insert(stream, &meta_end, meta_end.size) ;                      // insert last filter metadata into wordstream
    status = WS32_IN(*stream) - pop_stream ;                             // length of added metadata

    if(! in_place)
      ws32_insert(stream, pbuf.buffer, pipe_buffer_words_used(&pbuf)) ;  // copy final data filter output into wordstream
  }else if(flags & PIPE_REVERSE){
    // build filter table and apply filters in reverse order
    int fnumber = 0 ;
    filter_meta *m_rev ;
    filter_meta *filters[MAX_FILTER_CHAIN] ;
    array_properties out_dims ;

    pop_stream = WS32_OUT(*stream) ;                // start of metadata part of the stream
    for(i=0 ; i<MAX_FILTER_CHAIN ; i++){            // get metadata 
      m_rev = (filter_meta *) ws32_data(stream) ;   // pointer to reverse filter metadata
//           fprintf(stderr, "filter id = %d, filter size = %d, filter flags = %d [ %8.8x %8.8x]\n", 
//                   m_rev->id, m_rev->size, m_rev->flags, m_rev->meta[0], m_rev->meta[1]) ;
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
    ws32_skip_out(stream, fdata) ;          // skip data from stream

    for(i=fnumber ; i>0 ; i--){     // apply reverse filters in reverse order
      m_rev = filters[fnumber-1] ;
      fnumber-- ;
      pipe_filter_pointer fnr = pipe_filter_address(m_rev->id) ;
      int lsize = m_rev->size-1 ;
      fprintf(stderr, "applying reverse filter no %d(%d)  %8.8x %8.8x\n", 
                      m_rev->id, lsize, (lsize > 0) ? m_rev->meta[0] : 0, (lsize > 1) ? m_rev->meta[1] : 0) ;
      (*fnr) (flags, &out_dims, m_rev, &pbuf, NULL) ;  // call filter using run_pipe_filters flags
    }
    if(! in_place) memcpy(data_in->data, pbuf.buffer, pbuf.used) ;
  }
end:
  return status ;
}

// ssize_t run_pipe_filters(int flags, array_descriptor *data_in, const filter_list list, wordstream *stream)
ssize_t tiled_fwd_pipe_filters(int flags, array_descriptor *data_in, const filter_list list, wordstream *stream){
  array_descriptor ad = *data_in ;            // import information from global array
  uint32_t blki  = ad.ap.tilex ;              // tile size along 1st dimension
  uint32_t blkj  = ad.ap.tiley ;              // tile size along 2nd dimension
  uint32_t *array = (uint32_t *) ad.data ;    // base address of global array
  uint32_t ndims = ad.ap.ndims ;              // number of dimensions of global array
  int nadded ;
  ssize_t nbytes = 0 ;

  // if more than 2D,
  // do we want to tile using 1st dimension and product of other dimensions ?
  // or do we want a loop over other dimensions ?
  if(ndims == 2 && blki != 0 && blkj != 0){   // 2D data to be tiled for processing

    uint32_t tile[blki*blkj] ;
    int gni   = ad.ap.nx[0] ;                 // 1st dimension of global array
    int gnj   = ad.ap.nx[1] ;                 // 2nd dimension of global array
    int nblki = (gni + blki -1) / blki ;      // number of tiles along each dimension
    int nblkj = (gnj + blkj -1) / blkj ;
    // get current insertion address in stream (will be the starting address of data map)
    uint32_t *str0 = (uint32_t *) WS32_BUFFER_IN(*stream) ;
    WS32_INSERT1(*stream, gni)  ;
    WS32_INSERT1(*stream, gnj) ;
    WS32_INSERT1(*stream, (ad.ap.etype << 16) | blki) ;
    WS32_INSERT1(*stream, (ad.ap.esize << 16) | blkj) ;
    // gni, gnj, blki, blkj, row sizes ( rmap[nblkj] ), tile sizes ( tmap[nblki * nblkj] )
    uint32_t *rmap = str0 + 4, *tmap = rmap + nblkj ;
    // allocate (skip) space for data map in stream (nblkj rows + nblki * nblkj tiles)
    uint32_t mapsize = nblkj * (nblki + 1) ;
    if( (nadded = ws32_skip_in(stream, mapsize)) < 0) goto error ;

    int i0, j0, tni, tnj ;                           // start and dimension of tiles
    for(j0=0 ; j0<gnj ; j0+=blkj){                   // loop over tile rows
      uint32_t rowsize = 0 ;
      uint32_t *tile_base = array ;                  // lower left corner of first tile in current row
      tnj = (j0+blkj <= gnj) ? blkj : gnj-j0 ;       // tile dimension along j
      for(i0=0 ; i0 < gni ; i0+=blki){               // loop over row of tiles
        tni = (i0+blki <= gni) ? blki : gni-i0 ;     // tile dimension  along i
        ad.data = tile ;                             // point array descriptor to local tile
        ad.ap.nx[0] = tni ; ad.ap.nx[1] = tnj ;      // tile dimensions
        ad.ap.n0[0] = i0  ; ad.ap.n0[1] = j0 ;       // tile offset
        // collect local tile from global array ( array[i0:i0+tni-1,j0:j0+tnj-1] -> tile[tni,tnj] )
        if( (nadded = get_word_block(tile_base, tile, tni, gni, tnj)) < 0) goto error ;
        tile_base += tni ;    // lower left corner of next tile

        uint32_t istart = WS32_IN(*stream) ;         // get current position in stream
        // call filter chain with tile of dimensions (in-i0) , (jn-j0)
        if( (nadded = run_pipe_filters(PIPE_FORWARD, &ad, list, stream)) < 0) goto error ;
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
  uint32_t ndims = ad.ap.ndims ;             // number of dimensions of global array
  uint32_t gni   = ad.ap.nx[0] ;             // array dimensions from array descriptor
  uint32_t gnj   = ad.ap.nx[1] ;
  uint32_t sni   = str0[0] ;                 // array dimensions from stream
  uint32_t snj   = str0[1] ;
  if(gni == 0 && gnj == 0) { gni = sni ; gnj = snj ; }
  uint32_t blki  = ad.ap.tilex ;             // tile dimensions from array descriptor
  uint32_t blkj  = ad.ap.tiley ;
  uint32_t sblki = str0[2] & 0xFFFF ;        // tile dimensions from stream
  uint32_t sblkj = str0[3] & 0xFFFF ;
  if(blki == 0 && blkj == 0) { blki = sblki ; blkj = sblkj ; }
  uint32_t etype = ad.ap.etype ;             // data type from array descriptor
  uint32_t esize = ad.ap.esize ;
  uint32_t stype = str0[2] >> 16 ;           // data type and size from stream
  uint32_t ssize = str0[3] >> 16 ;
  if(etype == 0) etype = stype ;
  if(esize == 0) esize = ssize ;
  size_t msize = 0 ;                         // will be non zero only if array has been locally allocated
  uint32_t *array = (uint32_t *) ad.data ;   // base address of global array
  ssize_t nbytes = 0 ;
  uint32_t tile[blki*blkj] ;                 // local tile allocated on stack with largest tile dimensions

  if(blki == 0 || blkj == 0) goto error ;
  if(gni != sni || gnj != snj || blki != sblki || blkj != sblkj || etype != stype || esize != ssize) goto error ;

  if(array == NULL){                         // will need to allocate output array
    msize = (esize ? esize : 4) ;
    array = (uint32_t *)malloc(msize * gni * gnj) ;
  }
  int nblki = (gni + blki -1) / blki ;       // number of tiles along each dimension
  int nblkj = (gnj + blkj -1) / blkj ;

fprintf(stderr, "ndims = %d, [%d x %d], (%d,%d) tiles of dimension [%d x %d], data = %s_%d\n", 
        ndims, str0[0], str0[1], nblki, nblkj, sblki, sblkj, ptype[stype], ssize*8) ;
  if(ndims != 2) goto error ;
  if( gni != str0[0] || gnj != str0[1] ) goto error ;
  // nblki, nblkj, row sizes ( rmap[nblkj] ), followed by tile sizes ( tmap[nblki * nblkj] )
  int nskip ;
  nskip = ws32_skip_out(stream, 4) ;              // skip first 4 words
  uint32_t *rmap = WS32_BUFFER_OUT(*stream) ;
  nskip = ws32_skip_out(stream, nblkj) ;          // skip row sizes
  uint32_t *tmap = WS32_BUFFER_OUT(*stream) ;
  nskip = ws32_skip_out(stream, nblki*nblkj) ;    // skip tile sizes
fprintf(stderr, "data map : ") ;
int i ;
for(i=0 ; i<nblkj ; i++) fprintf(stderr, "%10d", rmap[i]) ;
for(i=0 ; i<nblki*nblkj ; i++) fprintf(stderr, "%10d", tmap[i]) ; fprintf(stderr, "\n") ;
fprintf(stderr, "size left in stream = %d\n", WS32_FILLED(*stream)) ;

  int i0, j0, tni, tnj, nadded ;                   // start and dimension of tiles
  array_descriptor ado ;                           // array descriptor for extraction
  ado.data = tile ;
  ado.ap.tilex = blki ; ado.ap.tiley = blkj ;
// int put_word_block(void *restrict f, void *restrict blk, int ni, int lni, int nj)
  for(j0=0 ; j0<gnj ; j0+=blkj){                   // loop over tile rows
    tnj = (j0+blkj <= gnj) ? blkj : gnj-j0 ;       // tile dimension along j
    for(i0=0 ; i0 < gni ; i0+=blki){               // loop over row of tiles
      tni = (i0+blki <= gni) ? blki : gni-i0 ;     // tile dimension  along i
      ado.ap.ndims = 2 ;
      ado.ap.nx[0] = tni ;
      ado.ap.nx[1] = tnj ;
      nadded = run_pipe_filters(PIPE_REVERSE, &ado, NULL, stream) ;
fprintf(stderr, "size left in stream = %d\n", WS32_FILLED(*stream)) ;
//       nadded = put_word_block( tile_base, tile, tni, gni, tnj ) ;
    }
  }

  return nbytes ;

error:
  if(msize != 0) free(array) ;
  return -1 ;
}
