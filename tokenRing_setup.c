/*
 * The program simulates a Token Ring LAN by forking off a process
 * for each LAN node, that communicate via shared memory, instead
 * of network cables. To keep the implementation simple, it jiggles
 * out bytes instead of bits.
 *
 * It keeps a count of packets sent and received for each node.
 */
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "tokenRing.h"

/*
 * The main program creates the shared memory region and forks off the
 * processes to emulate the token ring nodes.
 * This process generates packets at random and inserts them in
 * the to_send field of the nodes. When done it waits for each process
 * to be done receiving and then tells them to terminate and waits
 * for them to die and prints out the sent/received counts.
 */
struct TokenRingData *
setupSystem()
{
	register int i;
	struct TokenRingData *control;

	control = (struct TokenRingData *) malloc(sizeof(struct TokenRingData));

	/*
	 * Seed the random number generator.
	 */
	srandom(time(0));

	/*
	 * Create the shared memory region.
	 */
	control->shmid = shmget(IPC_PRIVATE, sizeof (struct shared_data), 0600);
	if (control->shmid < 0) {
		fprintf(stderr, "Can't create shared memory region\n");
		goto FAIL;
	}

	/*
	 * and map the shared data region into our address space at an
	 * address selected by Linux.
	 */
	control->shared_ptr = (struct shared_data *)
	    	shmat(control->shmid, (char *)0, 0);
	if (control->shared_ptr == (struct shared_data *)0) {
		fprintf(stderr, "Can't map shared memory region\n");
		goto FAIL;
	}

	/*
	 * Now, create the semaphores, by creating the semaphore set.
	 * Under Linux, semaphore sets are stored in an area of memory
	 * handled much like a shared region. (A semaphore set is simply
	 * a bunch of semaphores allocated to-gether.)
	 */
	control->semid = semget(IPC_PRIVATE, NUM_SEM, 0600);
	if (control->semid < 0) {
		fprintf(stderr, "Can't create semaphore set\n");
		goto FAIL;
	}

	/*
	 * and initialize them.
	 * Semaphores are meaningless if they are not initialized.
	 */
	for (i = 0; i < N_NODES; i++) {
		INITIALIZE_SEM(control, EMPTY(i), 1);
		INITIALIZE_SEM(control, FILLED(i), 0);
		INITIALIZE_SEM(control, TO_SEND(i), 0);
	}
	INITIALIZE_SEM(control, CRIT, 1);

	/*
	 * And initialize the shared data
	 */
	for (i = 0; i < N_NODES; i++) {
		control -> shared_ptr -> node[i].data_xfer = 0;
		control -> shared_ptr -> node[i].to_send.token_flag = 0;
		control -> shared_ptr -> node[i].to_send.to = 0;
		control -> shared_ptr -> node[i].to_send.from = 0;
		control -> shared_ptr -> node[i].to_send.length = 0;
		control -> shared_ptr -> node[i].sent = 0;
		control -> shared_ptr -> node[i].received = 0;
		control -> shared_ptr -> node[i].terminate = 0;
	}

#ifdef DEBUG
	fprintf(stderr, "main after initialization\n");
#endif

	return control;

FAIL:
	free(control);
	return NULL;
}


int
runSimulation(control, numberOfPackets)
	struct TokenRingData *control;
	int numberOfPackets;
{
	int num, to;
	int i;

	/*
	 * Fork off the children that simulate the disks.
	 */
	for (i = 0; i < N_NODES; i++){
		if (fork() == 0) {
			token_node(control, i);
			exit(0);
		}
	}

	/*
	 * Loop around generating packets at random.
	 * (the parent)
	 */
	for (i = 0; i < numberOfPackets; i++) {
		/*
		 * Add a packet to be sent to to_send for that node.
		 */
#ifdef DEBUG
		fprintf(stderr, "Main in generate packets\n");
#endif
		num = random() % N_NODES;
		WAIT_SEM(control, TO_SEND(num));
		WAIT_SEM(control, CRIT);
		if (control->shared_ptr->node[num].to_send.length > 0)
			panic("to_send filled\n");

		control->shared_ptr->node[num].to_send.token_flag = '0';


		do {
			to = random() % N_NODES;
		} while (to == num);

		control->shared_ptr->node[num].to_send.to = (char)to;
		control->shared_ptr->node[num].to_send.from = (char)num;
		control->shared_ptr->node[num].to_send.length = (random() % MAX_DATA) + 1;
		SIGNAL_SEM(control, CRIT);
	}

	return 1;
}

int
cleanupSystem(control)
	struct TokenRingData *control;
{
    	int child_status;
	union semun zeroSemun;
	int i;

	bzero(&zeroSemun, sizeof(union semun));
	/*
	 * Now wait for all nodes to finish sending and then tell them
	 * to terminate.
	 */
	for (i = 0; i < N_NODES; i++)
		WAIT_SEM(control, TO_SEND(i));
	WAIT_SEM(control, CRIT);
	for (i = 0; i < N_NODES; i++) {
		control->shared_ptr->node[i].terminate = 1;
		printf("Setting terminate flag for Node %d\n", i); //debug
	}
	SIGNAL_SEM(control, CRIT);

#ifdef DEBUG
	fprintf(stderr, "wait for children to terminate\n");
#endif
	/*
	 * Wait for the node processes to terminate.
	 */
	for (i = 0; i < N_NODES; i++)
		wait(&child_status);

	/*
	 * All done, just print out the results.
	 */
	for (i = 0; i < N_NODES; i++)
		printf("Node %d: sent=%d received=%d\n", i,
			control->shared_ptr->node[i].sent,
			control->shared_ptr->node[i].received);
#ifdef DEBUG
	fprintf(stderr, "parent at destroy shared memory\n");
#endif
	/*
	 * And destroy the shared data area and semaphore set.
	 * First detach the shared memory segment via shmdt() and then
	 * destroy it with shmctl() using the IPC_RMID command.
	 * Destroy the semaphore set in a similar manner using a semctl()
	 * call with the IPC_RMID command.
	 */
	shmdt((char *)control->shared_ptr);
	shmctl(control->shmid, IPC_RMID, (struct shmid_ds *)0);
	semctl(control->semid, 0, IPC_RMID, zeroSemun);

	return 1;
}


/*
 * Panic: Just print out the message and exit.
 */
void
panic(const char *fmt, ...)
{
    	va_list vargs;

	va_start(vargs, fmt);
	(void) vfprintf(stdout, fmt, vargs);
	va_end(vargs);

	exit(5);
}

