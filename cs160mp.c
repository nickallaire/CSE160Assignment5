/*
        Name: Nicholas Allaire
        Email: nallaire@ucsd.edu
	PID: A10639753
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include "cs160mp.h"

/** Initialize any internal data structures needed to implement message passing.
    Called once per program 

    Input: number of threads
    Return: number of threads requested
**/
int MP_Init(int nthreads) {
	maxSendCount = 100 * nthreads * nthreads;
	maxRecvCount = 100 * nthreads * nthreads;
	mSend =  malloc(maxSendCount * sizeof(MSGARR));
	mRecv =  malloc(maxRecvCount * sizeof(MSGARR));
	ptable = malloc(nthreads * sizeof(PROC_TABLE));
	msgSendCount = 0;
	msgRecvCount = 0;
	threadCount = nthreads;
	mutex = malloc(1 * sizeof(pthread_mutex_t));
	condVar = malloc(1 * sizeof(pthread_cond_t));
	pthread_mutex_init(mutex, NULL);
	pthread_cond_init(condVar, NULL);

	int i;
	for (i = 0; i < nthreads; i++) {
		ptable[i].thread = -1;
		ptable[i].rank = i;
	}

	for (i = 0; i < maxSendCount; i++) {
		mSend[i].r = &MSGRQST_NULL;
		mRecv[i].r = &MSGRQST_NULL;
	}

	return nthreads;
}

/** Clean up any allocated memory. Indicates no more message passing will be performed.
    Called after all message passing is complete. Called once per program 

    Return: 1 if successful freeing of memory
**/
int MP_Finalize() {
	pthread_mutex_destroy(mutex);
	pthread_cond_destroy(condVar);
	free(mutex);
	free(condVar);
	free(ptable);
	free(mSend);
	free(mRecv);
        return 1;
}

/** Return the number of threads passed to MPI_Init() 

    Return: number of threads passed to MP_Init()
**/
int MP_Size() {
	return threadCount;
}

/** Give a rank (0...MP_Size()) to the thread with a particular id 

    Input: pthread_t
    Return: rank of pthread_t in ptable
**/
int MP_Rank(pthread_t thread) {
	int i;
	for (i = 0; i < threadCount; i++) {
		if (ptable[i].thread == thread) 
			return ptable[i].rank;
	}

	for (i = 0; i < threadCount; i++) {
		if (ptable[i].thread == -1) {
			ptable[i].thread = thread;
			return ptable[i].rank;
		}
	}
	
	// assert error here because all spots in ptable are full
	assert(1 != 1);
	return 0;
}

/** Send a memory buffer (no type) of particular length from src to dest using
    a specific tag. Asynchronous. 

    Input: data to fill out a send request
    Return: 1 if send request is filled out
**/
int iSend(void *buf, int len, int src, int dest, int tag, MSGRQST * request) {
	int size = MP_Size();
	assert(tag >= 0);
	assert(src >= 0);
	assert(src < size);
	assert(dest >= 0);
	assert(dest < size);
	assert(buf);	

	request -> buf = buf;
	request -> len = len;
	request -> src = src;
	request -> dest = dest;
	request -> tag = tag;
	request -> type = 0;
	request -> completed = 0;
	request -> pair = -1;

	/* Reallocate array if reached maxSendCount */
	if (msgSendCount == (maxSendCount - 1)) {
		maxSendCount *= maxSendCount;
		MSGARR *temp  = (MSGARR *) realloc(mSend, maxSendCount * sizeof(MSGARR));
		if (temp == NULL) {
			printf(" CANNOT ALLOCATE MORE MEMORY\n");
		} else {
			mSend = temp;
		}
	}

	/* Find an empty spot in mSend */
	int i;
	for (i = 0; i < maxSendCount; i++) {
        	if (mSend[i].r -> len == -1) {
                        mSend[i].r = &*(request);
                        msgSendCount++;
                        break;
                }
        }
	return 1;
}

