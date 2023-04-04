#include <stdio.h>
#include <stdlib.h>
#include <rmn/identify_compiler_c.h>
#include <rmn/test_helpers.h>

char *identify_compiler() ;
int identify_address_mode() ;

int main(int argc, char **argv){
  start_of_test("test_identify_compiler_c");
  printf("compiler = '%s', address mode = %d bits\n", C_COMPILER, ADDRESS_MODE) ;
  printf("identify_compiler() = '%s'\n", identify_compiler()) ;
  printf("identify_address_mode() = %d\n", identify_address_mode()) ;
exit(1) ;
}
