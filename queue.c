#include "queue.h"
#include <stdlib.h>

void queue_init(Queue* q) {
    q->front = q->rear = NULL;
    q->closed = 0;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->cond, NULL);
}

void queue_push(Queue* q, void* data) {
    Node* n = malloc(sizeof(Node));
    n->data = data;
    n->next = NULL;

    pthread_mutex_lock(&q->lock);
    if (q->closed) {
        pthread_mutex_unlock(&q->lock);
        free(n);
        return;
    }
    if (q->rear)
        q->rear->next = n;
    else
        q->front = n;
    q->rear = n;
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->lock);
}

void* queue_pop(Queue* q) {
    pthread_mutex_lock(&q->lock);
    while (q->front == NULL && !q->closed)
        pthread_cond_wait(&q->cond, &q->lock);

    if (q->front == NULL && q->closed) {
        pthread_mutex_unlock(&q->lock);
        return NULL;
    }

    Node* temp = q->front;
    q->front = q->front->next;
    if (q->front == NULL) q->rear = NULL;

    void* data = temp->data;
    free(temp);
    pthread_mutex_unlock(&q->lock);
    return data;
}

void queue_close(Queue* q) {
    pthread_mutex_lock(&q->lock);
    q->closed = 1;
    pthread_cond_broadcast(&q->cond);
    pthread_mutex_unlock(&q->lock);
}

void queue_destroy(Queue* q) {
    /* free any remaining nodes */
    Node* cur = q->front;
    while (cur) {
        Node* nxt = cur->next;
        free(cur);
        cur = nxt;
    }
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->cond);
}
