// Hopefully useful code for C
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
// Author:
//     M. Valin,   Recherche en Prevision Numerique, 2023
//
#include <rmn/entropy.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

// create a new entropy table 
// n    [IN] largest expected (value & mask)
// mask [IN] used to mask values
// return pointer to new entropy table
entropy_table *NewEntropyTable(int n, uint32_t mask)
{
  entropy_table *etab = (entropy_table *) malloc( sizeof(entropy_table) + n * sizeof(uint32_t) ) ;
  etab->mask = mask ;
  etab->mark = 0xBEBEFADAu ; ;
  etab->size = n ;
  ENTROPY_INIT(*etab) ;
  return etab ;
}

int ValidEntropyTable(entropy_table *etab)
{
  if(etab == NULL) return 0 ;
  if(etab->mark != 0xBEBEFADAu) return 0 ;
  return 1 ;
}

// reset global count and population counts of entropy table to 0
// etab [INOUT] pointer to entropy table
int ResetEntropyTable(entropy_table *etab)
{
  if( ! ValidEntropyTable(etab)) return 0 ;
  ENTROPY_INIT(*etab) ;
  return 1 ;
}

// update entropy table using values from pdata
// etab [INOUT] pointer to entropy table
// pdata   [IN] data to use to update entropy table
// ndata   [IN] number of data values
// return number of updates made ( <= ndata)
// N.B. should pdata[i] & mask exceed the table size, it will be ignored
int32_t UpdateEntropyTable(entropy_table *etab, void *pdata, int ndata)
{
  int32_t nval = 0 ;
  uint32_t token ;
  int i ;
  uint32_t mask = etab->mask ;
  uint32_t *pop = etab->pop ;
  uint32_t npop = etab->npop ;
  uint32_t nrej = etab->nrej ;
  uint32_t size = etab->size ;
  uint32_t *data = (uint32_t *) pdata ;

  if( ! ValidEntropyTable(etab)) goto error ;
  for(i=0 ; i<ndata ; i++){
    token = data[i] & mask ;  // apply mask
    if(token < size){         // token within bounds
      npop++ ;                // bump global count
      nval++ ;                // bump number of data values processed
      pop[token]++ ;          // bump appropriate table entry
    }else{
      nrej++ ;                // bump reject count
    }
  }
  etab->npop = npop ;         // update global population count
  etab->nrej = nrej ;         // update global reject count
  return nval ;               // return number of data values processed
error:
  return -1 ;
}

// compute entropy in bits/value using entropy table
// etab [IN] pointer to entropy table
// return computed entropy from table contents
// N.B. rejected values will not be accounted for
float ComputeEntropy(entropy_table *etab)
{
  int i;
  uint32_t *tab = etab->pop ;
  float k = 1.0f / etab->npop ;
  float sum = 0.0, temp;

  // sum of P * log2(P) where P is the probability of tab[i]
  for(i=0 ; i<etab->size ; i++) { 
//     if(tab[i] != 0){
      temp = k * tab[i] ;                      // probability of tab[i]
//       sum -= ( temp * FasterLog2(temp) ) ;   // P * log2(P)
      sum -= ( temp * FastLog2(temp) ) ;       // P * log2(P)
//     }
  } ;

  return sum;
}

#if defined(__AVX2__)
float VComputeEntropy(entropy_table *etab)
{
  int i;
  uint32_t *tab = etab->pop ;
  float k = 1.0f / etab->npop ;
  float sum = 0.0 ;
  v48mf *ptab ;
  v8sf temp, vsum ;
  v8sf vk ;

  vk = v8sfl1(k) ;                                   // promote k to 8 element float vector
  vsum = (v8sf) v8zero(vsum) ;                    // set vsum to 0
  // sum of P * log2(P) where P is the probability of tab[i]
  for(i=0 ; i<etab->size-7 ; i+=8) { 
    ptab = (v48mf *) (tab + i) ;
    temp = ptab->v256 * vk ;                        // probability of tab[i]
    vsum = vsum - ( temp * V8FastLog2(temp) ) ;     // P * log2(P)
  } ;
  sum = v256sumf(vsum ) ;
  for(  ; i<etab->size ; i++) {
    float t ;
    t = k * tab[i] ;
    sum -= ( t * FastLog2(t) ) ;
  }

  return sum;
}
#endif
