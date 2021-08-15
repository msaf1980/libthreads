#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <sched.h>

#include <pthread.h>

#include <threads/lfthpool.h>

#include <ctest.h>

static void sleep_job(void *p){
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"	
	int *n = (int *) p;
	__atomic_add_fetch(n, 1, __ATOMIC_RELEASE);
#pragma GCC diagnostic pop	
	puts("SLEPT");
}

CTEST(lfthpool_api, test) {
	lfthpool_t pool;
	int n;

	/* Test if we can get the current number of working lfthpool */
	n = 0;
	pool = lfthpool_create(10, 10);
	lfthpool_add_task(pool, sleep_job, &n);
	lfthpool_add_task(pool, sleep_job, &n);
	lfthpool_add_task(pool, sleep_job, &n);
	lfthpool_add_task(pool, sleep_job, &n);
	
	lfthpool_wait(pool);
	sched_yield();
	usleep(200);
	lfthpool_wait(pool);

	ASSERT_EQUAL_U(0, lfthpool_active_tasks(pool));
	ASSERT_EQUAL_U(0, lfthpool_total_tasks(pool));
	ASSERT_EQUAL(4, __atomic_add_fetch(&n, 0, __ATOMIC_RELEASE));

	lfthpool_destroy(pool);

	/* Test (same as above) */
	n  = 0;
	pool = lfthpool_create(5, 5);
	lfthpool_add_task(pool, sleep_job, &n);
	lfthpool_add_task(pool, sleep_job, &n);

	lfthpool_wait(pool);
	sched_yield();
	usleep(200);
	lfthpool_wait(pool);

	ASSERT_EQUAL_U(0, lfthpool_active_tasks(pool));
	ASSERT_EQUAL_U(0, lfthpool_total_tasks(pool));
	ASSERT_EQUAL(2, __atomic_add_fetch(&n, 0, __ATOMIC_RELEASE));

	lfthpool_destroy(pool);
}

#define WRITERS 10
#define LOOP_COUNT 100000

struct task_param {
	int n;
	lfthpool_t pool;
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
		lfthpool_add_task_try(param->pool, increment_job, &param->n, 10, 4000);
	}
	return NULL;
}

CTEST(lfthpool_api, threads_test) {
	struct task_param param;
	int perr;
	size_t i;
    pthread_attr_t thr_attr;
    pthread_t t_handles[WRITERS];

	param.n = 0;
	param.pool = lfthpool_create(10, 1024000);

    pthread_attr_init(&thr_attr);
    pthread_attr_setdetachstate(&thr_attr, PTHREAD_CREATE_JOINABLE);

    for (i = 0; i < WRITERS; i++) {
        perr = pthread_create(&t_handles[i], &thr_attr, add_task_thread, &param);
        ASSERT_EQUAL_D(0, perr, "thread create");
	}

	/* Test if we can get the current number of working lfthpool */

	for (i = 0; i < WRITERS; i++) {
        pthread_join(t_handles[i], NULL);
	}

	lfthpool_wait(param.pool);
	sched_yield();
	usleep(300);
	sched_yield();
	lfthpool_wait(param.pool);

	lfthpool_shutdown(param.pool);

	ASSERT_EQUAL_U(0, lfthpool_active_tasks(param.pool));
	ASSERT_EQUAL_U(0, lfthpool_total_tasks(param.pool));
	ASSERT_EQUAL(WRITERS * LOOP_COUNT, __atomic_add_fetch(&param.n, 0, __ATOMIC_RELEASE));

	lfthpool_destroy(param.pool);
}
