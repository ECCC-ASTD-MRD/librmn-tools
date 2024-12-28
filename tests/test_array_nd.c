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

void array_lbounds_check(int low, int high){
  array_1d a1 = array_1d_0 ;
  array_2d a2 = array_2d_0 ;
  array_3d a3 = array_3d_0 ;
  array_4d a4 = array_4d_0 ;
  array_5d a5 = array_5d_0 ;
  int32_t scrap[1024*1024] ;

  new_array(&a1, scrap, sizeof(int32_t), 1, 8) ;
  set_array_lbounds(&a1 , low, high) ;
  new_array(&a2, scrap, sizeof(int32_t), 1, 8, 7) ;
  set_array_lbounds(&a2 , low, high, low, high) ;
  new_array(&a3, scrap, sizeof(int32_t), 1, 8, 7, 6) ;
  set_array_lbounds(&a3 , low, high, low, high, low, high) ;
  new_array(&a4, scrap, sizeof(int32_t), 1, 8, 7, 6, 5) ;
  set_array_lbounds(&a4 , low, high, low, high, low, high, low, high) ;
  new_array(&a5, scrap, sizeof(int32_t), 1, 8, 7, 6, 5, 4) ;
  set_array_lbounds(&a5 , low, high, low, high, low, high, low, high, low, high) ;
}

int main(int argc, char **argv){

  if(argc > 1 && argv[0] == NULL) return 1 ;  // useless code to get rid of compiler warning

  fprintf(stderr, "=============== array_lbounds check ===============\n") ;
  array_lbounds_check(1, 3);

  return 0 ;

}
