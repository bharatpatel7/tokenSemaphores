/*
	NAME: Bharatkumar Garsondiya
	ID: 1303213
	MAIL: bgarsond@uoguelph.ca

*/



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
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include "tokenRing.h"

void
printHelp(const char *progname)
{
	fprintf(stderr, "%s <nPackets>\n", progname);
	fprintf(stderr, "\n");
	fprintf(stderr, "Simulates a token ring network with %d machines,\n",
			N_NODES);
	fprintf(stderr, "sending <nPackets> randomly generated packets on the\n");
	fprintf(stderr, "network before exitting and printing statistics\n");
	fprintf(stderr, "\n");
}

/**
 * Parse the command line arguments and call for setup and running
 * of the program
 */
int
main(argc, argv)
	int argc;
	const char **argv;
{
	int numPackets;
	TokenRingData *simulationData;

	if (argc < 2) {
		printHelp(argv[0]);
		exit(1);
	}

	if (sscanf(argv[1], "%d", &numPackets) != 1) {
		fprintf(stderr, "Cannot parse number of packets from '%s'\n",
				argv[1]);
		printHelp(argv[0]);
		exit(1);
	}

	if (( simulationData = setupSystem()) == NULL) {
		fprintf(stderr, "Setup failed\n");
		printHelp(argv[0]);
		exit(1);
	}

	if ( runSimulation(simulationData, numPackets) < 0) {
		fprintf(stderr, "Simulation failed\n");
		printHelp(argv[0]);
		exit(1);
	}

	if ( cleanupSystem(simulationData) < 0) {
		fprintf(stderr, "Cleanup failed\n");
		printHelp(argv[0]);
		exit(1);
	}

	exit (0);
}

