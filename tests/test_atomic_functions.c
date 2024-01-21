//
// Copyright (C) 2024  Environnement Canada
//
// This is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation,
// version 2.1 of the License.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details .
//
// Author:
//     M. Valin,   Recherche en Prevision Numerique, 2024
//
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
// C atomic definitions
#include <stdatomic.h>
// RPN atomic functions
#include <rmn/atomic_functions.h>

#if !defined(NTHREADS)
#define NTHREADS 12
#endif
#define NREP 10000

static  _Atomic int acnt;
static  int xcnt, dummy;
static  int32_t bcnt = NREP*NTHREADS ;
static  int32_t pcnt = NREP*NTHREADS ;
static  pthread_t tid[NTHREADS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static  void *arguments = &pcnt;                  // argument list for called thread

int thread_index(pthread_t thread_id){
  int i;
  for(i=0 ; i<NTHREADS ; i++) if(thread_id == tid[i]) return i;
  return -1 ;
}

void *adding(void *in)
{
  int32_t *input = (int32_t *) in;
  int status = -1, bcnt_, i, status2 = -1 ;
  int64_t nwait ;
  pthread_t self = pthread_self();
  int thread_no = thread_index(self);
  int startvalue = bcnt;

  // launch next thread unless enough extra threads (NTHREADS-1) have been launched
  if(thread_no < NTHREADS-1){
//     fprintf(stderr,"thread %2d launching thread %2d\n",thread_no, thread_no+1);
    pthread_create(&tid[thread_no+1],NULL,adding,arguments);
  }else if(thread_no < 0){
    fprintf(stderr,"ERROR: bad thread ordinal = %2d (thread id = %ld)\n", thread_no, self);
    exit(1);
  }

  for(i=0 ; i<(NTHREADS-thread_no)*5000 ; i++)
    atomic_add_and_test_32(&dummy, i);            // spin for some time (earlier threads spin more)
  for(i=0; i<NREP; i++)
  {
    acnt++;                                       // atomic C variable
    xcnt++;                                       // ordinary add to ordinary variable (race condition)
    status = atomic_add_and_test_32(&bcnt, -1);   // atomic functions from rmn/atomic_functions.h
    status2 = atomic_add_and_test_32(input, -1);  // atomic functions from rmn/atomic_functions.h
  }
  bcnt_ = bcnt;
  nwait = spin_until(&bcnt, 0, UNTIL_EQ);
  fprintf(stderr,"thread %2d, bcnt = %6d(%6d), status = %d/%d, wait = %ldK\n", 
          thread_no, bcnt_, startvalue, status, status2, nwait>>10);
  if(thread_no != 0) pthread_exit(NULL);          // thread 0 returns, and must not call pthread_exit
  return NULL;
}

int main(int argc, char **argv)
{
  int i, success;

  tid[0] = pthread_self() ;                 // thread 0 is the main thread
  adding(arguments);                        // thread 0 will get extra threads launched

  for(i=1; i<NTHREADS; i++)                 // wait for extra threads to terminate
    pthread_join(tid[i],NULL);

  printf("the value of acnt is %7d, expecting %d\n", acnt, NREP*NTHREADS);
  printf("the value of bcnt is %7d, expecting %d\n", bcnt,             0);
  printf("the value of pcnt is %7d, expecting %d\n", pcnt,             0);
  printf("the value of xcnt is %7d, expecting %d\n", xcnt, NREP*NTHREADS);
  printf("the value of dummy is %8.8x\n", dummy);

  success = (acnt == NREP*NTHREADS && bcnt == 0 && pcnt == 0);
  fprintf(stderr,"%s\n", success ? "SUCCESS" : "FAILURE") ;
  return (success ? 0 : 1);
}

