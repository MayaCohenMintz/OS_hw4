#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <threads.h>

#include "queue.h"

/* MACROS */

// Allocates a zero-ed buffer of n elements from pointer p on the heap, casts the return value to the pointer's type
#define HEAPALLOCZ(p, n) calloc((n), sizeof(*p))

// Safely frees a buffer allocated on the heap 
#define HEAPFREE(p)					\
{									\
	if (NULL != (p))			    \
	{								\
		(void)free(p);	            \
		(p) = NULL;					\
	}								\
}

/* TYPEDEFS */
// item queue
typedef struct _ITEM // a single queue item (node)
{
    void* data;             // the data of the item
	struct _ITEM* next;		// the next item in the queue
} ITEM;
typedef ITEM* PITEM; 
typedef struct _IQUEUE // the item queue
{
    PITEM head;
    PITEM tail;
    size_t size;
    size_t visited;  // number of items that have passed inside the queue
    mtx_t mutex;     // a mutex for syncing access to queue
} IQUEUE;
typedef IQUEUE* PIQUEUE;

// thread queue
typedef struct _THREAD // a single thread data (node)
{
    cnd_t cv;               // a condition variable
    void* data;             // the data connected to this thread
	struct _THREAD* next;   // the next thread in the queue
} THREAD;
typedef THREAD* PTHREAD; 
typedef struct _TQUEUE // the thread queue
{
    PTHREAD head;
    PTHREAD tail;
    size_t waiting; // number of threads waiting for item queue to fill
} TQUEUE;
typedef TQUEUE* PTQUEUE; 

/* GLOBALS */
static IQUEUE iqueue = {};
static TQUEUE tqueue = {};

/* FUNCTIONS */
PITEM create_item(void* data); // creates a new item node with the given data
void free_item(PITEM item); // frees the memory for a single item
void add_item(PIQUEUE iqueue, PITEM item); // adds to queue tail
PITEM pop_item(PIQUEUE iqueue); // pops the queue head
void free_items(PITEM head); // frees the linked list of items

PTHREAD create_thread(); // creates a new thread node
void free_thread(PTHREAD thread); // frees the memory for a single item
void add_thread(PTQUEUE tqueue, PTHREAD thread); // adds to queue tail
PTHREAD pop_thread(PTQUEUE tqueue); // pops the queue head
void free_threads(PTHREAD head); // frees he linked list of threads

/* HELPER FUNCTIONS */
PITEM create_item(void* data)
{
    PITEM item = NULL;
    item = (PITEM)HEAPALLOCZ(item, 1);
    item->data = data;
    item->next = NULL;
    return item;
}

void free_item(PITEM item)
{
    HEAPFREE(item);
}

void add_item(PIQUEUE iqueue, PITEM item)
{
    if (0 == iqueue->size) // first item in the queue
    {
        iqueue->head = item;
        iqueue->tail = item;
    }
    else
    {
        iqueue->tail->next = item;
        iqueue->tail = item;
    }
    iqueue->size++;
}

PITEM pop_item(PIQUEUE iqueue)
{
    PITEM item = NULL;

    item = iqueue->head; // get the head
    iqueue->head = item->next; // update head
    if (NULL == iqueue->head)
        iqueue->tail = NULL;

    iqueue->size--;
    iqueue->visited++;
    return item;
}

void free_items(PITEM head)
{
    if (NULL != head)
    {
        free_items(head->next);
        free_item(head);
    }
}

PTHREAD create_thread()
{
    PTHREAD thread = NULL;
    thread = (PTHREAD)HEAPALLOCZ(thread, 1);
    cnd_init(&(thread->cv));
    return thread;
}

void free_thread(PTHREAD thread)
{
    cnd_destroy(&(thread->cv));
    HEAPFREE(thread);
}

void add_thread(PTQUEUE tqueue, PTHREAD thread)
{
    if (0 == tqueue->waiting) // first item in the queue
    {
        tqueue->head = thread;
        tqueue->tail = thread;
    }
    else
    {
        tqueue->tail->next = thread;
        tqueue->tail = thread;
    }
    tqueue->waiting++;
}

PTHREAD pop_thread(PTQUEUE tqueue)
{
    PTHREAD thread = NULL;

    thread = tqueue->head; // get the head
    tqueue->head = thread->next; // update head
    if (NULL == tqueue->head)
        tqueue->tail = NULL;

    tqueue->waiting--;
    return thread;
}

void free_threads(PTHREAD head)
{
    if (NULL != head)
    {
        free_threads(head->next);
        free_thread(head);
    }
}

/* QUEUE FUNCTIONS */
void initQueue(void)
{
    // initialize thread queue
    tqueue.head = NULL;
    tqueue.tail = NULL;
    tqueue.waiting = 0;
    // initialize item queue
    iqueue.head = NULL;
    iqueue.tail = NULL;
    iqueue.size = 0;
    iqueue.visited = 0;
    (void)mtx_init(&iqueue.mutex, mtx_plain);
}

void destroyQueue(void)
{
    mtx_lock(&iqueue.mutex); // lock
    free_items(iqueue.head);
    iqueue.tail = NULL;
    iqueue.size = 0;
    iqueue.visited = 0;
    free_threads(tqueue.head);
    tqueue.tail = NULL;
    tqueue.waiting = 0;
    mtx_unlock(&iqueue.mutex); // unlock
    mtx_destroy(&iqueue.mutex); // destroy the mutex itself
}

void enqueue(void* data)
{
    PITEM item = NULL;
    PTHREAD thread = NULL;
    mtx_lock(&iqueue.mutex); // lock

    // wake up the oldest waiting thread
    if (tqueue.waiting > 0) // if there are any
    {
        // pop thread (before waking up)
        thread = pop_thread(&tqueue);
        thread->data = data;
        cnd_signal(&(thread->cv));
    }
    else
    {
        // add item to queue
        item = create_item(data);   
        add_item(&iqueue, item);
    }
    mtx_unlock(&iqueue.mutex); // unlock
}

void* dequeue(void)
{
    PITEM item = NULL;
    PTHREAD thread = NULL;
    void* data = NULL;
    mtx_lock(&iqueue.mutex); // lock
    
    if (0 == iqueue.size || iqueue.size < tqueue.waiting)
    {
        // add thread to queue (put to sleep) if the item queue is empty
        thread = create_thread();
        add_thread(&tqueue, thread);
        cnd_wait(&(thread->cv), &iqueue.mutex);
        // the thread was popped from the queue (by enqueue and then signaled)
        iqueue.visited++;
        data = thread->data;
        free_thread(thread);
    }
    else // pop the recent item and free its memory
    {
        item = pop_item(&iqueue);
        data = item->data;
        free_item(item);
    }
    mtx_unlock(&iqueue.mutex); // unlock
    return data;
}

bool tryDequeue(void** pdata)
{
    bool result = false;
    PITEM item = NULL;
    mtx_lock(&iqueue.mutex); // lock

    if (0 == iqueue.size)
    {
        result = false;
        goto lblCleanup;
    }
    
    item = pop_item(&iqueue);
    *pdata = item->data;
    free_item(item);
    result = true;

lblCleanup:
    mtx_unlock(&iqueue.mutex); // unlock
    return result;
}

size_t size(void)
{
    return iqueue.size;
}

size_t waiting(void)
{
    return tqueue.waiting;
}

size_t visited(void)
{
    return iqueue.visited;
}