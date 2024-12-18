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
#include <string.h>

#include <rmn/data_map.h>

#define DEBUG 0

// translate block Z (zigzag) index into block (i,j) coordinates
// zij    [IN] : Z (zigzag) index
// nti    [IN] : row size
// ntj    [IN] : number of rows
// sf0    [IN] : stripe width, last (top) stripe may be narrower
// the function returns the i and j coordinates in struct ij_range
ij_pair Zindex_to_i_j(int32_t zij, int32_t nti, int32_t ntj, int32_t sf0){
  ij_pair ij ;
  int32_t sf1, i, j, st0, sz0, sti, stn, j0 ;

  ij.i = -1 ;                               // precondition for miserable failure
  ij.j = -1 ;
  // a negative value for zij would translate into a huge unsigned number
  if(zij < 0) goto end ;
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
  ij.i = i ;                                // i coordinate of block
  ij.j = j ;                                // j coordinate of block
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
int32_t Zindex_from_i_j(int32_t i, int32_t j, int32_t nti, int32_t ntj, int32_t sf0){
  int32_t zi, sf1, j0, stj, stn ;

  zi = -1 ;                                 // precondition for miserable failure
  // a negative value for i or j would translate into a huge unsigned number
  if(i < 0 || j < 0) goto end ;
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
// return [i,j] block coordinates (different from z index)
ij_pair block_index(zmap *map, int32_t i, int32_t j){
  ij_pair ij = {.i = -1, .j = -1 } ;  // precondition for failure
  if(map->gni > i && map->gnj > j){
    ij.i = (i - map->lix) / map->lni ;
    ij.j = (j - map->ljx) / map->lnj ;
    ij.i = (ij.i < 0) ? 0 : ij.i ;
    ij.j = (ij.j < 0) ? 0 : ij.j ;
  }
  return ij ;
}

// Z (zigzag) block index from block indexes, using data map
// map    [IN] : data map
// i      [IN] : i (column) position in 2D block grid
// j      [IN] : j (row) position in 2D block grid
// return [ij] Z block index
int32_t Z_map_index(zmap *map, int32_t i, int32_t j){
//   ij_pair ij = block_index(map, i, j) ;
  return Zindex_from_i_j(i, j, map->zni, map->znj, map->stripe) ;
}

// area covered by block[j][i]
// map    [IN] : data map
// bi     [IN] : block column index
// bj     [IN] : block row index
// return i and j index limits for area covered by block[j][i]
ij_range block_limits(zmap *map, int32_t bi, int32_t bj){
  ij_range ij = {.i0 = -1, .in = -1, .j0 = -1, .jn = -1 } ;  // precondition for failure

  if(bi < map->zni && bj < map->znj){                        // inside map limits ?
    ij_pair p ;
    p = b_limits(bi, map->lni, map->lix) ;                   // get block limits along first dimension (row)
    ij.i0 = p.i ; ij.in = p.j ;
    p = b_limits(bj, map->lnj, map->ljx) ;                   // get block limits along second dimension (column)
    ij.j0 = p.i ; ij.jn = p.j ;
  }
  return ij ;
}

// create a data map with a worst case buffer for map and packed data
// gni    [IN] : first dimension of array (row size)
// gnj    [IN] : second dimension of array (number of rows)
// stripe [IN] : block stripe width for map
// esize  [IN] : size in bytes of array elements (normally 1/2/4/8)
zmap *new_zmap(int32_t gni, int32_t gnj, int32_t stripe, size_t esize){
  int32_t bsize = 64 ;   // use default size of 64
  int_pair p ;
  p = split_array_dimension(gni, bsize) ;
  int32_t zni = p.i1 ;
  int32_t lni = bsize ;
  int32_t lix = p.i2 ;
  p = split_array_dimension(gnj, bsize) ;
  int32_t znj = p.i1 ;
  int32_t lnj = bsize ;
  int32_t ljx = p.i2 ;
  zmap *map = NULL ;
  ssize_t size = sizeof(zmap) + sizeof(uint16_t) * zni * znj ; // size of data map itself, header + table of sizes
  ssize_t hsize = size ;
  ssize_t lsize ;
  int32_t i, j, lbi, lbj ;
  int32_t zij, znij ;
  uint32_t *current ;

if(DEBUG) fprintf(stderr, "bsize = %d, gni = %d, gnj = %d, zni = %d, znj = %d\n", bsize, gni, gnj, zni, znj);
  // compute worst case block sizes for packed data = size of data rounded up to uint32_t size
  // packed blocks are supposed to be aligned to uint32_t boundaries
  lbj = ljx ;                       // longer/shorter second dimension in first row
  for(j=0 ; j<znj ; j++){
    lbi = lix ;                     // longer/shorter first dimension in first column
    for(i=0 ; i<zni ; i++){
      lsize = esize ;
      lsize = lbi * lbj * lsize ;   // worst case size for this block
      lsize = (lsize + sizeof(uint32_t) - 1) / sizeof(uint32_t) ;   // round to multiple of uint32_t size
      size = size + lsize * sizeof(uint32_t) ;
if(DEBUG) fprintf(stderr, "block[%d,%d] (%d,%d) size = %ld, esize = %ld\n", i, j, lbi, lbj, lsize, esize);
      lbi = lni ;                   // after first column
    }
    lbj = lnj ;                     // after first row
  }
if(DEBUG) fprintf(stderr, "buffer size = %ld\n", size) ;
  map = (zmap *) malloc(size) ;
  if(map){         // allocation was successful
    int32_t *data = (int32_t *)&(map->size[zni*znj]) ;
if(DEBUG) fprintf(stderr, "data offset = %ld bytes, hsize = %ld[%ld+%ld]\n", (uint8_t *)data - (uint8_t *)map, hsize, sizeof(zmap), sizeof(uint16_t) * zni * znj) ;
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
    znij        = zni * znj ;
    map->mh.mem = (zblocks *)malloc( (znij + 1) * sizeof(uint32_t *) ) ;
    if(map->mh.mem == NULL){    // failed to allocate pointer table
      free(map) ;
      map = NULL ;
      goto end ;
    }
    map->mh.mem[0] = map->mh.first = (uint32_t *)data ;
if(DEBUG) fprintf(stderr, "mem[0] offset : %ld\n",  (uint8_t *)map->mh.mem[0] - (uint8_t *) map) ;
    lbj = ljx ;                       // longer/shorter second dimension in first row
    for(j=0 ; j<znj ; j++){
      lbi = lix ;                     // longer/shorter first dimension in first column
      for(i=0 ; i<zni ; i++){
        lsize = esize ;
        lsize = lbi * lbj * lsize ;   // worst case size for this block
        lsize = (lsize + sizeof(uint32_t) - 1) / sizeof(uint32_t) ;   // round to multiple of uint32_t size
        zij = Zindex_from_i_j(i, j, zni, znj, stripe) ;
        map->size[zij] = lsize ;      // set worstcase size for this zigzag indexed block
        lbi = lni ;                   // after first column
      }
      lbj = lnj ;                     // after first row
    }
    for(i=0 ; i<znij ; i++) map->mh.mem[i+1] = map->mh.mem[i] + map->size[i] ;
    map->mh.limit = map->mh.mem[znij] ;
if(DEBUG) {
  fprintf(stderr, "range     : ");
  for(i=0 ; i<znij ; i++)fprintf(stderr, "%6ld", map->mh.mem[i] - map->mh.mem[0]);
  fprintf(stderr, "\n");
  fprintf(stderr, "            ");
  for(i=0 ; i<znij ; i++)fprintf(stderr, "%6ld", map->mh.mem[i+1] - map->mh.mem[0] - 1);
  fprintf(stderr, "\n");
  fprintf(stderr, "span      : ");
  for(i=0 ; i<znij ; i++)fprintf(stderr, "%6ld", map->mh.mem[i+1] - map->mh.mem[i]);
  fprintf(stderr, "\n");
  fprintf(stderr, "map->size : ");
  for(i=0 ; i<znij ; i++)fprintf(stderr, "%6d", map->size[i]);
  fprintf(stderr, "\n");
}

for(j=znj ; j>0 ; j--){
  for(i=0 ; i<zni ; i++){
    zij = Zindex_from_i_j(i, j-1, zni, znj, stripe) ;
    if(DEBUG) fprintf(stderr, "%6d[%2d,%2d](%2d)", map->size[zij], i, j-1, zij);
  }
  if(DEBUG) fprintf(stderr, "\n");
}
  }
end:
  current = map->mh.mem[0] ;          // initial position
  if(current != map->mh.first){
    int32_t *data = (int32_t *)&(map->size[map->zni*map->znj]) ;
    fprintf(stderr, "ERROR new_map : first map entry not pointing to start of stream\n") ;
    fprintf(stderr, "      first = %16p, start = %16p, data = %16p\n", (void *)map->mh.first, (void *)current, (void *)data) ;
//   }else{
//     fprintf(stderr, "DEBUG new_map : first map entry points to start of stream\n") ;
  }
  return map ;
}

// allocate table of pointers to packed blocks, fill it using map->size
// map  [INOUT] : pointer to data map
// data    [IN] : pointer to start of packed data (if NULL, packed data follows map in memory)
// return address of table of pointers to packed blocks, NULL if allocation failed
zblocks *mem_zmap(zmap *map, uint32_t *data){
  int32_t znij = map->zni * map->znj ;
  zblocks *mem = (zblocks *)malloc((znij+1) * sizeof(uint32_t *)) ;

  if(mem){          // allocation was successful
    int i ;
    map->mh.mem = mem ;
    mem[0] = data ? data : (uint32_t *)&(map->size[znij]) ;      // if data is NULL, packed data stream follows data map in memory
    for(i=1 ; i<znij+1 ; i++) mem[i] = mem[i-1] + map->size[i-1] ;
  }
  if(data != NULL){
    map->mh.first = mem[0] ;
    map->mh.limit = mem[znij] ;
    fprintf(stderr, "DEBUG mem_zmap, switching data buffer to %16p -> %16p\n", (void *)map->mh.first, (void *)map->mh.limit) ;
  }

//   uint32_t *current = map->mh.mem[0] ;          // initial position
//   if(current != map->mh.first){
//     int32_t *data = (int32_t *)&(map->size[map->zni*map->znj]) ;
//     fprintf(stderr, "ERROR mem_map : first map entry not pointing to start of stream\n") ;
//     fprintf(stderr, "      first = %16p, start = %16p, data = %16p\n", map->mh.first, current, data) ;
//   }
  return mem ;
}

// fill map data buffer with data from address src
// data element size and dimensions will be taken from map
// void fill_zmap(zmap *map, void *src){
// }

// full/partial deallocation of data map and its components
// map  [INOUT] : pointer to data map
// full    [IN] : if zero, only deallocate pointer table to packed blocks
int free_zmap(zmap *map, int full){
  if(map == NULL) return -1 ;
  if(map->mh.mem) free(map->mh.mem) ;
  map->mh.mem = NULL ;
  if(full) {
    free(map) ;
if(DEBUG) fprintf(stderr, "FULL map free\n") ;
  }else{
if(DEBUG) fprintf(stderr, "PART map free\n") ;
  }
  return 0 ;
}

ssize_t resize_map(zmap *map){
  int k ;
  uint32_t *current ;

  current = map->mh.mem[0] ;          // initial position
  if(current != map->mh.first){
    int32_t *data = (int32_t *)&(map->size[map->zni*map->znj]) ;
    fprintf(stderr, "ERROR resize_map : first map entry not pointing to start of stream\n") ;
    fprintf(stderr, "      first = %16p, start = %16p, data = %16p\n", (void *)map->mh.first, (void *)current, (void *)data) ;
  }
  for(k=0 ; k < map->zni * map->znj ; k++){
    map->mh.mem[k] = current ;
    current += map->size[k] ;
    if(current > map->mh.limit){
      fprintf(stderr, "ERROR: cannot resize map, not enough space occording to size table\n") ;
      return -1 ;
    }
  }
  return current - map->mh.mem[0] ;
}

// remove holes from data buffer, update list of memory addresses using updated sizes
ssize_t repack_map(zmap *map){
  int k ;
  uint32_t *current, *stream ;

  if(map == NULL)      return -1 ;
  if(map->mh.mem == NULL) return -1 ;

  current = map->mh.mem[0] ;          // initial target position
  if(current != map->mh.first){
    int32_t *data = (int32_t *)&(map->size[map->zni*map->znj]) ;
    fprintf(stderr, "ERROR resize_map : first map entry not pointing to start of stream\n") ;
    fprintf(stderr, "       first = %16p, start = %16p, data = %16p\n", (void *)map->mh.first, (void *)current, (void *)data) ;
  }
  for(k=0 ; k < map->zni * map->znj ; k++){
    stream = map->mh.mem[k] ;         // copy from this position in memory
    map->mh.mem[k] = current ;        // update mem pointer to new position in memory
    if(current < stream || map->size[k] != map->mh.mem[k+1] - map->mh.mem[k]) {  // need to copy ?
      if(DEBUG) fprintf(stderr, "copying from %6ld", current - map->mh.mem[0]) ;
      memmove(current, stream, map->size[k] * sizeof(uint32_t)) ;    // PGI/Nvidia compile problems with DEBUG =0 and copy loop
//       int i ;
//       for(i=0 ; i < map->size[k] ; i++){ current[i] = stream[i] ; }
      if(DEBUG) fprintf(stderr, " to %6ld [%6d]\n", current + map->size[k] - map->mh.mem[0] -1, map->size[k]) ;
    }
    current += map->size[k] ;      // update target position
  }
  map->mh.mem[k] = current ;
  return current - map->mh.mem[0] ;
}
