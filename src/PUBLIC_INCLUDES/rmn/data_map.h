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
// data tiles layout example
//
// tiles along i (x) : 10   (NTI)
// tiles along j (y) : 11   (NTJ)
// stripe factor : 4        (SF0)
// top stripe factor        (SF1)  (may be smaller than SF0)
//
// the number (ZI) in the tiles is the sequential position in the data map (Z index)
//
// SF1 = MODULO(NTJ , SF0)
// if(SF1 == 0) then SF1 = SF0
// STJ = J / SF0                ( stripe number for row J )
// J0  = STJ * SF0              ( J index of lower row in stripe )
// if(J0 + SF0 > NTJ) then SF = SF1 else SF = SF0    ( stripe factor for this row )
// ZI = (J0 * NTI) + (J - J0) + (SF1 * I)            ( Z index of tile[I,J] )
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
// * delimited region, 12 tiles
// option 1 : ( probably slowest )
//   read tiles 2->3, 6->7, 10->11, 40->41, 44->45, 48->49 [ 12 tiles read, 6 IO requests ]
// option 2 :
//   read tiles 2->11 and 40->49 [ 20 tiles read, 2 IO requests ]
// option 3 : ( probably fastest)
//   read tiles 2->49 [ 48 tiles read, 1 IO request ]
//
// % delimited region, 6 tiles
//  option 1 : ( probably slower )
//    read tiles 13->14, 17->18, 21->22 [ 6 tiles read, 3 IO requests ]
//  option 2 : ( probably fastest )
//    read tiles 13->22 [ 10 tiles read, 1 IO request ]
//
// # delimited region, 8 tiles
//  option 1 : ( ideal case )
//    read tiles 28->35 [ 8 tiles read, 1 IO request ]
# if ! defined(Z_DATA_MAP)
# define Z_DATA_MAP
#include <stdint.h>

static uint32_t Zindex_from_i_j_(uint32_t i, uint32_t j, uint32_t nti, uint32_t ntj, uint32_t sf0){
  uint32_t zi, sf1, j0, stj, cj ;

  sf1 = ntj - ((ntj / sf0) * sf0) ;         // modulo(ntj, sf0)
  if(sf1 == 0) sf1 = sf0 ;                  // same as sf0 if modulo  == 0
  stj = j / sf0 ;                           // stripe number for this row
  j0 = stj * sf0 ;                          // j index of lowest row in stripe
  cj = ((j0 + sf0) > ntj) ? sf1 : sf0 ;     // width along j of current stripe
  zi = (j0 * nti) +                         // lower left corner of stripe
       (j - j0) +                           // number of rows above bottom of stripe
       (i * sf1) ;                          // i * stripe width

  return zi ;
}

#endif
