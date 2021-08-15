/*
 * Try to run thpool with a non-zero heap and stack
 */
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <threads/lusem.h>

#include <pthread.h>
#if NO_PTHREAD_BARRIER
#include "pthread_barrier.h"
#endif

#define CTEST_MAIN
#define CTEST_SEGFAULT

#include <ctest.h>

size_t LOOP_COUNT = 1000000;

int ret = 0;

struct task_param {
	size_t n;
	size_t w;
	size_t loop_count;
	lusem_t lsem;
	pthread_barrier_t start_barrier;
};

static void *notify_thread(void *p){
	struct task_param *param = (struct task_param *) p;
	pthread_barrier_wait(&param->start_barrier);
	while (__atomic_fetch_add(&param->n, 0, __ATOMIC_RELAXED) != 0) {
		lusem_signal(&param->lsem);
	}
	return NULL;
}

static void *wait_thread(void *p){
	struct task_param *param = (struct task_param *) p;
	while (__atomic_fetch_add(&param->w, 1, __ATOMIC_RELAXED) < param->loop_count) {
		lusem_wait(&param->lsem);
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
	lusem_init(&param.lsem, 0, 2);

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

static void *signal_thread(void *p){
	lusem_t *lsem = (lusem_t *) p;
	lusem_signal(lsem);
	return NULL;
}

static int signal_post(pthread_t *tid, lusem_t *lsem) {
	pthread_attr_t thr_attr;
	pthread_attr_init(&thr_attr);
	pthread_attr_setdetachstate(&thr_attr, PTHREAD_CREATE_JOINABLE);
	usleep(1000);
	return pthread_create(tid, &thr_attr, signal_thread, lsem);
}

CTEST_DATA(lusem) {
    lusem_t lsem;
    pthread_t tid;
};

CTEST_SETUP(lusem) {
    lusem_init(&data->lsem, 0, 2);
    data->tid = 0;
}

CTEST_TEARDOWN(lusem) {
    if (data->tid != 0) {
		pthread_join(data->tid, NULL);
	}
	lusem_destroy(&data->lsem);
}

CTEST2(lusem, wait) {
	int rc;
    int perr = signal_post(&data->tid, &data->lsem);
	ASSERT_EQUAL_D(0, perr, strerror(perr));
	lusem_signal(&data->lsem);
	rc = lusem_wait(&data->lsem);
	ASSERT_EQUAL_D(0, rc, strerror(errno));
}

CTEST2(lusem, timed_wait_timeout) {
	int rc;
    uint64_t timeout = 20000;
	uint64_t start, duration;
	start = getCurrentTime();
	rc = lusem_timed_wait(&data->lsem, timeout);
	duration = getCurrentTime() - start;
	ASSERT_TRUE_D(rc != 0, strerror(errno));
	if (duration < 2 * timeout / 3 || duration > 10 * timeout) {
		CTEST_ERR("timeout duration is %llu us, want %llu", (unsigned long long) duration, (unsigned long long) timeout);
	}
}

CTEST2(lusem, timed_wait) {
	int rc;
    int perr = signal_post(&data->tid, &data->lsem);
	ASSERT_EQUAL_D(0, perr, strerror(perr));
	rc = lusem_timed_wait(&data->lsem, 20000);
	ASSERT_EQUAL(0, rc);
}

int main(int argc, const char *argv[]) {
	char *COUNT_STR = getenv("LOOP_COUNT");
	if (COUNT_STR) {
		unsigned long c = strtoul(COUNT_STR, NULL, 10);
		if (c > 0) {
			LOOP_COUNT = c;
		}
	}

    ret += ctest_main(argc, argv);

	bench(1, 4, LOOP_COUNT);
	bench(4, 4, LOOP_COUNT);
	return ret;
}
