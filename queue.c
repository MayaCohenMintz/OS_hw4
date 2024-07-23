#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
//#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>

// -------- HELPER FUNCTIONS IMPLEMENTATION ----------

// -------- LIBRARY FUNCTIONS IMPLEMENTATION ----------

void initQueue(void)
{
    /*
    This function will be called before the queue is used. This is your chance to initialize your data
    structure.
    */

}

void destroyQueue(void)
{
    /*
    This function will be used for cleanup when the queue is no longer needed. It is possible for
    initQueue to be called afterwards.
    */

}

void enqueue(void*)
{
    /*Adds an item to the queue.*/
}

void* dequeue(void)
{
    /*Remove an item from the queue. Will block if empty.*/
}

bool tryDequeue(void**)
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