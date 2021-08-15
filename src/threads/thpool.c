/* ********************************
 * Author:       Johan Hanssen Seferidis
 * License:	     MIT
 * Description:  Library providing a threading pool where you can add
 *               work. For usage, check the thpool.h file or README.md
 *
 *//** @file thpool.h *//*
 *
 ********************************/

#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#if defined(__linux__)
#include <sys/prctl.h>
#endif

#include "threads/thpool.h"

/* ========================== STRUCTURES ============================ */

/**
 * Struct to hold data for an individual task for a thread pool
 */
typedef struct task {
	void (*function)(void *); //pointer to the function the task executes
	void *arg;
} task_t;

/**
 * Struct to hold data for an individual thread pool.
 */
struct thpool {
	int shutdown;
	int hold; /* hold task queue */
	size_t running_count;	
	pthread_mutex_t lock;  /* lock for enqueue/dequeue task */
	pthread_cond_t notify; /* notify for enqueue task */
	pthread_cond_t notify_empty;   /* notify for end tasks processing */
	pthread_mutex_t lock_resize;  /* lock for resize workers */
	pthread_t *thpool; /* thpool */
	volatile size_t thread_count;
	task_t *task_queue;    /* task queue */
	size_t queue_size;
	volatile size_t queue_count;
	size_t head;
	size_t tail;
};

/* ========================== THREADPOOL ============================ */

static void* _thpool_worker(void* _pool);

/* ========================== THREADPOOL ============================ */

thpool_t thpool_create(size_t workers, size_t queue_size) {
	int err;
	size_t i;
	thpool_t pool;

	if (workers < 1 || queue_size < 1) {
		errno = EINVAL;
		return NULL;
	}
	/* allocate new pool */
	pool = (thpool_t) malloc(sizeof(struct thpool)); 
	if (pool == NULL)
		goto ERROR_ERRNO;

	/* Pool settings */
	pool->queue_size = queue_size;
	pool->queue_count = 0;
	pool->thread_count = workers;

	pool->head = 0;
	pool->tail = pool->head;

	pool->running_count = 0;
	pool->hold = 0;
	/* allocate thread array */
	pool->thpool = (pthread_t*) malloc(sizeof(pthread_t) * (size_t) pool->thread_count);
	/* allocate task queue */
	pool->task_queue = (task_t*) malloc(sizeof(task_t) * (size_t) pool->queue_size);

	if (pool->thpool == NULL || pool->task_queue == NULL) {
		err = ENOMEM;
		goto ERROR;
	}

	pool->shutdown = 0;

	for (i = 0; i < (pool->thread_count); i++) {
		pool->thpool[i] = 0;
	}

	/* initialise mutexes */
	if ((err = pthread_mutex_init(&(pool->lock), NULL)) != 0) {
		goto ERROR;
	}
	if ((err = pthread_cond_init(&(pool->notify), NULL)) != 0) {
		goto ERROR;
	}
	if ((err = pthread_cond_init(&(pool->notify_empty), NULL)) != 0) {
		goto ERROR;
	}
	/* instantiate worker thpool */
	for (i = 0; i < (pool->thread_count); i++) {
		if ((err = pthread_create(&pool->thpool[i], NULL, _thpool_worker, (void *) pool))) {
			goto ERROR_ERRNO;
		}
	}

	return pool;

ERROR_ERRNO:
	err = errno;
ERROR:
	if (pool) {
		thpool_destroy(pool);
	}
	errno = err;
	return NULL;
}

size_t thpool_workers_count(thpool_t pool) {
	size_t thread_count;
	pthread_mutex_lock(&(pool->lock));
	thread_count = pool->thread_count;
	pthread_mutex_unlock(&(pool->lock));
	return thread_count;
}

int thpool_add_task(thpool_t pool, void (*function)(void *), void* arg) {

	/* set up task */
	task_t task;
	task.function = function;
	task.arg = arg;

	pthread_mutex_lock(&(pool->lock)); /* enter critical section */

	if (pool->queue_count == pool->queue_size) {
		pthread_mutex_unlock(&(pool->lock)); /* release lock */
		sched_yield();
		return -1;
	}

	pool->task_queue[pool->tail] = task; /* add task to end of queue */
	pool->tail = (pool->tail + 1) % pool->queue_size; /* advance end of queue */
	pool->queue_count++; /* job added to queue */

	pthread_cond_signal(&(pool->notify)); /* notify waiting workers of new job */
	pthread_mutex_unlock(&(pool->lock)); /* end critical section */

	return 0;
}

int thpool_add_task_try(thpool_t pool, void (*function)(void *), void* arg, useconds_t usec, int max_try) {
	/* set up task */
	task_t task;
	task.function = function;
	task.arg = arg;

	for (; ; max_try--) {
		if (max_try < 0) {
			errno = EAGAIN;
			return -1;
		}
		pthread_mutex_lock(&(pool->lock)); /* enter critical section */
		if (pool->queue_count == pool->queue_size) {
			pthread_mutex_unlock(&(pool->lock)); /* release lock */
			sched_yield();
			usleep(usec);
		} else {
			pool->task_queue[pool->tail] = task; /* add task to end of queue */
			pool->tail = (pool->tail + 1) % pool->queue_size; /* advance end of queue */
			pool->queue_count++; /* job added to queue */

			pthread_cond_signal(&(pool->notify)); /* notify waiting workers of new job */
			pthread_mutex_unlock(&(pool->lock)); /* end critical section */
			break;
		}
	}

	return 0;
}

