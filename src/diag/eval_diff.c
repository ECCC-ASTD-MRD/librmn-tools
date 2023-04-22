// Hopefully useful software
// Copyright (C) 2023  Recherche en Prevision Numerique
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

#include <rmn/misc_operators.h>

typedef struct{
  int ndata ;         // numbre of data points involved
  float bias ;        // running sum of differences between float arrays
  float abs_error ;   // running sum of absolute differences between float arrays
  float max_error ;   // largest absolute difference
} error_stats ;

// update error statistics structure
// fref   : reference float array
// fnew   : array for which difference analysis is performed
// nd     : number of values to be added to statistics
// e      : error statistics structure
int update_error_stats(float *fref, float *fnew, int nd, error_stats *e){
  float bias = 0.0f ;
  float abs_error = 0.0f ;
  float max_error = 0.0f ;
  float temp ;
  int i ;

  if(nd <= 0){
    e->ndata = 0 ;
    e->bias = 0 ;
    e->abs_error = 0 ;
    e->max_error = 0 ;
  }
  nd = (nd < 0) ? -nd : nd ;
  for(i=0 ; i<nd ; i++){
    temp = (fref - fnew) ;
    bias += temp ;
    temp = ABS(temp) ;
    abs_error += temp ;
    max_error = MAX(max_error, temp) ;
  }
  e->ndata += nd ;
  e->bias  += bias ;
  e->abs_error += abs_error ;
  e->max_error =  MAX(e->max_error, max_error) ;
  return nd ;
}

// produce difference statistics between 2 full arrays
// fref   : reference float array
// fnew   : array for which difference analysis is performed
// ni     ; number of useful elements in a row
// lni    : storage length of rows
// nj     : number of rows
int float_array_differences(float *fref, float *fnew, int ni, int lni, int nj, error_stats *e){
  int j ;
  update_error_stats(fref, fnew, 0, e) ;
  for(j=0 ; j<nj ; j++){
    update_error_stats(fref, fnew, ni, e) ;    // update stats for this row
    fref += lni ;                              // next row
    fnew += lni ;                              // next row
  }
}