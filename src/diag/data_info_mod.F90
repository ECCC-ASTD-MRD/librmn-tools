! Copyright (C) 2023  Environnement et Changement climatique Canada
! 
! This is free software; you can redistribute it and/or
! modify it under the terms of the GNU Lesser General Public
! License as published by the Free Software Foundation,
! version 2.1 of the License.
! 
! This software is distributed in the hope that it will be useful,
! but WITHOUT ANY WARRANTY; without even the implied warranty of
! MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
! Lesser General Public License for more details.
! 
! Author:
!     M. Valin,   Environnement et Changement climatique Canada, 2023
!
module data_info_mod
  use ISO_C_BINDING
  implicit none
#define IN_FORTRAN_CODE
#include <rmn/data_info.h>
  interface extrema
    module procedure extrema_missing_f
    module procedure extrema_missing_i
  end interface
contains
  function extrema_missing_f(f, np, missing, mmask, pad) result(limits)
    implicit none
    real(C_FLOAT), dimension(*), intent(IN) :: f
    integer(C_INT32_T), intent(IN), value :: np
    integer(C_INT32_T), intent(IN), optional :: mmask
    real(C_FLOAT), intent(IN), optional, target :: missing, pad
    type(limits_f) :: limits

    integer(C_INT32_T) :: mmask_
    type(C_PTR) :: missing_, pad_

    missing_ = C_NULL_PTR
    pad_     = C_NULL_PTR
    mmask_   = -1
    if(present(missing))    missing_ = C_LOC(missing)
    if(present(    pad))    pad_     = C_LOC(pad)
    if(present(  mmask))    mmask_   = mmask
    limits = IEEE32_extrema_missing(f, np, missing_, mmask_, pad_ )
  end function
  function extrema_missing_i(f, np, missing, mmask, pad, unsigned) result(limits)
    implicit none
    integer(C_INT32_T), dimension(*), intent(IN) :: f
    integer(C_INT32_T), intent(IN), value :: np
    integer(C_INT32_T), intent(IN), optional, value  :: mmask
    logical, intent(IN), value, optional :: unsigned
    integer(C_INT32_T), intent(IN), optional, target :: missing, pad
    type(limits_i) :: limits

    integer(C_INT32_T) :: mmask_
    type(C_PTR) :: missing_, pad_
    logical :: unsigned_

    missing_ = C_NULL_PTR
    pad_     = C_NULL_PTR
    mmask_   = -1
    unsigned_= .false.
    if(present( missing))    missing_ = C_LOC(missing)
    if(present(     pad))    pad_     = C_LOC(pad)
    if(present(   mmask))    mmask_   = mmask
    if(present(unsigned))    unsigned_ = unsigned
    if(unsigned) then
      limits = UINT32_extrema_missing(f, np, missing_, mmask_, pad_)
    else
      limits =  INT32_extrema_missing(f, np, missing_, mmask_, pad_)
    endif
  end function
end module
