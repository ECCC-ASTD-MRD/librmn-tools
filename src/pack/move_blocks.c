// Hopefully useful code for C (memory block movers)
// Copyright (C) 2022  Recherche en Prevision Numerique
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

#include <stdint.h>

#include <rmn/identify_compiler_c.h>

// SIMD does not seem to be useful any more for these funtions with most compilers
#undef WITH_SIMD

#define VERBOSE_SIMD

// replace calls to Intel intrinsics with calls to SIMD functions
// (ignored if USE_INTEL_SIMD_INTRINSICS is defined)
#define ALIAS_INTEL_SIMD_INTRINSICS
// use C version of the SIMD intrinsics (llvm clang 19)
#define USE_INTEL_SIMD_INTRINSICS_FALSE

#if defined(COMPILER_IS_CLANG) && (__clang_major__ < 19)
// aocc clang seems to do a poor vectorizing job when using the C version of the SIMD intrinsics
#define USE_INTEL_SIMD_INTRINSICS   // for vector SIMD functions
#endif

#if defined(COMPILER_IS_GCC)
// give an explicit hint to the gcc optimizer
#pragma GCC optimize "tree-vectorize"
// gcc seems to do a poor vectorizing job when computing "properties"
#define USE_INTEL_SIMD_INTRINSICS   // for vector SIMD functions
#endif

#if defined(COMPILER_IS_ICX)
// icx seems to do a poor vectorizing job when using the C version of the SIMD intrinsics
#define USE_INTEL_SIMD_INTRINSICS    // for vector SIMD functions
#endif

#if defined(COMPILER_IS_ICC)
// icc seems to do a poor vectorizing job
#define USE_INTEL_SIMD_INTRINSICS    // for vector SIMD functions
#define WITH_SIMD                    // re-activate SIMD intrinsics everywhere
#endif

#if defined(COMPILER_IS_PGI)
// nvc seems to do a poor vectorizing job when using the C version of the SIMD intrinsics
#define USE_INTEL_SIMD_INTRINSICS   // for vector SIMD functions
#endif

#include <rmn/simd_functions.h>
#include <rmn/move_blocks.h>

#define MIN(OLD,NEW) OLD = (NEW < OLD) ? NEW : OLD
#define MAX(OLD,NEW) OLD = (NEW > OLD) ? NEW : OLD

// compute what is necessary to split a data block along one of its dimensions
// gdim   [IN] : size of data block along a dimension
// ldim   [IN] : desired size of sub-blocks along a dimension
// nsub  [OUT] : number of sub-blocks needed
// ldim0 [OUT] : size of first/last sub-block along that dimension
// normally, ldim/2 <= ldim0 < ldim + ldim/2 (extra small blocks are deemed undesirable)
// the only exception would be nsub ==1 because gdim < ldim/2
// the first block may be longer than ldim, the last block may be shorter than ldim
void split_block_dimension(uint32_t gdim, uint32_t ldim, uint32_t *nsub, uint32_t *ldim0){
  uint32_t nparts = gdim / ldim ;
  uint32_t extra = gdim - (nparts * ldim) ;   // modulo( gdim , ldim )
  if(extra < ldim/2){
    *ldim0 = ldim + extra ;    // < ldim + ldim/2
    *nsub = nparts ;
  }else{
    *ldim0 = extra ;           // >= ldim / 2
    *nsub = nparts + 1 ;       // need one more sub-block
  }
}

