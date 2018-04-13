/*
	Name: Nicholas Allaire
	Email: nallaire@ucsd.edu
	PID: A10639753
*/

/* Definitions for CS160 Message Passing */
#include <pthread.h>
#ifndef __CS160MP
#define __CS160MP
#define ANY_TAG -1
#define ANY_SRC -1
#define MPTRUE 1
#define MPFALSE 0

/* you will need to change the definitions of MSGRQST and MSGRQST_NULL */
typedef struct {
	void *buf;
	int len;
	int src;
	int dest;
	int tag;
	int type; 	// 0 = send request, 1 = recv request
	int completed;	// 0 = uncomplete send, 1 = completed send
	long pair;
} MSGRQST;

typedef struct {
	MSGRQST *r;
} MSGARR;

typedef struct {
	pthread_t thread;
	int rank;
} PROC_TABLE;

static MSGRQST MSGRQST_NULL = {(void *) -1, -1, -1, -1, -1, -1, -1, -1};

MSGARR *mSend;
MSGARR *mRecv;

PROC_TABLE *ptable;

int threadCount;

int msgSendCount;
int msgRecvCount;
int maxSendCount;
int maxRecvCount;

pthread_mutex_t *mutex;
pthread_cond_t *condVar;

/* you MUST NOT change the definitions of API below */
int MP_Init(int nthreads);
int MP_Size();
int MP_Rank(pthread_t thread);
int MP_Finalize();
int iSend(void *buf, int len, int src, int dest, int tag, MSGRQST * request);
int iRecv(void *buf, int len, int src, int dest, int tag, MSGRQST * request);
int msgWait( MSGRQST * request);
int msgWaitAll( MSGRQST *requests, int nqrsts);
int getRank(MSGRQST *request);
int getTag(MSGRQST *request);
int getLen(MSGRQST *request);

#endif
