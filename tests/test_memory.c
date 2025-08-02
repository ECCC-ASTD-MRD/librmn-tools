/*-----------------------------------------------------------------------*/
/* Original program Program: Stream                                      */
/* Revision: $Id: stream.c,v 5.9 2009/04/11 16:35:00 mccalpin Exp $      */
/* Original code developed by John D. McCalpin                           */
/* Programmers: John D. McCalpin                                         */
/*              Joe R. Zagar                                             */
/*                                                                       */
/* This program measures memory transfer rates in MB/s for simple        */
/* computational kernels coded in C.                                     */
/*-----------------------------------------------------------------------*/
/* Copyright 1991-2005: John D. McCalpin                                 */
/*-----------------------------------------------------------------------*/
/* License:                                                              */
/*  1. You are free to use this program and/or to redistribute           */
/*     this program.                                                     */
/*  2. You are free to modify this program for your own use,             */
/*     including commercial use, subject to the publication              */
/*     restrictions in item 3.                                           */
/*  3. You are free to publish results obtained from running this        */
/*     program, or from works that you derive from this program,         */
/*     with the following limitations:                                   */
/*     3a. In order to be referred to as "STREAM benchmark results",     */
/*         published results must be in conformance to the STREAM        */
/*         Run Rules, (briefly reviewed below) published at              */
/*         http://www.cs.virginia.edu/stream/ref.html                    */
/*         and incorporated herein by reference.                         */
/*         As the copyright holder, John McCalpin retains the            */
/*         right to determine conformity with the Run Rules.             */
/*     3b. Results based on modified source code or on runs not in       */
/*         accordance with the STREAM Run Rules must be clearly          */
/*         labelled whenever they are published.  Examples of            */
/*         proper labelling include:                                     */
/*         "tuned STREAM benchmark results"                              */
/*         "based on a variant of the STREAM benchmark code"             */
/*         Other comparable, clear and reasonable labelling is           */
/*         acceptable.                                                   */
/*     3c. Submission of results to the STREAM benchmark web site        */
/*         is encouraged, but not required.                              */
/*  4. Use of this program or creation of derived works based on this    */
/*     program constitutes acceptance of these licensing restrictions.   */
/*  5. Absolutely no warranty is expressed or implied.                   */
/*-----------------------------------------------------------------------*/

/*-----------------------------------------------------------------------*/
/* The original code has been extensively modified to allow for          */
/* multiple copies running in parallel in the same NUMA space            */
/* the preamble gives time to start the multiple copies in background    */
/* the main body (NTIMES iterations) is expected to run concurrently     */
/* the postamble part makes sure that any potentially faster copy will   */
/* still be running when the last copy terminates its NTIMES iterations  */
/* M.Valin 2025 Recherche en Prevision Numerique                         */
/*-----------------------------------------------------------------------*/

# include <stdio.h>
# include <stdlib.h>
# include <math.h>
# include <float.h>
# include <limits.h>
# include <string.h>
# include <time.h>
# include <sys/time.h>

/* N.B.:
 *
 * This program requires a good bit of memory to run.  Adjust the
 * value of 'N' (below) to give a 'timing calibration' of
 * at least 20 clock-ticks.  This will provide rate estimates
 * that should be good to about 5% precision.
 */

// default array size ~800 MBytes
#ifndef N
#   define N 100000000
#endif
// default number of iterations : 20
#ifndef NTIMES
#   define NTIMES 20
#endif
#ifndef OFFSET
#   define OFFSET 0
#endif

/*
 * Compile the code with full optimization.  Many compilers
 * generate unreasonably bad code before the optimizer tightens
 * things up.  If the results are unreasonably good, on the
 * other hand, the optimizer might be too smart for our good
 *
 * Try compiling with:
 * gcc -O2 -DN=100000000 -DNTIMES=20 -march=native -fopenmp stream.c -o stream.exe
 * 800 MByte arrays, 20 iterations
 * make sure arrays are large enough to flood all cache levels
 *
 * calling sequence
 * OMP_NUM_THREADS=xxx stream.exe test_number
 * test_number :
 *    0   memory copy test  array1 = array2
 *    1   scaling test      array1 = array2 * scalar
 *    2   add test          array1 = array2 + array3
 *    3 triad test          array1 = array2 + array3 * scalar
 * if test number is a ultui digit decimal number, multiple tests will be executed
 * e.g. 3210 will execute tests 0, then 1, then 2, then 3
 * if test_number is omitted, test 0 (copy) will be executed
 *
*/

# define HLINE "-------------------------------------------------------------\n"

