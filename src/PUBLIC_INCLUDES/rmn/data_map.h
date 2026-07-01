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
// =========== NO LONGER VALID, KEPT FOR HISTORICAL REASONS ===========
//
// data zblocks layout example (2D example)
//
// zblocks along i (x) : 10   (ZNI)
// zblocks along j (y) : 11   (ZNJ)
// stripe factor : 4        (SF0)
// top stripe factor        (SF1)  (may be smaller than SF0)
//
// the number (ZI) in the zblocks is the sequential position in the data map (Z index)
//
// SF1 = MODULO(ZNJ , SF0)
// if(SF1 == 0) then SF1 = SF0
// STJ = J / SF0                ( stripe number for row J )
// J0  = STJ * SF0              ( J index of lower row in stripe )
// if(J0 + SF0 > ZNJ) then SF = SF1 else SF = SF0    ( stripe factor for this row )
// ZI = (J0 * ZNI) + (J - J0) + (SF1 * I)            ( Z index of tile[I,J] )
//
// row (J)                                                           stripe
//     +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//     |     |     |     |     |     |     |     |     |     |     |
//  10 |  82 |  85 |  88 |  91 |  94 |  97 | 100 | 103 | 106 | 109 |
//     |     |     |     |     |     |     |     |     |     |     |
//     +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//     |     |     |     |     |     |     |     |     |     |     |
//   9 |  81 |  84 |  87 |  90 |  93 |  96 |  99 | 102 | 105 | 108 |
//     |     |     |     |     |     |     |     |     |     |     |
//     +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//     |     |     |     |     |     |     |     |     |     |     |
//   8 |  80 |  83 |  86 |  89 |  92 |  95 |  98 | 101 | 104 | 107 |
//     |     |     |     |     |     |     |     |     |     |     |  [2]
//     +=====+=====+=====+=====+=====+=====+=====+=====+=====+=====+=======
//     |     |     |     |     |     |     |     |     |     |     |
//   7 |  43 |  47 |  51 |  55 |  59 |  63 |  67 |  71 |  75 |  79 |
//     |     |     |     |     |     |     |     |     |     |     |
//     +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//     |     |     |     |     |     |     |     |     |     |     |
//   6 |  42 |  46 |  50 |  54 |  58 |  62 |  66 |  70 |  74 |  78 |
//     |     |     |     |     |     |     |     |     |     |     |
//     +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//     | ****************|     |     |     |     |     |     |     |
//   5 | *41 |  45 |  49*|  53 |  57 |  61 |  65 |  69 |  73 |  77 |
//     | *   |     |    *|     |     |     |     |     |     |     |
//     +-*---+-----+----*+-----+-----+-----+-----+-----+-----+-----+
//     | *   |     |    *|     |     |     |     |     |     |     |
//   4 | *40 |  44 |  48*|  52 |  56 |  60 |  64 |  68 |  72 |  76 |
//     | *   |     |    *|     |     |     |     |     |     |     |  [1]
//     +=*===+=====+====*+=====+=====+=====+=====+=====+=====+=====+=======
//     | *   |     |    *|     |     |     |     | ##########|     |
//   3 | * 3 |   7 |  11*|  15 |  19 |  23 |  27 | #31 |  35#|  39 |
//     | *   |     |    *|     |     |     |     | #   |    #|     |
//     +-*---+-----+----*+-----+-----+-----+-----+-#---+----#+-----+
//     | *   |     |    *| %%%%%%%%%%%%%%%%|     | #   |    #|     |
//   2 | * 2 |   6 |  10*| %14 |  18 |  22%|  26 | #30 |  34#|  38 |
//     | ****************| %   |     |    %|     | #   |    #|     |
//     +-----+-----+-----+-%---+-----+----%+-----+-#---+----#+-----+
//     |     |     |     | %   |     |    %|     | #   |    #|     |
//   1 |   1 |   5 |   9 | %13 |  17 |  21%|  25 | #29 |  33#|  37 |
//     |     |     |     | %%%%%%%%%%%%%%%%|     | #   |    #|     |
//     +-----+-----+-----+-----+-----+-----+-----+-#---+----#+-----+
//     |     |     |     |     |     |     |     | #   |    #|     |
//   0 |   0 |   4 |   8 |  12 |  16 |  20 |  24 | #28 |  32#|  36 |
//     |     |     |     |     |     |     |     | ##########|     |  [0]
//     +=====+=====+=====+=====+=====+=====+=====+=====+=====+=====+=======
//        0     1     2     3     4     5     6     7     8     9    column (I)
//
// stripe delimiter : '='
//
// * delimited region, 12 zblocks
// option 1 : ( probably slowest )
//   read zblocks 2->3, 6->7, 10->11, 40->41, 44->45, 48->49 [ 12 zblocks read, 6 IO requests ]
// option 2 :
//   read zblocks 2->11 and 40->49 [ 20 zblocks read, 2 IO requests ]
// option 3 : ( probably fastest)
//   read zblocks 2->49 [ 48 zblocks read, 1 IO request ]
//
// % delimited region, 6 zblocks
//  option 1 : ( probably slower )
//    read zblocks 13->14, 17->18, 21->22 [ 6 zblocks read, 3 IO requests ]
//  option 2 : ( probably fastest )
//    read zblocks 13->22 [ 10 zblocks read, 1 IO request ]
//
// # delimited region, 8 zblocks
//  option 1 : ( ideal case )
//    read zblocks 28->35 [ 8 zblocks read, 1 IO request ]
//
// the 3D extension is simple
// each block has dimensions lni|li0 x lnj|lj0 x lnk|lkx
// offset and size have dimensions zni x znj x znk
// there is no striping along z
//  compression may be 2D (lnk blocks lni x lnj) or 3D (1 block lni x lnj x lnk)
//
// =================================================================
//
#if ! defined(Z_DATA_MAP_VERSION)

