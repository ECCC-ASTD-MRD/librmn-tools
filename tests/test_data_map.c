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

#include <rmn/data_map.h>
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

#define MAX_PRINT_BLOCKS 16
#define HASH_BITS 8

// arguments for demo encoder/decoder
typedef struct{
    uint32_t unp ;        // original element size
    uint32_t pak ;        // packed element size
    uint64_t dummy ;      // not used for now
} test_codec_args;
CT_ASSERT(sizeof(test_codec_args) == CODEC_ARGS_SIZE, "sizeof(test_codec_args) != CODEC_ARGS_SIZE") ;

// arguments for get/put function with data in memory
typedef struct{
    uint32_t *data ;      // base memory address of encoded data
    uint32_t get_put ;    // 1 : get, 0 : put
    uint32_t dummy ;      // not used for now
} getput_memory_args ;
CT_ASSERT(sizeof(getput_memory_args) == GET_ARGS_SIZE, "sizeof(getput_memory_args) != GET_ARGS_SIZE") ;
typedef getput_memory_args get_memory_args ;
typedef getput_memory_args put_memory_args ;

// arguments for get/put function with data in ordinary file
typedef struct{
    size_t   offset ;     // base file address of encoded data
    uint32_t get_put ;    // 1 : get, 0 : put
    uint32_t fd ;         // file descriptor
} getput_file_args ;
CT_ASSERT(sizeof(getput_file_args) == PUT_ARGS_SIZE, "sizeof(getput_memory_args) != PUT_ARGS_SIZE") ;
typedef getput_file_args get_file_args ;
typedef getput_file_args put_file_args ;
//
// ================================= get zmap block(s) from memory =================================
//
// get contents of block[block0] from data stream (full data stream assumed to be in memory)
// map      [IN] : pointer to valid zmap struct
// block0   [IN] : block to copy
// drng     [IN] : if valid, transfer data into memory pointed to by drng (an adjusted drng will be returned)
//                 if not valid, a range pointing to the requested data will be returned
// return  a range pointing to the requested data block
// an invalid range will be returned in case of error
RANGE(zmap_t) get_zmap_memory_block(zmap *map, int block0, RANGE(zmap_t) drng){
  if(map == NULL) goto fail ;
  int max_bno = map->fhead.zijk ;
  if(block0 < 0 || block0 >= max_bno) goto fail ;

  get_memory_args *args ;
  uint64_t *offsets, offset ;
  uint32_t *base, size ;
  uint32_t dummy[1] ;

  offsets = map->mhead.orng.bot ;                                  // offsets table
  offset  = offsets[block0] ;                                      // block offset in bytes
  if(offset & 3) goto fail ;                                       // offset MUST BE a multiple of 4
  offset  = offset / sizeof(uint32_t) ;                            // block offset in 32 bit words
  size    = map->size[block0] ;                                    // block size in 32 bit words

  args = ZMAP_GET_ARGS(map) ;                                      // use get_args to retrieve base address of block
  if(args->get_put != 1) goto fail ;                               // NOT a GET call
  base = args->data ;                                              // base address of data
  if(base == NULL) base = ZMAP_DATA(map) ;                         // if NULL, use zmap data range bottom as base address
  base += offset ;                                                 // add data block offset

  if(VALID_RANGE(drng)){                                           // a valid destination was supplied

    if(RANGE_ELEMENTS(drng) < size) goto fail ;                    // OOPS, destination range is too small
    memcpy(RANGE_BOT(drng), base, size * sizeof(uint32_t)) ;       // copy into destination range drng
    SET_RANGE_ELEMENTS(drng, size) ;                               // adjust top of drng range

  }else{                                                           // no destination supplied, point to where data block is in memory
    drng = RANGE_KIND(zmap_t , base,  base+size) ;                 // range pointing to block data with correct size
  }

  return drng ;

fail:
  return (RANGE(zmap_t)){ dummy + 1, dummy } ;                       // invalid range, top < bot
}
//
// copy block(s) of 32 bit elements from memory pointed to by data map into user space
// demo function for tests purposes
// map      [IN] : pointer to valid zmap struct
// block0   [IN] : first block to copy
// block_nb [IN] : number of blocks to copy
// rng   [INOUT] : address range describing destination (32 bit elements)
// return number of words copied or negative error codes
//
block_fn get_zmap_mem_blocks ;  // check that get_zmap_mem_blocks prototype is compatible with block_fn
RANGE(zmap_t) get_zmap_mem_blocks(zmap *map, int block0, int block_nb, RANGE(zmap_t) rng){
  int status = -10 ;

  if(map == NULL) XIT(-1) ;                              // no map ;
  if(block0 < 0) XIT(-3) ;                               // invalid block number
  if(block_nb <= 0) XIT(-4) ;                            // invalid number of blocks
  int max_blocks = map->fhead.zijk ;
  if(block0+block_nb > max_blocks) XIT(-5) ;             // last block number exceeds available blocks

  if(block_nb == 1){                                     // special case : get 1 block
    rng = get_zmap_memory_block(map, block0, rng) ;      // get block from memory, rng does not need to be valid
  }else{
    if(! VALID_RANGE(rng)) XIT(-6) ;                   // range must be valid if multiple blocks are requested
    RANGE(zmap_t) rng0 = rng, rngt = rng ;
    // loop over block numbers, adjusting temporary rng on the fly
    for(int i = block0 ; i < block0+block_nb ; i++){
      rngt = get_zmap_memory_block(map, block0, rngt) ;  // get block from memory
      if(! VALID_RANGE(rngt)) XIT(-7) ;
      rngt.bot = rngt.top ;                              // bump bottom of temporary range to top of temporary
      rngt.top = rng0.top ;                              // set top of temporary range to original top
    }
    rng.top = rngt.top ;                                 // set top of rng to top of temporary range
    rng.bot = rng0.bot ;                                 // set bottom of rng to original value
  }
  return rng ;                                          // range of output data

  zmap_t dummy[1] ;
fail:
  return (RANGE(zmap_t)){ dummy - status, dummy } ;
}
//
// ================================= get zmap block(s) from file =================================
//
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

  get_file_args *args ;
  uint64_t *offsets, offset ;
  ssize_t size ;

  args = ZMAP_GET_ARGS(map) ;
  if(args->get_put != 1) goto fail ;                               // NOT a GET call

  offsets = map->mhead.orng.bot ;                                  // offsets table
  offset  = offsets[block0] ;                                      // block offset in bytes
  offset += args->offset ;                                         // add base offset in file
  size    = map->size[block0] * sizeof(uint32_t) ;                 // block size in bytes

  int fd = args->fd ;                                              // file descriptor
  if(VALID_RANGE(drng)){
    if(RANGE_BYTES(drng) < size) goto fail ;                       // OOPS, destination range is too small
    lseek(fd, offset, SEEK_SET) ;                                  // set file position
    ssize_t nc = read(fd, drng.bot, size) ;                        // read from file
    if(nc != size) goto fail ;                                     // short/failed read
    SET_RANGE_BYTES(drng, size) ;                                  // adjust top of drng range
  }else{
    goto fail ;
  }
  return drng ;                                                    // return adjusted range

  zmap_t dummy[1] ;
