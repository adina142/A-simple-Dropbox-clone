#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>

typedef struct Node {
    void* data;
    struct Node* next;
} Node;

typedef struct {
    Node* front, * rear;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    int closed; /* optional: signal closure */
} Queue;

void queue_init(Queue* q);
void queue_push(Queue* q, void* data);
void* queue_pop(Queue* q);
void queue_destroy(Queue* q);
void queue_close(Queue* q);

#endif
