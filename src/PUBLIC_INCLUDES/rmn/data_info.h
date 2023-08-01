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
  real(C_FLOAT)      :: mins   ! lowest signed value
  real(C_FLOAT)      :: maxs   ! highest signed value
  real(C_FLOAT)      :: mina   ! smallest absolute value
  real(C_FLOAT)      :: maxa   ! largest absolute value
  real(C_FLOAT)      :: min0   ! smallest non zero absolute value
  integer(C_INT8_T)  :: allp   ! 1 if all values are non negative
  integer(C_INT8_T)  :: allm   ! 1 if all values are negative
  integer(C_INT8_T)  :: maxe   ! highest IEEE exponent (with bias)
  integer(C_INT8_T)  :: mine   ! lowest IEEE exponent (with bias)
end type

type, BIND(C) :: limits_i
  integer(C_INT32_T) :: mins   ! lowest signed value
  integer(C_INT32_T) :: maxs   ! highest signed value
  integer(C_INT32_T) :: mina   ! smallest absolute value
  integer(C_INT32_T) :: maxa   ! largest absolute value
  integer(C_INT32_T) :: min0   ! smallest non zero absolute value
  integer(C_INT8_T)  :: allp   ! 1 if all values are non negative
  integer(C_INT8_T)  :: allm   ! 1 if all values are negative
  integer(C_INT8_T)  :: maxe   ! highest IEEE exponent (with bias)
  integer(C_INT8_T)  :: mine   ! lowest IEEE exponent (with bias)
end type

interface
  function IEEE32_extrema(f, np) result(limits) bind(C, name='IEEE32_extrema')
    import :: C_FLOAT, C_INT32_T, limits_f
    implicit none
    real(C_FLOAT), dimension(*), intent(IN) :: f
    integer(C_INT32_T), intent(IN), value :: np
    type(limits_f) :: limits
  end function
  function IEEE32_extrema_missing(f, np, missing, mmask, pad ) result(limits) bind(C, name='IEEE32_extrema_missing')
    import :: C_FLOAT, C_INT32_T, limits_f, C_PTR
    implicit none
    real(C_FLOAT), dimension(*), intent(IN) :: f
    integer(C_INT32_T), intent(IN), value :: np, mmask
    type(C_PTR), intent(IN), value :: missing, pad
    type(limits_f) :: limits
  end function
  function int32_extrema(f, np) result(limits) bind(C, name='INT32_extrema')
    import :: C_INT32_T, limits_i
    implicit none
    integer(C_INT32_T), dimension(*), intent(IN) :: f
    integer(C_INT32_T), intent(IN), value :: np
    type(limits_i) :: limits
  end function
  function int32_extrema_missing(f, np, missing, mmask, pad) result(limits) bind(C, name='INT32_extrema_missing')
    import :: C_INT32_T, limits_i, C_PTR
    implicit none
    integer(C_INT32_T), dimension(*), intent(IN) :: f
    integer(C_INT32_T), intent(IN), value :: np, mmask
    type(C_PTR), intent(IN), value :: missing, pad
    type(limits_i) :: limits
  end function
  function uint32_extrema(f, np) result(limits) bind(C, name='UINT32_extrema')
    import :: C_INT32_T, limits_i
    implicit none
    integer(C_INT32_T), dimension(*), intent(IN) :: f
    integer(C_INT32_T), intent(IN), value :: np
    type(limits_i) :: limits
  end function
  function uint32_extrema_missing(f, np, missing, mmask, pad) result(limits) bind(C, name='UINT32_extrema_missing')
    import :: C_INT32_T, limits_i, C_PTR
    implicit none
    integer(C_INT32_T), dimension(*), intent(IN) :: f
    integer(C_INT32_T), intent(IN), value :: np, mmask
    type(C_PTR), intent(IN), value :: missing, pad
    type(limits_i) :: limits
  end function
end interface

#else        // defined(IN_FORTRAN_CODE)

#include <stdint.h>
#include <rmn/ct_assert.h>

typedef struct{
  int32_t  mins ;   // lowest signed value
  int32_t  maxs ;   // highest signed value
  uint32_t mina ;   // smallest absolute value
  uint32_t maxa ;   // largest absolute value
  uint32_t min0 ;   // smallest non zero absolute value
  uint8_t  allp ;   // 1 if all values are non negative
  uint8_t  allm ;   // 1 if all values are negative
  uint8_t  maxe ;   // highest IEEE exponent (with bias)
  uint8_t  mine ;   // lowest IEEE exponent (with bias)
}limits_i ;

typedef struct{
  uint32_t mins ;   // lowest absolute value
  uint32_t maxs ;   // highest absolute value
  uint32_t mina ;   // smallest absolute value (same as mins)
  uint32_t maxa ;   // largest absolute value (same as maxs)
  uint32_t min0 ;   // smallest non zero absolute value
  uint8_t  allp ;   // 1 if all values are non negative
  uint8_t  allm ;   // 1 if all values are negative
  uint8_t  maxe ;   // highest IEEE exponent (with bias)
  uint8_t  mine ;   // lowest IEEE exponent (with bias)
}limits_u ;

typedef struct{
  float mins ;      // IEEE32 bit pattern of lowest signed value
  float maxs ;      // IEEE32 bit pattern of highest signed value
  float mina ;      // IEEE32 bit pattern of smallest absolute value
  float maxa ;      // IEEE32 bit pattern of largest absolute value
  float min0 ;      // IEEE32 bit pattern of smallest non zero absolute value
  uint8_t  allp ;   // 1 if all values are non negative
  uint8_t  allm ;   // 1 if all values are negative
  uint8_t  maxe ;   // highest IEEE exponent (with bias)
  uint8_t  mine ;   // lowest IEEE exponent (with bias)
}limits_f ;

typedef union{
  limits_i i ;
  limits_u u ;
  limits_f f ;
} limits_w32 ;

CT_ASSERT(sizeof(limits_w32) == 24)

// N.B. some compiler versions fail to compile when the return value of a function is larger than 128 bits

// true if all values are non negative (sign bit of maxs and mins is 0)
#define W32_ALLP(l) (((l.i.maxs | l.i.mins) >> 31) == 0)
// true if all values are negative or 0
#define W32_ALLM(l) ( (l.i.maxs <= 0) & (l.i.mins <= 0) )
// #define W32_ALLM(l) (((l.i.maxs & l.i.mins) >> 31) != 0)

int W32_replace_missing(void * restrict f, int np, void *spval, uint32_t mmask, void *pad);

limits_w32 UINT32_extrema(void * restrict f, int np);
limits_w32 UINT32_extrema_missing(void * restrict f, int np, void *spval, uint32_t mmask, void *pad);

limits_w32 INT32_extrema(void * restrict f, int np);
limits_w32 INT32_extrema_missing(void * restrict f, int np, void *spval, uint32_t mmask, void *pad);

limits_w32 IEEE32_extrema(void * restrict f, int np);
limits_w32 IEEE32_extrema_abs(void * restrict f, int np);
limits_w32 IEEE32_extrema_missing(void * restrict f, int np, void * missing, uint32_t mmask, void *pad);

#endif        // defined(IN_FORTRAN_CODE)

#endif        // ! defined(DATA_INFO_PROTOTYPES)
