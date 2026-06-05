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

// NOTE: Zindex_to_ij, Zindex_from_ij, Z_map_index  may become irrelevant (zigzag indexing no longer contemplated)
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

// allocate a new zmap struct in memory, using sizes from file (meta[1] and record size in record)
// map_words [IN] : size in 32 bit units of data map from rsf file ( 0 if no data map in record )
// rec_words [IN] : size in 32 bit units of data record from rsf file (0 if no data portion to be allocated)
// return address of zmap (space for data stream is optional)
//
// zmap will be nullified except for mmap signature, version, and the address ranges
// data record size includes data map size
// a record without a data map will have map_words == 0
// rec_words < map_words is the same as rec_words == 0
// in intending to only read the data map, set rec_words to 0
// if rec_words > map_words, map_words is ignored and rec_words is used
zmap *create_file_zmap(uint32_t map_words, uint32_t rec_words){
  size_t mapsize = map_words * sizeof(uint32_t) + sizeof(mmap) ;    // data map only
  size_t recsize = rec_words * sizeof(uint32_t) + sizeof(mmap) ;    // data map and data stream
  recsize = (recsize < mapsize) ? mapsize : recsize ;
  if(recsize <= sizeof(mmap)) return NULL ;                         // zero size record and no map

  zmap *r = (zmap *) malloc(recsize) ;               // attempt to allocate
fprintf(stderr, "create_file_zmap : map_words = %d, rec_words = %d, recsize = %ld\n", map_words, rec_words, recsize) ;
  if(r != NULL){
    r->mhead = base_mmap ;                           // set signature and version
    // entire zmap struct (with or without space for data stream)
    SET_RANGE(r->mhead.zrng, r, recsize) ;

    // fmap : base size + sizes table + mextra == data map size from record metadata
    SET_RANGE(r->mhead.frng, &(r->fhead.signature), map_words * sizeof(uint32_t)) ;

    // "extra region" size and address will be known once the data map has been read from file
    r->mhead.xrng.top = r->mhead.xrng.bot = NULL ;

    // "data region" not including "extra"
    if(recsize == mapsize){
      r->mhead.drng.bot =  r->mhead.drng.top = NULL ;          // map only, no data
    }else{
      r->mhead.drng.top = r->mhead.zrng.top ;                  // address of top known
      r->mhead.drng.bot = r->mhead.frng.top ;                  // address of bottom may be "extra" off
    }

    // sizes[] table (address known , size unknown yet)
    SET_RANGE(r->mhead.srng, &(r->size), 0) ;

    // TODO: worst case estimate of number of blocks : map_words * 2
    // offset[] table, no address, no space for it in zmpa
    r->mhead.orng.top = r->mhead.orng.bot = NULL ;

    r->fhead = null_fmap ;                           // fmap part zeroed out, ready to be read from file
  }
  return r ;
}

// update contents of mmap after fmap part has been read from file
// map   [INOUT] : pointer to valid zmap struct
// fix_mem  [IN] : allocate memory pointer table
int update_file_zmap(zmap *map, int fix_mem){
  int status = fmap_invalid(map) ;
  if(status != 0) return status ;      // is fmap portion valid ?
  void *offset = NULL ;

  // "extra" region size and position can now be determined
  map->mhead.xrng.top = map->mhead.frng.top ;                       // top of "extra" at top of fmap
  map->mhead.xrng.bot = map->mhead.frng.top - map->fhead.extra ;    // extra is in 32 bit units, top and bot are pointers to 32 bit integers

  // fix bottom address of data  (must include "extra") if there is room for data (data bottom pointer not NULL)
  if(map->mhead.drng.bot != NULL){
    map->mhead.drng.bot = map->mhead.xrng.top ;
  }
  // update sizes table range
  SET_RANGE(map->mhead.srng, &(map->size), sizeof(fmap_block_size) * map->fhead.zijk) ;
  // update offsets/offset range orng

  // allocate offset[] table ?
  if(fix_mem){
    size_t alloc_size = sizeof(void *) * (map->fhead.zijk + 1) ;
    offset = malloc(alloc_size) ;
    if(offset == NULL) return 2 ;               // allocation failed
    SET_RANGE(map->mhead.orng, offset, alloc_size );
    // fill offset table using data pointers and sizes table
  }
  return 0 ;
}

