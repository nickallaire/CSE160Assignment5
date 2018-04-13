/*
	Name: Nicholas Allaire
	Email: nallaire@ucsd.edu
	PID: A10639753
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <assert.h>
#include <string.h>
#include "cs160mp.h"

double cpu_time ( void );

void myAbort(char * msg) {
	fprintf(stderr,"%s\n",msg);
	exit(-1);
}

void usage() {
	char *usage = "usage: msgbench <size> <trials> [validate]" ;
	myAbort(usage);
}

typedef struct {
	void *sbuf;
	int len;
	int trials;
	int validate;
} BENCHARG;
	
int doValidate(char * sbuf, char *rbuf, int len) {
	int i;
	
	//printf("validate sbuf: %x rbuf: %x\n", (unsigned long) sbuf,
	//	(unsigned long) rbuf);
	for (i = 0; i < len; i++) {
		assert(sbuf[i] == rbuf[i]);
	}
	return 0;
}

void *bench(void * arg) {
	#define SEND 0
	#define ACK (SEND + 1)
	#define SENDTAG 1000
	#define ACKTAG  3000 
	
	/* read the arguments passed through arg */
	BENCHARG * targ = (BENCHARG *) arg;
	void *sbuf = targ -> sbuf;
	int trials = targ -> trials;
	int len = targ -> len;
	int validate = targ -> validate;
	void *rbuf = calloc(len,sizeof(char));
	int trial;
	int size = 2;
	MSGRQST  requests[size];
	int i;
	int id = MP_Rank(pthread_self());
	for (trial = 0; trial < trials; trial++)
 	{
		for (i = 0; i < size; i++)
			requests[i] = MSGRQST_NULL;	
		if (id == 0)
		{
			iSend(sbuf,len,id,1,SENDTAG+trial,requests+SEND);
		}
		else
		{
			iRecv(rbuf,len,0,id,SENDTAG+trial,requests+SEND);
		}
		msgWait(requests+SEND);

		/* Now need to send back what we received */
		if ( id == 0)
		{
			iRecv(rbuf,len,1,id,ACKTAG+trial,requests+ACK);
		}
		else
		{
			iSend(rbuf,len,id, 0,ACKTAG+trial,requests+ACK);
		}
		msgWait(requests+ACK);

		//msgWaitAll(requests, size);

		if (id == 0 && validate)
		{
			doValidate(sbuf,rbuf,len);
		}
		if (validate)
			bzero(rbuf,len);
	}
	
	free(rbuf);

	return 0;
}

#define NTHREAD 2
int main(int argc, char *argv[]) {
	int		msgSize;
	int 		trials;
	//int		numprocs;
	//int		myid;
	pthread_t	threads[NTHREAD];

	if (argc < 3 || argc > 4) 
		usage();

	msgSize = atoi(argv[1]);
	int nInts = (msgSize + sizeof(int))/sizeof(int);
	trials = atoi(argv[2]);
	int *sendBuf = calloc(nInts, sizeof(int));

	/* initialize buffers with random stuff */
	int i;
	printf("starting initialize\n");
	for (i = 0; i < nInts; i++) {
		sendBuf[i] = (int) mrand48();
	}
	printf("\n");

	BENCHARG arg;
	arg.sbuf = (void *) sendBuf;
	arg.len = msgSize;
	arg.trials = trials;
	arg.validate = (argc == 4);

	printf("starting bench\n");
	MP_Init(NTHREAD);
	double sTime,eTime,avgTime;
	sTime = cpu_time();
	
	/* create the benchmark threads. wait for both to complete */
	for (i=0; i < NTHREAD; i++) {
		pthread_create(threads+i,NULL,bench, (void *) &arg);
	}

	for (i=0; i < NTHREAD; i++) {
		pthread_join(threads[i],NULL);
	}

	eTime = cpu_time();
	int bytes = msgSize; 
	avgTime = (eTime-sTime)/(double) trials;
	double bw = 2.0*((double) bytes)/avgTime;
	printf("%d bytes (%d trials) %f secs/trial  %g Bytes/s\n", 
			bytes,trials, avgTime, bw); 
	free(sendBuf);
	MP_Finalize();
	return 0;
}

/******************************************************************************/
double cpu_time ( void )
/******************************************************************************/
/*
  Purpose:
    CPU_TIME returns the current reading on the CPU clock.
  Licensing:
    This code is distributed under the GNU LGPL license. 
  Modified:
    06 June 2005
  Author:
    John Burkardt
  Parameters:
    Output, double CPU_TIME, the current reading of the CPU clock, in seconds.
*/
{
  double value;
  value = ( double ) clock ( ) / ( double ) CLOCKS_PER_SEC;
  return value;
}
