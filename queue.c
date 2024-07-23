#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
//#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>


// -------- TYPEDEFS ----------

// Define the node structure that the queue is built of 
typedef struct Node { 
    void* data;
    struct Node* next; // next item in queue
} Node;

// Define the thread node structure for keeping track of waiting threads
typedef struct ThreadNode {
    cnd_t cond_var; // every thread has a cv + data associated with it
    void* pdata; // 
    struct ThreadNode* next;
} ThreadNode;

// Define the actual queue, built of Nodes
typedef struct Queue {
    Node* pfront;
    Node* prear;
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
remove_first_th(); // like pop() for th_queue
create_node(); // creates new Node
append_node(Node* pnode); // appends node to Queue
remove_first_node(); // like pop() for queue


// -------- HELPER FUNCTIONS IMPLEMENTATION ----------

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
    Node* pnode;

    mtx_lock(&queue.mutex);
    if(th_queue.waiting == 0)
    {
        // no thread is waiting - insert item into queue 
        pnode = create_node();
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