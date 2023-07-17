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
#include <rmn/eval_diff.h>

// update error statistics structure with differences between arrays fref and fnew
// fref   : reference float array
// fnew   : array for which difference analysis is performed
// nd     : number of values to be added to statistics
// e      : error statistics structure
int update_error_stats(float *fref, float *fnew, int nd, error_stats *e){
  double bias = 0.0 ;
  double abs_error = 0.0 ;
  float max_error = 0.0f ;
  float temp ;
  int i ;
  double sum = 0.0 ;

  if(nd <= 0){
    e->ndata = 0 ;
    e->bias = 0.0 ;
    e->abs_error = 0.0 ;
    e->max_error = 0.0f ;
    e->sum = 0.0 ;
  }
  nd = (nd < 0) ? -nd : nd ;
  for(i=0 ; i<nd ; i++){
    temp = (fref[i] - fnew[i]) ;
    bias += temp ;
    sum += fref[i] ;
    temp = ABS(temp) ;
    abs_error += temp ;
    max_error = MAX(max_error, temp) ;
  }
  e->ndata += nd ;
  e->bias  += bias ;
  e->abs_error += abs_error ;
  e->max_error =  MAX(e->max_error, max_error) ;
  e->sum  += sum ;
  return nd ;
}

// produce difference statistics between 2 full arrays
// fref   : reference float array
// fnew   : array for which difference analysis is performed
// nr     ; number of useful elements in a row
// lref   : storage length of rows (array fref)
// lnew   : storage length of rows (array fnew)
// nj     : number of rows
int float_array_differences(float *fref, float *fnew, int nr, int lref, int lnew, int nj, error_stats *e){
  int j ;
  update_error_stats(fref, fnew, 0, e) ;
  for(j=0 ; j<nj ; j++){
    update_error_stats(fref, fnew, nr, e) ;    // update stats for this row
    fref += lref ;                              // next row
    fnew += lnew ;                              // next row
  }
  return 0 ;
}
