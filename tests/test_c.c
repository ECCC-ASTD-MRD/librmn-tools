#include <stdio.h>

#include <internal.h>
#include <rmntools.h>
#include <rmn/rmn_tools.h>

void pred_dummy_c() ;
void dummy_c() ;

int main(int argc, char **argv){
#if defined(TESTC)
  printf("test_c O.K.\n");
#else
  printf("ERROR : test_c, TEST not defined\n");
#endif
dummy_c();
pred_dummy_c();
}

