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
#include <string.h>

#include <rmn/data_map.h>
#include <rmn/array_nd.h>
#include <rmn/move_blocks.h>

#undef FAIL
static int StAtUs = 0 ;
#define FAIL(ERR,...) { StAtUs = ERR ; fprintf(stderr, __VA_ARGS__); goto fail ; }

#define NTI 10
#define NTJ 11
#define SF0  4

// copy block(s) of 32 bit elements from memory pointed to by data map into user space
// demo function for tests purposes
// map      [IN] : pointer to valid zmap struct
// block0   [IN] : first block to copy
// block_nb [IN] : number of blocks to copy
// out_    [OUT] : pointer to reception area
// size_out [IN] : size of reception area (in 32 bit units)
// return number of words copied of negative error codes
//
block_fn get_mapped_blocks ;    // make sure we have the right types in the function prototype
int32_t get_mapped_blocks(zmap *map, int block0, int block_nb, void *out_, size_t size_out){
  if(map == NULL) return -1 ;                            // no map ;
  if(out_ == NULL) return -4 ;                           // invalid destination
  if(block0 < 0) return -1 ;                             // invalid block number
  if(block_nb <= 0) return -1 ;                          // invalid number of blocks
  int max_blocks = map->fhead.zijk ;
  if(block0+block_nb > max_blocks) return -2 ;           // last block number exceeds available blocks
  struct{
    uint32_t *ptr ;
    uint64_t dummy ;
  } *local = (void *)&(map->mhead.get_args) ;            // point local arguments into proper place in zmap
  CT_ASSERT( sizeof(*local) == sizeof(map->mhead.get_args) , "bad getblock arguments struc size" )

  uint32_t *base = local->ptr ;                          // get base address for memory copy from zmap
  if(base == NULL) return -5 ;
  uint32_t size = 0 ;
  uint64_t offset = map->mhead.orng.bot[block0] ;        // block offset of first relative to base address (in 32 bit units)
  // consecutive blocks in tables are assumed to be consecutive in storage
  while(block_nb > 0){
    size = size + map->size[block0] ;                    // size of block to copy (in 32 bit units)
    block_nb-- ;
    block0++ ;
  }
  if(size_out < size) return -3 ;                        // not enough space for copy
  uint32_t *out = out_ ;                                 // destination address
  uint32_t *src = base + offset ;                        // source address
  for(uint32_t i=0 ; i<size ; i++){ out[i] = src[i] ; }

  return size ;                                          // number of 32 bit words copied
}

// pack unsigned 32 ->16 ;
int test_pack_block_3216(zmap *map, void *out_, void *in_, int ninj){
  if(map == NULL || ninj <= 0) return -1 ;
  uint32_t *in = in_ ;
  uint16_t *out = out_ ;
  if(out == NULL || in == NULL) return -1 ;
  struct{
    uint32_t unp ;
    uint32_t pak ;
    uint64_t dummy ;
  } *local = (void *)&(map->mhead.codec_args) ;            // point local arguments into proper place in zmap
  CT_ASSERT( sizeof(*local) == sizeof(map->mhead.codec_args) , "bad codec arguments struc size" )
  if(local->unp != 32 || local->pak != 16) return -1 ;
  for(int i=0 ; i<ninj ; i++){ out[i] = in[i] & 0xFFFF ; } ;
  return ninj * sizeof(uint16_t) ;
}

// unpack unsigned 32 ->16 ;
int test_unpack_block_1632(zmap *map, void *out_, void *in_, int ninj){
  if(map == NULL || ninj <= 0) return -1 ;
  uint16_t *in = in_ ;
  uint32_t *out = out_ ;
  if(out == NULL || in == NULL) return -1 ;
  struct{
    uint32_t unp ;
    uint32_t pak ;
    uint64_t dummy ;
  } *local = (void *)&(map->mhead.codec_args) ;            // point local arguments into proper place in zmap
  CT_ASSERT( sizeof(*local) == sizeof(map->mhead.codec_args) , "bad codec arguments struc size" )
  if(local->pak != 16 || local->unp != 32) return -1 ;
  for(int i=0 ; i<ninj ; i++){ out[i] = in[i] & 0xFFFF ; } ;
  return ninj * sizeof(uint16_t) ;
}