// demo diagnostic function for split_and_process
// data[nj][lni] : sub-block
// lni           : storage length of data rows
// ni            : number of useful values in data rows
// nj            : number of rows
// fnargs        : reference point in sub-block, if NULL, data[0][0] is used
// TODO add properties argument (it will just be checked for validity)
static int diag_fn(int lni, int ni, int nj, block_properties *bp, void *data, sfn_args *fnargs){
  void *args = fnargs ? fnargs : data ;
  uint64_t offset  = ((char *)data - (char *)args)/sizeof(uint32_t) ;
  uint64_t offsetj = offset / lni ;
  uint64_t offseti = offset - (offsetj * lni) ;
  fprintf(stderr, "lni = %3d, ni = %3d, nj = %3d range = (%4ld,%4ld) (%4ld,%4ld), type = %s\n",
          lni, ni, nj, offseti, offsetj, offseti+ni-1, offsetj+nj-1, printable_type[bp->kind]) ;
//   fprintf(stderr, "lni = %3d, ni = %3d, nj = %3d, address = %p, args = %p, range = (%4ld,%4ld) (%4ld,%4ld)\n",
//           lni, ni, nj, data, args, offseti, offsetj, offseti+ni-1, offsetj+nj-1) ;
  return 0 ;
}

// VLA (variable Length Array) style version
// lgni   [IN] : storage length of rows in array
// gni    [IN] : number of useful elements in an array row
// gnj    [IN] : number of rows in array
// array  [IN] : data array (lgni * gnj elements)
// ni     [IN] : number of elements in sub-block rows
// nj     [IN] : number of rows in sub-block
// fnptr  [IN] : function to be called to process sub-block
//               if NULL, private diagnostic function will be called
// fnargs [IN] : argument list to be passed to function
// return error code from fn
// TODO add data type to arguments
// TODO adjust for longer first block / shorter last block strategy
static int split_and_process_(uint32_t lgni, uint32_t gni, uint32_t gnj, int_or_float datatype, uint32_t array[gnj][lgni], int ni, int nj, sfn_ptr fn, sfn_args *fnargs){
  uint32_t ni0, nj0, nbi, nbj, i, j, deltai, deltaj ;
  int status ;
  block_properties bp ;

  split_block_dimension(gni, ni, &nbi, &ni0) ;
  split_block_dimension(gnj, nj, &nbj, &nj0) ;

  if(fn == NULL){           // use private diagnostic function
    fn = diag_fn ;          // point to diagnostic function
    fnargs = NULL ;         // address of array[j][i] will be used
  }

  deltaj = nj0 ;            // if nj0 > nj
  for(j=0 ; j<gnj ; j+=deltaj , deltaj=nj){
    // if j+deltaj > gnj, deltaj = gnj-j
    deltai = ni0 ;          // if ni0 > ni
    for(i=0 ; i<gni ; i+=deltai , deltai = ni){
      // if i+deltai > gni, deltai = gni-i
      // extract sub_block[deltaj][deltai]
      // move_word32_block(&(array[j][i]), gni, sub_block, deltai, deltai, deltaj, datatype, &bp) ;
      // status = (*fn)(deltai, deltai, deltaj, sub_block, fnargs) ;
      bp.kind = datatype ;
      status = (*fn)(gni, deltai, deltaj, &bp, &(array[j][i]), fnargs) ;
      if(status != 0) return status ;
    }
  }
  return 0 ;
}

// lgni   [IN] : storage length of rows in array
// gni    [IN] : number of useful elements in an array row
// gnj    [IN] : number of rows in array
// array  [IN] : data array (lgni * gnj elements)
// ni     [IN] : number of elements in sub-block rows
// nj     [IN] : number of rows in sub-block
// fnptr  [IN] : function to be called to process sub-block
//               if NULL, private diagnostic function will be called
// fnargs [IN] : argument list to be passed to function
// call VLA style version, return its status
// TODO add data type to arguments
int split_and_process(void *array, uint32_t lgni, uint32_t gni, uint32_t gnj, int_or_float datatype, int ni, int nj, sfn_ptr fn, sfn_args *fnargs){
  return split_and_process_(lgni, gni, gnj, datatype, array, ni, nj, fn, fnargs) ;
}

// fold 8 value vectors for max / min / max_abs / min_abs into scalars and store into bp
// this works whether int or float data was analyzed
// bp  [OUT] : pointer to block properties struct (max / min / max_abs / min_abs) (IGNORED if NULL)

