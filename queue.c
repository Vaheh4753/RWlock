#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>

#include "queue.h"

struct queue {
    void **element;
    int size;
    int first;
    int last;
    sem_t empty;
    sem_t full;
    sem_t mutex;
};

queue_t *queue_new(int size) {
    queue_t *q = (queue_t *) malloc(sizeof(queue_t));
    q->element = (void **) malloc(sizeof(void *) * size);
    q->size = size;
    q->first = 0;
    q->last = 0;
    sem_init(&q->empty, 0, 0);
    sem_init(&q->full, 0, size);
    sem_init(&q->mutex, 0, 1);
    return q;
}

void queue_delete(queue_t **q) {
    free((*q)->element);
    free(*q);
    sem_destroy(&(*q)->empty);
    sem_destroy(&(*q)->full);
    sem_destroy(&(*q)->mutex);
    *q = NULL;
    return;
}

bool queue_push(queue_t *q, void *elem) {
    if (q == NULL)
        return false;
    sem_wait(&q->full);
    sem_wait(&q->mutex);
    q->element[q->last] = elem;
    q->last = (q->last + 1) % q->size;
    sem_post(&q->mutex);
    sem_post(&q->empty);
    return true;
}

bool queue_pop(queue_t *q, void **elem) {
    if (q == NULL)
        return false;
    sem_wait(&q->empty);
    sem_wait(&q->mutex);
    *elem = q->element[q->first];
    q->first = (q->first + 1) % q->size;
    sem_post(&q->mutex);
    sem_post(&q->full);
    return true;
}
