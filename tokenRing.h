#ifndef __TOKEN_CONTROL_HEADER__
#define __TOKEN_CONTROL_HEADER__

/*
 * Define any handy constants and structures.
 * Also define the functions.
 */
#define	MAX_DATA	250
#define	TOKEN_FLAG	1
#define	TO		2
#define	FROM		3
#define	LEN		4
#define	DATA		5
#define	DONE		6


#define	N_NODES		7


struct data_pkt {
	char		token_flag;	/* '1' for token, '0' for data	*/
	char		to;		/* Destination node #		*/
	char		from;		/* Source node #		*/
	unsigned char	length;		/* Data length 1<->MAX_DATA	*/
	char		data[MAX_DATA];	/* Up to MAX_DATA bytes of data	*/
};

/*
 * The shared memory region includes a byte used to transfer data between
 * the nodes, a packet structure for each node with a packet to send and
 * sent/received counts for the nodes.
 */
struct node_data {
	unsigned char	data_xfer;
	struct data_pkt	to_send;
	int		sent;
	int		received;
	int		terminate;
};

struct shared_data {
	struct node_data node[N_NODES];
};

/*
 * Assign a number/name to each semaphore.
 * Semaphores are used to co-ordinate access to the data_xfer and
 * to_send shared data structures and also to indicate when data transfers
 * occur between nodes.
 * Macros with the node # as argument are used to access the sets of
 * semaphores.
 */
#define	EMPTY0		0
#define	FILLED0		(N_NODES)
#define	TO_SEND0	(FILLED0 + N_NODES)
#define	CRIT		(TO_SEND0 + N_NODES)
#define	NUM_SEM		(CRIT + 1)

#define	EMPTY(n)	(EMPTY0 + (n))
#define	FILLED(n)	(FILLED0 + (n))
#define	TO_SEND(n)	(TO_SEND0 + (n))

/*
 * The Linux semaphore ops are done using the fields of sembuf.
 * sem_num - Which semaphore of the set, as defined below
 * sem_op  - set to 1 for "signal" and -1 for "wait"
 * sem_flg - set to 0 for our purposes
 * The macros WAIT_SEM, SIGNAL_SEM and INITIALIZE_SEM are defined
 * to, hopefully, simplify the code.
 */
/*
 * POSIX Now says this can't be in sem.h, so we have to put it in
 * ourselves? (It was in sys/sem.h in RedHat 5.2)
 */
union semun {
	int val;
	struct semid_ds *buf;
	unsigned short int *array;
	struct seminfo *__buf;
};


#define	WAIT_SEM(c,s) { \
	struct sembuf sb; \
	sb.sem_num = (s); \
	sb.sem_op = -1; \
	sb.sem_flg = 0; \
	if (semop((c)->semid, &sb, 1) < 0) { \
		fprintf(stderr, "Wait sem failed errno=%d\n", errno); \
		exit(4); \
	} }

#define	SIGNAL_SEM(c,s) { \
	struct sembuf sb; \
	sb.sem_num = (s); \
	sb.sem_op = 1; \
	sb.sem_flg = 0; \
	if (semop((c)->semid, &sb, 1) < 0) { \
		fprintf(stderr, "Signal sem failed errno=%d\n", errno); \
		exit(4); \
	} }

#define INITIALIZE_SEM(c, s, n) { \
	union semun semarg; \
	semarg.val = (n); \
	if (semctl((c)->semid, (s), SETVAL, semarg) < 0) { \
		fprintf(stderr, "Initialize sem failed errno=%d\n", errno); \
		exit(4); \
	} }

typedef struct TokenRingData {
    int semid;
    int shmid;
    int snd_state;
    struct shared_data *shared_ptr;
} TokenRingData;

/** prototypes */
void panic(const char *fmt, ...);

struct TokenRingData *setupSystem();
int runSimulation(struct TokenRingData *simulationData, int numPackets);
int cleanupSystem(struct TokenRingData *simulationData);

unsigned char rcv_byte(struct TokenRingData *control, int num);
void send_byte(struct TokenRingData *control, int num, unsigned byte);
void send_pkt(struct TokenRingData *control, int num);
void token_node(struct TokenRingData *control, int num);

#endif /* __TOKEN_CONTROL_HEADER__ */
