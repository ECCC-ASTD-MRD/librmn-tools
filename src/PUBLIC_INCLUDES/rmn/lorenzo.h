/*
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
 * (safe to include multiple times in same file)
 * the macros apply to both C and Fortran
 */

#undef LorenzoPredict1D
#define LorenzoPredict1D(orig, diff, ni)   LorenzoPredict(orig, diff, ni, ni, ni, 1)
#undef LorenzoUnpredict1D
#define LorenzoUnpredict1D(orig, diff, ni) LorenzoUnpredict(orig, diff, ni, ni, ni, 1)

#undef LorenzoPredictInplace
#define LorenzoPredictInplace(orig, ni, lnio, nj)   LorenzoPredict(orig, orig, ni, lnio, lnio, nj)
#undef LorenzoUnpredictInplace
#define LorenzoUnpredictInplace(orig, ni, lnio, nj) LorenzoUnpredict(orig, orig, ni, lnio, lnio, nj)

#if ! defined(IN_FORTRAN_CODE) && ! defined(__GFORTRAN__)

// the in place calls are now handled by the normal calls

void LorenzoPredict(int32_t * restrict orig, int32_t * restrict diff, int ni, int lnio, int lnid, int nj);
// void LorenzoPredictInplace(int32_t * restrict orig, int ni, int lnio, int nj);

void LorenzoUnpredict(int32_t * restrict orig, int32_t * restrict diff, int ni, int lnio, int lnid, int nj);
// void LorenzoUnpredictInplace(int32_t * restrict orig, int ni, int lnio, int nj);

#else

interface
  subroutine lorenzopredict(orig, diff, ni, lnio, lnid, nj) bind(C,name='LorenzoPredict')
    import :: C_INT32_T
    implicit none
    integer(C_INT32_T), intent(IN), value :: ni, lnio, lnid, nj
    integer(C_INT32_T), dimension(lnio,nj), intent(IN)  :: orig
    integer(C_INT32_T), dimension(lnid,nj), intent(OUT) :: diff
  end subroutine lorenzopredict
  subroutine lorenzounpredict(orig, diff, ni, lnio, lnid, nj) bind(C,name='LorenzoUnpredict')
    import :: C_INT32_T
    implicit none
    integer(C_INT32_T), intent(IN), value :: ni, lnio, lnid, nj
    integer(C_INT32_T), dimension(lnio,nj), intent(IN)  :: orig
    integer(C_INT32_T), dimension(lnid,nj), intent(OUT) :: diff
  end subroutine lorenzounpredict
end interface

#endif
