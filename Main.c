#include "Mutex.h"
#include "PCB_ERRORS.h"
#include "PCB.h"
#include <stdio.h>

enum PCB_ERROR error = PCB_SUCCESS;

int main() {
	Mutex_p m = Mutex_construct();
	PCB_p p = PCB_construct(&error);
	Mutex_lock(m, p);
	Mutex_unlock(m, p);
	PCB_print(p, &error);
	printf("a");

}