integer(C_INT) function FortranConstructor0(what) BIND(C, name='FortranConstructor0')
  use ISO_C_BINDING
  implicit none
  integer(C_INT), intent(IN), value :: what
  FortranConstructor0 = -what
end function