// typedef struct{            // in memory only part of data map
//   uint32_t signature ;     // should be 0x1AD0FADA, target for & operator to get address of header
//   uint16_t version ;       // version marker (MUST BE the same as in file header)
//   uint16_t  options ;      // reserved for internal use options
//   zblocks  *offset ;          // table[zni*znj] : memory addresses of encoded blocks in memory
//   RANGE(uint32_t) xrng ;   // address range for "extra" information
//   RANGE(uint32_t) frng ;   // address range for the file portion of the data map (includes "extra" information)
//   RANGE(uint32_t) drng ;   // address range for the data portion of the data map (above file part, includes "extra" information)
//   RANGE(uint32_t) zrng ;   // address range for the entire data map
// } mmap ;

// print contents of memory header from zmap
// map  [IN] : pointer to valid zmap struct
// msg  [IN] : map name (cosmetic)
void zmap_print(zmap *map, char *msg){
  if(map->mhead.signature != 0x1AD0FADA){
    fprintf(stderr, "zmap %s : INVALID signature, expected 0x1AD0FADA, got %8.8x\n", msg, map->fhead.signature) ;
    return ;
  }
  char *f1 = "   %s :  %16p -> %16p [+%6d] (%ld bytes)\n" ;
  char *f2 = "   %s :  %16p -> %16p           (%ld bytes)\n" ;
  size_t fmap_size = RANGE_SIZE(map->mhead.frng) ;
  size_t smap_size = RANGE_SIZE(map->mhead.srng) ;
  size_t xmap_size = RANGE_SIZE(map->mhead.xrng) ;
  size_t dmap_size = RANGE_SIZE(map->mhead.drng) ;
  size_t pmap_size = RANGE_SIZE(map->mhead.orng) ;
  size_t zmap_size = RANGE_SIZE(map->mhead.zrng) ;
  fprintf(stderr, "zmap %s : version %4.4x, '%4.4x', options = %8.8x, bit stream at %p \n",
          msg, map->mhead.version, map->mhead.signature, map->mhead.options, &(map->mhead.stream)) ;
//   fprintf(stderr, f1, "FULL", map->mhead.zrng.bot, map->mhead.zrng.top, ADDRESS_DIFF(map,map->mhead.zrng.bot), zmap_size) ;
  fprintf(stderr, f1, "FULL", map->mhead.zrng.bot, RANGE_LIMIT(map->mhead.zrng), ADDRESS_DIFF(map,map->mhead.zrng.bot), zmap_size) ;
//   fprintf(stderr, f1, "File", map->mhead.frng.bot, map->mhead.frng.top, ADDRESS_DIFF(map,map->mhead.frng.bot), fmap_size) ;
  fprintf(stderr, f1, "File", map->mhead.frng.bot, RANGE_LIMIT(map->mhead.frng), ADDRESS_DIFF(map,map->mhead.frng.bot), fmap_size) ;
//   fprintf(stderr, f1, "Smem", map->mhead.srng.bot, map->mhead.srng.top, ADDRESS_DIFF(map,map->mhead.srng.bot), smap_size) ;
  fprintf(stderr, f1, "Smem", map->mhead.srng.bot, RANGE_LIMIT(map->mhead.srng), ADDRESS_DIFF(map,map->mhead.srng.bot), smap_size) ;
  if(map->mhead.xrng.bot)
//     fprintf(stderr, f1, "Xtra", map->mhead.xrng.bot, map->mhead.xrng.top, ADDRESS_DIFF(map,map->mhead.xrng.bot), xmap_size) ;
    fprintf(stderr, f1, "Xtra", map->mhead.xrng.bot, RANGE_LIMIT(map->mhead.xrng), ADDRESS_DIFF(map,map->mhead.xrng.bot), xmap_size) ;
  else
//     fprintf(stderr, f2, "Xtra", map->mhead.xrng.bot, map->mhead.xrng.top, xmap_size) ;
    fprintf(stderr, f2, "Xtra", map->mhead.xrng.bot, RANGE_LIMIT(map->mhead.xrng), xmap_size) ;
//   fprintf(stderr, f1, "Data", map->mhead.drng.bot, map->mhead.drng.top, ADDRESS_DIFF(map,map->mhead.drng.bot), dmap_size) ;
  fprintf(stderr, f1, "Data", map->mhead.drng.bot, RANGE_LIMIT(map->mhead.drng), ADDRESS_DIFF(map,map->mhead.drng.bot), dmap_size) ;
  if(map->mhead.orng.bot)
    if((uint8_t *)(map->mhead.orng.bot) > (uint8_t *)(map->mhead.zrng.bot) && (uint8_t *)(map->mhead.orng.bot) < (uint8_t *)(map->mhead.zrng.top))
//       fprintf(stderr, f1, "Pmem", map->mhead.orng.bot, map->mhead.orng.top, ADDRESS_DIFF(map,map->mhead.orng.bot), pmap_size) ;
      fprintf(stderr, f1, "Pmem", map->mhead.orng.bot, RANGE_LIMIT(map->mhead.orng), ADDRESS_DIFF(map,map->mhead.orng.bot), pmap_size) ;
    else
//       fprintf(stderr, f2, "Pmem", map->mhead.orng.bot, map->mhead.orng.top, pmap_size) ;
      fprintf(stderr, f2, "Pmem", map->mhead.orng.bot, RANGE_LIMIT(map->mhead.orng), pmap_size) ;
  else
//     fprintf(stderr, f2, "Pmem", map->mhead.orng.bot, map->mhead.orng.top, pmap_size) ;
    fprintf(stderr, f2, "Pmem", map->mhead.orng.bot, RANGE_LIMIT(map->mhead.orng), pmap_size) ;
}