// version 1.0.0
#define Z_DATA_MAP_VERSION  0x0100

#include <stdint.h>
#include <stdlib.h>

#include <rmn/ct_assert.h>
#include <rmn/split_dimension.h>
#include <rmn/mem_range.h>
// use big endian bit stream
#include <rmn/be_stream.h>

//   packed data representation (in record from file and in memory)
//
//   |--------------------------- in memory (fully populated zmap struct) -----------------------------------|
//   |----------- sizeof(zmap) ----------|              |-mextra -|
//   |- sizeof(mhead) -|- sizeof(fhead) -|-- 2*zijk ---||pad (0 or 2 bytes) (2 bytes pad if zijk is odd)
//   +-----------------+-----------------+-------------++---------+-------//-----------+//+-----//-----------+
//   |                 |                 |   block     ||  extra  |                    |  | offsets|pointers |
//   |  memory header  |  file header    |   sizes     ||  global |    data blocks     |  |     to blocks    |
//   |signature , .....|signature, ......|   [zijk]    ||  info   |   (zijk blocks)    |  |     [zijk + 1]   |
//   +-----------------+-----------------+-------------++---------+-------//-----------+//+-----//-----------+
//   |-------------------------------------------- ZRNG -----------------------------------------------------|
//                     |--------------- FRNG ---------------------|------- DRNG -------|  |----- ORNG -------|
//                                       |---- SRNG ---||--XRNG --|                       |----- PRNG -------|
//                     |---------------------- in file record -------------------------|
//                     |---------------- data map ----------------|--- encoded data ---|
//                                                      |---------- bitstream ---------|
//
// ZRNG : entire memory arena (mhead, FRNG, DRNG, optional ORNG
// FRNG : data map (includes table of block sizes and "extras")
// DRNG : encoded data blocks (32 bit elements)
// SRNG : table of block sizes (16 bit elements)
// XRNG : optional "extras" (null range if extra == 0) (32 bit elements)
// ORNG : optional table of offsets (null range if not present) (64 bit elements)
// PRNG : optional table of pointers (null range if not present) (64 bit elements)
// NOTE: there is a possible overlap between the bit stream and the data map (mextra 32 bit elements)
// NOTE: either ORNG or PRNG is present, they are mutually exclusive
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
//   block[i,0] : ( li,lj0)  (i > 0) (first row)
//   block[0,j] : (li0, lj)  (j > 0) (first column)
//   block[i,j] : ( li, lj)  (i > 0, j > 0)
// - all blocks have the same dimension 
//   block[i,j] : ( li, lj)
//
typedef uint32_t *zblocks ;         // zblocks[ix] is address of encoded data block[ix]
typedef uint16_t fmap_block_size ;  // size needed for fmap block sizes (normally 16 bits)
CT_ASSERT(sizeof(fmap_block_size) == (sizeof(fmap_block_size) / sizeof(int16_t)) * sizeof(int16_t) , "fmap_block_size not a multiple of 16 bits")

#define ZERO128 {{0l, 0l}}
typedef struct{ uint64_t arg[2] ; } arg128 ;    // 128 bit memory block
static const arg128 zero_128 = ZERO128 ;
typedef struct{ arg128 arg[2] ; } arg256 ;    // 256 bit memory block
#define ZERO256 {{ZERO128, ZERO128}}
static const arg256 zero_256 = ZERO256 ;

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

// function to encode/decode data
typedef int32_t codec_fn(zmap *map, void *out, void *in, int ninj, int encode) ;

