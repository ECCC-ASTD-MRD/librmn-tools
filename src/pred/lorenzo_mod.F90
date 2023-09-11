! Copyright (C) 2022  Environnement et Changement climatique Canada
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
!     M. Valin,   Environnement et Changement climatique Canada, 2022-2023
!
! this module provides Fortran interfaces to the subroutines used for Loremzo prediction
! NOTE: the in place calls are now handled by the normal calls
!
module lorenzo_mod
  use ISO_C_BINDING
  implicit none
! interfaces to C versions
  interface
    ! the C version using SIMD is faster that the Fortran version
    ! 3 point Lorenzo predictor (forward derivative)
    ! upon exit, diff contains original value - predicted value
    subroutine lorenzopredict(orig, diff, ni, lnio, lnid, nj) bind(C,name='LorenzoPredict')
      import :: C_INT32_T
      implicit none
      integer(C_INT32_T), intent(IN), value :: ni, lnio, lnid, nj
      integer(C_INT32_T), dimension(lnio,nj), intent(IN)  :: orig
      integer(C_INT32_T), dimension(lnid,nj), intent(OUT) :: diff
    end subroutine lorenzopredict
    subroutine lorenzopredict_c(orig, diff, ni, lnio, lnid, nj) bind(C,name='LorenzoPredict')
      import :: C_INT32_T
      implicit none
      integer(C_INT32_T), intent(IN), value :: ni, lnio, lnid, nj
      integer(C_INT32_T), dimension(lnio,nj), intent(IN)  :: orig
      integer(C_INT32_T), dimension(lnid,nj), intent(OUT) :: diff
    end subroutine lorenzopredict_c

    ! the C version using SIMD is faster that the Fortran version
!     subroutine lorenzopredictinplace(f, ni, lni, nj) bind(C,name='LorenzoPredictInplace')
!       import :: C_INT32_T
!       implicit none
!       integer(C_INT32_T), intent(IN), value :: ni, lni, nj
!       integer(C_INT32_T), dimension(lni,nj), intent(IN)  :: f
!     end subroutine lorenzopredictinplace
!     subroutine lorenzopredictinplace_c(f, ni, lni, nj) bind(C,name='LorenzoPredictInplace')
!       import :: C_INT32_T
!       implicit none
!       integer(C_INT32_T), intent(IN), value :: ni, lni, nj
!       integer(C_INT32_T), dimension(lni,nj), intent(IN)  :: f
!     end subroutine lorenzopredictinplace_c

    subroutine lorenzounpredict(orig, diff, ni, lnio, lnid, nj) bind(C,name='LorenzoUnpredict')
      import :: C_INT32_T
      implicit none
      integer(C_INT32_T), intent(IN), value :: ni, lnio, lnid, nj
      integer(C_INT32_T), dimension(lnio,nj), intent(OUT)  :: orig
      integer(C_INT32_T), dimension(lnid,nj), intent(IN) :: diff
    end subroutine lorenzounpredict
    subroutine lorenzounpredict_c(orig, diff, ni, lnio, lnid, nj) bind(C,name='LorenzoUnpredict')
      import :: C_INT32_T
      implicit none
      integer(C_INT32_T), intent(IN), value :: ni, lnio, lnid, nj
      integer(C_INT32_T), dimension(lnio,nj), intent(OUT)  :: orig
      integer(C_INT32_T), dimension(lnid,nj), intent(IN) :: diff
    end subroutine lorenzounpredict_c

!     subroutine lorenzounpredictinplace(orig, ni, lnio, nj) bind(C,name='LorenzoUnpredictInplace')
!       import :: C_INT32_T
!       implicit none
!       integer(C_INT32_T), intent(IN), value :: ni, lnio, nj
!       integer(C_INT32_T), dimension(lnio,nj), intent(INOUT)  :: orig
!     end subroutine lorenzounpredictinplace
!     subroutine lorenzounpredictinplace_c(orig, ni, lnio, nj) bind(C,name='LorenzoUnpredictInplace')
!       import :: C_INT32_T
!       implicit none
!       integer(C_INT32_T), intent(IN), value :: ni, lnio, nj
!       integer(C_INT32_T), dimension(lnio,nj), intent(INOUT)  :: orig
!     end subroutine lorenzounpredictinplace_c
  end interface

  procedure(lorenzopredict_c) , pointer          :: predict => NULL()
