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
#include <rmn/array_nd.h>
#include <rmn/move_blocks.h>

void fill_2d_array(int32_t ni, int32_t nj, int32_t z[nj][ni]){
  int i, j ;
  for(j=0 ; j<nj ; j++){
    for(i=0 ; i<ni ; i++){
      z[j][i] = (i << 12) + j ;
    }
  }
fprintf(stderr, "z[0][0] = %8.8x, z[%3d][%3d] = %8.8x (%3d %3d)\n", z[0][0], ni-1, nj-1, z[nj-1][ni-1], ni-1, nj-1) ;
}

void  fill_array(array_2d *a){
  fill_2d_array(a->dim[0].gnn, a->dim[1].gnn, ( int32_t (*)[] )a->data) ;
}

int32_t check_2d_block(int32_t ni, int32_t nj, int32_t block[nj][ni], int32_t i0, int32_t j0, block_properties bp){
  int errors = 0, i, j ;
  for(j=0 ; j<nj ; j++){
    for(i=0 ; i<ni ; i++){
      int32_t expected = ( (i0+i) << 12 ) + (j0 + j) ;
      if(block[j][i] != expected) errors++ ;
    }
  }
  fprintf(stderr, "check_2d_block : errors = %d, |_ = [%3d,%3d], -| = [%3d,%3d]\n",
                    errors, bp.minu.u >> 12, bp.minu.u & 0xFFF, bp.maxu.u >> 12, bp.maxu.u & 0xFFF ) ;
  return errors ;
}

// int zmap_to_array(zmap *map, array_2d *a_in, sfn_ptr fn, sfn_args *fnargs){
//   return 0 ;
// }

int process_2d_block(array_2d *a_in, sfn_ptr fn, sfn_args *fnargs){
  (void) (fn) ; (void) (fnargs) ;      // unused for now
  if(a_in == NULL) return -1 ;
  if(a_in->ndim != 2) return -1 ;
  int32_t ni = a_in->dim[0].lnn, nj = a_in->dim[1].lnn ;

  block_properties bp ;
  // allocate local block for subarray copy
  int32_t block[nj][ni] ;
  // find base address of subarrray
  uint8_t *start_of_data = subarray_address((array_nd *)a_in) ;
  // get local copy of subarray
  int32_t nelem = move_data32_block(start_of_data , a_in->dim[0].gnn, &block[0][0], ni, ni, nj, &bp) ;

  fprintf(stderr, "process_2d_block : automatically allocated block[%3d][%3d], subarray offset = %ld\n"
                , nj, ni, start_of_data - a_in->data ) ;
  if(nelem <= 0){
    fprintf(stderr, "process_2d_block : ERROR, move_data32_block failed (%d)\n", nelem);
    fprintf(stderr, "                   lnis = %d, lnid = %d, ni = %d, nj = %d\n", a_in->dim[0].gnn, ni, ni, nj);
    return nelem ;
  }
  int errors = check_2d_block(ni, nj, (int32_t (*)[]) &block[0][0], a_in->dim[0].ln0, a_in->dim[1].ln0, bp) ;
  if(errors > 0) return (-errors) ;

  return nelem ;
}

