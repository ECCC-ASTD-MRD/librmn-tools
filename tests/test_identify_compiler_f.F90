#include <rmn/is_fortran_compiler.h>

program identify_compiler
  implicit none
#include <rmn/identify_compiler_f.h>

#if defined(IN_FORTRAN_CODE)
  print *, "IN_FORTRAN_CODE is defined"
#else
  print *, "IN_FORTRAN_CODE is NOT defined"
#endif

  print *, "compiler = '"//FORTRAN_COMPILER//"'"
  print *, "address size =", ADDRESS_SIZE
end