codec_fn test_codec_1632 ;
int test_codec_1632(zmap *map, void *out_, void *in_, int ninj, int encode){
  if(encode == 1){
    return test_pack_block_3216(map, out_, in_, ninj) ;
  }else{
    return test_unpack_block_1632(map, out_, in_, ninj) ;
  }
}

// #define NTI  4
// #define NTJ  3
// #define SF0  2
void  test_fill_offset(zmap *map){
  for(uint32_t i=map->fhead.zni * map->fhead.znj ; i<map->fhead.zijk ; i++) map->size[i] = map->fhead.zijk - i ;
  map->mhead.orng.bot[0] = 0 ;
  for(uint32_t i=1 ; i<(map->fhead.zijk)+1 ; i++){
    map->mhead.orng.bot[i] = map->mhead.orng.bot[i-1] + map->size[i-1] ;
  }
}

void  test_fill_size(int zni, int znj, fmap_block_size size[znj][zni], zmap *map){
  int li0 = map->fhead.li0, lni = map->fhead.lni, lj0 = map->fhead.lj0, lnj = map->fhead.lnj ;
  int i, j, sizei, sizej ;
  for(j=0, sizej=lj0 ; j<znj ; j++, sizej=lnj){
    for(i=0, sizei=li0 ; i<zni ; i++, sizei=lni){
      size[j][i] = sizei*sizej ;
    }
  }
}

// fill an array (gni x gnj) with known values
static void fill_data(int gni, int gnj, uint32_t data[gnj][gni]){
  if(gni < 256 && gnj < 256){
    for(int j=0 ; j<gnj ; j++){
      for(int i=0 ; i<gni ; i++){
        data[j][i] = (i<<8) | (j) ;
      }
    }
  }else{
    for(int j=0 ; j<gnj ; j++){
      for(int i=0 ; i<gni ; i++){
        data[j][i] = (i<<16) | (j) ;
      }
    }
  }
}

// check the contents of an array (gni x gnj) with known values
static uint32_t check_data(int gni, int gnj, uint32_t data[gnj][gni], int i0, int lni, int j0, int lnj){
  uint32_t errors = 0 ;
  uint32_t in = i0 + lni - 1, jn = j0 + lnj - 1 ;
  if(gni < 256 && gnj < 256){
    for(uint32_t j=j0 ; j<jn ; j++){
      for(uint32_t i=i0 ; i<in ; i++){
        if( data[j][i] != ((i<<8) | (j)) ) errors++ ;
      }
    }
  }else{
    for(uint32_t j=j0 ; j<jn ; j++){
      for(uint32_t i=i0 ; i<in ; i++){
        if( data[j][i] != ((i<<16) | (j)) ) errors++ ;
      }
    }
  }
  return errors ;
}

void put_block(uint32_t lni, uint32_t lnj, uint32_t blk[lnj][lni], uint32_t gni, uint32_t gnj, uint32_t dst[gnj][gni]){
  for(uint32_t j=0 ; j<lnj ; j++){
    for(uint32_t i=0 ; i<lni ; i++){
      dst[j][i] = blk[j][i] ;
    }
  }
}

void get_block(uint32_t lni, uint32_t lnj, uint32_t blk[lnj][lni], uint32_t gni, uint32_t gnj, uint32_t src[gnj][gni]){
  for(uint32_t j=0 ; j<lnj ; j++){
    for(uint32_t i=0 ; i<lni ; i++){
      blk[j][i] = src[j][i] ;
    }
  }
}

#undef MAX
#define MAX(A,B) ( ((A) > (B)) ? (A) : (B) )

static struct{
    uint32_t unp ;
    uint32_t pak ;
    uint64_t dummy ;
  } pack_args = {32, 16, 0} ;

