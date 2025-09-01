#include <stdio.h>
#include <rmn/identify_fc_compiler.h>

char *identify_c_compiler(){
//  printf("compiler = '%s', address mode = %d bits\n", C_COMPILER_ID, ADDRESS_SIZE) ;
  return C_COMPILER_ID ;
}
int identify_address_mode(){
  return ADDRESS_SIZE ;
}
