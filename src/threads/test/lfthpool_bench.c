/*
 * Try to run lfthpool with a non-zero heap and stack
 */
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <threads/lfthpool.h>

#include <pthread.h>
#if NO_PTHREAD_BARRIER
#include "pthread_barrier.h"
#endif

size_t LOOP_COUNT = 10000000;

int ret = 0;

static void task(void *arg) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"	
	size_t *n = (size_t *)arg;
	__atomic_add_fetch(n, 1, __ATOMIC_RELAXED);
#pragma GCC diagnostic pop
}

static uint64_t getCurrentTime(void) {
    struct timeval now;
    uint64_t now64;
    gettimeofday(&now, NULL);
    now64 = (uint64_t) now.tv_sec;
    now64 *= 1000000;
    now64 += ((uint64_t) now.tv_usec);
    return now64;
}

struct task_param {
	size_t n;
	size_t w;
	size_t loop_count;
	lfthpool_t pool;
	pthread_barrier_t start_barrier;
};

static void *add_task_thread(void *p){
	size_t i;
	struct task_param *param = (struct task_param *) p;
	pthread_barrier_wait(&param->start_barrier);
	for (i = 0; i < param->loop_count; i++) {
		if (lfthpool_add_task_try(param->pool, task, &param->n, 200, 200) == 0) {
			param->w++;
		}
	}
	return NULL;
}

void bench(size_t writers, size_t readers, size_t loop_count) {
	size_t i;
	uint64_t start, end, duration;
	struct task_param param;
	int perr;
    pthread_attr_t thr_attr;
    pthread_t *t_handles;
	size_t queue_size;

	if (loop_count > 40000000) {
		queue_size = 40000000;
	} else {
		queue_size = loop_count;
	}

	param.n = 0;
	param.w = 0;
	param.loop_count = loop_count;
	param.pool = lfthpool_create(readers, queue_size);

	pthread_barrier_init(&param.start_barrier, NULL, (unsigned int) writers + 1);

	pthread_attr_init(&thr_attr);
    pthread_attr_setdetachstate(&thr_attr, PTHREAD_CREATE_JOINABLE);
	t_handles = (pthread_t *) malloc(writers * sizeof(pthread_t));
	for (i = 0; i < (size_t) writers; i++) {
		perr = pthread_create(&t_handles[i], &thr_attr, add_task_thread, &param);
        if (perr) {
			fprintf(stderr, "%s\n", strerror(perr));
			exit(1);
		}
	}

	pthread_barrier_wait(&param.start_barrier);
	start = getCurrentTime();

	for (i = 0; i < (size_t) writers; i++) {
		pthread_join(t_handles[i], NULL);
	}

	lfthpool_wait(param.pool);
	lfthpool_wait(param.pool);
	
	end = getCurrentTime();

	lfthpool_destroy(param.pool);
	duration = end - start;
	if (param.n != loop_count * (size_t) writers) {
		ret++;	
	}
	printf("lfthpool, %llu threads pool, %llu writers (%f ms, %lu iterations, %llu ns/op, %llu op/s) ",
		(unsigned long long) readers, (unsigned long long) writers,
		((double) end - (double) start) / 1000,
		(unsigned long) loop_count,
		(unsigned long long) duration * 1000 / loop_count,
		(unsigned long long) 1000000 * loop_count / duration
	);
	if (param.n != loop_count * (size_t) writers) {
		printf("[ERR]: %llu != %llu after %llu\n", 
			(unsigned long long) param.n, (unsigned long long) loop_count * (size_t) writers,
			(unsigned long long) param.w
		);
	} else {
		printf("[OK]\n");
	}
}

int main() {
	char *COUNT_STR = getenv("LOOP_COUNT");
	if (COUNT_STR) {
		unsigned long c = strtoul(COUNT_STR, NULL, 10);
		if (c > 0) {
			LOOP_COUNT = c;
		}
	}
	bench(1, 4, LOOP_COUNT);
	bench(4, 4, LOOP_COUNT);
	return ret;
}
