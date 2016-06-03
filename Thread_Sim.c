/*
 * Thread_Sim.c
 * Group: Hunter Bennett, Geoffrey Tanay, Christine Vu, Daniel Bayless
 * Date: May 8, 2016
 *
 *
 * Simulates the running of a processor with two I/O devices.
 * Utilizes threads to simulate the interrupt timing.
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "PCB.h"
#include "Mutex.h"
#include "Cond.h"
#include "PCB_Queue.h"
#include "PCB_Errors.h"

// times must be under 1 billion
#define SLEEP_TIME 90000000
#define IO_TIME_MIN SLEEP_TIME * 3
#define IO_TIME_MAX SLEEP_TIME * 5

#define IDL_PID 0xFFFFFFFF

#define DEADLOCK_POSSIBLE 0

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
PCB_Queue_p readyQueue;
PCB_Queue_p terminatedQueue;
PCB_Queue_p waitingQueueA;
PCB_Queue_p waitingQueueB;
PCB_p currentPCB;
PCB_p idl;
unsigned long sysStack;
enum PCB_ERROR error = PCB_SUCCESS;

void starvationPrevention() {
	
}

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
	if (!PCB_Queue_is_empty(readyQueue, &error)) {
		currentPCB = PCB_Queue_dequeue(readyQueue, &error);
	} else {
		currentPCB = idl;
	}
	printf("Switching to:\t");
	PCB_print(currentPCB, &error);
	printf("Ready Queue:\t");
	PCB_Queue_print(readyQueue, &error);
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
			printf("deadlock detected\n");
		}
	}
}

void scheduler(enum INTERRUPT_TYPE interruptType) {
	deadlock_detector();

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
		PCB_Queue_enqueue(readyQueue, p, &error);
	}
	if (interruptType == INTERRUPT_TYPE_TIMER) {
		PCB_set_state(currentPCB, PCB_STATE_READY, &error);
		if (PCB_get_pid(currentPCB, &error) != IDL_PID) {
			printf("Returned:\t");
			PCB_print(currentPCB, &error);
			PCB_Queue_enqueue(readyQueue, currentPCB, &error);
		}
		dispatcher();
	} else if (interruptType == INTERRUPT_TYPE_IO_A) {
		PCB_p p = PCB_Queue_dequeue(waitingQueueA, &error);
		printf("Moved from IO A:");
		PCB_print(p, &error);
		printf("Waiting Queue A:");
		PCB_Queue_print(waitingQueueA, &error);
		PCB_set_state(p, PCB_STATE_READY, &error);
		PCB_Queue_enqueue(readyQueue, p, &error);
	} else if (interruptType == INTERRUPT_TYPE_IO_B) {
		PCB_p p = PCB_Queue_dequeue(waitingQueueB, &error);
		printf("Moved from IO B:");
		PCB_print(p, &error);
		printf("Waiting Queue B:");
		PCB_Queue_print(waitingQueueA, &error);
		PCB_set_state(p, PCB_STATE_READY, &error);
		PCB_Queue_enqueue(readyQueue, p, &error);
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
	readyQueue = PCB_Queue_construct(&error);
	terminatedQueue = PCB_Queue_construct(&error);

	// for (int j = 0; j < 16; j++) {
	// 	PCB_p p = PCB_construct(&error);
	// 	PCB_set_pid(p, j, &error);
	// 	PCB_Queue_enqueue(createdQueue, p, &error);
	// 	printf("Created:\t");
	// 	PCB_print(p, &error);
	// }


	// for (int i = 0; i < 2; i += 2) {
	// 	PCB_p producer = PCB_construct(&error);
	// 	PCB_p consumer = PCB_construct(&error);
	// 	PCB_set_max_pc(producer, 6, &error);
	// 	PCB_set_max_pc(consumer, 6, &error);
	// 	Paired_User_p pu = malloc(sizeof(struct Paired_User));
	// 	pu->flag = 0;
	// 	pu->mutex = Mutex_construct();
	// 	pu->condc = Cond_construct();
	// 	pu->condp = Cond_construct();
	// 	pu->data = 0;
	// 	producer->producer = pu;
	// 	consumer->consumer = pu;
	// 	producer->pid = i;
	// 	consumer->pid = i + 1;
	// 	producer->lock_traps[0] = 0;
	// 	producer->wait_traps[0] = 1;
	// 	producer->unlock_traps[0] = 4;
	// 	producer->signal_traps[0] = 3;
	// 	consumer->lock_traps[0] = 0;
	// 	consumer->wait_traps[0] = 1;
	// 	consumer->unlock_traps[0] = 4;
	// 	consumer->signal_traps[0] = 3;
	// 	PCB_Queue_enqueue(createdQueue, producer, &error);
	// 	PCB_Queue_enqueue(createdQueue, consumer, &error);
	// }



	// PCB_p u1 = PCB_construct(&error);
	// PCB_p u2 = PCB_construct(&error);
	// PCB_set_max_pc(u1, 6, &error);
	// PCB_set_max_pc(u2, 6, &error);
	// Mutual_User_p mu = malloc(sizeof(struct Mutual_User));
	// mu->mutex1 = Mutex_construct();
	// mu->mutex2 = Mutex_construct();
	// mu->resource1 = 0;
	// mu->resource2 = 0;
	// u1->mutual_user = mu;
	// u2->mutual_user = mu;
	// u1->pid = 0;
	// u2->pid = 1;
	// u1->lock_traps[0] = 0;
	// u1->lock2_traps[0] = 1;
	// u1->unlock_traps[0] = 3;
	// u1->unlock2_traps[0] = 4;
	// u2->lock_traps[0] = 0;
	// u2->lock2_traps[0] = 1;
	// u2->unlock_traps[0] = 3;
	// u2->unlock2_traps[0] = 4;
	// PCB_Queue_enqueue(createdQueue, u1, &error);
	// PCB_Queue_enqueue(createdQueue, u2, &error);

	Mutex_p m1 = Mutex_construct();
	Mutex_p m2 = Mutex_construct();

	Mutual_User_p mu1 = malloc(sizeof(struct Mutual_User));
	mu1->mutex1 = m1;
	mu1->mutex2 = m2;
	mu1->resource1 = 0;
	mu1->resource2 = 0;

	Mutual_User_p mu2 = malloc(sizeof(struct Mutual_User));
	mu2->mutex1 = m2;
	mu2->mutex2 = m1;
	mu2->resource1 = 0;
	mu2->resource2 = 0;

	PCB_p u1 = PCB_construct(&error);
	PCB_p u2 = PCB_construct(&error);
	PCB_set_max_pc(u1, 6, &error);
	PCB_set_max_pc(u2, 6, &error);

	u1->mutual_user = mu1;
	u2->mutual_user = mu2;

	u1->pid = 0;
	u2->pid = 1;
	u1->lock_traps[0] = 0;
	u1->lock2_traps[0] = 1;
	u1->unlock_traps[0] = 3;
	u1->unlock2_traps[0] = 4;
	u2->lock_traps[0] = 0;
	u2->lock2_traps[0] = 1;
	u2->unlock_traps[0] = 3;
	u2->unlock2_traps[0] = 4;
	PCB_Queue_enqueue(createdQueue, u1, &error);
	PCB_Queue_enqueue(createdQueue, u2, &error);
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


		// if (currentPCB->producer) {
		// 	Mutex_print(currentPCB->producer->mutex);
		// 	Cond_print(currentPCB->producer->condp);
		// } else if (currentPCB->consumer) {
		// 	Mutex_print(currentPCB->consumer->mutex);
		// 	Cond_print(currentPCB->consumer->condc);
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
				printf("Mutual %lu uses both resources\n", currentPCB->pid);
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
					printf("Producer %lu locks\n", currentPCB->pid);
					Mutex_lock(currentPCB->producer->mutex, currentPCB);
				} else if (currentPCB->consumer && !Mutex_contains(currentPCB->consumer->mutex, currentPCB)) {
					printf("Consumer %lu locks\n", currentPCB->pid);
					Mutex_lock(currentPCB->consumer->mutex, currentPCB);
				} else if (currentPCB->mutual_user && !Mutex_contains(currentPCB->mutual_user->mutex1, currentPCB)) {
					printf("Mutual %lu locks 1\n", currentPCB->pid);
					Mutex_lock(currentPCB->mutual_user->mutex1, currentPCB);
				}
			} else if (sysStack == currentPCB->lock2_traps[i]) {
				if (currentPCB->mutual_user && !Mutex_contains(currentPCB->mutual_user->mutex2, currentPCB)
							&& currentPCB->mutual_user->mutex1->owner == currentPCB) {
					printf("Mutual %lu locks 2\n", currentPCB->pid);
					Mutex_lock(currentPCB->mutual_user->mutex2, currentPCB);
				}
			} else if (sysStack == currentPCB->unlock_traps[i]) {
				if (currentPCB->producer && currentPCB->producer->mutex->owner == currentPCB) {
					printf("Producer %lu unlocks\n", currentPCB->pid);
					Mutex_unlock(currentPCB->producer->mutex, currentPCB);
				} else if (currentPCB->consumer && currentPCB->consumer->mutex->owner == currentPCB) {
					printf("Consumer %lu unlocks\n", currentPCB->pid);
					Mutex_unlock(currentPCB->consumer->mutex, currentPCB);
				} else if (currentPCB->mutual_user && currentPCB->mutual_user->mutex1->owner == currentPCB
						&& currentPCB->mutual_user->mutex2->owner == currentPCB) {
					printf("Mutual %lu unlocks 1\n", currentPCB->pid);
					Mutex_unlock(currentPCB->mutual_user->mutex1, currentPCB);
				}
			} else if (sysStack == currentPCB->unlock2_traps[i]) {
				if (currentPCB->mutual_user && currentPCB->mutual_user->mutex2->owner == currentPCB) {
					printf("Mutual %lu unlocks 2\n", currentPCB->pid);
					Mutex_unlock(currentPCB->mutual_user->mutex2, currentPCB);
				}
			} else if (sysStack == currentPCB->signal_traps[i]) {
				if (currentPCB->producer && currentPCB->producer->mutex->owner == currentPCB) {
					printf("Producer %lu signal\n", currentPCB->pid);
					Cond_signal(currentPCB->producer->condc);
				} else if (currentPCB->consumer && currentPCB->consumer->mutex->owner == currentPCB) {
					printf("Consumer %lu signal\n", currentPCB->pid);
					Cond_signal(currentPCB->consumer->condp);
				}
			} else if (sysStack == currentPCB->wait_traps[i]) {
				if (currentPCB->producer && currentPCB->producer->mutex->owner != currentPCB
							&& !Cond_contains(currentPCB->producer->condp, currentPCB)) {
					printf("Producer %lu waits\n", currentPCB->pid);
					Cond_wait(currentPCB->producer->condp,
							currentPCB->producer->mutex, currentPCB);
				} else if (currentPCB->consumer && currentPCB->consumer->mutex->owner != currentPCB
							&& !Cond_contains(currentPCB->consumer->condc, currentPCB)) {
					printf("Consumer %lu waits\n", currentPCB->pid);
					Cond_wait(currentPCB->consumer->condc,
							currentPCB->consumer->mutex, currentPCB);
				}
			}
		}

		if (currentPCB->producer) {
			if (currentPCB->producer->mutex->owner == currentPCB || (!Cond_contains(currentPCB->producer->condp, currentPCB) && currentPCB->producer->flag == 0)) {
			    if (currentPCB->producer->mutex->owner == currentPCB && currentPCB->producer->flag == 0) {
					currentPCB->producer->data++;
					printf("Producer %lu produces %u\n", currentPCB->pid, currentPCB->producer->data);
					currentPCB->producer->flag = 1;
				}
				step();
			}
		} else if (currentPCB->consumer) {
			if (currentPCB->consumer->mutex->owner == currentPCB || (!Cond_contains(currentPCB->consumer->condc, currentPCB) && currentPCB->consumer->flag == 1)) {
				if (currentPCB->consumer->mutex->owner == currentPCB && currentPCB->consumer->flag == 1) {
					printf("Consumer %lu consumes %u\n", currentPCB->pid, currentPCB->consumer->data);
					currentPCB->consumer->flag = 0;
				}
				step();
			}
		} else {
			step();
		}
	}
}

//producer-consumer only unlock if flag
