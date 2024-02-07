//
// Copyright (C) 2024  Environnement Canada
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
//     M. Valin,   Recherche en Prevision Numerique, 2024
//
#include <string.h>

#include <rmn/pipe_filters.h>
#include <rmn/filter_254.h>

// ----------------- id = 254, test filter (scale and offset) -----------------

// test filter id = 254, scale data and add offset
// with PIPE_VALIDATE, the only needed arguments are flags and meta_in, all other arguments could be NULL
// with PIPE_FWDSIZE, flags, ad, meta_in, stream_out are used, all other arguments could be NULL
// with PIPE_REVERSE, flags, ad, meta_in, buf are used, stream_out is not used
#define ID 254
ssize_t FILTER_FUNCTION(ID)(uint32_t flags, array_dimensions *ad, const filter_meta *meta_in, pipe_buffer *buf, wordstream *stream_out){
  ssize_t nbytes = 0 ;
  int errors = 0, nval, i ;
  typedef struct{    // used as m_out in forward mode, used as m_inv for the reverse filter
    FILTER_PROLOG ;
    // add relevant components here
    int32_t factor ;
    int32_t offset ;
  } filter_inverse ;
  filter_inverse *m_inv, m_out ;
  FILTER_TYPE(ID) *m_fwd ;

  if(meta_in == NULL)          goto error ;   // outright error
  if(meta_in->id != ID)        goto error ;   // wrong ID

  switch(flags & (PIPE_VALIDATE | PIPE_FWDSIZE | PIPE_FORWARD | PIPE_REVERSE)) {  // mutually exclusive flags

    case PIPE_FWDSIZE:                        // get worst case estimate for output data in PIPE_FORWARD mode
    case PIPE_REVERSE:                         // inverse filter
      m_inv = (filter_inverse *) meta_in ;
      if(flags & PIPE_FWDSIZE) {              // get worst case estimate for output data
        // insert appropriate code here
        nbytes = filter_data_values(ad) * sizeof(int32_t) ;
        goto end ;
      }
      if(m_inv->size < W32_SIZEOF(filter_inverse)) errors++ ;       // wrong size
      // validate contents of m_inv, increment errors if errors are detected
      if((m_inv->offset == 0) && (m_inv->factor == 0)) errors++ ;   // MUST NOT be both 0
      //
      if(errors)           goto error ;
      // insert appropriate inverse filter code here, check buf->used and buf->max_size
      {
        nval = filter_data_values(ad) ;                   // get number of data values
        int32_t *data = (int32_t *) buf->buffer ;
        if(filter_data_bytes(ad) != buf->used) {          // dimension mismatch
          errors = 1 ; goto error ;
        }
        for(i=0 ; i<nval ; i++) data[i] = (data[i] - m_inv->offset) / m_inv->factor ;
      }
      break ;

    case PIPE_VALIDATE:                            // validate input to forward filter
    case PIPE_FORWARD:                             // forward filter
      m_fwd = (FILTER_TYPE(ID) *) meta_in ; // cast meta_in to input metadata type for this filter
      if(m_fwd->size < W32_SIZEOF(FILTER_TYPE(ID))) errors++ ;      // wrong size
      //
      // check that meta_in is valid, increment errors if errors are detected
      if((m_fwd->factor == 0) && (m_fwd->factor == 0)) errors++ ;   // MUST NOT be both 0
      if(errors)           goto error ;

      if(flags & PIPE_VALIDATE) {              // validation call
        nbytes = W32_SIZEOF(filter_inverse) ;  // worst case size size of output metadata for inverse filter
        goto end ;
      }
      // insert appropriate filter code here, check buf->used and buf->max_size
      {
        nval = filter_data_values(ad) ;                   // get number of data values
        int32_t *data = (int32_t *) buf->buffer ;
        if(filter_data_bytes(ad) != buf->used) {          // dimension mismatch
          errors = 1 ; goto error ;
        }
        for(i=0 ; i<nval ; i++) data[i] = data[i] * m_fwd->factor + m_fwd->offset ;
      }
      //
      m_inv = &m_out ;    // output metadata
      *m_inv = (filter_inverse) {.size = W32_SIZEOF(filter_inverse), .id = ID, .flags = 0 } ;
      m_inv->factor = m_fwd->factor ;                     // metadata for inverse filter
      m_inv->offset = m_fwd->offset ;
      //
      ws32_insert(stream_out, (uint32_t *)(m_inv), W32_SIZEOF(filter_inverse)) ; // insert into stream_out
      nbytes = filter_data_values(ad) * sizeof(uint32_t) ;      // set nbytes to output size
      break ;

    default:
      goto error ; // invalid flag combination or none of the needed flags
  }

end:
  return nbytes ;

error :
  errors = (errors > 0) ? errors : 1 ;
  return -errors ;
}
#undef ID

