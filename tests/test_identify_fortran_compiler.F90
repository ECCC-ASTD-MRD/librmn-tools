#include <rmn/is_fortran_compiler.h>

program identify_compiler
  implicit none
#include <rmn/identify_fortran_compiler.hf>

  call start_of_test("is fortran compiler"//achar(0))
#if defined(IN_FORTRAN_CODE)
  print *, "IN_FORTRAN_CODE is defined"
#else
  print *, "IN_FORTRAN_CODE is NOT defined"
#endif

  print *, "string = '"//fortran_compiler_name//"'"
  print *, "macro  = '"//FORTRAN_COMPILER_NAME//"'"
  print *, "address size =", ADDRESS_SIZE
end
