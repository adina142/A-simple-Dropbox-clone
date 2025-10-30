#ifndef TASK_H
#define TASK_H

#include <pthread.h>
#include <stddef.h>

typedef struct {
    int client_socket;            // socket descriptor (owned by client thread)
    char command[32];             // "UPLOAD","DOWNLOAD","LIST","DELETE"
    char filename[128];
    char username[64];

    /* Upload buffer (filled by client thread when command==UPLOAD) */
    char* upload_buf;
    size_t upload_len;

    /* Worker result (filled by worker, consumed and freed by client thread) */
    char* result_buf;
    size_t result_len;

    /* Synchronization for worker -> client signaling */
    pthread_mutex_t done_lock;
    pthread_cond_t done_cond;
    int done; /* 0 = not done, 1 = done */
} Task;

/* Worker entrypoint: consumes Task*, fills result_buf/result_len, sets done */
void handle_task(void* arg);

#endif
