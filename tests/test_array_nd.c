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
#include <strings.h>

#include <rmn/array_nd.h>

void print_dims(void *a_, char *msg){
  array_nd *a = (array_nd *) a_ ;
  int i ;
  fprintf(stderr, valid_array(a) ? "[" : "<") ;
  for(i=0 ; i<a->rank ; i++){
    fprintf(stderr, "%3d(%d:%d)", a->dim[i].gnn, a->dim[i].ln0, a->dim[i].ln0+a->dim[i].lnn-1) ;
  }
  fprintf(stderr, "%s%s", valid_array(a) ? "]" : ">", msg) ;
}

void print_meta(void *a_, char *msg){
  array_nd *a = (array_nd *) a_ ;
  fprintf(stderr, "ndim = %d, rank = %d, flags = %d, type = %d, esize = %lu, s = %8.8x %s",
          a->ndim, a->rank, a->flags, a->type, (uint64_t)a->esize, a->signature, msg) ;
}

void print_flags(char *msg, void *a){
  array_nd *ap = (array_nd *) a ;
  uint8_t flags = ap->flags ;
  fprintf(stderr, "%s %2.2x %s%s%s%s%s%s\n",msg, flags,
                  (flags & DATA_IS_INTERNAL) ?  " MONOLITHIC" : "SPLIT_STRUCT" ,
                  (flags & DATA_MAY_REALLOC) ?  " MAY_REALLOC_DATA    " : " MAY_NOT_REALLOC_DATA" ,
                  (flags & STRUCT_CAN_FREE)  ?  " STRUCT_CAN_BE_FREED" : "",
                  array_is_signed(ap)        ?  " SIGNED" : "",
                  array_no_data(ap)          ?  " EMPTY" : "" ,
                  array_has_data(ap)         ?  " VALID_DATA" : ""
         ) ;
}

