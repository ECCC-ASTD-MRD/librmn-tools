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
  ij_pair ijp ;
  ij_range ijr ;

  if(argc > 1 && argv[0] == NULL) return 1 ;  // useless code to get rid of compiler warning

  fprintf(stderr, "=============== block indexing ===============\n") ;

  int ln0, ln, l, i0, lb ;
  ln = 64 ;

  for(ln0=ln/2 ; ln0<2*ln ; ln0++){  // loop over block sizes
    i0 = -1 ; lb = ln0 ;
    if(ln0==ln/2 || ln0==ln || ln0==2*ln-1) {
      fprintf(stderr, "ln0 = %3d, ln = %3d, %4d values,", ln0, ln, ln0 + (NTI-1)*ln) ;
      ijp = b_limits(0, ln, ln0) ;
      fprintf(stderr, " first block [%4d,%4d] (size = %3d),", ijp.i, ijp.j, ijp.j-ijp.i+1) ;
    }
    for(j=0 ; j<NTI ; j++, lb=ln){   // loop over blocks
      ijp = b_limits(j, ln, ln0) ;
      if(ijp.i != i0+1 || ijp.j != i0+lb){
        fprintf(stderr, "ERROR: block %d limits, expected [%d,%d], got [%d,%d]\n", j, i0+1, i0+lb, ijp.i, ijp.j) ;
        exit(1) ;
      }
      for(i=0 ; i<lb ; i++){
        i0++ ;
        l = b_index(i0, ln, ln0) ;
        if(l != j){
          fprintf(stderr, "ERROR: index = %d, ln0 = %d, ln = %d, expecting block %d, got %d\n", i0, ln0, ln, j, l) ;
          exit(1) ;
        }
      }
    }
    if(ln0==ln/2 || ln0==ln || ln0==2*ln-1) {
      fprintf(stderr, " last block [%4d,%4d] (size = %d)\n", ijp.i, ijp.j, ijp.j-ijp.i+1) ;
    }
  }
  fprintf(stderr, "SUCCESS\n") ;

  fprintf(stderr, "=============== zigzag block indexing ===============\n") ;
  for(j=NTJ-1 ; j>=0 ; j--){ 
    for(i=0 ; i<NTI ; i++) { 
      x[i] = Zindex_from_i_j(i, j, NTI, NTJ, SF0) ;
      y[i] = Zindex_from_i_j(i, j, NTI, NTJ, SF0) ;
      ijp   = Zindex_to_i_j(y[i], NTI, NTJ, SF0) ;
      if(ijp.i != i || ijp.j != j){
        fprintf(stderr, "ERROR: zij = %3d, expecting i,j = (%2d,%2d), got (%2d,%2d)\n", x[i], i, j, ijp.i, ijp.j) ;
        exit(1) ;
      }
    }
    if(argc > 1){
      for(i=0 ; i<NTI ; i++) { fprintf(stderr, "+------"             ) ; } fprintf(stderr, "+\n") ;
      for(i=0 ; i<NTI ; i++) { fprintf(stderr, "| %3d  " ,       x[i]) ; } fprintf(stderr, "| (Z index)\n") ;
      for(i=0 ; i<NTI ; i++) { fprintf(stderr, "|%2d,%3d",    i,    j) ; } fprintf(stderr, "| (expected i,j)\n") ;
      for(i=0 ; i<NTI ; i++) { 
        ijp   = Zindex_to_i_j(x[i], NTI, NTJ, SF0) ;
        fprintf(stderr, "|%2d,%3d", ijp.i, ijp.j) ; 
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

  fprintf(stderr, "=============== data map creation ===============\n") ;
  int gni = 128+66, gnj = 128+32, stripe = 2 ;
  zmap *map = new_zmap(gni, gnj, stripe, sizeof(uint8_t));
  if(map == NULL) exit(1) ;
  if(map->zni != 3 || map->znj != 3) exit(1) ;

  fprintf(stderr, "size of preamble = %ld\n", (uint8_t *)&(map->data_head) - (uint8_t *)&(map->mh.signature)) ;
  fprintf(stderr, "size of array_nd = %ld\n", sizeof(array_nd));
  fprintf(stderr, "size of array_1d = %ld\n", sizeof(array_1d));
  fprintf(stderr, "size of array_2d = %ld\n", sizeof(array_2d));
  fprintf(stderr, "size of array_3d = %ld\n", sizeof(array_3d));

  zblocks *mem = map->mh.mem ;
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

  uint32_t oldsize = map->mh.mem[znij] - map->mh.mem[0] ;
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

  fprintf(stderr, "=============== block limits ===============\n") ;
  fprintf(stderr, "blocks[%d,%d] => data[%4d,%4d]", map->zni, map->znj, map->gni, map->gnj) ;
  fprintf(stderr, ", first block along i is  %s"  , map->lix > map->lni ? "longer" : "shorter") ;
  fprintf(stderr, ", first block along j is  %s\n", map->ljx > map->lnj ? "longer" : "shorter") ;
  for(j = (int)map->znj ; j > 0 ; j--){
    ijr = block_limits(map, 0, 0) ;       // no more warning about possibility of ijr.j0 to be uninitialized
    for(i = 0 ; i < (int)map->zni ; i++){
      ijr = block_limits(map, i, j-1) ;
      fprintf(stderr, "data[%4d:%4d,%4d:%4d]  ", ijr.i0, ijr.in, ijr.j0, ijr.jn) ;
    }
    fprintf(stderr, "j_range : %4d)\n", ijr.jn - ijr.j0 + 1);
  }
  for(i = 0 ; i < (int)map->zni ; i++){
    ijr = block_limits(map, i, 0) ;
    fprintf(stderr, "i_range : %4d             ", ijr.in - ijr.i0 + 1);
  }
  fprintf(stderr, "\n");
  return 0 ;
}
