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
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// test double include protection
#include <rmn/data_kind.h>
#include <rmn/data_kind.h>

#include <rmn/data_map.h>
// #include <rmn/array_nd.h>
// #include <rmn/move_blocks.h>
#include <rmn/misc_helpers.h>
#define HASH(V,NBITS) kwik_hash((V),NBITS)

#undef FAIL
static int StAtUs = 0 ;
#define FAIL(ERR,...) { StAtUs = ERR ; fprintf(stderr, __VA_ARGS__); goto fail ; }

#undef XIT
#define XIT(CODE) { status = CODE ; goto fail ; }

#define NTI 10
#define NTJ 11
#define SF0  4

static uint32_t dummy[32] ;
static uint32_t *dummyp = &(dummy[32]) ;

// arguments for test encoder/decoder
typedef struct{
    uint32_t unp ;        // original element size
    uint32_t pak ;        // packed element size
    uint64_t dummy ;      // not used for now
} codec_args;
CT_ASSERT(sizeof(codec_args) == sizeof(arg128), "sizeof(codec_args) != sizeof(arg128)") ;

// arguments for get/put function with data in memory
typedef struct{
    uint32_t *data ;      // encoded data base memory address
    uint32_t get_put ;    // 1 : get, 0 : put
    uint32_t dummy ;      // not used for now
} getput_memory_args ;
CT_ASSERT(sizeof(getput_memory_args) == sizeof(arg128), "sizeof(getput_memory_args) != sizeof(arg128)") ;

// arguments for get/put function with data in ordinary file
typedef struct{
    size_t offset ;       // encoded data base file address
    uint32_t get_put ;    // 1 : get, 0 : put
    uint32_t fd ;         // file descriptor
} getput_file_args ;
CT_ASSERT(sizeof(getput_file_args) == sizeof(arg128), "sizeof(getput_memory_args) != sizeof(arg128)") ;
//
// ================================= get zmap block(s) =================================
//
// get address range of block[block0] from data stream (full data stream assumed to be in memory)
// map      [IN] : pointer to valid zmap struct
// block0   [IN] : block to copy
// drng     [IN] : if valid, memory range to be copied into
// transfer data into memory pointed to by drng if drng is valid
// in that case an adjusted drng will be returned
// return a range pointing to the requested data
// an invalid range is returned in case of error
RANGE(zmap_t) get_zmap_memory_block(zmap *map, int block0, RANGE(zmap_t) drng){
  if(map == NULL) goto fail ;
  int max_bno = map->fhead.zijk ;
  if(block0 < 0 || block0 >= max_bno) goto fail ;

  uint64_t offset = map->mhead.orng.bot[block0] ;
  offset /= sizeof(uint32_t) ;
  uint32_t size   = map->size[block0] ;
  uint32_t *base  = map->mhead.drng.bot ;
  if(VALID_RANGE(drng)){
    if(RANGE_ELEMENTS(drng) < size) goto fail ;                    // destination range is too small
    memcpy(drng.bot, base + offset, size * sizeof(uint32_t)) ;     // copy into destination
    SET_RANGE_ELEMENTS(drng, size) ;                               // adjust top of range
  }else{
    drng = (RANGE(zmap_t)){ base + offset,  base + offset + size } ;     // point to source with adjusted size
  }

  return drng ;

fail:
  return (RANGE(zmap_t)){ dummyp, dummyp - 1 } ;
}

// read block[block0] from file  (data map does not have a valid data range)
// map      [IN] : pointer to valid zmap struct
// block0   [IN] : block to read
// drng     [IN] : if valid, memory range to be copied into
// transfer data into memory pointed to by drng if drng is valid
// in that case an adjusted drng will be returned
// return a range pointing to the requested data
// an invalid range is returned in case of error
RANGE(zmap_t) get_zmap_file_block(zmap *map, int block0, RANGE(zmap_t) drng){
  if(map == NULL) goto fail ;
  int max_bno = map->fhead.zijk ;
  if(block0 < 0 || block0 >= max_bno) goto fail ;

  getput_file_args *file_args ;
  file_args = (getput_file_args *) &(map->mhead.get_args) ;
  if(file_args->get_put != 1) goto fail ;

  size_t offset = map->mhead.orng.bot[block0] ;                    // offset from beginning of data
  offset += file_args->offset ;                                    // add block offset in file
  ssize_t size   = map->size[block0] * sizeof(uint32_t) ;          // block size
  int fd = file_args->fd ;                                         // file descriptor
  if(VALID_RANGE(drng)){
    if(RANGE_ELEMENTS(drng) < size) goto fail ;                    // destination range is too small
    lseek(fd, offset, SEEK_SET) ;
    ssize_t nc = read(fd, drng.bot, size) ;                        // read from file
    if(nc != size) goto fail ;                                     // short/failed read
    SET_RANGE_BYTES(drng, size) ;                                  // adjust top of range
  }
  return drng ;                                                    // return adjusted range

fail:
  return (RANGE(zmap_t)){ dummyp, dummyp - 1 } ;
}
//
// ================================= pack / unpack / codec =================================
// pack unsigned 32 -> 16 ;
// return number of bytes written into out_
int demo_pack_block_32_16(zmap *map, void *out_, void *in_, int ninj){
  if(map == NULL || ninj <= 0) return -1 ;
  uint32_t *in  = in_ ;
  uint16_t *out = out_ ;
  if(out == NULL || in == NULL) return -1 ;
  codec_args *local = (void *)&(map->mhead.codec_args) ;            // point local arguments into proper place in zmap
  CT_ASSERT( sizeof(*local) == sizeof(map->mhead.codec_args) , "bad codec arguments struc size" )
  if(local->unp != 32 || local->pak != 16) return -1 ;
  for(int i=0 ; i<ninj ; i++){ out[i] = in[i] & 0xFFFF ; } ;
  return ninj * sizeof(uint16_t) ;
}

// unpack unsigned 16 -> 32 ;
// return number of bytes read from in_
int demo_unpack_block_16_32(zmap *map, void *out_, void *in_, int ninj){
  if(map == NULL || ninj <= 0) return -1 ;
  uint16_t *in = in_ ;
  uint32_t *out = out_ ;
  if(out == NULL || in == NULL) return -1 ;
  struct{
    uint32_t unp ;
    uint32_t pak ;
    uint64_t dummy ;
  } *local = (void *)&(map->mhead.codec_args) ;            // point local arguments into proper place in zmap
  CT_ASSERT( sizeof(*local) == sizeof(map->mhead.codec_args) , "bad codec arguments struc size" )
  if(local->pak != 16 || local->unp != 32) return -1 ;
  for(int i=0 ; i<ninj ; i++){ out[i] = in[i] & 0xFFFF ; } ;
  return ninj * sizeof(uint16_t) ;
}