// basic validity check on data map
// return 0 if valid, non zero error code if not
int fmap_invalid(zmap *map){
  if(map->fhead.signature != 0xBEBEFADA)       return 1 ;
  if(map->fhead.version != Z_DATA_MAP_VERSION) return 2 ;
  if(map->fhead.gni <= 0 || map->fhead.gnj <= 0 || map->fhead.gnk <= 0) return 3 ;
  if(map->fhead.zni <= 0 || map->fhead.znj <= 0) return 4 ;
  if(map->fhead.li0 <= 0 || map->fhead.lj0 <= 0) return 5 ;   // first block dimension may not be <= 0
  if(map->fhead.lni <  0 || map->fhead.lnj <  0) return 6 ;   // next blocks dimension may be 0 but not < 0
  return 0 ;
}

// initialize the fmap permanent part of the data map using dimensions and blocking information
// map  [INOUT] : pointer to zmap (mmap/fmap/sizes/...) struct
// gni     [IN] : first dimension of array (row size)
// gnj     [IN] : second dimension of array (number of rows)
// gnk     [IN] : third dimension of array (number of planes)
// bsizex  [IN] : blocking size along the first dimension (i)
// bsizey  [IN] : blocking size along the second dimension (i)
// a3      [IN] : optional pointer to array axis struct, if NULL, split_axis_3d will be called
// extra   [IN] : extra number of blocks to add to decomposition
// bsizex and bsizey are used only when a3 == NULL
void fmap_init(zmap *map, int32_t gni, int32_t gnj, int32_t gnk, int32_t bsizex, int32_t bsizey, array_axis_3d *a3, int extra){
  array_axis_3d r ;
  if(a3 == NULL){
    r = split_axis_3d(gni, gnj, gnk, bsizex, bsizey) ;    // 3D decomposition not supplied, compute it
    a3 = &r ;
  }
  if(extra < 0) extra = 0 ;
  map->fhead = base_fmap ;  // signature and version
  map->fhead.gni = gni ;
  map->fhead.gnj = gnj ;
  map->fhead.gnk = gnk ;
  map->fhead.zni = a3->x.nbk ;
  map->fhead.li0 = a3->x.ln0 ;
  map->fhead.lni = a3->x.ln1 ;
  map->fhead.znj = a3->y.nbk ;
  map->fhead.lj0 = a3->y.ln0 ;
  map->fhead.lnj = a3->y.ln1 ;
  map->fhead.zijk  = map->fhead.zni * map->fhead.znj * gnk + extra ;   // add extra blocks
}

