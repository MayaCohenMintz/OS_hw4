//// My Impressionist Friend, first try


#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include "queue.h"

typedef struct Node {
    void* data;
    struct Node* next;
    bool is_thread;
    cnd_t cond;
} Node;

typedef struct {
    Node* front;
    Node* rear;
    mtx_t mutex;
    size_t size;
    size_t waiting;
    size_t visited;
} Queue;

static Queue queue;

void initQueue(void) {
    queue.front = NULL;
    queue.rear = NULL;
    mtx_init(&queue.mutex, mtx_plain);
    queue.size = 0;
    queue.waiting = 0;
    queue.visited = 0;
}

void destroyQueue(void) {
    mtx_lock(&queue.mutex);
    while (queue.front) {
        Node* temp = queue.front;
        queue.front = queue.front->next;
        if (temp->is_thread) {
            cnd_destroy(&temp->cond);
        }
        free(temp);
    }
    queue.rear = NULL;
    queue.size = 0;
    queue.waiting = 0;
    queue.visited = 0;
    mtx_unlock(&queue.mutex);
    mtx_destroy(&queue.mutex);
}

void enqueue(void* data) {
    mtx_lock(&queue.mutex);
    if (queue.front && queue.front->is_thread) {
        Node* thread_node = queue.front;
        queue.front = queue.front->next;
        if (!queue.front) queue.rear = NULL;
        queue.waiting--;
        thread_node->data = data;
        cnd_signal(&thread_node->cond);
    } else {
        Node* new_node = malloc(sizeof(Node));
        new_node->data = data;
        new_node->next = NULL;
        new_node->is_thread = false;
        if (queue.rear) {
            queue.rear->next = new_node;
        } else {
            queue.front = new_node;
        }
        queue.rear = new_node;
        queue.size++;
    }
    mtx_unlock(&queue.mutex);
}

void* dequeue(void) {
    mtx_lock(&queue.mutex);
    void* result;
    if (queue.front && !queue.front->is_thread) {
        Node* temp = queue.front;
        queue.front = queue.front->next;
        if (!queue.front) queue.rear = NULL;
        queue.size--;
        queue.visited++;
        result = temp->data;
        free(temp);
    } else {
        Node* thread_node = malloc(sizeof(Node));
        thread_node->is_thread = true;
        thread_node->next = NULL;
        cnd_init(&thread_node->cond);
        if (queue.rear) {
            queue.rear->next = thread_node;
        } else {
            queue.front = thread_node;
        }
        queue.rear = thread_node;
        queue.waiting++;
        cnd_wait(&thread_node->cond, &queue.mutex);
        result = thread_node->data;
        free(thread_node);
    }
    mtx_unlock(&queue.mutex);
    return result;
}

bool tryDequeue(void** result) {
    mtx_lock(&queue.mutex);
    if (queue.front && !queue.front->is_thread) {
        Node* temp = queue.front;
        queue.front = queue.front->next;
        if (!queue.front) queue.rear = NULL;
        queue.size--;
        queue.visited++;
        *result = temp->data;
        free(temp);
        mtx_unlock(&queue.mutex);
        return true;
    }
    mtx_unlock(&queue.mutex);
    return false;
}

size_t size(void) {
    return queue.size;
}

size_t waiting(void) {
    return queue.waiting;
}

size_t visited(void) {
    return queue.visited;
}