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
#if ! defined(Z_DATA_MAP_VERSION)

// version 1.0.0
#define Z_DATA_MAP_VERSION  0x0100

#include <stdint.h>
#include <stdlib.h>

#include <rmn/ct_assert.h>
#include <rmn/data_kind.h>
#include <rmn/split_dimension.h>
#include <rmn/mem_range.h>
//
//   encoded data representation (in record from file and in memory)
//
//   |--------------------------- in memory (fully populated zmap struct) -----------------------------------|
//   |----------- sizeof(zmap) ----------|              |-mextra -|
//   |- sizeof(mhead) -|- sizeof(fhead) -|-- 2*zijk ---||pad (0 or 2 bytes) (2 bytes pad if zijk is odd)
//   +-----------------+-----------------+-------------++---------+-------//-----------+//+-----//-----------+
//   |                 |                 |   block     ||  extra  |      encoded       |  | offsets|pointers |
//   |  memory header  |  file header    |   sizes     ||  global |    data blocks     |  |    to blocks     |
//   |signature , .....|signature, ......|   [zijk]    ||  info   |   (zijk blocks)    |  |    [zijk + 1]    |
//   +-----------------+-----------------+-------------++---------+-------//-----------+//+-----//-----------+
//   |-------------------------------------------- ZRNG -----------------------------------------------------|
//                     |--------------- FRNG ---------------------|------- DRNG -------|  |----- ORNG -------|
//                                       |---- SRNG ---||--XRNG --|                       |----- PRNG -------|
//                     |---------------- data map ----------------|--- encoded data ---|
//                                                      |---------- bitstream ---------|
//                     |-------------------- in "file" record -------------------------|
//
// ZRNG : entire memory arena (mhead, FRNG, DRNG, optional ORNG
// FRNG : data map (includes table of block sizes and "extras")
// DRNG : encoded data blocks (32 bit elements)
// SRNG : table of block sizes (16 bit elements)
// XRNG : optional "extras" (null range if extra == 0) (32 bit elements)
// ORNG : optional table of offsets (null range if not present) (64 bit elements)
// PRNG : optional table of pointers (null range if not present) (64 bit elements)
// NOTE: there is a potential overlap between the bit stream and the data map (mextra 32 bit elements)
// NOTE: either ORNG or PRNG can be present, they are mutually exclusive
//
// the data map can be mapped directly to the beginning of the packed data representation if necessary
// zmap *map
// void *pointer = &(map->fhead)
// read(fd, pointer, stream_size)
// write(fd, pointer, stream_size)
// zijk = number of blocks,   (rounded up to even number)
// extra : size of extra global info in 32 bit words (may be 0)
// zmap->size[zi]        : size of block with index zi
// zmap->offset = malloc(zijk * sizeof(uint64_t))
//    table of data block addresses, starting after "extra"
//    data_ptr = data_map + sizeof(zmap) + zmap->znijk * sizeof(int16_t) + mextra * sizeof(int32_t)
//    zmap->offset[0] = data_ptr, zmap->offset[i] = zmap->offset[i-1] + zmap->size[i-1]
//    zmap->offset[ix] is the address of block with index ix
//    zmap->size[ix] is the size of block with index ix
//
// either
// - the first block along a dimension is larger or smaller than the other blocks
//   block[0,0] : (li0,lj0)          (first block of first row)
//   block[i,0] : (lni,lj0)  (i > 0) (first row)
//   block[0,j] : (li0,lnj)  (j > 0) (first column)
//   block[i,j] : (lni,lnj)  (i > 0, j > 0)
// - all blocks have the same dimension 
//   block[i,j] : (lni,lnj)
//
typedef uint32_t *zblocks ;         // zblocks[ix] is address of encoded data block[ix]
typedef uint16_t fmap_block_size ;  // size needed for fmap block sizes (normally 16 bits)
CT_ASSERT(sizeof(fmap_block_size) == (sizeof(fmap_block_size) / sizeof(int16_t)) * sizeof(int16_t) , "fmap_block_size not a multiple of 16 bits")