// NOTE: components not needed/used are nullified
// TODO ? add file descriptor and file offset to beginning of record/data in file ?
//        for RSF : READ function and pointer to record struct as argument + offset, size ?
//        pointer to IO-READ function, blind args array for this function, (*fn)(args, file, offset, size) ?
//        define a read(map, file, offset, size) function ?
// orng and prng ranges are mutually exclusive, they share memory storage
// prng.bot[zijk] == NULL : use prng.bot[] pointers
// prng.bot[zijk] != NULL : use orng.bot[] offsets
// the base address for offsets is drng.bot[0] (start of data range)
struct mmap{            // in memory only part of data map
  uint32_t signature ;     // should be 0x1AD0FADA, target for & operator to get address of header
  uint16_t version ;       // version marker (MUST BE the same as in file header)
  uint16_t options ;       // reserved for internal use options (MUST BE 0 FOR NOW)
//   bitstream stream ;       // encoding/decoding bit stream  (see rmn/be_stream.h, rmn/bitstream.h) (should be 64 bytes)
  block_fn *get_blocks ;   // pointer to get block(s) function uint32_t_range (*get_block)(zmap *map, int block0, int block_nb, uint32_t_range drng)
  arg128 get_args ;        // for use by get_blocks function
  block_fn *put_blocks ;   // pointer to put block(s) function uint32_t_range (*put_block)(zmap *map, int block0, int block_nb, uint32_t_range drng)
  arg128 put_args ;        // for use by put_blocks function
  codec_fn *codec ;        // pointer to encode/decode function (*codec)(zmap *map, void *out, void *in, int ninj, int encode)
  arg128 codec_args ;      // for use by codec_fn
  RANGE(uint32_t) xrng ;   // address range for "extra" information
  RANGE(uint32_t) frng ;   // address range for the file portion of the data map (includes "extra" information)
  RANGE(uint16_t) srng ;   // address range for the sizes table (uint16_t items)
  RANGE(zmap_t)   drng ;   // address range for the data blocks portion of the record (above "extra")
  RANGE(uint64_t) orng ;   // orng.bot[zijk] : uint64_t block offset (in bytes) (relative to drng.bot or other) (optional)
  RANGE(zmap_tp)  prng ;   // prng.bot[index] : pointer to block[index]  (32 bit items) (optional)
  RANGE(uint32_t) zrng ;   // address range for the entire data map
  uint32_t esize ;         // original data element size (bytes)
  uint32_t reserved ;      // provision for future expansion (MUST BE 0 FOR NOW)
} ;
static const mmap base_mmap = { 0x1AD0FADA, Z_DATA_MAP_VERSION, 0 , NULL, ZERO128, NULL, ZERO128, NULL, ZERO128,
                               RANGE_NULL(uint32_t), RANGE_NULL(uint32_t), RANGE_NULL(uint16_t), RANGE_NULL(zmap_t),
                               RANGE_NULL(uint64_t), RANGE_NULL(zmap_tp), RANGE_NULL(uint32_t), 0, 0 } ;
static const mmap zero_mmap = { 0x00000000,                  0, 0 , NULL, ZERO128, NULL, ZERO128, NULL, ZERO128,
                               RANGE_NULL(uint32_t), RANGE_NULL(uint32_t), RANGE_NULL(uint16_t), RANGE_NULL(zmap_t),
                               RANGE_NULL(uint64_t), RANGE_NULL(zmap_tp), RANGE_NULL(uint32_t), 0, 0 } ;
CT_ASSERT(sizeof(mmap) == (sizeof(mmap) / sizeof(int64_t)) * sizeof(int64_t) , "mmap struc size not a multiple of 64 bits")
// #define NULLIFY_ZMAP(MAP) { (MAP)->mhead = base_mmap ; (MAP)->mhead.signature = 0 ; }

// codec function related macros
#define SET_CODEC_ARGS(MAP, ARGS) { (MAP)->mhead.codec_args = *(arg128 *)(&(ARGS)) ; }
#define SET_CODEC_FN(MAP, FN)     { (MAP)->mhead.codec = (codec_fn *)(FN) ; }
#define ZMAP_CODEC(MAP, OUT, IN, NIJ, ENCODE) ( (*((MAP)->mhead.codec))(MAP, OUT, IN, NIJ, ENCODE) )
// get block(s) related macros
#define SET_GET_ARGS(MAP, ARGS) { (MAP)->mhead.get_args = *(arg128 *)(&(ARGS)) ; }
#define SET_GET_FN(MAP, FN)     { (MAP)->mhead.get_blocks = (block_fn *)(FN) ; }
#define ZMAP_GET(MAP, BLOCK0, NBLKS, DRNG) ( (*((MAP)->mhead.get_blocks))(MAP, BLOCK0, NBLKS, (RANGE(zmap_t))DRNG) )
// put block(s) related macros
#define SET_PUT_ARGS(MAP, ARGS) (MAP)->mhead.put_args = *(arg128 *)(&(ARGS))
#define SET_PUT_FN(MAP, FN) (MAP)->mhead.put_blocks = (block_fn *)(FN)
#define ZMAP_PUT(MAP, BLOCK0, NBLKS, DRNG) ( (*((MAP)->mhead.get_blocks))(MAP, BLOCK0, NBLKS, (RANGE(zmap_t))DRNG) )

