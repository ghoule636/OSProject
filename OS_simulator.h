/*
 * Group 3 OS Project
 * Spring 2016
 * Authors: Antonio Alvillar, Bethany Eastman, Gabriel Houle & Edgardo Gutierrez Jr.
 * GitHub: https://github.com/ghoule636/OSProject
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
// #include "pcb.h"
// #include "fifo_queue.h"
// #include "PriorityQ.h"
// #include "my_pthread.h"

PQ_p readyQ;

void createProcesses();
void starvationPrevention();
int main();