void fmap_print(zmap *map, char *msg){
  int code = fmap_invalid(map) ;
  if(code != 0) fprintf(stderr, "ERROR: %s (%d) invalid map\n", msg, code) ;
  if(map->fhead.signature != 0xBEBEFADA){
    fprintf(stderr, "fmap %s : INVALID signature, expecting 0xBEBEFADA, got %8.8x\n", msg, map->fhead.signature) ;
  }else{
    fprintf(stderr, "fmap %s : version %4.4x, reserved(%8.8x), data[%d:%d:%d], blocks[%d:%d:%d]+[%d], (%d,%d : %d,%d : %d), extra = %d\n",
            msg, map->fhead.version, map->fhead.reserved,
            map->fhead.gni, map->fhead.gnj, map->fhead.gnk,
            map->fhead.zni, map->fhead.znj, map->fhead.gnk, map->fhead.zijk - (map->fhead.zni * map->fhead.znj * map->fhead.gnk), 
            map->fhead.li0, map->fhead.lni, map->fhead.lj0, map->fhead.lnj, map->fhead.gnk, map->fhead.extra ) ;
  }
}

// address of file data map in zmap memory structure
void *filemap_address(zmap *map){
  return &(map->fhead.signature) ;
}

// size of file data map in 32 bit units
uint32_t filemap_words(zmap *map){
  return ( RANGE_ELEMENTS(map->mhead.frng) );
}

uint32_t filemap_blocks(int32_t gni, int32_t gnj, int32_t gnk, int32_t bsizex, int32_t bsizey){
  array_axis_3d r = split_axis_3d(gni, gnj, gnk, bsizex, bsizey) ;
// fprintf(stderr, "\n   x axis 1 x %d, %d x %d, y axis 1 x %d, %d x %d\n", r.x.ln0, r.x.nbk-1, r.x.ln1, r.y.ln0, r.y.nbk-1, r.y.ln1);
  return (r.x.nbk * r.y.nbk * r.z.nbk) ;
}

// needed size in bytes of file data map
size_t filemap_needed_size(int32_t gni, int32_t gnj, int32_t gnk, int32_t bsizex, int32_t bsizey, int32_t bextra){
  uint32_t blocks = filemap_blocks(gni, gnj, gnk, bsizex, bsizey) + bextra ;
  blocks = (blocks + 1) & 0xFFFFFFFE ;  // force number of blocks to even number >= number of blocks
  return ( blocks * sizeof(fmap_block_size) + sizeof(fmap) ) ;
}

// needed size in 32 bit units of file data map
size_t filemap_needed_words(int32_t gni, int32_t gnj, int32_t gnk, int32_t bsizex, int32_t bsizey, int32_t bextra){
  return filemap_needed_size(gni, gnj, gnk, bsizex, bsizey, bextra) / sizeof(uint32_t) ;
}

size_t zmap_needed_size(int32_t gni, int32_t gnj, int32_t gnk, int32_t bsizex, int32_t bsizey, int32_t bextra){
  return filemap_needed_size(gni, gnj, gnk, bsizex, bsizey, bextra) + sizeof(mmap) ;
}

// b_size [IN] : blocking size along first dimension (i)
// aspect [IN] : 2D aspect ratio (size along j = aspect * size along i) (1/2/3/4 supported, other values ignored)
// return a pair of adequate block sizes
static size_pair adjust_bsize(int32_t b_size, int32_t aspect){
  int32_t bi_size = b_size, bj_size ;

  if(bi_size <= 0) bi_size = 64 ;                                                          // default block size, 64 x 64
  bj_size = bi_size ;                                                                      // aspect ratio of 1 by default
  if(aspect == 2) { bi_size = bi_size * 3 / 4 ; bj_size = bi_size * 2 ; }                  // 48 x  96 if default size
  if(aspect == 3) { bi_size = bi_size * 5 / 8 ; bj_size = bi_size * 3 ; }                  // 40 x 120 if default size
  if(aspect == 4) { bi_size = bi_size / 2     ; bj_size = bi_size * 4 ; }                  // 32 x 128 if default size
  bi_size = (bi_size + 15) & 0xFFFFF0         ; bj_size = (bj_size + 15) & 0xEFFFFFF0 ;    // round to upper multiple of 16
  bi_size = (bi_size < 32) ? 32 : bi_size     ; bj_size = (bj_size < 32) ? 32 : bj_size ;  // block size at least 32 x 32

  return (size_pair) { bi_size, bj_size } ;
}

