#include <stdlib.h>
#include <stdio.h>
#include "Mutex.h"
#include "PCB.h"
#include "Cond.h"

Cond_p Cond_construct() {
	Cond_p c = malloc(sizeof(struct Cond));
	c->head = NULL;
	return c;
}

void Cond_signal(Cond_p c) {
	if (c->head != NULL) {
		struct Cond_Node* cn = c->head;
		if (!Mutex_contains(cn->mutex, cn->owner)) {
			Mutex_lock(cn->mutex, cn->owner);
		}
		c->head = cn->next;
		free(cn);
	}
}

void Cond_wait(Cond_p c, Mutex_p m, PCB_p p) {
	Mutex_unlock(m, p);
	struct Cond_Node* cn = malloc(sizeof(struct Cond_Node));
	cn->owner = p;
	cn->mutex = m;
	cn->next = NULL;
	if (c->head == NULL) {
		c->head = cn;
	} else {
		struct Cond_Node* curr = c->head;
		while (curr->next != NULL) {
			curr = curr->next;
		}
		curr->next = cn;
	}
}

void Cond_print(Cond_p c) {
	printf("Cond: ");
	struct Cond_Node* cn = c->head;
	while (cn != NULL) {
		printf("%lu->", cn->owner->pid);
		cn = cn->next;
	}
	printf("\n");
}

int Cond_contains(Cond_p c, PCB_p p) {
	struct Cond_Node *cn = c->head;
	while (cn != NULL) {
		if (cn->owner == p) {
			return 1;
		}
		cn = cn->next;
	}
	return 0;
}