/**
 * @File rwlock.h
 *
 * The header file that you need to implement for assignment 3.
 *
 * @author Andrew Quinn, Mitchell Elliott, and Gurpreet Dhillon.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "rwlock.h"
#include <semaphore.h>
#include <pthread.h>

/** @struct rwlock_t
 *
 *  @brief This typedef renames the struct rwlock.  Your `c` file
 *  should define the variables that you need for your reader/writer
 *  lock.
 */

struct rwlock {
    int activeReaders;
    int activeWriters;
    int waitingReaders;
    int waitingWriters;
    int readsDone;
    unsigned int n;
    pthread_mutex_t lock;
    pthread_cond_t writeGo;
    pthread_cond_t readGo;
    PRIORITY p;
    
};
/** @brief Dynamically allocates and initializes a new rwlock with
 *         priority p, and, if using N_WAY priority, n.
 *
 *  @param The priority of the rwlock
 *
 *  @param The n value, if using N_WAY priority
 *
 *  @return a pointer to a new rwlock_t
 */

rwlock_t *rwlock_new(PRIORITY p, uint32_t n) {
    rwlock_t *rw = (rwlock_t *) malloc(sizeof(rwlock_t));
    pthread_mutex_init(&rw->lock, NULL);
    pthread_cond_init(&rw->readGo, NULL);
    pthread_cond_init(&rw->writeGo, NULL);
    rw->p = p;
    rw->n = n;
    rw->activeReaders = 0;
    rw->activeWriters = 0;
    rw->waitingReaders = 0;
    rw->waitingWriters = 0;
    rw->readsDone = 0;
    return rw;
}

/** @brief Delete your rwlock and free all of its memory.
 *
 *  @param rw the rwlock to be deleted.  Note, you should assign the
 *  passed in pointer to NULL when returning (i.e., you should set *rw
 *  = NULL after deallocation).
 *
 */
void rwlock_delete(rwlock_t **rw) {
    pthread_cond_destroy(&(*rw)->readGo);
    pthread_cond_destroy(&(*rw)->writeGo);
    pthread_mutex_destroy(&(*rw)->lock);
    free(*rw);
    *rw = NULL;
}

/** @brief acquire rw for reading
 *
 */


int reader_wait(rwlock_t *rw) {
	if(rw->p == READERS) {
		return rw->activeWriters;
	}
	else if(rw->p == WRITERS) {
		return (rw->activeWriters || rw->waitingWriters);
	}
	else  { 
		if(rw->waitingWriters == 0 && rw->activeWriters == 0) {
			return 0;
			}
		else if(rw->activeWriters > 0 || (rw->waitingWriters > 0 && (rw->readsDone + rw->activeReaders) % (rw->n + 1) == rw->n)) {
		return 1;
		}
		else {
		return 0;
		}
		
	}
        
}

int writer_wait(rwlock_t *rw) {
	if(rw->p == READERS) {
		return (rw->activeReaders || rw->waitingReaders || rw->activeWriters);
	}
	else if(rw->p == WRITERS) {
		return (rw->activeWriters || rw->activeReaders);
	}
	else  {
		if (rw->waitingReaders == 0 && rw->activeReaders == 0 && rw->activeWriters == 0) {
			return 0;
		}
		else if(rw->activeWriters > 0 || rw->activeReaders > 0 || (rw->readsDone + rw->activeReaders) % (rw->n + 1) != rw->n) {
		return 1;
		}
		else {
		return 0;
		}
	}
        
}


void reader_lock(rwlock_t *rw) {
    pthread_mutex_lock(&rw->lock);
    rw->waitingReaders++;
    //READER PRIORITY
    if(rw->p == READERS) {
    while(reader_wait(rw)) {
	pthread_cond_wait(&rw->readGo, &rw->lock);
	}
	rw->waitingReaders--;
	rw->activeReaders++;
    }
    //WRITER PRIORITY
    else if(rw->p == WRITERS) {
    while(reader_wait(rw)) {
       pthread_cond_wait(&rw->readGo, &rw->lock);     
    }
        rw->waitingReaders--;
        rw->activeReaders++;
}
    else if (rw->p == N_WAY) {
	while(reader_wait(rw)) {
        pthread_cond_wait(&rw->readGo, &rw->lock);
        }
        rw->waitingReaders--;
        rw->activeReaders++;
	}
	

 pthread_mutex_unlock(&rw->lock);
}

/** @brief release rw for reading--you can assume that the thread
 * releasing the lock has *already* acquired it for reading.
 *
 */
void reader_unlock(rwlock_t *rw) {
    pthread_mutex_lock(&rw->lock);
    //READER PRIORITY
    rw->activeReaders--;
    rw->readsDone++;
    if(rw->p == READERS) {
    if(rw->waitingReaders > 0) {
    pthread_cond_broadcast(&rw->readGo);
        }
    else if(rw->activeReaders == 0 || rw->waitingWriters >= 0 ) { 
	pthread_cond_signal(&rw->writeGo);
	}
    }
    //WRITER PRIORITY
    else if (rw->p == WRITERS) {
	if(rw->waitingWriters > 0 && rw->activeReaders == 0) {
		pthread_cond_signal(&rw->writeGo);
	}
	else if(rw->activeWriters == 0 && rw->waitingWriters == 0) {
		pthread_cond_broadcast(&rw->readGo);
	}
	}
   else if (rw->p == N_WAY) {
	if((rw->readsDone % (rw->n + 1) == rw->n) || rw->activeReaders + rw->waitingReaders == 0 ) {
		pthread_cond_signal(&rw->writeGo); 	
	}
	else {
		pthread_cond_broadcast(&rw->readGo);
	}
	}
pthread_mutex_unlock(&rw->lock);
}
/** @brief acquire rw for writing
 *
 */
void writer_lock(rwlock_t *rw) {
    pthread_mutex_lock(&rw->lock);
    rw->waitingWriters++;
    while (writer_wait(rw)) {
	pthread_cond_wait(&rw->writeGo, &rw->lock);
	}
    rw->waitingWriters--;
    rw->activeWriters++;   
    pthread_mutex_unlock(&rw->lock);
}

void writer_unlock(rwlock_t *rw) {
    pthread_mutex_lock(&rw->lock);
    //READER PRIORITY
    if(rw->p == READERS) {
    	rw->activeWriters--;
    	if(rw->activeReaders == 0 && rw->waitingReaders == 0) {
    		pthread_cond_signal(&rw->writeGo);
    		}
    	else if(rw->waitingReaders > 0) {
    		pthread_cond_broadcast(&rw->readGo);
    }
    }
    //WRITER PRIORITY
    else if (rw->p == WRITERS) {
      	rw->activeWriters--;
	if(rw->activeReaders == 0 && rw->waitingWriters > 0) {
		pthread_cond_signal(&rw->writeGo);
	}
	else if (rw->waitingWriters == 0) {
		pthread_cond_broadcast(&rw->readGo);
	}
}
    else if (rw->p == N_WAY) {
	rw->activeWriters--;
	rw->readsDone = 0;
	if(rw->waitingReaders > 0) {
		pthread_cond_broadcast(&rw->readGo);
	}
	else if (rw->waitingReaders == 0 && rw->activeReaders == 0 && rw->waitingWriters > 0) {
		pthread_cond_signal(&rw->writeGo);
	}
	}    
pthread_mutex_unlock(&rw->lock);
}