!   procedure(lorenzopredictinplace_c) , pointer   :: predict_inplace => NULL()
  procedure(lorenzounpredict_c) , pointer        :: unpredict => NULL()
!   procedure(lorenzounpredictinplace_c) , pointer :: unpredict_inplace => NULL()


contains

! set procedure addresses to be used by the various Lorenzo predict/unpredict procedures
! subroutine set_lorenzopredict_addresses(predict_address, unpredict_address, predict_inplace_address, unpredict_inplace_address)
subroutine set_lorenzopredict_addresses(predict_address, unpredict_address)
  implicit none
  procedure(lorenzopredict_c) , pointer          :: predict_address
!   procedure(lorenzopredictinplace_c) , pointer   :: predict_inplace_address
  procedure(lorenzounpredict_c) , pointer        :: unpredict_address
!   procedure(lorenzounpredictinplace_c) , pointer :: unpredict_inplace_address

  if(associated(predict_address)) then
    predict => predict_address
  else
    predict => lorenzopredict_c
  endif

!   if(associated(predict_inplace_address)) then
!     predict_inplace => predict_inplace_address
!   else
!     predict_inplace => lorenzopredictinplace_c
!   endif

  if(associated(unpredict_address)) then
    unpredict => unpredict_address
  else
    unpredict => lorenzounpredict_c
  endif

!   if(associated(unpredict_inplace_address)) then
!     unpredict_inplace => unpredict_inplace_address
!   else
!     unpredict_inplace => lorenzounpredictinplace_c
!   endif
end subroutine

! reset addresses to C addresses of default C functions
! by calling set_lorenzopredict_addresses with typed NULL()
subroutine reset_lorenzopredict_addresses
  implicit none
  procedure(lorenzopredict_c) , pointer          :: predict_address => NULL()
!   procedure(lorenzopredictinplace_c) , pointer   :: predict_inplace_address => NULL()
  procedure(lorenzounpredict_c) , pointer        :: unpredict_address => NULL()
!   procedure(lorenzounpredictinplace_c) , pointer :: unpredict_inplace_address => NULL()
!   call set_lorenzopredict_addresses(predict_address, unpredict_address, predict_inplace_address, unpredict_inplace_address)
  call set_lorenzopredict_addresses(predict_address, unpredict_address)
end subroutine

#if ! defined(NO_EXTRA_FORTRAN__)
! 3 point Lorenzo predictor (forward derivative along i and j)
! upon exit, diff contains original value - predicted value
! with explicit SIMD primitives, the C version (LorenzoPredict_c) outperforms the Fortran version
subroutine lorenzopredict_f(orig, diff, ni, lnio, lnid, nj) bind(C,name='LorenzoPredict_f')
  implicit none
  integer(C_INT32_T), intent(IN), value :: ni, lnio, lnid, nj
  integer(C_INT32_T), dimension(lnio,nj), intent(IN)  :: orig
  integer(C_INT32_T), dimension(lnid,nj), intent(OUT) :: diff
  integer :: i, j

  if(associated(predict))then   ! if pointer to procedure is set, call that procedure
    call predict(orig, diff, ni, lnio, lnid, nj)
    return
  endif
  diff(1,1) = orig(1,1)
  do i = 2, ni
    diff(i,1) = orig(i,1) - orig(i-1,1)         ! botom row, 1D prediction along i
  enddo
  do j = 2 , nj                                 ! all rows except bottom row
    diff(1,j) = orig(1,j) - orig(1,j-1)         ! special case for leftmost column, 1D prediction along j
    do i = 2, ni
      diff(i,j) = orig(i,j) - ( orig(i-1,j) + orig(i,j-1) - orig(i-1,j-1) )  ! value - predicted value
    enddo
  enddo
