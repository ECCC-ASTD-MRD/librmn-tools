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

// test double include protection
#include <rmn/data_kind.h>
#include <rmn/data_kind.h>

#include <stdio.h>
#include <stdlib.h>

#include <rmn/data_map.h>
#include <rmn/array_nd.h>
#include <rmn/move_blocks.h>

#if 0
static void fill_2d_array(int32_t ni, int32_t nj, int32_t z[nj][ni]){
  int i, j ;
  for(j=0 ; j<nj ; j++){
    for(i=0 ; i<ni ; i++){
      z[j][i] = (i << 12) + j ;
    }
  }
fprintf(stderr, "z[0][0] = %8.8x, z[%3d][%3d] = %8.8x (%3d %3d)\n", z[0][0], ni-1, nj-1, z[nj-1][ni-1], ni-1, nj-1) ;
}
#endif
#if 0
static void  fill_array(array_2d *a){
  fill_2d_array(a->dim[0].gnn, a->dim[1].gnn, ( int32_t (*)[] )a->data) ;
}

static int32_t check_2d_block(int32_t ni, int32_t nj, int32_t block[nj][ni], int32_t i0, int32_t j0, block_properties bp){
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
#endif
#if 0
static int process_2d_block(array_2d *a_in, sfn_ptr fn, sfn_args *fnargs){
  (void) (fn) ; (void) (fnargs) ;      // unused for now
  if(a_in == NULL) return -1 ;
  if(a_in->rank != 2) return -1 ;
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
#endif
#if 0
// process array and store it into zmap
static zmap *array_to_zmap(zmap *map, array_2d *a_in, sfn_ptr fn, sfn_args *fnargs){
  int zx ;
  array_2d a ;
//   (void) (fn) ; (void) (fnargs) ;      // unused for now

  if(a_in == NULL) return NULL ;
  a = *a_in ;
  int32_t esize = a.esize ;

//   fprintf(stderr, "array_to_zmap : aspect = %d, esize = %d\n", map->fhead.aspect, esize) ;
  fprintf(stderr, "array_to_zmap : aspect = %d, esize = %d\n", 1, esize) ;
  fprintf(stderr, "map block sizes : ") ;for(zx=0 ; zx < map->fhead.zni * map->fhead.znj ; zx++){ fprintf(stderr, "%4d ",map->size[zx]);}  fprintf(stderr, "\n") ;
  for(zx=0 ; zx < map->fhead.zni * map->fhead.znj ; zx++){  // loop over zindex
//     index_pair  ijp = Zindex_to_ij(zx, map->fhead.zni, map->fhead.znj, map->fhead.aspect) ;
    index_pair  ijp = Zindex_to_ij(zx, map->fhead.zni, map->fhead.znj, 1) ;
    ij_range ijr = map_block_limits(map, ijp.i, ijp.j) ;
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
    if( ( ni == map->fhead.li0 || ni == map->fhead.lni) && ( nj == map->fhead.lj0 || nj == map->fhead.lnj) ){
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
                       ni, map->fhead.li0, map->fhead.lni, nj, map->fhead.lj0, map->fhead.lnj) ;
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
#endif

#define NTI 10
#define NTJ 11
#define SF0  4

// #define NTI  4
// #define NTJ  3
// #define SF0  2

int main(int argc, char **argv){
  (void)(argc) ; (void)(argv) ;  //  suppress unused argument warning
  int i, j, znij ;
//   int x[NTI], y[NTI] ;
//   index_pair ijp ;
  index_range irange ;
//   ij_range ijr ;
  char *msg = "" ;
  int32_t gni, gnj, gnk, bsize, aspect, bsizej ;
  zmap *map = NULL ;

  goto test ;
success:
  fprintf(stderr, "SUCCESS\n") ;
  return 0 ;
fail:
  fprintf(stderr, "FAIL : %s\n", msg) ;
  return 1 ;
test:

//   if(argc > 0) return 0 ;

  fprintf(stderr, "=============== base test ===============\n") ;

  gni = 129 ; gnj = 97 ; gnk = 1 ; bsize = 64 ;
  fprintf(stderr, "base size of mmap = %ld (%ld words)\n", sizeof(mmap), sizeof(mmap)/sizeof(uint32_t));
  fprintf(stderr, "base size of fmap = %ld (%ld words)\n", sizeof(fmap), sizeof(fmap)/sizeof(uint32_t));
  fprintf(stderr, "base size of zmap = %ld (%ld words)\n", sizeof(zmap), sizeof(zmap)/sizeof(uint32_t));

  for(aspect = 1 ; aspect < 4 ; aspect++){
    if(aspect < 3) continue ;
    fprintf(stderr, "=============== aspect = %d ===============\n", aspect) ;
    if(aspect == 2) bsize = 48 ;
    if(aspect == 3) bsize = 32 ;
    bsizej = aspect * bsize ;

    int32_t bextra = 3 ;
    uint32_t blocks = filemap_blocks(gni, gnj, gnk, bsize, bsizej);
    fprintf(stderr, "array[%d,%d,%d], block size = [%d:%d], nblocks = %d", gni, gnj, gnk, bsize, bsizej, blocks) ;
    fprintf(stderr, ", file map size = %ld words\n", filemap_needed_size(gni, gnj, gnk, bsize, bsizej, bextra)/sizeof(uint32_t)) ;
    uint32_t nwords = filemap_needed_size(gni, gnj, gnk, bsize, bsizej, bextra)/sizeof(uint32_t) ;
    if(filemap_needed_words(gni, gnj, gnk, bsize, bsizej, bextra) != nwords) {
      fprintf(stderr, "ERROR: filemap_needed_words | filemap_needed_size mismatch\n");
      goto fail ;
    }

    zmap *zp = create_file_zmap(nwords+4, nwords+4+100) ;         // mextra = 4, 100 words of data
    if(fmap_invalid(zp) == 0) goto fail ;           // fmap is invalid at this point
    fmap_init(zp, gni, gnj, gnk, bsize, bsizej, NULL, bextra);   // initialize fmap part with 3 extra blocks
    zp->fhead.extra = 4 ;                                    // set extra to 4 words
    fmap_print(zp, "zp") ;
    if(fmap_invalid(zp) != 0) goto fail ;           // fmap must be valid at this point
    fprintf(stderr, "    fmap element size = %ld\n", ELEMENT_SIZE(zp->mhead.frng)) ;
    fprintf(stderr, "    filemap words = %d, zmap at %p, fmap at %p, blocks[%d:%d]\n",
            filemap_words(zp), &(zp->mhead.signature), &(zp->fhead.signature), zp->fhead.zni, zp->fhead.znj) ;
    fprintf(stderr, "\n");
    zmap_print(zp, "created file zp") ;
    fprintf(stderr, "\n");
    update_file_zmap(zp, 1);
    zmap_print(zp, "updated file zp") ;
    fprintf(stderr, "-----------\n");

    free(zp) ;
    zp = create_zmap(gni, gnj, gnk, bsize, aspect, 4, 8*sizeof(uint32_t), 3, 12*sizeof(uint32_t)) ;
    fprintf(stderr, "\n");
    free(zp) ;
    zp = create_zmap(gni, gnj, gnk, bsize, aspect, 4, 8*sizeof(uint32_t), 0, 12*sizeof(uint32_t)) ;
    fprintf(stderr, "\n");
    zp = create_zmap(gni, gnj, gnk, bsize, aspect, 4, 8*sizeof(uint32_t), 0, 0) ;
    fprintf(stderr, "\n");
    free(zp) ;
  }

  goto success ;

  fprintf(stderr, "=============== syntax test ===============\n") ;

  sfn_args *sfn_t_args ;
  malloc_sfn_args(sfn_t_args, 20) ;
  if(sfn_t_args == NULL) goto fail ;
  if(sfn_t_args->maxargs != 20){
    fprintf(stderr, "ERROR : sfn_t_args->maxargs is %d, expected 20\n", sfn_t_args->maxargs) ;
    goto fail ;
  }
  fprintf(stderr, "SUCCESS\n") ;

  fprintf(stderr, "=============== block indexing ===============\n") ;

  int ln0, ln, l, i0, lb ;
  ln = 64 ;

  for(ln0=ln/2 ; ln0<2*ln ; ln0++){  // loop over block sizes
    i0 = -1 ; lb = ln0 ;
    if(ln0==ln/2 || ln0==ln || ln0==2*ln-1) {
      fprintf(stderr, "ln0 = %3d, ln = %3d, %4d values,", ln0, ln, ln0 + (NTI-1)*ln) ;
//       ijp = b_limits(0, ln, ln0) ;
      irange = r_limits(0, ln, ln0) ;
      fprintf(stderr, " first block [%4d,%4d] (size = %3d),", irange.ix0, irange.ixn, irange.ixn-irange.ix0+1) ;
    }
    for(j=0 ; j<NTI ; j++, lb=ln){   // loop over blocks
//       ijp = b_limits(j, ln, ln0) ;
      irange = r_limits(j, ln, ln0) ;
//       if(ijp.i != i0+1 || ijp.j != i0+lb){
      if(irange.ix0 != i0+1 || irange.ixn != i0+lb){
        fprintf(stderr, "ERROR: block %d limits, expected [%d,%d], got [%d,%d]\n", j, i0+1, i0+lb, irange.ix0, irange.ixn) ;
        goto fail ;
      }
      for(i=0 ; i<lb ; i++){
        i0++ ;
        l = b_index(i0, ln, ln0) ;
        if(l != j){
          fprintf(stderr, "ERROR: index = %d, ln0 = %d, ln = %d, expecting block %d, got %d\n", i0, ln0, ln, j, l) ;
          goto fail ;
        }
      }
    }
    if(ln0==ln/2 || ln0==ln || ln0==2*ln-1) {
      fprintf(stderr, " last block [%4d,%4d] (size = %d)\n", irange.ix0, irange.ixn, irange.ixn-irange.ix0+1) ;
    }
  }
  fprintf(stderr, "SUCCESS\n") ;

#if 0
  fprintf(stderr, "=============== zigzag block indexing ===============\n") ;
  for(j=NTJ-1 ; j>=0 ; j--){ 
    for(i=0 ; i<NTI ; i++) { 
      x[i] = Zindex_from_ij(i, j, NTI, NTJ, SF0) ;
      y[i] = Zindex_from_ij(i, j, NTI, NTJ, SF0) ;
      ijp   = Zindex_to_ij(y[i], NTI, NTJ, SF0) ;
      if(ijp.i != i || ijp.j != j){
        fprintf(stderr, "ERROR: zij = %3d, expecting i,j = (%2d,%2d), got (%2d,%2d)\n", x[i], i, j, ijp.i, ijp.j) ;
        goto fail ;
      }
    }
    if(argc > 1){
      for(i=0 ; i<NTI ; i++) { fprintf(stderr, "+------"             ) ; } fprintf(stderr, "+\n") ;
      for(i=0 ; i<NTI ; i++) { fprintf(stderr, "| %3d  " ,       x[i]) ; } fprintf(stderr, "| (Z index)\n") ;
      for(i=0 ; i<NTI ; i++) { fprintf(stderr, "|%2d,%3d",    i,    j) ; } fprintf(stderr, "| (expected i,j)\n") ;
      for(i=0 ; i<NTI ; i++) { 
        ijp   = Zindex_to_ij(x[i], NTI, NTJ, SF0) ;
        fprintf(stderr, "|%2d,%3d", ijp.i, ijp.j) ; 
      } fprintf(stderr, "| (computed i,j)\n") ;
    }else{
      for(j = NTJ ; j > 0 ; j--){
        for(i = 0 ; i < NTI ; i++){
          fprintf(stderr, "%3d => [%2d,%2d] ", Zindex_from_ij(i, j-1, NTI, NTJ, SF0), i, j-1) ;
        }
        fprintf(stderr, "\n");
      }
    }
  }
  if(argc > 1) {
    for(i=0 ; i<NTI ; i++) { fprintf(stderr, "+------"        ) ; } fprintf(stderr, "+\n") ;
  }
  fprintf(stderr, "SUCCESS\n") ;
#endif
  fprintf(stderr, "=============== data map creation ===============\n") ;
  gni = 128+65 ; gnj = 256+33 ; aspect = 2 ;
  size_t esize = sizeof(uint32_t) ;
fprintf(stderr, " new_zmap will change \n") ;
  goto fail ;     // new_zmap will change
//   map = new_zmap(gni, gnj, 1, 64, aspect, esize, 0);
  msg = "map == NULL" ;
  if(map == NULL) goto fail ;

  print_zmap(map, "gni = 128+65, gnj = 256+33, aspect = 2") ;
  msg = "map->fhead.zni != 3 || map->fhead.znj != 3" ;
  if(map->fhead.zni != 3 || map->fhead.znj != 3) goto fail ;
  znij = map->fhead.zni * map->fhead.znj ;

  fprintf(stderr, "size of preamble = %ld\n", (uint8_t *)&(map->fhead.signature) - (uint8_t *)&(map->mhead.signature)) ;
//   fprintf(stderr, "size of array_nd = %ld\n", sizeof(array_nd));
//   fprintf(stderr, "size of array_1d = %ld\n", sizeof(array_1d));
//   fprintf(stderr, "size of array_2d = %ld\n", sizeof(array_2d));
//   fprintf(stderr, "size of array_3d = %ld\n", sizeof(array_3d));
//   fprintf(stderr, "size of array_4d = %ld\n", sizeof(array_4d));
//   fprintf(stderr, "size of array_5d = %ld\n", sizeof(array_5d));

  msg = "bsize_zmap failed" ;
  if(znij != bsize_zmap(map, esize)) goto fail ;    // create sizes
  msg = "fillmem_zmap failed" ;
  if(znij != fillmem_zmap(map)) goto fail ;          // adjust map->mem
  zblocks *mem = map->mhead.mem ;
  znij = map->fhead.zni * map->fhead.znj ;
  fprintf(stderr, "size from old pointer table[%d] :", znij);
  for(i=0 ; i < znij ; i++) fprintf(stderr, "%6ld", mem[i+1] - mem[i]) ;
  fprintf(stderr, "\n");
  fprintf(stderr, "size from old sizes table  [%d] :", znij);
  for(i=0 ; i < znij ; i++) fprintf(stderr, "%6d", map->size[i]) ;
  fprintf(stderr, "\n");
  msg = "map->size[i] != (mem[i+1] - mem[i])" ;
  for(i=0 ; i < znij ; i++) if(map->size[i] != (mem[i+1] - mem[i])) goto fail ;
  fprintf(stderr, "SUCCESS\n") ;

  free_zmap(map, 0) ;             // partial free (only mem table)
  mem = mem_zmap(map, NULL, 0) ;  // reallocate mem table
  znij = map->fhead.zni * map->fhead.znj ;
  fprintf(stderr, "size from new pointer table[%d] :", znij);
  for(i=0 ; i < znij ; i++) fprintf(stderr, "%6ld", mem[i+1] - mem[i]) ;
  fprintf(stderr, "\n");
  fprintf(stderr, "size from old sizes table  [%d] :", znij);
  for(i=0 ; i < znij ; i++) fprintf(stderr, "%6d", map->size[i]) ;
  fprintf(stderr, "\n");
  for(i=0 ; i < znij ; i++) if(map->size[i] != (mem[i+1] - mem[i])) goto fail ;
  fprintf(stderr, "SUCCESS\n") ;

  fprintf(stderr, "=============== data map sizes reduce ===============\n") ;
  uint32_t oldsize = map->mhead.mem[znij] - map->mhead.mem[0] ;
  fprintf(stderr, "initial data size = %6d\n", oldsize) ;
  for(i=0 ; i<znij ; i++) map->size[i] -= 2 ;
  uint32_t newsize = repack_map(map) ;
  fprintf(stderr, "packed data size = %6d\n", newsize) ;
  if(newsize != oldsize - 2*znij) goto fail ;
  fprintf(stderr, "SUCCESS\n") ;
  fprintf(stderr, "size from new pointer table[%d] :", znij);
  for(i=0 ; i < znij ; i++) fprintf(stderr, "%6ld", mem[i+1] - mem[i]) ;
  fprintf(stderr, "\n");
  fprintf(stderr, "size from new sizes table  [%d] :", znij);
  for(i=0 ; i < znij ; i++) fprintf(stderr, "%6d", map->size[i]) ;
  fprintf(stderr, "\n");
  for(i=0 ; i < znij ; i++) if(map->size[i] != (mem[i+1] - mem[i])) goto fail ;
  fprintf(stderr, "SUCCESS\n") ;

  fprintf(stderr, "=============== data map sizes restore ===============\n") ;
  // restore packed stream pointers
  for(i=0 ; i<znij ; i++) map->size[i] += 2 ;
  newsize = resize_map(map) ;
  if(newsize == oldsize) fprintf(stderr, "SUCCESS\n") ;
#if 0
  fprintf(stderr, "=============== block limits ===============\n") ;
  fprintf(stderr, "blocks[%d,%d] => data[%4d,%4d]", map->fhead.zni, map->fhead.znj, map->fhead.gni, map->fhead.gnj) ;
  fprintf(stderr, ", first block along i is  %s"  , map->fhead.li0 > map->fhead.lni ? "longer" : "shorter") ;
  fprintf(stderr, ", first block along j is  %s\n", map->fhead.lj0 > map->fhead.lnj ? "longer" : "shorter") ;
  int32_t zx ;
  for(j = (int)map->fhead.znj ; j > 0 ; j--){
    ijr = map_block_limits(map, 0, 0) ;       // no more warning about possibility of ijr.j0 to be uninitialized
    for(i = 0 ; i < (int)map->fhead.zni ; i++){
      ijr = map_block_limits(map, i, j-1) ;
//       zx = Zindex_from_ij(i, j-1, map->fhead.zni, map->fhead.znj, map->fhead.aspect);
      zx = Z_map_index(map, i, j-1) ;
      fprintf(stderr, "data[%4d:%4d,%4d:%4d](Z %2d)  ", ijr.i0, ijr.in, ijr.j0, ijr.jn, zx) ;
    }
    fprintf(stderr, "j_range : %4d)\n", ijr.jn - ijr.j0 + 1);
  }
  for(i = 0 ; i < (int)map->fhead.zni ; i++){
    ijr = map_block_limits(map, i, 0) ;
    fprintf(stderr, "i_range : %4d                   ", ijr.in - ijr.i0 + 1);
  }
  fprintf(stderr, "\n");
#endif
#if 0
  fprintf(stderr, "=============== split array according to map ===============\n") ;
  array_2d a2d = array_2d_zero ;
//   new_array(&a2d, NULL, 4, 'U', map->fhead.gni, map->fhead.gnj) ;  // create 2D array, map->fhead.gni x map->fhead.gnj
  new_array(&a2d, NULL, 4, uint_data, map->fhead.gni, map->fhead.gnj) ;  // create 2D array, map->fhead.gni x map->fhead.gnj
  fill_array(&a2d) ;
  zmap *result = array_to_zmap(map, &a2d, NULL, NULL) ;
  if(result == NULL) goto fail ;
#endif
  goto success ;
}