fail:
  return (RANGE(zmap_t)){ dummy + 1, dummy } ;
}

// get block(s) of 32 bit elements from a file
// demo function for tests purposes
// map      [IN] : pointer to valid zmap struct
// block0   [IN] : first block to copy
// block_nb [IN] : number of blocks to copy
// rng   [INOUT] : address range describing destination (32 bit elements)
// return number of words copied or negative error codes
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
    return get_zmap_file_block(map, block0, rng) ;       // get block from file
  }else{
    XIT(-10) ;                                           // block_nb > 1 not supported yet
  }
// will have to allocate space
  rng = RANGE_NULL(zmap_t) ;
  return rng ;

  zmap_t dummy[1] ;
fail:
  return (RANGE(zmap_t)){ dummy - status, dummy } ;
}
//
// ================================= pack / unpack / demo codec =================================
// encode unsigned 32 -> 16 ;
// map   [IN] : pointer to valid zmap struct
// out_ [OUT] : output (packed 32 -> 16)
// in_   [IN] : input data (unpacked)
// ninj  [IN] : number of values
// return number of bytes written into out_
static int demo_pack_block_32_16(zmap *map, void *out_, void *in_, int ninj){
  if(map == NULL || ninj <= 0) return -1 ;
  uint32_t *in  = in_ ;
  uint16_t *out = out_ ;
  if(out == NULL || in == NULL) return -1 ;
  test_codec_args *local = ZMAP_CODEC_ARGS(map) ;            // point local arguments into proper place in zmap
  CT_ASSERT( sizeof(*local) == sizeof(map->mhead.args_codec) , "bad codec arguments struc size" )
  if(local->unp != 32 || local->pak != 16) return -1 ;
  for(int i=0 ; i<ninj ; i++){ out[i] = in[i] & 0xFFFF ; } ;
  return ninj * sizeof(uint16_t) ;
}

// decode unsigned 16 -> 32 ;
// map   [IN] : pointer to valid zmap struct
// out_ [OUT] : output (restored 16 -> 32)
// in_   [IN] : input data (packed)
// ninj  [IN] : number of values
// return number of bytes read from in_
static int demo_unpack_block_16_32(zmap *map, void *out_, void *in_, int ninj){
  if(map == NULL || ninj <= 0) return -1 ;
  uint16_t *in = in_ ;
  uint32_t *out = out_ ;
  if(out == NULL || in == NULL) return -1 ;
  struct{
    uint32_t unp ;
    uint32_t pak ;
    uint64_t dummy ;
  } *local = ZMAP_CODEC_ARGS(map) ;            // point local arguments into proper place in zmap
  CT_ASSERT( sizeof(*local) == sizeof(map->mhead.args_codec) , "bad codec arguments struc size" )
  if(local->pak != 16 || local->unp != 32) return -1 ;
  for(int i=0 ; i<ninj ; i++){ out[i] = in[i] & 0xFFFF ; } ;
  return ninj * sizeof(uint16_t) ;
}

// 16 <-> 32 encode/decode codec
// map       [IN] : pointer to valid zmap struct
// stream [INOUT] : output (encode 32->16), input (decode 16->32)
// block  [INOUT] : input data (encode 32->16), output data (decode 16->32)
// encode    [IN] : != 0 : encode(pack), == 0 : decode(unpack)
// return number of packed bytes
codec_fn demo_codec_16_32 ;
int demo_codec_16_32(zmap *map, zmap_block block, zmap_stream stream, int encode){
  if(block.nk != 1 || block.esize != 4) return 0 ;
  if(encode){
    return demo_pack_block_32_16(map, stream, block.byt, block.ni * block.nj) ;
  }else{
    return demo_unpack_block_16_32(map, block.byt, stream, block.ni * block.nj) ;
  }
}

// encode unsigned 32 -> 8 ;
// map   [IN] : pointer to valid zmap struct
// out_ [OUT] : output (packed 32 -> 8)
// in_   [IN] : input data (unpacked)
// ninj  [IN] : number of values
// return number of bytes written into out_
static int demo_pack_block_32_8(zmap *map, void *out_, void *in_, int ninj){
  if(map == NULL || ninj <= 0) return -1 ;
  if(map->fhead.esize != 4) exit(1) ;    // unpacked element size MUST BE 4 BYTES
  uint32_t *in = in_ ;
  uint8_t *out = out_ ;
  if(out == NULL || in == NULL) return -1 ;
  test_codec_args *local = ZMAP_CODEC_ARGS(map) ;            // point local arguments into proper place in zmap
  CT_ASSERT( sizeof(*local) == sizeof(map->mhead.args_codec) , "bad codec arguments struc size" )
  if(local->unp != 32 || local->pak != 8) return -1 ;
  for(int i=0 ; i<ninj ; i++){ out[i] = in[i] & 0xFF ; } ;
  return ninj * sizeof(uint8_t) ;
}

// decode unsigned 8 -> 32 ;
// map   [IN] : pointer to valid zmap struct
// out_ [OUT] : output (restored 8 -> 32)
// in_   [IN] : input data (packed)
// ninj  [IN] : number of values
// return number of bytes read from in_
static int demo_unpack_block_8_32(zmap *map, void *out_, void *in_, int ninj){
  if(map == NULL || ninj <= 0) return -1 ;    // unpacked element size MUST BE 4 BYTES
  if(map->fhead.esize != 4) exit(1) ;
  uint8_t *in = in_ ;
  uint32_t *out = out_ ;
  if(out == NULL || in == NULL) return -1 ;
  struct{
    uint32_t unp ;
    uint32_t pak ;
    uint64_t dummy ;
  } *local = ZMAP_CODEC_ARGS(map) ;            // point local arguments into proper place in zmap
  CT_ASSERT( sizeof(*local) == sizeof(map->mhead.args_codec) , "bad codec arguments struc size" )
  if(local->pak != 8 || local->unp != 32) return -1 ;
  for(int i=0 ; i<ninj ; i++){ out[i] = in[i] & 0xFF ; } ;
  return ninj * sizeof(uint8_t) ;
}