#define ZERO128 {{0l, 0l}}
typedef struct{ uint64_t arg[2] ; } arg128 ;    // 128 bit memory block
static const arg128 zero_128 = ZERO128 ;
#define CODEC_ARGS_NULL zero_128
#define GET_ARGS_NULL zero_128
#define PUT_ARGS_NULL zero_128

// typedef struct{ arg128 arg[2] ; } arg256 ;      // 256 bit memory block
// #define ZERO256 {{ZERO128, ZERO128}}
// static const arg256 zero_256 = ZERO256 ;

typedef struct zmap zmap ;     // mmap + fmap [+ optional data] + offsets table
typedef struct mmap mmap ;     // in memory only preamble
typedef struct fmap fmap ;     // data map from file record

// zmap_t is the expected data type for encoded data blocks (normally 32 bits)
typedef uint32_t zmap_t ;
typedef uint32_t_range RANGE(zmap_t) ;    // shorthand for data range
typedef zmap_t *zmap_tp ;                 // pointer to zmap_t
RANGE_TYPEDEF(zmap_tp) ;                  // associated range

// function to get/put a block of encoded data
typedef RANGE(zmap_t) block_fn(zmap *map, int block0, int block_nb, RANGE(zmap_t) drng) ;

// compact block[nk][nj][ni] containing data to encode
typedef struct{
  union{
    void    *mem ;                  // generic pointer
    uint8_t *byt ;                  // address of block (byte address)
  } ;
  uint16_t ni ;                     // first dimension
  uint16_t nj ;                     // second dimension (1 if block is 1D)
  uint16_t nk ;                     // third dimension (1 if block is 1D or 2D)
  uint8_t  etype ;                  // data element type, see rmn/data_kind.h
  uint8_t  esize ;                  // element size in bytes -1  (1 <= element size <= 256)
}zmap_block ;
CT_ASSERT(sizeof(zmap_block) == 2*sizeof(uint64_t), "zmap_block struct not 128 bits")
#define ZMAP_BLOCK(PTR, NI, NJ, NK, ETYPE, ESIZE) (zmap_block){ { (void *)PTR }, NI, NJ, NK, ETYPE, ESIZE }

typedef uint32_t *zmap_stream ;     // pointer to a stream of 32 bit unsigned words (encoded data)
typedef int32_t codec_fn(zmap *map, zmap_block block, zmap_stream stream, int encode) ;

// NOTE: components not needed/used are nullified
// NOTE: ordinary file : descriptor and offset to beginning of record/data in file ?
//       RSF file : READ function and pointer to record struct as argument + offset, size ?
// orng and prng ranges are mutually exclusive, they share memory storage
// prng.bot[zijk] == NULL : use prng.bot[] pointers ?
// prng.bot[zijk] != NULL : use orng.bot[] offsets ?
// the base address for offsets is drng.bot[0] (start of data range)
//
// codec arguments type
typedef arg128 codec_args ;
// get function arguments type
typedef arg128 get_args ;
// put function arguments type
typedef arg128 put_args ;
struct mmap{                  // in memory only part of data map
  uint32_t signature ;        // should be 0x1AD0FADA, target for & operator to get address of header
  uint16_t version ;          // version marker (MUST BE the same as in file header)
  uint16_t options ;          // reserved for internal use options (MUST BE 0 FOR NOW)
  block_fn *get_blocks ;      // pointer to get block(s) function uint32_t_range (*get_block)(zmap *, int, int, uint32_t_range)
  get_args args_get ;         // for use by get_blocks function
  block_fn *put_blocks ;      // pointer to put block(s) function uint32_t_range (*put_block)(zmap *, int, int, uint32_t_range)
  put_args args_put ;         // for use by put_blocks function
  codec_fn *codec ;           // pointer to encode/decode function (*codec)(zmap *map, void *out, void *in, int ninj, int encode)
  codec_args args_codec ;     // for use by codec_fn
  RANGE(uint32_t) xrng ;      // address range for "extra" information
  RANGE(uint32_t) frng ;      // address range for the file portion of the data map (includes "extra" information)
  RANGE(uint16_t) srng ;      // address range for the sizes table (uint16_t items)
  RANGE(zmap_t)   drng ;      // address range for the data blocks portion of the record (above "extra")
  RANGE(uint64_t) orng ;      // orng.bot[zijk] : uint64_t block offset (in bytes) (relative to drng.bot or other) (optional)
  RANGE(zmap_tp)  prng ;      // prng.bot[index] : pointer to block[index]  (32 bit items) (optional)
  RANGE(uint32_t) zrng ;      // address range for the entire data map
  uint32_t esize ;            // original data element size (bytes) (block esize is 8 bits wide)
  uint32_t reserved ;         // provision for future expansion (MUST BE 0 FOR NOW)
} ;
static const mmap base_mmap = { 0x1AD0FADA, Z_DATA_MAP_VERSION, 0 , NULL, ZERO128, NULL, ZERO128, NULL, ZERO128,
                               RANGE_NULL(uint32_t), RANGE_NULL(uint32_t), RANGE_NULL(uint16_t), RANGE_NULL(zmap_t),
                               RANGE_NULL(uint64_t), RANGE_NULL(zmap_tp), RANGE_NULL(uint32_t), 0, 0 } ;
