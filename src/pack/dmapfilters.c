//
// Copyright (C) 2025  Environnement Canada
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
//     M. Valin,   Recherche en Prevision Numerique, 2025
//

#include <stdlib.h>
#include <rmn/dmapfilters.h>

static dmapfilter_ptr dmapfilters[MAX_DMAPFILTERS] ;

dmapfilter_ptr filter_nnn(int index){
  if(index < 0 || index >= MAX_DMAPFILTERS) return NULL ;
  return dmapfilters[index] ;
}

int dmapfilter_000(zmap *map, int index, array_nd *array, dmapfilter_args *args){
  uint32_t *dest_address ;
  uint32_t  dest_size ;
  uint32_t  data_size ;

  if(dmapfilter_invalid(args, 0))          goto fail ;
  if(zmap_index_invalid(map, index))       goto fail ;
  if(array->ndim > 3)                      goto fail ;

  dest_address = map->mhead.mem[index] ;
  dest_size    = map->size[index] * sizeof(uint32_t) ;
  data_size    = subarray_size(array) ;
//   uint8_t  *data_address = subarray_address(array) ;

  if(data_size > dest_size) goto fail ;
  subarray_get_nd(array, dest_address, dest_size) ;

  return 0 ;

fail:
  return -1 ;
}