end subroutine lorenzopredict_f

! Lorenzo restoration
! upon exit, orig contains values restored from prediction in diff
subroutine lorenzounpredict_f(orig, diff, ni, lnio, lnid, nj) bind(C,name='LorenzoUnpredict_f')
  implicit none
  integer(C_INT32_T), intent(IN), value :: ni, lnio, lnid, nj
  integer(C_INT32_T), dimension(lnio,nj), intent(OUT) :: orig
  integer(C_INT32_T), dimension(lnid,nj), intent(IN)  :: diff
  integer :: i, j

  if(associated(unpredict))then   ! if pointer to procedure is set, call that procedure
    call unpredict(orig, diff, ni, lnio, lnid, nj)
    return
  endif
  orig(1,1) = diff(1,1)
  do i = 2, ni
    orig(i,1) = diff(i,1) + orig(i-1,1)         ! botom row, 1D prediction along i
  enddo
  do j = 2 , nj                                 ! all rows except bottom row
    orig(1,j) = diff(1,j) + orig(1,j-1)         ! special case for leftmost column, 1D prediction along j
    do i = 2, ni
      orig(i,j) = diff(i,j) + ( orig(i-1,j) + orig(i,j-1) - orig(i-1,j-1) ) ! value + predicted value
    enddo
  enddo
end subroutine lorenzounpredict_f

! in place Lorenzo prediction
! upon exit, f contains original value - predicted alue
! with explicit SIMD primitives, the C version (LorenzoPredictInplace_c) outperforms the Fortran version
! subroutine lorenzopredictinplace_f(f, ni, lni, nj) bind(C,name='LorenzoPredictInplace_f')
!   implicit none
!   integer(C_INT32_T), intent(IN), value :: ni, lni, nj
!   integer(C_INT32_T), dimension(lni,nj), intent(INOUT) :: f
!   integer :: i, j
! 
!   if(associated(predict_inplace))then   ! if pointer to procedure is set, call that procedure
!     call predict_inplace(f, ni, lni, nj)
!     return
!   endif
!   do j = nj, 2, -1                                            ! all rows except bottom row
!     do i = ni, 2, -1
!       f(i,j) = f(i,j) - ( f(i-1,j) + f(i,j-1) - f(i-1,j-1) )  ! value - predicted value
!     enddo
!     f(1,j) = f(1,j) - f(1,j-1)                                ! special case for leftmost column, 1D prediction along j
!   enddo
!   do i = ni, 2, -1
!     f(i,1) = f(i,1) - f(i-1,1)                                ! botom row, 1D prediction along i
!   enddo
! end subroutine lorenzopredictinplace_f

! in place Lorenzo restore
! upon entry, f contains original value - predicted alue
! upon exit, f contains restored original values
! the Fortran version seems to have a performance >= C version
! subroutine lorenzounpredictinplace_f(f, ni, lni, nj) bind(C,name='LorenzoUnpredictInplace_f')
!   implicit none
!   integer(C_INT32_T), intent(IN), value :: ni, lni, nj
!   integer(C_INT32_T), dimension(lni,nj), intent(INOUT) :: f
!   integer :: i, j
! 
!   if(associated(unpredict_inplace))then   ! if pointer to procedure is set, call that procedure
!     call unpredict_inplace(f, ni, lni, nj)
!     return
!   endif
!   do i = 2, ni                                                ! bottom row, 1D prediction along i
!     f(i,1) = f(i,1) + f(i-1,1)                                ! (original - precicted) + predicted
!   enddo
!   do j = 2, nj
!     f(1,j) = f(1,j) + f(1,j-1)                                ! special case for leftmost column, 1D prediction along j
!     do i = 2,ni
!       f(i,j) = f(i,j)+ ( f(i-1,j) + f(i,j-1) - f(i-1,j-1) )   ! (original - precicted) + predicted
!     enddo
!   enddo
! end subroutine lorenzounpredictinplace_f
#endif
end module lorenzo_mod
