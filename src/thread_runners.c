#include "thread_runners.h"

#include <pthread.h>
#include <stdlib.h>

#include "downloading.h"

void enqueue(queue* queue, uint32_t id, uint32_t value) {
    ll* new = malloc(sizeof(ll));
    new->val = malloc(sizeof(uint32_t));
    *(uint32_t*)new->val = id * 100 + value;
    new->next = nullptr;

    pthread_mutex_lock(&queue->lock);

    if (queue->tail == NULL) {
        queue->head = queue->tail = new;
    } else {
        queue->tail->next = new;
        queue->tail = new;
    }

    pthread_cond_signal(&queue->condition);
    pthread_mutex_unlock(&queue->lock);

    printf("Producer %d produced %d\n", id, *(uint32_t*)new->val);
}

uint32_t dequeue(queue* queue) {
    pthread_mutex_lock(&queue->lock);

    while (queue->head == NULL) {
        pthread_cond_wait(&queue->condition, &queue->lock);
    }

    ll* temp = queue->head;
    uint32_t value = *(uint32_t*)temp->val;

    queue->head = queue->head->next;
    if (queue->head == NULL)
        queue->tail = nullptr;

    pthread_mutex_unlock(&queue->lock);

    free(temp);
    return value;
}

void *torrent_runner(void *arg) {
    const torrent_args_t* torrent_args = arg;
    queue* queue = torrent_args->queue;

    for (int i = 0; i < 5; i++) {
        enqueue(queue, torrent_args->thread_id, i);
    }


    //torrent(*torrent_args->metainfo, torrent_args->peer_id, torrent_args->log_code);
    return nullptr;
}

void *disk_runner(void *arg) {
    disk_args_t* disk_args = arg;
    while (true) {
        dequeue(disk_args->queue);
    }
    return nullptr;
}
