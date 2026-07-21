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

// block position from grid index, using data map
// map    [IN] : data map
// i      [IN] : i (column) position in 2D grid
// j      [IN] : j (row) position in 2D grid
// return [i,j] block coordinates (different from z index)
index_pair map_block_position(zmap *map, int32_t i, int32_t j){
  index_pair ij = {.i = -1, .j = -1 } ;  // precondition for failure
  if(map->fhead.gni > i && map->fhead.gnj > j){
    ij.i = block_index(i, map->fhead.lni, map->fhead.li0) ;
    ij.j = block_index(j, map->fhead.lnj, map->fhead.lj0) ;
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

  if(bi < map->fhead.zni && bj < map->fhead.znj && bi >= 0 && bj >= 0){      // within block map limits ?
    index_range r ;
    r = index_limits(bi, map->fhead.lni, map->fhead.li0) ;                   // get block limits along first dimension (row)
    ij.i0 = r.ix0 ; ij.in = r.ixn ;
    r = index_limits(bj, map->fhead.lnj, map->fhead.lj0) ;                   // get block limits along second dimension (column)
    ij.j0 = r.ix0 ; ij.jn = r.ixn ;
  }
  return ij ;
}

// allocate a new zmap struct in memory, using data map size (meta[1]) and record size from file directory)
// map0   [INOUT] : pointer to valid zmap struct (may be NULL) (reusable map)
// map_words [IN] : size in 32 bit units of data map from rsf file ( 0 if no data map in record )
// rec_words [IN] : size in 32 bit units of data record from rsf file (0 if no data component to allocate)
// return address of zmap (space for data stream is optional) or NULL if error
//
// zmap will be nullified except for mmap signature, version, and the address ranges
// data record size includes data map size
// a record without a data map will have map_words == 0
// rec_words < map_words is the same as rec_words == 0
// in intending to only read the data map, set rec_words to 0
// if rec_words > map_words, map_words is ignored and rec_words is used
// rec_words will be zero if data is expected to be read by block(s) at a later time
zmap *create_file_zmap(zmap *map0, uint32_t map_words, uint32_t rec_words){
  size_t mapsize = map_words * sizeof(uint32_t) ;           // data map only
  size_t recsize = rec_words * sizeof(uint32_t) ;           // data map and data stream
  recsize = (recsize < mapsize) ? mapsize : recsize ;
  if(recsize <= 0) return NULL ;                            // zero size record and no map

  size_t frecsize = recsize ;                               // size without offsets table
  uint32_t zijkmax = map_words * 2 ;                        // worst case for number of blocks
  zijkmax = ((zijkmax + 1) >> 1) << 1 ;                     // round up to multiple of 2
  zijkmax = zijkmax - (sizeof(fmap) / 2) ;                  // base size of data map in 16 bit units
  recsize = frecsize + (zijkmax + 1) * sizeof(uint64_t) ;   // add size of estimated offsets table
  recsize = recsize + sizeof(mmap) ;                        // add mmap struct size
  recsize = ((recsize + 7) >> 3) << 3 ;                     // round up to multiple of 8 (offsets at top must be 64 bit aligned)

  if(DEBUG){
    fprintf(stderr, "create_file_zmap : map_words = %d, rec_words = %d, zmap size = %ld, est max blocks = %d, frecsize = %ld words\n",
                    map_words, rec_words, recsize/sizeof(uint32_t), zijkmax - 1, frecsize/sizeof(uint32_t)) ;
  }
  zmap *map = NULL ;
  if(map0){                                                     // potentially reusable map
    ssize_t needed = recsize ;
    ssize_t avail = ZMAP_BYTES(map0) ;
    if(needed <= avail){                                        // map0 is large enough, use it
      map = map0 ;
      recsize = avail ;                                         // set size to reusable map size
      if(DEBUG) fprintf(stderr, "need %ld bytes, REUSING map0 of size %ld at adddress %p\n", needed, avail, map0);
    }else{
      if(DEBUG) fprintf(stderr, "need %ld bytes, CANNOT reuse map0 of size %ld at adddress %p\n", needed, avail, map0);
    }
  }
  if(map == NULL) map = (zmap *) malloc(recsize) ;          // try to allocate memory for map if necessary

  if(map != NULL){
    map->mhead = base_mmap ;                                // set signature and version
    // entire zmap struct (with or without space for data stream)
    SET_BYTE_RANGE(map->mhead.zrng, map, recsize) ;              // sizeof(mmap) + file record size + offsets table

    // fmap : base size + sizes table + mextra == data map size from record metadata
    SET_BYTE_RANGE(map->mhead.frng, &(map->fhead.signature), map_words * sizeof(uint32_t)) ;

    // "extra region" size and bottom address will be known once the data map has been read from file
    // top is at top of fmap
    map->mhead.xrng.top = map->mhead.xrng.bot = map->mhead.frng.top ;

    // "data region" not including "extra"
    if(frecsize == mapsize){
    if(DEBUG) fprintf(stderr, "map only, no data\n");
      map->mhead.drng.bot =  map->mhead.drng.top = NULL ;        // map only, no data
    }else{
    if(DEBUG) fprintf(stderr, "map and data\n");
      map->mhead.drng.bot = map->mhead.frng.top ;                // address of data is just above data map
      SET_RANGE_BYTES(map->mhead.drng, frecsize - mapsize) ;      // file record size = map size + data size
    }

    // sizes[] table (bottom address known , size unknown yet)
    SET_BYTE_RANGE(map->mhead.srng, &(map->size), 0) ;                // set size to 0 for now

    // worst case estimate of number of blocks : zijkmax, the correct value will be known after the data map is read
    map->mhead.orng.top = (void *)(map->mhead.zrng.top) ;        // offset table top at top of zmap
    map->mhead.orng.bot = map->mhead.orng.top - zijkmax ;        // there is room for up to zijkmax entries

    map->fhead = null_fmap ;                                     // fmap part zeroed out, ready to be read from file
  }
  return map ;
}

#undef FAIL
#define FAIL(ERR,MSG) { status = ERR ; fprintf(stderr, "ERROR (%d) : %s\n", status, MSG); goto fail ; }

// update contents of mmap after fmap part has been read from file
// map   [INOUT] : pointer to valid zmap struct
// value of mextra and number of blocks (zijk) are now known
// return 0 if O.K., error code otherwise
// some consistency checks are performed
int update_file_zmap(zmap *map){
  int status = fmap_invalid(map) ;
  if(status != 0) return status ;      // are the fmap struct contents valid ?
//   void *offset = NULL ;

  // "extra" region size and bottom position can now be determined, top of "extra" should be at top of fmap
  if(PTR(map->mhead.xrng.top) != PTR(map->mhead.frng.top)) FAIL(1, "xrng.top != frng.top") ;
  map->mhead.xrng.bot = map->mhead.frng.top - map->fhead.extra ;    // extra is in 32 bit units, top and bot are pointers to 32 bit integers

  // check bottom address of data if there is data (data bottom pointer not NULL), data should be just above "extra"
  if(map->mhead.drng.bot != NULL){                                  // there is data
    if(PTR(map->mhead.drng.bot) != PTR(map->mhead.xrng.top)) FAIL(2, "drng.bot != xrng.top") ;
  }
  // update sizes table range
  SET_BYTE_RANGE(map->mhead.srng, &(map->size), sizeof(fmap_block_size) * map->fhead.zijk) ;
  // update offsets/offset range orng
  if(PTR(map->mhead.zrng.top) != PTR(map->mhead.orng.top)){         // offset table top should be at top of zmap
    FAIL(3, "zrng.top != orng.top");
  }
  map->mhead.orng.top = (void *)(map->mhead.zrng.top) ;
  int32_t zijkmax = 1 + map->fhead.zijk ;
  map->mhead.orng.bot = map->mhead.orng.top - zijkmax ;             // there is room for up to zijkmax entries

  map->mhead.orng.bot[0] = 0 ;                                      // adjust for offsets in bytes
  for(int i = 1 ; i < zijkmax ; i++){                               // fill offsets table using sizes table
    map->mhead.orng.bot[i] = map->mhead.orng.bot[i-1] + (map->size[i-1] * sizeof(uint32_t)) ;
  }
  if(DEBUG) fprintf(stderr, "update_file_zmap : actual number of blocks = %d\n", map->fhead.zijk) ;
  return status ;

fail:
  return status ;
}

void zmap_print(zmap *map, char *msg){
  mmap_print(map, msg) ;
  fmap_print(map, msg) ;
}

// shorthand macros
#undef VRM
#define VRM(RANGE) VALID_RANGE(map->mhead.RANGE) ? "valid" : "invalid"
#undef VRR
#define VRR(RANGE) VALID_RANGE(RANGE) ? "valid" : "invalid"
// print contents of memory header from zmap
// map  [IN] : pointer to valid zmap struct
// msg  [IN] : map name (cosmetic)
void mmap_print(zmap *map, char *msg){
  if(map->mhead.signature != 0x1AD0FADA){
    fprintf(stderr, "mmap %s : INVALID signature, expected 0x1AD0FADA, got %8.8x\n", msg, map->mhead.signature) ;
//     return ;
  }
  char *f1 = "   %s   %16p -> %16p [+%10d] (%10ld %s) %s\n" ;
  char *f2 = "   %s   %16p -> %16p               (%10ld %s) %s\n" ;
  size_t fmap_size = RANGE_BYTES(map->mhead.frng) ;
  size_t smap_size = RANGE_BYTES(map->mhead.srng) ;
  size_t xmap_size = RANGE_BYTES(map->mhead.xrng) ;
  size_t dmap_size = RANGE_BYTES(map->mhead.drng) ;
  size_t omap_size = RANGE_BYTES(map->mhead.orng) ;
  size_t pmap_size = RANGE_BYTES(map->mhead.prng) ;
  size_t zmap_size = RANGE_BYTES(map->mhead.zrng) ;
  RANGE(word) hrng ;

  if(PTR(map->mhead.drng.top)){
    hrng.bot = map->mhead.drng.top ; hrng.top = PTR_VOID(map->mhead.orng.bot) ;  // hole between top of data and bottom of offsets/pointers
  }else{
    hrng.bot = map->mhead.frng.top ; hrng.top = PTR_VOID(map->mhead.orng.bot) ;  // hole between top of file data map and bottom of offsets/pointers
  }
//   fprintf(stderr, "zmap %s : version %4.4x, '%4.4x', options = %8.8x, bit stream at %p \n",
//           msg, map->mhead.version, map->mhead.signature, map->mhead.options, &(map->mhead.stream)) ;
  fprintf(stderr, "zmap %s : version %4.4x, '%8.8x', options = %8.8x, no bitstream in map\n",
          msg, map->mhead.version, map->mhead.signature, map->mhead.options) ;
  fprintf(stderr, "   COPY   %16p\n   CODEC  %16p\n", map->mhead.get_blocks, map->mhead.codec) ;
  fprintf(stderr, "   segment        start               limit       offset        size\n") ;
  fprintf(stderr, f1, "ZMAP", map->mhead.zrng.bot, RANGE_LIMIT(map->mhead.zrng), PTR_OFFSET(map,map->mhead.zrng.bot), zmap_size, "bytes", VRM(zrng)) ;
  fprintf(stderr, f1, "FMAP", map->mhead.frng.bot, RANGE_LIMIT(map->mhead.frng), PTR_OFFSET(map,map->mhead.frng.bot), fmap_size, "bytes", VRM(frng)) ;
  fprintf(stderr, f1, "Sblk", map->mhead.srng.bot, RANGE_LIMIT(map->mhead.srng), PTR_OFFSET(map,map->mhead.srng.bot), smap_size, "bytes", VRM(srng)) ;
  if(map->mhead.xrng.bot)
    fprintf(stderr, f1, "Xtra", map->mhead.xrng.bot, RANGE_LIMIT(map->mhead.xrng), PTR_OFFSET(map,map->mhead.xrng.bot), xmap_size, "bytes", VRM(xrng)) ;
  else
    fprintf(stderr, f2, "Xtra", map->mhead.xrng.bot, RANGE_LIMIT(map->mhead.xrng), xmap_size, "bytes", VRM(xrng)) ;
  dmap_size /= sizeof(uint32_t) ;
  if(map->mhead.drng.bot)
    fprintf(stderr, f1, "Data", map->mhead.drng.bot, RANGE_LIMIT(map->mhead.drng), PTR_OFFSET(map,map->mhead.drng.bot), dmap_size, "words", VRM(drng)) ;
  else
    fprintf(stderr, f2, "Data", map->mhead.drng.bot, map->mhead.drng.top, dmap_size, "words", VRM(drng)) ;
  size_t hole_size = RANGE_ELEMENTS(hrng) ;
  if(hole_size != 0)
    fprintf(stderr, f1, "Void", hrng.bot, RANGE_LIMIT(hrng), PTR_OFFSET(map,hrng.bot), hole_size, "words", VRR(hrng)) ;
  if(map->mhead.orng.bot)
    if((uint8_t *)(map->mhead.orng.bot) > (uint8_t *)(map->mhead.zrng.bot) && (uint8_t *)(map->mhead.orng.bot) < (uint8_t *)(map->mhead.zrng.top))
      fprintf(stderr, f1, "Oblk", map->mhead.orng.bot, RANGE_LIMIT(map->mhead.orng), PTR_OFFSET(map,map->mhead.orng.bot), omap_size, "bytes", VRM(orng)) ;
    else
      fprintf(stderr, f2, "Oblk", map->mhead.orng.bot, RANGE_LIMIT(map->mhead.orng), omap_size, "bytes", VRM(orng)) ;
  else
    fprintf(stderr, f2, "Oblk", map->mhead.orng.bot, RANGE_LIMIT(map->mhead.orng), omap_size, "bytes", VRM(orng)) ;
  if(map->mhead.prng.bot)
    if((uint8_t *)(map->mhead.prng.bot) > (uint8_t *)(map->mhead.zrng.bot) && (uint8_t *)(map->mhead.prng.bot) < (uint8_t *)(map->mhead.zrng.top))
      fprintf(stderr, f1, "Pblk", map->mhead.prng.bot, RANGE_LIMIT(map->mhead.prng), PTR_OFFSET(map,map->mhead.prng.bot), pmap_size, "bytes", VRM(prng)) ;
    else
      fprintf(stderr, f2, "Pblk", map->mhead.prng.bot, RANGE_LIMIT(map->mhead.prng), pmap_size, "bytes", VRM(prng)) ;
  else
    fprintf(stderr, f2, "Pblk", map->mhead.prng.bot, RANGE_LIMIT(map->mhead.prng), pmap_size, "bytes", VRM(prng)) ;
}
#undef VRM
#undef VRR

// print blocks size/offset/limit
// map       [IN] : pointer to valid zmap struct
// maxblocks [IN] : only print up to block maxblocks-1
// return number of "out of range" blocks (not fully within map data range)
// adjusted for offsets in bytes
int print_zmap_blocks(zmap *map, uint32_t maxblocks){
  int oor = 0 ;  // number of "out of range" blocks
  int32_t hole = 0 ;
  uint32_t total_blocks = ZMAP_TOTAL_BLOCKS(map) ;
  uint32_t array_blocks = ZMAP_ARRAY_BLOCKS(map) ;
  if(maxblocks > total_blocks) maxblocks = total_blocks ;
  uint32_t modulo = (total_blocks+maxblocks-1) / maxblocks ;

  fprintf(stderr, "=============================================================== zmap blocks ===============================================================\n") ;
// return 0 ;
  if(map->mhead.drng.bot){
    fprintf(stderr, "          size       drng.bot           start       offset   to hole          limit                                         hole   from bot\n") ;
    for(uint32_t i=0 ; i<total_blocks ; i++){
      uint32_t *bk0 = map->mhead.drng.bot + (BLOCK_OFFSET(map,i)/sizeof(uint32_t)) ;
      uint32_t *bkn = bk0 + map->size[i] - 1 ;
      if(map->size[i] == 0) { bk0-- ; bkn = bk0 ; }    // 0 size block at top remains "in range"
      hole = (map->mhead.orng.bot[i+1]) - (map->mhead.orng.bot[i]) - (map->size[i] * sizeof(uint32_t)) ;

      if( ((i % modulo) == 0) || (i == (total_blocks - 1) || (i >= array_blocks)) ){
        fprintf(stderr, "block %4d(%5d), %p <= %p[%8ld][%8ld] <= %p ? (%s:%s), %s [%6d] <%6ld>\n",
                i, map->size[i], RANGE_BOT(map->mhead.drng), bk0,
                RANGE_OFFSET(map->mhead.drng, bk0), RANGE_AVAIL(map->mhead.drng, bk0),
                /*RANGE_LIMIT(map->mhead.drng)*/ bkn,
                IN_RANGE(map->mhead.drng, bk0, bk0) ? "IN RANGE" : "OUT OF RANGE",
                IN_RANGE(map->mhead.drng, bkn, bkn) ? "IN RANGE" : "OUT OF RANGE",
                (hole == 0) ? "contiguous" : "  disjoint", hole,
                contiguous_zmap_blocks(map, 0, i)*sizeof(uint32_t)  ) ;
      }

      if( ! IN_RANGE(map->mhead.drng,bk0,bkn) ) oor++ ;
    }
    fprintf(stderr, "%d block(s) not inside data range\n", oor) ;
  }else{
    fprintf(stderr, "          size     offset                hole  from bot\n") ;
    for(uint32_t i=0 ; i<total_blocks ; i++){
      if( ((i % modulo) == 0) || (i == (total_blocks - 1)) ){
        hole = (map->mhead.orng.bot[i+1]) - (map->mhead.orng.bot[i]) - (map->size[i] * sizeof(uint32_t)) ;
        fprintf(stderr, "block %4d(%5d) [%8ld] %s [%6d] <%6ld>\n",
                i, map->size[i], map->mhead.orng.bot[i], (hole == 0) ? "contiguous" : "  disjoint", hole, contiguous_zmap_blocks(map, 0, i)*sizeof(uint32_t)) ;
      }
    }
  }
  return oor ;
}

// are all blocks within data range
// map       [IN] : pointer to valid zmap struct
// return number of out of range encoded data blocks
int zmap_blocks_out_of_range(zmap *map){
  uint32_t total_blocks = ZMAP_TOTAL_BLOCKS(map) ;
  int oor = 0 ;                                  // number of "out of range" blocks
  if(map->mhead.drng.bot == NULL) return -1 ;    // no valid data range
  for(uint32_t i=0 ; i<total_blocks ; i++){
    uint32_t *bk0 = map->mhead.drng.bot + (BLOCK_OFFSET(map,i)/sizeof(uint32_t)) ;
    uint32_t *bkn = bk0 + map->size[i] - 1 ;
    if( ! IN_RANGE(map->mhead.drng,bk0,bkn) ) oor++ ;
  }
  return oor ;
}

// are these blocks in ascending memory order ?
// return cumulative space if blocks are in ascending memory order, -size if not
int ordered_zmap_blocks(zmap *map, int block0, int block_n){
  (void) (map) ;
  (void) (block0) ;
  (void) (block_n) ;
  return 0 ;
}

// map    [IN] : pointer to valid zmap struct
// block0 [IN] : first block to check
// blockn [IN] : last block to check
// return cumulative size in words if blocks are contiguous, -size if not
int contiguous_zmap_blocks(zmap *map, int block0, int blockn){
  // NOTE: the orng range is used to check if blocks are contiguous
  //       if prng rather than orng is valid, it does not affect the contiguity check
  //       addresses are 64 bit wide pointers to 32 bit items and can be used as offsets
  // TODO : if blockn >= number of blocks, set to maxblock ?
  // TODO : check if we have offsets or pointers (orng vs prng) and act accordingly
  uint64_t *offsets = map->mhead.orng.bot ;              // offsets table
  uint32_t size = 0 ;
  int contiguous = 1 ;
  size = map->size[block0] ;
  for(int i = block0 + 1 ; i <= blockn ; i++){
    size = size + map->size[i] ;                  // cumulative size of blocks to copy (in 32 bit units)
    contiguous = contiguous && ( offsets[i-1] + (map->size[i-1]*sizeof(uint32_t)) == offsets[i] ) ;
  }
  return contiguous ? size : (-size) ;
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
// bsizei  [IN] : blocking size along the first dimension (i)
// bsizej  [IN] : blocking size along the second dimension (i)
// a3      [IN] : optional pointer to array axis struct, if NULL, split_axis_3d will be called
// mextra  [IN] : length of extra metadata (in 32 bit units)
// bextra  [IN] : extra number of blocks to add to decomposition
// bsizei and bsizej are used only when a3 == NULL
int fmap_init(zmap *map, int32_t gni, int32_t gnj, int32_t gnk, int32_t bsizei, int32_t bsizej, array_axis_3d *a3, int mextra, int bextra){
  array_axis_3d r ;
  int errors = 0 ;
  if(a3 == NULL){
    r = split_axis_3d(gni, gnj, gnk, bsizei, bsizej) ;    // 3D decomposition not supplied, compute it
    a3 = &r ;
  }
  if(bextra < 0) bextra = 0 ;
  if(mextra < 0) mextra = 0 ;
  map->fhead = base_fmap ;                // set signature and version, nullify the rest
  map->fhead.esize = map->mhead.esize ;   // get esize from memory header
  if(map->fhead.esize > 16) errors++ ;    // make sure esize is legit
  map->fhead.gni   = gni ;
  map->fhead.gnj   = gnj ;
  map->fhead.gnk   = gnk ;
  map->fhead.zni   = a3->x.nbk ;
  map->fhead.li0   = a3->x.ln0 ;
  map->fhead.lni   = a3->x.ln1 ;
  map->fhead.znj   = a3->y.nbk ;
  map->fhead.lj0   = a3->y.ln0 ;
  map->fhead.lnj   = a3->y.ln1 ;
  map->fhead.zijk  = map->fhead.zni * map->fhead.znj * gnk + bextra ;   // add bextra blocks
  map->fhead.extra = mextra ;
  if(mextra < 0 || bextra < 0) errors++ ;
  if(fmap_invalid(map)) errors++ ;
  if((DEBUG != 0) && (errors > 0)) exit(1) ;
  return errors ;
}

// print description of the fixed part of the fmap component of a data map
// map [IN] : pointer to zmap (mmap/fmap/sizes/...) struct
// msg [IN] : extra text for message
void fmap_print(zmap *map, char *msg){
  int code = fmap_invalid(map) ;
  if(code != 0) fprintf(stderr, "ERROR: %s (%d) invalid map\n", msg, code) ;
  if(map->fhead.signature != 0xBEBEFADA){
    fprintf(stderr, "fmap %s : INVALID signature, expecting 0xBEBEFADA, got %8.8x\n", msg, map->fhead.signature) ;
  }else{
    fprintf(stderr, "fmap %s : version %4.4x, reserved(%8.8x), data[%d:%d:%d], %d blocks[%d:%d:%d]+[%d], (%d,%d : %d,%d : %d), extra metadata = %d words, esize = %d bytes\n",
            msg, map->fhead.version, map->fhead.reserved,
            map->fhead.gni, map->fhead.gnj, map->fhead.gnk,
            map->fhead.zijk, map->fhead.zni, map->fhead.znj, map->fhead.gnk, map->fhead.zijk - (map->fhead.zni * map->fhead.znj * map->fhead.gnk), 
            map->fhead.li0, map->fhead.lni, map->fhead.lj0, map->fhead.lnj, map->fhead.gnk, map->fhead.extra, map->fhead.esize ) ;
  }
}

// address of file data map in zmap memory structure
// map [IN] : pointer to zmap (mmap/fmap/sizes/...) struct
// return a pointer to the fmap component of a data map
void *filemap_address(zmap *map){
  return &(map->fhead.signature) ;
}

// size of file data map in 32 bit units
// map [IN] : pointer to zmap (mmap/fmap/sizes/...) struct
// return the size of the data map in the file (in 32 bit words)
uint32_t filemap_words(zmap *map){
  return ( RANGE_ELEMENTS(map->mhead.frng) );
}

// compute the number of data blocks in the data map
// gni    [IN] : first dimension of array
// gnj    [IN] : second dimension of array
// gnk    [IN] : third dimension of array
// bsizei [IN] : blocking size along the first dimension
// bsizej [IN] : blocking size along the second dimension
uint32_t filemap_blocks(int32_t gni, int32_t gnj, int32_t gnk, int32_t bsizei, int32_t bsizej){
  array_axis_3d r = split_axis_3d(gni, gnj, gnk, bsizei, bsizej) ;
// fprintf(stderr, "\n   x axis 1 x %d, %d x %d, y axis 1 x %d, %d x %d\n", r.x.ln0, r.x.nbk-1, r.x.ln1, r.y.ln0, r.y.nbk-1, r.y.ln1);
  return (r.x.nbk * r.y.nbk * r.z.nbk) ;
}

// needed size in bytes for the file component of the data map
// gni    [IN] : first dimension of array
// gnj    [IN] : second dimension of array
// gnk    [IN] : third dimension of array
// bsizei [IN] : blocking size along the first dimension
// bsizej [IN] : blocking size along the second dimension
// bextra [IN] : number of "extra" blocks (usually 0)
size_t filemap_needed_bytes(int32_t gni, int32_t gnj, int32_t gnk, int32_t bsizei, int32_t bsizej, int32_t bextra){
  uint32_t blocks = filemap_blocks(gni, gnj, gnk, bsizei, bsizej) + bextra ;
  blocks = (blocks + 1) & 0xFFFFFFFE ;  // force number of blocks to even number >= number of blocks
  return ( blocks * sizeof(fmap_block_size) + sizeof(fmap) ) ;
}

// needed size in 32 bit units to accomodate the file component of the data map
// see filemap_needed_bytes for argument description
size_t filemap_needed_words(int32_t gni, int32_t gnj, int32_t gnk, int32_t bsizei, int32_t bsizej, int32_t bextra){
  return filemap_needed_bytes(gni, gnj, gnk, bsizei, bsizej, bextra) / sizeof(uint32_t) ;
}

// needed size in bytes for the entire zmap struct
// see filemap_needed_bytes for argument description
size_t zmap_needed_bytes(int32_t gni, int32_t gnj, int32_t gnk, int32_t bsizei, int32_t bsizej, int32_t bextra){
  return filemap_needed_bytes(gni, gnj, gnk, bsizei, bsizej, bextra) + sizeof(mmap) ;   // add size of mmap component
}

// b_size [IN] : blocking size along first dimension (i)
// aspect [IN] : 2D aspect ratio (size along j = aspect * size along i) (1/2/3/4 supported, other values same as 1)
// return a pair of adequate blocking sizes
static size_pair adjust_bsize(int32_t b_size, int32_t aspect){
  int32_t bi_size, bj_size ;

  bi_size = (b_size <= 0) ? 64 : b_size ;                                                  // default block size, 64 x 64
  bj_size = bi_size ;                                                                      // aspect ratio is 1 by default
  if(aspect == 2) { bi_size = bi_size * 3 / 4 ; bj_size = bi_size * 2 ; }                  // 48 x  96 if default size
  if(aspect == 3) { bi_size = bi_size * 5 / 8 ; bj_size = bi_size * 3 ; }                  // 40 x 120 if default size
  if(aspect == 4) { bi_size = bi_size / 2     ; bj_size = bi_size * 4 ; }                  // 32 x 128 if default size
  bi_size = (bi_size + 15) & 0xEFFFFFF0       ; bj_size = (bj_size + 15) & 0xEFFFFFF0 ;    // round to >= multiple of 16
  bi_size = (bi_size < 32) ? 32 : bi_size     ; bj_size = (bj_size < 32) ? 32 : bj_size ;  // block size at least 32 x 32

  return (size_pair) { bi_size, bj_size } ;
}

// create a data map large enough to accomodate map, buffer for worst case packed data, and all tables
// map0 [INOUT] : pointer to valid zmap struct (may be NULL)
// gni     [IN] : first dimension of array (row size)
// gnj     [IN] : second dimension of array (number of rows)
// gnk     [IN] : third dimension of array (number of planes)
// bi_size [IN] : blocking size along first dimension (i)
// aspect  [IN] : 2D aspect ratio (size along j = aspect * size along i) (1/2/3/4 supported, other values ignored)
// esize   [IN] : size in bytes of array elements (supported : 1/2/4/8/16)
// mextra  [IN] : max size of extra global information for data decoding (in 32 bit units)
// zextra  [IN] : number of extra blocks (usually 0)
// zsize   [IN] : size needed (in bytes) for extra blocks (0 if zextra == 0)
// d_bytes [IN] : space to allocate for data (in bytes)
//                -1 : no space allocation for data, 0 : automatic allocation
// return pointer to partially initialized zmap struct, NULL if error
// NOTE: array dimensions are Fortran ordered (i index varying first)
// esize > 16 is not supported for now
// mextra may get updated later, provided that the new value is < value at zmap creation time
// in that case, frng, xrng, drng will need to be updated too
// if map0 is supplied and large enough, it will be reused
zmap *create_zmap(zmap *map0, int32_t gni, int32_t gnj, int32_t gnk, int32_t bi_size, int32_t aspect, int32_t esize,
                  int32_t mextra, int32_t zextra, int32_t zsize, ssize_t d_bytes){
  int status = 0 ;

  if(zextra == 0) zsize = 0 ;           // ignored (forced to zero) if there are no extra blocks
  if(esize <= 0 || mextra < 0 || gni <= 0 || gnj <= 0 || gnk <= 0 || aspect < 0 || zextra < 0 || zsize < 0) return NULL ;
  if(esize > 16) return NULL ;          // element size not supported (too large)

  zsize  = ((zsize  + 3) >> 2) << 2 ;   // round to multiple of sizeof(uint32_t) >= zsize

  size_pair bij = adjust_bsize(bi_size, aspect) ;                                      // find appropriate blocking sizes
  array_axis_3d a3 = split_axis_3d(gni, gnj, gnk, bij.i, bij.j) ;                      // perform decomposition into blocks
  uint32_t zni = a3.x.nbk ;            // number of blocks along i
  uint32_t znj = a3.y.nbk ;            // number of blocks along j
  // TODO : pencil mode with znk == 1 ?
  uint32_t znk = a3.z.nbk ;            // number of blocks along k
  uint32_t zijk = zni * znj * znk + zextra, zijk1 ;
  zijk1 = ((zijk + 1) >> 1) << 1 ;      // round number of blocks to upper multiple of 2 >= zijk

  zmap *map = NULL ;
  if(DEBUG){
    fprintf(stderr, "sizeof(mhead) = %ld, sizeof(fhead) = %ld, sizeof(zmap) = %ld\n", sizeof(map->mhead), sizeof(map->fhead), sizeof(zmap)) ;
    fprintf(stderr, "gni = %d, gnj = %d, gnk = %d, b_size = (%d,%d), zni = %d, znj = %d, zijk1 = %d\n", gni, gnj, gnk, bij.i, bij.j, zni, znj, zijk1);
  }

  size_t size, dsize, osize, fsize ;
  fsize = sizeof(fmap) + sizeof(fmap_block_size) * zijk1 ;      // size of data map section that will get written into file
  size  = sizeof(zmap) + sizeof(fmap_block_size) * zijk1 ;      // base size of data map + table of sizes
  size  = size + mextra * sizeof(uint32_t) ;                    // size += size of extra information

  // worst case : esize * nb_of_values + number_of_blocks * (4 + 4)  (up to 4 bytes round up + 4 bytes overhead per block)
  dsize = gni * gnj * gnk * esize + zni * znj * znk * 8 ;       // estimate of worst case size needed to encode data
  dsize = (d_bytes > 0) ? d_bytes : dsize ;                     // d_bytes > 0, force allocation for data blocks to d_bytes
  dsize = ((dsize + 3) >> 2) << 2 ;                             // bump to next multiple of 4 bytes
  dsize = (d_bytes == -1) ? 0 : dsize ;                         // if d_bytes == -1 , set dsize to 0 (no space allocated for data block)
  size = size + dsize ;                                         // size += size needed to encode data (worst case estimate)

  zsize = (d_bytes == -1) ? 0 : zsize ;                         // if d_bytes == -1 , set zsize to 0 (no space allocated for extra blocks)
  size = size + zsize ;                                         // size += size needed for extra blocks

  osize = ( sizeof(uint64_t) * (zijk + 1) ) ;                   // size of  offset[]/pointer[] table (pointer size assumed to be 64 bits or less)
  size = size + osize ;                                         // size += size needed for offsets
  size = ((size + 7) >> 3) << 3 ;                               // bump to multiple of 8 >= size

  if(map0){                                                     // potentially reusable map
    ssize_t needed = size ;
    ssize_t avail = ZMAP_BYTES(map0) ;
    if(needed <= avail){                                        // map0 is large enough, use it
      map = map0 ;
      size = avail ;                                            // set size to reusable map size
      fprintf(stderr, "need %ld bytes, REUSING map0 of size %ld at adddress %p\n", needed, size, map0);
    }else{
      fprintf(stderr, "need %ld bytes, CANNOT reuse map0 of size %ld at adddress %p\n", needed, size, map0);
    }
  }
  if(map == NULL){                                              // not reusing map0
    map = (zmap *) malloc(size) ;                               // allocate map with enough room for worst case data encoding
    if(map == NULL) FAIL(1, "map allocation failed\n") ;        // zmap allocation failed
  }

  map->mhead = base_mmap ;                                      // initialize signature, version, options, stream, fn, args
  map->mhead.esize = esize ;                                    // temporarily store esize in mhead, will be moved into fhead later
  // initialize memory address ranges
  // entire zmap struct address range
  SET_BYTE_RANGE(map->mhead.zrng, map, size) ;
  // data map "read from" / "written into" file address range (fhead + sizes + extra)
  SET_BYTE_RANGE(map->mhead.frng, &(map->fhead.signature), fsize + mextra * sizeof(uint32_t)) ;
  // sizes table address range
  SET_BYTE_RANGE(map->mhead.srng, map->size, zijk * sizeof(fmap_block_size)) ;
  // extra address range starts after sizes table (a 2 byte gap of is possible if the number of blocks zijk is odd)
  SET_BYTE_RANGE(map->mhead.xrng, (void *)(map->size + zijk1), mextra * sizeof(uint32_t)) ;
  // encoded data address range starts just after extra (encoded data + extra blocks)
  SET_BYTE_RANGE(map->mhead.drng, map->mhead.xrng.top, dsize + zsize) ;
  // block offsets address range
  map->mhead.orng.top = (void *)map->mhead.zrng.top ;           // top of offsets table is top of zmap
  map->mhead.orng.bot = map->mhead.orng.top - (zijk + 1) ;      // base address of offsets table [zijk+1]
  // initialize offsets and sizes table
  for(uint32_t ui=0 ; ui<zijk ; ui++){
    map->mhead.orng.bot[ui] = 0 ;                               // offsets are unknown
    map->size[ui] = 0 ;                                         // sizes are unknown
  }
  map->mhead.orng.bot[zijk] = 0 ;                               // last value in offsets table is unknown
  // block pointers remain NULL as set by base_mmap

  // initialize fmap sub structure, accounting for extra blocks
  fmap_init(map, gni, gnj, gnk, bij.i, bij.j, &a3, mextra, zextra) ;
  if(zijk != map->fhead.zijk) goto fail ;                       // inconsistent values for number of blocks

  if(DEBUG){
    zmap_print(map, "empty zmap") ;
  }

end:
  return map ;

fail :          // free what was allocated internally
  if((map != NULL) && (map != map0)){ free(map) ; }
  map = NULL ;
  goto end ;
}

// update offsets using offsets[0] and sizes, check that data area does not overlap offsets/pointers table
// update data range pointers
// map  [INOUT] : pointer to valid zmap struct
// return 0 if successsful, non zero otherwise
int finalize_zmap(zmap *map){
  uint64_t *offsets = map->mhead.orng.bot ;
  fmap_block_size *size = map->size ;
  uint64_t total_size = 0 ;

// it is assumed that offsets[0] has been set properly before calling this function
  for(uint32_t i = 0 ; i < map->fhead.zijk ; i++){              // loop over blocks
    offsets[i+1] = offsets[i] + size[i] * sizeof(uint32_t) ;    // adjust offset for next block
    total_size += size[i] ;                                     // cumulative size
  }
  // adjust top of data range according to cumulative data size
  map->mhead.drng.top = map->mhead.drng.bot + total_size ;

  uint32_t *packed = map->mhead.drng.top ;
  if( PTR(packed) > PTR(offsets) ){
    fprintf(stderr, "ERROR : data stream overlaps offsets/pointers table\n");
    return -1 ;
  }
#if defined(DEBUG)
// in DEBUG/TESTING mode, initialize memory gap between packed and the offsets table
  while(PTR(packed) < PTR(offsets)){
    *packed = 0xF0F0F0F0 ;
    packed++ ;
  }
#endif
  return 0 ;
}

// deallocation of a data map
// map  [INOUT] : pointer to data map
// full    [IN] : if zero, only deallocate pointer table to packed blocks
// INVALIDATE map to prevent accidents in case of inadvertent memory reuse
// return 0 if O.K., 1 if not
int free_zmap(zmap *map){
  if(map == NULL) return 1 ;                 // OUCH, map is NULL
  memset(map, 0xEE, 5) ;                     // after this, version cannot be 0
  if(map->mhead.version != 0) free(map) ;    // always free map
  return 0 ;                                 // success
}
#if 0
// adjust offset table using the sizes table
// map [INOUT] : pointer to zmap (mmap/fmap/sizes/...) struct
// return number of entries in tables if successful, 0 otherwise
int adjust_map_offsets(zmap *map){
  if(map == NULL) return 0 ;
  uint32_t nijk = map->fhead.zijk ;
  uint64_t t ;
  uint64_t limit = RANGE_ELEMENTS(map->mhead.drng) ;    // max number of elements that can be accomodated
  map->mhead.orng.bot[0] = 0 ;
  for(uint32_t i=0 ; i<nijk ; i++){
    t = map->mhead.orng.bot[i] + map->size[i] ;
    if(t > limit) return 0 ;                            // element number exceeded
    map->mhead.orng.bot[i+1] = t ;
  }
  return nijk ;
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

// TODO : redo the whole function

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
#endif
// fill map data buffer with data from address src
// data element size and dimensions will be taken from map
// void fill_zmap(zmap *map, void *src){
// }

// NOTE if first/last/limit out of map we have an external bit stream buffer
// TODO : redo the whole function
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
// TODO : redo the whole function
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