static inline void fold_properties_s(__m256i vmaxs, __m256i vmins, block_properties *bp){
  int32_t ti[8], i, *p = ti ;

  // storeu_v256( (__v256i *tu , vminu ) style code used to cause an internal error with nvc compiler
  storeu_v256( (__m256i *)p , vmaxs ) ;
  for(i=0 ; i<8 ; i++) ti[0] = (ti[i] > ti[0]) ? ti[i] : ti[0] ;
  bp->maxs.i = ti[0] ;
  storeu_v256( (__m256i *)p , vmins ) ;
  for(i=0 ; i<8 ; i++) ti[0] = (ti[i] < ti[0]) ? ti[i] : ti[0] ;
  bp->mins.i = ti[0] ;
}

static inline void fold_properties_u(__m256i vmaxu, __m256i vminu, block_properties *bp){
  int32_t i ;
  uint32_t tu[8], *p=tu ;

  // storeu_v256( (__v256i *tu , vminu ) style code used to cause an internal error with nvc compiler
  storeu_v256( (__m256i *)p , vmaxu ) ;
  for(i=0 ; i<8 ; i++) tu[0] = (tu[i] > tu[0]) ? tu[i] : tu[0] ;
  bp->maxu.u = tu[0] ;
  storeu_v256( (__m256i *)p , vminu ) ;
  for(i=0 ; i<8 ; i++) tu[0] = (tu[i] < tu[0]) ? tu[i] : tu[0] ;
  bp->minu.u = tu[0] ;
}

static inline void fold_properties(__v256i vmaxs, __v256i vmins, __v256i vmaxu, __v256i vminu, block_properties *bp){
  fold_properties_s(vmaxs, vmins, bp) ;
  fold_properties_u(vmaxu, vminu, bp) ;
}

// transform a float into a fake signed integer (comparison order preserving)
int32_t fake_int(float f){
  iuf32_t iuf ;
  iuf.f = f ;
  return (iuf.i & 0x7FFFFFFF) ^ (iuf.i >> 31) ;
}
// restore float from fake integer representing float
float unfake_float(int32_t fake){
  iuf32_t iuf ;
  iuf.i = ((fake >> 31) ^ fake) | (fake & 0x80000000) ;
  return iuf.f ;
}

// move a block (ni x nj) of 32 bit integers from src and store it into blk
// no block properties are computed
// src  [IN] : integer array to extract data from (NON CONTIGUOUS storage)
// lnis [IN] : row storage size in src
// dst [OUT] : array to put extracted data into (NON CONTIGUOUS storage)
// lnid [IN] : row storage size in dst
// ni   [IN] : row size (row storage size in blk)
// nj   [IN] : number of rows
// bp   [IN] : pointer to block properties struct (min / max / min abs) (IGNORED if NULL)
// return number of values processed
// bp is really expected to be NULL, as no properties are computed
// kind is set ro raw data, all properties are set to 0 if bp is not NULL
int move_mem32_block(void *restrict src, int lnis, void *restrict dst, int lnid, int ni, int nj, block_properties *bp){
  uint32_t *restrict d = (uint32_t *) dst ;
  uint32_t *restrict s = (uint32_t *) src ;
  int32_t ninj = ni * nj ;

  if(lnis <= 0 || lnid <= 0 || ni <= 0 || nj <= 0){
    fprintf(stderr, "ERROR move_mem32_block : lnis = %d, lnid = %d, ni = %d, nj = %d\n", lnis, lnid, ni, nj);
    return -1 ;
  }
  if(bp != NULL) {
    bp->kind   = raw_data ;
    bp->mins.u = bp->maxs.u = bp->minu.u = bp->maxu.u = 0 ;
    bp->zeros = -1 ;    // consistent with other movers (for now)
  }

  if(ni < 8){
    while(nj--){
      switch(ni & 7){   // switch on row length, fall through
        //       copy value
        case 7 : d[6] = s[6] ;
        case 6 : d[5] = s[5] ;
        case 5 : d[4] = s[4] ;
        case 4 : d[3] = s[3] ;
        case 3 : d[2] = s[2] ;
        case 2 : d[1] = s[1] ;
        case 1 : d[0] = s[0] ;
        case 0 : d += lnid ; s += lnis ;   // pointers to next row
      }
    }
  }else{
    __v256i vdata ;
    int ni7, n ;
    ni7 = (ni & 7) ;               // modulo(ni , 8)
    while(nj--){
      uint32_t *s0, *d0 ;
      n = ni ; s0 = s ; d0 = d ;
      if(ni7){                                   // first slice with less thatn 8 elements
        vdata = loadu_v256((__v256i *)s0) ;      // load data from source array
        storeu_v256((__v256i *)d0, vdata) ;      // store into destination array (CONTIGUOUS)
        n -= ni7 ; s0 += ni7 ; d0 += ni7 ;       // bump count and pointers
      }
      while(n > 7){                              // following slices with 8 elements
        vdata = loadu_v256((__v256i *)s0) ;      // load data from source array
        storeu_v256((__v256i *)d0, vdata) ;      // store into destination array (CONTIGUOUS)
        n -= 8 ; s0 += 8 ; d0 += 8 ;
      }
      s += lnis ; d += lnid ;                       // pointers to next row
    }
  }

  return ninj ;
}

