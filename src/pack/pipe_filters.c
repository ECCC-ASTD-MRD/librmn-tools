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

#include <rmn/pipe_filters.h>

#define MAX_FILTERS     256
#define MAX_FILTER_TYPE  16

// macros for internal use
// #define PUSH_BITS(acc, data, nbits)    { acc = (uint64_t)(acc) << (nbits) ; acc = (acc) | (data) ; }
// #define POP_BITS(acc, data, nbits)     { data = (acc) >> ((8 * sizeof(acc)) - (nbits)) ; acc <<= ((8 * sizeof(acc)) - (nbits)) ; }

typedef struct{
  pipe_filter_pointer fn ;
  char name[MAX_FILTER_TYPE] ;
} filter_table_element ;

static filter_table_element filter_table[MAX_FILTERS] ;

// translate dims[] into number of values
// static int buffer_dimension(int *dims){
//   int nval = 0 ;
//   int i ;
//   int ndims = PIPE_DATA_NDIMS(dims) ;
//   if((ndims > 5) || (ndims < 0)) goto end ;
//   nval = 1 ;
//   for(i=0 ; i<ndims ; i++){
//     nval *= dims[i+1] ;
//   }
// end:
//   return nval ;
// }

// ----------------- id = 000, template filter -----------------

// dummy filter, can be used as a template for new filters
// 000 MUST be replaced by a 3 digit integer between 001 and 254
#define ID 000
ssize_t FILTER_FUNCTION(ID)(uint32_t flags, int *dims, filter_meta *meta_in, pipe_buffer *buf, wordstream *stream_out){
  // the definition of FILTER_TYPE(ID) (filter_xxx) will come from pipe_filters.h or an appropriate include file
  ssize_t nbytes = 0 ;
  int errors = 0 ;
  // used as m_out in forward mode
  // used as m_inv for the reverse filter
  typedef struct{
    FILTER_PROLOG ;
    // add specific components here
  } filter_inverse ;
  filter_inverse *m_inv ;

  if(meta_in == NULL)          goto error ;   // outright error
  if(meta_in->id != ID)        goto error ;   // wrong ID

  switch(flags & (PIPE_VALIDATE | PIPE_FWDSIZES | PIPE_FORWARD | PIPE_REVERSE)) {  // mutually exclusive flags

    case PIPE_FWDSIZES:                        // get worst case estimates of meta_out and buffer sizes for PIPE_FORWARD mode
    case PIPE_REVERSE:                         // inverse filter
      m_inv = (filter_inverse *) meta_in ;
      if(flags & PIPE_FWDSIZES) {              // get worst case estimates for meta_out and stream_out
        // insert appropriate code here
        goto end ;
      }
      if(m_inv->size < W32_SIZEOF(filter_inverse)) errors++ ;       // wrong size
      // validate contents of m_inv, set nbytes to negative value if errors are detected
      //
      if(errors)           goto error ;
      //
      // insert appropriate inverse filter code here
      //
      break ;

    case PIPE_VALIDATE:                            // validate input to forward filter
    case PIPE_FORWARD:                             // forward filter
      nbytes = 0 ;
      FILTER_TYPE(ID) *m_fwd = (FILTER_TYPE(ID) *) meta_in ;        // cast meta_in to input metadata type for this filter
      if(m_fwd->size < W32_SIZEOF(FILTER_TYPE(ID))) errors++ ;      // wrong size
      //
      // check that meta_in is valid, set nbytes to negative value if errors are detected
      //
      if(errors)           goto error ;

      if(flags & PIPE_VALIDATE) goto end ;   // validation call
      //
      // insert appropriate filter code here
      //
      filter_inverse m_out ;  // output metadata
      m_inv = &m_out ;
      *m_inv = (filter_inverse) {.size = W32_SIZEOF(filter_inverse), .id = ID, .flags = 0 } ;
      // prepare metadata for inverse filter
      //
      ws32_insert(stream_out, (uint32_t *)(m_inv), W32_SIZEOF(filter_inverse)) ; // insert into stream_out
      break ;

    default:
      goto error ; // invalid flag combination or none of the needed flags
  }

end:
  return nbytes ;

error :
  errors = (errors > 0) ? errors : 1 ;
  nbytes = -errors ;
  goto end ;
}
#undef ID