codec_fn demo_codec_16_32 ;
int demo_codec_16_32(zmap *map, void *out_, void *in_, int ninj, int encode){
  if(encode == 1){
    return demo_pack_block_32_16(map, out_, in_, ninj) ;
  }else{
    return demo_unpack_block_16_32(map, out_, in_, ninj) ;
  }
}

// pack unsigned 32 -> 8 ;
// return number of bytes written into out_
int demo_pack_block_32_8(zmap *map, void *out_, void *in_, int ninj){
  if(map == NULL || ninj <= 0) return -1 ;
  if(map->fhead.esize != 4) exit(1) ;    // unpacked element size MUST BE 4 BYTES
  uint32_t *in = in_ ;
  uint8_t *out = out_ ;
  if(out == NULL || in == NULL) return -1 ;
  codec_args *local = (void *)&(map->mhead.codec_args) ;            // point local arguments into proper place in zmap
  CT_ASSERT( sizeof(*local) == sizeof(map->mhead.codec_args) , "bad codec arguments struc size" )
  if(local->unp != 32 || local->pak != 8) return -1 ;
  for(int i=0 ; i<ninj ; i++){ out[i] = in[i] & 0xFF ; } ;
  return ninj * sizeof(uint8_t) ;
}

// unpack unsigned 8 -> 32 ;
// return number of bytes read from in_
int demo_unpack_block_8_32(zmap *map, void *out_, void *in_, int ninj){
  if(map == NULL || ninj <= 0) return -1 ;    // unpacked element size MUST BE 4 BYTES
  if(map->fhead.esize != 4) exit(1) ;
  uint8_t *in = in_ ;
  uint32_t *out = out_ ;
  if(out == NULL || in == NULL) return -1 ;
  struct{
    uint32_t unp ;
    uint32_t pak ;
    uint64_t dummy ;
  } *local = (void *)&(map->mhead.codec_args) ;            // point local arguments into proper place in zmap
  CT_ASSERT( sizeof(*local) == sizeof(map->mhead.codec_args) , "bad codec arguments struc size" )
  if(local->pak != 8 || local->unp != 32) return -1 ;
  for(int i=0 ; i<ninj ; i++){ out[i] = in[i] & 0xFF ; } ;
  return ninj * sizeof(uint8_t) ;
}

static codec_fn demo_codec_8_32 ;
static int demo_codec_8_32(zmap *map, void *out_, void *in_, int ninj, int encode){
  if(encode == 1){
    return demo_pack_block_32_8(map, out_, in_, ninj) ;
  }else{
    return demo_unpack_block_8_32(map, out_, in_, ninj) ;
  }
}
//
// ================================= fill and verify data ==================================
//
// fill an array (gni x gnj) with known values
static void fill_data(int gni, int gnj, uint32_t data[gnj][gni]){
  if(gni < 256){
    for(int j=0 ; j<gnj ; j++){
      for(int i=0 ; i<gni ; i++){
        data[j][i] = HASH((i<<16)|j, 8) ;
      }
    }
  }else{
    for(int j=0 ; j<gnj ; j++){
      for(int i=0 ; i<gni ; i++){
        data[j][i] = HASH((i<<16)|j, 16) ;
      }
    }
  }
}

static uint32_t verify_hash(int gni, uint32_t data[][gni], int i0, int lni, int j0, int lnj){
  uint32_t errors = 0 ;
  if(gni < 256){
    for(int j=0 ; j<lnj ; j++){
      for(int i=0 ; i<lni ; i++){
        if( data[j][i] != HASH(((i+i0)<<16)|(j+j0), 8) ) errors++ ;
      }
    }
  }else{
    for(int j=0 ; j<lnj ; j++){
      for(int i=0 ; i<lni ; i++){
        if( data[j][i] != HASH(((i+i0)<<16)|(j+j0), 16) ) errors++ ;
      }
    }
  }
  return errors ;
}

// check the contents of an array (gni x gnj) with known values
static uint32_t check_data(int gni, int gnj, uint32_t data[gnj][gni], int i0, int lni, int j0, int lnj){
  uint32_t errors = 0 ;
  errors += verify_hash(gni, (void *)&(data[j0][i0]), i0, lni, j0, lnj) ;
  return errors ;
}
//
// =========================================================================================
//
block_fn get_zmap_file_blocks ;  // check that get_zmap_file_blocks prototype is compatible with block_fn
RANGE(zmap_t) get_zmap_file_blocks(zmap *map, int block0, int block_nb, RANGE(zmap_t) rng){
  int status ;

  if(map == NULL) XIT(-1) ;                              // no map ;
  if(block0 < 0) XIT(-3) ;                               // invalid block number
  if(block_nb <= 0) XIT(-4) ;                            // invalid number of blocks
  int max_blocks = map->fhead.zijk ;
  if(block0+block_nb > max_blocks) XIT(-5) ;             // last block number exceeds available blocks

  if(block_nb == 1){                                     // special case : get 1 block
    return get_zmap_file_block(map, block0, rng) ;
  }else{
    XIT(-10) ;                                           // block_nb > 1 not supported for the time being
  }
// will have to allocate space
  rng = RANGE_NULL(zmap_t) ;
  return rng ;

fail:
  return (RANGE(zmap_t)){ dummyp, dummyp + status } ;
}
//
// copy block(s) of 32 bit elements from memory pointed to by data map into user space
// demo function for tests purposes
// map      [IN] : pointer to valid zmap struct
// block0   [IN] : first block to copy
// block_nb [IN] : number of blocks to copy
// drng  [INOUT] : address range describing destination (32 bit elements)
// return number of words copied of negative error codes
//
block_fn get_zmap_mem_blocks ;  // check that get_zmap_mem_blocks prototype is compatible with block_fn
RANGE(zmap_t) get_zmap_mem_blocks(zmap *map, int block0, int block_nb, RANGE(zmap_t) drng){
  (void) (drng) ;                                        // not used in this case
  int status ;

  if(map == NULL) XIT(-1) ;                              // no map ;
  if(block0 < 0) XIT(-3) ;                               // invalid block number
  if(block_nb <= 0) XIT(-4) ;                            // invalid number of blocks
  int max_blocks = map->fhead.zijk ;
  if(block0+block_nb > max_blocks) XIT(-5) ;             // last block number exceeds available blocks

  if(block_nb == 1){                                     // special case : get 1 block
    return get_zmap_memory_block(map, block0, RANGE_NULL(zmap_t)) ;
  }else{
    XIT(-10) ;                                           // block_nb > 1 not supported for the time being
// temporarily deactivate code to remove warnings
#if 0
  getput_memory_args *local = (getput_memory_args *)(&(map->mhead.get_args)) ;
  CT_ASSERT( sizeof(*local) == sizeof(map->mhead.get_args) , "bad getblock arguments struc size" )
  uint32_t *base = local->data ;                          // get base address for memory copy from zmap (usually data area)
  if(base == NULL) XIT(-6) ;

  if(INVALID_RANGE(drng)) XIT(-2) ;                       // invalid destination range
  uint32_t *out = drng.bot ;
  int32_t size_out = RANGE_ELEMENTS(drng) ;

  // consecutive blocks in tables are assumed to be consecutive in storage
  // check that this is the case. if not, transfer block by block
  int32_t size = contiguous_zmap_blocks(map, block0, block0+block_nb-1) ;
  int contiguous = (size > 0) ;
  size = (size < 0) ? (-size) : size ;
  if(size_out < size) XIT(-7) ;                          // not enough space for copy

  uint64_t offset = map->mhead.orng.bot[block0] ;        // block offset of first block relative to base address (in bytes)
  offset = offset / sizeof(uint32_t) ;                   // offset in 32 bit words
  uint32_t *src = base + offset ;                        // source address
  if(contiguous){
    for(int32_t i=0 ; i<size ; i++){ out[i] = src[i] ; }
  }else{
    // OUCH for now
  }
#endif
  }
  return drng ;                                          // number of 32 bit words copied

fail:
  return (RANGE(zmap_t)){ dummyp, dummyp + status } ;
}

