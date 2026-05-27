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
// mem and size have dimensions zni x znj x znk
// there is no striping along z
//  compression may be 2D (lnk blocks lni x lnj) or 3D (1 block lni x lnj x lnk)
#if ! defined(Z_DATA_MAP_VERSION)

// version 1.0.0
#define Z_DATA_MAP_VERSION  0x0100

#include <stdint.h>
#include <stdlib.h>

#include <rmn/ct_assert.h>
#include <rmn/split_dimension.h>

// packed data representation (as in buffer read from file or in memory)
//
//   <----------------------------------- in memory ----------------------------------->
//   <----------- sizeof(zmap) ---------->              <-mextra ->
//   <- sizeof(mhead) ->                 <- 2*zni*znj ->
//                     <- sizeof(fhead) ->             /pad (0/2 bytes)/ (2 bytes pad if zni*znj is odd)
//   +-----------------+-----------------+-------------++---------+--------------------+
//   |                 |                 |   size      ||  extra  |                    |
//   |  memory header  |  file header    |   table     ||  global | encoded bit stream |
//   |signature        |                 |   [znij]    ||  info   |                    |
//   +-----------------+-----------------+-------------++---------+--------------------+
//                     |                 |             ||
//                     |signature, ......|size[]       || data bit stream
//                     <-------------------------- in file ---------------------------->
//
// data map can be mapped directly to the beginning of the packed data representation
// uint8_t buffer[buffer_size]
// zmap *data_map = (zmap *) buffer ;
// read(fd, buffer, buffer_size)
// zij = zmap->zni * zmap->znj : number of blocks
// znij = zij rounded up to even number
// extra : size of extra global info in 32 bit words (may be 0)
// zmap->size[zi]        : size of block with zigzag index zi
// zmap->mem = malloc(zij * sizeof(void *))
//    table of data block addresses, starting at data_ptr
//    data_ptr = data_map + sizeof(zmap) + zmap->zni * zmap->znj * sizeof(int16_t)
//               zmap->zni * zmap->znj rounded to multiple of 32 bits
//    zmap->mem[0] = data_ptr, zmap->mem[i] = zmap->mem[i-1] + zmap->size[i-1]
//    zmap->mem[zi] is the address of block with zigzag index zi
//
// (li0,lj0) may differ from(li,lj)  (half size to size and a half - 1)
//   li/2 <= li0 < li + li/2
//   lj/2 <= lj0 < lj + lj/2
// either
// - the first block along a dimension is larger or smaller
//   block[0,0] : (li0,lj0)          (first block of first row)
//   block[i,0] : ( li,lj0)  (i > 0) (first row)
//   block[0,j] : (li0, lj)  (j > 0) (first column)
//   block[i,j] : ( li, lj)  (i > 0, j > 0)
// - all blocks have the same dimension 
//   block[i,j] : ( li, lj)
//
typedef uint32_t *zblocks ;         // zblocks[zi] is address of encoded data block[ zindex(i,j) ]
typedef uint16_t fmap_block_size ;  // size needed for fmap block sizes

// NOTE: first, last, limit, extra are not NULL only when needed/used
typedef struct{            // in memory only part of data map
    uint32_t signature ;   // should be 0x1AD0FADA, target for & operator to get address of header
    uint16_t version ;     // version marker (MUST BE the same as in file header)
    uint16_t  flags ;      // reserved for internal use flags
    zblocks  *mem ;        // table[zni*znj] : memory addresses of encoded blocks in memory
    uint32_t *first ;      // start of the encoded bit stream
    uint32_t *last ;       // one past the end of the encoded bit stream
    uint8_t  *limit ;      // one past the end of the allocated space for the encoded bit stream
    uint32_t *extra ;      // reserved, points to extra information
    uint32_t *fmapend ;    // one past end of fmap struct part ( ( map->fmapend - &(map->fhead) ) = fmap struct size )
    uint32_t *zmapend ;    // one past end of zmap struct ( (map->zmapend - &map) = zmap struct size )
} mmap ;
static const mmap null_mmap = { 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL } ;

CT_ASSERT(sizeof(mmap) == (sizeof(mmap) / sizeof(int32_t)) * sizeof(int32_t) , "mmap struc size not a multiple of 32 bits")

