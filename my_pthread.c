/*
 * Group 3 OS Project
 * Spring 2016
 * Authors: Antonio Alvillar, Bethany Eastman, Gabriel Houle & Edgardo Gutierrez Jr.
 * GitHub: https://github.com/ghoule636/OSProject
 */

void my_mutex_init(my_mutex_p the_mutex) {
	the_mutex->owner = NULL;
	the_mutex->blockedList = FIFOq_construct();
}

int my_mutex_lock(my_mutex_p the_mutex, PCB_p the_pcb) {
	if (the_mutex->owner == NULL) {
		the_mutex->owner = the_pcb;
	} else {
		FIFOq_enqueue(the_mutex->blockedList, the_pcb);
	}
	return 0;
}


int my_mutex_trylock(my_mutex_p the_mutex, PCB_p the_pcb) {
	if (the_mutex->owner == NULL) {
		the_mutex->owner = the_pcb;
		return 0;
	} else {
		return 1;
	}
}

int my_mutex_unlock(my_mutex_p the_mutex) {
	if (FIFOq_is_empty(the_mutex->blockedList)) {
		the_mutex->owner = NULL;
	} else {
		the_mutex->owner = FIFOq_dequeue(the_mutex->blockedList);
	}
	return 0;
}

void my_cond_init(my_cond_p the_cond) {
	the_cond->head = NULL;
}

int my_cond_wait(my_cond_p the_cond, my_mutex_p the_mutex, PCB_p the_pcb) {
	cond_list cl = malloc(sizeof(my_cond_node));
	cl->owner = the_pcb;
	cl->lock = the_mutex;
	cl->next = NULL;
	if (the_cond->head == NULL) {
		the_cond->head = cl;
	} else {
		cond_list curr = the_cond->head;
		while (curr->next != NULL) {
			curr = curr->next;
		}
		curr->next = cl;
	}
	return 0;
}

int my_cond_signal(my_cond_p the_cond) {
	if (the_cond->head != NULL) {
		cond_list cl = the_cond->head;
		my_mutex_lock(cl->lock, cl->owner);
		the_cond->head = cl->next;
		free(cl);
	}
	return 0;
}