// test filter id = 254, scale data and add offset
// with PIPE_VALIDATE, the only needed arguments are flags and meta_in, all other arguments could be NULL
// with PIPE_FWDSIZES, flags, dims, meta_in, meta_out are used, all other arguments could be NULL
#define ID 254
ssize_t FILTER_FUNCTION(ID)(uint32_t flags, int *dims, filter_meta *meta_in, pipe_buffer *buf, wordstream *stream_out){
  ssize_t nbytes ;
  int errors = 0 ;
  // used as m_out in forward mode
  // used as m_inv for the reverse filter
  typedef struct{
    FILTER_PROLOG ;
    // add relevant components here
    int32_t factor ;
    int32_t offset ;
  } filter_inverse ;
  filter_inverse *m_inv ;

  if(meta_in == NULL)          goto error ;   // outright error
  if(meta_in->id != ID)        goto error ;   // wrong ID

  switch(flags & (PIPE_VALIDATE | PIPE_FWDSIZES | PIPE_FORWARD | PIPE_REVERSE)) {  // mutually exclusive flags

    case PIPE_FWDSIZES:                        // get worst case estimates of meta_out and buffer sizes for PIPE_FORWARD mode
    case PIPE_REVERSE:                         // inverse filter
      m_inv = (filter_inverse *) meta_in ;
      if(flags & PIPE_FWDSIZES) {              // get worst case estimates for meta_out and stream_out
        // insert appropriate code here
        goto end ;
      }
      if(m_inv->size < W32_SIZEOF(filter_inverse)) errors++ ;       // wrong size
      // validate contents of m_inv, set nbytes to negative value if errors are detected
      if((m_inv->offset == 0) && (m_inv->factor == 0)) errors++ ;   // MUST NOT be both 0
      //
      if(errors)           goto error ;
      //
      // insert appropriate inverse filter code here
      {
        int i ;
        int32_t nval = filter_data_values(dims) ;       // get data size
        int32_t *data = (int32_t *) buf->buffer ;
        for(i=0 ; i<nval ; i++) data[i] = (data[i] - m_inv->offset) / m_inv->factor ;
      }
      //
      break ;

    case PIPE_VALIDATE:                            // validate input to forward filter
    case PIPE_FORWARD:                             // forward filter
      nbytes = 0 ;
      FILTER_TYPE(ID) *m_fwd = (FILTER_TYPE(ID) *) meta_in ; // cast meta_in to input metadata type for this filter
      if(m_fwd->size < W32_SIZEOF(FILTER_TYPE(ID))) errors++ ;      // wrong size
      //
      // check that meta_in is valid, set nbytes to negative value if errors are detected
      if((m_fwd->factor == 0) && (m_fwd->factor == 0)) errors++ ;   // MUST NOT be both 0
      if(errors)           goto error ;

      if(flags & PIPE_VALIDATE) goto end ;   // validation call
      //
      // insert appropriate filter code here
      {
        int i ;
        int32_t nval = filter_data_values(dims) ;       // get data size
        int32_t *data = (int32_t *) buf->buffer ;
        for(i=0 ; i<nval ; i++) data[i] = data[i] * m_fwd->factor + m_fwd->offset ;
      }
      //
      filter_inverse m_out ;  // output metadata
      m_inv = &m_out ;
      *m_inv = (filter_inverse) {.size = W32_SIZEOF(filter_inverse), .id = ID, .flags = 0 } ;
      // prepare metadata for inverse filter
      m_out.factor = m_fwd->factor ;
      m_out.offset = m_fwd->factor ;
      //
      ws32_insert(stream_out, (uint32_t *)(m_inv), W32_SIZEOF(filter_inverse)) ; // insert into stream_out
      break ;

    default:
      goto error ; // invalid flag combination or none of the needed flags
  }

end:
  return nbytes ;

error :
  errors = (errors > 0) ? errors : 1 ;
  nbytes = -errors ;
  goto end ;
}
#undef ID