// TODO: add flags for 3D storage ni/nj/nk vs nk/ni/nj vs ... and compression(2D/3D)
// TODO: add flags for Z ordering algorithm kind (Morton order, stripes, ...)
// TODO: finalize what is needed and what is not needed
// NOTE : signature, version, stripe, ztype, flags can probably be moved out of fmap.
//        leaving in fmap only the spatial decomposition
typedef struct{            // in file part of data map (also present in memory, after mmap)
    uint32_t signature ;   // should be 0xBEBEFADA, target for & operator to get address of header
    uint16_t version ;     // version marker (MUST BE the same as in memory header)
    uint16_t flags ;       // reserved
    int32_t  gni ;         // first dimension of data array   = li0 + (zni - 1) * lni (row size)
    int32_t  gnj ;         // second dimension of data array  = lj0 + (znj - 1) * lnj (column size)
    int32_t  gnk ;         // third dimension of data array ( 1 for 2D data) (number of data planes)
    int16_t  zni ;         // number of blocks in a row
    int16_t  li0 ;         // first dimension of the first block(s) in first (leftmost) column
    int16_t  lni ;         // first dimension of all but first (leftmost) block in row(s) (number of values)
    int16_t  znj ;         // number of block rows
    int16_t  lj0 ;         // second dimension of block(s) in the first (bottom) row
    int16_t  lnj ;         // second dimension of all but first (bottom) block in column(s) (number of values)
} fmap ;
static const fmap null_fmap = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0} ;

CT_ASSERT(sizeof(fmap) == (sizeof(fmap) / sizeof(int32_t)) * sizeof(int32_t) , "fmap struc size not a multiple of 32 bits")

// in memory data map struct
// the first part (mhead) is only present in memory
// the second part (fhead) is the file data map
typedef struct{
  // ---------------- start of in memory header ----------------
  mmap mhead ;
  // ---------------- start of in file header ----------------
  fmap fhead ;
  // ---------------- sizes table ----------------
  fmap_block_size size[] ;        // size (in 32 bit units) of encoded blocks ( size[znj*zni] )
  // if znj*zni is odd, there is a supplementary uint16_t (padding to multiple of uint32_t length)
  // ---------------- end of data map ----------------
}zmap ;

CT_ASSERT(sizeof(zmap) == (sizeof(zmap) / sizeof(int32_t)) * sizeof(int32_t) , "zmap struc size not a multiple of 32 bits")

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
index_pair  block_index(zmap *map, int32_t i, int32_t j);
ij_range map_block_limits(zmap *map, int32_t i, int32_t j);

zmap *new_file_zmap(uint32_t map_words, uint32_t rec_words);
int fmap_invalid(zmap *map);
void fmap_init(zmap *map, int32_t gni, int32_t gnj, int32_t gnk, int32_t bsizex, int32_t bsizey);
void fmap_print(zmap *map);
void zmap_print(zmap *map);

void *filemap_address(zmap *map);
uint32_t filemap_words(zmap *map);
uint32_t filemap_blocks(int32_t gni, int32_t gnj, int32_t gnk, int32_t bsize, int32_t aspect);
size_t filemap_needed_words(int32_t gni, int32_t gnj, int32_t gnk, int32_t bsize, int32_t aspect);
size_t filemap_needed_size(int32_t gni, int32_t gnj, int32_t gnk, int32_t bsize, int32_t aspect);
size_t zmap_needed_size(int32_t gni, int32_t gnj, int32_t gnk, int32_t bsize, int32_t aspect);

// zmap    *new_zmap(int32_t gni, int32_t gnj, int32_t gnk, int32_t bsize, int32_t aspect, size_t esize, int32_t extra);
zblocks *mem_zmap(zmap *map, uint32_t *data, size_t size);
int bsize_zmap(zmap *map, size_t esize);
int fillmem_zmap(zmap *map);
ssize_t repack_map(zmap *map);
ssize_t resize_map(zmap *map);
int     free_zmap(zmap *map, int full);
void print_zmap(zmap *map, char *msg);

static inline int zmap_index_invalid(zmap *map, int index){
  return (index < 0 || index >= map->fhead.zni * map->fhead.znj) ;
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
