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
// data zblocks layout example
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
// stripe delimiter =
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
# if ! defined(Z_DATA_MAP_VERSION)

# define Z_DATA_MAP_VERSION  10

#include <stdint.h>
#include <stdlib.h>

typedef struct{
  int32_t i0  ;
  int32_t in  ;
  int32_t j0  ;
  int32_t jn  ;
}ij_range ;

// packed data representation (as in buffer read from file or in memory)
//
//   <----------------- buffer_size ---------------------->
//   <-- sizeof(zmap) ->
//                     <- 2*zni*znj ->
//   +-----------------+-------------+--------------------+
//   |                 |             |                    |
//   | data map header |    size     | packed data stream |
//   |                 |  [zni*znj]  |                    |
//   +-----------------+-------------+--------------------+
//   |buffer                         |
//   |data_map                       |data_ptr
//
// data map can be mapped directly to the beginning of the packed data representation
// uint8_t buffer[buffer_size]
// zmap *data_map = (zmap *) buffer ;
// read(fd, buffer, buffer_size)
// zij = zmap->zni * zmap->znj : number of blocks
// zmap->size[zi]        : size of block with zigzag index zi
// zmap->mem = malloc(zij * sizeof(void *))
//    table of data block addresses, starting at data_ptr
//    data_ptr = data_map + sizeof(zmap) + zmap->zni * zmap->znj * sizeof(int16_t)
//    zmap->mem[0] = data_ptr, zmap->mem[i] = zmap->mem[i-1] + zmap->size[i-1]
//    zmap->mem[zi] is the address of block with zigzag index zi
//
// (lix,ljx) may differ from(li,lj)
// either
// - the first block along a dimension will be larger
//   block[1,1] : (lix,ljx), block[i,1] : (li,ljx), block[1,j] : (lix,lj)
// - the last block along a dimension will be smaller
//   block[zni,znj] : (lix,ljx), block[i,znj] : (li,ljx), block[zni,j] : (lix,lj)
// - all blocks have the same dimension 
//   block[i,j] : (li,lj)
//
typedef uint32_t *zblocks ;   // zblocks[zi] is address of block[ zindex(i,j) ]

typedef struct{          // data map
  // ---------------- start of header ----------------
  uint32_t version:16 ,  // version marker
           stripe:16 ;   // stripe width (last stripe may be narrower)
  uint32_t flags ;       // reserved for flags
  uint32_t gni ;         // first dimension of data array   = lix + (zni - 1) * lni
  uint32_t gnj ;         // second dimension of data array  = ljx + (znj - 1) * lnj
  uint32_t zni ;         // number of blocks in a row
  uint32_t znj ;         // number of block rows
  uint32_t lni:16 ,      // first dimension of most blocks (number of values)
           lnj:16 ;      // second dimension of most blocks (number of values)
  uint32_t lix:16 ,      // first dimension of the first/last block in row
           ljx:16 ;      // second dimension of blocks in the first/last (bottom/top) row
  zblocks *mem ;         // table of memory addresses of encoded blocks when in memory
  // ---------------- end of header ----------------
  uint16_t size[] ;      // size (in 32 bit units) of encoded block ( size[znj*zni] )
}zmap ;

int32_t Zindex_from_i_j(uint32_t i, uint32_t j, uint32_t nti, uint32_t ntj, uint32_t sf0);
ij_range Zindex_to_i_j(uint32_t zij, uint32_t nti, uint32_t ntj, uint32_t sf0);

int32_t Z_block_index(zmap map, uint32_t i, uint32_t j);
ij_range block_index(zmap map, uint32_t i, uint32_t j);

zmap *new_zmap(uint32_t gni, uint32_t gnj, uint32_t stripe, size_t esize);
zblocks *mem_zmap(zmap *map, uint32_t *data);
int free_zmap(zmap *map, int full);

#endif