// copy block(s) of 32 bit elements from user space into memory pointed to by data map
// demo function for tests purposes
// map   [INOUT] : pointer to valid zmap struct
// block0   [IN] : first block to copy
// block_nb [IN] : number of blocks to copy
// drng     [IN] : address range describing source (32 bit elements)
//TODO : UPDATE AND DEBUG THIS FUNCTION
RANGE(zmap_t)  put_mapped_blocks(zmap *map, int block0, int block_nb, RANGE(zmap_t) drng){
  int status ;

  if(map == NULL) XIT(-1) ;                              // no map ;
  if(INVALID_RANGE(drng)) XIT(-2) ;                      // drng MUST be a valid range

  if(block0 < 0) XIT(-3) ;                               // invalid block number
  if(block_nb <= 0) XIT(-4) ;                            // invalid number of blocks
  int max_blocks = map->fhead.zijk ;
  if(block0+block_nb > max_blocks) XIT(-5) ;             // last block number exceeds available blocks

  if(block_nb == 1){                                     // special case : get 1 block
//     return putt_zmap_mem_block(map, block0, RANGE_NULL(zmap_t)) ;
    XIT(-10) ;                                           // NOT IMPLEMENTED YET
  }else{
    XIT(-10) ;                                           // block_nb > 1 not supported for the time being
  }

fail:
  return (RANGE(zmap_t)){ dummyp, dummyp + status } ;
}

// copy blk[0:lnj-1][0:lni-1] into dst[0:lnj-1][0:lni-1], dst is dimensioned [gnj][gni]
void put_block(uint32_t lni, uint32_t lnj, uint32_t blk[lnj][lni], uint32_t gni, uint32_t gnj, uint32_t dst[gnj][gni]){
  for(uint32_t j=0 ; j<lnj ; j++){
    for(uint32_t i=0 ; i<lni ; i++){
      dst[j][i] = blk[j][i] ;
    }
  }
}
// copy src[0:lnj-1][0:lni-1] into blk[0:lnj-1][0:lni-1], src is dimensioned [gnj][gni]
void get_block(uint32_t lni, uint32_t lnj, uint32_t blk[lnj][lni], uint32_t gni, uint32_t gnj, uint32_t src[gnj][gni]){
  for(uint32_t j=0 ; j<lnj ; j++){
    for(uint32_t i=0 ; i<lni ; i++){
      blk[j][i] = src[j][i] ;
    }
  }
}
//
// =========================================================================================
//
void  test_fill_offset(zmap *map){
  for(uint32_t i=map->fhead.zni * map->fhead.znj ; i<map->fhead.zijk ; i++) map->size[i] = map->fhead.zijk - i ;
  map->mhead.orng.bot[0] = 0 ;
  for(uint32_t i=1 ; i<(map->fhead.zijk)+1 ; i++){
    map->mhead.orng.bot[i] = map->mhead.orng.bot[i-1] + (map->size[i-1] * sizeof(uint32_t)) ;
  }
}

void  test_fill_size(int zni, int znj, fmap_block_size size[znj][zni], zmap *map){
  int li0 = map->fhead.li0, lni = map->fhead.lni, lj0 = map->fhead.lj0, lnj = map->fhead.lnj ;
  int i, j, sizei, sizej ;
  for(j=0, sizej=lj0 ; j<znj ; j++, sizej=lnj){
    for(i=0, sizei=li0 ; i<zni ; i++, sizei=lni){
      size[j][i] = sizei*sizej ;
    }
  }
}

#undef MAX
#define MAX(A,B) ( ((A) > (B)) ? (A) : (B) )

static int64_t filewords = 0 ;

