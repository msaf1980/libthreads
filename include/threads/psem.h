#ifndef _THREADS_PSEM_H_
#define _THREADS_PSEM_H_

#include <stddef.h>
#include <pthread.h>

/**
 * @file
*
* Public header
*/

/**
 * @brief   Pthreads semaphore (condition variable with mutex)
 * @typedef psem_t
 */
typedef struct psem {
    pthread_mutex_t lock;
    pthread_cond_t notify;
} psem_t;

#define PSEM_INLINE static inline 

/**
 * @brief       Init semaphore
 * @param  s    Semaphore
 * @retval      0 - on success, pthread-like error code on error
 */
PSEM_INLINE int psem_init(psem_t *s) {
    int err;
    if ((err = pthread_mutex_init(&(s->lock), NULL)) != 0) {
		return err;
	}
	return pthread_cond_init(&(s->notify), NULL);
}

/**
 * @brief       Destroy semaphore
 * @param  s    Semaphore
 */
PSEM_INLINE void psem_destroy(psem_t *s) {
    pthread_mutex_destroy(&(s->lock));
	pthread_cond_destroy(&(s->notify));
}

/**
 * @brief       Signal with semaphore
 * @param  s    Semaphore
 * @retval      0 - on success, pthread-like error code on error
 */
PSEM_INLINE int psem_signal(psem_t *s) {
    int err = pthread_mutex_lock(&(s->lock));
    if (err == 0) {
        err = pthread_cond_signal(&(s->notify));
        pthread_mutex_unlock(&(s->lock));
    }
    return err;
}

/**
 * @brief       Broadcast signal with semaphore
 * @param  s    Semaphore
 * @retval      0 - on success, pthread-like error code on error
 */
PSEM_INLINE int psem_broadcast(psem_t *s) {
    int err = pthread_mutex_lock(&(s->lock));
    if (err == 0) {
        err = pthread_cond_broadcast(&(s->notify));
        pthread_mutex_unlock(&(s->lock));
    }
    return err;
}

/**
 * @brief       Wait for signal with semaphore
 * @param  s    Semaphore
 * @retval      0 - on success, pthread-like error code on error
 */
PSEM_INLINE int psem_wait(psem_t *s) {
    int err = pthread_mutex_lock(&(s->lock));
    if (err == 0) {
        err = pthread_cond_wait(&(s->notify), &(s->lock));
        pthread_mutex_unlock(&(s->lock));
    }
    return err;
}

#undef PSEM_INLINE

#endif /* _THREADS_PSEM_H_ */
