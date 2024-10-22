// Hopefully useful code for C
// Copyright (C) 2024  Recherche en Prevision Numerique
//
// This code is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation,
// version 2.1 of the License.
//
// This code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// Author:
//     M. Valin,   Recherche en Prevision Numerique, 2024
//

#include <stdio.h>

#include <rmn/data_map.h>

// zij    [IN] : Z (zigzag) index
// nti    [IN] : row size
// ntj    [IN] : number of rows
// sf0    [IN] : stripe width (last stripe may be narrower)
// the function returns i and j coordinates in struct ij_range
ij_range Zindex_to_i_j(int32_t zij, uint32_t nti, uint32_t ntj, uint32_t sf0){
  return Zindex_to_i_j_(zij, nti, ntj, sf0) ;
}

// i      [IN] : index in row
// j      [IN] : index of row
// nti    [IN] : row size
// ntj    [IN] : number of rows
// sf0    [IN] : stripe width (last stripe may be narrower)
// the function returns the Z (zigzag index associated to i and j
int32_t Zindex_from_i_j(int32_t i, int32_t j, uint32_t nti, uint32_t ntj, uint32_t sf0){
  return Zindex_from_i_j_(i, j, nti, ntj, sf0) ;
}

// allocate a worst case buffer for datamap and packed data
zmap *new_zmap(uint32_t gni, uint32_t gnj, uint32_t stripe, size_t esize){
  uint32_t bsize = 64 ;   // use default size of 64
  uint32_t zni = (gni + bsize / 2) / bsize ;
  uint32_t znj = (gnj + bsize / 2) / bsize ;
  uint32_t lni = bsize ;
  uint32_t lnj = bsize ;
  uint32_t lix = gni - (zni - 1) * bsize ;
  uint32_t ljx = gnj - (znj - 1) * bsize ;
  zmap *map = NULL ;
  size_t size = sizeof(zmap) + sizeof(uint16_t) * zni * znj ; // size of data map itself, header + table of sizes
  size_t hsize = size ;
  size_t lsize ;
  uint32_t i, j, lbi, lbj ;
  uint32_t zij ;
fprintf(stderr, "bsize = %d, gni = %d, gnj = %d, zni = %d, znj = %d\n", bsize, gni, gnj, zni, znj);
  // compute worst case block sizes for packed data = size of data rounded up to uint32_t size
  // packed blocks are supposed to be aligned to uint32_t boundaries
  for(j=0 ; j<znj ; j++){
    lbj = (j ==     0 && ljx > lnj) ? ljx : lnj ;      // longer second dimension
    lbj = (j == znj-1 && ljx < lnj) ? ljx : lnj ;      // shorter second dimension
    for(i=0 ; i<zni ; i++){
      lbi = (i ==     0 && lix > lni) ? lix : lni ;    // longer first dimension
      lbi = (i == zni-1 && lix < lni) ? lix : lni ;    // shorter first dimension
      lsize = lbi * lbj * esize ;
      lsize = (lsize + sizeof(uint32_t) - 1) / sizeof(uint32_t) * sizeof(uint32_t) ;
      size = size + lsize ;
fprintf(stderr, "block[%d,%d] (%d,%d) size = %ld\n", i, j, lbi, lbj, lsize/sizeof(uint32_t));
    }
  }
fprintf(stderr, "buffer size = %ld\n", size) ;
  map = (zmap *) malloc(size) ;
  if(map){         // allocation was successful
    uint32_t *data = (uint32_t *)&(map->size[zni*znj]) ;
fprintf(stderr, "data offset = %ld bytes, hsize = %ld[%ld+%ld]\n", (uint8_t *)data - (uint8_t *)map, hsize, sizeof(zmap), sizeof(uint16_t) * zni * znj) ;
    map->version = Z_DATA_MAP_VERSION ;
    map->stripe = stripe ;
    map->gni    = gni ;
    map->gnj    = gnj ;
    map->zni    = zni ;
    map->znj    = znj ;
    map->lni    = lni ;
    map->lnj    = lnj ;
    map->lix    = lix ;
    map->ljx    = ljx ;
    map->flags  = 0 ;
    for(j=0 ; j<znj ; j++){
      lbj = (j ==     0 && ljx > lnj) ? ljx : lnj ;      // longer second dimension
      lbj = (j == znj-1 && ljx < lnj) ? ljx : lnj ;      // shorter second dimension
      for(i=0 ; i<zni ; i++){
        lbi = (i ==     0 && lix > lni) ? lix : lni ;    // longer first dimension
        lbi = (i == zni-1 && lix < lni) ? lix : lni ;    // shorter first dimension
        lsize = lbi * lbj * esize ; lsize = (lsize + sizeof(uint32_t) - 1) / sizeof(uint32_t) ;
        zij = Zindex_from_i_j(i, j, zni, znj, stripe) ;
        map->size[zij] = lsize ;                     // set worstcase size for this zigzag indexed block
      }
    }
fprintf(stderr, "map->size : ");
for(i=0 ; i<zni*znj ; i++)fprintf(stderr, "%6d", map->size[i]);
fprintf(stderr, "\n");
for(j=znj ; j>0 ; j--){
  for(i=0 ; i<zni ; i++){
    zij = Zindex_from_i_j(i, j-1, zni, znj, stripe) ;
    fprintf(stderr, "%6d[%2d,%2d](%2d)", map->size[zij], i, j-1, zij);
  }
  fprintf(stderr, "\n");
}
  }

  return map ;
}