// extract data from full zmap, store into array
// for test purposes, get blocks in reverse order
// map    [IN] : pointer to valid zmap struct
// gni    [IN] : row length of array
// gnj    [IN] : number of rows in array
// array [OUT] : data destination for decoded blocks
void  get_data_from_zmap(zmap *map, int gni, int gnj, uint32_t array[gnj][gni]){
  int32_t lni, lnj, i0, j0, in, jn, i, j, bno ;
  int32_t zni = map->fhead.zni, znj = map->fhead.znj ;
  uint32_t maxi = MAX(map->fhead.li0, map->fhead.lni) ;
  uint32_t maxj = MAX(map->fhead.lj0, map->fhead.lnj) ;
  uint32_t esize = map->fhead.esize ;
  uint8_t block[esize*maxi*maxj] ;                                // largest decoded block
  uint8_t coded[sizeof(uint32_t)*maxi*maxj+sizeof(uint32_t)] ;    // largest encoded block
  RANGE(zmap_t) r_temp ;
  fmap_block_size *sizes, size, siz0 ;
  size_t total_size = 0 ;
  int nbytes, errors = 0 ;
  codec_fn *codec = map->mhead.codec ;

  SET_RANGE(r_temp, coded, sizeof(coded)) ;
  bno = 0 ;
  sizes = map->size ;
  fprintf(stderr, "========== extracting %d array blocks (%d byte elements) from zmap ==========\n", zni*znj, esize) ;
  for(j=znj-1 ; j>=0 ; j--){
    index_range j_index = index_limits(j, map->fhead.lnj, map->fhead.lj0) ;
    j0 = j_index.ix0 ; jn = j_index.ixn ; lnj = jn - j0 + 1 ;

    for(i=zni-1 ; i>=0 ; i--){
      index_range i_index = index_limits(i, map->fhead.lni, map->fhead.li0) ;
      i0 = i_index.ix0 ; in = i_index.ixn ; lni = in - i0 + 1 ;
      bno = BLOCK_IJ(map,i,j) ;

      // some get functions may not have a need for r_temp and will just ignore it
      RANGE(zmap_t) r_coded = ZMAP_GET(map, bno, 1, r_temp) ;                  // use get block function from zmap to get encoded range
      nbytes = (*codec)(map, block, r_coded.bot, lni * lnj, 0) ;               // decode block of packed data (lni * lnj values)
      put_block(lni, lnj, (void *)block, gni, gnj, (void *)(&array[j0][i0])) ; // insert decoded data into array
      errors = verify_hash(lni, (void *)block, i0, lni, j0, lnj) ;             // does decoded data match expected values

      siz0 = nbytes / sizeof(uint32_t) ;
      size = (nbytes + sizeof(uint32_t) - 1) / sizeof(uint32_t) ;              // round size up to multiple of sizeof(uint32_t)
      total_size += size ;
      if(nbytes == -1) exit(1) ;
      if(size != sizes[bno]) exit(1) ;
      fprintf(stderr, "zblock[%d,%d]<%d> = array[%3d:%3d,%3d:%3d]", i, j, bno, i0, in, j0, jn) ;
      fprintf(stderr, ", codec unpack : nb = %d", nbytes) ;
      fprintf(stderr, ", sizes[%d] = %4d(%4d), offset = %6ld, data = %8.8x\n", bno, sizes[bno], siz0, map->mhead.orng.bot[bno], block[0]);
      if(errors != 0){
        fprintf(stderr, "ERROR at block [j=%d][i=%d], restored data not as expected\n", j, i) ;
        exit(1) ;
      }

    }
  }
  fprintf(stderr, "words extracted = %ld, expected = %ld\n\n", total_size, filewords) ;
}

// map    [INOUT] : pointer to valid zmap struct
// gni       [IN] : row length of array
// gnj       [IN] : number of rows in array
// array     [IN] : array to encode and store into zmap
// restored [OUT] : array to receive decode/move result for error checking purposes
void fill_zmap_with_data(zmap *map, int gni, int gnj, uint32_t array[gnj][gni], uint32_t restored[gnj][gni]){
  uint32_t lni, lnj, i0, j0, in, jn, i, j ;
  uint32_t zni = map->fhead.zni, znj = map->fhead.znj ;
  uint32_t maxi = MAX(map->fhead.li0, map->fhead.lni) ;
  uint32_t maxj = MAX(map->fhead.lj0, map->fhead.lnj) ;
  uint32_t block[maxi*maxj] ;
  uint32_t *packed, bno, *bot, *top ;
  int nbytes, nrestored, errors = 0 ;
  fmap_block_size *sizes, size /*, siz0*/ ;
  uint64_t *offsets, offset ;
  int64_t inserted ;

  sizes = map->size ;
  offsets = map->mhead.orng.bot ;
  offsets[0] = 0 ;
  bno = 0 ;

  fprintf(stderr, "sizeof(block) = %ld bytes, %ld elements\n", sizeof(block), sizeof(block)/sizeof(uint32_t));
  bot    = PTR_CAST(map->mhead.drng.bot, uint32_t) ;   // bottom of data area
  top    = PTR_CAST(map->mhead.drng.top, uint32_t) ;   // top of data area
  packed = bot ;
  for(j=0, j0 = 0, lnj = map->fhead.lj0 ; j<znj ; j++, j0+=lnj, lnj=map->fhead.lnj){
    jn = j0 + lnj - 1 ;      // top row
    for(i=0, i0 = 0, lni = map->fhead.li0 ; i<zni ; i++, i0+=lni, lni=map->fhead.lni){
      in = i0 + lni - 1 ;    // last column
      fprintf(stderr, "zblock[%d,%d]<%d> = array[%3d:%3d,%3d:%3d]", i, j, bno, i0, in, j0, jn) ;
      get_block(lni, lnj, (void *)block, gni, lnj, (void *)(&array[j0][i0])) ;          // get a block of data

      if((top - packed) < (lni * lnj + 1)) exit(1) ;                                    // worst case encoding would fail

      nbytes = ZMAP_CODEC(map, (uint16_t *)packed, block, lni * lnj, 1) ;               // encode data
      fprintf(stderr, ", codec   pack : nb = %d bytes", nbytes) ;
      if(nbytes == -1) exit(1) ;
//       siz0 = nbytes / sizeof(uint32_t) ;
      size = (nbytes + sizeof(uint32_t) - 1) / sizeof(uint32_t) ;                       // round size up to multiple of sizeof(uint32_t)
      sizes[bno] = size ;                                                               // size in 32 bit units
      offset = offsets[bno] / sizeof(uint32_t) ;                                        // offset in 32 bit units
      offsets[bno+1] = offsets[bno] + (size * sizeof(uint32_t)) ;                       // offset in bytes for next block

      nrestored = ZMAP_CODEC(map, block, (uint16_t *)packed, lni * lnj, 0) ;            // decode block into  restored
      if(nrestored != nbytes) exit(1) ;                                                 // size mismatch or decoding failed
      errors = verify_hash(lni, (void *)block, i0, lni, j0, lnj) ;                      // does decoded data match original data
      put_block(lni, lnj, (void *)block, gni, gnj, (void *)(&restored[j0][i0])) ;       // move to restored array

//       fprintf(stderr, ", codec unpack : nb = %d", nbytes) ;
      fprintf(stderr, ", sizes[%d] = %4d Bytes (%4d), offset = %6ld, block[0][0] = %8.8x, check %s\n",
              bno, sizes[bno]*4, nbytes, map->mhead.orng.bot[bno], block[0], errors ? "FAILED" : "O.K.");
      if(errors) exit(1) ;
      if(packed != bot+offset) exit(1) ;

      packed += size ;
      bno++ ;
    }
  }
  filewords = inserted = PTR_ELEMENTS(map->mhead.drng.bot, packed) ;
  fprintf(stderr, "inserted %ld words (%ld bytes)\n", inserted, inserted*4) ;
  map->mhead.drng.top = packed ;                         // adjust top of data range
  if( PTR(packed) > PTR(offsets) ) exit(1) ;             // OUCH !! top of data range overlaps offset table
  while(PTR(packed) < PTR(offsets)){                     // fill rest of data space with garbage
    *packed = 0xF0F0F0F0 ;
    packed++ ;
  }
}