// TODO:
// separate zmap create from zmap populate ?
// function to calculate worst case data size from block_sizes[]
//
// create a data map with a worst case buffer for map, packed data, and all tables
// gni     [IN] : first dimension of array (row size)
// gnj     [IN] : second dimension of array (number of rows)
// gnk     [IN] : third dimension of array (number of planes)
// bi_size [IN] : blocking size along first dimension (i)
// aspect  [IN] : 2D aspect ratio (size along j = aspect * size along i) (1/2/3/4 supported, other values ignored)
// esize   [IN] : size in bytes of array elements (normally 1/2/4/8/16)
// mextra  [IN] : size of extra global information for data decoding (in bytes) (worst case)
// zextra  [IN] : number of extra blocks (usually 0)
// zsize   [IN] : size needed (in bytes) for extra blocks (0 if zextra == 0)
// return pointer to initialized zmap struct, NULL if error
//               mextra will be rounded to a multiple of sizeof(uint32_t) >= mextra
// NOTE: array dimensions are Fortran ordered (i index varying first)
// esize > 16 is not supported
// mextra may get updated later, provided that the new value is < value at zmap creation time
// in that case, frng and xrng would need to be updated
zmap *create_zmap(int32_t gni, int32_t gnj, int32_t gnk, int32_t bi_size, int32_t aspect, int32_t esize,
                  int32_t mextra, int32_t zextra, int32_t zsize){

  if(esize <= 0 || mextra < 0 || gni <= 0 || gnj <= 0 || gnk <= 0 || aspect < 0 || zextra < 0 || zsize < 0) return NULL ;
  if(esize > 16) return NULL ;          // element size too large
  if(zextra == 0) zsize = 0 ;           // ignored if no extra blocks

  mextra = ((mextra + 3) >> 2) << 2 ;   // round to multiple of sizeof(uint32_t) >= mextra
  zsize  = ((zsize  + 3) >> 2) << 2 ;   // round to multiple of sizeof(uint32_t) >= zsize

  size_pair bij = adjust_bsize(bi_size, aspect) ;                                      // find appropriate blocking sizes
  array_axis_3d a3 = split_axis_3d(gni, gnj, gnk, bij.i, bij.j) ;                      // perform decomposition into blocks
  uint32_t zni = a3.x.nbk ;            // number of blocks along i
  uint32_t znj = a3.y.nbk ;            // number of blocks along j
  uint32_t znk = a3.z.nbk ;            // number of blocks along k
  uint32_t zijk = zni * znj * znk + zextra ;
  zijk = ((zijk + 1) >> 1) << 1 ;      // round to upper multiple of 2 >= zijk

  zmap *map = NULL ;
  if(DEBUG){
    fprintf(stderr, "sizeof(mhead) = %ld, sizeof(fhead) = %ld, sizeof(zmap) = %ld\n", sizeof(map->mhead), sizeof(map->fhead), sizeof(zmap)) ;
    fprintf(stderr, "gni = %d, gnj = %d, bi_size = %d, zni = %d, znj = %d\n", gni, gnj, bij.i, zni, znj);
  }

  size_t size, bsize, hsize, dsize, msize, fsize ;
  fsize = sizeof(fmap) + sizeof(fmap_block_size) * zijk ;       // size of data map part that gets written into file
  size  = sizeof(zmap) + sizeof(fmap_block_size) * zijk ;       // base size of data map + table of sizes
  bsize = size ;                                                // size without data and without extra information
  size  = size + mextra ;                                       // + size of extra information
  hsize = size ;                                                // size without data but including extra information

  // worst case : gni * gnj * esize + zni * znj * (4 + 4)  (4 bytes round up + 4 bytes overhead per block)
  dsize = gni * gnj * esize + zni * znj * 8 ;                   // worst case size needed to encode data
  dsize = ((dsize + 3) >> 2) << 2 ;                             // bump to next multiple of 4 bytes
  size = size + dsize ;                                         // + size needed to encode data
  size = size + zsize ;                                         // + size needed for extra blocks
  msize = ( sizeof(uint32_t *) * (zijk + 1) ) ;                 // size for  offset[] pointer array
  size = size + msize ;                                         // + size needed for memory pointers

  map = (zmap *) malloc(size) ;     // allocate map with enough space for worst case data compression
  if(map == NULL) goto fail ;       // zmap allocation failed

  map->mhead = base_mmap ;                                      // initialize mmap portion
  // data map "read from file" / "written into file" address range
  SET_RANGE(map->mhead.frng, &(map->fhead.signature), fsize + mextra) ;
  // sizes table range
  SET_RANGE(map->mhead.srng, map->size, zijk * sizeof(fmap_block_size)) ;
  // extra range starts just after sizes table
  SET_RANGE(map->mhead.xrng, (uint8_t *)map + bsize, mextra) ;
  // encoded data address range starts just after extra
  SET_RANGE(map->mhead.drng, (uint8_t *)map + bsize + mextra, dsize + zsize) ;
  // memory pointers range
  map->mhead.offset = PTR_OFFSET(map, size - msize) ;          // address of memory pointers table [zijk+1]
  SET_RANGE(map->mhead.orng, map->mhead.offset, (zijk+1) * sizeof(void *)) ;
  // entire zmap struct address range
  SET_RANGE(map->mhead.zrng, map, size) ;
// TODO : what to do with bitstream ?

  for(uint32_t ui=0 ; ui<zijk ; ui++){                          // initialize memory pointers and sizes table
    map->mhead.offset[ui] = 0 ;                                    // no valid offset
    map->size[ui] = 0 ;                                         // size = 0
  }
  map->mhead.offset[0] = 0 ;                                       // start of data, after extra info
  fmap_init(map, gni, gnj, gnk, bij.i, bij.j, &a3, zextra) ;    // initialize fmap fixed portion, accounting for extra blocks

  if(DEBUG){
    zmap_print(map, "create") ;
    fmap_print(map, "create") ;
  }

//   uint8_t *data = (uint8_t *)map + bsize ;
//   uint8_t *extra = data - mextra ;
//   if(DEBUG)
//     fprintf(stderr, "allocated zmap at %p, [%ld bytes], size table[%d,%d,%d] at %p\n", map, size, zni, znj, znk, map->size) ;
//   if(DEBUG)
//     fprintf(stderr, "data offset = %ld bytes, hsize = %ld[base=%ld , sizes=%ld, extra=%d]\n",
//                 (uint8_t *)data - (uint8_t *)map, hsize, sizeof(zmap), sizeof(uint16_t)*zijk, mextra) ;
//   if(DEBUG){
//     fprintf(stderr, "map at %p", map) ;
//     fprintf(stderr, ", size table [%d + 1] at %p", zijk, &(map->size)) ;
//     fprintf(stderr, ", extra [%d] at %p", mextra, extra) ;
//     fprintf(stderr, ", data  at %p\n", data) ;
//   }
//   if(DEBUG){
//     fprintf(stderr, "offset[0] = %p,  at offset : %ld", map->mhead.offset[0], ADDRESS_DIFF(map->mhead.offset[0], map)) ;
//     fprintf(stderr, "\n") ;
//   }

  return map ;

fail :          // free what was allocated internally
  if(map){
    free(map) ;
  }
  return NULL ;
}