void array_lbounds_check(int low, int high){
  array_0d a0 = array_0d_null, *ap0 = &a0 ;
  array_1d a1 = array_1d_null, *ap1 = &a1 ;
  array_2d a2 = array_2d_null, *ap2 = &a2 ;
  array_3d a3 = array_3d_null, *ap3 = &a3 ;
  array_4d a4 = array_4d_null, *ap4 = &a4 ;
  array_5d a5 = array_5d_null, *ap5 = &a5 ;
  array_nd an = array_nd_null, *apn = &an ;
  char *errmsg = "" ;
  int32_t scrap[1024*1024], status, rank ;

  errmsg = "a0 syntax rank"  ; if(ARRAY_SYNTAX_RANK(a0) !=  0) goto fail ;
  errmsg = "a1 syntax rank"  ; if(ARRAY_SYNTAX_RANK(a1) !=  1) goto fail ;
  errmsg = "a2 syntax rank"  ; if(ARRAY_SYNTAX_RANK(a2) !=  2) goto fail ;
  errmsg = "a3 syntax rank"  ; if(ARRAY_SYNTAX_RANK(a3) !=  3) goto fail ;
  errmsg = "a4 syntax rank"  ; if(ARRAY_SYNTAX_RANK(a4) !=  4) goto fail ;
  errmsg = "a5 syntax rank"  ; if(ARRAY_SYNTAX_RANK(a5) !=  5) goto fail ;
  errmsg = "an syntax rank"  ; if(ARRAY_SYNTAX_RANK(an) != -1) goto fail ;

  errmsg = "ap0 syntax rank"  ; if(ARRAY_SYNTAX_RANK(ap0) !=  0) goto fail ;
  errmsg = "ap1 syntax rank"  ; if(ARRAY_SYNTAX_RANK(ap1) !=  1) goto fail ;
  errmsg = "ap2 syntax rank"  ; if(ARRAY_SYNTAX_RANK(ap2) !=  2) goto fail ;
  errmsg = "ap3 syntax rank"  ; if(ARRAY_SYNTAX_RANK(ap3) !=  3) goto fail ;
  errmsg = "ap4 syntax rank"  ; if(ARRAY_SYNTAX_RANK(ap4) !=  4) goto fail ;
  errmsg = "ap5 syntax rank"  ; if(ARRAY_SYNTAX_RANK(ap5) !=  5) goto fail ;
  errmsg = "apn syntax rank"  ; if(ARRAY_SYNTAX_RANK(apn) != -1) goto fail ;

  rank = ARRAY_ALLOC_RANK(a0)  ; errmsg = "a0 rank"  ; if(rank != 0) goto fail ;
  rank = ARRAY_ALLOC_RANK(ap0) ; errmsg = "ap0 rank" ; if(rank != 0) goto fail ;
  rank = ARRAY_ALLOC_RANK(a1)  ; errmsg = "a1 rank"  ; if(rank != 1) goto fail ;
  rank = ARRAY_ALLOC_RANK(ap1) ; errmsg = "ap1 rank" ; if(rank != 1) goto fail ;
  rank = ARRAY_ALLOC_RANK(a2)  ; errmsg = "a2 rank"  ; if(rank != 2) goto fail ;
  rank = ARRAY_ALLOC_RANK(ap2) ; errmsg = "ap2 rank" ; if(rank != 2) goto fail ;
  rank = ARRAY_ALLOC_RANK(a3)  ; errmsg = "a3 rank"  ; if(rank != 3) goto fail ;
  rank = ARRAY_ALLOC_RANK(ap3) ; errmsg = "ap3 rank" ; if(rank != 3) goto fail ;
  rank = ARRAY_ALLOC_RANK(a4)  ; errmsg = "a4 rank"  ; if(rank != 4) goto fail ;
  rank = ARRAY_ALLOC_RANK(ap4) ; errmsg = "ap4 rank" ; if(rank != 4) goto fail ;
  rank = ARRAY_ALLOC_RANK(a5)  ; errmsg = "a5 rank"  ; if(rank != 5) goto fail ;
  rank = ARRAY_ALLOC_RANK(ap5) ; errmsg = "ap5 rank" ; if(rank != 5) goto fail ;
  rank = ARRAY_ALLOC_RANK(an)  ; errmsg = "an rank"  ; if(rank != 0) goto fail ;
  rank = ARRAY_ALLOC_RANK(apn) ; errmsg = "apn rank" ; if(rank != 0) goto fail ;
  apn = (array_nd *)ap1 ; rank = ARRAY_ALLOC_RANK(apn) ; errmsg = "apn_1 rank" ; if(rank != 1) goto fail ;
  apn = (array_nd *)ap3 ; rank = ARRAY_ALLOC_RANK(apn) ; errmsg = "apn_3 rank" ; if(rank != 3) goto fail ;
  apn = (array_nd *)ap5 ; rank = ARRAY_ALLOC_RANK(apn) ; errmsg = "apn_5 rank" ; if(rank != 5) goto fail ;

  fprintf(stderr,"sizeof(array_0d) = %ld, ", sizeof(array_0d)) ;
  fprintf(stderr,"sizeof(array_1d) = %ld, ", sizeof(array_1d)) ;
  fprintf(stderr,"sizeof(array_2d) = %ld, ", sizeof(array_2d)) ;
  fprintf(stderr,"sizeof(array_3d) = %ld, ", sizeof(array_3d)) ;
  fprintf(stderr,"sizeof(array_4d) = %ld, ", sizeof(array_4d)) ;
  fprintf(stderr,"sizeof(array_5d) = %ld, ", sizeof(array_5d)) ;
  fprintf(stderr,"sizeof(array_nd) = %ld\n", sizeof(array_nd)) ;
  for(uint32_t i=0 ; i<sizeof(printable_type)/sizeof(printable_type[0]) ; i++){
    fprintf(stderr,"data_code : %2d '%-7s', bit size = %3d\n", i, printable_type[i], size_of_type[i]) ;
  }
  print_meta((void *) ap0, " : ap0 meta\n") ;
  print_meta((void *) ap1, " : ap1 meta\n") ;
  print_meta((void *) ap2, " : ap2 meta\n") ;
  print_meta((void *) ap3, " : ap3 meta\n") ;
  print_meta((void *) ap4, " : ap4 meta\n") ;
  print_meta((void *) ap5, " : ap5 meta\n") ;

  // make new arrays using caller supplied storage, set bounds
  new_array(&a1, NULL, sizeof(int32_t), int_data, 8) ;
  print_flags("a1 flags : ", &a1) ;
  set_array_lbounds(&a1 , low, high) ;
  new_array(&a2, scrap, sizeof(int32_t), int_data, 8, 7) ;
  set_array_lbounds(&a2 , low, high, low, high) ;
  array_set_used(&a2) ;
  print_flags("a2 flags : ", &a2) ;
  errmsg = "should not be able to free" ;
  if(free_array(&a2) != 0) goto fail ;
  new_array(&a3, NULL, sizeof(int32_t), int_data, 8, 7, 6) ;
  set_array_lbounds(&a3 , low, high, low, high, low, high) ;
  print_flags("a3 flags : ", &a3) ;
  new_array(&a4, scrap, sizeof(int32_t), int_data, 8, 7, 6, 5) ;
  set_array_lbounds(&a4 , low, high, low, high, low, high, low, high) ;
  array_set_used(&a4) ;
  print_flags("a4 flags : ", &a4) ;
  new_array(&a5, NULL, sizeof(int32_t), int_data, 8, 7, 6, 5, 4) ;
  set_array_lbounds(&a5 , low, high, low, high, low, high, low, high, low, high) ;
  print_flags("a5 flags : ", &a5) ;

  array_signature(&a5) = NO_DATA ;
  fprintf(stderr, "before reshape , signature = %8.8x(%s), ", array_signature(&a5), array_no_data(&a5) ? "is empty" : "has data" ) ;
  print_dims(&a5, "\n") ;

  errmsg = "reshaped array is not valid" ;
  reshape_array(&a5, sizeof(int32_t), int_data, 8, 7, 6, 5, 4) ;
  array_set_used(&a5) ;
//   new_array(&a5, array_address(&a5), sizeof(int32_t), int_data, 8, 7, 6, 5, 4) ;
  fprintf(stderr, "after reshape 1, signature = %8.8x(%s), ", array_signature(&a5), array_no_data(&a5) ? "is empty" : "has data" ) ;
  print_dims(&a5, "\n") ;
  if(invalid_array(&a5)) goto fail ;

  errmsg = "reshaped array is valid and should not be" ;
  reshape_array(&a5, sizeof(int32_t), int_data, 9, 7, 6, 5, 4) ;    // reshape is deliberately oversized
  status = valid_array(&a5) ; 
//   new_array(&a5, array_address(&a5), sizeof(int32_t), int_data, 9, 7, 6, 5, 4) ;
  fprintf(stderr, "after reshape 2, signature = %8.8x(%s), ", array_signature(&a5), status ? "valid   " : "invalid " ) ;
  print_dims(&a5, "\n") ;
print_meta(&a5, " |metadata|\n") ;
  if(status) goto fail ;

  errmsg = "reshaped array is not valid" ;
  reshape_array(&a5, sizeof(int32_t), int_data, 7, 6, 5, 4, 3) ;
  array_set_empty(&a5) ;
//   new_array(&a5, array_address(&a5), sizeof(int32_t), int_data, 7, 6, 5, 4, 3) ;
  fprintf(stderr, "after reshape 3, signature = %8.8x(%s), ", array_signature(&a5), array_no_data(&a5) ? "is empty" : "has data" ) ;
  print_dims(&a5, "\n") ;
  if(invalid_array(&a5)) goto fail ;

  reshape_array(&a5, sizeof(int32_t), int_data, 8, 7, 6, 5, 4) ;
  array_signature(&a5) = HAS_DATA ;
//   new_array(&a5, array_address(&a5), sizeof(int32_t), int_data, 8, 7, 6, 5, 4) ;
  fprintf(stderr, "after reshape 4, signature = %8.8x(%s), ", array_signature(&a5), array_no_data(&a5) ? "is empty" : "has data" ) ;
  print_dims(&a5, "\n") ;
  if(invalid_array(&a5)) goto fail ;

  errmsg = "w32 does not point to data" ;
  create_array(ap1, DATA_IS_INTERNAL, sizeof(int32_t), int_data, 8) ;                 // specific 1D interface
  if( (uint8_t *)(ap1->w32) != array_address(ap1) ) goto fail ;
  errmsg = "array is invalid" ;
  if( ! valid_array(ap1) ) goto fail ;
  print_flags("ap1 flags : ", ap1) ;
  fprintf(stderr, "%12s array size is %6ld, subarray elements = %6d ", array_kind(ap1), array_bytes(ap1), subarray_dimension(ap1)) ; print_dims(ap1, "") ;
  fprintf(stderr, ", free_array(ap1) = %d\n", free_array(ap1)) ;

  create_array(ap2, DATA_IS_INTERNAL, sizeof(int32_t), int_data, 8, 7) ;              // specific 2D interface
  errmsg = "w32 does not point to data" ;
  if( (uint8_t *)(ap2->w32) != ap2->data ) goto fail ;
  errmsg = "array is invalid" ;
  if( invalid_array(ap2) ) goto fail ;
  print_flags("ap2 flags : ", ap2) ;
  fprintf(stderr, "%12s array size is %6ld, subarray elements = %6d ", array_kind(ap2), array_bytes(ap2), subarray_dimension(ap2)) ; print_dims(ap2, "") ;
  fprintf(stderr, ", free_array(ap2) = %d\n", free_array(ap2)) ;

  create_array(ap3, DATA_IS_INTERNAL, sizeof(int32_t), int_data, 8, 7, 6) ;           // specific 3D interface
  errmsg = "w32 does not point to data" ;
  if( (uint8_t *)(ap3->w32) != ap3->data ) goto fail ;
  errmsg = "array is invalid" ;
  if( ! valid_array(ap3) ) goto fail ;
  print_flags("ap3 flags : ", ap3) ;
  fprintf(stderr, "%12s array size is %6ld, subarray elements = %6d ", array_kind(ap3), array_bytes(ap3), subarray_dimension(ap3)) ; print_dims(ap3, "") ;
  fprintf(stderr, ", free_array(ap3) = %d\n", free_array(ap3)) ;

  create_array(ap4, DATA_IS_INTERNAL, sizeof(int32_t), int_data, 8, 7, 6, 5) ;        // specific 4D interface
  errmsg = "w32 does not point to data" ;
  if( (uint8_t *)(ap4->w32) != ap4->data ) goto fail ;
  errmsg = "array is invalid" ;
  if( invalid_array(ap4) ) goto fail ;
  print_flags("ap4 flags : ", ap4) ;
  fprintf(stderr, "%12s array size is %6ld, subarray elements = %6d ", array_kind(ap4), array_bytes(ap4), subarray_dimension(ap4)) ; print_dims(ap4, "") ;
  fprintf(stderr, ", free_array(ap4) = %d\n", free_array(ap4)) ;

  create_array(ap5, DATA_IS_INTERNAL, sizeof(int32_t), int_data, 8, 7, 6, 5, 4) ;     // specific 5D interface
  errmsg = "w32 does not point to data" ;
  if( (uint8_t *)(ap5->w32) != ap5->data ) goto fail ;
  errmsg = "array is invalid" ;
  if( ! valid_array(ap5) ) goto fail ;
  print_flags("ap5 flags : ", ap5) ;
  fprintf(stderr, "%12s array size is %6ld, subarray elements = %6d ", array_kind(ap5), array_bytes(ap5), subarray_dimension(ap5)) ; print_dims(ap5, "") ;
  fprintf(stderr, ", free_array(ap5) = %d\n", free_array(ap5)) ;

  create_array(apn, 0, sizeof(int32_t), int_data, 8, 7, 6, 5, 4) ;     // generic nD interface
  errmsg = "array is invalid" ;
  if( ! valid_array(apn) ) goto fail ;
  errmsg = "DATA_IS_INTERNAL is true" ;
  if( apn->flags & DATA_IS_INTERNAL ) goto fail ;
  errmsg = "DATA_MAY_REALLOC not present" ;
  if( (apn->flags & DATA_MAY_REALLOC) == 0) goto fail ;
  set_array_lbounds(apn , low, high, low, high, low, high, low, high, low, high) ;
  print_flags("apn flags : ", apn) ;
  fprintf(stderr, "%12s array size is %6ld, subarray elements = %6d ", array_kind(apn), array_bytes(apn), subarray_dimension(apn)) ; print_dims(apn, "") ;
  fprintf(stderr, ", free_array(apn) = %d\n", free_array(apn)) ;

  return ;

fail:
  fprintf(stderr, "ERROR : %s, TEST FAILED\n", errmsg) ;
  exit(1) ;

}

