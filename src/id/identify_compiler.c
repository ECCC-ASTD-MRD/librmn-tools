#include <stdio.h>
#include <rmn/identify_compiler_c.h>

char *identify_compiler(){
//  printf("compiler = '%s', address mode = %d bits\n", C_COMPILER, ADDRESS_MODE) ;
  return C_COMPILER ;
}
int identify_address_mode(){
  return ADDRESS_MODE ;
}
