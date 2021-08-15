#ifndef _lfthpool_
#define _lfthpool_

#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>

/**
 * @file
*
* Public header
*/

/* =================================== API ======================================= */

/**
 * @typedef lfthpool_t
 * @brief   Thread pool
 */
typedef struct lfthpool* lfthpool_t;

/**
 * @brief  Creates a pool of worker lfthpool for later use
 * @param  workers           Workers count (if < 1, hostcpu count is used).
 * @param  queue_size        Maximum lenght of job queue for workers to take work from.
 * @retval                   Returns a pointer to an initialised threadpool on
 *                           success or NULL on error (error code stored in errno).
 */
lfthpool_t lfthpool_create(size_t workers, size_t queue_size);

/**
 * @brief  Creates a pool of worker lfthpool for later use
 * @param  workers           Workers count (if < 1, hostcpu count is used).
 * @param  queue_size        Maximum lenght of job queue for workers to take work from.
 * @param  sleep_func        Sleep function (integrated with your scheduler). If NULL, sched_yield is used.
 * @retval                   Returns a pointer to an initialised threadpool on
 *                           success or NULL on error (error code stored in errno).
 */
lfthpool_t lfthpool_create_sched(size_t workers, size_t queue_size, int (*sleep_func)(useconds_t));

/**
 * @brief  Count of workers lfthpool in thread poool
 * @param  pool            Threadpool
 */
size_t lfthpool_workers_count(lfthpool_t pool);

/**
 * @brief   Add a task to a thread pool
 * @param	pool			Threadpool to add task to.
 * @param	function	Function/task for worker to execute.
 * @param	arg				Arguments to function/task.
 * @retval					Returns 0 on success and -1 on error.
 */
int lfthpool_add_task(lfthpool_t pool, void (*function)(void *), void* arg);

/**
 * @brief   Add a task to a thread pool
 * @param	pool			Threadpool to add task to.
 * @param	function	Function/task for worker to execute.
 * @param	arg				Arguments to function/task.
 * @param usec      Sleep (millisec).
 * @param max_try   Try count (if queue is full).
 * @retval					Returns 0 on success, -1 or QERR_* on error.
 */
int lfthpool_add_task_try(lfthpool_t pool, void (*function)(void *), void* arg, useconds_t usec, int max_try);

/**
 * @brief  Pause tasks process in thread poool
 * @param  pool            Threadpool
 */
void lfthpool_pause(lfthpool_t pool);

/**
 * @brief  Resume tasks process in thread poool
 * @param  pool            Threadpool
 */
void lfthpool_resume(lfthpool_t pool);

/**
 * @brief  Count of active tasks (lfthpool in thread poool, that processing task)
 * @param  pool            Threadpool
 */
size_t lfthpool_active_tasks(lfthpool_t pool);

/**
 * @brief  Total count of tasks (queued and active)
 * @param  pool            Threadpool
 */
size_t lfthpool_total_tasks(lfthpool_t pool);

/**
 * @brief  Wait for process all tasks in thread poool
 * @param  pool            Threadpool
 */

/**
 * @brief  Process one task from thread poool queue (also pause or shutdown for pool is ignored)
 * @param  pool            Threadpool
 * @retval 0 - on success, -1 - no task
 */
int lfthpool_worker_try_once(lfthpool_t pool);

/**
 * @brief  Process one task from thread poool queue (also pause or shutdown for pool is ignored)
 * 
 * Can return when task dequeue/increment active task count (so some tasks prepared to execute)
 * @param  pool            Threadpool
  */
void lfthpool_wait(lfthpool_t pool);

/**
 * @brief  Shutdown thread poool
 * @param  pool            Threadpool
 */
void lfthpool_shutdown(lfthpool_t pool);

/**
 * @brief  Shutdown and destroy thread poool
 * @param  pool            Threadpool
 */
void lfthpool_destroy(lfthpool_t pool);

#ifdef __cplusplus
}
#endif

#endif /* _lfthpool_ */