void  fill_zmap_with_data(zmap *map, int gni, int gnj, uint32_t data[gnj][gni]){
  uint32_t lni, lnj, i0, j0, in, jn, i, j ;
  uint32_t zni = map->fhead.zni, znj = map->fhead.znj ;
  uint32_t maxi = MAX(map->fhead.li0, map->fhead.lni) ;
  uint32_t maxj = MAX(map->fhead.lj0, map->fhead.lnj) ;
  uint32_t block[maxi*maxj] ;
  uint16_t packed[maxi*maxj] ;
  int np ;
  codec_fn *codec = map->mhead.codec ;
  uint32_t *stream, bno ;
  fmap_block_size *sizes, size ;
  uint64_t *offsets ;

  stream = map->mhead.drng.bot ;
  sizes = map->size ;
  offsets = map->mhead.orng.bot ;
  offsets[0] = 0 ;
  bno = 0 ;

  fprintf(stderr, "sizeof(block) = %ld bytes, %ld elements\n", sizeof(block), sizeof(block)/sizeof(uint32_t));
  for(j=0, j0 = 0, lnj = map->fhead.lj0 ; j<znj ; j++, lnj=map->fhead.lnj){
    jn = j0 + lnj - 1 ;
    for(i=0, i0 = 0, lni = map->fhead.li0 ; i<zni ; i++, lni=map->fhead.lni){
      in = i0 + lni - 1 ;
      fprintf(stderr, "zblock[%d,%d] = array[%3d:%3d,%3d:%3d]", i, j, i0, in, j0, jn) ;
      get_block(lni, lnj, (void *)block, gni, gnj, (void *)(&data[j0][i0])) ;

      np = (*codec)(map, packed, block, lni * lnj, 1) ;             // pack
      fprintf(stderr, ", codec pack : nb = %d", np) ;
      if(np == -1) exit(1) ;
      size = np / sizeof(uint32_t) ;
      sizes[bno] = size ;
      offsets[bno+1] = offsets[bno] + size ;

      np = (*codec)(map, block, packed, lni * lnj, 0) ;             // unpack
      fprintf(stderr, ", codec unpack : nb = %d", np) ;
      if(np == -1) exit(1) ;

      put_block(lni, lnj, (void *)block, gni, gnj, (void *)(&data[j0][i0])) ;
      fprintf(stderr, ", sizes[%d] = %d\n\n", bno, sizes[bno]);
      i0 = in + 1 ;

      stream = stream + size ;
      bno++ ;
    }
    j0 = jn + 1 ;
  }
  for(i=bno ; i<map->fhead.zijk ; i++) { sizes[i] = 0 ; offsets[i+1] = offsets[i] ; } ;
}

