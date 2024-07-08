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

#include <errno.h>
#define handle_error_en(en, msg) do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

#if !defined(MAX_THREADS)
#define MAX_THREADS 12
#endif
#define NREP 10000

static  _Atomic int acnt;
static  int xcnt, dummy;
static  int32_t bcnt = NREP*MAX_THREADS ;
static  int32_t pcnt = NREP*MAX_THREADS ;
// static  pthread_t tid[MAX_THREADS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
// static  pthread_t tid[MAX_THREADS+1] = {[0 ... MAX_THREADS] = 0xFFFFFFFFFFFFFFFFu};
static  pthread_t tid[MAX_THREADS+1] ;
static  void *arguments = &pcnt;                  // argument list for called thread
static  pthread_attr_t attr;

int pthread_getattr_np(pthread_t thread, pthread_attr_t *attr); // in case it is missing

static void thread_display_attributes(int my_thread_no, char *prefix)
{
  int s, i;
  size_t v;
  void *stkaddr;
  struct sched_param sp;
  pthread_attr_t gattr;
  pthread_attr_t *attr = &gattr;

  printf("thread %d\n", my_thread_no);

  s = pthread_getattr_np(tid[my_thread_no], attr);
  if (s != 0)
    handle_error_en(s, "pthread_getattr_np");

  s = pthread_attr_getdetachstate(attr, &i);
  if (s != 0)
      handle_error_en(s, "pthread_attr_getdetachstate");
  printf("%sDetach state        = %s\n", prefix,
          (i == PTHREAD_CREATE_DETACHED) ? "PTHREAD_CREATE_DETACHED" :
          (i == PTHREAD_CREATE_JOINABLE) ? "PTHREAD_CREATE_JOINABLE" :
          "???");

  s = pthread_attr_getscope(attr, &i);
  if (s != 0)
      handle_error_en(s, "pthread_attr_getscope");
  printf("%sScope               = %s\n", prefix,
          (i == PTHREAD_SCOPE_SYSTEM)  ? "PTHREAD_SCOPE_SYSTEM" :
          (i == PTHREAD_SCOPE_PROCESS) ? "PTHREAD_SCOPE_PROCESS" :
          "???");

  s = pthread_attr_getinheritsched(attr, &i);
  if (s != 0)
      handle_error_en(s, "pthread_attr_getinheritsched");
  printf("%sInherit scheduler   = %s\n", prefix,
          (i == PTHREAD_INHERIT_SCHED)  ? "PTHREAD_INHERIT_SCHED" :
          (i == PTHREAD_EXPLICIT_SCHED) ? "PTHREAD_EXPLICIT_SCHED" :
          "???");

  s = pthread_attr_getschedpolicy(attr, &i);
  if (s != 0)
      handle_error_en(s, "pthread_attr_getschedpolicy");
  printf("%sScheduling policy   = %s\n", prefix,
          (i == SCHED_OTHER) ? "SCHED_OTHER" :
          (i == SCHED_FIFO)  ? "SCHED_FIFO" :
          (i == SCHED_RR)    ? "SCHED_RR" :
          "???");

  s = pthread_attr_getschedparam(attr, &sp);
  if (s != 0)
      handle_error_en(s, "pthread_attr_getschedparam");
  printf("%sScheduling priority = %d\n", prefix, sp.sched_priority);

  s = pthread_attr_getguardsize(attr, &v);
  if (s != 0)
      handle_error_en(s, "pthread_attr_getguardsize");
  printf("%sGuard size          = %zu bytes\n", prefix, v);

  s = pthread_attr_getstack(attr, &stkaddr, &v);
  if (s != 0)
      handle_error_en(s, "pthread_attr_getstack");
  printf("%sStack address       = %p\n", prefix, stkaddr);
  printf("%sStack size          = %ld bytes\n", prefix, v);
//   printf("%sStack size          = 0x%zx bytes\n", prefix, v);
}

// next free slot in thread table
int thread_get_free_slot(){
  int i;
  for(i=0 ; i<MAX_THREADS ; i++){
    if( atomic_compare_and_swap_64(&tid[i], 0xFFFFFFFFFFFFFFFFu, 0) ) return i ;
  }
  return -1;
}

int thread_my_slot(){
  int i;
  pthread_t thread_id = pthread_self() ;
  for(i=0 ; i<MAX_THREADS ; i++) if(thread_id == tid[i]) return i;
  return -1 ;
}

int thread_get_slot(pthread_t thread_id){
  int i;
  for(i=0 ; i<MAX_THREADS ; i++) if(thread_id == tid[i]) return i;
  return -1 ;
}

void *adding(void *in)
{
  int32_t *input = (int32_t *) in;
  int status = -1, bcnt_, i, status2 = -1 ;
  int64_t nwait ;
  int thread_no = thread_my_slot();
  pthread_t self = tid[thread_no] ;
  pthread_t self1 = pthread_self();    // for consistency check
  int startvalue = bcnt;
  int next_thread = thread_get_free_slot() ;
  size_t mystacksize;
  pthread_attr_t gattr;

  if(self != self1) exit(1) ;

//   fprintf(stderr,"thread %2d, next free slot = %2d\n", thread_no, next_thread);
  thread_display_attributes(thread_no, "\t");

  // launch next thread unless enough extra threads (MAX_THREADS-1) have been launched
  if(next_thread != -1) {
//     fprintf(stderr,"thread %2d launching thread %2d\n",thread_no, thread_no+1);
    pthread_create(&tid[next_thread], &attr, adding, arguments);
//     pthread_create(&tid[next_thread], NULL, adding, arguments);
//     pthread_create(&tid[thread_no+1],NULL,adding,arguments);
  }else if(thread_no < 0){
    fprintf(stderr,"ERROR: bad thread ordinal = %2d (thread id = %ld %ld)\n", thread_no, self, self1);
    exit(1);
  }

  for(i=0 ; i<(MAX_THREADS-thread_no)*5000 ; i++)
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
//   size_t stacksize = 2048l << 20;   // 2GBytes
  size_t stacksize = 2000l * 1000000;   // 2GBytes
//   size_t stacksize = 2 << 10;   // 2KBytes

  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, stacksize) ;

//   fprintf(stderr,"size of pthread_t = %ld\n", sizeof(pthread_t));
  for(i=0 ; i<MAX_THREADS+1 ; i++) tid[i] = 0xFFFFFFFFFFFFFFFFu ;
  tid[0] = pthread_self() ;                 // thread 0 is the main thread
  adding(arguments);                        // thread 0 will get extra threads launched

  for(i=1; i<MAX_THREADS; i++)                 // wait for extra threads to terminate
    pthread_join(tid[i],NULL);

  printf("the value of acnt is %7d, expecting %d\n", acnt, NREP*MAX_THREADS);
  printf("the value of bcnt is %7d, expecting %d\n", bcnt,             0);
  printf("the value of pcnt is %7d, expecting %d\n", pcnt,             0);
  printf("the value of xcnt is %7d, expecting %d\n", xcnt, NREP*MAX_THREADS);
  printf("the value of dummy is %8.8x\n", dummy);

  success = (acnt == NREP*MAX_THREADS && bcnt == 0 && pcnt == 0);
  fprintf(stderr,"%s\n", success ? "SUCCESS" : "FAILURE") ;
  return (success ? 0 : 1);
}

