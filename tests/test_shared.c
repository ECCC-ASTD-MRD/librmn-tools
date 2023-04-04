#include <stdio.h>
#include <rmn/test_helpers.h>

int name1(int) ;
int name2(int) ;
int name3(int) ;

int main(int argc, char **argv){
  start_of_test(argv[0]);
  fprintf(stderr, "name1(1) = %d\n", name1(1)) ;
  fprintf(stderr, "name2(2) = %d\n", name2(2)) ;
  fprintf(stderr, "name3(3) = %d\n", name3(3)) ;
}
