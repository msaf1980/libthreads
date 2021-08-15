#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <sched.h>

#include <threads/thpool.h>

#include <pthread.h>
#if NO_PTHREAD_BARRIER
#include "pthread_barrier.h"
#endif

#include <ctest.h>

static void sleep_job(void *p){
	int *n = (int *) p;
	__atomic_add_fetch(n, 1, __ATOMIC_RELEASE);
	puts("SLEPT");
}


CTEST(thpool_api, test) {
	thpool_t pool;
	int n;

	/* Test if we can get the current number of working thpool */
	n = 0;
	pool = thpool_create(10, 10);
	thpool_add_task(pool, sleep_job, &n);
	thpool_add_task(pool, sleep_job, &n);
	thpool_add_task(pool, sleep_job, &n);
	thpool_add_task(pool, sleep_job, &n);
	
	thpool_wait(pool);

	ASSERT_EQUAL_U(0, thpool_active_tasks(pool));
	ASSERT_EQUAL_U(0, thpool_total_tasks(pool));
	ASSERT_EQUAL(4, __atomic_add_fetch(&n, 0, __ATOMIC_RELEASE));

	thpool_destroy(pool);

	/* Test (same as above) */
	n  = 0;
	pool = thpool_create(5, 5);
	thpool_add_task(pool, sleep_job, &n);
	thpool_add_task(pool, sleep_job, &n);

	thpool_wait(pool);

	ASSERT_EQUAL_U(0, thpool_active_tasks(pool));
	ASSERT_EQUAL_U(0, thpool_total_tasks(pool));
	ASSERT_EQUAL(2, __atomic_add_fetch(&n, 0, __ATOMIC_RELEASE));

	thpool_destroy(pool);
}

#define WRITERS 10
#define LOOP_COUNT 100000

struct task_param {
	int n;
	thpool_t pool;
};

static void increment_job(void *p){
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"
	int *i = (int *) p;
	__atomic_add_fetch(i, 1, __ATOMIC_RELEASE);
#pragma GCC diagnostic pop	
}

static void *add_task_thread(void *p){
	size_t i;
	struct task_param *param = (struct task_param *) p;
	for (i = 0; i < LOOP_COUNT; i++) {
		thpool_add_task_try(param->pool, increment_job, &param->n, 10, 1000);
	}
	return NULL;
}

CTEST(thpool_api, threads_test) {
	struct task_param param;
	int perr;
	size_t i;
    pthread_attr_t thr_attr;
    pthread_t t_handles[WRITERS];

	param.n = 0;
	param.pool = thpool_create(10, 10240);

    pthread_attr_init(&thr_attr);
    pthread_attr_setdetachstate(&thr_attr, PTHREAD_CREATE_JOINABLE);

    for (i = 0; i < WRITERS; i++) {
        perr = pthread_create(&t_handles[i], &thr_attr, add_task_thread, &param);
        ASSERT_EQUAL_D(0, perr, "thread create");
	}

	/* Test if we can get the current number of working thpool */

	for (i = 0; i < WRITERS; i++) {
        pthread_join(t_handles[i], NULL);
	}

	thpool_wait(param.pool);

	ASSERT_EQUAL_U(0, thpool_active_tasks(param.pool));
	ASSERT_EQUAL_U(0, thpool_total_tasks(param.pool));
	ASSERT_EQUAL(WRITERS * LOOP_COUNT, __atomic_add_fetch(&param.n, 0, __ATOMIC_RELEASE));

	thpool_destroy(param.pool);
}