// 8 <-> 32 encode/decode codec
// map       [IN] : pointer to valid zmap struct
// stream [INOUT] : output (encode 32->8), input (decode 8->32)
// block  [INOUT] : input data (encode 32->8), output data (decode 8->32)
// encode    [IN] : != 0 : encode(pack), == 0 : decode(unpack)
// return number of packed bytes
codec_fn demo_codec_8_32 ;
int demo_codec_8_32(zmap *map, zmap_block block, zmap_stream stream, int encode){
  if(block.nk != 1 || block.esize != 4) return 0 ;
  if(encode){
    return demo_pack_block_32_8(map, stream, block.byt, block.ni * block.nj) ;
  }else{
    return demo_unpack_block_8_32(map, block.byt, stream, block.ni * block.nj) ;
  }
}
//
// ================================= fill and verify data ==================================
//
// gni   [IN] : row length
// gnj   [IN] : number of rows
// data [OUT] : array to fill (unsigned integers)
// fill an array (gni x gnj) with known values
static void fill_data(int gni, int gnj, uint32_t data[gnj][gni]){
  for(int j=0 ; j<gnj ; j++){
    for(int i=0 ; i<gni ; i++){
      data[j][i] = HASH((i<<16)|j, HASH_BITS) ;
    }
  }
}

// gni   [IN] : row storage length
// data  [IN] : address of array[j0][i0]
// i0    [i0] : index of first point in row
// lni   [IN] : number of values to check along row
// j0    [IN] : index of first row
// lnj   [IN] : number of rows to check
// verify that array section contains expected values (see fill_data)
// return number of discrepancies
static uint32_t verify_hash(int gni, uint32_t data[][gni], int i0, int lni, int j0, int lnj){
  uint32_t errors = 0 ;
  for(int j=0 ; j<lnj ; j++){
    for(int i=0 ; i<lni ; i++){
      if( data[j][i] != HASH(((i+i0)<<16)|(j+j0), HASH_BITS) ) errors++ ;
    }
  }
  return errors ;
}

// check the contents of a part of an array[gnj][gni] with known values
// gni   [IN] : row length
// gnj   [IN] : number of rows
// data [OUT] : array to check (unsigned integers)
// i0    [i0] : index of first point to check in row
// lni   [IN] : number of values to check along row
// j0    [IN] : index of first row to check
// lnj   [IN] : number of rows to check
// return number of discrepancies
static uint32_t check_data(int gni, int gnj, uint32_t data[gnj][gni], int i0, int lni, int j0, int lnj){
  uint32_t errors = 0 ;
  errors += verify_hash(gni, (void *)&(data[j0][i0]), i0, lni, j0, lnj) ;
  return errors ;
}
//
// =========================================================================================
//
// copy block(s) of 32 bit elements from user space into memory pointed to by data map
// demo function for tests purposes
// map   [INOUT] : pointer to valid zmap struct
// block0   [IN] : first block to copy
// block_nb [IN] : number of blocks to copy
// drng     [IN] : address range describing source (32 bit elements)
//TODO : UPDATE AND DEBUG THIS FUNCTION
block_fn put_zmap_mem_blocks ;  // check that put_zmap_mem_blocks prototype is compatible with block_fn
RANGE(zmap_t)  put_zmap_mem_blocks(zmap *map, int block0, int block_nb, RANGE(zmap_t) drng){
  int status ;

  if(map == NULL) XIT(-1) ;                              // no map ;
  if(INVALID_RANGE(drng)) XIT(-2) ;                      // drng MUST be a valid range

  if(block0 < 0) XIT(-3) ;                               // invalid block number
  if(block_nb <= 0) XIT(-4) ;                            // invalid number of blocks
  int max_blocks = map->fhead.zijk ;
  if(block0+block_nb > max_blocks) XIT(-5) ;             // last block number exceeds available blocks

  size_t size = sizeof(zmap_t) * map->size[block0] ;     // size of data to copy
  uint8_t *stream = (uint8_t *)ZMAP_DATA(map) ;          // base address of map data
  stream += ZMAP_OFFSETS(map)[block0] ;                  // offset for this block
// fprintf(stderr, "put_zmap_mem_blocks, block %3d, %5ld words, size = %5d, offset = %6ld\n", block0, RANGE_ELEMENTS(drng), map->size[block0], ZMAP_OFFSETS(map)[block0]) ;
  if(block_nb == 1){                                     // special case : put 1 block
    memcpy(stream, drng.bot, size) ;                     // copy into data range of zmap
    return drng ;
//     return put_zmap_memory_block(map, block0, RANGE_NULL(zmap_t)) ;
    XIT(-10) ;                                           // NOT IMPLEMENTED YET
  }else{
    XIT(-10) ;                                           // block_nb > 1 not supported for the time being
  }

  zmap_t dummy[1] ;
fail:
  return (RANGE(zmap_t)){ dummy - status, dummy } ;
}
//
// =========================================================================================
//
uint8_t  _is_always_zerob_ = 0 ;                  // always 0, but the compiler cannot know that
// move data between sub arrays       source sub array -> destination sub array
//
// copy a byte block of dimension [nj][ni] from src[nj][sni] into dst[nj][dni]
// src[0:nj-1][0:sni-1] -> dst[0:nj-1][0:dni-1]
// ni MUST BE <= sni and <= dni
static inline void mov_byte_block(void * restrict dst_, uint32_t dni, void * restrict src_, uint32_t sni, uint32_t ni, uint32_t nj){
  uint8_t *src = (uint8_t *)src_, *dst = (uint8_t *)dst_ ;
  for(uint32_t j=0 ; j<nj ; j++){
    for(uint32_t i=0 ; i<ni ; i++){
      dst[i] = src[i] | _is_always_zerob_ ;    // prevents compiler from using memcpy function
    }
    dst += dni ;
    src += sni ;
  }
}
// static inline void mov_hword_block(void * restrict dst_, uint32_t dni, void * restrict src_, uint32_t sni, uint32_t ni, uint32_t nj){
//   mov_byte_block(dst_, 2*dni, src_, 2*sni, 2*ni, nj) ;
// }
// static inline void mov_word_block(void * restrict dst_, uint32_t dni, void * restrict src_, uint32_t sni, uint32_t ni, uint32_t nj){
//   mov_byte_block(dst_, 4*dni, src_, 4*sni, 4*ni, nj) ;
// }
// copy block[0:lnj-1][0:lni-1] into dst[0:lnj-1][0:lni-1]
// lni = block.ni, lnj = block.ni
// dst is dimensioned [>=lnj][gni], block is dimensioned [>=lnj][lni]
// array elements are 32 bit words
void store_zblock(zmap_block block,  uint32_t gni, void *dst){
  mov_byte_block(dst, 4*gni, block.byt, 4*block.ni, 4*block.ni, block.nj) ;
//   mov_word_block(dst, gni, block.byt, block.ni, block.ni, block.nj) ;
}
// copy src[0:lnj-1][0:lni-1] into block[0:lnj-1][0:lni-1], src is dimensioned [][gni]
// lni = block.ni, lnj = block.ni
// dst is dimensioned [>=lnj][gni], block is dimensioned [>=lnj][lni]
// array elements are 32 bit words
void fetch_zblock(zmap_block block,  uint32_t gni, void *src){
  mov_byte_block(block.byt, 4*block.ni, src, 4*gni, 4*block.ni, 4*block.nj) ;
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
  zmap_block zblk ;
  RANGE(zmap_t) r_temp ;
  fmap_block_size *sizes, size ;
  size_t total_size = 0 ;
  int nbytes, errors = 0 ;

  SET_BYTE_RANGE(r_temp, coded, sizeof(coded)) ;
  bno = 0 ;
  sizes = map->size ;
  fprintf(stderr, "DEBUG : ========== extracting %d array blocks (%d byte elements) from zmap ==========\n", zni*znj, esize) ;
  for(j=znj-1 ; j>=0 ; j--){
    index_range j_index = index_limits(j, map->fhead.lnj, map->fhead.lj0) ;
    j0 = j_index.ix0 ; jn = j_index.ixn ; lnj = jn - j0 + 1 ;

    for(i=zni-1 ; i>=0 ; i--){
      index_range i_index = index_limits(i, map->fhead.lni, map->fhead.li0) ;
      i0 = i_index.ix0 ; in = i_index.ixn ; lni = in - i0 + 1 ;
      bno = BLOCK_IJ(map,i,j) ;

      // some get functions may not have a need for r_temp and will just ignore it
      RANGE(zmap_t) r_coded = ZMAP_GET(map, bno, 1, r_temp) ;                          // use get block function from zmap to get encoded range
      zblk = ZMAP_BLOCK( block, lni, lnj, 1, uint_data, sizeof(uint32_t) ) ;           // describe 2D block to be decoded
      nbytes = ZMAP_DECODE(map, zblk, r_coded.bot) ;                                   // decode block of packed data (lni * lnj values)
      store_zblock(zblk,  gni, &array[j0][i0]) ;                                       // insert zblk[lnj][lni] into array[][gni]
      errors = verify_hash(lni, (void *)block, i0, lni, j0, lnj) ;                     // does decoded data match expected values ?

      size = (nbytes + sizeof(uint32_t) - 1) / sizeof(uint32_t) ;                      // round size up to multiple of sizeof(uint32_t)
      total_size += size ;
      if(nbytes == -1) exit(1) ;
      if(size != sizes[bno]) exit(1) ;
//       fprintf(stderr, "zblock[%d,%d]<%d> = array[%3d:%3d,%3d:%3d]", i, j, bno, i0, in, j0, jn) ;
//       fprintf(stderr, ", codec unpack : nb = %d", nbytes) ;
//       fprintf(stderr, ", sizes[%d] = %4d(%4d), offset = %6ld, data = %8.8x\n", bno, sizes[bno], siz0, map->mhead.orng.bot[bno], block[0]);
      if(errors != 0){
        fprintf(stderr, "ERROR at block [j=%d][i=%d], restored data not as expected\n", j, i) ;
        exit(1) ;
      }

    }
  }
  fprintf(stderr, "DEBUG : words extracted = %ld, expected = %ld\n\n", total_size, filewords) ;
}