// move a block (ni x nj) of 32 bit elements from src and store it into blk,
// compute moved block min/max properties if bp is not NULL
// src  [IN] : array to extract data from (NON CONTIGUOUS storage)
// lnis [IN] : row storage size in src
// dst [OUT] : array to put extracted data into (NON CONTIGUOUS storage)
// lnid [IN] : row storage size in dst
// ni   [IN] : row size (row storage size in blk)
// nj   [IN] : number of rows
// bp  [OUT] : pointer to block properties struct (min / max / min abs) (IGNORED if NULL)
// return number of values processed
int move_data32_block(void *restrict src, int lnis, void *restrict dst, int lnid, int ni, int nj, block_properties *bp){
  int32_t *restrict s = (int32_t *) src ;
  int32_t *restrict d = (int32_t *) dst ;
  int32_t ninj = ni * nj ;

  if(bp == NULL) return move_mem32_block(src, lnis, dst, lnid, ni, nj, NULL) ;

  if(lnis <= 0 || lnid <= 0 || ni <= 0 || nj <= 0){
    fprintf(stderr, "ERROR move_data32_block : lnis = %d, lnid = %d, ni = %d, nj = %d\n", lnis, lnid, ni, nj);
    return -1 ;
  }

  bp->zeros  = -1 ;          // initialize for failure
  bp->kind   = bad_data ;

  if(ni  <  8) {
    int32_t maxs = 0x80000000, mins = 0x7FFFFFFF, t ;
    uint32_t minu = 0x7FFFFFFF, maxu = 0 ;
    while(nj--){
      switch(ni & 7){   // switch on row length
        //       copy value        signed min    signed max    unsigned min             unsigned max
        case 7 : d[6] = t = s[6] ; MIN(mins,t) ; MAX(maxs,t) ; MIN(minu, (uint32_t)t) ; MAX(maxu, ((uint32_t)t)) ;
        case 6 : d[5] = t = s[5] ; MIN(mins,t) ; MAX(maxs,t) ; MIN(minu, (uint32_t)t) ; MAX(maxu, ((uint32_t)t)) ;
        case 5 : d[4] = t = s[4] ; MIN(mins,t) ; MAX(maxs,t) ; MIN(minu, (uint32_t)t) ; MAX(maxu, ((uint32_t)t)) ;
        case 4 : d[3] = t = s[3] ; MIN(mins,t) ; MAX(maxs,t) ; MIN(minu, (uint32_t)t) ; MAX(maxu, ((uint32_t)t)) ;
        case 3 : d[2] = t = s[2] ; MIN(mins,t) ; MAX(maxs,t) ; MIN(minu, (uint32_t)t) ; MAX(maxu, ((uint32_t)t)) ;
        case 2 : d[1] = t = s[1] ; MIN(mins,t) ; MAX(maxs,t) ; MIN(minu, (uint32_t)t) ; MAX(maxu, ((uint32_t)t)) ;
        case 1 : d[0] = t = s[0] ; MIN(mins,t) ; MAX(maxs,t) ; MIN(minu, (uint32_t)t) ; MAX(maxu, ((uint32_t)t)) ;
        case 0 : d += lnid ; s += lnis ;   // pointers to next row
      }
    }
    bp->maxs.i = maxs ; bp->mins.i = mins ; bp->minu.u = minu ; bp->maxu.u = maxu ;
  }else{      // (ni  <  8)
    __v256i vmaxs, vmins, vmaxu, vminu, vdata, v1111 ;
    int32_t *s0, *d0 ;
    int ni7, n ;

    v1111 = ones_v256() ;
    vmins = srli_v8i(v1111, 1)  ;  // 0x7FFFFFFF  huge positive
    vmaxs = slli_v8i(v1111, 31) ;  // 0x80000000  huge negative
    vminu = v1111 ;                // 0xFFFFFFFF  huge positive
    vmaxu = zero_v256() ;
    ni7 = (ni & 7) ;               // modulo(ni , 8)
    while(nj--){                                 // loop over rows
      n = ni ; s0 = s ; d0 = d ;
      if(ni7){                                   // first slice with less thatn 8 elements
        vdata = loadu_v256((__v256i *)s0) ;      // load data from source array (CONTIGUOUS)
        storeu_v256((__v256i *)d0, vdata) ;      // store into destination array (CONTIGUOUS)
        vminu = min_v8u(vminu, vdata) ;          // minimum absolute value
        vmaxu = max_v8u(vmaxu, vdata) ;          // max value with data treated as UNSIGNED
        vmaxs = max_v8i(vmaxs, vdata) ;          // maximum signed value
        vmins = min_v8i(vmins, vdata) ;          // minimum signed value
        n -= ni7 ; s0 += ni7 ; d0 += ni7 ;       // bump count and pointers
      }
      while(n > 7){                              // following slices with 8 elements
        vdata = loadu_v256((__v256i *)s0) ;      // load data from source array (CONTIGUOUS)
        storeu_v256((__v256i *)d0, vdata) ;      // store into destination array (CONTIGUOUS)
        vminu = min_v8u(vminu, vdata) ;          // min value with data treated as UNSIGNED
        vmaxu = max_v8u(vmaxu, vdata) ;          // max value with data treated as UNSIGNED
        vmaxs = max_v8i(vmaxs, vdata) ;          // maximum signed value
        vmins = min_v8i(vmins, vdata) ;          // minimum signed value
        n -= 8 ; s0 += 8 ; d0 += 8 ;
      }
      s += lnis ; d += lnid ;                    // pointers to next row (can be NON CONTIGUOUS)
    }
    fold_properties(vmaxs, vmins, vmaxu, vminu, bp) ; // fold results into a single scalar
  }      // (ni  <  8)
  bp->kind   = raw_data ;
// fprintf(stderr,"move_data32_block : mins = %8.8x, maxs = %8.8x, minu = %8.8x, maxu = %8.8x\n",bp->mins.u, bp->maxs.u, bp->minu.u, bp->maxu.u);
  return ninj ;
}

