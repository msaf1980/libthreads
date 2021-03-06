/*
 * Try to run thpool with a non-zero heap and stack
 */
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <threads/psem.h>

#include <pthread.h>
#if NO_PTHREAD_BARRIER
#include "pthread_barrier.h"
#endif

size_t LOOP_COUNT = 1000000;

int ret = 0;

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
	psem_t sem;
	pthread_barrier_t start_barrier;
};

static void *notify_thread(void *p){
	struct task_param *param = (struct task_param *) p;
	pthread_barrier_wait(&param->start_barrier);
	while (__atomic_fetch_add(&param->n, 0, __ATOMIC_RELAXED) != 0) {
		psem_signal(&param->sem);
	}
	return NULL;
}

static void *wait_thread(void *p){
	struct task_param *param = (struct task_param *) p;
	while (__atomic_fetch_add(&param->w, 1, __ATOMIC_RELAXED) < param->loop_count) {
		psem_wait(&param->sem);
	}
	__atomic_fetch_sub(&param->n, 1, __ATOMIC_RELAXED);
	return NULL;
}

void bench(size_t writers, size_t readers, size_t loop_count) {
	size_t i;
	uint64_t start, end, duration;
	struct task_param param;
	int perr;
	pthread_attr_t thr_attr;
	pthread_t *t_handles_write, *t_handles_read;

	param.n = readers;
	param.w = 0;
	param.loop_count = loop_count;
	psem_init(&param.sem);

	pthread_barrier_init(&param.start_barrier, NULL, (unsigned int) writers + 1);

	pthread_attr_init(&thr_attr);
	pthread_attr_setdetachstate(&thr_attr, PTHREAD_CREATE_JOINABLE);
	t_handles_write = (pthread_t *) malloc((size_t) writers * sizeof(pthread_t));
	for (i = 0; i < writers; i++) {
		perr = pthread_create(&t_handles_write[i], &thr_attr, notify_thread, &param);
        if (perr) {
			fprintf(stderr, "%s\n", strerror(perr));
			exit(1);
		}
	}
	t_handles_read = (pthread_t *) malloc((size_t) readers * sizeof(pthread_t));
	for (i = 0; i < readers; i++) {
		perr = pthread_create(&t_handles_read[i], &thr_attr, wait_thread, &param);
        if (perr) {
			fprintf(stderr, "%s\n", strerror(perr));
			exit(1);
		}
	}

	pthread_barrier_wait(&param.start_barrier);
	start = getCurrentTime();

	for (i = 0; i < writers; i++) {
		pthread_join(t_handles_write[i], NULL);
	}
	for (i = 0; i < readers; i++) {
		pthread_join(t_handles_read[i], NULL);
	}

	end = getCurrentTime();

	free(t_handles_write);
	free(t_handles_read);

	duration = end - start;
	if (param.w < loop_count || param.w > loop_count + readers) {
		ret++;	
		perr = 1;
	} else {
		perr = 0;
	}
	printf("%llu writers, %llu readers (%f ms, %lu iterations, %llu ns/op, %llu op/s) [%s]\n",
		(unsigned long long) writers, (unsigned long long) readers,
		((double) end - (double) start) / 1000,
		(unsigned long) loop_count,
		(unsigned long long) duration * 1000 / loop_count,
		(unsigned long long) 1000000 * loop_count / duration,			
		perr == 0 ? "OK" : "ERR");
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