void thpool_pause(thpool_t pool) {
	__atomic_store_n(&(pool->hold), 1, __ATOMIC_RELEASE);
}

void thpool_resume(thpool_t pool) {
	__atomic_store_n(&(pool->hold), 0, __ATOMIC_RELEASE);
	pthread_mutex_lock(&(pool->lock));
	pthread_cond_signal(&(pool->notify));
	pthread_mutex_unlock(&(pool->lock));
}

size_t thpool_active_tasks(thpool_t pool) {
	return __atomic_add_fetch(&pool->running_count, 0, __ATOMIC_RELAXED);
}

size_t thpool_total_tasks(thpool_t pool) {
	size_t count;

	pthread_mutex_lock(&(pool->lock));
	count = __atomic_add_fetch(&pool->running_count, 0, __ATOMIC_RELAXED) + pool->queue_count;
	pthread_mutex_unlock(&(pool->lock));

	return count;
}

void thpool_wait(thpool_t pool) {
	size_t queue_count, active_count;
	while (1) {		
		pthread_mutex_lock(&(pool->lock));
		queue_count = pool->queue_count;
		active_count = thpool_active_tasks(pool);
		if (queue_count == 0 && active_count == 0) {
			pthread_mutex_unlock(&(pool->lock));
			return;
		}
		pthread_cond_wait(&(pool->notify_empty), &(pool->lock));
		pthread_mutex_unlock(&(pool->lock));
	}
}

void thpool_shutdown(thpool_t pool) {
	size_t i;
	__atomic_store_n(&pool->shutdown, 1, __ATOMIC_RELEASE);
	pthread_mutex_lock(&(pool->lock));
	pthread_cond_broadcast(&(pool->notify));
	pthread_mutex_unlock(&(pool->lock));
	for (i = 0; i < pool->thread_count; i++) {
		pthread_join(pool->thpool[i], NULL);
	}
}

void thpool_destroy(thpool_t pool) {
	if (pool) {
		thpool_shutdown(pool);
		free(pool->thpool);
		free(pool->task_queue);
		pthread_cond_destroy(&(pool->notify));
		pthread_cond_destroy(&(pool->notify_empty));
		pthread_mutex_destroy(&(pool->lock));
		free(pool);
	}
}

int thpool_worker_try_once(thpool_t pool) {
	task_t task;

	/*
	* take lock. thread is blocked if not possible to take, that's fine.
	* need the lock in order to wait on condition. don't want condition
	* being changed.
	*/
	pthread_mutex_lock(&(pool->lock));

	/* wait for notification of new task when pool is empty */
	if (pool->queue_count == 0) {
		pthread_mutex_unlock(&(pool->lock));
		return -1;
	}

	/* ignore thread pool hold */

	/* increment active tasks count */
	__atomic_add_fetch(&pool->running_count, 1, __ATOMIC_RELAXED);

	/* grab the next task in the queue and run it */
	task.function = pool->task_queue[pool->head].function;
	task.arg = pool->task_queue[pool->head].arg;

	/* increment head of queue */
	pool->head = (pool->head+1) % pool->queue_size;
	pool->queue_count--; /* removed a task from queue */

	/* end critical section */
	pthread_mutex_unlock(&(pool->lock));

	/* execute task*/
	(*task.function)(task.arg);

	__atomic_sub_fetch(&pool->running_count, 1, __ATOMIC_RELAXED);

	return 0;
}

/* pool background worker */
static void* _thpool_worker(void* p) {
	thpool_t pool = (thpool_t) p;
	task_t task;

	while (1) {
		/*
		* take lock. thread is blocked if not possible to take, that's fine.
		* need the lock in order to wait on condition. don't want condition
		* being changed.
		*/
		pthread_mutex_lock(&(pool->lock));

		/* wait for notification of new task when pool is empty */
		while(pool->queue_count == 0) {
			if (thpool_active_tasks(pool) == 0) {
				pthread_cond_signal(&(pool->notify_empty)); /* notify when empty */
			}
			/* check shutdown flag */
			if (__atomic_add_fetch(&pool->shutdown, 0, __ATOMIC_ACQUIRE) == 1) {
				pthread_mutex_unlock(&(pool->lock));
				return NULL;
			}
			/*
			* thread is blocked while waiting for a notification of work.
			* does this with an atomic release of lock so other workers can
			* also be waiting. lock is retained upon notification
			* no more busy waiting!
			*/
			pthread_cond_wait(&(pool->notify), &(pool->lock));
		}
		/* check thread pool hold */
		if ( __atomic_add_fetch(&(pool->hold), 0, __ATOMIC_RELEASE)) {
			pthread_mutex_unlock(&(pool->lock));
			sleep(1);
			continue;
		}

		/* increment active tasks count */
		__atomic_add_fetch(&pool->running_count, 1, __ATOMIC_RELAXED);

		/* grab the next task in the queue and run it */
		task.function = pool->task_queue[pool->head].function;
		task.arg = pool->task_queue[pool->head].arg;

		/* increment head of queue */
		pool->head = (pool->head+1) % pool->queue_size;
		pool->queue_count--; /* removed a task from queue */

		/* end critical section */
		pthread_mutex_unlock(&(pool->lock));

		/* execute task*/
		(*task.function)(task.arg);

		/* decrement active tasks count */
		__atomic_sub_fetch(&pool->running_count, 1, __ATOMIC_RELAXED);
	}

	return NULL;
}

/* ========================== THREADPOOL THREAD ===================== */
