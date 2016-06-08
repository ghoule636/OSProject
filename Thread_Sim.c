/*
 * Thread_Sim.c
 * Group: Hunter Bennett, Gabriel Houle, Antonio Alvillar, Bethany Eastman, Edgardo Gutierrez Jr.
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "PCB.h"
#include "Mutex.h"
#include "Cond.h"
#include "PCB_Queue.h"
#include "PCB_Priority_Queue.h"
#include "PCB_Errors.h"

// times must be under 1 billion
#define SLEEP_TIME 9000000
#define IO_TIME_MIN SLEEP_TIME * 3
#define IO_TIME_MAX SLEEP_TIME * 5

#define IDL_PID 0xFFFFFFFF

#define DEADLOCK_POSSIBLE 0

void create_mutual_pcbs(int priority);
void create_sync_pcbs(int priority);
void create_io_pcb(int priority);
void create_pcbs();

enum INTERRUPT_TYPE {
    INTERRUPT_TYPE_TIMER = 1, 
    INTERRUPT_TYPE_IO_A, 
    INTERRUPT_TYPE_IO_B
};

typedef struct {
    pthread_mutex_t *mutex;
    pthread_cond_t *condition;
    int flag;
    int sleep_length_min;
    int sleep_length_max;
} timer_arguments;

typedef timer_arguments* timer_args_p;

PCB_Queue_p createdQueue;
PCB_Priority_Queue_p readyQueue;
PCB_Queue_p terminatedQueue;
PCB_Queue_p waitingQueueA;
PCB_Queue_p waitingQueueB;
PCB_p currentPCB;
PCB_p idl;
unsigned long sysStack;
enum PCB_ERROR error = PCB_SUCCESS;

void* timer(void *arguments) {
    struct timespec sleep_time;
    timer_args_p args = (timer_args_p) arguments; 
    pthread_mutex_lock(args->mutex);
    for(;;) {
        int sleep_length = args->sleep_length_min + rand() % (args->sleep_length_max - args->sleep_length_min + 1);
        sleep_time.tv_nsec = sleep_length;
        nanosleep(&sleep_time, NULL);
        pthread_cond_wait(args->condition, args->mutex);
        args->flag = 0;
    }
}

void dispatcher() {
    if (!PCB_Priority_Queue_is_empty(readyQueue, &error)) {
        currentPCB = PCB_Priority_Queue_dequeue(readyQueue, &error);
    } else {
        currentPCB = idl;
    }
    printf("Switching to:\t");
    PCB_print(currentPCB, &error);
    printf("Ready Queue: Size: %u\n", readyQueue->size);
    PCB_Priority_Queue_print(readyQueue, &error);
    PCB_set_state(currentPCB, PCB_STATE_RUNNING, &error);
    sysStack = PCB_get_pc(currentPCB, &error);
}

void deadlock_detector() {
    if (currentPCB->mutual_user) {
        Mutex_p m1 = currentPCB->mutual_user->mutex1;
        Mutex_p m2 = currentPCB->mutual_user->mutex2;
        if ((m1->owner == currentPCB && m2->owner != NULL && m2->owner != currentPCB &&
                Mutex_contains(m2, currentPCB) && Mutex_contains(m1, m2->owner)) ||
                (m2->owner == currentPCB && m1->owner != NULL && m1->owner != currentPCB &&
                Mutex_contains(m1, currentPCB) && Mutex_contains(m2, m1->owner))) {
            printf("Deadlock detected on Mutual pair %u, PID: 0x%lX & 0x%lX\n", currentPCB->mutual_user->id, m1->owner->pid, m2->owner->pid);
        }
    }
}

void scheduler(enum INTERRUPT_TYPE interruptType) {
    deadlock_detector();
    create_pcbs();

    // Removal of boost if needed.
    if (PCB_is_boosting(currentPCB, &error)) {
        PCB_set_boosting(currentPCB, false, &error);
        printf("Boost off:\t");
        PCB_print(currentPCB, &error);
    }
    set_last_executed(currentPCB, &error);

    while (!PCB_Queue_is_empty(terminatedQueue, &error)) {
        PCB_p p = PCB_Queue_dequeue(terminatedQueue, &error);
        printf("Deallocated:\t");
        PCB_print(p, &error);
        PCB_destruct(p, &error);
    }
    while (!PCB_Queue_is_empty(createdQueue, &error)) {
        PCB_p p = PCB_Queue_dequeue(createdQueue, &error);
        PCB_set_state(p, PCB_STATE_READY, &error);
        printf("Scheduled:\t");
        PCB_print(p, &error);
        PCB_Priority_Queue_enqueue(readyQueue, p, &error);
    }

    // Boosting starved processes
    for (int i = 1; i <= PCB_PRIORITY_MAX; i++) {
        for (int j = 0; j < readyQueue->queues[i]->size; j++) {
            PCB_p temp = PCB_Queue_dequeue(readyQueue->queues[i], &error);
            readyQueue->size--;
            if (duration_met(temp, 3, &error) && !PCB_is_boosting(temp, &error)) {
                PCB_set_boosting(temp, true, &error);
                printf("Boost on:\t");
                PCB_print(temp, &error);
            }
            PCB_Priority_Queue_enqueue(readyQueue, temp, &error);
        }
    }
    

    if (interruptType == INTERRUPT_TYPE_TIMER) {
        PCB_set_state(currentPCB, PCB_STATE_READY, &error);
        if (PCB_get_pid(currentPCB, &error) != IDL_PID) {
            printf("Returned:\t");
            PCB_print(currentPCB, &error);
            PCB_Priority_Queue_enqueue(readyQueue, currentPCB, &error);
        }
        dispatcher();
    } else if (interruptType == INTERRUPT_TYPE_IO_A) {
        PCB_p p = PCB_Queue_dequeue(waitingQueueA, &error);
        printf("Moved from IO A:");
        PCB_print(p, &error);
        printf("Waiting Queue A:");
        PCB_Queue_print(waitingQueueA, &error);
        PCB_set_state(p, PCB_STATE_READY, &error);
        PCB_Priority_Queue_enqueue(readyQueue, p, &error);
    } else if (interruptType == INTERRUPT_TYPE_IO_B) {
        PCB_p p = PCB_Queue_dequeue(waitingQueueB, &error);
        printf("Moved from IO B:");
        PCB_print(p, &error);
        printf("Waiting Queue B:");
        PCB_Queue_print(waitingQueueA, &error);
        PCB_set_state(p, PCB_STATE_READY, &error);
        PCB_Priority_Queue_enqueue(readyQueue, p, &error);
    }
}

void isrTimer() {
    printf("\nSwitching from:\t");
    PCB_print(currentPCB, &error);
    PCB_set_state(currentPCB, PCB_STATE_INTERRUPTED, &error);
    PCB_set_pc(currentPCB, sysStack, &error);
    scheduler(INTERRUPT_TYPE_TIMER);
}

void isrIO(enum INTERRUPT_TYPE interruptType) {
    scheduler(interruptType);
}

void terminate() {
    if (currentPCB->producer) {
        if (*(currentPCB->producer->dead)) {
            free(currentPCB->producer->dead);
            free(currentPCB->producer->mutex);
            free(currentPCB->producer->condc);
            free(currentPCB->producer->condp);
            free(currentPCB->producer);
            currentPCB->producer = NULL;
        } else {
            *(currentPCB->producer->dead) = 1;
        }
    } else if (currentPCB->consumer) {
        if (*(currentPCB->consumer->dead)) {
            free(currentPCB->consumer->dead);
            free(currentPCB->consumer->mutex);
            free(currentPCB->consumer->condc);
            free(currentPCB->consumer->condp);
            free(currentPCB->consumer);
            currentPCB->consumer = NULL;
        } else {
            *(currentPCB->consumer->dead) = 1;
        }
    } else if (currentPCB->mutual_user) {
        if (*(currentPCB->mutual_user->dead)) {
            free(currentPCB->mutual_user->dead);
            free(currentPCB->mutual_user->mutex1);
            free(currentPCB->mutual_user->mutex2);
            free(currentPCB->mutual_user);
            currentPCB->mutual_user = NULL;
        } else {
            *(currentPCB->mutual_user->dead) = 1;
        }
    }
    printf("Switching from:\t");
    PCB_print(currentPCB, &error);
    PCB_set_pc(currentPCB, sysStack, &error);
    PCB_set_state(currentPCB, PCB_STATE_HALTED, &error);
    PCB_set_termination(currentPCB, time(NULL), &error);
    printf("Terminated:\t");
    PCB_print(currentPCB, &error);
    PCB_Queue_enqueue(terminatedQueue, currentPCB, &error);
    dispatcher();
}

void tsr(enum INTERRUPT_TYPE interruptType) {
    printf("Switching from:\t");
    PCB_print(currentPCB, &error);
    PCB_set_pc(currentPCB, sysStack, &error);
    PCB_set_state(currentPCB, PCB_STATE_WAITING, &error);
    if (interruptType == INTERRUPT_TYPE_IO_A) {
        printf("Moved to IO A:\t");
        PCB_Queue_enqueue(waitingQueueA, currentPCB, &error);
        PCB_print(currentPCB, &error);
        printf("Waiting Queue A:");
        PCB_Queue_print(waitingQueueA, &error);
    } else if (interruptType == INTERRUPT_TYPE_IO_B) {
        printf("Moved to IO B:\t");
        PCB_Queue_enqueue(waitingQueueB, currentPCB, &error);
        PCB_print(currentPCB, &error);
        printf("Waiting Queue B:");
        PCB_Queue_print(waitingQueueB, &error);
    } 
    dispatcher();
}

void initialize_globals() {
    idl = PCB_construct(&error);
    PCB_set_pid(idl, IDL_PID, &error);
    PCB_set_state(idl, PCB_STATE_RUNNING, &error);
    PCB_set_terminate(idl, 0, &error);
    PCB_set_max_pc(idl, 0, &error);
    for (int i = 0; i < PCB_TRAP_LENGTH; i++) {
        idl->io_1_traps[i] = 1;
        idl->io_2_traps[i] = 1;
    }
    currentPCB = idl;

    createdQueue = PCB_Queue_construct(&error);
    waitingQueueA = PCB_Queue_construct(&error);
    waitingQueueB = PCB_Queue_construct(&error);
    readyQueue = PCB_Priority_Queue_construct(&error);
    terminatedQueue = PCB_Queue_construct(&error);
}

void step() {
    // printf("step\n");
    if (sysStack >= currentPCB->max_pc) {
        sysStack = 0;
        currentPCB->term_count++;
        if (currentPCB->terminate != 0 && currentPCB->terminate <= currentPCB->term_count) {
            terminate();
        }
    } else {
        sysStack++;
    }
}

void create_mutual_pcbs(int priority) {
    PCB_p u1 = PCB_construct(&error);
    PCB_p u2 = PCB_construct(&error);
    u1->terminate = 40;
    u2->terminate = 40;
    Mutex_p m1 = Mutex_construct();
    Mutex_p m2 = Mutex_construct();
    Mutual_User_p mu = malloc(sizeof(struct Mutual_User));
    mu->id = rand() % 10000;
    mu->mutex1 = m1;
    mu->mutex2 = m2;
    mu->resource1 = 0;
    mu->resource2 = 0;
    mu->dead = malloc(sizeof(int));
    *(mu->dead) = 0;
    PCB_set_priority(u1, priority, &error);
    PCB_set_priority(u2, priority, &error);
    PCB_set_max_pc(u1, 10, &error);
    PCB_set_max_pc(u2, 10, &error);
    int temp_pid = rand() % 100000;
    u1->pid = temp_pid;
    u2->pid = temp_pid + 1;
    u1->lock_traps[0] = 0;
    u1->lock2_traps[0] = 1;
    u1->unlock_traps[0] = 3;
    u1->unlock2_traps[0] = 4;
    u2->lock_traps[0] = 0;
    u2->lock2_traps[0] = 1;
    u2->unlock_traps[0] = 3;
    u2->unlock2_traps[0] = 4;
    u1->mutual_user = mu;
    if (DEADLOCK_POSSIBLE) {
        Mutual_User_p mu2 = malloc(sizeof(struct Mutual_User));
        mu2->id = mu->id;
        mu2->mutex1 = m2;
        mu2->mutex2 = m1;
        mu2->resource1 = 0;
        mu2->resource2 = 0;
        mu2->dead = mu->dead;
        u2->mutual_user = mu2;
    } else {
        u2->mutual_user = mu;
    }

    PCB_Queue_enqueue(createdQueue, u1, &error);
    printf("Created:\t");
    PCB_print(u1, &error);
    PCB_Queue_enqueue(createdQueue, u2, &error);
    printf("Created:\t");
    PCB_print(u2, &error);
}

void create_sync_pcbs(int priority) {
    PCB_p producer = PCB_construct(&error);
    PCB_p consumer = PCB_construct(&error);
    producer->terminate = 10;
    consumer->terminate = 10;
    PCB_set_max_pc(producer, 6, &error);
    PCB_set_max_pc(consumer, 6, &error);
    PCB_set_priority(producer, priority, &error);
    PCB_set_priority(consumer, priority, &error);
    Paired_User_p pu = malloc(sizeof(struct Paired_User));
    pu->id = rand() % 10000;
    pu->flag = 0;
    pu->mutex = Mutex_construct();
    pu->condc = Cond_construct();
    pu->condp = Cond_construct();
    pu->data = 0;
    pu->dead = malloc(sizeof(int));
    *(pu->dead) = 0;
    producer->producer = pu;
    consumer->consumer = pu;
    int temp_pid = rand() % 100000;
    producer->pid = temp_pid;
    consumer->pid = temp_pid + 1;
    producer->lock_traps[0] = 0;
    producer->wait_traps[0] = 1;
    producer->unlock_traps[0] = 4;
    producer->signal_traps[0] = 3;
    consumer->lock_traps[0] = 0;
    consumer->wait_traps[0] = 1;
    consumer->unlock_traps[0] = 4;
    consumer->signal_traps[0] = 3;
    PCB_Queue_enqueue(createdQueue, producer, &error);
    printf("Created:\t");
    PCB_print(producer, &error);
    PCB_Queue_enqueue(createdQueue, consumer, &error);
    printf("Created:\t");
    PCB_print(consumer, &error);
}

void create_io_pcb(int priority) {
    PCB_p io_process = PCB_construct(&error);
    PCB_set_priority(io_process, priority, &error);
    
    int temp_pid = rand() % 100000;
    io_process->pid = temp_pid;
    io_process->terminate = 30;
    int temp_max = rand() % 1000;
    PCB_set_max_pc(io_process, temp_max, &error);

    for (int i = 0; i < PCB_TRAP_LENGTH; i++) {
        io_process->io_1_traps[i] = rand() % io_process->max_pc;
        io_process->io_2_traps[i] = rand() % io_process->max_pc;
    }
    PCB_Priority_Queue_enqueue(readyQueue, io_process, &error);
    printf("Created:\t");
    PCB_print(io_process, &error);
}

void create_intense_pcb(int priority) {
    PCB_p intense = PCB_construct(&error);
    PCB_set_priority(intense, priority, &error);
    int temp_pid = rand() % 1000;
    intense->pid = temp_pid;
    int temp_max = rand() % 1000;
    intense->terminate = 15;
    PCB_set_max_pc(intense, temp_max, &error);
    PCB_Priority_Queue_enqueue(readyQueue, intense, &error);
    printf("Created:\t");
    PCB_print(intense, &error);
}

void create_pcbs() {
    int i, j, k, rand_pcb;
    int mutual_count = 0;
    for (i = 0; i <= PCB_PRIORITY_MAX; i++) {
        if (!i) { // check priority 0
            if (readyQueue->queues[i]->size < 2) {
                int creation_count = 2 - readyQueue->queues[i]->size;
                for (j = 0; j < creation_count; j++) {
                    if (rand() % 10 == 1) {
                        create_intense_pcb(0);
                    }
                }
            }
        } else if (i == 1) { // priority 1
            if (readyQueue->queues[i]->size < 32) {
                for (k = 0; k < readyQueue->queues[i]->size; k++) { // checks quantity levels of pcbs
                    struct node* n = readyQueue->queues[i]->first_node_ptr;
                    while (n != NULL) {
                        if (n->value->producer != NULL || n->value->consumer != NULL || n->value->mutual_user != NULL) {
                            mutual_count++;
                        }
                        n = n->next_node;
                    }
                }
                for (j = 0; j < 16 - mutual_count; j += 2) {
                    if (rand() % 10 == 1) {
                        create_io_pcb(1);
                    } else {
                        rand_pcb = rand() % 10;
                        if (rand_pcb < 5) {
                            create_sync_pcbs(1);
                        } else {
                            create_mutual_pcbs(1);
                        }
                    }
                    
                }
                // int creation_count = 32 - readyQueue->queues[i]->size;
                // for (j = 0; j < creation_count; j++) {
                //     rand_pcb = rand() % 10;
                //     if (rand_pcb < 5) {
                //         create_intense_pcb(1);
                //     } else if (rand_pcb >= 5) {
                //         create_io_pcb(1);
                //     }
                // }

            }
        } else if (i == 2) { // priority 2
            if (readyQueue->queues[i]->size < 4) {
                rand_pcb = rand() % 10;
                if (rand_pcb < 5) {
                    create_io_pcb(2);
                } else {
                    create_intense_pcb(2);
                }
            }

        } else if (i == 3) { // priority 3
            if (readyQueue->queues[i]->size < 2) {
                rand_pcb = rand() % 10;
                if (rand_pcb < 5) {
                    create_io_pcb(3);
                } else {
                    create_intense_pcb(3);
                }
            }
        }
    }
}

int main() {
    pthread_t system_timer, io_timer_a, io_timer_b;
    pthread_mutex_t mutex_timer, mutex_io_a, mutex_io_b;
    pthread_cond_t cond_timer, cond_io_a, cond_io_b;
    timer_args_p system_timer_args, io_timer_a_args, io_timer_b_args;
    
    pthread_mutex_init(&mutex_timer, NULL);
    pthread_mutex_init(&mutex_io_a, NULL);
    pthread_mutex_init(&mutex_io_b, NULL);
    
    pthread_cond_init(&cond_timer, NULL);
    pthread_cond_init(&cond_io_a, NULL);
    pthread_cond_init(&cond_io_b, NULL);
    
    system_timer_args = malloc(sizeof(timer_arguments));
    io_timer_a_args = malloc(sizeof(timer_arguments));
    io_timer_b_args = malloc(sizeof(timer_arguments));
    
    system_timer_args->mutex = &mutex_timer;
    system_timer_args->condition = &cond_timer;
    system_timer_args->sleep_length_min = SLEEP_TIME;
    system_timer_args->sleep_length_max = SLEEP_TIME;
    
    io_timer_a_args->mutex = &mutex_io_a;
    io_timer_a_args->condition = &cond_io_a;
    io_timer_a_args->sleep_length_min = IO_TIME_MIN;
    io_timer_a_args->sleep_length_max = IO_TIME_MAX;
    
    io_timer_b_args->mutex = &mutex_io_b;
    io_timer_b_args->condition = &cond_io_b;
    io_timer_b_args->sleep_length_min = IO_TIME_MIN;
    io_timer_b_args->sleep_length_max = IO_TIME_MAX;
    
    srand(time(NULL));
    
    pthread_create(&system_timer, NULL, &timer, (void*) system_timer_args);
    pthread_create(&io_timer_a, NULL, &timer, (void*) io_timer_a_args);
    pthread_create(&io_timer_b, NULL, &timer, (void*) io_timer_b_args);

    initialize_globals();

    while(1) {
        if (error != PCB_SUCCESS) {
            printf("\nERROR: error != PCB_SUCCESS");
            return 1;
        }

        if ((currentPCB->producer && *(currentPCB->producer->dead)) ||
                (currentPCB->consumer && *(currentPCB->consumer->dead)) ||
                (currentPCB->mutual_user && *(currentPCB->mutual_user->dead))) {
            terminate();
        }

        // if (currentPCB->producer) {
        //     Mutex_print(currentPCB->producer->mutex);
        //     Cond_print(currentPCB->producer->condp);
        // } else if (currentPCB->consumer) {
        //     Mutex_print(currentPCB->consumer->mutex);
        //     Cond_print(currentPCB->consumer->condc);
        // }

        
        if(system_timer_args->flag == 0 && !pthread_mutex_trylock(&mutex_timer)) {
            system_timer_args->flag = 1;
            isrTimer();
            pthread_cond_signal(&cond_timer);    
            pthread_mutex_unlock(&mutex_timer);
        } 
        if(io_timer_a_args->flag == 0 && !PCB_Queue_is_empty(waitingQueueA, &error) && !pthread_mutex_trylock(&mutex_io_a)) {
            isrIO(INTERRUPT_TYPE_IO_A);
            if (!PCB_Queue_is_empty(waitingQueueA, &error)) {
                io_timer_a_args->flag = 1;
                pthread_cond_signal(&cond_io_a);
            }
            pthread_mutex_unlock(&mutex_io_a);
        } 
        if(io_timer_b_args->flag == 0 && !PCB_Queue_is_empty(waitingQueueB, &error) && !pthread_mutex_trylock(&mutex_io_b)) {
            isrIO(INTERRUPT_TYPE_IO_B);
            if (!PCB_Queue_is_empty(waitingQueueB, &error)) {
                io_timer_b_args->flag = 1;
                pthread_cond_signal(&cond_io_b);
            }
            pthread_mutex_unlock(&mutex_io_b);
        } 



        if (currentPCB->mutual_user) {
            if (currentPCB->mutual_user->mutex1->owner == currentPCB && 
                    currentPCB->mutual_user->mutex2->owner == currentPCB) {
                printf("Mutual 0x%lX of pair %u uses both resources\n", currentPCB->pid, currentPCB->mutual_user->id);
            } 
        }
        


        for (int i = 0; i < PCB_TRAP_LENGTH; i++) {
            if (sysStack == currentPCB->io_1_traps[i]) {
                tsr(INTERRUPT_TYPE_IO_A);
                if(io_timer_a_args->flag == 0 && !pthread_mutex_trylock(&mutex_io_a)) {
                    io_timer_a_args->flag = 1;
                    pthread_cond_signal(&cond_io_a);
                    pthread_mutex_unlock(&mutex_io_a);
                } 
            } else if (sysStack == currentPCB->io_2_traps[i]) {
                tsr(INTERRUPT_TYPE_IO_B);
                if(io_timer_b_args->flag == 0 && !pthread_mutex_trylock(&mutex_io_b)) {
                    io_timer_b_args->flag = 1;
                    pthread_cond_signal(&cond_io_b);
                    pthread_mutex_unlock(&mutex_io_b);
                } 
            } else if (sysStack == currentPCB->lock_traps[i]) {
                if (currentPCB->producer && !Mutex_contains(currentPCB->producer->mutex, currentPCB)) {
                    printf("Producer 0x%lX of pair %u locks\n", currentPCB->pid, currentPCB->producer->id);
                    Mutex_lock(currentPCB->producer->mutex, currentPCB);
                } else if (currentPCB->consumer && !Mutex_contains(currentPCB->consumer->mutex, currentPCB)) {
                    printf("Consumer 0x%lX of pair %u locks\n", currentPCB->pid, currentPCB->consumer->id);
                    Mutex_lock(currentPCB->consumer->mutex, currentPCB);
                } else if (currentPCB->mutual_user && !Mutex_contains(currentPCB->mutual_user->mutex1, currentPCB)) {
                    printf("Mutual 0x%lX of pair %u locks 1\n", currentPCB->pid, currentPCB->mutual_user->id);
                    Mutex_lock(currentPCB->mutual_user->mutex1, currentPCB);
                }
            } else if (sysStack == currentPCB->lock2_traps[i]) {
                if (currentPCB->mutual_user && !Mutex_contains(currentPCB->mutual_user->mutex2, currentPCB) 
                            && currentPCB->mutual_user->mutex1->owner == currentPCB) {
                    printf("Mutual 0x%lX of pair %u locks 2\n", currentPCB->pid, currentPCB->mutual_user->id);
                    Mutex_lock(currentPCB->mutual_user->mutex2, currentPCB);
                }
            } else if (sysStack == currentPCB->unlock_traps[i]) {
                if (currentPCB->producer && currentPCB->producer->mutex->owner == currentPCB) {
                    printf("Producer 0x%lX of pair %u unlocks\n", currentPCB->pid, currentPCB->producer->id);
                    Mutex_unlock(currentPCB->producer->mutex, currentPCB);
                } else if (currentPCB->consumer && currentPCB->consumer->mutex->owner == currentPCB) {
                    printf("Consumer 0x%lX of pair %u unlocks\n", currentPCB->pid, currentPCB->consumer->id);
                    Mutex_unlock(currentPCB->consumer->mutex, currentPCB);
                } else if (currentPCB->mutual_user && currentPCB->mutual_user->mutex1->owner == currentPCB
                        /*&& currentPCB->mutual_user->mutex2->owner == currentPCB*/) {
                    printf("Mutual 0x%lX of pair %u unlocks 1\n", currentPCB->pid, currentPCB->mutual_user->id);
                    Mutex_unlock(currentPCB->mutual_user->mutex1, currentPCB);
                }
            } else if (sysStack == currentPCB->unlock2_traps[i]) {
                if (currentPCB->mutual_user && currentPCB->mutual_user->mutex2->owner == currentPCB) {
                    printf("Mutual 0x%lX of pair %u unlocks 2\n", currentPCB->pid, currentPCB->mutual_user->id);
                    Mutex_unlock(currentPCB->mutual_user->mutex2, currentPCB);
                }
            } else if (sysStack == currentPCB->signal_traps[i]) {
                if (currentPCB->producer && currentPCB->producer->mutex->owner == currentPCB) {
                    printf("Producer 0x%lX of pair %u signals\n", currentPCB->pid, currentPCB->producer->id);
                    Cond_signal(currentPCB->producer->condc);
                } else if (currentPCB->consumer && currentPCB->consumer->mutex->owner == currentPCB) {
                    printf("Consumer 0x%lX of pair %u signals\n", currentPCB->pid, currentPCB->consumer->id);
                    Cond_signal(currentPCB->consumer->condp);
                }
            } else if (sysStack == currentPCB->wait_traps[i]) {
                if (currentPCB->producer && currentPCB->producer->mutex->owner != currentPCB
                            && !Cond_contains(currentPCB->producer->condp, currentPCB)) {
                    printf("Producer 0x%lX of pair %u waits\n", currentPCB->pid, currentPCB->producer->id);
                    Cond_wait(currentPCB->producer->condp, currentPCB->producer->mutex, currentPCB);
                } else if (currentPCB->consumer && currentPCB->consumer->mutex->owner != currentPCB
                            && !Cond_contains(currentPCB->consumer->condc, currentPCB)) {
                    printf("Consumer 0x%lX of pair %u waits\n", currentPCB->pid, currentPCB->consumer->id);
                    Cond_wait(currentPCB->consumer->condc, currentPCB->consumer->mutex, currentPCB);
                }
            } 
        }

        if (currentPCB->producer) {
            if (currentPCB->producer->mutex->owner == currentPCB || (!Cond_contains(currentPCB->producer->condp, currentPCB) && currentPCB->producer->flag == 0)) {
                if (currentPCB->producer->mutex->owner == currentPCB && currentPCB->producer->flag == 0) {
                    currentPCB->producer->data++;
                    printf("Producer 0x%lX of pair %u produces %u\n", currentPCB->pid, currentPCB->producer->id, currentPCB->producer->data);
                    currentPCB->producer->flag = 1;
                } 
                step();
            } 
        } else if (currentPCB->consumer) {
            if (currentPCB->consumer->mutex->owner == currentPCB || (!Cond_contains(currentPCB->consumer->condc, currentPCB) && currentPCB->consumer->flag == 1)) {
                if (currentPCB->consumer->mutex->owner == currentPCB && currentPCB->consumer->flag == 1) {
                    printf("Consumer 0x%lX of pair %u consumes %u\n", currentPCB->pid, currentPCB->consumer->id, currentPCB->consumer->data);
                    currentPCB->consumer->flag = 0;
                } 
                step();
            } 
        } else if (currentPCB->mutual_user) {
            if ((currentPCB->mutual_user->mutex1->owner != currentPCB && Mutex_contains(currentPCB->mutual_user->mutex1, currentPCB)) ||
                    (currentPCB->mutual_user->mutex2->owner != currentPCB && Mutex_contains(currentPCB->mutual_user->mutex2, currentPCB)) ) {

            } else {
                step();
            }
        } else {
            step();
        }
    }
}