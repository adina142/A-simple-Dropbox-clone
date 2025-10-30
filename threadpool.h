#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>
#include "queue.h"

typedef struct {
    pthread_t* threads;
    int size;
    Queue* task_queue;
    void (*worker_func)(void*); /* function each worker thread calls with popped task */
    int stopped;
} ThreadPool;

/* Initialize threadpool:
   pool: pointer
   size: number of threads
   task_queue: queue to pop tasks from
   worker_func: function called with void* popped from queue
*/
void threadpool_init(ThreadPool* pool, int size, Queue* task_queue, void (*worker_func)(void*));
void threadpool_shutdown(ThreadPool* pool); /* signal threads to stop and join them */
void threadpool_destroy(ThreadPool* pool);  /* free thread array */

#endif