#define GNI 127
#define GNJ 129
#define GNK 31
#define SUB 10

static /*inline*/ int32_t fijk(int i, int j, int k){
  return k | (j << 8) | (i << 20) ;
}

// set array element to a given value
void set_subarray(int gni, int gnj, int gnk, int32_t f[gnk][gnj][gni], int i, int j, int k, int32_t value){
  f[k][j][i] = value ;
}

// get value from array element
int32_t get_subarray(int gni, int gnj, int gnk, int32_t f[gnk][gnj][gni], int i, int j, int k){
  return f[k][j][i] ;
}

// SLOW with gcc
int subarray_check(int gni, int gnj, int gnk, int32_t f[gnk][gnj][gni], int i0, int in, int j0, int jn, int k0, int kn){
  int i, j, k, errors = 0 ;
  for(k=0 ; k<kn ; k++){
    for(j=0 ; j<jn ; j++){
      for(i=0 ; i<in ; i++){
//         if(f[k][j][i] != ( ((j+j0) << 8) | (k+k0) | ((i+i0) << 20) ) ) errors++ ;
        if(f[k][j][i] != fijk(i+i0, j+j0, k+k0) ) errors++ ;
      }
    }
  }
  return errors ;
}

void  print_strides(char *msg, __i32__5__ strides){
  int i ;
  fprintf(stderr, "%s strides : ", msg) ;
  for(i=0 ; i<5 ; i++){
    fprintf(stderr, "%d ", strides.i32[i]) ;
  }
  fprintf(stderr, "\n") ;
}

