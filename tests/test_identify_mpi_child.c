
#include <stdio.h>
#include <rmn/identify_mpi_child.h>

int main(int argc, char **argv){
  fprintf(stderr,"MPI RANK = %d\n", identify_mpi_child()) ;
}