static const mmap zero_mmap = { 0x00000000,                  0, 0 , NULL, ZERO128, NULL, ZERO128, NULL, ZERO128,
                               RANGE_NULL(uint32_t), RANGE_NULL(uint32_t), RANGE_NULL(uint16_t), RANGE_NULL(zmap_t),
                               RANGE_NULL(uint64_t), RANGE_NULL(zmap_tp), RANGE_NULL(uint32_t), 0, 0 } ;
CT_ASSERT(sizeof(mmap) == (sizeof(mmap) / sizeof(int64_t)) * sizeof(int64_t) , "mmap struc size not a multiple of 64 bits")

// base address of extra information
#define ZMAP_EXTRAS(MAP) ((MAP)->mhead.xrng.bot)

// base address of encoded data blocks
#define ZMAP_DATA(MAP) ((MAP)->mhead.drng.bot)

// base address of block sizes table
#define ZMAP_SIZES(MAP) ((MAP)->mhead.srng.bot)

// base address of offsets table
#define ZMAP_OFFSETS(MAP) ((MAP)->mhead.orng.bot)
// base address of block pointers table
#define ZMAP_POINTERS(MAP) ((MAP)->mhead.prng.bot)
// NOTE: offsets and pointers CANNOT BOTH BE VALID
// #define NULLIFY_ZMAP(MAP) { (MAP)->mhead = base_mmap ; (MAP)->mhead.signature = 0 ; }

// ========== codec function related macros/function(s) ==========
// size of codec arguments block
#define CODEC_ARGS_SIZE sizeof(codec_args)

// if ARGS has the right size, return its address, otherwise return NULL
#define CODEC_ARGS_PTR(ARGS) ( (sizeof(ARGS) == CODEC_ARGS_SIZE) ? (&(ARGS)) : NULL )
// is ARGS a potentially valid codec_args struct
#define MAYBE_CODEC_ARGS(ARGS) ( CODEC_ARGS_PTR(ARGS) != NULL )

// transform an address into a pointer to a codec_args struct
static inline codec_args *codec_args_address(void *args){ codec_args *tmp = (codec_args *)args ; return tmp ; } ;
#define CODEC_ARGS_ADDRESS(ARGS) codec_args_address( &(ARGS) )

// get codec arguments struct as a 128 bit value from generic pointer
static inline codec_args codec_args_value(void *args) { codec_args *tmp = (codec_args *)args ; return *tmp ; }
// return ARGS as a codec_args struct if size is right, CODEC_ARGS_NULL otherwise
#define CODEC_ARGS(ARGS) ( MAYBE_CODEC_ARGS(ARGS) ? codec_args_value( &(ARGS) ) : CODEC_ARGS_NULL )