// map    [INOUT] : pointer to valid zmap struct
// gni       [IN] : row length of array
// gnj       [IN] : number of rows in array
// array     [IN] : array to encode and store into zmap
// restored [OUT] : array to receive decode/move result for error checking purposes
void fill_zmap_with_data(zmap *map, int gni, int gnj, uint32_t array[gnj][gni], uint32_t restored[gnj][gni]){
  uint32_t lni, lnj, i0, j0, /*in, jn,*/ i, j ;
  uint32_t zni = map->fhead.zni, znj = map->fhead.znj ;
  uint32_t maxi = MAX(map->fhead.li0, map->fhead.lni) ;
  uint32_t maxj = MAX(map->fhead.lj0, map->fhead.lnj) ;
  uint32_t block[maxi*maxj] ;
  zmap_block zblk ;
  uint32_t *encoded, bno, *bot, *top ;
  int nbytes, nrestored, errors = 0 ;
  fmap_block_size *sizes, size ;
  uint64_t *offsets, offset ;
  int64_t inserted ;

  sizes = map->size ;
  offsets = map->mhead.orng.bot ;
  offsets[0] = 0 ;
  bno = 0 ;

  fprintf(stderr, "sizeof(block) = %ld bytes, %ld elements\n", sizeof(block), sizeof(block)/sizeof(uint32_t));
  bot    = PTR_CAST(map->mhead.drng.bot, uint32_t) ;   // bottom of data area
  top    = PTR_CAST(map->mhead.drng.top, uint32_t) ;   // top of data area
  encoded = bot ;
  for(j=0, j0 = 0, lnj = map->fhead.lj0 ; j<znj ; j++, j0+=lnj, lnj=map->fhead.lnj){
    for(i=0, i0 = 0, lni = map->fhead.li0 ; i<zni ; i++, i0+=lni, lni=map->fhead.lni){

      zblk = ZMAP_BLOCK( block, lni, lnj, 1, uint_data, sizeof(uint32_t) ) ;           // describe 2D block to be encoded
      fetch_zblock(zblk, gni, &array[j0][i0]) ;                                        // extract zblk[lnj][lni] from array[][gni]

      if((top - encoded) < (lni * lnj + 1)) exit(1) ;                                  // worst case encoding would fail

      nbytes = ZMAP_ENCODE(map, zblk, encoded) ;                                       // encode data
//       fprintf(stderr, ", codec   pack : nb = %d bytes", nbytes) ;
      if(nbytes == -1) exit(1) ;
      size = (nbytes + sizeof(uint32_t) - 1) / sizeof(uint32_t) ;                      // round size up to multiple of sizeof(uint32_t)
      sizes[bno] = size ;                                                              // size in 32 bit units
      offset = offsets[bno] / sizeof(uint32_t) ;                                       // offset in 32 bit units
      offsets[bno+1] = offsets[bno] + (size * sizeof(uint32_t)) ;                      // offset in bytes for next block

      zblk = ZMAP_BLOCK( block, lni, lnj, 1, uint_data, sizeof(uint32_t) ) ;           // describe 2D block to be decoded
      nrestored = ZMAP_DECODE(map, zblk, encoded) ;                                    // decode block into  restored
      if(nrestored != nbytes) exit(1) ;                                                // size mismatch or decoding failed
      errors = verify_hash(lni, (void *)block, i0, lni, j0, lnj) ;                     // does decoded data match original data
      store_zblock(zblk,  gni, &restored[j0][i0]) ;                                    // insert zblk[lnj][lni] into restored array[][gni]

//       fprintf(stderr, ", sizes[%d] = %4d Bytes (%4d), offset = %6ld, block[0][0] = %8.8x, check %s\n",
//               bno, sizes[bno]*4, nbytes, map->mhead.orng.bot[bno], block[0], errors ? "FAILED" : "O.K.");
      if(errors) exit(1) ;
      if(encoded != bot+offset) exit(1) ;

      encoded += size ;
      bno++ ;
    }
  }
  filewords = inserted = PTR_ELEMENTS(map->mhead.drng.bot, encoded) ;
  fprintf(stderr, "inserted %ld words (%ld bytes)\n", inserted, inserted*4) ;
  map->mhead.drng.top = encoded ;                         // adjust top of data range
  if( PTR(encoded) > PTR(offsets) ) exit(1) ;             // OUCH !! top of data range overlaps offset table
  while(PTR(encoded) < PTR(offsets)){                     // fill rest of data space with garbage
    *encoded = 0xF0F0F0F0 ;
    encoded++ ;
  }
}

