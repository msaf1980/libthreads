#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <sched.h>

#include <threads/lfthpool.h>

#include <ctest.h>

static void increment(void *p){
	int *n = (int *) p;
	__atomic_fetch_add(n, 1, __ATOMIC_RELAXED);
}

CTEST(lfthpool_pause_resume, test) {
	size_t i, num_lfthpool = 4, jobs = 2;
	int n = 0;

	lfthpool_t pool = lfthpool_create(num_lfthpool, jobs);
	
	lfthpool_pause(pool);

	sleep(1);
	
	/* Since pool is paused, lfthpool should not start before main's sleep */
	for (i = 0; i < jobs; i++) {
		lfthpool_add_task(pool, increment, &n);
	}
	
	sleep(2);

	ASSERT_EQUAL_D(0, __atomic_load_n(&n, __ATOMIC_RELAXED), "lfthpool_pause not work");
	
	/* Now we will start lfthpool in no-parallel with main */
	lfthpool_resume(pool);

	lfthpool_wait(pool);
	sched_yield();	
	usleep(100); /* Give some time to lfthpool to get the work */

	ASSERT_EQUAL_D((ssize_t) jobs, __atomic_load_n(&n, __ATOMIC_RELAXED), "lfthpool_resume not work");
	
	lfthpool_destroy(pool); // Wait for work to finish
}