void print_bounds(array_nd *a1, char *msg){
  int i ;
  fprintf(stderr, "%s bounds : ", msg) ;
  for(i=0 ; i<a1->rank ; i++){
    fprintf(stderr, "[g = %d:%d, l = %d:%d, lnn = %d] ",
                    a1->dim[i].gn0, a1->dim[i].gnn-1, a1->dim[i].ln0, a1->dim[i].ln0+a1->dim[i].lnn-1, a1->dim[i].lnn) ;
  }
  fprintf(stderr, "\n") ;
}

int32_t compare_array(int gni, int gnj, int gnk, int gnl, int gnm, int32_t a1[gnm][gnl][gnk][gnj][gni], int32_t a2[gnm][gnl][gnk][gnj][gni]){
  int diff = 0 ;
  int i, j, k, l, m ;
  for(m = 0 ; m < gnm ; m++){
    for(l = 0 ; l < gnl ; l++){
      for(k = 0 ; k < gnk ; k++){
        for(j = 0 ; j < gnj ; j++){
          for(i = 0 ; i < gni ; i++){
            if( a1[m][l][k][j][i] != a2[m][l][k][j][i] ) diff++ ;
          }
        }
      }
    }
  }
  return diff ;
}

void fill_array(int gni, int gnj, int gnk, int gnl, int gnm, int32_t dst[gnm][gnl][gnk][gnj][gni]){
  int i, j, k, l, m ;
  for(m = 0 ; m < gnm ; m++){
    for(l = 0 ; l < gnl ; l++){
      for(k = 0 ; k < gnk ; k++){
        for(j = 0 ; j < gnj ; j++){
          for(i = 0 ; i < gni ; i++){
            dst[m][l][k][j][i] = ((i+1) << 16) | ((j+1) << 12) | ((k+1) << 8) | ((l+1) << 4) | (m+1) ;
          }
        }
      }
    }
  }
}

