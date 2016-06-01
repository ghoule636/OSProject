CFLAGS=-Wall -std=c99




all: Thread_Sim


Thread_Sim: Thread_Sim.o PCB.o PCB_Queue.o Mutex.o Cond.o
	gcc Thread_Sim.o PCB.o PCB_Queue.o Mutex.o Cond.o -o Thread_Sim





Thread_Sim.o: Thread_Sim.c
	gcc $(CFLAGS) -c Thread_Sim.c
Mutex.o: Mutex.c
	gcc $(CFLAGS) -c Mutex.c
Cond.o: Cond.c
	gcc $(CFLAGS) -c Cond.c
PCB_Queue.o: PCB_Queue.c
	gcc $(CFLAGS) -c PCB_Queue.c 
PCB.o: PCB.c
	gcc $(CFLAGS) -c PCB.c