/*
 * Hopefully useful code for C or Fortran
 * Copyright (C) 2022  Recherche en Prevision Numerique
 *
 * This code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation,
 * version 2.1 of the License.
 *
 * This code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 */
#if ! defined(DATA_INFO_PROTOTYPES)
#define DATA_INFO_PROTOTYPES

#if defined(IN_FORTRAN_CODE) || defined(__GFORTRAN__)

type, BIND(C) :: limits_f
  real(C_FLOAT) :: mins
  real(C_FLOAT) :: maxs
  real(C_FLOAT) :: mina
  real(C_FLOAT) :: maxa
  real(C_FLOAT) :: min0
end type

type, BIND(C) :: limits_i
  integer(C_INT32_T) :: mins
  integer(C_INT32_T) :: maxs
  integer(C_INT32_T) :: mina
  integer(C_INT32_T) :: maxa
  integer(C_INT32_T) :: min0
end type

interface
  function IEEE32_extrema(f, np) result(limits) bind(C, name='IEEE32_extrema')
    import :: C_FLOAT, C_INT32_T, limits_f
    implicit none
    real(C_FLOAT), dimension(*), intent(IN) :: f
    integer(C_INT32_T), intent(IN), value :: np
    type(limits_f) :: limits
  end function
  function int32_extrema(f, np) result(limits) bind(C, name='int32_extrema')
    import :: C_INT32_T, limits_i
    implicit none
    integer(C_INT32_T), dimension(*), intent(IN) :: f
    integer(C_INT32_T), intent(IN), value :: np
    type(limits_i) :: limits
  end function
!  function uint32_extrema(f, np) result(limits) bind(C, name='uint32_extrema')
!    import :: C_INT32_T, limits_i
!    implicit none
!    integer(C_INT32_T), dimension(*), intent(IN) :: f
!    integer(C_INT32_T), intent(IN), value :: np
!    type(limits_i) :: limits
!  end function
end interface

#else

#include <stdint.h>

typedef struct{
  int32_t  mins ;   // lowest signed value
  int32_t  maxs ;   // highest signed value
  uint32_t mina ;   // smallest absolute value
  uint32_t maxa ;   // largest absolute value
  uint32_t min0 ;   // smallest non zero absolute value
}limits_i ;

typedef struct{
  uint32_t mins ;   // lowest absolute value
  uint32_t maxs ;   // highest absolute value
  uint32_t mina ;   // smallest absolute value (same as mins)
  uint32_t maxa ;   // largest absolute value (same as maxs)
  uint32_t min0 ;   // smallest non zero absolute value
}limits_u ;

typedef struct{
  float mins ;      // IEEE32 bit pattern of lowest signed value
  float maxs ;      // IEEE32 bit pattern of highest signed value
  float mina ;      // IEEE32 bit pattern of smallest absolute value
  float maxa ;      // IEEE32 bit pattern of largest absolute value
  float min0 ;      // IEEE32 bit pattern of smallest non zero absolute value
}limits_f ;

typedef union{
  limits_i i ;
  limits_f f ;
} limits_w32 ;

limits_w32 uint32_extrema(void * restrict f, int np);

limits_w32 int32_extrema(void * restrict f, int np);
limits_w32 int32_extrema_missing(void * restrict f, int np, void * missing, uint32_t mmask, void *pad);

limits_w32 IEEE32_extrema(void * restrict f, int np);
limits_w32 IEEE32_extrema_missing(void * restrict f, int np, void * missing, uint32_t mmask, void *pad);
limits_w32 IEEE32_extrema_missing_simd(void * restrict f, int np, void * missing, uint32_t mmask, void *pad);
// limits_f IEEE32_extrema_c_missing(void * restrict f, int np, void *missing, uint32_t mmask, void *pad);

int float_info_no_missing(float *zz, int ni, int lni, int nj, float *maxval, float *minval, float *minabs);
int float_info_missing(float *zz, int ni, int lni, int nj, float *maxval, float *minval, float *minabs, float *spval, uint32_t spmask);
int float_info(float *zz, int ni, int lni, int nj, float *maxval, float *minval, float *minabs, float *spval, uint32_t spmask);

#endif

#endif