// create a populated 2 D data map
zmap *create_test_zmap_2d(int32_t gni, int32_t gnj, int32_t bsize, int32_t mextra, int32_t bextra){
  zmap *map ;
  int32_t aspect = 1, errors ;
  uint32_t data[gnj][gni], restored[gnj][gni] ;

  // create and populate the data_map + data struct (bextra supplementary blocks) 4 byte elements, allocate data
  map = create_zmap(gni, gnj, 1, bsize, aspect, 1*sizeof(uint32_t), mextra, bextra, 8*bextra*sizeof(uint32_t), 0) ;
  for(uint32_t i = (map->fhead.zni * map->fhead.znj * map->fhead.gnk) ; i < map->fhead.zijk ; i++){
    map->size[i] = map->fhead.zijk - i ;                                                       // fix supplementary blocks size
  }
  SET_CODEC_FN(map, demo_codec_8_32) ;                               // set pack/restore codec function address
  SET_CODEC_ARGS(map, ((codec_args){32, 8, 0}) ) ;                   // set pack/restore codec arguments
//   SET_GET_FN(map, get_zmap_mem_blocks) ;                             // set get block function address
//   SET_GET_ARGS(map, ((getput_memory_args){ ZMAP_DATA(map), 1, 0 }) ) ;      // set get block function argument

  fill_data(gni, gnj, (void *)data) ;                                // fill and check reference array
  errors = check_data(gni, gnj, (void *)data, 0, gni, 0, gnj) ;
  if( errors != 0){ FAIL(1, "ERROR : %d error(s) creating reference data\n", errors) }
  fprintf(stderr, "reference data created and checked\n") ;

  fill_zmap_with_data(map, gni, gnj, (void *)data, (void *)restored) ;    // fill zmap blocks with encoded data, decode to check
  errors = check_data(gni, gnj, (void *)restored, 0, gni, 0, gnj) ;       // check that restored while filling data is as expected
  if( errors != 0){ FAIL(1, "ERROR : %d error(s) filling zmap\n", errors) ; }
  fprintf(stderr, "zmap data filled and checked\n") ;
  ZMAP_OFFSETS(map)[0] = 0 ;                                              // offset for first block
  if(finalize_zmap(map) != 0) exit(1) ;                                   // update offsets table

  return map ;
fail:
  return NULL ;
}