void set_block_properties(block_properties *bp, int_or_float datatype){
  if(bp == NULL) return ;
  if(datatype == any_data) datatype = bp->kind ;
  if(datatype == float_data){
    if(bp->maxs.i < 0){           // all numbers are negative
      float max = bp->minu.f ;
      float min = bp->maxu.f ;
      bp->mins.f =  min ;
      bp->maxs.f =  max ;
      bp->minu.f = -min ;
      bp->maxu.f = -max ;
    }else if(bp->mins.i < 0) {    // negative and non negative numbers
      float max = bp->maxs.f ;    // most positive value
      float min = bp->maxu.f ;    // most negative value
      float mins = bp->mins.f ;   // negative value closest to zero
      float minu = bp->minu.f ;   // positive value closest to zero
      bp->mins.f =  min ;
      bp->maxs.f =  max ;
      bp->minu.f = (minu < (-mins)) ? minu : (-mins) ;
      bp->maxu.f = ((max > (-min)) ? max : (-min) ) ;        // max is positive, is |max| > |min| ?
    }
    bp->kind = float_data ;
  }else if(datatype == int_data){
    if(bp->maxs.i < 0){           // all numbers are negative
      uint32_t minu = -bp->maxu.i ;
      uint32_t maxu = -bp->minu.i ;
      bp->minu.u = minu ;
      bp->maxu.u = maxu ;
    }else if(bp->mins.i < 0) {    // negative and non negative numbers
      uint32_t max1 = bp->maxs.i ;  // largest positive value
      int64_t max2  = bp->mins.i ;  // largest negative value
      max2 = -max2 ;
      bp->minu.u = 0 ;
      bp->maxu.u = ((max1 > max2) ? max1 : max2 ) ;
    }
    bp->kind = int_data ;
  }else if(datatype == uint_data){
    bp->kind = uint_data ;
    bp->maxs.u = bp->mins.u = 0 ;
  }else if(datatype == raw_data){
    bp->kind = raw_data ;
  }else{
    bp->kind = bad_data ;
  }
}

