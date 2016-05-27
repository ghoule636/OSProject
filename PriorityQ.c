/*
 * Group 3 OS Project
 * Spring 2016
 * Authors: Antonio Alvillar, Bethany Eastman, Gabriel Houle & Edgardo Gutierrez Jr.
 * GitHub: https://github.com/ghoule636/OSProject
 */

//#include "PriorityQ.h"

PQ_p PQ_construct(void) {
  PQ_p result = malloc(sizeof(PriorityQ));
  int i;
  for (i = 0; i < MAX_PRIORITIES; i++) {

    FIFOq_p myFIFO = FIFOq_construct();
    FIFOq_init(myFIFO);

    result->priority[i] = myFIFO;
  }

  return result;
}

void PQ_destruct(PQ_p theQ) {
  if (theQ) {
    int i;
    for (i = 0; i < MAX_PRIORITIES; i++) {
      FIFOq_destruct(theQ->priority[i]);
    }
    free(theQ);
  }
}

void PQ_enqueue(PQ_p theQ, PCB_p thePCB) {
    FIFOq_enqueue(theQ->priority[thePCB->priority], thePCB);
}

PCB_p PQ_dequeue(PQ_p theQ) {
  int i;
  int end = 0;
  PCB_p result = NULL;
  for (i = 0; i < MAX_PRIORITIES && !end; i++) {
    if (theQ->priority[i]->front) {
      result = FIFOq_dequeue(theQ->priority[i]); // FIFO or priorirty
      end = 1;
    }
  }
  return result;
}

int PQ_is_empty(PQ_p theQ) {
    int result = 1;
    int i;
    for (i = 0; i < MAX_PRIORITIES; i++) {
        if (!FIFOq_is_empty(theQ->priority[i])) {
            result = 0;
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
      FIFOq_p temp = theQ->priority[i];
      if (temp->front) {
        int queue_string_size = FIFOq_toString_size(temp); 
        FIFOq_toString(temp, testStr, queue_string_size);
        sprintf(appendStr, "Q%d: %s\n", i, testStr);
        strcat(theStr, appendStr);
      }
    }
    free(testStr);
    free(appendStr);
  }
}