// process array and store it into zmap
zmap *array_to_zmap(zmap *map, array_2d *a_in, sfn_ptr fn, sfn_args *fnargs){
  int zx ;
  array_2d a ;
//   (void) (fn) ; (void) (fnargs) ;      // unused for now

  if(a_in == NULL) return NULL ;
  a = *a_in ;
  int32_t esize = a.esize ;

  fprintf(stderr, "array_to_zmap : stripe = %d, esize = %d\n", map->stripe, esize) ;
  fprintf(stderr, "map block sizes : ") ;for(zx=0 ; zx < map->zni * map->znj ; zx++){ fprintf(stderr, "%4d ",map->size[zx]);}  fprintf(stderr, "\n") ;
  for(zx=0 ; zx < map->zni * map->znj ; zx++){  // loop over zindex
    ij_pair  ijp = Zindex_to_i_j(zx, map->zni, map->znj, map->stripe) ;
    ij_range ijr = block_limits(map, ijp.i, ijp.j) ;
    int32_t gni = a.dim[0].gnn ;
    int32_t i0 = ijr.i0 ;
    int32_t in = ijr.in ;
    int32_t ni = in-i0+1 ;
    int32_t j0 = ijr.j0 ;
    int32_t jn = ijr.jn ;
    int32_t nj = jn-j0+1 ;
    uint32_t bsize = map->size[zx] ;
    fprintf(stderr, "array_to_zmap : zblock %3d [%3d,%3d] (%3d:%3d,%3d:%3d), gni = %3d, i0 = %3d, j0 = %3d, bsize = %d\n",
                     zx, ijp.i, ijp.j, i0, in, j0, jn, gni, i0, j0, bsize) ;
    if( ( ni == map->lix || ni == map->lni) && ( nj == map->ljx || nj == map->lnj) ){
      a.dim[0].ln0 = i0 ;  // set subarray limits
      a.dim[1].ln0 = j0 ;
      a.dim[0].lnn = ni ;
      a.dim[1].lnn = nj ;
      if(process_2d_block(&a, fn, fnargs) <= 0){
        fprintf(stderr, "array_to_zmap : ERROR in process_2d_block\n") ;
        return NULL ;
      }
//       block_properties bp ;
//       int32_t block[nj][ni] ;
//       uint8_t *start_of_data = a.data + ((gni * j0) + i0) * esize ;  // lower left corner of data
//       fprintf(stderr, "array_to_zmap : automatically allocated block[%3d][%3d], subarray offset = %ld\n"
//                     , nj, ni, start_of_data - a.data) ;
//       int32_t nelem = move_data32_block(start_of_data , gni, &block[0][0], ijr.in-ijr.i0+1, ijr.in-ijr.i0+1, ijr.jn-ijr.j0+1, &bp) ;
//       if(nelem <= 0) {
//         fprintf(stderr, "array_to_zmap : ERROR, move_data32_block failed (%d), zblock %d\n", nelem, zx);
//         fprintf(stderr, "                lnis = %d, lnid = %d, ni = %d, nj = %d\n", gni, ijr.in-ijr.i0+1, ijr.in-ijr.i0+1, ijr.jn-ijr.j0+1);
//         return NULL ;
//       }
//       int errors = check_2d_block(ni, nj, (int32_t (*)[]) &block[0][0], i0, j0, bp) ;
//       if(errors > 0) return NULL ;
    }else{
      fprintf(stderr, "array_to_zmap : ERROR, wrong block dimensions, ni = %3d, must be %d or %d, nj = %3d, must be %3d or %3d\n",
                       ni, map->lix, map->lni, nj, map->ljx, map->lnj) ;
      return NULL ;
    }
    // check compressed stream size in map for this block
    if(bsize * esize < ni * nj * sizeof(uint32_t)){
      fprintf(stderr, "array_to_zmap : ERROR, compressed stream area is too small, size = %d, should be at least %ld\n", bsize , ni * nj * sizeof(uint32_t)) ;
    }
    fprintf(stderr, "\n") ;
  }
  return map ;
}

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
//   if(argc > 0) return 0 ;

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
  zmap *map = new_zmap(gni, gnj, stripe, sizeof(uint32_t));
  if(map == NULL) exit(1) ;
  if(map->zni != 3 || map->znj != 3) exit(1) ;

  fprintf(stderr, "size of preamble = %ld\n", (uint8_t *)&(map->data_head) - (uint8_t *)&(map->mh.signature)) ;
  fprintf(stderr, "size of array_nd = %ld\n", sizeof(array_nd));
  fprintf(stderr, "size of array_1d = %ld\n", sizeof(array_1d));
  fprintf(stderr, "size of array_2d = %ld\n", sizeof(array_2d));
  fprintf(stderr, "size of array_3d = %ld\n", sizeof(array_3d));
  fprintf(stderr, "size of array_4d = %ld\n", sizeof(array_4d));
  fprintf(stderr, "size of array_5d = %ld\n", sizeof(array_5d));

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

  fprintf(stderr, "=============== data map sizes reduce ===============\n") ;
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

  fprintf(stderr, "=============== data map sizes restore ===============\n") ;
  // restore packed stream pointers
  for(i=0 ; i<znij ; i++) map->size[i] += 2 ;
  newsize = resize_map(map) ;
  if(newsize == oldsize) fprintf(stderr, "SUCCESS\n") ;

  fprintf(stderr, "=============== block limits ===============\n") ;
  fprintf(stderr, "blocks[%d,%d] => data[%4d,%4d]", map->zni, map->znj, map->gni, map->gnj) ;
  fprintf(stderr, ", first block along i is  %s"  , map->lix > map->lni ? "longer" : "shorter") ;
  fprintf(stderr, ", first block along j is  %s\n", map->ljx > map->lnj ? "longer" : "shorter") ;
  int32_t zx ;
  for(j = (int)map->znj ; j > 0 ; j--){
    ijr = block_limits(map, 0, 0) ;       // no more warning about possibility of ijr.j0 to be uninitialized
    for(i = 0 ; i < (int)map->zni ; i++){
      ijr = block_limits(map, i, j-1) ;
//       zx = Zindex_from_i_j(i, j-1, map->zni, map->znj, map->stripe);
      zx = Z_map_index(map, i, j-1) ;
      fprintf(stderr, "data[%4d:%4d,%4d:%4d](Z %2d)  ", ijr.i0, ijr.in, ijr.j0, ijr.jn, zx) ;
    }
    fprintf(stderr, "j_range : %4d)\n", ijr.jn - ijr.j0 + 1);
  }
  for(i = 0 ; i < (int)map->zni ; i++){
    ijr = block_limits(map, i, 0) ;
    fprintf(stderr, "i_range : %4d                   ", ijr.in - ijr.i0 + 1);
  }
  fprintf(stderr, "\n");

  fprintf(stderr, "=============== split array according to map ===============\n") ;
  array_2d a2d = array_2d_null ;
//   new_array(&a2d, NULL, 4, 'U', map->gni, map->gnj) ;  // create 2D array, map->gni x map->gnj
  new_array(&a2d, NULL, 4, uint_data, map->gni, map->gnj) ;  // create 2D array, map->gni x map->gnj
  fill_array(&a2d) ;
  zmap *result = array_to_zmap(map, &a2d, NULL, NULL) ;
  if(result == NULL) exit(1) ;
  fprintf(stderr, "SUCCESS\n") ;
  return 0 ;
}