// adjust offset contents according to sizes
int fillmem_zmap(zmap *map){
  if(map == NULL) return 0 ;
  int ij, nij ;
  uint64_t t ;
  nij = map->fhead.zni * map->fhead.znj ;
  for(ij=0 ; ij<nij ; ij++){
    t = map->mhead.offset[ij] + map->size[ij] ;
//     if((uint8_t *)t >= map->mhead.limit) return 0 ;
    map->mhead.offset[ij+1] = t ;
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
uint64_t *mem_zmap(zmap *map, uint32_t *data, size_t size){
  int32_t zijk = map->fhead.zni * map->fhead.znj, i ;
  size_t needed = 0 ;

  if(data != NULL){    // check that enough space is available, set first/last/limit
    for(i=0 ; i<zijk ; i++){ needed += map->size[i] ; }
    if(size < needed) return NULL ;
    map->mhead.xrng.bot = map->mhead.xrng.top = data ;
//     map->mhead.first = data /*+ map->fhead.mextra*/ ;
//     map->mhead.last  = map->mhead.first ;
//     map->mhead.limit = (uint8_t *)data + size ;
//     if(DEBUG)
//       fprintf(stderr, "DEBUG mem_zmap, switching stream buffer to %16p -> %16p\n", (void *)map->mhead.first, (void *)map->mhead.limit) ;
  }
  // allocate offset table
  uint64_t *offset = (uint64_t *)malloc((zijk+1) * sizeof(uint64_t)) ;  // zijk + 1 entries needed
#if 0
  if(offset){          // allocation was successful
    if(map->mhead.offset) free(map->mhead.offset) ;  // free old table if there was one
    map->mhead.offset = offset ;
    offset[0] = map->mhead.first ;
    for(i=1 ; i<zijk+1 ; i++){
      offset[i] = offset[i-1] + map->size[i-1] ;   // recalculate offset[] using block sizes
    }
    if((uint8_t *)offset[zijk] > map->mhead.limit){   // OOPS, not enough space in data
      free(offset) ;
      offset = NULL ;
    }else{
      map->mhead.last = offset[zijk] ;                      // update last
    }
  }
#endif
  return offset ;
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
  // free offset if not inside zmap struct
//   if(map->mhead.offset){
//     if(DEBUG) fprintf(stderr, "freeing map->mhead.offset at %p\n", map->mhead.offset) ;
//     free(map->mhead.offset) ;
//   }
  // free data  if not inside zmap struct
  map->mhead.offset = NULL ;
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
#if 0
ssize_t resize_map(zmap *map){
  int k ;
  uint32_t *current ;

  current = map->mhead.offset[0] ;          // initial position
  if(current != map->mhead.first){
    int32_t *data = (int32_t *)&(map->size[map->fhead.zni*map->fhead.znj]) ;
    fprintf(stderr, "ERROR resize_map : first map entry not pointing to start of stream\n") ;
    fprintf(stderr, "      first = %16p, start = %16p, data = %16p\n", (void *)map->mhead.first, (void *)current, (void *)data) ;
  }
  for(k=0 ; k < map->fhead.zni * map->fhead.znj ; k++){
    map->mhead.offset[k] = current ;
    current += map->size[k] ;
    if(current > map->mhead.last){
      fprintf(stderr, "ERROR: cannot resize map, not enough space occording to size table\n") ;
      return -1 ;
    }
  }
  return current - map->mhead.offset[0] ;
}
#endif
#if 0
// remove holes from data buffer, update list of memory addresses using updated sizes
ssize_t repack_map(zmap *map){
  int k ;
  uint32_t *current, *stream ;

  if(map == NULL)      return -1 ;
  if(map->mhead.offset == NULL) return -1 ;

  current = map->mhead.offset[0] ;          // initial target position
  if(current != map->mhead.first){
    int32_t *data = (int32_t *)&(map->size[map->fhead.zni*map->fhead.znj]) ;
    fprintf(stderr, "ERROR resize_map : first map entry not pointing to start of stream\n") ;
    fprintf(stderr, "current = %p, first = %16p, start = %16p, data = %16p\n", (void *)current, (void *)map->mhead.first, (void *)current, (void *)data) ;
  }
  for(k=0 ; k < map->fhead.zni * map->fhead.znj ; k++){
    stream = map->mhead.offset[k] ;         // copy from this position in memory
    map->mhead.offset[k] = current ;        // update offset pointer to new position in memory
    if(current < stream || map->size[k] != map->mhead.offset[k+1] - map->mhead.offset[k]) {  // need to copy ?
      if(DEBUG) fprintf(stderr, "copying from %6ld", current - map->mhead.offset[0]) ;
      memmove(current, stream, map->size[k] * sizeof(uint32_t)) ;    // PGI/Nvidia compile problems with DEBUG =0 and copy loop
//       int i ;
//       for(i=0 ; i < map->size[k] ; i++){ current[i] = stream[i] ; }
      if(DEBUG) fprintf(stderr, " to %6ld [%6d]\n", current + map->size[k] - map->mhead.offset[0] -1, map->size[k]) ;
    }
    current += map->size[k] ;      // update target position
  }
  map->mhead.offset[k] = current ;
  return current - map->mhead.offset[0] ;
}
#endif
