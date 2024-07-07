#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <rmn/c_record_io.h>

int main(int argc, char **argv){
  int dims[10] ;
  int dim3[] = { 1,2,3 } ;
  int buf3[6] ;
  int ndim = 0 ;
  int fd = 0 ;
  int i, j ;
  void *buf ;
  int ndata ;

  for(i=1 ; i<argc ; i++){
    fd = 0 ;
    ndim = 0 ;
    buf = read_32bit_data_record(argv[i], &fd, dims, &ndim, &ndata) ;
    fprintf(stderr, "file = '%s', fd = %d\n", argv[i], fd) ;
    while(buf != NULL){
      fprintf(stderr, "number of dimensions = %d : (", ndim) ;
      for(j=0 ; j<ndim ; j++) { fprintf(stderr, "%d ", dims[j]) ; } ; fprintf(stderr, ") [%d]\n", ndata);
      free(buf) ;
      fprintf(stderr, "reading next record, fd = %d\n", fd);
      buf = read_32bit_data_record("", &fd, dims, &ndim, &ndata) ;
    }
    if(ndata == 0) {
      fprintf(stderr, "end of file = '%s', close status = %d\n", argv[i], close(fd)) ;
    }else{
      fprintf(stderr, "error reading record\n") ;
    }
  }
  fd = open("./temporary_file", O_WRONLY | O_CREAT, 0777) ;
  if(fd <0) exit(1) ;
  ndata = write_32bit_data_record("", &fd, dim3, 3, buf3) ;
  close(fd) ;
}
