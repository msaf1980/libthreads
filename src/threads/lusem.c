#include <threads/lusem.h>

static int lusem_wait_with_part_spin(lusem_t *lsem, uint64_t timeout_usecs) {
    ssize_t old_count;
    int spin = lsem->max_spins;
    while (--spin >= 0)
    {
        old_count = __atomic_fetch_add(&lsem->m_count, 0, __ATOMIC_RELAXED);
        if ((old_count > 0) && __atomic_compare_exchange_n (&lsem->m_count, &old_count, old_count - 1, 0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
            return 0;
        __atomic_signal_fence(__ATOMIC_ACQUIRE); /* Prevent the compiler from collapsing the loop. */
    }
    old_count = __atomic_fetch_sub(&lsem->m_count, 1, __ATOMIC_ACQUIRE);
    if (old_count > 0)
        return 0;
    if (timeout_usecs == 0)
    {
        if (usem_wait(&lsem->sem) == 0)
            return 0;
    } else {
        if (usem_timed_wait(&lsem->sem, timeout_usecs) == 0)
            return 0;
    }
    /*
    * At this point, we've timed out waiting for the semaphore, but the
    * count is still decremented indicating we may still be waiting on
    * it. So we have to re-adjust the count, but only if the semaphore
    * wasn't signaled enough times for us too since then. If it was, we
    * need to release the semaphore too.
    */
    while (1)
    {
        old_count = __atomic_fetch_add(&lsem->m_count, 0, __ATOMIC_ACQUIRE);
        if (old_count >= 0 && usem_try_wait(&lsem->sem) == 0)
            return 0;
        if (old_count < 0 && __atomic_compare_exchange_n (&lsem->m_count, &old_count, old_count + 1, 0, __ATOMIC_RELAXED, __ATOMIC_RELAXED))
            return -1;
    }
}

int lusem_wait(lusem_t *lsem) {
    if (lusem_try_wait(lsem) == 0) {
        return 0;
    }
    return lusem_wait_with_part_spin(lsem, 0);
}

int lusem_timed_wait(lusem_t *lsem, uint64_t timeout_usecs) {
    if (lusem_try_wait(lsem) == 0) {
        return 0;
    }
	return lusem_wait_with_part_spin(lsem, timeout_usecs);
}

// USEM_INLINE int usem_timed_wait(usem_t *sem, uint64_t timeout_usecs) {
// 	int rc;

// 	struct timespec ts;
// 	static const uint64_t usecs_in_1_sec = 1000000;
// 	static const long nsecs_in_1_sec = 1000000000;
// 	clock_gettime(CLOCK_REALTIME, &ts);
// 	ts.tv_sec += (time_t)(timeout_usecs / usecs_in_1_sec);
// 	ts.tv_nsec += (long)(timeout_usecs % usecs_in_1_sec) * 1000;
// 	// sem_timedwait bombs if you have more than 1e9 in tv_nsec
// 	// so we have to clean things up before passing it in
// 	if (ts.tv_nsec >= nsecs_in_1_sec) {
// 		ts.tv_nsec -= nsecs_in_1_sec;
// 		++ts.tv_sec;
// 	}

// 	do {
// 		rc = sem_timedwait(sem, &ts);
// 	} while (rc == -1 && errno == EINTR);
// 	return rc;
// }