// TODO: add options for 3D storage ni/nj/nk vs nk/ni/nj vs ... and compression(2D/3D) ?
// TODO: store esize somewhere ? (reserved ?)
struct fmap{       // in file part of data map (also present in memory, after mmap)
    uint32_t signature ;   // should be 0xBEBEFADA, target for & operator to get address of header
    uint16_t version ;     // version marker (MUST BE the same as in memory header)
    uint16_t extra ;       // extra metadata size after block sizes table in 32 bit units (often 0)
    uint16_t esize ;       // original data element size (bytes)
    uint16_t reserved ;    // provision for future expansion (MUST BE 0 FOR NOW)
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

CT_ASSERT(sizeof(fmap) == (sizeof(fmap) / sizeof(int32_t)) * sizeof(int32_t) , "fmap struc size not a multiple of 32 bits")

// in memory data map struct
// the first part (mhead) is only present in memory
// the second part (fhead) is the file data map
struct zmap{
  // ---------------- in  memory header -----------------
  mmap mhead ;
  // ------------- start of in file record part ---------
    // --------------- start of data map  ---------------
    //               [ file record header ]
  fmap fhead ;
  // ----------------    sizes table     ----------------
  fmap_block_size size[] ;        // size (in 32 bit word units) of encoded data blocks ( size[znj*zni*gnk + zextra == zijk] )
    // if zijk is odd, there will be a padding element (padding to multiple of uint32_t length)
    // "extra" (may have 0 size)
  // ---------------  end of data map   ---------------
  // "data stream" (encoded bit stream) (zmap_t elements)
  // ---------------  end of file record --------------
// "pointer table" (never written into file)
} ;
CT_ASSERT(sizeof(zmap) == (sizeof(zmap) / sizeof(int32_t)) * sizeof(int32_t) , "zmap struc size not a multiple of 32 bits")

// WORDS refers to 32 bit words (4 bytes)
#define ZMAP_WORDS(MAP)    RANGE_ELEMENTS((MAP)->mhead.zrng)
#define FILEMAP_WORDS(MAP) RANGE_ELEMENTS((MAP)->mhead.frng)
#define DATA_WORDS(MAP)    RANGE_ELEMENTS((MAP)->mhead.drng)
#define RECORD_WORDS(MAP)  (FILEMAP_WORDS(MAP) + DATA_WORDS(MAP))
// block offset
#define BLOCK_OFFSET(MAP, BLOCK)   ((MAP)->mhead.orng.bot[(BLOCK)])
#define BLOCK_OFFSET32(MAP, BLOCK) ( BLOCK_OFFSET(MAP, BLOCK) / sizeof(uint32_t) )
// block size
#define BLOCK_WORDS(MAP, BLOCK) ((MAP)->size[(BLOCK)])
#define BLOCK_BYTES(MAP, BLOCK) ( BLOCK_WORDS(MAP, BLOCK) * sizeof(uint32_t) )
// 1D block index
#define BLOCK_IJ(MAP, I, J) ( I + J * ((MAP)->fhead.zni) )

#define ZMAP_TOTAL_BLOCKS(MAP) ((MAP)->fhead.zijk)
#define ZMAP_ARRAY_BLOCKS(MAP) (((MAP)->fhead.zni) * ((MAP)->fhead.znj) * ((MAP)->fhead.gnk))

// base address of encoded data blocks
#define ZMAP_DATA(MAP) ((MAP)->mhead.drng.bot)
//
#define ZMAP_OFFSETS(MAP) ((MAP)->mhead.orng.bot)

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

zmap *create_file_zmap(uint32_t map_words, uint32_t rec_words);
int update_file_zmap(zmap *map);

zmap *create_zmap(int32_t gni, int32_t gnj, int32_t gnk,
                  int32_t bi_size, int32_t aspect, int32_t esize,
                  int32_t mextra, int32_t zextra, int32_t zsize, int nodata);
int finalize_zmap(zmap *map);
int free_zmap(zmap *map);

int fmap_invalid(zmap *map);
void fmap_init(zmap *map, int32_t gni, int32_t gnj, int32_t gnk, int32_t bsizex, int32_t bsizey, array_axis_3d *a3, int32_t mextra, int32_t bextra);
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
