#pragma once

#include "PCB.h"
#include "PCB_Queue.h"

struct Mutex {
	PCB_p owner;
	PCB_Queue_p queue;
};

typedef struct Mutex * Mutex_p;

Mutex_p Mutex_construct();
void Mutex_lock(Mutex_p, PCB_p);
int Mutex_trylock(Mutex_p, PCB_p);
void Mutex_unlock(Mutex_p, PCB_p);
void Mutex_print(Mutex_p);
int Mutex_contains(Mutex_p, PCB_p);