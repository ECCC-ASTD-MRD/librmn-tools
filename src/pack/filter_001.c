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

#include <rmn/filter_001.h>
#include <rmn/data_info.h>

// ----------------- id = 001, array properties filter -----------------

// array properties filter, reverse mode is a NO-OP
// this filter will modify array properties (ap), adding min and max information
// this filter can be used before the linear quantizer filter (254)
#define ID 001
// PIPE_VALIDATE and PIPE_FWDSIZE : flags, ap, meta_in are used, buf and stream_out may be NULL
ssize_t FILTER_FUNCTION(ID)(uint32_t flags, array_descriptor *ap, const filter_meta *meta_in, pipe_buffer *buf, wordstream *stream_out){
  // the definition of FILTER_TYPE(ID) (filter_xxx) will come from filter_xxx.h or the appropriate include file
  ssize_t nbytes = 0 ;
  int errors = 0 ;
//   typedef struct{    // used as m_out in forward mode, used as m_inv for the reverse filter
//     FILTER_PROLOG ;
//     // empty filter, no metadata
//   } filter_inverse ;
  FILTER_TYPE(ID) *m_fwd ;

  if(meta_in == NULL)          goto error ;   // outright error
  if(meta_in->id != ID)        goto error ;   // wrong ID

  switch(flags & (PIPE_VALIDATE | PIPE_FWDSIZE | PIPE_FORWARD | PIPE_REVERSE)) {  // mutually exclusive flags

    case PIPE_FWDSIZE:                        // get worst case estimate for output data in PIPE_FORWARD mode
    case PIPE_REVERSE:                         // inverse filter
      if(flags & PIPE_FWDSIZE) {              // get worst case size estimate for output data
        // insert appropriate code here
        nbytes = filter_data_values(ap) * sizeof(int32_t) ;   // output data unchanged from input data
        goto end ;
      }
      // inverse mode is a NO-OP, forward mode inserts nothing into stream_out
      break ;

    case PIPE_VALIDATE:                            // validate input to forward filter
    case PIPE_FORWARD:                             // forward filter
      m_fwd = (FILTER_TYPE(ID) *) meta_in ;        // cast meta_in to input metadata type for this filter
      if(m_fwd->size < W32_SIZEOF(FILTER_TYPE(ID))) errors++ ;      // wrong size
      // no metadata is expected in meta_in
      if(errors)           goto error ;

      if(flags & PIPE_VALIDATE) {              // validation call
        nbytes = 0 ;                           // size of metadata for inverse filter
        goto end ;
      }
      // insert appropriate filter code here
      // min and max values in metadata will be modified
      // data will not be modified
      limits_w32 l32 ;
      if(ap->esize != 4) goto error ;          // 32 bit data only
      ap->fid     = ID ;                       // information origin
      ap->ptype   = PROP_MIN_MAX ;             // min/max/min0 trio
      ap->version = PROP_VERSION ;             // match with current library version
      switch(ap->etype) {
        case PIPE_DATA_SIGNED:
          l32 = INT32_extrema(buf->buffer, filter_data_values(ap)) ;
          ap->nprop = 3 ;
          ap->prop[0].i = l32.i.mins ;
          ap->prop[1].i = l32.i.maxs ;
          ap->prop[2].i = l32.i.min0 ;
          break ;
        case PIPE_DATA_UNSIGNED:
          l32 = UINT32_extrema(buf->buffer, filter_data_values(ap)) ;
          ap->nprop = 3 ;
          ap->prop[0].u = l32.u.mina ;
          ap->prop[1].u = l32.u.maxa ;
          ap->prop[2].u = l32.u.min0 ;
          break ;
        case PIPE_DATA_FP:
          l32 = IEEE32_extrema(buf->buffer, filter_data_values(ap)) ;
          ap->nprop = 3 ;
          ap->prop[0].f = l32.f.mins ;
          ap->prop[1].f = l32.f.maxs ;
          ap->prop[2].f = l32.f.min0 ;
          break ;
        default:
          goto error ; // invalid data type
      }
      // no metadata for inverse filter
      // nothing to insert into stream_out, there should be no reverse call
      // set nbytes to output size
      nbytes = filter_data_values(ap) * sizeof(int32_t) ;
      break ;

    default:
      goto error ; // invalid flag combination or none of the needed flags
  }

end:
if(ap)fprintf(stderr, "filter 001 : forward etype = %d, esize = %d, nbytes = %ld\n", ap->etype, ap->esize, nbytes);
  return nbytes ;

error :
  errors = (errors > 0) ? errors : 1 ;
  return -errors ;
}
#undef ID