// map  [INOUT] : pointer to valid zmap struct
// array   [IN] : array[gnk][gnj][gni][esize] to encode and store into zmap
// return number of 32 bit words generated in encoded stream
size_t fill_zmap_blocks(zmap *map, uint8_t *array){
  uint32_t lni, lnj, i0, j0, i, j ;
  uint32_t gni = map->fhead.gni ;
  uint32_t gnj = map->fhead.gnj ;
  uint32_t gnk = map->fhead.gnk ;
  uint32_t esize = map->fhead.esize ;
  uint32_t zni = map->fhead.zni, znj = map->fhead.znj ;
  uint32_t maxi = MAX(map->fhead.li0, map->fhead.lni) ;
  uint32_t maxj = MAX(map->fhead.lj0, map->fhead.lnj) ;
  uint8_t block[maxi*maxj*esize] ;
  uint8_t temp[maxi*maxj*esize + 32] ;      // size needed for worst case encoding
  zmap_stream estream = (zmap_stream)temp ;
  zmap_block zblk ;
  uint32_t *encoded, bno ;
  uint32_t *bot, *top ;
  fmap_block_size *sizes ;
  uint64_t *offsets, offset, index_i, index_j, g_row_size, l_row_size ;
  int nbytes ;
  uint32_t size ;
  RANGE(zmap_t) r_put ;
  getput_memory_args put_args = { (uint32_t *)temp, 0, 0 } ;

  SET_PUT_ARGS(map,put_args) ;

  i = j = 0 ;
  if(gnk != 1) goto fail ;                              // gnk != 1 not supported initially

  sizes = map->size ;                                   // block size table
  offsets = map->mhead.orng.bot ;                       // block offset table
  bno = 0 ;
  g_row_size = gni * esize ;

  offsets[0] = 0 ;

  bot     = PTR_CAST(map->mhead.drng.bot, uint32_t) ;   // bottom of data area
  top     = PTR_CAST(map->mhead.drng.top, uint32_t) ;   // top of data area
  encoded = bot ;
  for(j=0, j0 = 0, lnj = map->fhead.lj0 ; j<znj ; j++, j0+=lnj, lnj=map->fhead.lnj){
    if(j0 + lnj > gnj) goto fail ;       // overflow along second dimension
    index_j = j0 * g_row_size ;          // base of row j0

    for(i=0, i0 = 0, lni = map->fhead.li0, index_i = 0 ; i<zni ; i++, i0+=lni, lni=map->fhead.lni, index_i += l_row_size){
      if(i0 + lni > gni) goto fail ;        // overflow along first dimension
      index_i    = i0 * esize ;             // displacement for column i0
      l_row_size = lni * esize ;            // number of bytes to transfer along rows

      if((encoded + (lnj * l_row_size + 2)) > top) goto fail ;                          // worst case encoding would fail

      // get a block[lnj][lni][esize] from array[gnj][gni][esize] ( at position [j0][i0][0] )
      mov_byte_block((void *)block, l_row_size, (void *)(array + index_j + index_i), g_row_size, l_row_size, lnj) ;
      // encode block -> encoded, arguments to codec function from zmap->mhead.args_codec
      zblk = ZMAP_BLOCK( block, lni, lnj, 1, uint_data, sizeof(uint32_t) ) ;            // describe 2D block to be encoded
      nbytes = ZMAP_ENCODE(map, zblk, encoded) ;                                        // encode block
      nbytes = ZMAP_ENCODE(map, zblk, estream) ;
      if(nbytes <= 0) goto fail ;                                                       // encoding error(s)
      size = nbytes ;
      size = (size + sizeof(zmap_t) - 1) / sizeof(zmap_t) ;                             // round size up to multiple of sizeof(zmap_t)
      if(encoded + size > top) goto fail ;                                              // not enough space to accomodate encoded data

      sizes[bno] = size ;                                                               // size in 32 bit units
      offsets[bno+1] = offsets[bno] + (size * sizeof(zmap_t)) ;                         // offset in bytes for next block

      // copy encoded block into data map
      SET_BYTE_RANGE(r_put, estream, size * sizeof(zmap_t)) ;                           // buffer space used for encoding
      ZMAP_PUT(map, bno, 1, r_put) ;                                                    // store into zmap data area

      encoded += size ;                                                                 // bump encoded pointer
      offset = offsets[bno+1] / sizeof(zmap_t) ;                                        // offset in 32 bit units
      if(encoded != bot + offset) goto fail ;                                           // inconsistent offset
      bno++ ;                                                                           // bump block number
    }

  }

  filewords = encoded - bot ;
  fprintf(stderr, "inserted %ld words (%ld bytes)\n", filewords, filewords*4) ;
  return encoded - bot ;                   // number of 32 bit words generated

fail:
fprintf(stderr,"ERROR: fill_zmap_blocks, i=%d, j=%d\n",i,j);
  return 0 ;
}

// gni     [IN] : first dimension of array (row size)
// gnj     [IN] : second dimension of array (number of rows)
// gnk     [IN] : third dimension of array (number of planes)
// esize   [IN] : size in bytes of array elements (supported : 1/2/4/8/16)
// array   [IN] : array[gnk][gnj][gni] to encode into zmap block
// bi_size [IN] : blocking size along first dimension (i)
// aspect  [IN] : 2D aspect ratio (size along j = aspect * size along i) (1/2/3/4 supported, other values ignored)
// codec   [IN] : encoding function
// c_args  [IN] : arguments for encoding function
// mextra  [IN] : max size of extra global information for data decoding (in 32 bit units)
// zextra  [IN] : number of extra blocks (usually 0)
// zsize   [IN] : size needed (in bytes) for extra blocks (0 if zextra == 0)
// d_bytes [IN] : controls space to allocate for data (in bytes)
//                -1 : no space allocation for data, 0 : automatic allocation for worst case
zmap *create_and_fill_zmap(int32_t gni, int32_t gnj, int32_t gnk, int32_t esize, void *array,
                           int32_t bi_size, int32_t aspect, codec_fn *codec, codec_args c_args,
                           int32_t mextra, int32_t zextra, int32_t zsize, ssize_t d_bytes){
  zmap *map = NULL, *map0 = NULL ;

  if(esize != 4) FAIL(1, "ERROR : esize == %d not supported\n", esize) ;  // 4 byte items only for now
  if(gnk != 1) FAIL(1, "ERROR :gnk == %d not supported\n", gnk) ;         // gnk != 1 not supported yet
  // create map0 with enough space for data
  map0 = create_zmap(NULL, gni, gnj, gnk, bi_size, aspect, esize, mextra, zextra, zsize, d_bytes) ;
  // test reusable map feature
  map = create_zmap(map0, gni, gnj, gnk, bi_size, aspect, esize, mextra, zextra, zsize, d_bytes) ;
  if(map == NULL) FAIL(3, "ERROR : failed to allocate zmap struct\n") ;
  if(map != map0) FAIL(4, "ERROR : failed to reuse existing zmap struct\n") ;

  SET_CODEC_FN(map, codec) ;                               // set encode/restore codec function address
  SET_CODEC_ARGS(map, c_args) ;                            // set encode/restore codec arguments
  SET_PUT_FN(map,put_zmap_mem_blocks) ;
  fill_zmap_blocks(map, array) ;                           // fill zmap blocks with encoded data
  for(uint32_t i = (map->fhead.zni * map->fhead.znj * map->fhead.gnk) ; i < map->fhead.zijk ; i++){
    map->size[i] = map->fhead.zijk - i ;                   // fix supplementary blocks size
  }
  int status = finalize_zmap(map) ;                        // update offsets table
  if(status){
    free(map) ;
    map = NULL ;
    FAIL(4, "ERROR : finalize_zmap failed\n") ;
  }

end:
  return map ;

fail:
  if((map != NULL) && (map != map0)) free(map) ;
  map = NULL ;
  goto end ;
}


