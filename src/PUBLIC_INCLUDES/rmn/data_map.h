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
// each block has dimensions lni|lix x lnj|ljx x lnk|lkx
// mem and size have dimensions zni x znj x znk
// there is no striping along z
//  compression may be 2D (lnk blocks lni x lnj) or 3D (1 block lni x lnj x lnk)
#if ! defined(Z_DATA_MAP_VERSION)

#define Z_DATA_MAP_VERSION  10

#include <stdint.h>
#include <stdlib.h>

#include <rmn/ct_assert.h>
#include <rmn/split_dimension.h>

// packed data representation (as in buffer read from file or in memory)
//
//   <----------------------------------- in memory ---------------------------------->
//   <----------- sizeof(zmap) ---------->             <-mextra ->
//   <- sizeof(mhead) ->                 <- 2*zni*znj ->
//   +-----------------+-----------------+-------------+---------+--------------------+
//   |                 |                 |   size      |  extra  |                    |
//   |  memory header  | data map header |   table     |  global | packed data stream |
//   |                 |                 |   [znij]    |  info   |                    |
//   +-----------------+-----------------+-------------+---------+--------------------+
//                     |                 |
//                     |data_head        |size[]       |data stream
//                     <-------------------------- in file --------------------------->
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
// (lix,ljx) may differ from(li,lj)  (half size to size and a half - 1)
//   li/2 <= lix < li + li/2
//   lj/2 <= ljx < lj + lj/2
// either
// - the first block along a dimension is larger or smaller
//   block[0,0] : (lix,ljx)          (first block of first row)
//   block[i,0] : ( li,ljx)  (i > 0) (first row)
//   block[0,j] : (lix, lj)  (j > 0) (first column)
//   block[i,j] : ( li, lj)  (i > 0, j > 0)
// - all blocks have the same dimension 
//   block[i,j] : ( li, lj)
//
typedef uint32_t *zblocks ;   // zblocks[zi] is address of block[ zindex(i,j) ]

// global metadata, in the data map
typedef struct{
  union{
    float    f ;
    int32_t  i ;
    uint32_t u ;
  } m[4] ;
} zmeta ;
#define zmeta_null (zmeta) { .m = { {0}, {0}, {0}, {0} } }

// in memory data map struct
// the first part (mhead) is only present in memory
// the second part (fhead) is the file data map
typedef struct{
  // ---------------- start of in memory header ----------------
  struct{
    union{
      uint32_t data_head ; // target for & operator to get address of header
      uint32_t signature ; // should be 0x1AD0FADA
    } ;
    uint32_t version: 8,   // same as in file header
              spare :16,
              flags : 8 ;  // reserved for internal use flags
    zblocks *mem ;         // table[zni*znj] : memory addresses of encoded blocks in memory
    uint8_t *options ;     // same dimension as size, options associated with each encoded block
    uint32_t *first ;      // start of compressed data stream
    uint32_t *limit ;      // one past the end of compressed data stream
    uint32_t *extra ;      // points to extra information
  } mhead ;
  // ---------------- start of in file header ----------------
  // TODO: add flags for 3D storage ni/nj/nk vs nk/ni/nj vs ... and compression(2D/3D)
  // TODO: add flags for Z ordering algorithm kind (Morton order, stripes, ...)
  // TODO: finalize what is needed and what is not needed
  struct{
    union{
      uint32_t data_head ;  // target for & operator to get address of header
      struct{
        uint32_t version : 8, // version marker (same as in memory header)
                 stripe  : 8, // stripe width (last/top stripe may be narrower)
                 mextra  : 8, // extra global info length (in 32 bit units) (after size table)
                 flags   : 8; // reserved for flags
      } ;
    } ;
    zmeta   meta ;         // global metadata (applies to all blocks)
    int32_t gni ;          // first dimension of data array   = lix + (zni - 1) * lni (row size)
    int32_t gnj ;          // second dimension of data array  = ljx + (znj - 1) * lnj (column size)
    int32_t gnk ;          // third dimension of data array
    int32_t zni ;          // number of blocks in a row
    int32_t znj ;          // number of block rows
    int32_t lni:16 ,       // first dimension of all but first block (number of values)
            lnj:16 ;       // second dimension of all but first block (number of values)
    int32_t lix:16 ,       // first dimension of the first block in row
            ljx:16 ;       // second dimension of blocks in the first (bottom) row
    int32_t znk:16 ,       // number of block planes
            lnk: 8 ,       // third dimension of data blocks
            lkx: 8 ;       // third dimension of data blocks in the first(bottom) plane
    uint32_t signature ;   // should be 0xBEBEFADA (position allows to check that the size of meta is as expected)
  }fhead ;
  // ---------------- end of in file header ----------------
  uint16_t size[] ;        // size (in 32 bit units) of encoded blocks ( size[znj*zni] )
  // if znj*zni is odd, there is a supplementary uint16_t (padding to multiple of uint32_t length)
  // if extra is not 0, mextra uint32_t items are added after the size table
  // ---------------- end of data map ----------------
}zmap ;
//                        mhead              fhead - zmta         zmeta
CT_ASSERT(sizeof(zmap) == 6*sizeof(void *) + 10*sizeof(int32_t) + sizeof(zmeta), "unexpected size of zmap structure")

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

int32_t Zindex_from_ij(int32_t i, int32_t j, int32_t nti, int32_t ntj, int32_t sf0);
index_pair Zindex_to_ij(int32_t zij, int32_t nti, int32_t ntj, int32_t sf0);

int32_t  Z_map_index(zmap *map, int32_t i, int32_t j);
index_pair  block_index(zmap *map, int32_t i, int32_t j);
ij_range map_block_limits(zmap *map, int32_t i, int32_t j);

zmap    *new_zmap(int32_t gni, int32_t gnj, int32_t stripe, size_t esize, int32_t extra);
zblocks *mem_zmap(zmap *map, uint32_t *data);
ssize_t repack_map(zmap *map);
ssize_t resize_map(zmap *map);
int     free_zmap(zmap *map, int full);

static inline int zmap_index_invalid(zmap *map, int index){
  return (index < 0 || index >= map->fhead.zni * map->fhead.znj) ;
}

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
