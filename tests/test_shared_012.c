#include <stdio.h>
#include <rmn/test_helpers.h>

int name1(int) ;
int name2(int) ;
int name3(int) ;
int _name_0_1(int) ;
int _name_0_2(int) ;
int _name_0_3(int) ;
int _name_1_1(int) ;
int _name_1_2(int) ;
int _name_1_3(int) ;
int _name_2_1(int) ;
int _name_2_2(int) ;
int _name_2_3(int) ;

int main(int argc, char **argv){
  start_of_test(argv[0]);
  fprintf(stderr, "name1(1) = %d\n", name1(1)) ;
  fprintf(stderr, "name2(2) = %d\n", name2(2)) ;
  fprintf(stderr, "name3(3) = %d\n", name3(3)) ;

  fprintf(stderr, "_name_0_1(1) = %d\n", _name_0_1(1)) ;
  fprintf(stderr, "_name_0_2(2) = %d\n", _name_0_2(2)) ;
  fprintf(stderr, "_name_0_3(3) = %d\n", _name_0_3(3)) ;

  fprintf(stderr, "_name_1_1(1) = %d\n", _name_1_1(1)) ;
  fprintf(stderr, "_name_1_2(2) = %d\n", _name_1_2(2)) ;
  fprintf(stderr, "_name_1_3(3) = %d\n", _name_1_3(3)) ;

  fprintf(stderr, "_name_2_1(1) = %d\n", _name_2_1(1)) ;
  fprintf(stderr, "_name_2_2(2) = %d\n", _name_2_2(2)) ;
  fprintf(stderr, "_name_2_3(3) = %d\n", _name_2_3(3)) ;
}
