
#include <stdlib.h>
#include <rmn/identify_mpi_child.h>

// get MPI process rank from known environment variables
int identify_mpi_child(){
  char *envar ;

  if( (envar = getenv("OMPI_COMM_WORLD_RANK")) ) return atoi(envar) ;   // OpenMPI
  if( (envar = getenv("PMI_RANK")) ) return atoi(envar) ;               // Mpich
  if( (envar = getenv("ALPS_APP_PE")) ) return atoi(envar) ;            // Cray ALPS
  if( (envar = getenv("PMIX_RANK")) ) return atoi(envar) ;              // OpenMPI
  if( (envar = getenv("MP_CHILD")) ) return atoi(envar) ;               // IBM POE (and maybe r.run_in_parallel)
  return -1 ;
}

// get MPI WORLD size from known environment variables
int identify_mpi_world(){
  char *envar ;

  if( (envar = getenv("OMPI_COMM_WORLD_SIZE")) ) return atoi(envar) ;   // OpenMPI
  if( (envar = getenv("MPI_LOCALNRANKS")) ) return atoi(envar) ;        // Mpich
  return -1 ;
}

// OMPI_COMM_WORLD_SIZE=2
// MPI_LOCALNRANKS=4
