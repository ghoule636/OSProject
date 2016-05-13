/*
 * Group 3 OS Project
 * Spring 2016
 * Authors: Antonio Alvillar, Bethany Eastman, Gabriel Houle & Edgardo Gutierrez Jr.
 * GitHub: https://github.com/ghoule636/OSProject
 */

#ifndef PQ
#define PQ
#include "PriorityQ.h"
#endif

unsigned short MAX_PRIORITIES = 16;

PQ_p PQ_construct(void) {
  PQ_p result = malloc(sizeof(PriorityQ));
  int i;
  for (i = 0; i < MAX_PRIORITIES; i++) {

    FIFO myFIFO = FIFO_construct();
    FIFO_init(myFIFO);

    result->priority[i] = myFIFO;
  }

  return result;
}

void PQ_destruct(PQ_p theQ) {
  if (theQ) {
    int i;
    for (i = 0; i < MAX_PRIORITIES; i++) {
      FIFO_deconstruct(theQ->priority[i]);
    }
    free(theQ);
  }
}

void PQ_enqueue(PQ_p theQ, PCB_p thePCB) {
  enqueue(theQ->priority[thePCB->priority], thePCB);
}

PCB_p PQ_dequeue(PQ_p theQ) {
  int i;
  int end = 0;
  PCB_p result = NULL;
  for (i = 0; i < MAX_PRIORITIES && !end; i++) {
    if (theQ->priority[i]->head) {
      result = dequeue(theQ->priority[i]);
      end = 1;
    }
  }
  return result;
}

void PQ_toString(PQ_p theQ, char *theStr) {
  if (theStr) {

    char *testStr = malloc(10000);
    char *appendStr = malloc(1000);
    int i;

    for (i = 0; i < MAX_PRIORITIES; i++) {
      FIFO temp = theQ->priority[i];
      if (temp->head) {
        FIFO_toString(temp, testStr);
        sprintf(appendStr, "Q%d: %s\n", i, testStr);
        strcat(theStr, appendStr);
      }
    }
    free(testStr);
    free(appendStr);
  }
}
