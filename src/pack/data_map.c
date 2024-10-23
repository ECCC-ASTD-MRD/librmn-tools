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

// translate block Z (zigzag) index into block (i,j) coordinates
// zij    [IN] : Z (zigzag) index
// nti    [IN] : row size
// ntj    [IN] : number of rows
// sf0    [IN] : stripe width, last (top) stripe may be narrower
// the function returns the i and j coordinates in struct ij_range
ij_range Zindex_to_i_j(uint32_t zij, uint32_t nti, uint32_t ntj, uint32_t sf0){
  ij_range ij ;
  uint32_t sf1, i, j, st0, sz0, sti, stn, j0 ; //, zij = zij_ ;

  ij.i0 = -1 ;                              // precondition for miserable failure
  ij.j0 = -1 ;
  // a negative value for zij would translate into a huge unsigned number
  if(zij >= nti * ntj) goto end ;           // zij is out of bounds

  stn = (ntj - 1) / sf0 ;                   // stripe number for last (top) row
  j0  = stn * sf0 ;                         // j index of lowest row in last stripe
  sz0 = stn * nti * sf0 ;                   // z index of first point in last stripe
  sf1 = ntj - j0 ;                          // current width = width of last stripe
  if(zij < sz0) sf1 = sf0 ;                 // not in last stripe, current width = sf0

  st0 = zij / (sf0 * nti) ;                 // current stripe number
  sz0 = st0 * (sf0 * nti) ;                 // first z index in stripe
  sti = (zij - sz0) ;                       // z index offset in stripe
  i   = sti / sf1 ;                         // position along i
  j   = sti - (i * sf1) ;                   // modulo(sti, sf1) (j position in current stripe)
  j  += st0 * sf0 ;                         // position along j (add stripe j start position)
  ij.i0 = i ;                               // i coordinate of block
  ij.j0 = j ;                               // j coordinate of block
end:
  return ij ;                               // return pair of coordinates
}

// translate block i, j coordinates into block Z (zigzag) index
// i      [IN] : index in row
// j      [IN] : index of row
// nti    [IN] : row size
// ntj    [IN] : number of rows
// sf0    [IN] : stripe width (last stripe may be narrower)
// the function returns the Z (zigzag) index associated with block(i,j)
int32_t Zindex_from_i_j(uint32_t i, uint32_t j, uint32_t nti, uint32_t ntj, uint32_t sf0){
  uint32_t zi, sf1, j0, stj, stn ;

  zi = 0xFFFFFFFFu ;                        // precondition for miserable failure
  // a negative value for i or j would translate into a huge unsigned number
  if( i >= nti || j >= ntj) goto end ;      // i or j is out of bounds

  stn = (ntj - 1) / sf0 ;                   // stripe number for last row
  j0  = stn * sf0 ;                         // j index of lowest row in last stripe
  stj = j / sf0 ;                           // stripe number for this row
  sf1 = ntj - j0 ;                          // current width = width of last stripe
  if(j < j0) sf1 = sf0 ;                    // not in last stripe, current width = sf0

  j0 = stj * sf0 ;                          // j index of lowest row in current stripe
  zi = (j0 * nti) +                         // lower left corner of stripe
       (j - j0) +                           // number of rows above bottom of stripe
       (i * sf1) ;                          // i * current stripe width
end:
  return zi ;
}

// block position from grid index, using data map
// map    [IN] : data map
// i      [IN] : i (column) position in 2D grid
// j      [IN] : j (row) position in 2D grid
// return [ij] Z block index
int32_t Z_block_index(zmap map, uint32_t i, uint32_t j){
  ij_range ij = block_index(map, i, j) ;
  return Zindex_from_i_j(ij.i0, ij.j0, map.zni, map.znj, map.stripe) ;
}

// block position from grid index, using data map
// map    [IN] : data map
// i      [IN] : i (column) position in 2D grid
// j      [IN] : j (row) position in 2D grid
// return [i,j] block coordinates (different from z index)
ij_range block_index(zmap map, uint32_t i, uint32_t j){
  ij_range ij = {.i0 = -1, .j0 = -1 } ;
  if(map.gni > i && map.gnj > j){
    ij.i0 = (i - map.lix) / map.lni ;
    ij.j0 = (j - map.ljx) / map.lnj ;
    ij.i0 = (ij.i0 < 0) ? 0 : ij.i0 ;
    ij.j0 = (ij.j0 < 0) ? 0 : ij.j0 ;
  }
  return ij ;
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

// allocate table of pointers to packed blocks, fill it using map->size
// map  [INOUT] : pointer to data map
// data    [IN] : pointer to start of packed data (if NULL, packed data follows map in memory)
// return address of table of pointers to packed blocks, NULL if allocation failed
zblocks *mem_zmap(zmap *map, uint32_t *data){
  int32_t znij = map->zni * map->znj ;
  zblocks *mem = (zblocks *)malloc(znij * sizeof(uint32_t *)) ;

  if(mem){          // allocation was successful
    int i ;
    map->mem = mem ;
    mem[0] = data ? data : (uint32_t *)&(map->size[znij]) ;      // if data is NULL, packed data follows data map in memory
    for(i=1 ; i<znij ; i++) mem[i] = mem[i-1] + map->size[i-1] ;
  }

  return mem ;
}

// ful/partial deallocation of data map and its components
// map  [INOUT] : pointer to data map
// full    [IN] : if zero, only deallocate pointer table to packed blocks
int free_zmap(zmap *map, int full){
  if(map == NULL) return -1 ;
  if(map->mem) free(map->mem) ;
  map->mem = NULL ;
  if(full) free(map) ;
  return 0 ;
}
