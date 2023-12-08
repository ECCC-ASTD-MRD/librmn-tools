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
#include <rmn/fastapprox.h>

#define ENTROPY_INIT(E) { int i ; for(i=0 ; i<(E).size ; i++) (E).pop[i] = 0 ; (E).npop = 0 ; (E).nrej = 0 ; }

typedef struct{
  uint32_t mask ;
  uint32_t size ;
  uint32_t mark ;
  uint32_t npop ;
  uint32_t nrej ;
  uint32_t pop[] ;
} entropy_table ;

#if 0
static float VeryFastPow2 (float p)
{
  float clipp = (p < -126) ? -126.0f : p;
  union { uint32_t i; float f; } v = { (uint32_t) ( (1 << 23) * (clipp + 126.94269504f) ) };
  return v.f;
}

static float FastPow2 (float p)
{
  float offset = (p < 0) ? 1.0f : 0.0f;
  float clipp = (p < -126) ? -126.0f : p;
  int w = clipp;
  float z = clipp - w + offset;
  union { uint32_t i; float f; } v = { (uint32_t) ( (1 << 23) * (clipp + 121.2740575f + 27.7280233f / (4.84252568f - z) - 1.49012907f * z) ) };

  return v.f;
}

static float VeryFastLog2(float x)
{
  union { float f; uint32_t i; } vx = { x };
  float y = vx.i;
  y *= 1.1920928955078125e-7f;
  return y - 126.94269504f;
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
#endif

entropy_table *NewEntropyTable(int n, uint32_t mask);
int ValidEntropyTable(entropy_table *etab);
int ResetEntropyTable(entropy_table *etab);
int32_t UpdateEntropyTable(entropy_table *etab, void *pdata, int ndata);
float ComputeEntropy(entropy_table *etab);

#endif
