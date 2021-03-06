#include "PCB.h"
#include "PCB_Errors.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>

PCB_p PCB_construct(enum PCB_ERROR *error) {
	PCB_p p = malloc(sizeof(struct PCB));
	if (p == NULL) {
		*error = PCB_MEM_ALLOC_FAIL;
		return NULL;
	}
	p->priority_boost = 0;
	PCB_set_pid(p, 0, error);
	PCB_set_state(p, PCB_STATE_NEW, error);
	PCB_set_priority(p, PCB_PRIORITY_MAX, error);
	PCB_set_pc(p, 0, error);
	PCB_set_max_pc(p, 5000000, error);
	PCB_set_terminate(p, 0, error);
	PCB_set_term_count(p, 0, error);
	PCB_set_creation(p, time(NULL), error);
	set_last_executed(p, error);
	PCB_set_termination(p, 0, error);
	for (int i = 0; i < PCB_TRAP_LENGTH; i++) {
		p->io_1_traps[i] = 6000000;
		p->io_2_traps[i] = 6000000;
		p->lock_traps[i] = 6000000;
		p->unlock_traps[i] = 6000000;
		p->signal_traps[i] = 6000000;
		p->wait_traps[i] = 6000000;
	}

	p->producer = NULL;
	p->consumer = NULL;
	p->mutual_user = NULL;

	return p;
}

void PCB_destruct(PCB_p p, enum PCB_ERROR *error) {
	free(p);
}

void PCB_init(PCB_p p, enum PCB_ERROR *error) {
}

void PCB_set_pid(PCB_p p, unsigned long pid, enum PCB_ERROR *error) {
	if (p == NULL) {
		*error = PCB_NULL_POINTER;
		return;
	}
	p->pid = pid;
}

void PCB_set_state(PCB_p p, enum PCB_STATE_TYPE state, enum PCB_ERROR *error) {
	if (p == NULL) {
		*error = PCB_NULL_POINTER;
		return;
	}
	if (state < 0 || state >= PCB_STATE_LAST_ERROR) {
		*error = PCB_INVALID_ARG;
		return;
	}
	p->state = state;
}

void PCB_set_priority(PCB_p p, unsigned short priority, enum PCB_ERROR *error) {
	if (p == NULL) {
		*error = PCB_NULL_POINTER;
		return;
	}
	if (priority > PCB_PRIORITY_MAX) {
		*error = PCB_INVALID_ARG;
		return;
	}
	p->priority = priority;
}

void PCB_set_pc(PCB_p p, unsigned long pc, enum PCB_ERROR *error) {
	if (p == NULL) {
		*error = PCB_NULL_POINTER;
		return;
	}
	p->pc = pc;
}

void PCB_set_sw(PCB_p p, unsigned int i, enum PCB_ERROR *error) {
	if (p == NULL) {
		*error = PCB_NULL_POINTER;
		return;
	}
	p->sw = i;
}
void PCB_set_max_pc(PCB_p p, unsigned long i, enum PCB_ERROR *error) {
	if (p == NULL) {
		*error = PCB_NULL_POINTER;
		return;
	}
	p->max_pc = i;
}
void PCB_set_creation(PCB_p p, unsigned long i, enum PCB_ERROR *error) {
	if (p == NULL) {
		*error = PCB_NULL_POINTER;
		return;
	}
	p->creation = i;
}
void PCB_set_termination(PCB_p p, unsigned long i, enum PCB_ERROR *error) {
	if (p == NULL) {
		*error = PCB_NULL_POINTER;
		return;
	}
	p->termination = i;
}
void PCB_set_terminate(PCB_p p, unsigned int i, enum PCB_ERROR *error) {
	if (p == NULL) {
		*error = PCB_NULL_POINTER;
		return;
	}
	p->terminate = i;
}
void PCB_set_term_count(PCB_p p, unsigned int i, enum PCB_ERROR *error) {
	if (p == NULL) {
		*error = PCB_NULL_POINTER;
		return;
	}
	p->term_count = i;
}

unsigned long PCB_get_pid(PCB_p p, enum PCB_ERROR *error) {
	if (p == NULL) {
		*error = PCB_NULL_POINTER;
		return 0;
	}
	return p->pid;
}

enum PCB_STATE_TYPE PCB_get_state(PCB_p p, enum PCB_ERROR *error) {
	if (p == NULL) {
		*error = PCB_NULL_POINTER;
		return PCB_STATE_ERROR;
	}
	return p->state;
}

unsigned short PCB_get_priority(PCB_p p, enum PCB_ERROR *error) {
	if (p == NULL) {
		*error = PCB_NULL_POINTER;
		return 0;
	}
	return p->priority - p->priority_boost;
}

unsigned long PCB_get_pc(PCB_p p, enum PCB_ERROR *error) {
	if (p == NULL) {
		*error = PCB_NULL_POINTER;
		return 0;
	}
	return p->pc;
}

unsigned int PCB_get_sw(PCB_p p, enum PCB_ERROR *error) {
	if (p == NULL) {
		*error = PCB_NULL_POINTER;
		return 0;
	}
	return p->sw;
}
unsigned long PCB_get_max_pc(PCB_p p, enum PCB_ERROR *error) {
	if (p == NULL) {
		*error = PCB_NULL_POINTER;
		return 0;
	}
	return p->max_pc;
}

unsigned long PCB_get_creation(PCB_p p, enum PCB_ERROR *error) {
	if (p == NULL) {
		*error = PCB_NULL_POINTER;
		return 0;
	}
	return p->creation;
}

unsigned long PCB_get_termination(PCB_p p, enum PCB_ERROR *error) {
	if (p == NULL) {
		*error = PCB_NULL_POINTER;
		return 0;
	}
	return p->termination;
}

unsigned int PCB_get_terminate(PCB_p p, enum PCB_ERROR *error) {
	if (p == NULL) {
		*error = PCB_NULL_POINTER;
		return 0;
	}
	return p->terminate;
}

unsigned int PCB_get_term_count(PCB_p p, enum PCB_ERROR *error) {
	if (p == NULL) {
		*error = PCB_NULL_POINTER;
		return 0;
	}
	return p->term_count;
}

void PCB_print(PCB_p p, enum PCB_ERROR *error) {
	if (p == NULL) {
		*error = PCB_NULL_POINTER;
		return;
	}
	printf("PID: 0x%lX, Priority: 0x%X, State: %u, PC: 0x%lX, MaxPC: 0x%lx, Terminate: %u, TermCount: %u\n",
			PCB_get_pid(p, error), PCB_get_priority(p, error),
			PCB_get_state(p, error), PCB_get_pc(p, error),
			PCB_get_max_pc(p, error), PCB_get_terminate(p, error),
			PCB_get_term_count(p, error));
}

void PCB_set_boosting(PCB_p p, bool b, enum PCB_ERROR *error) {
	if (p == NULL) {
		*error = PCB_NULL_POINTER;
		return;
	}
	p->priority_boost = b;
}

bool PCB_is_boosting(PCB_p p, enum PCB_ERROR *error) {
	if (p == NULL) {
		*error = PCB_NULL_POINTER;
		return 0;
	}
	return p->priority_boost;
}

void set_last_executed(PCB_p p, enum PCB_ERROR *error) {
	if (p == NULL) {
		*error = PCB_NULL_POINTER;
	} else {
		p->last_executed = time(NULL);
	}
}

bool duration_met(PCB_p p, int seconds, enum PCB_ERROR *error) {
	if (p == NULL) {
		*error = PCB_NULL_POINTER;
		return 0;
	}
	return time(NULL) - p->last_executed >= seconds;
}
