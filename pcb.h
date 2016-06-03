#pragma once

#include <stdbool.h>
#include "PCB_Errors.h"
// #include "Mutex.h"
// #include "Cond.h"

#define PCB_PRIORITY_MAX 15
#define PCB_TRAP_LENGTH 4

enum PCB_STATE_TYPE {
	PCB_STATE_NEW = 0, 
	PCB_STATE_READY, 
	PCB_STATE_RUNNING, 
	PCB_STATE_INTERRUPTED, 
	PCB_STATE_WAITING, 
	PCB_STATE_HALTED,

	PCB_STATE_LAST_ERROR, // invalid type, used for bounds checking

	PCB_STATE_ERROR // Used as return value if null pointer is passed to getter.
};

struct Paired_User {
	int flag;
	struct Mutex *mutex;
	struct Cond *condc;
	struct Cond *condp;
	int data;
};

typedef struct Paired_User* Paired_User_p;

struct Mutual_User {
	struct Mutex *mutex1;
	struct Mutex *mutex2;
	int resource1;
	int resource2;
};

typedef struct Mutual_User *Mutual_User_p;

struct PCB {
    unsigned long pid;        // process ID #, a unique number
	unsigned short priority;  // priorities 0=highest, 15=lowest
	enum PCB_STATE_TYPE state;    // process state (running, waiting, etc.)
	unsigned long pc;         // holds the current pc value when preempted
	unsigned int sw;
	unsigned long max_pc;
	unsigned long creation;
	unsigned long termination;
	unsigned int terminate;
	unsigned int term_count;
	unsigned long io_1_traps[PCB_TRAP_LENGTH];
	unsigned long io_2_traps[PCB_TRAP_LENGTH];

	bool priority_boost;

	Paired_User_p producer;
	Paired_User_p consumer;
	Mutual_User_p mutual_user;

	unsigned long lock_traps[PCB_TRAP_LENGTH];
	unsigned long lock2_traps[PCB_TRAP_LENGTH];
	unsigned long unlock_traps[PCB_TRAP_LENGTH];
	unsigned long unlock2_traps[PCB_TRAP_LENGTH];
	unsigned long signal_traps[PCB_TRAP_LENGTH];
	unsigned long wait_traps[PCB_TRAP_LENGTH];
};

typedef struct PCB * PCB_p;

PCB_p PCB_construct(enum PCB_ERROR*); // returns a pcb pointer to heap allocation
void PCB_destruct(PCB_p, enum PCB_ERROR*);  // deallocates pcb from the heap
void PCB_init(PCB_p, enum PCB_ERROR*);       // sets default values for member data

void PCB_set_pid(PCB_p, unsigned long, enum PCB_ERROR*);///////
void PCB_set_priority(PCB_p, unsigned short, enum PCB_ERROR*);
void PCB_set_state(PCB_p, enum PCB_STATE_TYPE, enum PCB_ERROR*);
void PCB_set_pc(PCB_p, unsigned long, enum PCB_ERROR*);
void PCB_set_sw(PCB_p, unsigned int, enum PCB_ERROR*);
void PCB_set_max_pc(PCB_p, unsigned long, enum PCB_ERROR*);
void PCB_set_creation(PCB_p, unsigned long, enum PCB_ERROR*);
void PCB_set_termination(PCB_p, unsigned long, enum PCB_ERROR*);
void PCB_set_terminate(PCB_p, unsigned int, enum PCB_ERROR*);
void PCB_set_term_count(PCB_p, unsigned int, enum PCB_ERROR*);
void PCB_set_boosting(PCB_p, bool, enum PCB_ERROR*);

unsigned long PCB_get_pid(PCB_p, enum PCB_ERROR*);  // returns pid of the process
unsigned short PCB_get_priority(PCB_p, enum PCB_ERROR*);
enum PCB_STATE_TYPE PCB_get_state(PCB_p, enum PCB_ERROR*);
unsigned long PCB_get_pc(PCB_p, enum PCB_ERROR*);
unsigned int PCB_get_sw(PCB_p, enum PCB_ERROR*);
unsigned long PCB_get_max_pc(PCB_p, enum PCB_ERROR*);
unsigned long PCB_get_creation(PCB_p, enum PCB_ERROR*);
unsigned long PCB_get_termination(PCB_p, enum PCB_ERROR*);
unsigned int PCB_get_terminate(PCB_p, enum PCB_ERROR*);
unsigned int PCB_get_term_count(PCB_p, enum PCB_ERROR*);
bool PCB_is_boosting(PCB_p, enum PCB_ERROR*);

void PCB_print(PCB_p, enum PCB_ERROR*);  // a string representing the contents of the pcb