void block_copy_check(int gni, int gnj, int gnk, int gnl, int gnm){
  (void)(gnj) ;
  (void)(gnk) ;
  (void)(gnl) ;
  (void)(gnm) ;
  array_1d a1 = array_1d_null, b1 = array_1d_null ;
  array_2d a2 = array_2d_null, b2 = array_2d_null ;
  array_3d a3 = array_3d_null, b3 = array_3d_null;
  array_4d a4 = array_4d_null, b4 = array_4d_null;
  array_5d a5 = array_5d_null, b5 = array_5d_null;
  int32_t l1[gni], r1[gni], t1[gni] ;                                                                // array1, array2, temp
  int32_t l2[gnj][gni], r2[gnj][gni], t2[gnj][gni] ;                                                 // array1, array2, temp
  int32_t l3[gnk][gnj][gni], r3[gnk][gnj][gni], t3[gnk][gnj][gni] ;                                  // array1, array2, temp
  int32_t l4[gnl][gnk][gnj][gni], r4[gnl][gnk][gnj][gni], t4[gnl][gnk][gnj][gni] ;                   // array1, array2, temp
  int32_t l5[gnm][gnl][gnk][gnj][gni], r5[gnm][gnl][gnk][gnj][gni], t5[gnm][gnl][gnk][gnj][gni] ;    // array1, array2, temp
  int i, j, k, l, diff ;
  ssize_t sz1, sz2 ;
  __i32__5__ strides ;
  block_properties bp ;

  fprintf(stderr, "\n ======================== 1D test =================================\n") ;

  new_array(&a1, l1, sizeof(int32_t), int_data, gni) ; bzero(l1, sizeof(l1)) ;   // set a1 to 0
  fprintf(stderr, "a1 :") ; for(i=0 ; i<gni ; i++){ fprintf(stderr, " %8.8x", l1[i]) ;  } ; fprintf(stderr, "\n") ;
  new_array(&b1, r1, sizeof(int32_t), int_data, gni) ; bzero(r1, sizeof(r1)) ;

  fill_array(gni, 1, 1, 1, 1, (void *)t1) ;
  print_bounds((array_nd *)&a1, "a1") ;
  sz1 = subarray_set(&a1, (void *)t1, sizeof(t1)) ;                              // set entire a1
  fprintf(stderr, "sz1 = %ld\n", sz1) ;
  fprintf(stderr, "a1 :") ; for(i=0 ; i<gni ; i++){ fprintf(stderr, " %8.8x", l1[i]) ;  } ; fprintf(stderr, "\n") ;
  if(sz1 <= 0) goto fail ;
  diff = compare_array(gni, 1, 1, 1, 1, (void *)&l1[0], (void *)&t1[0]) ;
  fprintf(stderr, "diff = %d\n", diff) ;
  if(diff != 0) goto fail ;

  set_array_lbounds(&a1 , 2, gni-1) ;
  print_bounds((array_nd *)&a1, "a1") ;
  bzero(t1, sizeof(t1)) ;
  sz1 = subarray_get(&a1, (void *)t1, sizeof(t1), &bp) ;                              // get part of a1
  fprintf(stderr, "sz1 = %ld\n", sz1) ;
  if(sz1 <= 0) goto fail ;

  fprintf(stderr, "t1 :") ; for(i=0 ; i<(int)sz1 ; i++){ fprintf(stderr, " %8.8x", t1[i]) ;  } ; fprintf(stderr, "\n") ;
  for(i=0 ; i<(int)sz1 ; i++){ if(t1[i] != l1[i+2]) goto fail ; }
  for(i=(int)sz1 ; i<gni ; i++) { if(t1[i] != 0) goto fail ; }

  set_array_lbounds(&b1 , 1, gni-1) ;                                            // set bounds of b1 too large
  print_bounds((array_nd *)&b1, "b1") ;
  sz2 = subarray_set(&b1, (void *)t1, sz1 * sizeof(int32_t)) ;                   // sz1 is too small, not enough data
  fprintf(stderr, "sz2 = %ld\n", sz2) ;
  if(sz2 > 0) goto fail ;
  fprintf(stderr, "b1 :") ; for(i=0 ; i<gni ; i++){ fprintf(stderr, " %8.8x", r1[i]) ;  } ; fprintf(stderr, "\n") ;
  for(i=0 ; i<gni ; i++){ if(r1[i] != 0) goto fail ; }

  set_array_lbounds(&b1 , 1, gni-2) ;
  print_bounds((array_nd *)&b1, "b1") ;
  sz2 = subarray_set(&b1, (void *)t1, sz1 * sizeof(int32_t)) ;                   // subarray dimanesion now matches data
  fprintf(stderr, "sz2 = %ld\n", sz2) ;
  if(sz2 <= 0) goto fail ;
  fprintf(stderr, "b1 :") ; for(i=0 ; i<gni ; i++){ fprintf(stderr, " %8.8x", r1[i]) ;  } ; fprintf(stderr, "\n") ;
  for(i=0     ; i<1     ; i++){ if(r1[i] !=       0) goto fail ; }
  for(i=1     ; i<gni-1 ; i++){ if(r1[i] != l1[i+1]) goto fail ; } 
  for(i=gni-1 ; i<gni   ; i++){ if(r1[i] !=       0) goto fail ; }

  array_strides(&a1, &strides);
  print_strides("a1", strides);

  fprintf(stderr, "\n ======================== 2D test =================================\n") ;

  new_array(&a2, l2, sizeof(int32_t), int_data, gni, gnj) ; bzero(l2, sizeof(l2)) ;
  fill_array(gni, gnj, 1, 1, 1, (void *)l2) ;
  fprintf(stderr, "a2[0][] :") ;
  for(i=0 ; i<gni ; i++){ fprintf(stderr, " %8.8x", l2[0][i] >> 12) ;  } ; fprintf(stderr, "\n") ;
  print_bounds((array_nd *)&a2, "a2") ;
  set_array_lbounds(&a2 , 0, gni-2, 1, gnj-1) ;                           // suppress right column and bottom row
  print_bounds((array_nd *)&a2, "a2") ;
  sz1 = subarray_get(&a2, (void *)t2, sizeof(t2), &bp) ;
  fprintf(stderr, "sz1 = %ld, expecting %d\n", sz1, (gni-1)*(gnj-1));

  new_array(&b2, r2, sizeof(int32_t), int_data, gni, gnj) ; bzero(r2, sizeof(r2)) ;
  set_array_lbounds(&b2 , 1, gni-1, 0, gnj-2) ;                           // suppress left column and top row
  sz2 = subarray_set(&b2, (void *)t2, sz1 * sizeof(int32_t)) ;
  fprintf(stderr, "sz2 = %ld\n", sz2) ;
  diff = 0 ;
  for(j=0 ; j<gnj-1 ; j++){
    for(i=0 ; i<gni-1 ; i++){
      if(l2[j+1][i] != r2[j][i+1]) diff ++ ;
    }
  }
  fprintf(stderr, "diff = %d\n", diff) ;
  if(diff != 0) goto fail ;

  fprintf(stderr, "\n ======================== 3D test =================================\n") ;

  new_array(&a3, l3, sizeof(int32_t), int_data, gni, gnj, gnk) ; bzero(l3, sizeof(l3)) ;
  fill_array(gni, gnj, gnk, 1, 1, (void *)l3) ;
  fprintf(stderr, "a3[gnk-1][0][] :") ;
  for(i=0 ; i<gni ; i++){ fprintf(stderr, " %8.8x", l3[gnk-1][0][i] >> 8) ;  } ; fprintf(stderr, "\n") ;
  print_bounds((array_nd *)&a3, "a3") ;
  set_array_lbounds(&a3 , 0, gni-1, 0, gnj-1, 1, gnk-2) ;
  print_bounds((array_nd *)&a3, "a3") ;
  sz1 = subarray_get(&a3, (void *)t3, sizeof(t3), &bp) ;
  fprintf(stderr, "sz1 = %ld, expecting %d\n", sz1, gni*gnj*(gnk-1));

  new_array(&b3, r3, sizeof(int32_t), int_data, gni, gnj, gnk) ; bzero(r3, sizeof(r3)) ;
  set_array_lbounds(&b3 , 0, gni-1, 0, gnj-1, 1, gnk-2) ;                 // suppress top and bottom plane
  sz2 = subarray_set(&b3, (void *)t3, sz1 * sizeof(int32_t)) ;
  fprintf(stderr, "sz2 = %ld\n", sz2) ;

  diff = compare_array(gni, gnj, gnk-2, 1, 1, (void *)&l3[1][0][0], (void *)&r3[1][0][0]) ;
  fprintf(stderr, "diff = %d\n", diff) ;
  if(diff != 0) goto fail ;

  set_array_lbounds(&a3 , 1, gni-3, 1, gnj-3, 3, gnk-1) ;                  // more complex subarray
  print_bounds((array_nd *)&a3, "a3") ;
  sz1 = subarray_get(&a3, (void *)t3, sizeof(t3), &bp) ;
  bzero(r3, sizeof(r3)) ;
  set_array_lbounds(&b3 , 0, gni-4, 2, gnj-2, 2, gnk-2) ;                  // same dimension, different position subarray
  print_bounds((array_nd *)&b3, "b3") ;
  sz2 = subarray_set(&b3, (void *)t3, sz1 * sizeof(int32_t)) ;
  fprintf(stderr, "sz1 = %ld, expecting %d", sz1, (gni-3)*(gnj-3)*(gnk-3));
  fprintf(stderr, ", sz2 = %ld\n", sz2) ;
  diff = 0 ;
  for(k=0 ; k<gnk-3 ; k++){
    for(j=0 ; j<gnj-3 ; j++){
      for(i=0 ; i<gni-3 ; i++){
        if(l3[k+3][j+1][i+1] != r3[k+2][j+2][i]) diff++ ;
      }
    }
  }
  fprintf(stderr, "diff = %d\n", diff) ;
  if(diff != 0) goto fail ;

  array_strides(&a3, &strides);
  print_strides("a3", strides);

  fprintf(stderr, "\n ======================== 4D test =================================\n") ;

  new_array(&a4, l4, sizeof(int32_t), int_data, gni, gnj, gnk, gnl) ; bzero(l4, sizeof(l4)) ;
  fill_array(gni, gnj, gnk, gnl, 1, (void *)l4) ;
  fprintf(stderr, "a4[0][0][0][] :") ;
  for(i=0 ; i<gni ; i++){ fprintf(stderr, " %8.8x", l4[0][0][0][i] >> 4) ;  } ; fprintf(stderr, "\n") ;
  print_bounds((array_nd *)&a4, "a4") ;

  sz1 = subarray_get(&a4, (void *)t4, sizeof(t4), &bp) ;
  new_array(&b4, r4, sizeof(int32_t), int_data, gni, gnj, gnk, gnl) ; bzero(r4, sizeof(r4)) ;
  sz2 = subarray_set(&b4, (void *)t4, sz1 * sizeof(int32_t)) ;
  fprintf(stderr, "sz1 = %ld, expecting %d", sz1, gni*gnj*gnk*gnl);
  fprintf(stderr, ", sz2 = %ld\n", sz2) ;
  if(sz1 != sz2) goto fail ;
  diff = compare_array(gni, gnj, gnk, gnl, 1, (void *)&l4[0][0][0][0], (void *)&r4[0][0][0][0]) ;
  fprintf(stderr, "diff = %d\n", diff) ;
  if(diff != 0) goto fail ;

  set_array_lbounds(&a4 , 0, gni-1, 0, gnj-1, 0, gnk-2, 1, gnl-1) ;               // suppress bottom cube, top planes
  print_bounds((array_nd *)&a4, "a4") ;
  sz1 = subarray_get(&a4, (void *)t4, sizeof(t4), &bp) ;

  set_array_lbounds(&b4 , 0, gni-1, 0, gnj-1, 1, gnk-1, 0, gnl-2)  ;              // suppress top cube, bottom planes
  print_bounds((array_nd *)&b4, "b4") ;
  sz2 = subarray_set(&b4, (void *)t4, sz1 * sizeof(int32_t)) ;
  fprintf(stderr, "sz1 = %ld, expecting %d", sz1, gni*gnj*gnk*(gnl-1));
  fprintf(stderr, ", sz2 = %ld\n", sz2) ;
  diff = 0 ;
  for(l=0 ; l<gnl-1 ; l++){
    for(k=0 ; k<gnk-1 ; k++){
      for(j=0 ; j<gnj-1 ; j++){
        for(i=0 ; i<gni-1 ; i++){
          if(l4[l+1][k][j][i] != r4[l][k+1][j][i]) diff++ ;
        }
      }
    }
  }
  fprintf(stderr, "diff = %d\n", diff) ;
  if(diff != 0) goto fail ;

  fprintf(stderr, "\n ======================== 5D test =================================\n") ;

  new_array(&a5, l5, sizeof(int32_t), int_data, gni, gnj, gnk, gnl, gnm) ; bzero(l5, sizeof(l5)) ;
  fill_array(gni, gnj, gnk, gnl, gnm, (void *)l5) ;
  fprintf(stderr, "a5[0][0][0][0][] :") ;
  for(i=0 ; i<gni ; i++){ fprintf(stderr, " %8.8x", l5[0][0][0][0][i]) ;  } ; fprintf(stderr, "\n") ;

  sz1 = subarray_get(&a5, (void *)t5, sizeof(t5), &bp) ;
  fprintf(stderr, "sz1 = %ld(%ld), sizeof(l5) = %ld, sizeof(t5) = %ld\n", sz1, sz1*sizeof(int32_t), sizeof(l5),sizeof(t5) ) ;

  new_array(&b5, r5, sizeof(int32_t), int_data, gni, gnj, gnk, gnl, gnm) ; bzero(r5, sizeof(l5)) ;
  sz2 = subarray_set(&b5, (void *)t5, sz1 * sizeof(int32_t)) ;
  fprintf(stderr, "sz2 = %ld\n", sz2) ;
  diff = compare_array(gni, gnj, gnk, gnl, gnm, (void *)l5, (void *)r5) ;
  fprintf(stderr, "diff = %d\n", diff) ;
  if(diff != 0) goto fail ;

  array_strides(&a5, &strides);
  print_strides("a5", strides);

  return ;

fail:
  fprintf(stderr, "FAILED\n") ;
  exit(1) ;
}

