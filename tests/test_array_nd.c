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

#include <rmn/array_nd.h>

void print_dims(void *a_, char *msg){
  array_nd *a = (array_nd *) a_ ;
  int i ;
  fprintf(stderr, "[") ;
  for(i=0 ; i<a->ndim ; i++){
    fprintf(stderr, "%3d(%d:%d)", a->dim[i].gnn, a->dim[i].ln0, a->dim[i].ln0+a->dim[i].lnn-1) ;
  }
  fprintf(stderr, "]%s", msg) ;
}

void array_lbounds_check(int low, int high){
  array_1d a1 = array_1d_null, *ap1 ;
  array_2d a2 = array_2d_null, *ap2 ;
  array_3d a3 = array_3d_null, *ap3 ;
  array_4d a4 = array_4d_null, *ap4 ;
  array_5d a5 = array_5d_null, *ap5 ;
  array_nd *apn ;
  int32_t scrap[1024*1024] ;

  new_array(&a1, scrap, sizeof(int32_t), int_data, 8) ;
  set_array_lbounds(&a1 , low, high) ;
  new_array(&a2, scrap, sizeof(int32_t), int_data, 8, 7) ;
  set_array_lbounds(&a2 , low, high, low, high) ;
  new_array(&a3, scrap, sizeof(int32_t), int_data, 8, 7, 6) ;
  set_array_lbounds(&a3 , low, high, low, high, low, high) ;
  new_array(&a4, scrap, sizeof(int32_t), int_data, 8, 7, 6, 5) ;
  set_array_lbounds(&a4 , low, high, low, high, low, high, low, high) ;
  new_array(&a5, scrap, sizeof(int32_t), int_data, 8, 7, 6, 5, 4) ;
  set_array_lbounds(&a5 , low, high, low, high, low, high, low, high, low, high) ;

  create_array(ap1, sizeof(int32_t), int_data, 8) ;
  if( (uint8_t *)(ap1->w32) != array_address(ap1) ) goto fail ;
  fprintf(stderr, "%12s array size is %6d, dimension = %6d ", array_kind(ap1), array_size(ap1), subarray_dimension(ap1)) ; print_dims(ap1, "\n") ;

  create_array(ap2, sizeof(int32_t), int_data, 8, 7) ;
  if( (uint8_t *)(ap1->w32) != ap1->data ) goto fail ;
  fprintf(stderr, "%12s array size is %6d, dimension = %6d ", array_kind(ap2), array_size(ap2), subarray_dimension(ap2)) ; print_dims(ap2, "\n") ;

  create_array(ap3, sizeof(int32_t), int_data, 8, 7, 6) ;
  if( (uint8_t *)(ap1->w32) != ap1->data ) goto fail ;
  fprintf(stderr, "%12s array size is %6d, dimension = %6d ", array_kind(ap3), array_size(ap3), subarray_dimension(ap3)) ; print_dims(ap3, "\n") ;

  create_array(ap4, sizeof(int32_t), int_data, 8, 7, 6, 5) ;
  if( (uint8_t *)(ap1->w32) != ap1->data ) goto fail ;
  fprintf(stderr, "%12s array size is %6d, dimension = %6d ", array_kind(ap4), array_size(ap4), subarray_dimension(ap4)) ; print_dims(ap4, "\n") ;

  create_array(ap5, sizeof(int32_t), int_data, 8, 7, 6, 5, 4) ;
  if( (uint8_t *)(ap1->w32) != ap1->data ) goto fail ;
  fprintf(stderr, "%12s array size is %6d, dimension = %6d ", array_kind(ap5), array_size(ap5), subarray_dimension(ap5)) ; print_dims(ap5, "\n") ;

  create_array(apn, sizeof(int32_t), int_data, 9, 8, 7, 6, 5, 4) ;
  set_array_lbounds(apn , low, high, low, high, low, high, low, high, low, high, low, high) ;
  fprintf(stderr, "%12s array size is %6d, dimension = %6d ", array_kind(apn), array_size(apn), subarray_dimension(apn)) ; print_dims(apn, "\n") ;

  return ;

fail:
  fprintf(stderr, "TEST FAILED\n") ;
  exit(1) ;

}

#define GNI 127
#define GNJ 129
#define GNK 31
#define SUB 10

int32_t fijk(int i, int j, int k){
  return k | (j << 8) | (i << 20) ;
}

void set_subarray(int gni, int gnj, int gnk, int32_t f[gnk][gnj][gni], int i, int j, int k, int32_t value){
  f[k][j][i] = value ;
}

int32_t get_subarray(int gni, int gnj, int gnk, int32_t f[gnk][gnj][gni], int i, int j, int k){
  return f[k][j][i] ;
}

