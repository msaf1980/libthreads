#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

#include <threads/thpool.h>

#include <ctest.h>

static void increment(void *p){
	int *n = (int *) p;
	__atomic_fetch_add(n, 1, __ATOMIC_RELAXED);
}

CTEST(thpool_worker, try_once) {
	int n = 0;
	size_t i, jobs = 2, num_thpool = 4;

	thpool_t pool = thpool_create(num_thpool, jobs);
	
	thpool_pause(pool);

	usleep(300);
	
	/* Since pool is paused, thpool should not start before main's sleep */
	for (i = 0; i < jobs; i++) {
		thpool_add_task(pool, increment, &n);
	}
	
	sleep(2);

	ASSERT_EQUAL_D(0, __atomic_load_n(&n, __ATOMIC_RELAXED), "thpool_pause not work");
	
    for (i = 0; i < jobs; i++) {
        ASSERT_EQUAL_D(0, thpool_worker_try_once(pool), "thpool_worker_try_once can't process task");
	}

    ASSERT_EQUAL_D(-1, thpool_worker_try_once(pool), "thpool_worker_try_once proccess task, it's error");
	
	ASSERT_EQUAL_D((ssize_t) jobs, __atomic_load_n(&n, __ATOMIC_RELAXED), "thpool_resume not work");
	
	thpool_destroy(pool); // Wait for work to finish
}