// return pointer to codec arguments from zmap
#define ZMAP_CODEC_ARGS(MAP) ((void *)(&(MAP->mhead.args_codec)))

// insert codec arguments into zmap
#define SET_CODEC_ARGS(MAP, ARGS) { (MAP)->mhead.args_codec = CODEC_ARGS(ARGS) ; }

// insert codec funtion address into zmap
#define SET_CODEC_FN(MAP, FN)     { (MAP)->mhead.codec = (codec_fn *)(FN) ; }

// call codec from zmap
#define ZMAP_CODEC(MAP, TILE, BLOCK, ENCODE) ( (*((MAP)->mhead.codec))(MAP, TILE, BLOCK, ENCODE) )
// simpler encoder call
#define ZMAP_ENCODE(MAP, TILE, BLOCK) ZMAP_CODEC(MAP, TILE, BLOCK, 1)
// simpler decoder call
#define ZMAP_DECODE(MAP, TILE, BLOCK) ZMAP_CODEC(MAP, TILE, BLOCK, 0)

// ========== get block(s) related macros/function(s) ==========
// size of get function arguments block
#define GET_ARGS_SIZE sizeof(get_args)

// if ARGS has the right size, return its address, otherwise return NULL
#define GET_ARGS_PTR(ARGS) ( (sizeof(ARGS) == GET_ARGS_SIZE) ? (&(ARGS)) : NULL )

// is ARGS a potentially valid get_args struct
#define MAYBE_GET_ARGS(ARGS) ( GET_ARGS_PTR(ARGS) != NULL )

// return an address as a pointer to a get_args struct
static inline get_args *get_args_address_(void *args){ get_args *tmp = (get_args *)args ; return tmp ; } ;
#define GET_ARGS_ADDRESS(ARGS) get_args_address_( &(ARGS) )

// return get arguments struct as a 128 bit value from generic pointer
static inline get_args get_args_value(void *args) { get_args *tmp = (get_args *)args ; return *tmp ; }

// return ARGS as a get_args struct if size is right, GET_ARGS_NULL otherwise
#define GET_ARGS(ARGS) ( MAYBE_GET_ARGS(ARGS) ? get_args_value( &(ARGS) ) : GET_ARGS_NULL )

// return pointer to get arguments from zmap
#define ZMAP_GET_ARGS(MAP) ((void *)(&(MAP->mhead.args_get)))

// insert get function arguments into zmap
#define SET_GET_ARGS(MAP, ARGS) { (MAP)->mhead.args_get = GET_ARGS(ARGS) ; }

// insert address of get block function into zmap
#define SET_GET_FN(MAP, FN)     { (MAP)->mhead.get_blocks = (block_fn *)(FN) ; }

// store encoded data block(s) into range DRNG using zmap
#define ZMAP_GET(MAP, BLOCK0, NBLKS, DRNG) ( (*((MAP)->mhead.get_blocks))(MAP, BLOCK0, NBLKS, (RANGE(zmap_t))DRNG) )

// ========== put block(s) related macros/function(s) ==========
// size of put function arguments block
#define PUT_ARGS_SIZE sizeof(put_args)

// if ARGS has the right size, return its address, otherwise return NULL
#define PUT_ARGS_PTR(ARGS) ( (sizeof(ARGS) == PUT_ARGS_SIZE) ? (&(ARGS)) : NULL )

// is ARGS a potentially valid put_args struct
#define MAYBE_PUT_ARGS(ARGS) ( PUT_ARGS_PTR(ARGS) != NULL )

// transform an address into a pointer to a put_args struct
static inline put_args *put_args_address(void *args){ put_args *tmp = (put_args *)args ; return tmp ; } ;
#define PUT_ARGS_ADDRESS(ARGS) put_args_address( &(ARGS) )

