#ifndef _THREADS_LUSEM_H_
#define _THREADS_LUSEM_H_

#include <sys/types.h>
#include <errno.h>
#include <stdint.h>
#include <time.h>

/* Unnamed lightweight semaphore */

#include <threads/usem.h>

#define LUSEM_INLINE static inline

typedef struct lusem {
    ssize_t m_count;
	usem_t sem;
	int max_spins;
} lusem_t;

LUSEM_INLINE int lusem_init(lusem_t *lsem, unsigned int initial_count, int max_spins) {
    lsem->max_spins = max_spins;
    lsem->m_count = initial_count;
	return usem_init(&lsem->sem, initial_count);
}

LUSEM_INLINE int lusem_destroy(lusem_t *lsem) {
	return usem_destroy(&lsem->sem);
}

LUSEM_INLINE int lusem_try_wait(lusem_t *lsem) {
    ssize_t old_count = __atomic_fetch_add(&lsem->m_count, 0, __ATOMIC_RELAXED);
    while (old_count > 0)
    {
        if (__atomic_compare_exchange_n (&lsem->m_count, &old_count, old_count - 1, 1, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
            return 0;
    }
    return -1;
}

int lusem_wait(lusem_t *lsem);

int lusem_timed_wait(lusem_t *lsem, uint64_t timeout_usecs);

LUSEM_INLINE void lusem_signal_count(lusem_t *lsem, ssize_t count) {
	if (count > 0) {
		ssize_t old_count = __atomic_fetch_add(&lsem->m_count, count, __ATOMIC_RELEASE);
		ssize_t to_release = -old_count < count ? -old_count : count;
		if (to_release > 0)
			usem_signal_count(&lsem->sem, to_release);
	}
}

LUSEM_INLINE void lusem_signal(lusem_t *lsem) {
	ssize_t old_count = __atomic_fetch_add(&lsem->m_count, 1, __ATOMIC_RELEASE);
	ssize_t to_release = -old_count < 1 ? -old_count : 1;
	if (to_release > 0)
		usem_signal(&lsem->sem);
}

#undef LUSEM_INLINE

#endif /* _THREADS_LUSEM_H_ */