# ifndef MIN
# define MIN(x,y) ((x)<(y)?(x):(y))
# endif
# ifndef MAX
# define MAX(x,y) ((x)>(y)?(x):(y))
# endif

static double	*a, *b, *c;

static double avgtime[4] = {0, 0, 0, 0},
              maxtime[4] = {0, 0, 0, 0},
              mintime[4] = {FLT_MAX,FLT_MAX,FLT_MAX,FLT_MAX};
static char	*label[4] = {"Copy:      ",
                         "Scale:     ",
                         "Add:       ",
                         "Triad:     "};
static double	bytes[4] = {
    2 * sizeof(double) * N,
    2 * sizeof(double) * N,
    3 * sizeof(double) * N,
    3 * sizeof(double) * N
    };

extern double mysecond();
extern void print_time(char *str);
extern void tuned_STREAM_Copy();
extern void tuned_STREAM_Scale(double scalar);
extern void tuned_STREAM_Add();
extern void tuned_STREAM_Triad(double scalar);

#ifdef _OPENMP
extern int omp_get_num_threads();
#endif

int test(int j0)
    {
    int quantum, checktick();
    int BytesPerWord = sizeof(double);
    int j, k;
    double scalar, t, times[4][NTIMES];

    j0 &= 0x3;
    a = malloc((N+OFFSET)*sizeof(double)) ;
    if(a == NULL) fprintf(stderr, "allocation of a failed\n") ;
    b = malloc((N+OFFSET)*sizeof(double)) ;
    if(b == NULL) fprintf(stderr, "allocation of b failed\n") ;
    c = malloc((N+OFFSET)*sizeof(double)) ;
    if(c == NULL) fprintf(stderr, "allocation of c failed\n") ;

    /* --- SETUP --- determine precision and check timing --- */
#if 0
    printf(HLINE);
    BytesPerWord = sizeof(double);
    printf("This system uses %d bytes per DOUBLE PRECISION word.\n", BytesPerWord);
    printf(HLINE);
    printf("Each test is run %d times, but only\n", NTIMES);
    printf("the *best* time for each is used.\n");
#endif
    printf("\n-Total memory required = %.1f MB. (NTIMES = %d) (test %d)",
           (3.0 * BytesPerWord) * ( (double) N / 1048576.0), NTIMES, j0);
#ifdef _OPENMP
//     printf(HLINE);
#pragma omp parallel 
{
#pragma omp master
  {
    k = omp_get_num_threads();
    printf (", Threads requested = %i\n",k);
  }
}
#endif

/* Get initial value for system clock. */
#pragma omp parallel for
  for (j=0; j<N; j++) {
    a[j] = 1.0;
    b[j] = 2.0;
    c[j] = 4.0;
  }

  if  ( (quantum = checktick()) < 1){
    printf("Your clock granularity appears to be %d (less than one microsecond).\n", quantum);
    quantum = 1;
  }

  t = mysecond();
#pragma omp parallel for
  for (j = 0; j < N/4; j++){ a[j] = 2.0E0 * a[j]; }
  t = 4.0E6 * (mysecond() - t);
  printf(">The test will need on the order of %d microseconds.\n", (int) t*NTIMES);

  if(t/quantum < 20){
    printf(HLINE);
    printf("The test iterations need about %d clock ticks\n", (int) (t/quantum) );
    printf("You are not getting at least 20 clock ticks per test.\n");
    printf("You should increase the size of the arrays.\n");
    printf(HLINE);
    printf("WARNING -- The above is only a rough guideline.\n");
    printf("For best results, please be sure you know the\n");
    printf("precision of your system timer.\n");
    printf(HLINE);
  }

  scalar = 3.0;
  /* 5 test iterations as a preamble ot prime the pump */
  print_time(">pre = ") ;
  if(j0 == 0) {
    for (j=0; j<5; j++) tuned_STREAM_Copy() ;
  }
  if(j0 == 1) {
    for (j=0; j<5; j++) tuned_STREAM_Scale(scalar) ;
  }
  if(j0 == 2) {
    for (j=0; j<5; j++) tuned_STREAM_Add() ;
  }
  if(j0 == 3){
    for (j=0; j<5; j++) tuned_STREAM_Triad(scalar) ;
  }

  /*	--- MAIN LOOP --- repeat test case NTIMES times --- */
  print_time(", start = ") ;
  for (k=0; k<NTIMES; k++)
  {
    if(j0 == 0){
      times[j0][k] = mysecond();
#pragma omp parallel for
      for (j=0; j<N; j++){
        c[j] = a[j];
      }
      times[j0][k] = mysecond() - times[j0][k];
    }
    if(j0 == 1){
      times[j0][k] = mysecond();
#pragma omp parallel for
      for (j=0; j<N; j++){
        b[j] = scalar*c[j];
      }
      times[j0][k] = mysecond() - times[j0][k];
    }
    if(j0 == 2){
      times[j0][k] = mysecond();
#pragma omp parallel for
      for (j=0; j<N; j++){
        c[j] = a[j]+b[j];
      }
      times[j0][k] = mysecond() - times[j0][k];
    }
    if(j0 == 3){
      times[j0][k] = mysecond();
#pragma omp parallel for
      for (j=0; j<N; j++){
        a[j] = b[j]+scalar*c[j];
      }
      times[j0][k] = mysecond() - times[j0][k];
    }
  }
  /* NTIMES/2 test iterations as a postamble */
  print_time(", post = ") ;
  if(j0 == 0) {
    for (j=0; j<NTIMES/2; j++) tuned_STREAM_Copy() ;
  }
  if(j0 == 1) {
    for (j=0; j<NTIMES/2; j++) tuned_STREAM_Scale(scalar) ;
  }
  if(j0 == 2) {
    for (j=0; j<NTIMES/2; j++) tuned_STREAM_Add() ;
  }
  if(j0 == 3){
    for (j=0; j<NTIMES/2; j++) tuned_STREAM_Triad(scalar) ;
  }
  print_time(", end = ") ;
  printf("\n");

  /*	--- SUMMARY --- */
  for (k=1; k<NTIMES; k++) /* note -- skip first iteration */
  {
    avgtime[j0] =     avgtime[j0] + times[j0][k];
    mintime[j0] = MIN(mintime[j0],  times[j0][k]);
    maxtime[j0] = MAX(maxtime[j0],  times[j0][k]);
  }
  printf("Function      Rate (MB/s)   Avg time     Min time     Max time\n");
  avgtime[j0] = avgtime[j0]/(double)(NTIMES-1);

  printf("%s%11.4f  %11.4f  %11.4f  %11.4f\n", label[j0],
        1.0E-06 * bytes[j0]/mintime[j0],
        avgtime[j0],
        mintime[j0],
        maxtime[j0]);

  free(a) ;
  free(b) ;
  free(c) ;
  return 0;
}

