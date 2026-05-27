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
//     M. Valin,   Recherche en Prevision Numerique, 2024-2026
//

#include <stdio.h>
#include <string.h>

#include <rmn/data_map.h>

#define DEBUG 1

// NOTE: Zindex_to_ij, Zindex_from_ij, Z_map_index  may become irrelevant 
#if 0
// translate block Z (zigzag) index into block (i,j) coordinates
// zij    [IN] : Z (zigzag) index
// nti    [IN] : row size
// ntj    [IN] : number of rows
// sf0    [IN] : stripe width, last (top) stripe may be narrower
// the function returns the i and j coordinates in struct ij_range
index_pair Zindex_to_ij(int32_t zij, int32_t nti, int32_t ntj, int32_t sf0){
  index_pair ij ;
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
int32_t Zindex_from_ij(int32_t i, int32_t j, int32_t nti, int32_t ntj, int32_t sf0){
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

// Z (zigzag) block index from block indexes, using data map
// map    [IN] : data map
// i      [IN] : i (column) position in 2D block grid
// j      [IN] : j (row) position in 2D block grid
// return [ij] Z block index
int32_t Z_map_index(zmap *map, int32_t i, int32_t j){
//   index_pair ij = block_index(map, i, j) ;
//   return Zindex_from_ij(i, j, map->fhead.zni, map->fhead.znj, map->fhead.stripe) ;
  return Zindex_from_ij(i, j, map->fhead.zni, map->fhead.znj, 1) ;
}
#endif

// block position from grid index, using data map
// map    [IN] : data map
// i      [IN] : i (column) position in 2D grid
// j      [IN] : j (row) position in 2D grid
// return [i,j] block coordinates (different from z index)
index_pair block_index(zmap *map, int32_t i, int32_t j){
  index_pair ij = {.i = -1, .j = -1 } ;  // precondition for failure
  if(map->fhead.gni > i && map->fhead.gnj > j){
    ij.i = b_index(i, map->fhead.lni, map->fhead.li0) ;
    ij.j = b_index(j, map->fhead.lnj, map->fhead.lj0) ;
  }
  return ij ;
}

// area covered by block[j][i]
// map    [IN] : data map
// bi     [IN] : block column index
// bj     [IN] : block row index
// return i and j index limits for area covered by block[j][i]
ij_range map_block_limits(zmap *map, int32_t bi, int32_t bj){
  ij_range ij = {.i0 = -1, .in = -2, .j0 = -1, .jn = -2 } ;  // precondition for failure

  if(bi < map->fhead.zni && bj < map->fhead.znj && bi >= 0 && bj >= 0){  // inside map limits ?
    index_range r ;
    r = r_limits(bi, map->fhead.lni, map->fhead.li0) ;                   // get block limits along first dimension (row)
    ij.i0 = r.ix0 ; ij.in = r.ixn ;
    r = r_limits(bj, map->fhead.lnj, map->fhead.lj0) ;                   // get block limits along second dimension (column)
    ij.j0 = r.ix0 ; ij.jn = r.ixn ;
  }
  return ij ;
}

// allocate a new data map in memory, using size from file (meta[1] in record)
// map_words [IN] : size in 32 bit units of data map in rsf file ( 0 if no data map in record )
// rec_words [IN] : size in 32 bit units of data record in rsf file
// return address of zmap
// zmap will be nullified except for mmap signature, version, zmapend, and fmapend
// data record size includes data map size
// a record without a data map will have map_words == 0
// rec_words < map_words is the same as map_words == 0
zmap *new_file_zmap(uint32_t map_words, uint32_t rec_words){
  size_t mapsize = map_words * sizeof(uint32_t) + sizeof(mmap) ;
  size_t recsize = rec_words * sizeof(uint32_t) + sizeof(mmap) ;
  recsize = (recsize < mapsize) ? mapsize : recsize ;
  if(recsize <= sizeof(mmap)) return NULL ;    // zero size record

  zmap *r = (zmap *) malloc(recsize) ;
  if(r != NULL){
    r->mhead = null_mmap ;
    r->mhead.signature = 0x1AD0FADA ;
    r->mhead.version   = Z_DATA_MAP_VERSION ;
    r->mhead.zmapend = (uint32_t *)( (char *)r + recsize ) ;
    r->mhead.fmapend = (uint32_t *)( (char *)r + mapsize ) ;
    r->fhead = null_fmap ;
  }
  return r ;
}

void zmap_print(zmap *map){
  size_t mmap_size = (char *)&(map->fhead.signature) - (char *)map ;
  size_t fmap_size = (char *)map->mhead.fmapend - (char *)&(map->fhead.signature) ;
  size_t zmap_size = (char *)map->mhead.zmapend - (char *)map ;
  fprintf(stderr, "zmap_print : zmap @ %p, mmap size = %ld, fmap size = %ld, data size = %ld, zmap size = %ld\n",
          (void *)map, mmap_size, fmap_size, zmap_size - fmap_size - mmap_size, zmap_size ) ;
}

int fmap_invalid(zmap *map){
  if(map->fhead.signature != 0xBEBEFADA)       return 1 ;
  if(map->fhead.version != Z_DATA_MAP_VERSION) return 2 ;
  if(map->fhead.gni <= 0 || map->fhead.gnj <= 0 || map->fhead.gnk <= 0) return 3 ;
  if(map->fhead.zni <= 0 || map->fhead.znj <= 0) return 4 ;
  if(map->fhead.li0 <= 0 || map->fhead.lj0 <= 0) return 5 ;   // first block dimension may not be <= 0
  if(map->fhead.lni <  0 || map->fhead.lnj <  0) return 6 ;   // next blocks dimension may be 0 but not < 0
  return 0 ;
}

void fmap_init(zmap *map, int32_t gni, int32_t gnj, int32_t gnk, int32_t bsizex, int32_t bsizey){
  array_axis_3d r = split_axis_3d(gni, gnj, gnk, bsizex, bsizey) ;
  map->fhead.signature = 0xBEBEFADA ;
  map->fhead.version   = Z_DATA_MAP_VERSION ;
  map->fhead.flags = 0 ;
  map->fhead.gni = gni ;
  map->fhead.gnj = gnj ;
  map->fhead.gnk = gnk ;
  map->fhead.zni = r.x.nbk ;
  map->fhead.li0 = r.x.ln0 ;
  map->fhead.lni = r.x.ln1 ;
  map->fhead.znj = r.y.nbk ;
  map->fhead.lj0 = r.y.ln0 ;
  map->fhead.lnj = r.y.ln1 ;
}

void fmap_print(zmap *map){
  int code = fmap_invalid(map) ;
  if(code != 0) fprintf(stderr, "ERROR: (%d) invalid map\n", code) ;
  if(map->fhead.signature != 0xBEBEFADA){
    fprintf(stderr, "3D fmap : INVALID signature, expecting 0xBEBEFADA, got %8.8x\n", map->fhead.signature) ;
  }else{
    fprintf(stderr, "3D fmap : version %4.4x, flags(%4.4x), data[%d:%d:%d], blocks[%d:%d:%d], (%d,%d : %d,%d : %d)\n",
            map->fhead.version, map->fhead.flags,
            map->fhead.gni, map->fhead.gnj, map->fhead.gnk,
            map->fhead.zni, map->fhead.znj, map->fhead.gnk,
            map->fhead.li0, map->fhead.lni, map->fhead.lj0, map->fhead.lnj, map->fhead.gnk ) ;
  }
}

// address of file data map in zmap memory structure
void *filemap_address(zmap *map){
  return &(map->fhead.signature) ;
}

// size of file data map in 32 bit units
uint32_t filemap_words(zmap *map){
  return ( (char *)map->mhead.fmapend - (char *)(&(map->fhead.signature)) ) / sizeof(uint32_t);
}

uint32_t filemap_blocks(int32_t gni, int32_t gnj, int32_t gnk, int32_t bsizex, int32_t bsizey){
  array_axis_3d r = split_axis_3d(gni, gnj, gnk, bsizex, bsizey) ;
// fprintf(stderr, "\n   x axis 1 x %d, %d x %d, y axis 1 x %d, %d x %d\n", r.x.ln0, r.x.nbk-1, r.x.ln1, r.y.ln0, r.y.nbk-1, r.y.ln1);
  return (r.x.nbk * r.y.nbk * r.z.nbk) ;
}

// needed size in bytes of file data map
size_t filemap_needed_size(int32_t gni, int32_t gnj, int32_t gnk, int32_t bsizex, int32_t bsizey){
  uint32_t blocks = filemap_blocks(gni, gnj, gnk, bsizex, bsizey) ;
  blocks = (blocks + 1) & 0xFFFFFFFE ;  // force number of blocks to even number >= number of blocks
  return ( blocks *sizeof(fmap_block_size) + sizeof(fmap) ) ;
}

// needed size in 32 bit units of file data map
size_t filemap_needed_words(int32_t gni, int32_t gnj, int32_t gnk, int32_t bsizex, int32_t bsizey){
  return filemap_needed_size(gni, gnj, gnk, bsizex, bsizey) / sizeof(uint32_t) ;
}

size_t zmap_needed_size(int32_t gni, int32_t gnj, int32_t gnk, int32_t bsizex, int32_t bsizey){
  return filemap_needed_size(gni, gnj, gnk, bsizex, bsizey) + sizeof(mmap) ;
}

// TODO:
// separate zmap create from zmap populate ?
// function to calculate worst case data size from block_sizes[]
//
// create a data map with a worst case buffer for map and packed data
// gni    [IN] : first dimension of array (row size)
// gnj    [IN] : second dimension of array (number of rows)
// aspect [IN] : 2D aspect ratio (size along j = aspect * size along i)
// esize  [IN] : size in bytes of array elements (normally 1/2/4/8)
// mextra [IN] : size of extra global information for data decoding (in bytes)
//               mextra will be roubded up to a multiple of sizeof(uint32_t)
// NOTE: array dimensions are Fortran ordered (i index varying first)
//
// zmap    *new_zmap(int32_t gni, int32_t gnj, int32_t aspect, size_t esize, int32_t extra,
//                   int32_t blocksize, int32_t *data, int32_t *mem);
zmap *new_zmap(int32_t gni, int32_t gnj, int32_t gnk, int32_t bx_size, int32_t aspect, size_t esize, int32_t mextra){
  mextra = (mextra + sizeof(uint32_t) - 1) / sizeof(uint32_t) ; // round up to multiple of uint32_t size
  mextra = mextra * sizeof(uint32_t) ;
  if(bx_size == 0) bx_size = 64 ;         // default block size of 64 x 64
  array_axis p ;
  // split first dimension of array
  p = split_axis(gni, bx_size) ;
  int32_t zni = p.nbk ;               // number of blocks along i
  int32_t lni = p.ln1 ;               // bx_size
  int32_t li0 = p.ln0 ;               // size of first block along i
  // split second dimension of array
  p = split_axis_min(gnj, bx_size*aspect, bx_size/2)  ;
  int32_t znj = p.nbk ;               // number of blocks along j
  int32_t lnj = p.ln1 ;               // bx_size
  int32_t lj0 = p.ln0 ;               // size of first block along j
  // no split along third dimension  TODO : add gnk to argument list ?
//   int32_t znk = 1 ;

  zmap *map = NULL ;
  ssize_t size ;
  size = sizeof(zmap) + sizeof(uint16_t) * zni * znj ;          // base size of data map + table of sizes
  ssize_t spad = size ;
  size = (size + sizeof(uint32_t) - 1) / sizeof(uint32_t) ;     // round up to next multiple of uint32_t size
  size = size * sizeof(uint32_t) ;
  spad = size - spad ;                                          // size of padding (0,1,2,3)
  size = size + mextra * sizeof(uint32_t) ;                     // + size of global information
  ssize_t hsize = size ;                                        // size without data but including global information
  int32_t i ;
  int32_t znij ;
  uint32_t *current ;

  // compute worst case size needed to encode data (esize bytes per element)
  // worst case = gni * gnj * ( esize * 8 + 1) bits  (assume 1 overhead bit per element)
  // worst case = gni * gnj * esize + zni * znj * (4 + 4)  (4 bytes round up + 4 bytes overhead per block)
  ssize_t nbytes = esize * 8 + 1 ; nbytes *= gni ; nbytes *= gnj ;
  nbytes = ((nbytes + 3) / 4) * 4 ;  // bump to next multiple of 4 bytes
  size += nbytes ;

  if(DEBUG)
    fprintf(stderr, "sizeof(mhead) = %ld, sizeof(fhead) = %ld, sizeof(zmap) = %ld\n", sizeof(map->mhead), sizeof(map->fhead), sizeof(zmap)) ;
  if(DEBUG)
    fprintf(stderr, "bx_size = %d, gni = %d, gnj = %d, zni = %d, znj = %d\n", bx_size, gni, gnj, zni, znj);

  // allocate map with enough space for worst case
  map = (zmap *) malloc(size) ;     // hsize + sum of lsize(s)
  if(map){         // allocation was successful
    map->mhead.limit = (uint8_t *)map + size ;   // 1 + end of data stream buffer
    if(DEBUG)
      fprintf(stderr, "allocated zmap at %p, [%ld bytes], size table[%d,%d] at %p\n", map, size, zni, znj, map->size) ;
    znij  = zni * znj ;
    uint8_t *data = (uint8_t *) map ;
    data += hsize ;                         // data bit stream just after extra global information
    if(DEBUG)
      fprintf(stderr, "data offset = %ld bytes, hsize = %ld[base=%ld , sizes=%ld, pad=%ld, extra=%ld]\n",
                (uint8_t *)data - (uint8_t *)map, hsize, sizeof(zmap), sizeof(uint16_t)*znij, spad, mextra*sizeof(uint32_t)) ;
    map->fhead.signature = 0xBEBEFADA ;
    map->fhead.version   = Z_DATA_MAP_VERSION ;
//     map->fhead.stripe    = stripe ;
//     map->fhead.mextra     = mextra ;
    map->fhead.flags     = 0 ;
//     map->fhead.meta      = zmeta_null ;
    map->fhead.gni       = gni ;
    map->fhead.gnj       = gnj ;
    map->fhead.gnk       = gnk ;
    map->fhead.zni       = zni ;
    map->fhead.znj       = znj ;
    map->fhead.lni       = lni ;
    map->fhead.lnj       = lnj ;
    map->fhead.li0       = li0 ;
    map->fhead.lj0       = lj0 ;
    map->mhead.signature = 0x1AD0FADA ;
//     map->mhead.options   = NULL ;
    map->mhead.mem = (zblocks *)malloc( (znij + 1) * sizeof(uint32_t *) ) ;
    if(DEBUG){
      fprintf(stderr, "map at %p", map) ;
      fprintf(stderr, ", size table [%d + 1] at %p", znij, &(map->size)) ;
      fprintf(stderr, ", extra [%d] at %p", mextra, data - mextra) ;
      fprintf(stderr, ", data  at %p\n", data) ;
    }
    if(map->mhead.mem == NULL){    // failed to allocate pointer table
      free(map) ;
      map = NULL ;
      goto end ;
    }
    map->mhead.mem[0] = map->mhead.first = (uint32_t *)data ;   // start of data
    map->mhead.extra = map->mhead.first - mextra/sizeof(uint32_t) ;
    if(DEBUG){
      fprintf(stderr, "mem[0] = %p,  offset : %ld", map->mhead.mem[0], (uint8_t *)map->mhead.mem[0] - (uint8_t *) map) ;
      fprintf(stderr, ", extra  offset : %ld\n",  (uint8_t *)map->mhead.extra - (uint8_t *) map) ;
    }
    for(i=0 ; i<znij ; i++){ map->size[i] = 0 ; }
  }

end:
  current = map->mhead.mem[0] ;          // initial position
  if(current != map->mhead.first){
    // data starts after the size table
    int32_t *data = (int32_t *)&(map->size[map->fhead.zni*map->fhead.znj]) ;
    data += mextra ;
    fprintf(stderr, "ERROR new_map : first map entry not pointing to start of stream\n") ;
    fprintf(stderr, "      first = %16p, start = %16p, data = %16p\n", (void *)map->mhead.first, (void *)current, (void *)data) ;
//   }else{
//     fprintf(stderr, "DEBUG new_map : first map entry points to start of stream\n") ;
  }
  return map ;
}

void print_zmap(zmap *map, char *msg){
  fprintf(stderr, "================= %s =================\n", msg) ;
  int bszi = map->fhead.lni ; bszi = (bszi == 0) ? map->fhead.li0 : bszi ;
  int bszj = map->fhead.lnj ; bszj = (bszj == 0) ? map->fhead.lj0 : bszj ;
  fprintf(stderr, "array[%d,%d,%d]", map->fhead.gni, map->fhead.gnj, map->fhead.gnk) ;
  fprintf(stderr, ", map[%d,%d], sizes = [%d(%d) , %d(%d)]", map->fhead.zni, map->fhead.znj, bszi, map->fhead.li0, bszj, map->fhead.lj0) ;
  fprintf(stderr, "\n");
  fprintf(stderr, "================= end of map =================\n") ;
}

// adjust mem contents according to sizes
int fillmem_zmap(zmap *map){
  if(map == NULL) return 0 ;
  int ij, nij ;
  uint32_t *t ;
  nij = map->fhead.zni * map->fhead.znj ;
  for(ij=0 ; ij<nij ; ij++){
    t = map->mhead.mem[ij] + map->size[ij] ;
    if((uint8_t *)t >= map->mhead.limit) return 0 ;
    map->mhead.mem[ij+1] = t ;
  }
  return nij ;
}

// create sizes from block dimensions
int bsize_zmap(zmap *map, size_t esize){
  if(map == NULL) return 0 ;
  int i, j, lni, lnj, ij ;
  ssize_t lsize ;
  lnj = map->fhead.lj0 ;
  ij = 0 ;
  for(j=0 ; j<map->fhead.znj ; j++, lnj=map->fhead.lnj){
    lni = map->fhead.li0 ;
    for(i=0 ; i<map->fhead.zni ; i++, lni=map->fhead.lni ){
      lsize = esize ;
      lsize *= (lni * lnj) ;
      map->size[ij++] = lsize ;
    }
  }
  return ij ;
}

// (re)allocate table of pointers to packed blocks, fill it using map->size
// map  [INOUT] : pointer to data map
// data    [IN] : pointer to start of packed data (if NULL, packed data follows map in memory)
// size    [IN] : size of memory block at data in bytes
// return address of table of pointers to packed blocks, NULL if there was any error
zblocks *mem_zmap(zmap *map, uint32_t *data, size_t size){
  int32_t znij = map->fhead.zni * map->fhead.znj, i ;
  size_t needed = 0 ;

  if(data != NULL){    // check that enough space is available, set first/last/limit
    for(i=0 ; i<znij ; i++){ needed += map->size[i] ; }
    if(size < needed) return NULL ;
    map->mhead.extra = data ;
    map->mhead.first = data /*+ map->fhead.mextra*/ ;
    map->mhead.last  = map->mhead.first ;
    map->mhead.limit = (uint8_t *)data + size ;
    if(DEBUG)
      fprintf(stderr, "DEBUG mem_zmap, switching stream buffer to %16p -> %16p\n", (void *)map->mhead.first, (void *)map->mhead.limit) ;
  }
  // allocate mem table
  zblocks *mem = (zblocks *)malloc((znij+1) * sizeof(uint32_t *)) ;  // znij + 1 entries needed

  if(mem){          // allocation was successful
    if(map->mhead.mem) free(map->mhead.mem) ;  // free old table if there was one
    map->mhead.mem = mem ;
    mem[0] = map->mhead.first ;
    for(i=1 ; i<znij+1 ; i++){
      mem[i] = mem[i-1] + map->size[i-1] ;   // recalculate mem[] using block sizes
    }
    if((uint8_t *)mem[znij] > map->mhead.limit){   // OOPS, not enough space in data
      free(mem) ;
      mem = NULL ;
    }else{
      map->mhead.last = mem[znij] ;                      // update last
    }
  }

  return mem ;
}

// fill map data buffer with data from address src
// data element size and dimensions will be taken from map
// void fill_zmap(zmap *map, void *src){
// }

// NOTE if first/last/limit out of map we have an external bit stream buffer

// full/partial deallocation of data map and its components
// map  [INOUT] : pointer to data map
// full    [IN] : if zero, only deallocate pointer table to packed blocks
int free_zmap(zmap *map, int full){
  if(map == NULL) return -1 ;
  if(map->mhead.mem){
    if(DEBUG) fprintf(stderr, "freeing map->mhead.mem at %p\n", map->mhead.mem) ;
    free(map->mhead.mem) ;
  }
  map->mhead.mem = NULL ;
//   if(map->mhead.options){
//     if(DEBUG) fprintf(stderr, "freeing map->mhead.options at %p\n", map->mhead.options) ;
//     free(map->mhead.options) ;
//   }
//   map->mhead.options = NULL ;
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

  current = map->mhead.mem[0] ;          // initial position
  if(current != map->mhead.first){
    int32_t *data = (int32_t *)&(map->size[map->fhead.zni*map->fhead.znj]) ;
    fprintf(stderr, "ERROR resize_map : first map entry not pointing to start of stream\n") ;
    fprintf(stderr, "      first = %16p, start = %16p, data = %16p\n", (void *)map->mhead.first, (void *)current, (void *)data) ;
  }
  for(k=0 ; k < map->fhead.zni * map->fhead.znj ; k++){
    map->mhead.mem[k] = current ;
    current += map->size[k] ;
    if(current > map->mhead.last){
      fprintf(stderr, "ERROR: cannot resize map, not enough space occording to size table\n") ;
      return -1 ;
    }
  }
  return current - map->mhead.mem[0] ;
}

// remove holes from data buffer, update list of memory addresses using updated sizes
ssize_t repack_map(zmap *map){
  int k ;
  uint32_t *current, *stream ;

  if(map == NULL)      return -1 ;
  if(map->mhead.mem == NULL) return -1 ;

  current = map->mhead.mem[0] ;          // initial target position
  if(current != map->mhead.first){
    int32_t *data = (int32_t *)&(map->size[map->fhead.zni*map->fhead.znj]) ;
    fprintf(stderr, "ERROR resize_map : first map entry not pointing to start of stream\n") ;
    fprintf(stderr, "current = %p, first = %16p, start = %16p, data = %16p\n", (void *)current, (void *)map->mhead.first, (void *)current, (void *)data) ;
  }
  for(k=0 ; k < map->fhead.zni * map->fhead.znj ; k++){
    stream = map->mhead.mem[k] ;         // copy from this position in memory
    map->mhead.mem[k] = current ;        // update mem pointer to new position in memory
    if(current < stream || map->size[k] != map->mhead.mem[k+1] - map->mhead.mem[k]) {  // need to copy ?
      if(DEBUG) fprintf(stderr, "copying from %6ld", current - map->mhead.mem[0]) ;
      memmove(current, stream, map->size[k] * sizeof(uint32_t)) ;    // PGI/Nvidia compile problems with DEBUG =0 and copy loop
//       int i ;
//       for(i=0 ; i < map->size[k] ; i++){ current[i] = stream[i] ; }
      if(DEBUG) fprintf(stderr, " to %6ld [%6d]\n", current + map->size[k] - map->mhead.mem[0] -1, map->size[k]) ;
    }
    current += map->size[k] ;      // update target position
  }
  map->mhead.mem[k] = current ;
  return current - map->mhead.mem[0] ;
}