int main(int argc, char **argv){
  (void)(argc) ; (void)(argv) ;  //  suppress unused argument warning
  int i, j ;
//   int x[NTI], y[NTI] ;
//   index_pair ijp ;
  index_range irange ;
//   ij_range ijr ;
  char *msg = "" ;
  int32_t gni, gnj, gnk, bsize, aspect, bsizej ;
  int32_t bextra = 3 ;
  int32_t mextra = 4 ;

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

    uint32_t blocks = filemap_blocks(gni, gnj, gnk, bsize, bsizej);
    fprintf(stderr, "array[%d,%d,%d], block size = [%d:%d], nblocks = %d", gni, gnj, gnk, bsize, bsizej, blocks) ;
    fprintf(stderr, ", file map size = %ld words\n", filemap_needed_bytes(gni, gnj, gnk, bsize, bsizej, bextra)/sizeof(uint32_t)) ;
    uint32_t nwords = filemap_needed_bytes(gni, gnj, gnk, bsize, bsizej, bextra)/sizeof(uint32_t) ;
    if(filemap_needed_words(gni, gnj, gnk, bsize, bsizej, bextra) != nwords) {
      fprintf(stderr, "ERROR: filemap_needed_words | filemap_needed_bytes mismatch\n");
      goto fail ;
    }

    zmap *zp = create_file_zmap(nwords+mextra, nwords+mextra+100) ;         // mextra, 100 words of data
    if(fmap_invalid(zp) == 0) goto fail ;           // fmap is invalid at this point
    fmap_init(zp, gni, gnj, gnk, bsize, bsizej, NULL, mextra, bextra);   // initialize fmap part with bextra extra blocks
    zp->fhead.extra = mextra ;                                    // set extra to mextra words
    fmap_print(zp, "zp") ;
    if(fmap_invalid(zp) != 0) goto fail ;           // fmap must be valid at this point
    fprintf(stderr, "    fmap element size = %ld\n", ELEMENT_SIZE(zp->mhead.frng)) ;
    fprintf(stderr, "    filemap words = %d, zmap at %p, fmap at %p, blocks[%d:%d]\n",
            filemap_words(zp), &(zp->mhead.signature), &(zp->fhead.signature), zp->fhead.zni, zp->fhead.znj) ;
    fprintf(stderr, "\n");
    zmap_print(zp, "created file zp") ;
    fprintf(stderr, "\n");
    update_file_zmap(zp);
    zmap_print(zp, "updated file zp") ;
    fprintf(stderr, "-----------\n");

    free(zp) ;
    zp = create_zmap(gni, gnj, gnk, bsize, aspect, 2*sizeof(uint32_t), mextra, bextra, 3*bextra*sizeof(uint32_t)) ;
    fprintf(stderr, "data map length = %ld words, record length = %ld words\n", FILEMAP_WORDS(zp), RECORD_WORDS(zp)) ;
    fprintf(stderr, "\n");
    free(zp) ;
    zp = create_zmap(gni, gnj, gnk, bsize, aspect, 2*sizeof(uint32_t), mextra, 0, 3*bextra*sizeof(uint32_t)) ;
    fprintf(stderr, "data map length = %ld words, record length = %ld words\n", FILEMAP_WORDS(zp), RECORD_WORDS(zp)) ;
    fprintf(stderr, "\n");
    free(zp) ;
    zp = create_zmap(gni, gnj, gnk, bsize, aspect, 2*sizeof(uint32_t), mextra, 0, 0) ;
    fprintf(stderr, "\n");
    free(zp) ;
  }

  fprintf(stderr, "=============== simulated file test ===============\n") ;
  zmap *zp0, *zp1, *zp2 ;
  int status ;
  uint32_t map_words, rec_words, zmap_words, data_words, errors ; 
  gni = 129 ; gnj = 97 ; gnk = 1 ; bsize = 64 ; aspect = 1 ; mextra = 2 ; bextra = 3 ;
  uint32_t *data = (uint32_t *)malloc(gni * gnj * sizeof(uint32_t *)) ;
  if(data == NULL) goto fail;

  fill_data(gni, gnj, (void *)data) ;                            // create and check reference array
  errors = check_data(gni, gnj, (void *)data, 0, gni, 0, gnj) ;
  if( errors != 0) FAIL(1, "ERROR : %d error(s) in data\n", errors)

  // create and populate the data_map + data struct
  zp0 = create_zmap(gni, gnj, gnk, bsize, aspect, 2*sizeof(uint32_t), mextra, bextra, 3*bextra*sizeof(uint32_t)) ;
//   zp0->mhead.codec_args = *((arg128 *) &pack_args ) ;
  SET_CODEC_ARGS(zp0, pack_args) ;
  SET_CODEC_FN(zp0, test_codec_1632) ;
//   memcpy( &(zp0->mhead.codec_args), &pack_args, sizeof(zp0->mhead.codec_args) );    // set packing codec arguments
  fill_zmap_with_data(zp0, gni, gnj, (void *)data) ;
  errors = check_data(gni, gnj, (void *)data, 0, gni, 0, gnj) ;
  if( errors != 0){ FAIL(1, "ERROR : %d error(s) in get/put\n", errors) ; }

  zmap_print(zp0, "zp0+") ;
  fmap_print(zp0, "zp0+") ;

