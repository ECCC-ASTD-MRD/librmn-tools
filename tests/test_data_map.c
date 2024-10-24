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
#include <stdlib.h>
#include <rmn/data_map.h>

#define NTI 10
#define NTJ 11
#define SF0  4

// #define NTI  4
// #define NTJ  3
// #define SF0  2

int main(int argc, char **argv){
  int i, j, x[NTI], y[NTI], znij ;
  ij_range ij ;

  if(argc > 1 && argv[0] == NULL) return 1 ;  // useless code to get rid of compiler warning

  fprintf(stderr, "=============== zigzag indexing check ===============\n") ;
  for(j=NTJ-1 ; j>=0 ; j--){ 
    for(i=0 ; i<NTI ; i++) { 
      x[i] = Zindex_from_i_j(i, j, NTI, NTJ, SF0) ;
      y[i] = Zindex_from_i_j(i, j, NTI, NTJ, SF0) ;
      ij   = Zindex_to_i_j(y[i], NTI, NTJ, SF0) ;
      if(ij.i0 != i || ij.j0 != j){
        fprintf(stderr, "ERROR: zij = %3d, expecting i,j = (%2d,%2d), got (%2d,%2d)\n", x[i], i, j, ij.i0, ij.j0) ;
        exit(1) ;
      }
    }
    if(argc > 1){
      for(i=0 ; i<NTI ; i++) { fprintf(stderr, "+------"             ) ; } fprintf(stderr, "+\n") ;
      for(i=0 ; i<NTI ; i++) { fprintf(stderr, "| %3d  " ,       x[i]) ; } fprintf(stderr, "| (Z index)\n") ;
      for(i=0 ; i<NTI ; i++) { fprintf(stderr, "|%2d,%3d",    i,    j) ; } fprintf(stderr, "| (expected i,j)\n") ;
      for(i=0 ; i<NTI ; i++) { 
        ij   = Zindex_to_i_j(x[i], NTI, NTJ, SF0) ;
        fprintf(stderr, "|%2d,%3d", ij.i0, ij.j0) ; 
      } fprintf(stderr, "| (computed i,j)\n") ;
    }else{
      for(j = NTJ ; j > 0 ; j--){
        for(i = 0 ; i < NTI ; i++){
          fprintf(stderr, "%3d => [%2d,%2d] ", Zindex_from_i_j(i, j-1, NTI, NTJ, SF0), i, j-1) ;
        }
        fprintf(stderr, "\n");
      }
    }
  }
  if(argc > 1) {
    for(i=0 ; i<NTI ; i++) { fprintf(stderr, "+------"        ) ; } fprintf(stderr, "+\n") ;
  }
  fprintf(stderr, "SUCCESS\n") ;

  fprintf(stderr, "=============== data map creation check ===============\n") ;
  int gni = 128+66, gnj = 128+32, stripe = 2 ;
  zmap *map = new_zmap(gni, gnj, stripe, sizeof(uint8_t));
  if(map == NULL) exit(1) ;
  if(map->zni != 3 || map->znj != 3) exit(1) ;

  zblocks *mem = map->mem ;
  znij = map->zni * map->znj ;
  fprintf(stderr, "size from old pointer table[%d] :", znij);
  for(i=0 ; i < znij ; i++) fprintf(stderr, "%6ld", mem[i+1] - mem[i]) ;
  fprintf(stderr, "\n");
  fprintf(stderr, "size from old sizes table  [%d] :", znij);
  for(i=0 ; i < znij ; i++) fprintf(stderr, "%6d", map->size[i]) ;
  fprintf(stderr, "\n");
  for(i=0 ; i < znij ; i++) if(map->size[i] != (mem[i+1] - mem[i])) exit(1) ;
  fprintf(stderr, "SUCCESS\n") ;

  free_zmap(map, 0) ;             // partial free (only mem table)
  mem = mem_zmap(map, NULL) ;     // reallocate mem table
  znij = map->zni * map->znj ;
  fprintf(stderr, "size from new pointer table[%d] :", znij);
  for(i=0 ; i < znij ; i++) fprintf(stderr, "%6ld", mem[i+1] - mem[i]) ;
  fprintf(stderr, "\n");
  fprintf(stderr, "size from old sizes table  [%d] :", znij);
  for(i=0 ; i < znij ; i++) fprintf(stderr, "%6d", map->size[i]) ;
  fprintf(stderr, "\n");
  for(i=0 ; i < znij ; i++) if(map->size[i] != (mem[i+1] - mem[i])) exit(1) ;
  fprintf(stderr, "SUCCESS\n") ;

  uint32_t oldsize = map->mem[znij] - map->mem[0] ;
  fprintf(stderr, "initial data size = %6d\n", oldsize) ;
  for(i=0 ; i<znij ; i++) map->size[i] -= 2 ;
  uint32_t newsize = repack_map(map) ;
  fprintf(stderr, "packed data size = %6d\n", newsize) ;
  if(newsize != oldsize - 2*znij) exit(1) ;
  fprintf(stderr, "SUCCESS\n") ;
  fprintf(stderr, "size from new pointer table[%d] :", znij);
  for(i=0 ; i < znij ; i++) fprintf(stderr, "%6ld", mem[i+1] - mem[i]) ;
  fprintf(stderr, "\n");
  fprintf(stderr, "size from new sizes table  [%d] :", znij);
  for(i=0 ; i < znij ; i++) fprintf(stderr, "%6d", map->size[i]) ;
  fprintf(stderr, "\n");
  for(i=0 ; i < znij ; i++) if(map->size[i] != (mem[i+1] - mem[i])) exit(1) ;
  fprintf(stderr, "SUCCESS\n") ;

  fprintf(stderr, "=============== block limits check ===============\n") ;
  fprintf(stderr, "blocks[%d,%d] => data[%4d,%4d]", map->zni, map->znj, map->gni, map->gnj) ;
  fprintf(stderr, " %s ", map->lix > map->lni ? ", first block along i is longer" : ", last block along i may be shorter") ;
  fprintf(stderr, " %s\n", map->ljx > map->lnj ? ", first block along j is longer" : ", last block along j may be shorter") ;
  for(j = map->znj ; j > 0 ; j--){
    for(i = 0 ; i < map->zni ; i++){
      ij = block_limits(*map, i, j-1) ;
      fprintf(stderr, "data[%4d:%4d,%4d:%4d]  ", ij.i0, ij.in, ij.j0, ij.jn) ;
    }
    fprintf(stderr, "j_range : %4d)\n", ij.jn - ij.j0 + 1);
  }
  for(i = 0 ; i < map->zni ; i++){
    ij = block_limits(*map, i, 0) ;
    fprintf(stderr, "i_range : %4d             ", ij.in - ij.i0 + 1);
  }
  fprintf(stderr, "\n");
  return 0 ;
}