// create a populated 2 D data map
zmap *create_test_zmap_2d(int32_t gni, int32_t gnj, int32_t bsize, int32_t mextra, int32_t bextra){
  zmap *map ;
  int32_t aspect = 1, errors ;
  uint32_t data[gnj][gni] ;

  test_codec_args c_args = (test_codec_args){32, 8, 0} ;                       // 
  fill_data(gni, gnj, (void *)data) ;                                // fill and check reference array
  errors = check_data(gni, gnj, (void *)data, 0, gni, 0, gnj) ;
  if( errors != 0){ FAIL(1, "ERROR : %d error(s) creating reference data\n", errors) }
  fprintf(stderr, "reference data created and checked\n") ;
  map = create_and_fill_zmap(gni, gnj, 1, sizeof(uint32_t), data,
                             bsize, aspect, demo_codec_8_32, CODEC_ARGS(c_args),
                             mextra, bextra, 8*bextra*sizeof(uint32_t), 0) ;
  return map ;
#if 0
  uint32_t restored[gnj][gni] ;
  // create and populate the data_map + data struct (bextra supplementary blocks) 4 byte elements, allocate data
  map = create_zmap(gni, gnj, 1, bsize, aspect, 1*sizeof(uint32_t), mextra, bextra, 8*bextra*sizeof(uint32_t), 0) ;
  for(uint32_t i = (map->fhead.zni * map->fhead.znj * map->fhead.gnk) ; i < map->fhead.zijk ; i++){
    map->size[i] = map->fhead.zijk - i ;                                                       // fix supplementary blocks size
  }
  SET_CODEC_FN(map, demo_codec_8_32) ;                               // set pack/restore codec function address
  SET_CODEC_ARGS(map, ((test_codec_args){32, 8, 0}) ) ;                   // set pack/restore codec arguments

//   fill_data(gni, gnj, (void *)data) ;                                // fill and check reference array
//   errors = check_data(gni, gnj, (void *)data, 0, gni, 0, gnj) ;
//   if( errors != 0){ FAIL(1, "ERROR : %d error(s) creating reference data\n", errors) }
//   fprintf(stderr, "reference data created and checked\n") ;

  fill_zmap_with_data(map, gni, gnj, (void *)data, (void *)restored) ;    // fill zmap blocks with encoded data, decode to check
  errors = check_data(gni, gnj, (void *)restored, 0, gni, 0, gnj) ;       // check that restored while filling data is as expected
  if( errors != 0){ FAIL(1, "ERROR : %d error(s) filling zmap\n", errors) ; }
  fprintf(stderr, "zmap data filled and checked, ") ;
  for(uint32_t i = (map->fhead.zni * map->fhead.znj * map->fhead.gnk) ; i < map->fhead.zijk ; i++){
    map->size[i] = map->fhead.zijk - i ;                                                       // fix supplementary blocks size
  }
  ZMAP_OFFSETS(map)[0] = 0 ;                                              // offset for first block
end:
  fprintf(stderr, "FINALIZING zmap\n") ;
  if(finalize_zmap(map) != 0) exit(1) ;                                   // update offsets table

  return map ;
#endif
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
  int32_t gni, gnj, gnk, bsize, aspect, bsizej, oor = 0 ;
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
  // TODO : test with large gni and gnj
  gni = 787 ; gnj = 1025 ; gnk = 1 ; bsize = 64 ; aspect = 1 ; mextra = 2 ; bextra = 4 ;

  zpw = create_test_zmap_2d(gni, gnj, bsize, mextra, bextra) ;
  if(zpw == NULL) FAIL(1, "create_test_zmap_2d failed\n") ;
  mmap_print(zpw, "zpw") ;
  print_zmap_blocks(zpw, MAX_PRINT_BLOCKS) ;
  oor = zmap_blocks_out_of_range(zpw) ;
  if(oor != 0) FAIL(2, "%d blocks are out of range in zpw\n", oor) ;

// if(argc < 1000) goto success ;  // avoid warning about unreachable code

  fprintf(stderr, "=============== zmap in memory read test ===============\n") ;

  map_words = FILEMAP_WORDS(zpw) ;
  rec_words = RECORD_WORDS(zpw) ;
  zpr = create_file_zmap(NULL, map_words, rec_words) ;                               // STEP 1
  // test reusable map feature
  zmap *zpr2 = create_file_zmap(zpr, map_words, rec_words) ;
  if(zpr != zpr2){
    free(zpr2) ;
    FAIL(3, "create_file_zmap map reuse test failed\n") ;
  }else{
    fprintf(stderr, "SUCCESS: create_file_zmap map reuse test O.K.\n") ;
  }
  // only copy data map part fom zp0
  memcpy(&(zpr->fhead), &(zpw->fhead), map_words * sizeof(uint32_t)) ;               // STEP 2a 
  // now copy data part
  memcpy(zpr->mhead.drng.bot, zpw->mhead.drng.bot, RANGE_BYTES(zpw->mhead.drng)) ;   // STEP 2b
  update_file_zmap(zpr) ;                                                            // STEP 2c

  // test validity checker for potential codec arguments
  test_codec_args codec_args_1 = (test_codec_args){32, 8, 0} ;
  struct{ int dummy[6] ; } maybe_codec_args ;
  if( ! MAYBE_CODEC_ARGS(codec_args_1) ) goto fail ;
  fprintf(stderr, "SUCCESS : codec_args_1 is %s as codec arguments\n", MAYBE_CODEC_ARGS(codec_args_1) ? "VALID" : "INVALID") ;
  if(MAYBE_CODEC_ARGS(maybe_codec_args)) goto fail ;
  fprintf(stderr, "SUCCESS : maybe_codec_args is %s as codec arguments\n", MAYBE_CODEC_ARGS(maybe_codec_args) ? "VALID" : "INVALID") ;
  codec_args codec_args_2 = CODEC_ARGS(codec_args_1) ;
  codec_args codec_args_3 = CODEC_ARGS(maybe_codec_args) ;
  if (! bcmp( &codec_args_2, &CODEC_ARGS_NULL, sizeof(codec_args) ) ) goto fail ;  // check that zero_128 was not returned
  if (  bcmp( &codec_args_3, &CODEC_ARGS_NULL, sizeof(codec_args) ) ) goto fail ;

  SET_CODEC_FN(zpr, demo_codec_8_32) ;                                      // set pack/restore codec function address
  SET_CODEC_ARGS(zpr, codec_args_1) ;                                       // set pack/restore codec arguments

  // base address will be start of drng(data)range in zmap zpr (NULL base address)
  getput_memory_args get_args_1 = (getput_memory_args){ NULL, 1, 0 } ;      // zmap data address, get, dummy
  struct{ int dummy[6] ; } maybe_get_args ;
  if( ! MAYBE_GET_ARGS(get_args_1) ) goto fail ;
  fprintf(stderr, "SUCCESS : get_args_1 is %s as get argument\n", MAYBE_GET_ARGS(get_args_1) ? "VALID" : "INVALID") ;
  if(   MAYBE_GET_ARGS(maybe_get_args) ) goto fail ;
  fprintf(stderr, "SUCCESS : maybe_get_args is %s as codec arguments\n", MAYBE_GET_ARGS(maybe_get_args) ? "VALID" : "INVALID") ;
  get_args get_args_2 = GET_ARGS(get_args_1) ;
  get_args get_args_3 = GET_ARGS(maybe_get_args) ;
  if (! bcmp( &get_args_2, &GET_ARGS_NULL, sizeof(get_args) ) ) goto fail ;  // check that zero_128 was not returned
  if (  bcmp( &get_args_3, &GET_ARGS_NULL, sizeof(get_args) ) ) goto fail ;

  SET_GET_FN(zpr, get_zmap_mem_blocks) ;                                    // set get block function address
  SET_GET_ARGS(zpr, get_args_1) ;      // set get block function argument

  // base address will be start of drng(data)range in zmap zpr
  getput_memory_args put_args_1 = (getput_memory_args){ ZMAP_DATA(zpr), 0, 0 } ;  // address, get, dummy
  struct{ int dummy[6] ; } maybe_put_args ;
  if( ! MAYBE_PUT_ARGS(put_args_1) ) goto fail ;
  fprintf(stderr, "SUCCESS : put_args_1 is %s as put argument\n", MAYBE_PUT_ARGS(put_args_1) ? "VALID" : "INVALID") ;
  if(   MAYBE_PUT_ARGS(maybe_put_args) ) goto fail ;
  fprintf(stderr, "SUCCESS : maybe_put_args is %s as codec arguments\n", MAYBE_PUT_ARGS(maybe_put_args) ? "VALID" : "INVALID") ;
  put_args put_args_2 = PUT_ARGS(put_args_1) ;
  put_args put_args_3 = PUT_ARGS(maybe_put_args) ;
  if (! bcmp( &put_args_2, &PUT_ARGS_NULL, sizeof(put_args) ) ) goto fail ;  // check that zero_128 was not returned
  if (  bcmp( &put_args_3, &PUT_ARGS_NULL, sizeof(put_args) ) ) goto fail ;

  SET_PUT_ARGS(zpr, put_args_1) ;      // set put block function argument

  mmap_print(zpr, "zpr") ;
  print_zmap_blocks(zpr, MAX_PRINT_BLOCKS) ;

  // extract data blocks
  restored = (uint32_t *)malloc(gni*gnj*sizeof(uint32_t)) ;
  if(restored == NULL) FAIL(4, "ERROR : allocate restored array failed") ;
  bzero(restored, gni * gnj * sizeof(uint32_t)) ;                    // set restored to 0
  get_data_from_zmap(zpr, gni, gnj, (void *)restored) ;
  errors = check_data(gni, gnj, (void *)restored, 0, gni, 0, gnj) ;
  if( errors != 0){ FAIL(5, "ERROR : %d error(s) in restored data\n", errors) ; }
  oor = zmap_blocks_out_of_range(zpr) ;
  if(oor != 0) FAIL(6, "%d blocks are out of range in zpr\n", oor) ;
  fprintf(stderr, "restored array with %d errors, and %d blocks out of range\n", errors, oor) ;

//   free(restored) ;
  free_zmap(zpr) ;

// if(argc < 1000) goto success ;  // avoid warning about unreachable code

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

  zpf = create_file_zmap(NULL, map_words, map_words) ;                                     // STEP 1
  mmap_print(zpf, "zpf") ;
  fd = open("/tmp/zmap_file", O_RDONLY) ;
  if(fd < 0) exit(1) ;
  if( read(fd, &map_words, sizeof(uint32_t))    != sizeof(uint32_t)) exit(1) ;       // step 2a
  if(map_words != FILEMAP_WORDS(zpw)) exit(1) ;
  if( read(fd, &rec_words, sizeof(uint32_t))    != sizeof(uint32_t)) exit(1) ;       // step 2b
  if(rec_words != RECORD_WORDS(zpw)) exit(1) ;
  displacement = 0 ;
  if( read(fd, &displacement, sizeof(uint64_t)) != sizeof(uint64_t)) exit(1) ;       // step 2c
  if(displacement != 32768*32) exit(1) ;
  lseek(fd, displacement, SEEK_SET) ;
  read(fd, &(zpf->fhead), map_words * sizeof(uint32_t)) ;                            // step 2d    read data map
  displacement += (map_words * sizeof(uint32_t)) ;                                   // offset for data
  update_file_zmap(zpf) ;                                                            // STEP 2c

  SET_CODEC_FN(zpf, demo_codec_8_32) ;                                               // set pack/restore codec function address
  SET_CODEC_ARGS(zpf, ((test_codec_args){32, 8, 0}) ) ;                              // set pack/restore codec arguments
  SET_GET_FN(zpf, get_zmap_file_blocks) ;                                            // set get block function
  SET_GET_ARGS(zpf, ((getput_file_args){ displacement, 1, fd }) ) ;                  // and its arguments
  mmap_print(zpf, "zpf") ;
  print_zmap_blocks(zpf, MAX_PRINT_BLOCKS) ;

  bzero(restored, gni * gnj * sizeof(uint32_t)) ;                    // set restored to 0
  get_data_from_zmap(zpf, gni, gnj, (void *)restored) ;
  errors = check_data(gni, gnj, (void *)restored, 0, gni, 0, gnj) ;
  if( errors != 0){ FAIL(7, "ERROR : %d error(s) in restored data\n", errors) ; }

  fprintf(stderr, "%d errors in  restored data\n", errors) ;
  if(zmap_blocks_out_of_range(zpf) >= 0) { FAIL(8, "ERROR : out of range detection returned %d, expected -1 \n", oor) ; }

  close(fd) ;
  if( unlink(file_name) ) { FAIL(9, "ERROR : failed to delete file '%s'\n", file_name) ; }
  fprintf(stderr, "successfully removed file '%s'\n", file_name) ;
  free_zmap(zpf) ;
  fprintf(stderr, "freed data map zpf\n") ;

if(argc < 1000) goto success ;  // avoid warning about unreachable code

// =========================== aspect ratio test ===========================
  for(aspect = 1 ; aspect < 4 ; aspect++){
//     if(aspect < 3) continue ;

    fprintf(stderr, "=============== block aspect ratio = %d ===============\n", aspect) ;
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

    zmap *zp = create_file_zmap(NULL, nwords+mextra, nwords+mextra+100) ;         // mextra, 100 words of data
    if(fmap_invalid(zp) == 0) goto fail ;           // fmap is invalid at this point
    fmap_init(zp, gni, gnj, gnk, bsize, bsizej, NULL, mextra, bextra);   // initialize fmap part with bextra extra blocks
    zp->fhead.extra = mextra ;                                    // set extra to mextra words
    fmap_print(zp, "zp") ;
    if(fmap_invalid(zp) != 0) goto fail ;           // fmap must be valid at this point
    fprintf(stderr, "    fmap element size = %ld\n", ELEMENT_SIZE(zp->mhead.frng)) ;
    fprintf(stderr, "    filemap words = %d, zmap at %p, fmap at %p, blocks[%d:%d]\n",
            filemap_words(zp), &(zp->mhead.signature), &(zp->fhead.signature), zp->fhead.zni, zp->fhead.znj) ;
    fprintf(stderr, "\n");
    mmap_print(zp, "created file map zp") ;
    fprintf(stderr, "\n");
    update_file_zmap(zp);
    mmap_print(zp, "updated file map zp") ;
    fprintf(stderr, "-----------\n");

    free(zp) ;
    zp = create_zmap(NULL, gni, gnj, gnk, bsize, aspect, 2*sizeof(uint32_t), mextra, bextra, 3*bextra*sizeof(uint32_t), 0) ;
    fprintf(stderr, "data map length = %ld words, record length = %ld words\n", FILEMAP_WORDS(zp), RECORD_WORDS(zp)) ;
    fprintf(stderr, "\n");
    free(zp) ;
    zp = create_zmap(NULL, gni, gnj, gnk, bsize, aspect, 2*sizeof(uint32_t), mextra, 0, 3*bextra*sizeof(uint32_t), 0) ;
    fprintf(stderr, "data map length = %ld words, record length = %ld words\n", FILEMAP_WORDS(zp), RECORD_WORDS(zp)) ;
    fprintf(stderr, "\n");
    free(zp) ;
    zp = create_zmap(NULL, gni, gnj, gnk, bsize, aspect, 2*sizeof(uint32_t), mextra, 0, 0, 0) ;
    fprintf(stderr, "\n");
    free(zp) ;
  }

if(argc < 1000) goto success ;  // avoid warning about unreachable code

  fprintf(stderr, "=============== zmap in memory test ===============\n") ;
//   zmap *zp0, *zp1, *zp2, *zpw ;
// 
//   int status ;
//   uint32_t map_words, rec_words, zmap_words, data_words, errors ; 
  gni = 129 ; gnj = 97 ; gnk = 1 ; bsize = 64 ; aspect = 1 ; mextra = 2 ; bextra = 4 ;

//   zpw = create_test_zmap_2d(gni, gnj, bsize, mextra, bextra) ;
//   mmap_print(zpw, "zpw") ;
//   print_zmap_blocks(zpw, MAX_PRINT_BLOCKS) ;
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
  zp0 = create_zmap(NULL, gni, gnj, gnk, bsize, aspect, 2*sizeof(uint32_t), mextra, bextra, 8*bextra*sizeof(uint32_t), 0) ;
  for(uint32_t i = (zp0->fhead.zni * zp0->fhead.znj * zp0->fhead.gnk) ; i < zp0->fhead.zijk ; i++){
    zp0->size[i] = zp0->fhead.zijk - i ;                                                       // fix supplementary blocks size
  }
  SET_CODEC_FN(zp0, demo_codec_8_32) ;                                    // set pack/restore codec function address
  SET_CODEC_ARGS(zp0, ((test_codec_args){32, 8, 0}) ) ;                   // set pack/restore codec arguments
  SET_GET_FN(zp0, get_zmap_mem_blocks) ;                                  // set get block function address
  SET_GET_ARGS(zp0, ((getput_memory_args){ ZMAP_DATA(zp0), 1, 0 }) ) ;    // set get block function argument

  fill_zmap_with_data(zp0, gni, gnj, (void *)data, (void *)restored) ;    // fill zmap blocks with encoded data
  errors = check_data(gni, gnj, (void *)restored, 0, gni, 0, gnj) ;       // check that restored while filling data is as expected
  if( errors != 0){ FAIL(1, "ERROR : %d error(s) in put\n", errors) ; }
  if(finalize_zmap(zp0) != 0) exit(1) ;
  mmap_print(zp0, "zp0") ;
  print_zmap_blocks(zp0, MAX_PRINT_BLOCKS) ;

  // extract data blocks
  bzero(restored, gni * gnj * sizeof(uint32_t)) ;                   // set restored to 0
  get_data_from_zmap(zp0, gni, gnj, (void *)restored) ;
  errors = check_data(gni, gnj, (void *)restored, 0, gni, 0, gnj) ;
  if( errors != 0){ FAIL(1, "ERROR : %d error(s) in restored data\n", errors) ; }

  fprintf(stderr, "=============== zmap in pseudo file test ===============\n") ;

  map_words = FILEMAP_WORDS(zp0) ;
  rec_words = RECORD_WORDS(zp0) ;
  fprintf(stderr, "data map length = %d words, record length = %d words\n", map_words, rec_words) ;

  zpf = create_file_zmap(NULL, map_words, rec_words) ;                                     // STEP 1
  mmap_print(zpf, "zpf0") ;
  // only copy data map part fom zp0
  memcpy(&(zpf->fhead), &(zp0->fhead), map_words * sizeof(uint32_t)) ;               // STEP 2a 
  // now copy data part
  memcpy(zpf->mhead.drng.bot, zp0->mhead.drng.bot, RANGE_BYTES(zp0->mhead.drng)) ;   // STEP 2b
  update_file_zmap(zpf) ;                                                            // STEP 2c

  SET_CODEC_FN(zpf, demo_codec_8_32) ;                               // set pack/restore codec function address
  SET_CODEC_ARGS(zpf, ((test_codec_args){32, 8, 0}) ) ;                  // set pack/restore codec arguments
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
  zp1 = create_file_zmap(NULL, map_words, rec_words) ;
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
  zp2 = create_file_zmap(NULL, map_words, 0) ;     // map only, no data
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
