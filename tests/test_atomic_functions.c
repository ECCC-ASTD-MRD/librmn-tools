#include <stdio.h>
#include <stdatomic.h>
#include <pthread.h>
#include <rmn/atomic_functions.h>

#if !defined(NTHREADS)
#define NTHREADS 12
#endif
#define NREP 10000

_Atomic int acnt;
int cnt;
int32_t bcnt = NREP*NTHREADS ;

void *adding(void *input)
{
  int status = -1 ;
  for(int i=0; i<NREP; i++)
  {
    acnt++;                                       // atomic C variable
    cnt++;                                        // ordinary add to ordinary variable (race condition)
    status = atomic_add_and_test_32(&bcnt, -1);   // atomic functions from rmn/atomic_functions.h
  }
  fprintf(stderr,"thread id = %lx, bcnt = %d, status = %d\n", pthread_self(), bcnt, status);
  pthread_exit(NULL);
}

int main(int argc, char **argv)
{
  pthread_t tid[NTHREADS];
  for(int i=0; i<NTHREADS; i++)                 // create and start threads
    pthread_create(&tid[i],NULL,adding,NULL);
  for(int i=0; i<NTHREADS; i++)                 // wait for threads to terminate
    pthread_join(tid[i],NULL);

  printf("the value of acnt is %7d, expecting %d\n", acnt, NREP*NTHREADS);
  printf("the value of bcnt is %7d, expecting %d\n", bcnt, 0);
  printf("the value of cnt  is %7d, expecting %d\n" , cnt, NREP*NTHREADS);
  if(acnt == NREP*NTHREADS && bcnt == 0) {
    fprintf(stderr,"SUCCESS\n");
    return 0;
  }else{
    fprintf(stderr,"FAILURE\n");
    return 1;
  }
}

