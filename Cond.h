#pragma once

#include "Mutex.h"
#include "PCB.h"

struct Cond_Node {
	PCB_p owner;
	Mutex_p mutex;
	struct Cond_Node* next;
};

struct Cond {
	struct Cond_Node* head;
};

typedef struct Cond * Cond_p;

Cond_p Cond_construct();
void Cond_signal(Cond_p);
void Cond_wait(Cond_p, Mutex_p, PCB_p);
void Cond_print(Cond_p);
int Cond_contains(Cond_p, PCB_p);