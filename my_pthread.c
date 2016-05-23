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

}
int my_cond_wait(my_cond_p the_cond, my_mutex_p the_mutex) {

}
int my_cond_signal(my_cond_p the_cond) {

}