int main(int argc, char **argv){
  int32_t ref[GNK][GNJ][GNI], cpy[GNK][GNJ][GNI] ;
  int i, j, k, l, errors, errsub ;
  block_properties bp ;

  if(argc > 1 && argv[0] == NULL) return 1 ;  // useless code to get rid of compiler warning

  fprintf(stderr, "=============== errors test ===============\n") ;
  array_1d a1 = array_1d_null, *ap1 = &a1 ;
  array_2d a2 = array_2d_null, *ap2 = &a2 ; ;
  new_array(&a2, ref, sizeof(int32_t), 1, GNI, GNJ, GNK) ;
  create_array(ap1, DATA_IS_INTERNAL, 4, int_data, GNI, GNJ, GNK) ;
  fix_array_nd(NULL) ;
  create_array(ap1, DATA_IS_INTERNAL, 4, int_data, 10 ) ;
  ap1->esize = 8 ;
  fix_array(ap1) ;
  ap1->dim[0].gnn = 0 ;
  fix_array(ap1) ;
  set_array_lbounds(ap1, 1, 100) ;
  ap1->dim[0].gnn = 10 ;
  set_array_lbounds(ap1, 1, 100) ;
  ap1->data = NULL ;
  set_array_lbounds(ap1, 1, 100) ;
  new_array(ap2, ref, sizeof(int32_t), 1, GNI, GNJ) ;
  set_array_lbounds(ap2, 1, 2, 3) ;
  set_array_lbounds(ap2, 0, GNI, 0, GNJ) ;
// return 0 ;
  fprintf(stderr, "=============== block copy test ===============\n") ;
  block_copy_check(9, 8, 7, 6, 5) ;
// goto end ;
  fprintf(stderr, "=============== array_lbounds test ===============\n") ;
  array_lbounds_check(1, 3);      // call bounds test, lower bound : 1, upper bound : 3
  fprintf(stderr, "SUCCESS\n") ;
// goto end ;
  fprintf(stderr, "=============== sub array test ===============\n") ;
//   array_1d a1 = array_1d_null ;
//   array_2d a2 = array_2d_null ;
  array_3d a3 = array_3d_null ;
  array_3d b3 = array_3d_null ;
//   new_array(&a1, ref, sizeof(int32_t), 1, GNI) ;
//   new_array(&a2, ref, sizeof(int32_t), 1, GNI, GNJ) ;
  new_array(&a3, ref, sizeof(int32_t), 1, GNI, GNJ, GNK) ;
  new_array(&b3, cpy, sizeof(int32_t), 1, GNI, GNJ, GNK) ;
  for(k=0 ; k<GNK ; k++){
    for(j=0 ; j<GNJ ; j++){
      for(i=0 ; i<GNI ; i++){
        ref[k][j][i] = fijk(i, j, k) ;
        cpy[k][j][i] = -1 ;
      }
    }
  }
  errors = 0 ;
  int32_t *ptra, *ptrb ;
  int32_t copy[SUB][SUB][SUB], saved[SUB] ;
  size_t subsize ;
  errsub = 0 ;
  for(k=0 ; k<GNK-SUB ; k++){
    for(j=0 ; j<GNJ-SUB ; j++){
      for(i=0 ; i<GNI-SUB ; i++){
        set_array_lbounds(&a3, i, i+SUB-1, j, j+SUB-1, k, k+SUB-1) ;  // SUB x SUB x SUB sub array at [k][j][i] in ba3
        set_array_lbounds(&b3, i, i+SUB-1, j, j+SUB-1, k, k+SUB-1) ;  // SUB x SUB x SUB sub array at [k][j][i] in b3
        ptra = (int32_t *) subarray_address((array_nd *)&a3) ;
        ptrb = (int32_t *) subarray_address((array_nd *)&b3) ;
        if(*ptra != fijk(i, j, k)){
          fprintf(stderr, "[%3d,%3d,%3d], expected %8.8x, got %8.8x\n", i, j, k, fijk(i, j, k), *ptra) ;
          errors++ ;
          goto fail ;
        }
        // get block from a3
        subsize = subarray_get(&a3, copy, sizeof(copy), &bp) ;
        if(subsize != 1000){
          fprintf(stderr, "subsize(get) = %ld, expected 1000\n", subsize) ;
          goto fail ;
        }
        // check block
        errsub = 0 ;
        errsub = subarray_check(SUB, SUB, SUB, copy,  i, SUB, j, SUB, k, SUB) ;
        if(0 != errsub){
          fprintf(stderr, "errsub(copy) = %d [%3d,%3d,%3d]\n", errsub, i, j, k) ;
          goto fail ;
        }
        // copy block into b3
        subsize = subarray_set(&b3, copy, sizeof(copy)) ;
        if(subsize != 1000){
          fprintf(stderr, "subsize(set) = %ld, expected 1000\n", subsize) ;
          goto fail ;
        }
        // check a3
        errsub = 0 ;
        errsub = subarray_check(GNI, GNJ, GNK, (void *) ptra, i, SUB, j, SUB, k, SUB) ;
        if(0 != errsub){
          fprintf(stderr, "errsub(ptra) = %d\n", errsub) ;
          goto fail ;
        }
        // check b3
        errsub = 0 ;
        errsub = subarray_check(GNI, GNJ, GNK, (void *) ptrb, i, SUB, j, SUB, k, SUB) ;
        if(0 != errsub){
          fprintf(stderr, "errsub(ptrb) = %d\n", errsub) ;
          goto fail ;
        }

        // set erroneous values in block, check that we are getting the right number of errors
        for(l=0 ; l<SUB ; l++) copy[l][l][l] = -1 ;
           errsub = SUB ;
        errsub = subarray_check(SUB, SUB, SUB, copy,  i, SUB, j, SUB, k, SUB) ;
        if(SUB != errsub){
          fprintf(stderr, "errsub(copy) = %d, expected %d\n", errsub, SUB) ;
          goto fail ;
        }
        // save current value from a3
        for(l=0 ; l<SUB ; l++) saved[l] = get_subarray(GNI, GNJ, GNK, (void *) ptra, l, l, l) ;
        // set erroneous values in a3
        for(l=0 ; l<SUB ; l++) set_subarray(GNI, GNJ, GNK, (void *) ptra, l, l, l, -1) ;
           errsub = SUB ;
        errsub = subarray_check(GNI, GNJ, GNK, (void *) ptra, i, SUB, j, SUB, k, SUB) ;
        if(SUB != errsub){
          fprintf(stderr, "errsub(ptra) = %d, expected %d [%3d,%3d,%3d]\n", errsub, SUB, i, j, k) ;
          goto fail ;
        }
        // restore saved value into a3
        for(l=0 ; l<SUB ; l++) set_subarray(GNI, GNJ, GNK, (void *) ptra, l, l, l, saved[l]) ;
      }
    }
  }

  goto end ;

end:
  fprintf(stderr, "SUCCESS\n") ;
  return 0 ;

fail:
  if(errors > 0) fprintf(stderr, "errors = %d\n", errors) ;
  fprintf(stderr, "FAILED\n") ;
  exit(1) ;
}