# define	M	20

int
checktick()
{
  int i, minDelta, Delta;
  double t1, t2, timesfound[M];

/*  Collect a sequence of M unique time values from the system. */
  for (i = 0; i < M; i++) {
    t1 = mysecond();
    while( ((t2=mysecond()) - t1) < 1.0E-6 );
    timesfound[i] = t1 = t2;
  }

/*
 * Determine the minimum difference between these M values.
 * This result will be our estimate (in microseconds) for the
 * clock granularity.
 */

  minDelta = 1000000;
  for (i = 1; i < M; i++) {
    Delta = (int)( 1.0E6 * (timesfound[i]-timesfound[i-1]));
    minDelta = MIN(minDelta, MAX(Delta,0));
  }

  return(minDelta);
}


/* A gettimeofday routine to give access to the wall
   clock timer on most UNIX-like systems.  */

#include <sys/time.h>

double mysecond()
{
  struct timeval tp;
  struct timezone tzp;
  int i;

  i = gettimeofday(&tp,&tzp);
  return ( (double) tp.tv_sec + (double) tp.tv_usec * 1.e-6 );
}

void tuned_STREAM_Copy()
{
#if defined(TUNED)
  memcpy(c,a,N*4);
  return;
#else
  int j;
#pragma omp parallel for
  for (j=0; j<N; j++)
    c[j] = a[j];
#endif
}

void tuned_STREAM_Scale(double scalar)
{
  int j;
#pragma omp parallel for
  for (j=0; j<N; j++)
    b[j] = scalar*c[j];
}

void tuned_STREAM_Add()
{
  int j;
#pragma omp parallel for
  for (j=0; j<N; j++)
    c[j] = a[j]+b[j];
}

void tuned_STREAM_Triad(double scalar)
{
  int j;
#pragma omp parallel for
  for (j=0; j<N; j++)
    a[j] = b[j]+scalar*c[j];
}

void print_time(char *str){
  time_t mytime = time(NULL);
  char *time_str = ctime(&mytime);
  time_str[strlen(time_str)-6] = '\0';
  while(*time_str != ':') time_str++ ;
  time_str -= 2 ;
  printf("%s %s", str, time_str);
}

int main(int argc, char **argv){
  int j0 = 0 ;

  if(argc > 1) j0 = atoi(argv[1]) ;
again:
  test(j0 % 10) ;
  j0 = j0/10 ;
  if(j0 > 0) goto again ;
}
