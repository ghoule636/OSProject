/*
 * Group 3 OS Project
 * Spring 2016
 * Authors: Antonio Alvillar, Bethany Eastman, Gabriel Houle & Edgardo Gutierrez Jr.
 * GitHub: https://github.com/ghoule636/OSProject
 */

#ifndef FIFO_QUEUE_H
#define FIFO_QUEUE_H

typedef struct node Node;
typedef Node * Node_p;

struct node {
    PCB_p pcb;
    Node_p next;
};

typedef struct FIFO_q {
    int size;
    Node_p front;
    Node_p rear;
} FIFOq;
typedef FIFOq * FIFOq_p;

FIFOq_p FIFOq_construct(void);
int FIFOq_destruct(FIFOq_p);
int FIFOq_init(FIFOq_p);
int FIFOq_is_empty(FIFOq_p);
int FIFOq_size(FIFOq_p);
int FIFOq_enqueue(FIFOq_p, PCB_p);
PCB_p FIFOq_dequeue(FIFOq_p);
int FIFOq_toString_size(FIFOq_p);
char* FIFOq_toString(FIFOq_p, char*, int);

#include "fifo_queue.c"
#endif