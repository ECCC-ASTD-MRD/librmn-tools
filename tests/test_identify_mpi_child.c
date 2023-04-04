
#include <stdio.h>
#include <rmn/identify_mpi_child.h>
#include <rmn/test_helpers.h>

int main(int argc, char **argv){
  start_of_test("identify_mpi_child (C)");
  fprintf(stderr,"MPI RANK = %d\n", identify_mpi_child()) ;
  fprintf(stderr,"MPI WORLD SIZE = %d\n", identify_mpi_world()) ;
}
