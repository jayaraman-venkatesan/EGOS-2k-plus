#pragma once

#include "egos.h"
#include "malloc.h"

/* Note:
 * you should test your code thoroughly.
 * It will be extremely difficult to debug your user-level threading,
 * if there is a bug in your queue implementation.
 */

typedef struct node_t {
    struct node_t *next;
    void *item;
} node_t;


typedef struct queue_t {
    struct node_t *head;
    struct node_t *tail;
} queue_t;


/* initialize queue head and tail */
void queue_init(queue_t *q) {
  
    q->head = (void *)0;
    q->tail = (void *)0;
   
}

/* add the "item" to the tail of the queue "q".
 * note: you should wrap the "item" in a "node_t"*/
void enqueue(queue_t *q, void *item) {
    node_t *new_node = (node_t *)malloc(sizeof(node_t));

    if (new_node == NULL) {
        printf("RETURNING CAUSE NULL\n");
        return;
    }

    new_node->item = (node_t *)item;
    new_node->next = NULL;

    if (q->tail == NULL ) {
        // If the queue is empty, set both head and tail to the new node
        //printf("ADDED HERE1\n");
        q->head = new_node;
        q->tail = new_node;
    } else {
        // Otherwise, add the new node to the tail and update the tail pointer
        //printf("ADDED HERE2\n");
        q->tail->next = new_node;
        q->tail = new_node;
    }

    // printf("[IN ENQUEUE] calling dump..\n");
    // dump_queue(q);


}

/* remove and return the item at the front of the queue "q".
 * note: you should free memory allocated by enqueue.
 */
void *dequeue(queue_t *q) {
    /* [lab2-ex3]
     * TODO: your code here
     */

    //printf("HERE BRO\n");

     if (q->head == NULL) {
        // Queue is empty, return NULL
        return NULL;
    }

    // Get the item at the front of the queue and the corresponding node
    void *item = q->head->item;
    node_t *old_head = q->head;

    // Update the head pointer to the next node
    q->head = q->head->next;

    // Free the memory of the old head node

    free(old_head);

    // If the queue becomes empty after dequeue, update the tail pointer
    if (q->head == NULL) {
        q->tail = NULL;
    }


    return item;
}

/* [lab3-ex2]
 * the following functions can be useful for your MLFQ.
 */

// check if the "target" is in the queue "q"
int in_queue(queue_t *q, void *target) {
    /* TODO: your code here */

    node_t *current = q->head;
    while (current != NULL) {
        if (current->item == target) {
            return 1; // Found the target in the queue
        }
        current = current->next;
    }
    return 0; 
}

// try to remove the "target" from the queue "q",
// but the target may or may not in the queue.
int try_rm_item(queue_t *q, void *target) {
    /* TODO: your code here */

    node_t *current = q->head;
    node_t *prev = NULL;
    
    //dump_queue(q);

    while (current != NULL) {
        if (current->item == target) {
            if (prev == NULL) {
                // The target is at the head of the queue
                q->head = current->next;
                if(q->head == NULL){
                    q->tail = NULL;
                }
            } else {
                // The target is in the middle or at the tail of the queue
                prev->next = current->next;
                if (current->next == NULL) {
                    q->tail = prev;
                }
            }

            free(current);
            return 1; // Successfully removed the target
        }

        prev = current;
        current = current->next;
    }

    return 0; 
}

// remove the "target" from the queue "q",
// the target must be in the queue; otherwise, there is an error
void rm_item(queue_t *q, void *target) {
    /* TODO: your code here */
    if (!try_rm_item(q, target)) {
        printf("ERROR REMOVING ITEM FROM THE QUEUE. EXITING \n");
        return;
    }
}

// print the queue with whatever information you think is useful
void dump_queue(queue_t *q) {
    /* TODO: your code here */
    printf("Printing queue  \n");
    node_t *current = q->head;
    while (current != NULL) {
        struct process *p = (struct process *) current->item;
        printf("id:[%d]  -- status: [%d]\n",p->pid,p->status);
        current = current->next;
    }
    printf("\n");
}
