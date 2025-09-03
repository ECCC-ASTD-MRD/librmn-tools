#include <stdio.h>

void vfunctions_init() ;
void vfunctions2_init() ;

int main(int argc, char **argv){
  fprintf(stderr, "start of vfunctions test\n") ;
  vfunctions_init() ;
  vfunctions2_init() ;
  fprintf(stderr, "end of vfunctions test\n") ;
}
