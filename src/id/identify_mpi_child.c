
#include <stdlib.h>
#include <rmn/identify_mpi_child.h>

// get MPI process rank from known environment variables
int identify_mpi_child(){
  char *envar ;

  if( envar = getenv("OMPI_COMM_WORLD_RANK") ) return atoi(envar) ;   // OpenMPI
  if( envar = getenv("PMI_RANK") ) return atoi(envar) ;               // Mpich
  if( envar = getenv("ALPS_APP_PE") ) return atoi(envar) ;            // Cray ALPS
  if( envar = getenv("PMIX_RANK") ) return atoi(envar) ;              // OpenMPI
  if( envar = getenv("MP_CHILD") ) return atoi(envar) ;               // IBM POE (and maybe r.run_in_parallel)
  return -1 ;
}
