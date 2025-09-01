#include <stdio.h>
#include <stdlib.h>
#include <rmn/identify_fc_compiler.h>
#include <rmn/test_helpers.h>

// int main(int argc, char **argv){
int main(){
  start_of_test("identify_c_compiler");
#if defined(C_COMPILER_ID)
  fprintf(stderr, "compiler = '%s', address mode = %d bits\n", C_COMPILER_ID, ADDRESS_SIZE) ;
#else
  fprintf(stderr, "ERROR: C_COMPILER_ID is not defined\n") ;
  exit(1) ;
#endif
  return 0 ;
}