// return put arguments struct as a 128 bit value from generic pointer
static inline put_args put_args_value(void *args) { put_args *tmp = (put_args *)args ; return *tmp ; }

// return ARGS as a put_args struct if size is right, PUT_ARGS_NULL otherwise
#define PUT_ARGS(ARGS) ( MAYBE_PUT_ARGS(ARGS) ? put_args_value( &(ARGS) ) : PUT_ARGS_NULL )

// insert put function arguments into zmap
#define SET_PUT_ARGS(MAP, ARGS) (MAP)->mhead.args_put = PUT_ARGS(ARGS)

// insert address of put block function into zmap (encoded blocks)
#define SET_PUT_FN(MAP, FN) (MAP)->mhead.put_blocks = (block_fn *)(FN)

// store encoded data block(s) from range DRNG according to zmap
#define ZMAP_PUT(MAP, BLOCK0, NBLKS, DRNG) ( (*((MAP)->mhead.put_blocks))(MAP, BLOCK0, NBLKS, (RANGE(zmap_t))DRNG) )

// TODO: add options for 3D storage ni/nj/nk vs nk/ni/nj vs ... and compression(2D/3D) ?
struct fmap{       // in file part of data map (also present in memory, after mmap)
    uint32_t signature ;   // should be 0xBEBEFADA, target for & operator to get address of header
    uint16_t version ;     // version marker (MUST BE the same as in memory header)
    uint16_t extra ;       // extra metadata size after block sizes table in 32 bit units (often 0)
    uint16_t esize ;       // original data element size (bytes) (block esize is 8 bits wide)
    // TODO : add encoding type marker MISSING(S)/ENCODING_STYLE/... ?
    uint16_t reserved ;    // provision for future options (MUST BE 0 FOR NOW)
    uint32_t zijk ;        // total number of blocks (may be 0 if no map, or >= zni * znj * gnk if there are extra blocks)
    int32_t  gni ;         // first dimension of data array   = li0 + (zni - 1) * lni (row size)
    int32_t  gnj ;         // second dimension of data array  = lj0 + (znj - 1) * lnj (column size)
    int32_t  gnk ;         // third dimension of data array ( 1 for 2D data) (number of data planes)
    int16_t  zni ;         // number of blocks in a row
    int16_t  li0 ;         // first dimension of the first block(s) in first (leftmost) column
    int16_t  lni ;         // first dimension of all but first (leftmost) block in row(s) (number of values)
    int16_t  znj ;         // number of block rows
    int16_t  lj0 ;         // second dimension of block(s) in the first (bottom) row
    int16_t  lnj ;         // second dimension of all but first (bottom) block in column(s) (number of values)
} ;
static const fmap null_fmap = {          0,                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0} ;
static const fmap base_fmap = { 0xBEBEFADA, Z_DATA_MAP_VERSION, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0} ;

// in memory data map struct
// the first part (mhead) is only present in memory
// the second part (fhead) is the "file" data map
struct zmap{
  // ---------------- in  memory header -----------------
  mmap mhead ;
  // ------------- start of in file record part ---------
    // --------------- start of data map  ---------------
    //               [ file record header ]
  fmap fhead ;
  // ----------------    sizes table     ----------------
  fmap_block_size size[] ;        // size (in 32 bit word units) of encoded data blocks ( size[znj*zni*gnk + zextra == zijk] )
                                  // if zijk is odd, there will be a padding element (size table length must be a multiple of uint32_t size)
  // "extra" (may have 0 size)
  // ---------------  end of data map   ---------------
  // "data stream" (encoded bit stream) (zmap_t elements)
  // ---------------  end of file record --------------
// "pointer table" (never written into file)
} ;

