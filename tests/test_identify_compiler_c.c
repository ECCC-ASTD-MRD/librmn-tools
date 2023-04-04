#include <stdio.h>
#include <stdlib.h>
#include <rmn/identify_compiler_c.h>

char *identify_compiler() ;
int identify_address_mode() ;

int main(int argc, char **argv){
  printf("compiler = '%s', address mode = %d bits\n", C_COMPILER, ADDRESS_MODE) ;
  printf("identify_compiler() = '%s'\n", identify_compiler()) ;
  printf("identify_address_mode() = %d\n", identify_address_mode()) ;
exit(1) ;
}