// test filter id = 254, scale data and add offset
// with PIPE_VALIDATE, the only needed arguments are flags and meta_in, all other arguments could be NULL
// with PIPE_FWDSIZES, flags, dims, meta_in, meta_out are used, all other arguments could be NULL
ssize_t pipe_filter_254_old(uint32_t flags, int *dims, filter_meta *meta_in, pipe_buffer *buf, wordstream *stream_out){
  int nval, i, esize, errors = 0 ;
  ssize_t nbytes = 0 ;
  int32_t *data ;
  struct meta{
    FILTER_PROLOG ;           // used for meta_out in forward mode
    int32_t factor ;
    int32_t offset ;
  } *m_out, meta_out ;             // type used by the filter API

  if(buf != NULL) data = (int32_t *) buf->buffer ;
// fprintf(stderr, "entering pipe_filter_254, flags = %8.8x\n", flags);
  if(meta_in == NULL)          goto error ;
  if(meta_in->id != 254)       goto error ;   // wrong ID
  switch(flags & (PIPE_VALIDATE | PIPE_FORWARD | PIPE_REVERSE | PIPE_FWDSIZES)) {  // mutually exclusive flags

    case PIPE_FWDSIZES:                      // get worst case estimates of meta_out and buffer sizes (PIPE_FORWARD mode)
      nbytes = filter_data_values(dims) ;      // get data dimensions
      nbytes *= PIPE_DATA_SIZE(dims) ;       // will return "worst case" size of output data in bytes
// fprintf(stderr, "pipe_filter_254 PIPE_FWDSIZES size = %d, nbytes = %ld\n", meta_out.size, nbytes) ;
      break ;

    case PIPE_VALIDATE:                      // validate input flags for eventual call in PIPE_FORWARD mode
      if(meta_in->size <  3)   errors++ ;
      if(meta_in->meta[0].i == 0 && meta_in->meta[1].i == 0) errors++ ;   // cannot be both 0
      nbytes = errors ? -errors : 3 ;         // if no errors, return length needed for meta_out
// fprintf(stderr, "pipe_filter_254 PIPE_VALIDATE errors = %d, nbytes = %ld\n", errors, nbytes) ;
      break ;

    // NOTE: PIPE_REVERSE with meta_in, buf, or stream_out == NULL could indicate PIPE_FWDSIZES
    case PIPE_REVERSE:                       // inverse filter
      m_out = (struct meta *) meta_in ;
fprintf(stderr, "pipe_filter_254 PIPE_REVERSE\n") ;
      if(m_out->size <  3)   goto error ;   // not enough space for metadata
      nval = filter_data_values(dims) ;         // get data size
      nbytes = nval * sizeof(uint32_t) ;
      int32_t factor = m_out->factor ;   // get metadata prepared during the forward filter pass
      int32_t offset = m_out->offset ;
      for(i=0 ; i<nval ; i++) data[i] = (data[i] - offset) / factor ;
      break ;

    // NOTE: PIPE_FORWARD with buf == NULL or stream_out == NULL could indicate PIPE_VALIDATE
    case PIPE_FORWARD:                       // forward filter
fprintf(stderr, "pipe_filter_254 PIPE_FORWARD\n") ;
      if(buf == NULL || stream_out == NULL){   // validation call
        if(meta_in->size <  3)   nbytes-- ;
        if(meta_in->meta[0].i == 0 && meta_in->meta[1].i == 0) nbytes-- ;   // cannot be both 0
        if(nbytes != 0) goto end ;
      }
      nval = filter_data_values(dims) ;         // get data size
      // TODO: check that data element size is 4 bytes
      // cast meta_in to actual metadata type for this filter
      filter_254 *meta = (filter_254 *) meta_in ;
      // execute filter
      for(i=0 ; i<nval ; i++) data[i] = data[i] * meta->factor + meta->offset ;
      // prepare metadata for reverse filter
      meta_out.id    = 254 ;
      meta_out.flags = 0 ;
      meta_out.size  = 3 ;
      meta_out.factor = meta->factor ;
      meta_out.offset = meta->offset ;
      // TODO check that there is space in stream_out for insertion
      ws32_insert(stream_out, (uint32_t *)(&meta_out), W32_SIZEOF(meta_out)) ;
      // number of used bytes in data buffer
      nbytes = nval * sizeof(uint32_t) ;
      break ;

    default:
fprintf(stderr, "pipe_filter_254 invalid flags\n") ;
      goto error ; // invalid flag combination or none of the needed flags
  }

end:
// fprintf(stderr, "pipe_filter_254 exiting, nbytes = %ld\n", nbytes) ;
  return nbytes ;

error :
// fprintf(stderr, "pipe_filter_254 ERROR, nbytes = %ld\n", nbytes) ;
  return (nbytes = -1) ;
}