/** Receive into a memory buffer of size len from src to dest (src may be wildcard)
    using tag (tag may be wildcard). Asynchronous. 

    Input: data to fill out a receive request
    Return: 1 if receive request is filled out
**/
int iRecv(void *buf, int len, int src, int dest, int tag, MSGRQST * request) {

	int size = MP_Size();
        assert(tag >= 0);
        assert(src >= 0);
        assert(src < size);
        assert(dest >= 0);
        assert(dest < size);
	assert(buf);

	request -> buf = buf;
	request -> len = len;
	request -> src = src;
	request -> dest = dest;
	request -> tag = tag;
	request -> type = 1;
	request -> completed = 0;
	request -> pair = -1;
	
	/* Reallocate array if reached maxRecvCount */
	if (msgRecvCount == (maxRecvCount - 1)) {
		maxRecvCount *= maxRecvCount;
		MSGARR *temp = (MSGARR *) realloc(mRecv, maxRecvCount * sizeof(MSGARR));
		if (temp == NULL) {
			printf("CANNOT ALLOCATE MOE MEMORY\n");
		} else {
			mRecv = temp;
		}
	}
	
	/* Find an empty spot in mRecv */
	int i;
	for (i = 0; i < maxRecvCount; i++) {
		if (mRecv[i].r -> len == -1) {
			mRecv[i].r = &*(request);
			msgRecvCount++;
			break;
		}
	}

	return 1;
}

