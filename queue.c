#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
//#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include "queue.h"
// -------- TYPEDEFS ----------

// Define the structure that the queue is built of 
typedef struct ItemNode { 
    void* pdata;
    struct ItemNode* pnext; // next item in queue
} ItemNode;

// Define the thread node structure for keeping track of waiting threads
typedef struct ThreadNode {
    cnd_t cond_var; // every thread has a cv + data associated with it
    void* pdata; // 
    struct ThreadNode* pnext;
} ThreadNode;

// Define the actual queue, built of Nodes
typedef struct Queue {
    ItemNode* pfront;
    ItemNode* prear;
    mtx_t mutex; // note that each queue requires only one mutex, but number of conds = number of threads associated with the queue
    size_t size;
    size_t visited;
} Queue;

// Define queue of ThreadNodes, signifying waiting threads in FIFO order
typedef struct ThreadQueue {
    ThreadNode* pfirst;
    ThreadNode* plast;
    size_t waiting
} ThreadQueue;

// -------- GLOBAL VARIABLES ----------
static Queue queue;
static ThreadQueue th_queue;

// -------- HELPER FUNCTIONS SIGNATURES ----------
ItemNode* create_item_node(void* pdata); // creates new ItemNode corresponding to pdata
void append_item_node(Queue* pqueue, ItemNode* pitem); // appends ItemNode to Queue
ItemNode* remove_first_item_node(Queue* pqueue); // removes and returns first ItemNode in queue (like pop())

ThreadNode* create_th_node(); // creates new ThreadNode (the pdata field is set in a different function)
void append_th_node(ThreadQueue* pth_queue, ThreadNode* pth); // appends ThreadNode to ThreadQueue
ThreadNode* remove_first_th_node(ThreadQueue* pth_queue); // removes and returns first ThreadNode in th_queue (like pop())


// -------- QUEUE HELPER FUNCTIONS IMPLEMENTATION ----------
ItemNode* create_item_node(void* pdata)
{
    ItemNode* pnew;

    pnew = (ItemNode*)malloc(sizeof(ItemNode)); // No error checking since we assume malloc never fails
    pnew->pdata = pdata;
    pnew->pnext = NULL;
    return pnew;
}

void append_item_node(Queue* pqueue, ItemNode* pitem)
{
    if(pqueue->size == 0)
    {
        pqueue->pfront = pitem;
        pqueue->prear = pitem;
    }
    else
    {
        pqueue->prear->pnext = pitem;
        pqueue->prear = pqueue->prear->pnext;
    }
    pqueue->size++;
}

// this function will only be used when there is at least one ItemNode in the queue
ItemNode* remove_first_item_node(Queue* pqueue)
{
    ItemNode* p_removed;

    p_removed = pqueue->pfront;
    pqueue->pfront = pqueue->pfront->pnext;
    pqueue->size--;
    if(pqueue->size == 0) // if size is 0 then 
    {
        pqueue->prear = NULL;
    }

    pqueue->visited--;
    return p_removed;
}

// -------- THREADQUEUE HELPER FUNCTIONS IMPLEMENTATION ----------
ThreadNode* create_th_node()
{
    ThreadNode* pnew;

    pnew = (ThreadNode*)malloc(sizeof(ThreadNode)); // No error checking since we assume malloc never fails
    // pdata of the newly created thread stays NULL for now, will be set when the thread is woken up
    pnew->pnext = NULL; // QUESTION is this needed?
    // setting conditional variable for the thread corresponding with this ThreadNode
    cnd_init(&(pnew->cond_var));
    return pnew;
}

void append_th_node(ThreadQueue* pth_queue, ThreadNode* pth)
{
    if(pth_queue->waiting == 0) 
    {
        pth_queue->pfirst = pth;
        pth_queue->plast = pth;
    }
    else
    {
        pth_queue->plast->pnext = pth;
        pth_queue->plast = pth_queue->plast->pnext;
    }
    pth_queue->waiting++;
}

// this function will only be used when there is at least one ThreadNode in th_queue
ThreadNode* remove_first_th_node(ThreadQueue* pth_queue)
{
    ThreadNode* p_removed_th;

    p_removed_th = pth_queue->pfirst;
    pth_queue->pfirst = pth_queue->pfirst->pnext;
    pth_queue->waiting--;
    if(pth_queue->waiting == 0) // if num of waiting threads is now 0 we need to set plast to NULL
    {
        pth_queue->plast = NULL;
    }

    return p_removed_th;
}

// -------- LIBRARY FUNCTIONS IMPLEMENTATION ----------

void initQueue(void)
{
    // Initializing queue
    queue.pfront = NULL;
    queue.prear = NULL;
    mtx_init(&queue.mutex, mtx_plain);
    queue.size = 0;
    queue.visited = 0;
    // Initializing th_queue
    th_queue.pfirst = NULL;
    th_queue.plast = NULL;
    th_queue.waiting = 0;
}

void destroyQueue(void)
{
    /*
    This function will be used for cleanup when the queue is no longer needed. It is possible for
    initQueue to be called afterwards.
    */

}

void enqueue(void* pdata)
{
    ThreadNode* pthread;
    ItemNode* pnode;

    mtx_lock(&queue.mutex);
    if(th_queue.waiting == 0)
    {
        // no thread is waiting - insert item into queue 
        pnode = create_item_node(pdata);
        append_node(pnode);
    }
    else
    {
        // threads are waiting - wake up the right one
        pthread = remove_first();
        pthread -> pdata = pdata;
        cnd_signal(&(pthread -> cond_var));
    }
    mtx_unlock(&queue.mutex);
}

void* dequeue(void)
{
    Node* pnode;
    void* pdata = NULL;

    if(queue.size > 0 && th_queue.waiting <= queue.size)
    {
        
    }

    else
    {
        // in this case, queue.size == 0 OR th_queue.waiting > queue.size


    }

}

bool tryDequeue(void** changethis)
{
    /*
    Try to remove an item from the queue. If succeeded, return it via the argument and return true.
    If the queue is empty, return false and leave the pointer unchanged.
    */

}

size_t size(void)
{
    /*Return the current amount of items in the queue.*/

}

size_t waiting(void)
{
    /*Return the current amount of threads waiting for the queue to fill.*/

}

size_t visited(void)
{
    /*
    Return the amount of items that have passed inside the queue (i.e., inserted and then removed).
    This should not block due to concurrent operations, i.e., you may not take a lock at all.
    */
}

int main(int argc, char* args[])
{

}