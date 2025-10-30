#include "threadpool.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

/* worker loop: pop item from queue and call pool->worker_func(item) */
static void* threadpool_worker(void* arg) {
    ThreadPool* pool = (ThreadPool*)arg;
    while (1) {
        if (pool->stopped) break;

        void* item = queue_pop(pool->task_queue);
        if (item == NULL) {
            /* queue closed and empty -> time to exit */
            break;
        }

        /* call the provided worker function */
        pool->worker_func(item);
        /* loop back for next item */
    }
    return NULL;
}

void threadpool_init(ThreadPool* pool, int size, Queue* task_queue, void (*worker_func)(void*)) {
    pool->size = size;
    pool->task_queue = task_queue;
    pool->worker_func = worker_func;
    pool->stopped = 0;
    pool->threads = malloc(sizeof(pthread_t) * size);
    if (!pool->threads) {
        perror("threadpool malloc");
        exit(1);
    }

    for (int i = 0; i < size; ++i) {
        if (pthread_create(&pool->threads[i], NULL, threadpool_worker, pool) != 0) {
            perror("pthread_create");
            /* keep going so other threads may start */
        }
    }
}

void threadpool_shutdown(ThreadPool* pool) {
    /* mark stopped and close the queue to wake workers */
    pool->stopped = 1;
    queue_close(pool->task_queue);

    for (int i = 0; i < pool->size; ++i) {
        pthread_join(pool->threads[i], NULL);
    }
}

void threadpool_destroy(ThreadPool* pool) {
    if (pool->threads) {
        free(pool->threads);
        pool->threads = NULL;
    }
}
