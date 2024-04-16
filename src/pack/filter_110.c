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
#include <stdio.h>

#include <rmn/filter_110.h>

// ----------------- id = 110, simple packer -----------------

#define ID 110
// PIPE_VALIDATE and PIPE_FWDSIZE : flags, ap, meta_in are used, buf and stream_out may be NULL
ssize_t FILTER_FUNCTION(ID)(uint32_t flags, array_descriptor *ap, const filter_meta *meta_in, pipe_buffer *buf, wordstream *stream_out){
  // the definition of FILTER_TYPE(ID) (filter_xxx) will come from filter_xxx.h or the appropriate include file
  ssize_t nbytes = 0 ;
  int errors = 0 ;
  typedef struct{    // used as m_out in forward mode, used as m_inv for the reverse filter
    FILTER_PROLOG ;
    // add specific components here
    uint32_t meta[MAX_ARRAY_DIMENSIONS+1];
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
fprintf(stderr, "filter 110 : reverse\n");
      if(m_inv->size < W32_SIZEOF(filter_inverse)) errors++ ;       // wrong size
      // validate contents of m_inv, increment errors if errors are detected
      //
      if(errors)           goto error ;
      //
      // insert appropriate inverse filter code here
      //
      {
        int i, nval = filter_data_values(ap) ;                   // get number of data values
        uint32_t *wds = (uint32_t *) buf->buffer ;
        uint16_t *hwds = (uint16_t *) buf->buffer ;
        for(i=0 ; i<nval ; i++) wds[i] = hwds[i] ;
fprintf(stderr, "filter 110 : reverse, nval = %d, wds %d -> %d\n", nval, wds[0], wds[nval-1]) ;
      }
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
      {
        int i, nval = filter_data_values(ap) ;                   // get number of data values
        uint32_t *wds = (uint32_t *) buf->buffer ;
        uint16_t *hwds = (uint16_t *) buf->buffer ;
        if(filter_data_bytes(ap) != buf->used) {          // dimension mismatch
fprintf(stderr, "filter 110 : dimension mismatch\n") ;
          errors = 1 ; goto error ;
        }
        for(i=0 ; i<nval ; i++) hwds[i] = wds[i] ;
fprintf(stderr, "filter 110 : properties = %d min = %d, max = %d, min0 = %d\n", ap->nprop, ap->prop[0].u, ap->prop[1].u, ap->prop[2].u) ;
      }
      m_inv = &m_out ;    // output metadata (may have to use malloc() if not fixed size structure)
      *m_inv = (filter_inverse) {.size = W32_SIZEOF(filter_inverse), .id = ID, .flags = 0 } ;
      // prepare metadata for reverse filter
      // encode dimensions
      int nw32 = encode_dimensions(ap, m_out.meta) ;
      // encode dimensions into m_out.meta[], set size to 1 + required size
      ws32_insert(stream_out, (uint32_t *)(m_inv), W32_SIZEOF(filter_inverse)) ; // insert into stream_out
      nbytes = filter_data_values(ap) * sizeof(uint16_t) ;      // set nbytes to output size
fprintf(stderr, "filter 110 : forward, nbytes = %ld, nw32 = %d\n", nbytes, nw32);
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