/** Wait for a specific message request to complete 

    Input: request pointer
    Return: 1 when request has been completed
**/
int msgWait(MSGRQST * request) {

	if (request == &MSGRQST_NULL) {
		return 1;
	}

	int type = request -> type;
	int i, j;
	int pos;
	
	// request is a send if type == 0
	if (type == 0) {
		// Find position of request in mSend
		for (i = 0; i < maxSendCount; i++) {
			if (mSend[i].r == &*(request)) {
				pos = i;
				break;
			}
		}

		// Loop until request has a pair
		while (mSend[pos].r -> completed != 1) {
			pthread_mutex_lock(mutex);
			if (mSend[pos].r -> pair != -1 && mSend[pos].r -> completed == 0) {
				mSend[pos].r -> completed = 1;
			}
		
			struct timespec ts;
                	struct timeval tp;
                	gettimeofday(&tp, NULL);
                	ts.tv_sec = tp.tv_sec;
	                ts.tv_nsec = tp.tv_usec * 1000;
        	        ts.tv_sec += (double) 2 / 1000;
                	pthread_cond_timedwait(condVar, mutex, &ts);
			pthread_mutex_unlock(mutex);
		}

		// Free up position in mSend once a request is completed
		pthread_mutex_lock(mutex);
		mSend[pos].r = &MSGRQST_NULL;
		msgSendCount--;
		pthread_mutex_unlock(mutex);

	} else {
		// Find position of request in mRecv
                for (j = 0; j < maxRecvCount; j++) {
                        if (mRecv[j].r == &*(request)) {
                                pos = j;
                                break;
                        }
                }

		// Loop until request has copmleted
      		while (mRecv[pos].r -> completed != 1) {
			pthread_mutex_lock(mutex);
			if (mRecv[pos].r -> pair != -1 && mRecv[pos].r -> completed == 0) {
				mRecv[pos].r -> completed = 1;
			} else {
				// Loop through all requests in mSend
				for (i = 0; i < maxSendCount; i++) {
					if (mSend[i].r -> len != -1) {
						if (mRecv[pos].r -> src == ANY_SRC) {
							// Any src and Any tag for request
							if (mRecv[pos].r -> tag == ANY_TAG) {
								// Check if a match
								if (mRecv[pos].r -> completed == 0 && mSend[i].r -> completed == 0) {
									memcpy(mRecv[pos].r -> buf, mSend[i].r -> buf, mRecv[pos].r -> len);
									mSend[i].r -> completed = 1;
									mSend[i].r -> pair = (long) mRecv[pos].r;
									mRecv[pos].r -> completed = 1;
									mRecv[pos].r -> pair = (long) mSend[i].r;
									pthread_cond_broadcast(condVar);
								}
							// Any src and specific tag for request
							} else {
								// Check if a match
								if (mRecv[pos].r -> tag == mSend[i].r -> tag && mRecv[pos].r -> completed == 0 && mSend[i].r -> completed == 0) {
									memcpy(mRecv[pos].r -> buf, mSend[i].r -> buf, mRecv[pos].r -> len);
                                                                        mSend[i].r -> completed = 1;
                                                                        mSend[i].r -> pair = (long ) mRecv[pos].r;
                                                                        mRecv[pos].r -> completed = 1;
                                                                        mRecv[pos].r -> pair = (long) mSend[i].r;
                                                                        pthread_cond_broadcast(condVar);
								}
							}
						// specific src and any tag for request
						} else if (mRecv[pos].r -> tag == ANY_TAG) {
							// Check if a match
							if (mRecv[pos].r -> src == mSend[i].r -> src && mRecv[pos].r -> completed == 0 && mSend[i].r -> completed == 0) {
								memcpy(mRecv[pos].r -> buf, mSend[i].r -> buf, mRecv[pos].r -> len);
                                                                mSend[i].r -> completed = 1;
                                                                mSend[i].r -> pair = (long) mRecv[pos].r;
                                                                mRecv[pos].r -> completed = 1;
                                                                mRecv[pos].r -> pair = (long) mSend[i].r;
                                                                pthread_cond_broadcast(condVar);
							}
						// specific src and specific tag for request
						} else {
							// Check if a match
							if (mRecv[pos].r -> src == mSend[i].r -> src && mRecv[pos].r -> tag == mSend[i].r -> tag && mRecv[pos].r -> completed == 0 && mSend[i].r -> completed == 0) {
								memcpy(mRecv[pos].r -> buf, mSend[i].r -> buf, mRecv[pos].r -> len);
								mSend[i].r -> completed = 1;
								mSend[i].r -> pair = (long) mRecv[pos].r;
								mRecv[pos].r -> completed = 1;
								mRecv[pos].r -> pair = (long) mSend[i].r;
								pthread_cond_broadcast(condVar);
							}
						}
					}
				}
			}

			struct timespec ts;
	                struct timeval tp;
        	        gettimeofday(&tp, NULL);
                	ts.tv_sec = tp.tv_sec;
                	ts.tv_nsec = tp.tv_usec * 1000;
                	ts.tv_sec += (double)2 / 100;
                	pthread_cond_timedwait(condVar, mutex, &ts);
			pthread_mutex_unlock(mutex);
		}

		// Free up position in mRecv once a request is completed
		pthread_mutex_lock(mutex);
		mRecv[pos].r = &MSGRQST_NULL;
		msgRecvCount--;
		pthread_mutex_unlock(mutex);
	} 
	return 1;
}

/** Wait for an array of message requests to complete 

    Input: Array of request pointers, number of requests in array
    Return: 1 when all requests have completed
**/
int msgWaitAll(MSGRQST *requests, int nqrsts) {
	int i;
	for (i = 0; i < nqrsts; i++) {
		msgWait(&requests[i]);
	}
	return 1;
}

/** Read the rank of a (completed) message request 

    Input: request pointer
    Return: src associated with request
**/
int getRank(MSGRQST *request) {
	return request -> src;
}

/** Read the tag of a (completed) message request 

    Input: request pointer
    Return: tag associated with request
**/
int getTag(MSGRQST *request) {
	return request -> tag;
}

/** Read the length of a (completed) message request 

    Input: request pointer
    Return: length of buffer assocaited with request
**/
int getLen(MSGRQST *request) {
	return request -> len;
}
