// queue.c - adding specification that threads are woken up in FIFO order. 

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

// Define the thread node structure for keeping track of waiting threads
typedef struct ThreadNode {
    cnd_t cond; // every thread has a cond associated with it
    struct ThreadNode* next;
} ThreadNode;

// Define the queue structure
typedef struct Queue {
    Node* front;
    Node* rear;
    mtx_t mutex; // note that each queue requires only one mutex, but number of conds = number of threads
    // associated with the queue
    ThreadNode* waiting_front;
    ThreadNode* waiting_rear; // MAYA why do we need one for rear?
    atomic_size_t size;
    atomic_size_t waiting;
    atomic_size_t visited;
} Queue;

// Global queue instance
// MAYA - why a global instance? Does that mean that there can be only one queue at any given time?
static Queue queue;

// Initialize the queue
void initQueue(void) {
    queue.front = NULL;
    queue.rear = NULL;
    queue.waiting_front = NULL; 
    queue.waiting_rear = NULL;
    mtx_init(&queue.mutex, mtx_plain);
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
    while (queue.waiting_front != NULL) { 
        ThreadNode* temp = queue.waiting_front;
        queue.waiting_front = queue.waiting_front->next;
        cnd_destroy(&temp->cond);
        free(temp);
    }
    queue.rear = NULL;
    queue.waiting_rear = NULL;
    mtx_unlock(&queue.mutex);
    mtx_destroy(&queue.mutex);
}

// Add an item to the queue
void enqueue(void* item) {
    Node* newNode = malloc(sizeof(Node));
    assert(newNode != NULL); // Assume malloc does not fail
    newNode->data = item;
    newNode->next = NULL;

    mtx_lock(&queue.mutex);

    if (queue.rear == NULL) { // if queue was empty, the new node is both the front and the rear
        queue.front = newNode;
    } else {
        queue.rear->next = newNode;
    }
    queue.rear = newNode; // note that new node is always the rear

    atomic_fetch_add(&queue.size, 1); // incrementing queue size

    // Wake up the first waiting thread (if any)
    if (queue.waiting_front != NULL) { // if there is at least one sleeping thread waiting to be woken up by an enqueue operation
        ThreadNode* waitingThread = queue.waiting_front; // this is the oldest thread, i.e. the one that should be used
        // now according to FIFO order
        queue.waiting_front = queue.waiting_front->next;
        if (queue.waiting_front == NULL) { // if there are now no threads waiting, update rear
            queue.waiting_rear = NULL;
        }
        cnd_signal(&waitingThread->cond); // signal the thread to be woken up using its specific cond var 
        free(waitingThread); // MAYA why do we need to free this thread?
    }

    mtx_unlock(&queue.mutex);
}

// Remove an item from the queue, blocking if empty 
void* dequeue(void) {
    void* item;

    mtx_lock(&queue.mutex);
    while (queue.front == NULL) { // if queue is empty
        // Add this thread to the waiting list
        ThreadNode* newThread = malloc(sizeof(ThreadNode));
        assert(newThread != NULL); // Assume malloc does not fail
        cnd_init(&newThread->cond);
        newThread->next = NULL;
        
        if (queue.waiting_rear == NULL) { // if this is the only thread, make it both front and rear
            queue.waiting_front = newThread;
        } else {
            queue.waiting_rear->next = newThread;
        }
        queue.waiting_rear = newThread;
        atomic_fetch_add(&queue.waiting, 1);
        
        // Wait for an item to be added to the queue (this is the "blocking")
        cnd_wait(&newThread->cond, &queue.mutex);
        atomic_fetch_sub(&queue.waiting, 1);
        // MAYA problem: why destroy the cond and thread?
        // MAYA problem: how do we make sure the mutex is passed straight to the correct thread?
        cnd_destroy(&newThread->cond);
        free(newThread);
    }

    Node* temp = queue.front;
    item = temp->data;
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

