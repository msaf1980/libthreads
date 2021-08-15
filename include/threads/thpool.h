#ifndef _THPOOL_
#define _THPOOL_

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
 * @typedef thpool_t
 * @brief   Thread pool
 */
typedef struct thpool* thpool_t;

/**
 * @brief  Creates a pool of worker thpool for later use
 * @param  workers           Workers count (if < 1, hostcpu count is used).
 * @param  queue_size        Maximum lenght of job queue for workers to take work from.
 * @retval                   Returns a pointer to an initialised threadpool on
 *                           success or NULL on error (error code stored in errno).
 */
thpool_t thpool_create(size_t workers, size_t queue_size);

/**
 * @brief  Count of workers thpool in thread poool
 * @param  pool            Threadpool
 */
size_t thpool_workers_count(thpool_t pool);

/**
 * @brief   Add a task to a thread pool (no memory allocation, task reused from static queue)
 * @param	pool			Threadpool to add task to.
 * @param	function		Function/task for worker to execute.
 * @param	arg				Arguments to function/task.
 * @retval					Returns 0 on success and -1 on error.
 */
int thpool_add_task(thpool_t pool, void (*function)(void *), void* arg);

/**
 * @brief   Add a task to a thread pool (no memory allocation, task reused from static queue)
 * @param	pool      Threadpool to add task to.
 * @param	function  Function/task for worker to execute.
 * @param	arg		  Arguments to function/task.
 * @param   usec      Sleep (millisec).
 * @param   max_try   Try count (if queue is full).
 * @retval			  Returns 0 on success, -1 or QERR_* on error.
 */
int thpool_add_task_try(thpool_t pool, void (*function)(void *), void* arg, useconds_t usec, int max_try);

/**
 * @brief  Pause tasks process in thread poool
 * @param  pool            Threadpool
 */
void thpool_pause(thpool_t pool);

/**
 * @brief  Resume tasks process in thread poool
 * @param  pool            Threadpool
 */
void thpool_resume(thpool_t pool);

/**
 * @brief  Count of active tasks (thpool in thread poool, that processing task)
 * @param  pool            Threadpool
 */
size_t thpool_active_tasks(thpool_t pool);

/**
 * @brief  Total count of tasks (queued and active)
 * @param  pool            Threadpool
 */
size_t thpool_total_tasks(thpool_t pool);

/**
 * @brief  Wait for process all tasks in thread poool
 * @param  pool            Threadpool
 */

/**
 * @brief  Process one task from thread poool queue (also pause or shutdown for pool is ignored)
 * @param  pool            Threadpool
 * @retval 0 - on success, -1 - no task
 */
int thpool_worker_try_once(thpool_t pool);

void thpool_wait(thpool_t pool);

/**
 * @brief  Shutdown thread poool
 * @param  pool            Threadpool
 */
void thpool_shutdown(thpool_t pool);

/**
 * @brief  Shutdown and destroy thread poool
 * @param  pool            Threadpool
 */
void thpool_destroy(thpool_t pool);

#ifdef __cplusplus
}
#endif

#endif /* _THPOOL_ */