// check alignment and sizes
CT_ASSERT(sizeof(fmap) == (sizeof(fmap) / sizeof(int64_t)) * sizeof(int64_t) , "fmap struc size not a multiple of 64 bits")
CT_ASSERT(sizeof(zmap) == (sizeof(zmap) / sizeof(int64_t)) * sizeof(int64_t) , "zmap struc size not a multiple of 64 bits")
CT_ASSERT(sizeof(zmap) == (sizeof(mmap) + sizeof(fmap)) , "zmap struc size not sizeof(mmap) + sizeof(fmap))")

// WORDS refers to 32 bit words (4 bytes)
// size in words of zmap
#define ZMAP_WORDS(MAP)    RANGE_ELEMENTS((MAP)->mhead.zrng)
// size in bytes of zmap
#define ZMAP_BYTES(MAP)    RANGE_BYTES((MAP)->mhead.zrng)
// size in words of data map in file
#define FILEMAP_WORDS(MAP) RANGE_ELEMENTS((MAP)->mhead.frng)
// size in words of data portion
#define DATA_WORDS(MAP)    RANGE_ELEMENTS((MAP)->mhead.drng)
// size in words of data map and data
#define RECORD_WORDS(MAP)  (FILEMAP_WORDS(MAP) + DATA_WORDS(MAP))
// get offset of data block (bytes)
#define BLOCK_OFFSET(MAP, BLOCK)   ((MAP)->mhead.orng.bot[(BLOCK)])
// get offset of data block (32 bit words)
#define BLOCK_OFFSET32(MAP, BLOCK) ( BLOCK_OFFSET(MAP, BLOCK) / sizeof(uint32_t) )
// get size of data block (32 bit words)
#define BLOCK_WORDS(MAP, BLOCK) ((MAP)->size[(BLOCK)])
// get size of data block (bytes)
#define BLOCK_BYTES(MAP, BLOCK) ( BLOCK_WORDS(MAP, BLOCK) * sizeof(uint32_t) )
// get 1D block index from 2 D block coordinates
#define BLOCK_IJ(MAP, I, J) ( I + J * ((MAP)->fhead.zni) )
// get number of blocks in zmap
#define ZMAP_TOTAL_BLOCKS(MAP) ((MAP)->fhead.zijk)
// get number of array related blocks in zmap
#define ZMAP_ARRAY_BLOCKS(MAP) (((MAP)->fhead.zni) * ((MAP)->fhead.znj) * ((MAP)->fhead.gnk))

static inline int invalid_zmap(zmap *map){
  if(map->mhead.signature != 0x1AD0FADA || map->fhead.signature != 0xBEBEFADA) return 1 ;
  if(map->mhead.version != Z_DATA_MAP_VERSION || map->fhead.version != Z_DATA_MAP_VERSION) return 1 ;
  return 0 ;
}

typedef struct{
  uint16_t nb ;         // number of blocks
  uint8_t  n0 ;         // size of first block
  uint8_t  n1 ;         // size of folloring blocks
} mapdim ;
CT_ASSERT(sizeof(mapdim) == 4, "unexpected size of mapdim structure")

typedef struct{
  uint8_t  version:6 ,  // version marker
           rank:2 ;     // number of dimensions (0 == NO MAP)
  uint8_t  xdim:2 ,     // dimension ext applies to (0 == NONE)
           spare:6 ;
  uint16_t ext ;        // upper 16 bits of dimension indicated by xdim
} map0 ;
CT_ASSERT(sizeof(map0) == 4, "unexpected size of map0 structure")

typedef struct{
  map0     map ;        // present for all maps
  mapdim   dim[] ;      // dimensions of block map ( 1/2/3 dimensions )
} bmap ;

typedef struct{
  map0     map ;        // present for all maps
  mapdim   dim[1] ;     // dimensions of block map
} bmap1 ;               // 1D block map
CT_ASSERT(sizeof(bmap1) == 8, "unexpected size of bmap1 structure")

typedef struct{
  map0     map ;        // present for all maps
  mapdim   dim[2] ;     // dimensions of block map
} bmap2 ;               // 2D block map
CT_ASSERT(sizeof(bmap2) == 12, "unexpected size of bmap2 structure")

