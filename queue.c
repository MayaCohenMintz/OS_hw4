#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
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
    size_t waiting;
} ThreadQueue;

// -------- GLOBAL VARIABLES ----------
static Queue queue;
static ThreadQueue th_queue;

// -------- HELPER FUNCTIONS SIGNATURES ----------
ItemNode* create_item_node(void* pdata); // creates new ItemNode corresponding to pdata
void append_item_node(Queue* pqueue, ItemNode* pitem); // appends ItemNode to Queue
ItemNode* remove_first_item_node(Queue* pqueue); // removes and returns first ItemNode in queue (like pop())
void iter_free_item_nodes(Queue* pqueue); // iteratively frees queue 

ThreadNode* create_th_node(); // creates new ThreadNode (the pdata field is set in a different function)
void append_th_node(ThreadQueue* pth_queue, ThreadNode* pth); // appends ThreadNode to ThreadQueue
ThreadNode* remove_first_th_node(ThreadQueue* pth_queue); // removes and returns first ThreadNode in th_queue (like pop())
void iter_free_thread_nodes(ThreadQueue* pth_queue); // iteratively frees th_queue

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
        pqueue->prear = pitem;
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

    pqueue->visited++;
    return p_removed;
}

void iter_free_item_nodes(Queue* pqueue)
{
    ItemNode* pcurr;
    ItemNode* pto_free; // this is the ItemNode to be freed

    // Iteratively freeing ItemNodes in queue
    pcurr = queue.pfront;
    while(pcurr != NULL)
    {
    pto_free = pcurr;
    pcurr = pcurr->pnext;
    free(pto_free);
    }
    pqueue->pfront = NULL;
    pqueue->prear = NULL;
}

// -------- THREADQUEUE HELPER FUNCTIONS IMPLEMENTATION ----------
ThreadNode* create_th_node()
{
    ThreadNode* pnew;

    pnew = (ThreadNode*)malloc(sizeof(ThreadNode)); // No error checking since we assume malloc never fails
    // pdata of the newly created thread stays NULL for now, will be set when the thread is woken up
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
        pth_queue->plast = pth;
    }
    pth_queue->waiting++;
}

// this function will only be used when there is at least one ThreadNode in th_queue
ThreadNode* remove_first_th_node(ThreadQueue* pth_queue)
{
    ThreadNode* p_removed_th; // pointer to the thread to be removed

    p_removed_th = pth_queue->pfirst;
    pth_queue->pfirst = p_removed_th->pnext;
    pth_queue->waiting--;
    if(pth_queue->waiting == 0) // if num of waiting threads is now 0 we need to set plast to NULL
    {
        pth_queue->plast = NULL;
    }

    return p_removed_th;
}


void iter_free_th_nodes(ThreadQueue* pth_queue)
{
    ThreadNode* pcurr;
    ThreadNode* pto_free; // this is the ItemNode to be freed

    // Iteratively freeing ItemNodes in queue
    pcurr = th_queue.pfirst;
    while(pcurr != NULL)
    {
    pto_free = pcurr;
    pcurr = pcurr->pnext;
    cnd_destroy(&(pto_free->cond_var));
    free(pto_free);
    }
    pth_queue->pfirst = NULL;
    pth_queue->plast = NULL;
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
    mtx_lock(&queue.mutex);
    iter_free_item_nodes(&queue); // iteratively freeing ItemNodes in queue
    queue.size = 0;
    queue.visited = 0;
    iter_free_th_nodes(&th_queue); // iteratively freeing ThreadNodes in th_queue
    th_queue.waiting = 0;

    mtx_unlock(&queue.mutex);
    mtx_destroy(&queue.mutex);
}

void enqueue(void* pdata)
{
    ThreadNode* pth;
    ItemNode* pitem;

    mtx_lock(&queue.mutex);
    if(th_queue.waiting == 0)
    {
        // no thread is waiting - insert item into queue without waking a thread up 
        pitem = create_item_node(pdata);
        append_item_node(&queue, pitem);
    }
    else
    {
        // threads are waiting - wake up the right one
        pth = remove_first_th_node(&th_queue);
        pth -> pdata = pdata;
        cnd_signal(&(pth->cond_var));
    }
    mtx_unlock(&queue.mutex);
}

void* dequeue(void)
{
    ItemNode* pitem;
    ThreadNode* pth;
    void* pret_data = NULL;

    mtx_lock(&queue.mutex);

    if(queue.size == 0) // no item to dequeue
    {
        // create thread node to be associated with this dequeue action, and append it to th_queue
        pth = create_th_node();
        append_th_node(&th_queue, pth);
        // put thread to sleep so it can be signaled by enqueue when another item is inserted 
        cnd_wait(&(pth->cond_var), &queue.mutex);
        // pth is popped from th_queue by enqueue
        queue.visited++;
        // enqueue transfers the item's data to pth, so it can be returned before even being inserted into queue
        // now transferring data associated with dequeued item to be returned
        pret_data = pth->pdata;
        // freeing removed thread
        free(pth);
    }

    else // there is an item in the queue to dequeue
    {
        // remove front of queue
        pitem = remove_first_item_node(&queue);
        // transfer the data from popped front to pret_data
        pret_data = pitem->pdata;
        // free popped item
        free(pitem);
    }
    mtx_unlock(&queue.mutex);
    return pret_data;
}

bool tryDequeue(void** returned_ptr)
{
    bool ret;
    ItemNode* pret = NULL;

    mtx_lock(&queue.mutex);
    if(queue.size == 0)  // no item to dequeue
    {
        ret = false;
        mtx_unlock(&queue.mutex);
        return ret;
    }
    
    else // there is an item to dequeue
    {
        // remove front of queue
        pret = remove_first_item_node(&queue);
        // inserting popped item's data into returned_ptr so that we can free popped item
        *returned_ptr = pret->pdata;
        ret = true;
        free(pret);
        mtx_unlock(&queue.mutex);
        return ret;
    }
}

size_t size(void)
{
    /*Return the current amount of items in the queue.*/
    return queue.size;
}

size_t waiting(void)
{
    /*Return the current amount of threads waiting for the queue to fill.*/
    return th_queue.waiting;
}

size_t visited(void)
{
    /*
    Return the amount of items that have passed inside the queue (i.e., inserted and then removed).
    This should not block due to concurrent operations, i.e., you may not take a lock at all.
    */
   return queue.visited;
}

// int main(int argc, char* args[])
// {

//     return 0;
// }