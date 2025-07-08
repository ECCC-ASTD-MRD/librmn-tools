#include <stdio.h>
#include <rmn/identify_c_compiler.h>

char *identify_c_compiler(){
//  printf("compiler = '%s', address mode = %d bits\n", C_COMPILER, ADDRESS_MODE) ;
  return C_COMPILER_NAME ;
}
int identify_address_mode(){
  return ADDRESS_MODE ;
}
