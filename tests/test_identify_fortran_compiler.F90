#include <rmn/is_fortran_compiler.h>

program identify_compiler
  implicit none
#include <rmn/identify_fc_compiler.h>

  call start_of_test("is fortran compiler"//achar(0))
#if defined(IN_FORTRAN_CODE)
  print *, "IN_FORTRAN_CODE is defined"
#else
  print *, "ERROR : IN_FORTRAN_CODE is NOT defined"
  call abort   ! IN_FORTRAN_CODE is NOT defined
#endif

#if ! defined(FORTRAN_COMPILER_ID)
  print *, 'ERROR : FORTRAN_COMPILER_ID is NOT defined'
  call abort   ! FORTRAN_COMPILER_ID is NOT defined
#else
  print '(A,I3,A)', ' Fortran address size =', ADDRESS_SIZE,' bits'
  print '(A)', ' Fortran compiler is '//FORTRAN_COMPILER_ID
#endif
end
