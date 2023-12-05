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

#if ! defined(ENTROPY_INIT) 

#include <stdint.h>

#define ENTROPY_INIT(E) { int i ; for(i=0 ; i<(E).size ; i++) (E).pop[i] = 0 ; (E).npop = 0 ; }
#define CM126 126.96f

typedef struct{
  uint32_t mask ;
  uint32_t size ;
  uint32_t npop ;
  uint32_t mark ;
  uint32_t pop[] ;
} entropy_table ;

static float  VeryFastLog2(float x)
{
  union { float f; uint32_t i; } vx = { .f = x } ;
  float y = vx.i;
  y *= 1.1920928955078125e-7f;
  return y - CM126;
}

static float FastLog2 (float x)
{
  union { float f; uint32_t i; } vx = { x };
  union { uint32_t i; float f; } mx = { (vx.i & 0x007FFFFF) | 0x3f000000 };
  float y = vx.i;
  y *= 1.1920928955078125e-7f;

  return y - 124.22551499f
           - 1.498030302f * mx.f 
           - 1.72587999f / (0.3520887068f + mx.f);
}

entropy_table *NewEntropyTable(int n, uint32_t mask);
int ValidEntropyTable(entropy_table *etab);
int ResetEntropyTable(entropy_table *etab);
int32_t UpdateEntropyTable(entropy_table *etab, void *pdata, int ndata);
float ComputeEntropy(entropy_table *etab);

#endif
