#ifndef _THREADS_USEM_H_
#define _THREADS_USEM_H_

#include <errno.h>
#include <stdint.h>
#include <time.h>

/* Unnamed semaphore */

#if defined(__MACH__)

#define USEM_INLINE static inline

/*
* ---------------------------------------------------------
* Semaphore (Apple iOS and OSX)
* Can't use POSIX semaphores due to http://lists.apple.com/archives/darwin-kernel/2009/Apr/msg00010.html
*---------------------------------------------------------
*/

#include <dispatch/dispatch.h>

typedef dispatch_semaphore_t usem_t;

USEM_INLINE int usem_init(usem_t *sem, unsigned int initial_count) {
	*sem = dispatch_semaphore_create(initial_count);
	return 0;
}

USEM_INLINE int usem_destroy(usem_t *sem) {
	dispatch_release(*sem);
	return 0;
}

USEM_INLINE int usem_wait(usem_t *sem) {
	if (dispatch_semaphore_wait(*sem, DISPATCH_TIME_FOREVER) == 0) {
		return 0;
	}
	errno = ETIMEDOUT;
	return -1;
}

USEM_INLINE int usem_timed_wait(usem_t *sem, uint64_t timeout_usecs) {
	dispatch_time_t timeout = dispatch_time(DISPATCH_TIME_NOW, timeout_usecs * 1000);

	if (dispatch_semaphore_wait(*sem, timeout) == 0) {
		return 0;
	}
	errno = ETIMEDOUT;
	return -1;
}

USEM_INLINE int usem_try_wait(usem_t *sem) {
	if (dispatch_semaphore_wait(*sem, DISPATCH_TIME_NOW) == 0) {
		return 0;
	}
	errno = ETIMEDOUT;
	return -1;
}

USEM_INLINE void usem_signal(usem_t *sem) {
	dispatch_semaphore_signal(*sem);
}

USEM_INLINE void usem_signal_count(usem_t *sem, int count) {
	while (count-- > 0) {
		usem_signal(sem);
	}
}

#elif defined(__unix__)
/*
* ---------------------------------------------------------
* Semaphore (POSIX, Linux)
* ---------------------------------------------------------
*/

#include <semaphore.h>

#define USEM_INLINE static inline

typedef sem_t usem_t;

USEM_INLINE int usem_init(usem_t *sem, unsigned int initial_count) {
	return sem_init(sem, 0, initial_count);
}

USEM_INLINE int usem_destroy(usem_t *sem) {
	return sem_destroy(sem);
}

USEM_INLINE int usem_wait(usem_t *sem) {
	/* http://stackoverflow.com/questions/2013181/gdb-causes-sem-wait-to-fail-with-eintr-error */
	int rc;
	do {
		rc = sem_wait(sem);
	} while (rc == -1 && errno == EINTR);
	return rc;
}

USEM_INLINE int usem_try_wait(usem_t *sem) {
	int rc;
	do {
		rc = sem_trywait(sem);
	} while (rc == -1 && errno == EINTR);
	return rc;
}

USEM_INLINE int usem_timed_wait(usem_t *sem, uint64_t timeout_usecs) {
	int rc;

	struct timespec ts;
	static const uint64_t usecs_in_1_sec = 1000000;
	static const long nsecs_in_1_sec = 1000000000;
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += (time_t)(timeout_usecs / usecs_in_1_sec);
	ts.tv_nsec += (long)(timeout_usecs % usecs_in_1_sec) * 1000;
	/*
	sem_timedwait bombs if you have more than 1e9 in tv_nsec
	so we have to clean things up before passing it in
	*/
	if (ts.tv_nsec >= nsecs_in_1_sec) {
		ts.tv_nsec -= nsecs_in_1_sec;
		++ts.tv_sec;
	}

	do {
		rc = sem_timedwait(sem, &ts);
	} while (rc == -1 && errno == EINTR);
	return rc;
}

USEM_INLINE void usem_signal(usem_t *sem) {
	while (sem_post(sem) == -1);
}

USEM_INLINE void usem_signal_count(usem_t *sem, ssize_t count) {
	while (count-- > 0) {
		while (sem_post(sem) == -1);
	}
}

#else
#error Unsupported platform! (No semaphore wrapper available)
#endif

#undef USEM_INLINE

#endif /* _THREADS_SEMAPHORE_H_ */