int subarray_check(int gni, int gnj, int gnk, int32_t f[gnk][gnj][gni], int i0, int in, int j0, int jn, int k0, int kn){
  int i, j, k, errors = 0 ;
  for(k=0 ; k<kn ; k++){
    for(j=0 ; j<jn ; j++){
      for(i=0 ; i<in ; i++){
        if(f[k][j][i] != fijk(i+i0, j+j0, k+k0)){
          errors++ ;
        }
      }
    }
  }
  return errors ;
}

int main(int argc, char **argv){
  int32_t ref[GNK][GNJ][GNI], cpy[GNK][GNJ][GNI] ;
  int i, j, k, l, errors, errsub ;

  if(argc > 1 && argv[0] == NULL) return 1 ;  // useless code to get rid of compiler warning

  fprintf(stderr, "=============== array_lbounds test ===============\n") ;
  array_lbounds_check(1, 3);
  fprintf(stderr, "SUCCESS\n") ;

  fprintf(stderr, "=============== sub array test ===============\n") ;
  array_1d a1 = array_1d_null ;
  array_2d a2 = array_2d_null ;
  array_3d a3 = array_3d_null ;
  array_3d b3 = array_3d_null ;
  new_array(&a1, ref, sizeof(int32_t), 1, GNI) ;
  new_array(&a2, ref, sizeof(int32_t), 1, GNI, GNJ) ;
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
          fprintf(stderr, "FAILED\n") ;
          exit(1) ;
        }
        // get block from a3
        subsize = subarray_get(&a3, copy, sizeof(copy)) ;
        if(subsize != 1000){
          fprintf(stderr, "subsize(get) = %ld, expected 1000\n", subsize) ;
          fprintf(stderr, "FAILED\n") ;
          exit(1) ;
        }
        // check block
        errsub = subarray_check(SUB, SUB, SUB, copy,  i, SUB, j, SUB, k, SUB) ;
        if(0 != errsub){
          fprintf(stderr, "errsub(copy) = %d [%3d,%3d,%3d]\n", errsub, i, j, k) ;
          fprintf(stderr, "FAILED\n") ;
          exit(1) ;
        }
        // copy block into b3
        subsize = subarray_set(&b3, copy, sizeof(copy)) ;
        if(subsize != 1000){
          fprintf(stderr, "subsize(set) = %ld, expected 1000\n", subsize) ;
          fprintf(stderr, "FAILED\n") ;
          exit(1) ;
        }
        // check a3
        errsub = subarray_check(GNI, GNJ, GNK, (void *) ptra, i, SUB, j, SUB, k, SUB) ;
        if(0 != errsub){
          fprintf(stderr, "errsub(ptra) = %d\n", errsub) ;
          fprintf(stderr, "FAILED\n") ;
          exit(1) ;
        }
        // check b3
        errsub = subarray_check(GNI, GNJ, GNK, (void *) ptrb, i, SUB, j, SUB, k, SUB) ;
        if(0 != errsub){
          fprintf(stderr, "errsub(ptrb) = %d\n", errsub) ;
          fprintf(stderr, "FAILED\n") ;
          exit(1) ;
        }

        // set erroneous values in block, check that we are getting the right number of errors
        for(l=0 ; l<SUB ; l++) copy[l][l][l] = -1 ;
        errsub = subarray_check(SUB, SUB, SUB, copy,  i, SUB, j, SUB, k, SUB) ;
        if(SUB != errsub){
          fprintf(stderr, "errsub(copy) = %d, expected %d\n", errsub, SUB) ;
          fprintf(stderr, "FAILED\n") ;
          exit(1) ;
        }
        // save current value from a3
        for(l=0 ; l<SUB ; l++) saved[l] = get_subarray(GNI, GNJ, GNK, (void *) ptra, l, l, l) ;
        // set erroneous values in a3
        for(l=0 ; l<SUB ; l++) set_subarray(GNI, GNJ, GNK, (void *) ptra, l, l, l, -1) ;
        errsub = subarray_check(GNI, GNJ, GNK, (void *) ptra, i, SUB, j, SUB, k, SUB) ;
        if(SUB != errsub){
          fprintf(stderr, "errsub(ptra) = %d, expected %d [%3d,%3d,%3d]\n", errsub, SUB, i, j, k) ;
          fprintf(stderr, "FAILED\n") ;
          exit(1) ;
        }
        // restore saved value into a3
        for(l=0 ; l<SUB ; l++) set_subarray(GNI, GNJ, GNK, (void *) ptra, l, l, l, saved[l]) ;
      }
    }
  }
  fprintf(stderr, "SUCCESS\n") ;

  return 0 ;

}
