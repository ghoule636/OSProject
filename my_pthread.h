/*
 * Group 3 OS Project
 * Spring 2016
 * Authors: Antonio Alvillar, Bethany Eastman, Gabriel Houle & Edgardo Gutierrez Jr.
 * GitHub: https://github.com/ghoule636/OSProject
 */

 /* Error Handling Values */
//#define SUCCESS 1
//#define BLOCKED 0
//#define NULL_OBJECT -1

typedef struct my_mutex {
    PCB_p owner;

    FIFOq_p blockedList;

} my_mutex;

/* Pointer to a mutex type */
typedef my_mutex *my_mutex_p;

/* Pointer to a queue (as a linked list) of cond nodes */
typedef struct my_cond_node *cond_list;

typedef struct my_cond_node {
    PCB_p owner;
    my_mutex_p lock;
    cond_list next;
} my_cond_node;

typedef struct my_cond {
    cond_list head;
} my_cond;

/* Pointer to a cond var type */
typedef my_cond *my_cond_p;

//Prototypes
void my_mutex_init(my_mutex_p);
int my_mutex_lock(my_mutex_p, PCB_p);
int my_mutex_trylock(my_mutex_p, PCB_p);
int my_mutex_unlock(my_mutex_p);

void my_cond_init(my_cond_p);
int my_cond_wait(my_cond_p, my_mutex_p, PCB_p);
int my_cond_signal(my_cond_p);

#include "my_pthread.c"