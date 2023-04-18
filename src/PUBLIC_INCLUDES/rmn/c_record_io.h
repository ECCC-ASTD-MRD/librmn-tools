/* Hopefully useful software
 * Copyright (C) 2023  Recherche en Prevision Numerique
 *
 * This is free software; you can redistribute it and/or
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
#if defined(IN_FORTRAN_CODE) || defined(__GFORTRAN__)

interface
  function write_32bit_data_record(filename, fdi, dims, ndim, buf) result(ndata) bind(C, name='write_32bit_data_record')
    import :: C_CHAR, C_INT32_T, C_PTR
    implicit none
    character(C_CHAR), intent(IN) :: filename
    integer(C_INT32_T), intent(INOUT) :: fdi
    integer(C_INT32_T), intent(IN), value :: ndim
    integer(C_INT32_T), dimension(ndim), intent(IN) :: dims
    type(C_PTR), intent(IN), value :: buf
    integer(C_INT32_T) :: ndata
  end function
  function read_32bit_data_record(filename, fdi, dims, ndim, ndata) result(array) bind(C,name='read_32bit_data_record')
    import :: C_CHAR, C_INT32_T, C_PTR
    implicit none
    character(C_CHAR), intent(IN) :: filename
    integer(C_INT32_T), intent(INOUT) :: fdi
    integer(C_INT32_T), intent(INOUT) :: ndim
    integer(C_INT32_T), dimension(ndim), intent(INOUT) :: dims
    integer(C_INT32_T), intent(OUT) :: ndata
    type(C_PTR) :: array
  end function
end interface

#else

int write_32bit_data_record(char *filename, int *fdi, int *dims, int ndim, void *buf);
void *read_32bit_data_record(char *filename, int *fdi, int *dims, int *ndim, int *ndata);

#endif
