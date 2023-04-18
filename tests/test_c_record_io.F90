program test_c_record
  use ISO_C_BINDING
  use ISO_FORTRAN_ENV
  implicit none
#define IN_FORTRAN_CODE
#include <rmn/c_record_io.h>
  integer, dimension(6), target :: buf
  integer :: ndim, fd, ndata
  integer, dimension(3) :: dims
  type(C_PTR) :: pbufw, pbufr

  dims = [1, 2, 3]
  ndim = 3
  pbufw = C_LOC(buf(1))
  call start_of_test("test c record"//achar(0))
  fd = 0
  ndata = write_32bit_data_record("./temporary_file"//achar(0), fd, [1, 2, 3], 3, pbufw)
  print *,'number of items written =', ndata
  ndata = write_32bit_data_record(achar(0), fd, [1, 2, 3], 3, C_NULL_PTR)
  print *,'ndata =', ndata
end
