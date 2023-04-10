#if 0
/*
  Copyright (C) 2023  Environnement et Changement climatique Canada

  This is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation,
  version 2.1 of the License.

  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  Author:
      M. Valin,   Recherche en Prevision Numerique, 2023

  C and Fortran interfaces to the rmntools plugin software
*/
#endif

#if ! defined(IN_FORTRAN_CODE)
#include <rmn/is_fortran_compiler.h>
#endif

#if defined(IN_FORTRAN_CODE)
  interface
    function Unload_plugin(handle) result(status) BIND(C,name='Unload_plugin')
      import :: C_INT, C_PTR
      integer(C_INT) :: status
      type(C_PTR), intent(IN), value :: handle
    end function
    subroutine Set_plugin_diag(level) BIND(C,name='Set_plugin_diag')
      import C_INT
      integer(C_INT), value :: level
    end subroutine
    function Load_plugin(lib) result(handle) BIND(C,name='Load_plugin')
      import :: C_CHAR, C_INT, C_PTR
      character(C_CHAR), dimension(*), intent(IN) :: lib
      type(C_PTR) :: handle
    end function
    function Plugin_n_functions(handle) result(number) BIND(C,name='Plugin_n_functions')
      import :: C_PTR, C_INT
      type(C_PTR), intent(IN), value :: handle
      integer(C_INT) :: number
    end function
    function Plugin_function_name(handle,ordinal) result(string) BIND(C,name='Plugin_function_name')
      import :: C_PTR, C_INT
      type(C_PTR), intent(IN), value :: handle
      integer(C_INT), value :: ordinal
      type(C_PTR) :: string
    end function
    function Plugin_function(handle,fname) result(faddress) BIND(C,name='Plugin_function')
      import :: C_PTR, C_FUNPTR, C_CHAR
      type(C_PTR), intent(IN), value :: handle
      character(C_CHAR), dimension(*), intent(IN) :: fname
      type(C_FUNPTR) :: faddress
    end function
  end interface

#else

  int Unload_plugin(void *handle);
  void Set_plugin_diag(int diag);
  void *Load_plugin(const char *lib);
  int Plugin_n_functions(const void *handle);
  const char* Plugin_function_name(const void *handle, int ordinal);
  const char* *Plugin_function_names(const void *handle);
  void *Plugin_function(const void *handle, const char *name);

#endif
