#ifndef _THREADS_USEM_H_
#define _THREADS_USEM_H_

#include <errno.h>
#include <stdint.h>
#include <time.h>

/* Unnamed semaphore */

#if defined(__MACH__)

#define USEM_INLINE static inline

//---------------------------------------------------------
// Semaphore (Apple iOS and OSX)
// Can't use POSIX semaphores due to http://lists.apple.com/archives/darwin-kernel/2009/Apr/msg00010.html
//---------------------------------------------------------

#include <mach/mach.h>

typedef semaphore_t usem_t;

USEM_INLINE int usem_init(usem_t *sem, unsigned int initial_count) {
	return semaphore_create(mach_task_self(), sem, SYNC_POLICY_FIFO, (int) initial_count);
}

USEM_INLINE int usem_destroy(usem_t *sem) {
	return semaphore_destroy(mach_task_self(), *sem);
}

USEM_INLINE int usem_wait(usem_t *sem) {
	return semaphore_wait(*sem) == KERN_SUCCESS;
}

USEM_INLINE int usem_timed_wait(usem_t *sem, uint64_t timeout_usecs) {
	mach_timespec_t ts;
	ts.tv_sec = (unsigned int) (timeout_usecs / 1000000);
	ts.tv_nsec = (timeout_usecs % 1000000) * 1000;

	// added in OSX 10.10: https://developer.apple.com/library/prerelease/mac/documentation/General/Reference/APIDiffsMacOSX10_10SeedDiff/modules/Darwin.html
	return semaphore_timedwait(*sem, ts);
}

USEM_INLINE int usem_try_wait(usem_t *sem) {
	return usem_timed_wait(sem, 0);
}

USEM_INLINE void usem_signal(usem_t *sem) {
	while (semaphore_signal(*sem) != KERN_SUCCESS);
}

USEM_INLINE void usem_signal_count(usem_t *sem, int count) {
	while (count-- > 0)
	{
		while (semaphore_signal(*sem) != KERN_SUCCESS);
	}
}

#elif defined(__unix__)
//---------------------------------------------------------
// Semaphore (POSIX, Linux)
//---------------------------------------------------------

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
	// sem_timedwait bombs if you have more than 1e9 in tv_nsec
	// so we have to clean things up before passing it in
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
