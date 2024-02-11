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
#include <rmn/filter_000.h>

// a filter id of 000 is used as the last element of a filter chain
// and will contain encoded dimensions. the length will vary from 2 to 7 
// according to the number of dimensions and the dimension values
// ----------------- id = 000, filter code template -----------------

// dummy filter, can be used as a template for new filters
// 000 MUST be replaced by an integer between 001 and 254
#define ID 000
// PIPE_VALIDATE and PIPE_FWDSIZE : flags, ad, meta_in are used, buf and stream_out may be NULL
ssize_t FILTER_FUNCTION(ID)(uint32_t flags, array_properties *ad, const filter_meta *meta_in, pipe_buffer *buf, wordstream *stream_out){
  // the definition of FILTER_TYPE(ID) (filter_xxx) will come from filter_xxx.h or the appropriate include file
  ssize_t nbytes = 0 ;
  int errors = 0 ;
  typedef struct{    // used as m_out in forward mode, used as m_inv for the reverse filter
    FILTER_PROLOG ;
    // add specific components here
  } filter_inverse ;
  filter_inverse *m_inv, m_out ;
  FILTER_TYPE(ID) *m_fwd ;

  if(meta_in == NULL)          goto error ;   // outright error
  if(meta_in->id != ID)        goto error ;   // wrong ID

  switch(flags & (PIPE_VALIDATE | PIPE_FWDSIZE | PIPE_FORWARD | PIPE_REVERSE)) {  // mutually exclusive flags

    case PIPE_FWDSIZE:                        // get worst case estimate for output data in PIPE_FORWARD mode
    case PIPE_REVERSE:                         // inverse filter
      m_inv = (filter_inverse *) meta_in ;
      if(flags & PIPE_FWDSIZE) {              // get worst case size estimate for output data
        // insert appropriate code here
        goto end ;
      }
      if(m_inv->size < W32_SIZEOF(filter_inverse)) errors++ ;       // wrong size
      // validate contents of m_inv, increment errors if errors are detected
      //
      if(errors)           goto error ;
      //
      // insert appropriate inverse filter code here
      //
      break ;

    case PIPE_VALIDATE:                            // validate input to forward filter
    case PIPE_FORWARD:                             // forward filter
      m_fwd = (FILTER_TYPE(ID) *) meta_in ;        // cast meta_in to input metadata type for this filter
      if(m_fwd->size < W32_SIZEOF(FILTER_TYPE(ID))) errors++ ;      // wrong size
      //
      // check that meta_in is valid, increment errors if errors are detected
      //
      if(errors)           goto error ;

      if(flags & PIPE_VALIDATE) {              // validation call
        nbytes = W32_SIZEOF(filter_inverse) ;  // worst case size size of output metadata for inverse filter
        goto end ;
      }
      //
      // insert appropriate filter code here
      //
      m_inv = &m_out ;    // output metadata (may have to use malloc() if not fixed size structure)
      *m_inv = (filter_inverse) {.size = W32_SIZEOF(filter_inverse), .id = ID, .flags = 0 } ;
      // prepare metadata for inverse filter
      //
      ws32_insert(stream_out, (uint32_t *)(m_inv), W32_SIZEOF(filter_inverse)) ; // insert into stream_out
      // set nbytes to output size
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
