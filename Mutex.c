#include <stdlib.h>
#include <stdio.h>

#include "Mutex.h"
#include "PCB.h"
#include "PCB_Queue.h"
#include "PCB_ERRORS.h"

Mutex_p Mutex_construct() {
	Mutex_p m = malloc(sizeof(struct Mutex));
	m->owner = NULL;
	enum PCB_ERROR error = 0;
	m->queue = PCB_Queue_construct(&error);
	return m;
}

void Mutex_lock(Mutex_p m, PCB_p p) {
	if (m->owner == NULL) {
		m->owner = p;
	} else {
		enum PCB_ERROR error = 0;
		PCB_Queue_enqueue(m->queue, p, &error);
	}
}

int Mutex_trylock(Mutex_p m, PCB_p p) {
	if (m->owner == NULL) {
		m->owner = p;
		return 0;
	} else {
		return 1;
	}
}

void Mutex_unlock(Mutex_p m, PCB_p p) {
	enum PCB_ERROR error = 0;
	if (m->owner == p) {
		if (PCB_Queue_is_empty(m->queue, &error)) {
			m->owner = NULL;
		} else {
			m->owner = PCB_Queue_dequeue(m->queue, &error);
		}
	}
}

void Mutex_print(Mutex_p m) {
	printf("Mutex:");
	enum PCB_ERROR error = 0;
	if (m->owner != NULL) {
		printf(" %lu: ", m->owner->pid);
	}
	PCB_Queue_print(m->queue, &error);
}

int Mutex_contains(Mutex_p m, PCB_p p) {
	enum PCB_ERROR e = 0;
	if (m->owner == p) {
		return 1;
	}
	return PCB_Queue_contains(m->queue, p, &e);
}