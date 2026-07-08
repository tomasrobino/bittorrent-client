#include "thread_runners.h"

#include <pthread.h>
#include <stdlib.h>

#include "downloading.h"

/**
 * Puts element in queue
 * @param queue
 * @param id
 * @param value
 */
void enqueue(queue_t* queue, uint32_t id, uint32_t value) {
    ll_queue_member_t* new = malloc(sizeof(ll_queue_member_t));
    new->torrent_id = id;
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

    printf("Producer %d produced %d\n", id, new->torrent_id);
}

/**
 * Removes element from queue
 * @param queue
 * @return
 */
uint32_t dequeue(queue_t* queue) {
    pthread_mutex_lock(&queue->lock);

    while (queue->head == NULL) {
        pthread_cond_wait(&queue->condition, &queue->lock);
    }

    ll_queue_member_t* temp = queue->head;
    uint32_t value = temp->torrent_id;

    queue->head = queue->head->next;
    if (queue->head == nullptr)
        queue->tail = nullptr;

    pthread_mutex_unlock(&queue->lock);

    free(temp);
    return value;
}


/**
 * Thread for each concurrent torrent, when it has something to save to disk, calls enqueue() which puts it in the queue
 * @param arg
 * @return
 */
void *torrent_runner(void *arg) {
    const torrent_args_t* torrent_args = arg;
    queue_t* queue = torrent_args->queue;

    for (int i = 0; i < 5; i++) {
        enqueue(queue, torrent_args->thread_id, i);
    }


    //torrent(*torrent_args->metainfo, torrent_args->peer_id, torrent_args->log_code);
    return nullptr;
}


/**
 * Thread for saving to disk. reads queue elements, saves them to disk, and calls dequeue()
 * @param arg
 * @return
 */
void *disk_runner(void *arg) {
    disk_args_t* disk_args = arg;
    while (true) {
        dequeue(disk_args->queue);
    }
    return nullptr;
}
