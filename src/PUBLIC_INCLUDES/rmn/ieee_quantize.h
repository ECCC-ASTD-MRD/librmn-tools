/* Hopefully useful routines for C and FORTRAN
 * Copyright (C) 2021  Recherche en Prevision Numerique
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation,
 * version 2.1 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#if ! defined(IEEE_QUANTIZE_INCLUDES)
#define IEEE_QUANTIZE_INCLUDES

#include <stdint.h>
// #include <rmn/tools_types.h>

typedef struct{
  int32_t e0 ;       // reference exponent (used at unquantize time) (ieee quantization)
                     // true exponent of largest absolute value
  int32_t nbits ;    // maximum number of bits retained in quantized token
  int32_t nexp ;     // number of bits for the exponenent (ieee quantization)
  int32_t min ;      // used for minimum quantized value (all quantizations)
  int32_t max ;      // used for maximum quantized value (all quantizations)
  float amin ;       // smallest non zero absolute value (setup)
  float fmin ;       // minimum signed value (setup)
  float fmax ;       // largest signed value (setup)
  float fmaxa ;      // largest absolute value (setup)
  float rng ;        // range (power of 2) (setup)
  float rnga ;       // range of absolute values (power of 2) (setup)
  float epsi ;       // lowest absolute value considered as non zero
  float quant ;      // quantization unit (power of 2) (linear quantization)
  int32_t sbit ;     // 1 if sign bit needed
  int32_t negative ; // all numbers are negative
  uint32_t limit ;   // maximum absolute value possible
} qhead ;            // quantization information header

typedef struct{
  uint32_t maxa ;   // IEEE32 bit pattern of largest absolute value
  uint32_t mina ;   // IEEE32 bit pattern of smallest absolute value
  int32_t  maxs ;   // IEEE32 bit pattern of highest signed value
  int32_t  mins ;   // IEEE32 bit pattern of lowest signed value
}limits_i ;

typedef struct{
  float maxa ;      // IEEE32 bit pattern of largest absolute value
  float mina ;      // IEEE32 bit pattern of smallest absolute value
  float maxs ;      // IEEE32 bit pattern of highest signed value
  float mins ;      // IEEE32 bit pattern of lowest signed value
}limits_f ;

typedef union{
  limits_i i ;
  limits_f f ;
} limits_32 ;

void int32_extrema(void * restrict f, int np, limits_32 *l);

float quantum_adjust(float quantum);

void IEEE32_extrema(void * restrict f, int np, limits_32 *extrema);
uint64_t IEEE32_limits(void * restrict f, int np);
void IEEE32_exp_limits(uint64_t u64, int *emin, int *emax);

uint64_t IEEE32_linear_prep_0(uint64_t u64, int np, int nbits, float quant);
uint64_t IEEE32_linear_prep_1(uint64_t u64, int np, int nbits, float quant);

uint64_t IEEE32_linear_quantize_0(void * restrict f, int ni, int nbits, float quantum, void * restrict q);
uint64_t IEEE32_linear_quantize_1(void * restrict f, int ni, int nbits, float quantum, void * restrict q);

int IEEE32_linear_unquantize_0(void * restrict q, uint64_t h64, int ni, void * restrict f);
int IEEE32_linear_unquantize_1(void * restrict q, uint64_t h64, int ni, void * restrict f);

uint64_t IEEE32_fakelog_quantize_0(void * restrict f, int ni, int nbits, float qzero, void * restrict qs);
int IEEE32_fakelog_unquantize_0(void * restrict q, uint64_t h64, int ni, void * restrict f);

void quantize_setup(float *z,            // array to be quantized (IEEE 754 32 bit float) (INPUT)
                        int n,           // number of data elements
                        qhead *h);       // quantization control information (OUTPUT)
void ieee_clip(void *f, int n, int nbits);
int32_t ieee_quantize(float *f,        // array to quantize (IEEE 754 32 bit float) (INPUT)
                      int32_t *q,      // quantized data (OUTPUT)
                      int n,           // number of data elements
                      int nexp,        // number of bits for the exponent part of quantized data (INPUT)
                      int nbits,       // number of bits in quantized data (INPUT)
                      qhead *h);       // quantization control information (OUTPUT)
int32_t ieee_quantize_v4(float *f,        // array to quantize (IEEE 754 32 bit float) (INPUT)
                      int32_t *q,      // quantized data (OUTPUT)
                      int n,           // number of data elements
                      int nexp,        // number of bits for the exponent part of quantized data (INPUT)
                      int nbits,       // number of bits in quantized data (INPUT)
                      qhead *h);       // quantization control information (OUTPUT)
int32_t ieee_unquantize(float *f,      // restored array (IEEE 754 32 bit float) (OUTPUT)
                        int32_t *q,    // quantized array (INPUT)
                        int n,         // number of data elements (INPUT)
                        qhead *h);     // quantization control information (INPUT)
void fp32_to_fp16_scaled(float *f, uint16_t *q, int n, float scale);
void fp32_to_fp16(float *f, uint16_t *q, int n);
void fp16_to_fp32(float *f, void *f16, int n, void *inf);
void fp16_to_fp32_scaled(float *f, void *f16, int n, void *inf, float scale);

#endif

