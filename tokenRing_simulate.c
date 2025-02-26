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
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "tokenRing.h"

/*
 * This function is the body of a child process emulating a node.
 */
void
token_node(struct TokenRingData *control, int num)
	// int rcv_state = TOKEN_FLAG, not_done = 1, sending = 0, len;
	// int num;
{
	int rcv_state = TOKEN_FLAG, not_done = 1, sending = 0, len;
	unsigned char byte;

	printf("Node %d starting\n", num); //debug

	/*
	 * If this is node #0, start the ball rolling by creating the
	 * token.
	 */
	if (num == 0) {
		send_byte (control, num, TOKEN_FLAG);
	}

	/*
	 * Loop around processing data, until done.
	 */
	while (not_done) {

		if (control->shared_ptr->node[num].terminate) {
			printf("Node %d received terminate signal\n", num); //debug
			not_done = 0;
			break;
		}

		byte = rcv_byte(control, num);
	
		/*
		 * Handle the byte, based upon current state.
		 */
		switch (rcv_state) {
		case TOKEN_FLAG:
			if(byte == TOKEN_FLAG){
				if(control -> shared_ptr -> node[num].to_send.length > 0){
					sending = 1;
					rcv_state = TO;
					send_pkt(control, num);
				} else {
					send_byte(control, num, TOKEN_FLAG);
				}
				rcv_state = TOKEN_FLAG;
			} else {
				rcv_state = TO;
			}
			break;

		case TO:
			if(sending){
				send_pkt(control, num);
				rcv_state = FROM;
			} else {
				rcv_state = FROM;
			}
			break;

		case FROM:
			if(sending){
				send_pkt(control, num);
				rcv_state = LEN;
			} else {
				rcv_state = LEN;
			}
			break;

		case LEN:
			if(sending){
				send_pkt(control, num);
				rcv_state = DATA;
			} else {
				len = byte;
				rcv_state = DATA;
			}
			
			break;

		case DATA:
			if(sending){
				send_pkt(control, num);
				if(--len == 0){
					rcv_state = TOKEN_FLAG;
					sending = 0;
				}
			} else {
				if(--len == 0){
					rcv_state = TOKEN_FLAG;
				}
			}
			break;
	}

		if(control -> shared_ptr -> node[num].terminate){
			printf("Node %d recived terminate singal\n", num); //debug
			not_done = 0;
		}
	}
	printf("Node %d terminating\n", num); //debug
}

	


/*
 * This function sends a data packet followed by the token, one byte each
 * time it is called.
 */
void
send_pkt(control, num)
	struct TokenRingData *control;
	int num;
{
	static int sndpos, sndlen;

	switch (control->snd_state) {
	case TOKEN_FLAG:
		printf("Node %d sending TOKEN_FLAG\n", num); //debug
		send_byte(control, num, control -> shared_ptr -> node[num].to_send.token_flag);
		control -> snd_state = TO;
		break;

	case TO:
		printf("Node %d sending TO\n", num); //debug
		send_byte(control, num, control -> shared_ptr -> node[num].to_send.to);
		control -> snd_state = FROM;
		break;

	case FROM:
		printf("Node %d sending FROM\n", num); //debug
		send_byte(control, num, control -> shared_ptr -> node[num].to_send.from);
		control -> snd_state = LEN;
		break;

	case LEN:
		sndlen = control -> shared_ptr -> node[num].to_send.length;
		printf("Node %d sending LEN: %d\n", num, sndlen); //debug
		send_byte(control, num, sndlen);
		control -> snd_state = DATA;
		sndpos = 0;
		break;

	case DATA:
		printf("Node %d sending DATA: %d\n", num, sndpos); //debug
		send_byte(control, num, control -> shared_ptr -> node[num].to_send.data[sndpos++]);
		if(sndpos == sndlen){
			control -> snd_state = DONE;
		}
		break;

	case DONE:
		printf("Node %d sending DONE\n", num); //debug
		send_byte(control, num, TOKEN_FLAG);
		control -> snd_state = TOKEN_FLAG;
		control -> shared_ptr -> node[num].to_send.length = 0;
		SIGNAL_SEM(control, TO_SEND(num));
		break;
	}
}

/*
 * Send a byte to the next node on the ring.
 */
void
send_byte(control, num, byte)
	struct TokenRingData *control;
	int num;
	unsigned byte;
{
	printf("Node %d sending to Node %d: %d\n", num, (num + 1) %N_NODES, byte); //debug
	WAIT_SEM(control, EMPTY((num + 1) % N_NODES));
	control -> shared_ptr -> node[(num + 1) % N_NODES].data_xfer = byte;
	SIGNAL_SEM(control, FILLED((num + 1) % N_NODES));
}

/*
 * Receive a byte for this node.
 */
unsigned char
rcv_byte(control, num)
	struct TokenRingData *control;
	int num;
{
	unsigned char byte;

	WAIT_SEM(control, FILLED(num));
	byte = control -> shared_ptr -> node[num].data_xfer;
	SIGNAL_SEM(control, EMPTY(num));
	printf("Node %d received: %d\n", num, byte); //debug

	return byte;
}