int main(int argc, char **argv){
  (void)(argc) ; (void)(argv) ;  //  suppress unused argument warning
  int i, j, fd ;
//   int x[NTI], y[NTI] ;
//   index_pair ijp ;
  index_range irange ;
//   ij_range ijr ;
  char *msg = "" ;
  int32_t gni, gnj, gnk, bsize, aspect, bsizej, oor ;
  int32_t bextra, mextra ;
  uint32_t total_blocks, *restored ;
  zmap *zp0, *zp1, *zp2, *zpw, *zpr, *zpf ;

  goto test ;
success:
  fprintf(stderr, "SUCCESS\n") ;
  return 0 ;
fail:
  fprintf(stderr, "FAIL : status = %d, %s\n", StAtUs, msg) ;
  return 1 ;
test:

//   if(argc > 0) retumap->mheadrn 0 ;

  fprintf(stderr, "=============== base test ===============\n") ;

  gni = 129 ; gnj = 97 ; gnk = 1 ; bsize = 64 ;
  fprintf(stderr, "base size of mmap = %ld (%ld words)\n", sizeof(mmap), sizeof(mmap)/sizeof(uint32_t));
  fprintf(stderr, "base size of fmap = %ld (%ld words)\n", sizeof(fmap), sizeof(fmap)/sizeof(uint32_t));
  fprintf(stderr, "base size of zmap = %ld (%ld words)\n", sizeof(zmap), sizeof(zmap)/sizeof(uint32_t));

  fprintf(stderr, "=============== zmap creation test ===============\n") ;

  int status ;
  uint32_t map_words, rec_words, zmap_words, data_words, errors ;
  uint64_t displacement ;
  gni = 129 ; gnj = 97 ; gnk = 1 ; bsize = 64 ; aspect = 1 ; mextra = 2 ; bextra = 4 ;

  zpw = create_test_zmap_2d(gni, gnj, bsize, mextra, bextra) ;
  mmap_print(zpw, "zpw") ;
  print_zmap_blocks(zpw, 100) ;
  oor = zmap_blocks_out_of_range(zpw) ;
  if(oor != 0) FAIL(1, "%d blocks are out of range in zpw\n", oor) ;

  fprintf(stderr, "=============== zmap in memory read test ===============\n") ;

  map_words = FILEMAP_WORDS(zpw) ;
  rec_words = RECORD_WORDS(zpw) ;
  zpr = create_file_zmap(map_words, rec_words) ;                                     // STEP 1
  // only copy data map part fom zp0
  memcpy(&(zpr->fhead), &(zpw->fhead), map_words * sizeof(uint32_t)) ;               // STEP 2a 
  // now copy data part
  memcpy(zpr->mhead.drng.bot, zpw->mhead.drng.bot, RANGE_BYTES(zpw->mhead.drng)) ;   // STEP 2b
  update_file_zmap(zpr) ;                                                            // STEP 2c
  SET_CODEC_FN(zpr, demo_codec_8_32) ;                               // set pack/restore codec function address
  SET_CODEC_ARGS(zpr, ((codec_args){32, 8, 0}) ) ;                   // set pack/restore codec arguments
  SET_GET_FN(zpr, get_zmap_mem_blocks) ;                             // set get block function address
  SET_GET_ARGS(zpr, ((getput_memory_args){ ZMAP_DATA(zpr), 1, 0 }) ) ;      // set get block function argument
  mmap_print(zpr, "zpr") ;
  print_zmap_blocks(zpr, 100) ;

  // extract data blocks
  restored = (uint32_t *)malloc(gni*gnj*sizeof(uint32_t)) ;
  if(restored == NULL) FAIL(1, "ERROR : allocate restored array failed") ;
  bzero(restored, gni * gnj * sizeof(uint32_t)) ;                    // set restored to 0
  get_data_from_zmap(zpr, gni, gnj, (void *)restored) ;
  errors = check_data(gni, gnj, (void *)restored, 0, gni, 0, gnj) ;
  if( errors != 0){ FAIL(1, "ERROR : %d error(s) in restored data\n", errors) ; }
  oor = zmap_blocks_out_of_range(zpr) ;
  if(oor != 0) FAIL(1, "%d blocks are out of range in zpr\n", oor) ;

  free(restored) ;
  free_zmap(zpr) ;

  fprintf(stderr, "=============== zmap file create ===============\n") ;

  char *file_name = "/tmp/zmap_file" ;
  map_words = FILEMAP_WORDS(zpw) ;
  rec_words = RECORD_WORDS(zpw) ;
  fd = open(file_name, O_CREAT | O_RDWR, 0777) ;
  if(fd < 0) exit(1) ;
  if( write(fd, &map_words, sizeof(uint32_t))    != sizeof(uint32_t)) exit(1) ;
  if( write(fd, &rec_words, sizeof(uint32_t))    != sizeof(uint32_t)) exit(1) ;
  displacement = 32768*32 ;
  if( write(fd, &displacement, sizeof(uint64_t)) != sizeof(uint64_t)) exit(1) ;
  lseek(fd, displacement, SEEK_SET) ;
  write(fd, &(zpw->fhead), rec_words*sizeof(uint32_t)) ;
  close(fd) ;
  fprintf(stderr, "successfully created file '%s'\n", file_name) ;

  fprintf(stderr, "=============== zmap from file read test ===============\n") ;

  zpf = create_file_zmap(map_words, map_words) ;                                     // STEP 1
  mmap_print(zpr, "zpf") ;
  fd = open("/tmp/zmap_file", O_RDONLY) ;
  if(fd < 0) exit(1) ;
  if( read(fd, &map_words, sizeof(uint32_t))    != sizeof(uint32_t)) exit(1) ;
  if(map_words != FILEMAP_WORDS(zpw)) exit(1) ;
  if( read(fd, &rec_words, sizeof(uint32_t))    != sizeof(uint32_t)) exit(1) ;
  if(rec_words != RECORD_WORDS(zpw)) exit(1) ;
  if( read(fd, &displacement, sizeof(uint64_t)) != sizeof(uint64_t)) exit(1) ;
  if(displacement != 32768*32) exit(1) ;
  lseek(fd, displacement, SEEK_SET) ;
  read(fd, &(zpf->fhead), map_words * sizeof(uint32_t)) ;                            // read data map
  displacement += (map_words * sizeof(uint32_t)) ;                                   // offset for data
  update_file_zmap(zpf) ;                                                            // STEP 2c

  SET_CODEC_FN(zpf, demo_codec_8_32) ;                               // set pack/restore codec function address
  SET_CODEC_ARGS(zpf, ((codec_args){32, 8, 0}) ) ;                   // set pack/restore codec arguments
  SET_GET_FN(zpf, get_zmap_file_blocks) ;
  SET_GET_ARGS(zpf, ((getput_file_args){ displacement, 1, fd }) ) ;
  mmap_print(zpr, "zpf") ;
  print_zmap_blocks(zpr, 100) ;

  bzero(restored, gni * gnj * sizeof(uint32_t)) ;                    // set restored to 0
  get_data_from_zmap(zpr, gni, gnj, (void *)restored) ;
  errors = check_data(gni, gnj, (void *)restored, 0, gni, 0, gnj) ;
  if( errors != 0){ FAIL(1, "ERROR : %d error(s) in restored data\n", errors) ; }

  close(fd) ;
  if( unlink(file_name) ) { FAIL(1, "ERROR : failed to delete file '%s'\n", file_name) ; }
  fprintf(stderr, "successfully closed file '%s'\n", file_name) ;
  free_zmap(zpf) ;
  fprintf(stderr, "freed data map zpf\n") ;

if(argc < 1000) goto success ;  // avoid warning about unreachable code

  for(aspect = 1 ; aspect < 4 ; aspect++){
    if(aspect < 3) continue ;

    fprintf(stderr, "=============== aspect = %d ===============\n", aspect) ;
    if(aspect == 2) bsize = 48 ;
    if(aspect == 3) bsize = 32 ;
    bsizej = aspect * bsize ;

    uint32_t blocks = filemap_blocks(gni, gnj, gnk, bsize, bsizej);
    fprintf(stderr, "array[%d,%d,%d], block size = [%d:%d], nblocks = %d", gni, gnj, gnk, bsize, bsizej, blocks) ;
    fprintf(stderr, ", file map size = %ld words\n", filemap_needed_bytes(gni, gnj, gnk, bsize, bsizej, bextra)/sizeof(uint32_t)) ;
    uint32_t nwords = filemap_needed_bytes(gni, gnj, gnk, bsize, bsizej, bextra)/sizeof(uint32_t) ;
    if(filemap_needed_words(gni, gnj, gnk, bsize, bsizej, bextra) != nwords) {
      fprintf(stderr, "ERROR: filemap_needed_words | filemap_needed_bytes mismatch\n");
      goto fail ;
    }

    zmap *zp = create_file_zmap(nwords+mextra, nwords+mextra+100) ;         // mextra, 100 words of data
    if(fmap_invalid(zp) == 0) goto fail ;           // fmap is invalid at this point
    fmap_init(zp, gni, gnj, gnk, bsize, bsizej, NULL, mextra, bextra);   // initialize fmap part with bextra extra blocks
    zp->fhead.extra = mextra ;                                    // set extra to mextra words
    fmap_print(zp, "zp") ;
    if(fmap_invalid(zp) != 0) goto fail ;           // fmap must be valid at this point
    fprintf(stderr, "    fmap element size = %ld\n", ELEMENT_SIZE(zp->mhead.frng)) ;
    fprintf(stderr, "    filemap words = %d, zmap at %p, fmap at %p, blocks[%d:%d]\n",
            filemap_words(zp), &(zp->mhead.signature), &(zp->fhead.signature), zp->fhead.zni, zp->fhead.znj) ;
    fprintf(stderr, "\n");
    mmap_print(zp, "created file zp") ;
    fprintf(stderr, "\n");
    update_file_zmap(zp);
    mmap_print(zp, "updated file zp") ;
    fprintf(stderr, "-----------\n");

    free(zp) ;
    zp = create_zmap(gni, gnj, gnk, bsize, aspect, 2*sizeof(uint32_t), mextra, bextra, 3*bextra*sizeof(uint32_t), 0) ;
    fprintf(stderr, "data map length = %ld words, record length = %ld words\n", FILEMAP_WORDS(zp), RECORD_WORDS(zp)) ;
    fprintf(stderr, "\n");
    free(zp) ;
    zp = create_zmap(gni, gnj, gnk, bsize, aspect, 2*sizeof(uint32_t), mextra, 0, 3*bextra*sizeof(uint32_t), 0) ;
    fprintf(stderr, "data map length = %ld words, record length = %ld words\n", FILEMAP_WORDS(zp), RECORD_WORDS(zp)) ;
    fprintf(stderr, "\n");
    free(zp) ;
    zp = create_zmap(gni, gnj, gnk, bsize, aspect, 2*sizeof(uint32_t), mextra, 0, 0, 0) ;
    fprintf(stderr, "\n");
    free(zp) ;
  }

  fprintf(stderr, "=============== zmap in memory test ===============\n") ;
//   zmap *zp0, *zp1, *zp2, *zpw ;
// 
//   int status ;
//   uint32_t map_words, rec_words, zmap_words, data_words, errors ; 
  gni = 129 ; gnj = 97 ; gnk = 1 ; bsize = 64 ; aspect = 1 ; mextra = 2 ; bextra = 4 ;

//   zpw = create_test_zmap_2d(gni, gnj, bsize, mextra, bextra) ;
//   mmap_print(zpw, "zpw") ;
//   print_zmap_blocks(zpw, 100) ;
//   free_zmap(zpw) ;

if(argc < 1000) goto success ;  // avoid warning about unreachable code

  uint32_t *data = (uint32_t *)malloc(gni * gnj * sizeof(uint32_t *)) ;          // allocate reference data array
  if(data == NULL) goto fail;
  fill_data(gni, gnj, (void *)data) ;                                            // fill reference array
  errors = check_data(gni, gnj, (void *)data, 0, gni, 0, gnj) ;
  if( errors != 0) FAIL(1, "ERROR : %d error(s) in reference data\n", errors)

  restored = (uint32_t *)malloc(gni * gnj * sizeof(uint32_t *)) ;                // allocate memory for restoring data
  if(restored == NULL) goto fail;

  // create and populate the data_map + data struct (bextra supplementary blocks), allocate space for data
  zp0 = create_zmap(gni, gnj, gnk, bsize, aspect, 2*sizeof(uint32_t), mextra, bextra, 8*bextra*sizeof(uint32_t), 0) ;
  for(uint32_t i = (zp0->fhead.zni * zp0->fhead.znj * zp0->fhead.gnk) ; i < zp0->fhead.zijk ; i++){
    zp0->size[i] = zp0->fhead.zijk - i ;                                                       // fix supplementary blocks size
  }
  SET_CODEC_FN(zp0, demo_codec_8_32) ;                               // set pack/restore codec function address
  SET_CODEC_ARGS(zp0, ((codec_args){32, 8, 0}) ) ;                  // set pack/restore codec arguments
  SET_GET_FN(zp0, get_zmap_mem_blocks) ;                             // set get block function address
  SET_GET_ARGS(zp0, ((getput_memory_args){ ZMAP_DATA(zp0), 1, 0 }) ) ;      // set get block function argument

  fill_zmap_with_data(zp0, gni, gnj, (void *)data, (void *)restored) ;    // fill zmap blocks with encoded data
  errors = check_data(gni, gnj, (void *)restored, 0, gni, 0, gnj) ;       // check that restored while filling data is as expected
  if( errors != 0){ FAIL(1, "ERROR : %d error(s) in put\n", errors) ; }
  if(finalize_zmap(zp0) != 0) exit(1) ;
  mmap_print(zp0, "zp0") ;
  print_zmap_blocks(zp0, 100) ;

  // extract data blocks
  bzero(restored, gni * gnj * sizeof(uint32_t)) ;                   // set restored to 0
  get_data_from_zmap(zp0, gni, gnj, (void *)restored) ;
  errors = check_data(gni, gnj, (void *)restored, 0, gni, 0, gnj) ;
  if( errors != 0){ FAIL(1, "ERROR : %d error(s) in restored data\n", errors) ; }

  fprintf(stderr, "=============== zmap in pseudo file test ===============\n") ;

  map_words = FILEMAP_WORDS(zp0) ;
  rec_words = RECORD_WORDS(zp0) ;
  fprintf(stderr, "data map length = %d words, record length = %d words\n", map_words, rec_words) ;

  zpf = create_file_zmap(map_words, rec_words) ;                                     // STEP 1
  mmap_print(zpf, "zpf0") ;
  // only copy data map part fom zp0
  memcpy(&(zpf->fhead), &(zp0->fhead), map_words * sizeof(uint32_t)) ;               // STEP 2a 
  // now copy data part
  memcpy(zpf->mhead.drng.bot, zp0->mhead.drng.bot, RANGE_BYTES(zp0->mhead.drng)) ;   // STEP 2b
  update_file_zmap(zpf) ;                                                            // STEP 2c

  SET_CODEC_FN(zpf, demo_codec_8_32) ;                               // set pack/restore codec function address
  SET_CODEC_ARGS(zpf, ((codec_args){32, 8, 0}) ) ;                  // set pack/restore codec arguments
  SET_GET_FN(zpf, get_zmap_mem_blocks) ;                             // set get block function address
  SET_GET_ARGS(zpf, ((getput_memory_args){ ZMAP_DATA(zp0), 1, 0 }) ) ;      // set get block function argument
  zmap_print(zpf, "zpf1") ;

  bzero(restored, gni * gnj * sizeof(uint32_t)) ;              // set restored to 0
  get_data_from_zmap(zpf, gni, gnj, (void *)restored) ;              // get previously stored data
  errors = check_data(gni, gnj, (void *)restored, 0, gni, 0, gnj) ;
  if( errors != 0){ FAIL(1, "ERROR : %d error(s) in restored data\n", errors) ; }
  free_zmap(zpf) ;
  mmap_print(zpf, "zpf_null") ;  // just after free

if(argc < 1000) goto success ;  // avoid warning about unreachable code
//==========================================================================================================================
  for(uint32_t i = (zp0->fhead.zni * zp0->fhead.znj * zp0->fhead.gnk) ; i < zp0->fhead.zijk ; i++){
    zp0->size[i] = zp0->fhead.zijk - i ;                                  // fix supplementary block size
    zp0->mhead.orng.bot[i+1] = zp0->mhead.orng.bot[i] + (zp0->size[i] * sizeof(uint32_t)) ;    // fix supplementary block offset
  }
  if( errors != 0){ FAIL(1, "ERROR : %d error(s) in get/put\n", errors) ; }

  mmap_print(zp0, "zp0+") ;
  fmap_print(zp0, "zp0+") ;
  fprintf(stderr, "\n");

  total_blocks = ZMAP_TOTAL_BLOCKS(zp0) ;
  uint64_t saved_offset = zp0->mhead.orng.bot[total_blocks-1] ;
  uint16_t saved_size = zp0->size[total_blocks-1] ;
  zp0->mhead.orng.bot[total_blocks-1] = 999999*sizeof(uint32_t) ;    // excessive offset+size for last block
  zp0->size[total_blocks-1] = 65000 ;
  oor = print_zmap_blocks(zp0, 99) ;
//   if(oor != 1) FAIL(1, "block not in range count = %d, expecting 1\n", oor) ;
  if(oor != 1) fprintf(stderr, "block not in range count = %d, expecting 1\n", oor) ;
  fprintf(stderr, "\n");
  zp0->mhead.orng.bot[total_blocks-1] = saved_offset ;   // restore correct offset and size fot last block
  zp0->size[total_blocks-1] = saved_size ;
  oor = print_zmap_blocks(zp0, 99) ;
//   if(oor != 0) FAIL(1, "block not in range count = %d, expecting 0\n", oor) ;
  if(oor != 0) fprintf(stderr, "block not in range count = %d, expecting 0\n", oor) ;
  fprintf(stderr, "\n");

// TODO : adjust for offsets in bytes
  map_words = FILEMAP_WORDS(zp0) ; rec_words = RECORD_WORDS(zp0) ; zmap_words = ZMAP_WORDS(zp0) ; data_words = DATA_WORDS(zp0) ;
  fprintf(stderr, "zp0 data map length = %d , data length = %d , record length = %d , zmap length = %d \n", map_words, data_words, rec_words, zmap_words) ;
  fprintf(stderr, "zp0 reference : ARRAY_BLOCKS = %d, TOTAL_BLOCKS = %d\n", ZMAP_ARRAY_BLOCKS(zp0), ZMAP_TOTAL_BLOCKS(zp0)) ;

  fprintf(stderr, "    data map block sizes and offsets\n") ;
  for(uint32_t i=0 ; i<ZMAP_TOTAL_BLOCKS(zp0) ; i++){ fprintf(stderr, "%6d ", BLOCK_WORDS(zp0,i)) ; } ;
  fprintf(stderr, "\n");
  for(uint32_t i=0 ; i<ZMAP_TOTAL_BLOCKS(zp0)+1 ; i++){ fprintf(stderr, "%6ld ", BLOCK_OFFSET(zp0,i)/sizeof(uint32_t)) ; } ;
  fprintf(stderr, "\n");
  for(uint32_t i=0 ; i<ZMAP_TOTAL_BLOCKS(zp0) ; i++){ fprintf(stderr, "%6ld ", (BLOCK_OFFSET(zp0,i+1) - BLOCK_OFFSET(zp0,i))/sizeof(uint32_t) ) ; } ;
  fprintf(stderr, "\n");
  fprintf(stderr, "\n");
if(argc < 1000) goto success ;  // avoid warning about unreachable code
  map_words = FILEMAP_WORDS(zp0) ; rec_words = RECORD_WORDS(zp0) ; zmap_words = ZMAP_WORDS(zp0) ; data_words = DATA_WORDS(zp0) ;
  zp1 = create_file_zmap(map_words, rec_words) ;
  mmap_print(zp1, "zp1") ;
  map_words  = FILEMAP_WORDS(zp1) ;
  data_words = DATA_WORDS(zp1) ;
  rec_words  = RECORD_WORDS(zp1) ;
  zmap_words = ZMAP_WORDS(zp1) ;
  fprintf(stderr, "zp1 data map length = %d , data length = %d , record length = %d , zmap length = %d \n", map_words, data_words, rec_words, zmap_words) ;
  fprintf(stderr, "\n") ;
  memcpy(&(zp1->fhead), &(zp0->fhead), map_words * sizeof(uint32_t)) ;   // simulate read from file
  status = update_file_zmap(zp1) ;
  if(status) fprintf(stderr, "ERROR: update_file_zmap %d\n", status) ;
  zp1->mhead.codec = demo_codec_16_32 ;
//   zp1->mhead.get_blocks = move_mapped_blocks ;
  mmap_print(zp1, "zp1+") ;
  fmap_print(zp1, "zp1+") ;
  fprintf(stderr, "\n") ;

  map_words = FILEMAP_WORDS(zp0) ; rec_words = RECORD_WORDS(zp0) ; zmap_words = ZMAP_WORDS(zp0) ; data_words = DATA_WORDS(zp0) ;
  zp2 = create_file_zmap(map_words, 0) ;     // map only, no data
  mmap_print(zp2, "zp2") ;
  map_words  = FILEMAP_WORDS(zp2) ;
  data_words = DATA_WORDS(zp2) ;
  rec_words  = RECORD_WORDS(zp2) ;
  zmap_words = ZMAP_WORDS(zp2) ;
  fprintf(stderr, "zp2 data map length = %d , data length = %d , record length = %d , zmap length = %d \n", map_words, data_words, rec_words, zmap_words) ;
// //   fmap_print(zp2, "zp2") ;

if(argc < 1000) goto success ;  // avoid warning about unreachable code

  fprintf(stderr, "=============== syntax test ===============\n") ;

  sfn_args *sfn_t_args ;
  malloc_sfn_args(sfn_t_args, 20) ;
  if(sfn_t_args == NULL) goto fail ;
  if(sfn_t_args->maxargs != 20){
    fprintf(stderr, "ERROR : sfn_t_args->maxargs is %d, expected 20\n", sfn_t_args->maxargs) ;
    goto fail ;
  }
  fprintf(stderr, "SUCCESS\n") ;

  fprintf(stderr, "=============== block indexing ===============\n") ;

  int ln0, ln, l, i0, lb ;
  ln = 64 ;

  for(ln0=ln/2 ; ln0<2*ln ; ln0++){  // loop over block sizes
    i0 = -1 ; lb = ln0 ;
    if(ln0==ln/2 || ln0==ln || ln0==2*ln-1) {
      fprintf(stderr, "ln0 = %3d, ln = %3d, %4d values,", ln0, ln, ln0 + (NTI-1)*ln) ;
//       ijp = b_limits(0, ln, ln0) ;
      irange = index_limits(0, ln, ln0) ;
      fprintf(stderr, " first block [%4d,%4d] (size = %3d),", irange.ix0, irange.ixn, irange.ixn-irange.ix0+1) ;
    }
    for(j=0 ; j<NTI ; j++, lb=ln){   // loop over blocks
//       ijp = b_limits(j, ln, ln0) ;
      irange = index_limits(j, ln, ln0) ;
//       if(ijp.i != i0+1 || ijp.j != i0+lb){
      if(irange.ix0 != i0+1 || irange.ixn != i0+lb){
        fprintf(stderr, "ERROR: block %d limits, expected [%d,%d], got [%d,%d]\n", j, i0+1, i0+lb, irange.ix0, irange.ixn) ;
        goto fail ;
      }
      for(i=0 ; i<lb ; i++){
        i0++ ;
        l = block_index(i0, ln, ln0) ;
        if(l != j){
          fprintf(stderr, "ERROR: index = %d, ln0 = %d, ln = %d, expecting block %d, got %d\n", i0, ln0, ln, j, l) ;
          goto fail ;
        }
      }
    }
    if(ln0==ln/2 || ln0==ln || ln0==2*ln-1) {
      fprintf(stderr, " last block [%4d,%4d] (size = %d)\n", irange.ix0, irange.ixn, irange.ixn-irange.ix0+1) ;
    }
  }
  fprintf(stderr, "SUCCESS\n") ;

  goto success ;
}
