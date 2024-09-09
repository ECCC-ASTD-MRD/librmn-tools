#include <stdio.h>
#include <stdlib.h>
#include <rmn/identify_compiler_c.h>
#include <rmn/test_helpers.h>

char *identify_compiler() ;
int identify_address_mode() ;

// int main(int argc, char **argv){
int main(){
  start_of_test("identify_compiler_c");
  fprintf(stderr, "compiler = '%s', address mode = %d bits\n", C_COMPILER, ADDRESS_MODE) ;
  fprintf(stderr, "identify_compiler() = '%s'\n", identify_compiler()) ;
  fprintf(stderr, "identify_address_mode() = %d\n", identify_address_mode()) ;
  return 0 ;
}
