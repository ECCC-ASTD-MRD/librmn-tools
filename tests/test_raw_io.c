#include <stdio.h>
#include <stdlib.h>

void *read_32bit_data_record(char *filename, int *fdi, int *dims, int *ndim);

int main(int argc, char **argv){
  int dims[10] ;
  int ndim = 0 ;
  int fd = 0 ;
  int i, j ;
  void *buf ;

  for(i=1 ; i<argc ; i++){
    fd = 0 ;
    ndim = 0 ;
    buf = read_32bit_data_record(argv[i], &fd, dims, &ndim) ;
    fprintf(stderr, "file = '%s', fd = %d\n", argv[i], fd) ;
    while(buf != NULL){
      fprintf(stderr, "number of dimensions = %d : (", ndim) ;
      for(j=0 ; j<ndim ; j++) fprintf(stderr, "%d ", dims[j]) ; fprintf(stderr, ")\n");
      free(buf) ;
      fprintf(stderr, "reading next record, fd = %d\n", fd);
      buf = read_32bit_data_record(NULL, &fd, dims, &ndim) ;
    }
    fprintf(stderr, "end of file = '%s'\n", argv[i]) ;
  }
}
