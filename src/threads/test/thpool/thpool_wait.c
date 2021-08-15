#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include <threads/thpool.h>

#include <ctest.h>

static void sleep_1(void* p) {
	int *i = (int *) p;
	__atomic_fetch_add(i, 1, __ATOMIC_RELAXED);
	usleep(10);
}

static void wait_jobs(size_t num_jobs, size_t num_thpool, int wait_each_job) {
	thpool_t pool = thpool_create(num_thpool, num_jobs);

	size_t i;
	int n = 0;
	for (i = 0; i < num_jobs; i++){
		thpool_add_task(pool, sleep_1, &n);
		if (wait_each_job) {
			thpool_wait(pool);
			ASSERT_EQUAL((ssize_t) i+1, __atomic_add_fetch(&n, 0, __ATOMIC_RELAXED));
		}
	}
	if (!wait_each_job) {
		thpool_wait(pool);
		ASSERT_EQUAL((ssize_t) num_jobs, __atomic_add_fetch(&n, 0, __ATOMIC_RELAXED));
	}

	thpool_destroy(pool);
}

CTEST(thpool_wait, wait_each_job_1) {
	wait_jobs(1000, 1, 1);
}

CTEST(thpool_wait, wait_each_job_4) {
	wait_jobs(1000, 4, 1);
}

CTEST(thpool_wait, wait_all_job_1) {
	wait_jobs(1000, 1, 0);
}

CTEST(thpool_wait, wait_all_job_8) {
	wait_jobs(1000, 8, 0);
}