// move a block (ni x nj) of 32 bit floats from src to dst, set moved block properties
// src  [IN] : float array to extract data from (NON CONTIGUOUS storage)
// dst [OUT] : array to put extracted data into (NON CONTIGUOUS storage)
// ni   [IN] : row size
// lnis [IN] : row storage size in src
// lnid [IN] : row storage size in dst
// nj   [IN] : number of rows
// bp  [OUT] : pointer to block properties struct (min / max / min abs) (IGNORED if NULL)
// return number of values processed
int move_float_block(float *restrict src, int lnis, void *restrict dst, int lnid, int ni, int nj, block_properties *bp){

  if(bp == NULL) return move_mem32_block(src, lnis, dst, lnid, ni, nj, NULL) ;

  int rc = move_data32_block(src, lnis, dst, lnid, ni, nj, bp) ;
// fprintf(stderr,"move_float_block     : mins = %8.8x, maxs = %8.8x, minu = %8.8x, maxu = %8.8x\n",bp->mins.u, bp->maxs.u, bp->minu.u, bp->maxu.u);
  if(bp->maxs.i < 0){           // all numbers are negative
    float max = bp->minu.f ;
    float min = bp->maxu.f ;
    bp->mins.f =  min ;
    bp->maxs.f =  max ;
    bp->minu.f = -min ;
    bp->maxu.f = -max ;
  }else if(bp->mins.i < 0) {    // negative and non negative numbers
    float max = bp->maxs.f ;    // most positive value
    float min = bp->maxu.f ;    // most negative value
    float mins = bp->mins.f ;   // negative value closest to zero
    float minu = bp->minu.f ;   // positive value closest to zero
    bp->mins.f =  min ;
    bp->maxs.f =  max ;
    bp->minu.f = (minu < (-mins)) ? minu : (-mins) ;
    bp->maxu.f = ((max > (-min)) ? max : (-min) ) ;        // max is positive, is |max| > |min| ?
  }
  bp->kind = float_data ;
  return rc ;
}

