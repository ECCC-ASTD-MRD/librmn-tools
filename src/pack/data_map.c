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
#include <rmn/data_map.h>

// zij    [IN] : Z (zigzag) index
// nti    [IN] : row size
// ntj    [IN] : number of rows
// sf0    [IN] : stripe width (last stripe may be narrower)
// the function returns i and j coordinates in struct ij_index
ij_index Zindex_to_i_j(int32_t zij, uint32_t nti, uint32_t ntj, uint32_t sf0){
  return Zindex_to_i_j_(zij, nti, ntj, sf0) ;
}

// i      [IN] : index in row
// j      [IN] : index of row
// nti    [IN] : row size
// ntj    [IN] : number of rows
// sf0    [IN] : stripe width (last stripe may be narrower)
// the function returns the Z (zigzag index associated to i and j
int32_t Zindex_from_i_j(int32_t i, int32_t j, uint32_t nti, uint32_t ntj, uint32_t sf0){
  return Zindex_from_i_j_(i, j, nti, ntj, sf0) ;
}
