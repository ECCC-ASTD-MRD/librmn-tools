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

#if ! defined(EVAL_DIFF)
#define EVAL_DIFF


typedef struct{
  int ndata ;         // number of data points involved
  float max_error ;   // largest absolute difference
  double bias ;       // running sum of differences between float arrays
  double abs_error ;  // running sum of absolute differences between float arrays
  double sum ;        // running sum for reference array
} error_stats ;

int update_error_stats(float *fref, float *fnew, int nd, error_stats *e);
int float_array_differences(float *fref, float *fnew, int nr, int lref, int lnew, int nj, error_stats *e);
int32_t array_compare_masked(void *f1, void *f2, int n, uint32_t mask);

#endif