// move a block (ni x nj) of 32 bit integers from src and store it into blk, set moved block properties
// src  [IN] : integer array to extract data from (NON CONTIGUOUS storage)
// lnis [IN] : row storage size in src
// dst [OUT] : array to put extracted data into (NON CONTIGUOUS storage)
// lnid [IN] : row storage size in dst
// ni   [IN] : row size (row storage size in blk)
// nj   [IN] : number of rows
// bp  [OUT] : pointer to block properties struct (min / max / min abs) (IGNORED if NULL)
// return number of values processed
int move_int32_block(int32_t *restrict src, int lnis, void *restrict dst, int lnid, int ni, int nj, block_properties *bp){

  if(bp == NULL) return move_mem32_block(src, lnis, dst, lnid, ni, nj, NULL) ;

  int rc = move_data32_block(src, lnis, dst, lnid, ni, nj, bp) ;
  if(bp == NULL) return rc ;
// fprintf(stderr,"move_int32_block      : mins = %8.8x, maxs = %8.8x, minu = %8.8x, maxu = %8.8x\n",bp->mins.u, bp->maxs.u, bp->minu.u, bp->maxu.u);
  if(bp->maxs.i < 0){           // all numbers are negative
    uint32_t minu = -bp->maxu.i ;
    uint32_t maxu = -bp->minu.i ;
    bp->minu.u = minu ;
    bp->maxu.u = maxu ;
  }else if(bp->mins.i < 0) {    // negative and non negative numbers
    uint32_t max1 = bp->maxs.i ;  // largest positive value
    int64_t max2  = bp->mins.i ;  // largest negative value
    max2 = -max2 ;
    bp->minu.u = 0 ;
    bp->maxu.u = ((max1 > max2) ? max1 : max2 ) ;
  }
  bp->kind = int_data ;
  return rc ;
}

// same as above but for unsigned integers
// maxs and mins are components of bp are meanigless and set to 0
int move_uint32_block(int32_t *restrict src, int lnis, void *restrict dst, int lnid, int ni, int nj, block_properties *bp){
  if(bp == NULL) return move_mem32_block(src, lnis, dst, lnid, ni, nj, NULL) ;
  int rc = move_data32_block(src, lnis, dst, lnid, ni, nj, bp) ;
  if(bp) {
//     fprintf(stderr,"move_uint32_block     : mins = %8.8x, maxs = %8.8x, minu = %8.8x, maxu = %8.8x\n",bp->mins.u, bp->maxs.u, bp->minu.u, bp->maxu.u);
    bp->kind = uint_data ;
    bp->maxs.u = bp->mins.u = 0 ;
  }
  return rc ;
}

// move a block (ni x nj) of 32 bit items from src to dst and compute properties
// src   [IN] : array the data comes from
// lnis  [IN] : row storage size of src
// dst  [OUT] : array to receive data
// lnid  [IN] : row storage size of dst
// ni    [IN] : row size (row storage size of blk)
// nj    [IN] : number of rows
// dtype [IN] : data type int_data / uint_data / float_data / raw_data
// bp   [OUT] : pointer to block properties struct (min / max / min abs) (IGNORED if NULL)
// return number of values processed
int move_word32_block(void *restrict src, int lnis, void *restrict dst, int lnid, int ni, int nj, int_or_float datatype, block_properties *bp){
  if(datatype == float_data && bp != NULL){
    return move_float_block(src, lnis, dst, lnid, ni, nj, bp) ;

  }else if(datatype == int_data && bp != NULL){
    return move_int32_block(src, lnis, dst, lnid, ni, nj, bp) ;

  }else if(datatype == uint_data && bp != NULL){
    return move_uint32_block(src, lnis, dst, lnid, ni, nj, bp) ;

  }else if(datatype == raw_data || bp == NULL){     // no data analysis will be performed
    int nij = move_mem32_block(src, lnis, dst, lnid, ni, nj, NULL) ;
    if(bp != NULL){
      bp->maxu.u = 0 ;
      bp->maxs.u = 0 ;
      bp->minu.u = 0 ;
      bp->mins.u = 0 ;
      bp->kind   = (nij > 0) ? raw_data : bad_data ;
      bp->zeros  = -1 ;
    }
    return nij ;

  }else{       // bad data type
    if(bp != NULL){
      bp->kind   = bad_data ;
      bp->zeros  = -1 ;
    }
    return -1 ;
  }
}