//   test_fill_size(zp0->fhead.zni, zp0->fhead.znj, (void *)zp0->size, zp0) ;
//   test_fill_offset(zp0) ;

  map_words = FILEMAP_WORDS(zp0) ; rec_words = RECORD_WORDS(zp0) ; zmap_words = ZMAP_WORDS(zp0) ; data_words = DATA_WORDS(zp0) ;
  fprintf(stderr, "zp0 data map length = %d , data length = %d , record length = %d , zmap length = %d \n", map_words, data_words, rec_words, zmap_words) ;
  fprintf(stderr, "zp0 reference : ARRAY_BLOCKS = %d, TOTAL_BLOCKS = %d\n", ZMAP_ARRAY_BLOCKS(zp0), ZMAP_TOTAL_BLOCKS(zp0)) ;
  fprintf(stderr, "    data map block sizes and offsets\n") ;
  for(uint32_t i=0 ; i<ZMAP_TOTAL_BLOCKS(zp0) ; i++){ fprintf(stderr, "%6d ", BLOCK_WORDS(zp0,i)) ; } ;
  fprintf(stderr, "\n");
  for(uint32_t i=0 ; i<ZMAP_TOTAL_BLOCKS(zp0)+1 ; i++){ fprintf(stderr, "%6ld ", BLOCK_OFFSET(zp0,i)) ; } ;
  fprintf(stderr, "\n");
  for(uint32_t i=0 ; i<ZMAP_TOTAL_BLOCKS(zp0) ; i++){ fprintf(stderr, "%6ld ", BLOCK_OFFSET(zp0,i+1) - BLOCK_OFFSET(zp0,i) ) ; } ;
  fprintf(stderr, "\n");
  fprintf(stderr, "\n");
goto success ;
  map_words = FILEMAP_WORDS(zp0) ; rec_words = RECORD_WORDS(zp0) ; zmap_words = ZMAP_WORDS(zp0) ; data_words = DATA_WORDS(zp0) ;
  zp1 = create_file_zmap(map_words, rec_words) ;
  zmap_print(zp1, "zp1") ;
  map_words  = FILEMAP_WORDS(zp1) ;
  data_words = DATA_WORDS(zp1) ;
  rec_words  = RECORD_WORDS(zp1) ;
  zmap_words = ZMAP_WORDS(zp1) ;
  fprintf(stderr, "zp1 data map length = %d , data length = %d , record length = %d , zmap length = %d \n", map_words, data_words, rec_words, zmap_words) ;
  fprintf(stderr, "\n") ;
  memcpy(&(zp1->fhead), &(zp0->fhead), map_words * sizeof(uint32_t)) ;   // simulate read from file
  status = update_file_zmap(zp1) ;
  if(status) fprintf(stderr, "ERROR: update_file_zmap %d\n", status) ;
  zp1->mhead.codec = test_codec_1632 ;
  zp1->mhead.get_blocks = get_mapped_blocks ;
  zmap_print(zp1, "zp1+") ;
  fmap_print(zp1, "zp1+") ;
  fprintf(stderr, "\n") ;

  map_words = FILEMAP_WORDS(zp0) ; rec_words = RECORD_WORDS(zp0) ; zmap_words = ZMAP_WORDS(zp0) ; data_words = DATA_WORDS(zp0) ;
  zp2 = create_file_zmap(map_words, 0) ;     // map only, no data
  zmap_print(zp2, "zp2") ;
  map_words  = FILEMAP_WORDS(zp2) ;
  data_words = DATA_WORDS(zp2) ;
  rec_words  = RECORD_WORDS(zp2) ;
  zmap_words = ZMAP_WORDS(zp2) ;
  fprintf(stderr, "zp2 data map length = %d , data length = %d , record length = %d , zmap length = %d \n", map_words, data_words, rec_words, zmap_words) ;
// //   fmap_print(zp2, "zp2") ;

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
      irange = index_limits(0, ln, ln0) ;
      fprintf(stderr, " first block [%4d,%4d] (size = %3d),", irange.ix0, irange.ixn, irange.ixn-irange.ix0+1) ;
    }
    for(j=0 ; j<NTI ; j++, lb=ln){   // loop over blocks
//       ijp = b_limits(j, ln, ln0) ;
      irange = index_limits(j, ln, ln0) ;
//       if(ijp.i != i0+1 || ijp.j != i0+lb){
      if(irange.ix0 != i0+1 || irange.ixn != i0+lb){
        fprintf(stderr, "ERROR: block %d limits, expected [%d,%d], got [%d,%d]\n", j, i0+1, i0+lb, irange.ix0, irange.ixn) ;
        goto fail ;
      }
      for(i=0 ; i<lb ; i++){
        i0++ ;
        l = block_index(i0, ln, ln0) ;
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

  goto success ;
}
