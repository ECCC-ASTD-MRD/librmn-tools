/*
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation,
 * version 2.1 of the License.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 */
/*
 * interfaces to the Fortran and C functions/subroutines for Lorenzo prediction
 */

#if ! defined(IN_FORTRAN_CODE) && ! defined(__GFORTRAN__)

// the in place calls are now handled by the normal calls

void LorenzoPredict(int32_t *orig, int32_t *diff, int ni, int lnio, int lnid, int nj);
// void LorenzoPredictInplace(int32_t * restrict orig, int ni, int lnio, int nj);
void LorenzoUnpredict(int32_t *orig, int32_t *diff, int ni, int lnio, int lnid, int nj);
// void LorenzoUnpredictInplace(int32_t * restrict orig, int ni, int lnio, int nj);

#define LorenzoPredict1D(orig, diff, ni)   LorenzoPredict(orig, diff, ni, ni, ni, 1)
#define LorenzoUnpredict1D(orig, diff, ni) LorenzoUnpredict(orig, diff, ni, ni, ni, 1)

#else

interface
  subroutine lorenzopredict(orig, diff, ni, lnio, lnid, nj) bind(C,name='LorenzoPredict')
    import :: C_INT32_T
    implicit none
    integer(C_INT32_T), intent(IN), value :: ni, lnio, lnid, nj
    integer(C_INT32_T), dimension(lnio,nj), intent(IN)  :: orig
    integer(C_INT32_T), dimension(lnid,nj), intent(OUT) :: diff
  end subroutine lorenzopredict
!  subroutine lorenzopredictinplace(orig, ni, lnio, nj) bind(C,name='LorenzoPredictInplace')
!    import :: C_INT32_T
!    implicit none
!    integer(C_INT32_T), intent(IN), value :: ni, lnio, nj
!    integer(C_INT32_T), dimension(lnio,nj), intent(IN)  :: orig
!  end subroutine lorenzopredictinplace
  subroutine lorenzounpredict(orig, diff, ni, lnio, lnid, nj) bind(C,name='LorenzoUnpredict')
    import :: C_INT32_T
    implicit none
    integer(C_INT32_T), intent(IN), value :: ni, lnio, lnid, nj
    integer(C_INT32_T), dimension(lnio,nj), intent(IN)  :: orig
    integer(C_INT32_T), dimension(lnid,nj), intent(OUT) :: diff
  end subroutine lorenzounpredict
!  subroutine lorenzounpredictinplace(orig, ni, lnio, nj) bind(C,name='LorenzoUnpredictInplace')
!    import :: C_INT32_T
!    implicit none
!    integer(C_INT32_T), intent(IN), value :: ni, lnio, nj
!    integer(C_INT32_T), dimension(lnio,nj), intent(IN)  :: orig
!  end subroutine lorenzounpredictinplace
end interface

#endif