// test filter id = 253, delta filter
ssize_t pipe_filter_253(uint32_t flags, int *dims, filter_meta *meta_in, pipe_buffer *buf, wordstream *stream_out){
  ssize_t nbytes ;
error :
  return (nbytes = -1) ;
}

#if 0
ssize_t pack_filter_253(pack_flags flags, pack_meta *meta_in, pipe_buffer *buf_in, pack_meta *meta_out, pipe_buffer *buf_out){
  int errors = 0 ;
  if(FPACK_ID(flags) != 254)     goto error ;  // bad ID
  if(flags & PIPE_REVERSE){                   // inverse filter
    if(meta_in->id != 253)       goto error ;  // bad ID
    if(meta_out.size < 2)       goto error ;  // not enough space in meta_out
    if(meta_out.id != 253)      errors++ ;
    if(meta_out.nmeta != 1)     errors++ ;
    if(meta_out.meta[0].u != 3) errors++ ;
    if(errors > 0)               goto error ;
  }else if(flags & PIPE_FORWARD){             // forward filter
    if(meta_out.size < 2)       goto error ;  // not enough space in meta_out
    meta_out.id    = 253 ;
    meta_out.nmeta = 1 ;
    meta_out.meta[0].u = 3 ;
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
  if(flags & PIPE_REVERSE){                // inverse filter, restore quantized data

    if(meta_in->nmeta != 2)   goto error ;    // need 64 bits of metadata information
    qdesc.u = ((uint64_t)meta_in->meta[0].u << 32) | meta_in->meta[0].u ;   // we can now use qdesc.f (q_rules)
    // we can now call the quantizer inverse operation (restore)
    // call un-quantizer(buf, nval, q_encode desc, buf)
    // the operation will work in place
    meta_out.meta[0].u = (qdesc.u >> 32) ;           // using q_decode (optional)
    meta_out.meta[1].u = (qdesc.u & 0xFFFFFFFFu) ;

  }else if(flags & PIPE_FORWARD){                              // forward filter, quantize data using rules from meta_in

    if(meta_in->nmeta != 3)        goto error ;
    ref   = meta_in->meta[0].f ;      // get reference value
    nbits = meta_in->meta[1].u ;      // get nbits
    qtype = meta_in->meta[2].u ;      // get quantizer type
    if(nbits == 0 && ref == 0.0f) goto error ;    // cannot be both set to 0
    if(qtype > 2)                 goto error ;    // invalid quantizer type
    if(meta_out.nmeta < 2)       goto error ;    // not enough space in meta_out
    meta_out.id = 001 ;
    // the operation will work in place
    // call quantizer(*buf, nval, q_rules rules, *buf, limits_w32 *limits, NULL)
    // get qdesc from the quantizer qdesc.q / qdesc.u
    meta_out.meta[0].u = (qdesc.u >> 32) ;
    meta_out.meta[1].u = (qdesc.u & 0xFFFFFFFFu) ;

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
// function returns -1 in case of error, non negative value from filter if O.K.
ssize_t filter_validate(filter_meta *meta_in){
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
int filter_is_defined(int id){
  if(id > MAX_FILTERS-1) return -1 ;               // invalid id
  return (filter_table[id].fn == NULL) ? 1 : 0 ;   // 0 if defined, 1 if not
}

// register a pipe filter
// id     [IN] : id for this pipe filter (0 < ID < MAX_FILTERS)
// name   [IN] : name of pipe filter (ta most MAX_FILTER_TYPE characters)
// fn     [IN] : address of pipe filter function
int filter_register(int id, char *name, pipe_filter_pointer fn){
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
int filter_list_valid(filter_list list){
  int i ;

  for(i=0 ; list[i] != NULL ; i++){                    // loop until NULL terminator
    if(filter_validate(list[i]) != 0) return -(i+1) ;  // error at filter i
  }
  return i - 1 ;                                       // number of valid filters
}

void pipe_filter_init(void){
  filter_register(254, "test254", pipe_filter_254) ;
//   pack_filter_register(254, "test253", pack_filter_253) ;
//   pack_filter_register(001, "linear1", pack_filter_001) ;
}

// ssize_t pipe_filter_253(uint32_t flags, int *dims, filter_meta *meta_in, pipe_buffer *buf, filter_meta *meta_out)
// run a filter cascade on input data
// flags   [IN] : flags for the cascade
// dims    [IN] : dimensions of input data (input data is assumed to be 32 bit)
// list    [IN] : list of filters to be run
// buffer [OUT] : cascade result
ssize_t run_pipe_filters(int flags, int *dims, void *data, filter_list list, pipe_buffer *buffer){
  int i ;
  int dsize = PIPE_DATA_SIZE(dims) ;
  int ndata = filter_data_values(dims) ;
  int ndims = PIPE_DATA_NDIMS(dims) ;
  pipe_buffer pbuf ;
  ssize_t outsize, status ;
  filter_meta meta_out ;
  filter_meta *m_out ;
  wordstream stream_out ;

  pbuf.buffer = data ;
  pbuf.max_size = pbuf.used = ndata * dsize ;
fprintf(stderr, "run_pipe_filters : dsize = %d, ndims = %d, ndata = %d, used = %ld, address = %p\n\n", dsize, ndims, ndata, pbuf.max_size, pbuf.buffer) ;
  for(i=0 ; list[i] != NULL ; i++){
    filter_meta *meta = list[i] ;
    int id = meta->id ;
    pipe_filter_pointer fn = pipe_filter_address(id) ;
    pipe_filter_pointer fn2 = pipe_filter_254 ;
fprintf(stderr, "run_pipe_filters : filter %p, id = %d, address = %p(%p)\n", meta, id, fn, fn2) ;
    status  = (*fn)(PIPE_VALIDATE, NULL, list[i], NULL, NULL) ;
fprintf(stderr, "                   meta out size = %ld\n", status) ;
    outsize = (*fn)(PIPE_FWDSIZES, dims, list[i], NULL, NULL) ;
fprintf(stderr, "                   meta out size = %d, outsize = %ld\n", meta_out.size, outsize) ;
    m_out = ALLOC_META(meta_out.size) ;
    m_out->size = meta_out.size ;
    outsize = (*fn) (flags, dims, list[i], &pbuf, &stream_out) ;
fprintf(stderr, "                   meta out size = %d, outsize = %ld\n", m_out->size, outsize) ;
  }
  return 0 ;
}

