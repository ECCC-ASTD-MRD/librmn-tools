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
#include <stdio.h>

#include <rmn/filter_255.h>

// ----------------- id = 255, diagnostic filter -----------------

#define ID 255
// PIPE_VALIDATE and PIPE_FWDSIZE : flags, ap, meta_in are used, buf and stream_out may be NULL
ssize_t FILTER_FUNCTION(ID)(uint32_t flags, array_properties *ap, const filter_meta *meta_in, pipe_buffer *buf, wordstream *stream_out){
  // the definition of FILTER_TYPE(ID) (filter_xxx) will come from filter_xxx.h or the appropriate include file
  ssize_t nbytes = 0 ;
  int errors = 0 ;
  filter_meta m_out ;
  FILTER_TYPE(ID) *m_fwd ;

  if(meta_in == NULL)          goto error ;   // outright error
  if(meta_in->id != ID)        goto error ;   // wrong ID

  switch(flags & (PIPE_VALIDATE | PIPE_FWDSIZE | PIPE_FORWARD | PIPE_REVERSE)) {  // mutually exclusive flags

    case PIPE_FWDSIZE:                        // get worst case estimate for output data in PIPE_FORWARD mode
    case PIPE_REVERSE:                         // inverse filter
      if(flags & PIPE_FWDSIZE) {              // get worst case size estimate for output data
        // insert appropriate code here
        nbytes = filter_data_values(ap) * sizeof(uint32_t) ; // (size of incoming data)
        goto end ;
      }
      if(meta_in->size != W32_SIZEOF(filter_meta)) errors++ ;       // wrong size
      // validate contents of meta_in, increment errors if errors are detected
      //
      if(errors)           goto error ;
      //
      // insert appropriate inverse filter code here
      switch(meta_in->flags) {
        case 3:
          break ;
        case 2:
          break ;
        case 1:
          fprintf(stderr, "%d dimensions, %d-%d-%d-%d-%d, tiling[%dx%d] from[%d,%d]\n",
                  ap->ndims,ap->nx[0],ap->nx[1],ap->nx[2],ap->nx[3],ap->nx[4],ap->tilex,ap->tiley,ap->n0[0],ap->n0[1]);
          break ;
        case 0:    // NO-OP
          break ;
        default:   // flags has 2 bits, can't happen
          break ;
      }
      // quasi NO-OP for now
      break ;

    case PIPE_VALIDATE:                            // validate input to forward filter
    case PIPE_FORWARD:                             // forward filter
      m_fwd = (FILTER_TYPE(ID) *) meta_in ;        // cast meta_in to input metadata type for this filter
      if(m_fwd->size != W32_SIZEOF(FILTER_TYPE(ID))) errors++ ;      // wrong size
      //
      // check that meta_in is valid, increment errors if errors are detected
      //
      if(errors)           goto error ;

      if(flags & PIPE_VALIDATE) {              // validation call
        nbytes = W32_SIZEOF(filter_meta) ;  // worst case size size of output metadata for inverse filter
        goto end ;
      }
      //
      // insert appropriate filter code here
      //
      switch(meta_in->flags) {
        case 3:
          break ;
        case 2:
          break ;
        case 1:    // print incoming array base properties
          fprintf(stderr, "%d dimensions, %d-%d-%d-%d-%d, tiling[%dx%d] from[%d,%d] to[%d,%d]\n",
                  ap->ndims,ap->nx[0],ap->nx[1],ap->nx[2],ap->nx[3],ap->nx[4],ap->tilex,ap->tiley,
                  ap->n0[0],ap->n0[1],ap->n0[0]+ap->nx[0]-1,ap->n0[1]+ap->nx[1]-1);
          break ;
        case 0:    // NO-OP
          break ;
        default:   // flags has 2 bits, can't happen
          break ;
      }
      // normally metadata for inverse filter
//       m_out = (filter_meta) {.size = W32_SIZEOF(filter_meta), .id = ID, .flags = meta_in->flags } ;
//       ws32_insert(stream_out, &m_out, W32_SIZEOF(filter_meta)) ; // insert into stream_out
      // set nbytes to output size
      nbytes = filter_data_values(ap) * sizeof(uint32_t) ;      // set nbytes to output size
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
