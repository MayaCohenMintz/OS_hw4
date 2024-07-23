// queue.c

#include <stdlib.h>
#include <stdatomic.h>
#include <threads.h>
#include <stdbool.h>
#include <assert.h>

// Define the node structure for the queue
typedef struct Node {
    void* data;
    struct Node* next;
} Node;

// Define the queue structure
typedef struct Queue {
    Node* front;
    Node* rear;
    mtx_t mutex;
    cnd_t cond;
    atomic_size_t size;
    atomic_size_t waiting;
    atomic_size_t visited;
} Queue;

// Global queue instance
static Queue queue;

// Initialize the queue
void initQueue(void) {
    queue.front = NULL;
    queue.rear = NULL;
    mtx_init(&queue.mutex, mtx_plain);
    cnd_init(&queue.cond);
    atomic_init(&queue.size, 0);
    atomic_init(&queue.waiting, 0);
    atomic_init(&queue.visited, 0);
}

// Destroy the queue and clean up resources
void destroyQueue(void) {
    mtx_lock(&queue.mutex);
    while (queue.front != NULL) {
        Node* temp = queue.front;
        queue.front = queue.front->next;
        free(temp);
    }
    queue.rear = NULL;
    mtx_unlock(&queue.mutex);
    mtx_destroy(&queue.mutex);
    cnd_destroy(&queue.cond);
}

// Add an item to the queue
void enqueue(void* item) {
    Node* newNode = malloc(sizeof(Node));
    assert(newNode != NULL); // Assume malloc does not fail
    newNode->data = item;
    newNode->next = NULL;

    mtx_lock(&queue.mutex);

    if (queue.rear == NULL) {
        queue.front = newNode;
    } else {
        queue.rear->next = newNode;
    }
    queue.rear = newNode;

    atomic_fetch_add(&queue.size, 1);

    // Wake up one waiting thread
    cnd_signal(&queue.cond);
    mtx_unlock(&queue.mutex);
}

// Remove an item from the queue, blocking if empty
void* dequeue(void) {
    mtx_lock(&queue.mutex);
    while (queue.front == NULL) {
        atomic_fetch_add(&queue.waiting, 1);
        cnd_wait(&queue.cond, &queue.mutex);
        atomic_fetch_sub(&queue.waiting, 1);
    }

    Node* temp = queue.front;
    void* item = temp->data;
    queue.front = queue.front->next;
    if (queue.front == NULL) {
        queue.rear = NULL;
    }

    atomic_fetch_add(&queue.visited, 1);
    atomic_fetch_sub(&queue.size, 1);

    free(temp);
    mtx_unlock(&queue.mutex);
    return item;
}

// Attempt to remove an item from the queue without blocking
bool tryDequeue(void** item) {
    mtx_lock(&queue.mutex);
    if (queue.front == NULL) {
        mtx_unlock(&queue.mutex);
        return false;
    }

    Node* temp = queue.front;
    *item = temp->data;
    queue.front = queue.front->next;
    if (queue.front == NULL) {
        queue.rear = NULL;
    }

    atomic_fetch_add(&queue.visited, 1);
    atomic_fetch_sub(&queue.size, 1);

    free(temp);
    mtx_unlock(&queue.mutex);
    return true;
}

// Return the current number of items in the queue
size_t size(void) {
    return atomic_load(&queue.size);
}

// Return the number of threads waiting for the queue to fill
size_t waiting(void) {
    return atomic_load(&queue.waiting);
}

// Return the number of items that have been inserted and removed
size_t visited(void) {
    return atomic_load(&queue.visited);
}

/*
 * Documentation:
 * This library implements a concurrent FIFO queue using C11 threads.
 * 
 * Functions:
 * - void initQueue(void): Initializes the queue.
 * - void destroyQueue(void): Cleans up resources used by the queue.
 * - void enqueue(void* item): Adds an item to the queue.
 * - void* dequeue(void): Removes an item from the queue, blocking if the queue is empty.
 * - bool tryDequeue(void** item): Attempts to remove an item from the queue without blocking.
 * - size_t size(void): Returns the current number of items in the queue.
 * - size_t waiting(void): Returns the number of threads waiting for the queue to fill.
 * - size_t visited(void): Returns the number of items that have been inserted and removed.
 * 
 * Thread Safety:
 * The queue is thread-safe and ensures that threads operate in parallel with proper synchronization.
 * 
 * Assumptions:
 * - The malloc function does not fail.
 * - No need to check for errors in calls to mtx_* and cnd_* functions.
 */