typedef struct{
  map0     map ;        // present for all maps
  mapdim   dim[3] ;     // dimensions of block map
} bmap3 ;               // 3D block map
CT_ASSERT(sizeof(bmap3) == 16, "unexpected size of bmap3 structure")
#if 0
int32_t Zindex_from_ij(int32_t i, int32_t j, int32_t nti, int32_t ntj, int32_t sf0);
index_pair Zindex_to_ij(int32_t zij, int32_t nti, int32_t ntj, int32_t sf0);
int32_t  Z_map_index(zmap *map, int32_t i, int32_t j);
#endif
index_pair  map_block_position(zmap *map, int32_t i, int32_t j);
ij_range map_block_limits(zmap *map, int32_t i, int32_t j);

zmap *create_file_zmap(zmap *map, uint32_t map_words, uint32_t rec_words);
int update_file_zmap(zmap *map);

zmap *create_zmap(zmap *map, int32_t gni, int32_t gnj, int32_t gnk,
                  int32_t bi_size, int32_t aspect, int32_t esize,
                  int32_t mextra, int32_t zextra, int32_t zsize, ssize_t d_bytes);
int finalize_zmap(zmap *map);
int free_zmap(zmap *map);

int fmap_invalid(zmap *map);
int fmap_init(zmap *map, int32_t gni, int32_t gnj, int32_t gnk, int32_t bsizex, int32_t bsizey, array_axis_3d *a3, int32_t mextra, int32_t bextra);
void zmap_print(zmap *map, char *msg);
void mmap_print(zmap *map, char *msg);
void fmap_print(zmap *map, char *msg);

int zmap_blocks_out_of_range(zmap *map);
int print_zmap_blocks(zmap *map, uint32_t maxblocks);

void *filemap_address(zmap *map);
uint32_t filemap_words(zmap *map);

uint32_t filemap_blocks(int32_t gni, int32_t gnj, int32_t gnk, int32_t bsizex, int32_t bsizey);
int contiguous_zmap_blocks(zmap *map, int block0, int block_n);

size_t filemap_needed_words(int32_t gni, int32_t gnj, int32_t gnk, int32_t bsizex, int32_t bsizey, int32_t bextra);
size_t filemap_needed_bytes(int32_t gni, int32_t gnj, int32_t gnk, int32_t bsizex, int32_t bsizey, int32_t bextra);
size_t zmap_needed_bytes(int32_t gni, int32_t gnj, int32_t gnk, int32_t bsizex, int32_t bsizey, int32_t bextra);

// uint64_t *mem_zmap(zmap *map, uint32_t *data, size_t size);
// int bsize_zmap(zmap *map, size_t esize);
// int fillmem_zmap(zmap *map);
// ssize_t repack_map(zmap *map);
// ssize_t resize_map(zmap *map);
// void print_zmap(zmap *map, char *msg);

static inline int zmap_index_invalid(zmap *map, int index){
  int zijk = map->fhead.zijk ;
  return (index < 0 || index >= zijk) ;
}

// need block_properties definition
#include <rmn/data_properties.h>

// generic argument list
typedef struct{
  uint32_t maxargs ;    // max number of arguments
  uint32_t nargs ;      // number of arguments
  iuf64_t  args[] ;     // arguments ( [0] .. [nargs-1] )
} function_args ;       // function argument list

// allocate a generic argument list with room for at most nmax arguments
#define malloc_fn_args(arglist, nmax) { arglist = (function_args *) malloc(sizeof(function_args) + nmax * sizeof(iuf64_t)) ; arglist->maxargs = nmax ; }

// sfn argument list
typedef function_args sfn_args ;

// pointer to sfn function
typedef int (*sfn_ptr)(int lni, int ni, int nj, block_properties *bp, void *data, sfn_args *args) ;   // pointer to sfn processing function

// allocate an sfn argument list with room for at most nmax arguments
#define malloc_sfn_args(arglist, nmax) { malloc_fn_args(arglist, nmax) ; }

#endif
