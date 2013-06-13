/*
 ============================================================================
 Name        : Trunks_Qin_Qout_client_udp_SIG_RT_MT.c
 Author      :  Manuel Gómez Sobradelo
 Version     :
 Copyright   : copyright notice
 Description : message in a bottle C, Ansi-style
 ============================================================================
 */

/**
 *
 *	@short	: A solution to the Concurrent , Realtime multi-TRUNK's ETHER problem.
 *	@Short	: This also addresses bidirectional communication with bounded buffers.
 *	@Short	: Another covered issue is the interleaving of RT Event signaling with
 *	@Short	: Multithread concurrency.
 *	Short	: Each ETHER maintains 2 bounded circular FIFOs for output /input
 *	Short	: Each TRUNK consumes synchronously the ETHER output circular FIFO
 *	Short	: and after processing data (sending messages to an echo server down the line)
 *	Short	: every TRUNK posts back results to the ETHER's input FIFO
 *	Short	: At this moment a RT signal is raised and the handling function traces the
 *	Short	: RT-event and its associated value to a file for further exploitation.
 *
 *	Short	: Every ETHER/TRUNK  has some differentiated properties:
 *	- Different internal buffer processing Capacity
 *	- Processing delays to mimic time-lags or various different features
 *	- down the field.
 */

#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "udpclient.h"
/*
 ============================================================================
 *
 * constants
 *
 ============================================================================
 */
#define MAXCOUNT 5

#define NUMTRUNKS	4
#define TRUNK1   50000
#define TRUNK2   100000
#define TRUNK3   400000
#define TRUNK4   800000
/*
 #define TRUNK1   50000
 #define TRUNK2   50000
 #define TRUNK3   50000
 #define TRUNK4   50000
 */
#define ETHER1   50000

#define QUEUESIZE 32
#define CAPACITYTRUNK 6
#define CAPACITYETHER (CAPACITYTRUNK*4)

#define SIGRTMIN_1 SIGRTMIN+1
/*
 ============================================================================
 *
 * Structures
 *
 ============================================================================
 */
typedef struct {
	int buf[QUEUESIZE];
	long head, tail;
	int full, empty;
	pthread_mutex_t *mut;
	pthread_cond_t *writeOK, *readOK;
	int ETHERs;
	int TRUNKs;
	int waiting2Write;
	int waiting2Read;
} queue;

typedef struct _ETHER {
	queue *qOUT, *qIN;
	int id;

	unsigned int delay;
	unsigned int Capacity;//bytes
} ETHER;

typedef struct _TRUNK {
	queue* qIN, *qOUT;
	int id;
	client* conn;
	unsigned int delay;//ms
	unsigned int Capacity;//bytes
} TRUNK;

typedef struct _argumentos {
	ETHER *lock;
	queue* fifoOUT, *fifoIN;
	int id;
	long delay;
	unsigned int Capacity;
} argumentos;

typedef struct _argumentosTRUNK {
	TRUNK *lock;
	queue* fifoIN, *fifoOUT, *internalBUF;
	int id;
	long delay;
	unsigned int Capacity;
} argumentosTRUNK;

/*
 ============================================================================
 *
 * Definitions
 *
 ============================================================================
 */
queue *queueInit(void);
void queueDelete(queue *q);
void queueAdd(queue *q, int in);
void queueDel(queue *q, int *out);

ETHER *initlock(void);
TRUNK *initTRUNK(void);
void readlock(ETHER *lock, int d);
void writelock(ETHER *lock, int d);
void readunlock(ETHER *lock);
void writeunlock(ETHER *lock);
void deletelock(ETHER *lock);

void readlockTr(TRUNK *lock, int d);
void readunlockTr(TRUNK *lock);
void writelockTr(TRUNK *lock, int d);
void writeunlockTr(TRUNK *lock);

argumentos *newRWargs(ETHER *l, int i, long d, unsigned int capac,
		queue* fifoOUT, queue* fifoIN);
argumentosTRUNK *newRWargsTr(TRUNK *l, int i, long d, unsigned int capac,
		queue* fifoIN, queue* fifoOUT);

void *runTRUNK(void *args);
void *runETHER(void *args);

/*
 ============================================================================
 *
 * Real Time section
 *
 ============================================================================
 */
/**
 * @brief prints a message in the console
 * @param format
 */
void PrintMessage(const char * format, ...) {
	char buffer[256] = { 0 };
	va_list args;
	va_start (args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	perror(buffer);
	va_end (args);
}
/**
 * @brief Prints a message in a file
 * @param fd file descriptor
 * @param format
 */
void PrintMessageFd(int fd, const char * format, ...) {
	char buffer[25] = { 0 };
	va_list args;
	va_start (args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	int readbytes = write((int) fd, buffer, sizeof(buffer));
	if (readbytes < 0)
		PrintMessage("\n\twrite error %d", readbytes);
	va_end (args);
}

FILE* fd = NULL;
/**
 * @brief RT signal Handler: the third argument can be cast to a pointer to an object of type ucontext_t to refer
 * to the receiving process' context that was interrupted when the signal was delivered.
 * @param signo
 * @param reasonWhy
 * @param pcontext
 */
void SIGRTMIN_1_Handler(int signo, siginfo_t * reasonWhy, void * pcontext) {
	if (signo == SIGRTMIN_1) {
		PrintMessage("\n\tSIGRTMIN_1 detected !!! \n\tREASON_uid: %ld\n\tCONTEXT_oldmask: %ld\n",
				(long) (reasonWhy->si_value.sival_int),
				((ucontext_t*) (pcontext))->uc_mcontext.oldmask);

		if (fd == NULL)
			PrintMessage("\n\tFile descriptor ==NULL", fd);
		else
			PrintMessageFd((int) fd, (char*) "\nSIGRTMIN_1  \tVALUE: %ld",
					reasonWhy->si_value.sival_int);

	}
}
/**
 *
 * @param signo
 * @param value
 */
void sig_send(int signo, int value) {
	int pid;

	union sigval qval1;

	qval1.sival_int = value;

	pid = getpid();

	sigqueue(pid, signo, qval1);

}

/*
 ============================================================================
 *
 * Main
 *
 ============================================================================
 */
/**
 * main thread entry point
 *
 * @return int
 */
int main(void) {

	pthread_t r1, r2, r3, r4, w1;
	argumentos *a1 = NULL;
	argumentosTRUNK *a2, *a3, *a4, *a5;

	ETHER *lock = NULL;
	TRUNK *lockTr1, *lockTr2, *lockTr3, *lockTr4;
	queue *fifoETHEROUT = NULL;
	queue *fifoETHERIN = NULL;

	char** host_port = NULL;
	client* conn = NULL;

	//Semaphore to signal end
	sem_t end;
	sem_init(&end, 0, 0);
	struct sigaction sa;
	sigemptyset(&sa.sa_mask);

	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = SIGRTMIN_1_Handler;
	// Establish a handler to catch SIGRTMIN_1 and use it for exiting.
	if (sigaction(SIGRTMIN_1, &sa, NULL) == -1) {
		perror("sigaction for SIGRTMIN_1_Handler failed");
		exit(EXIT_FAILURE);
	}

	fd = (FILE*) open("RTtrace.log", O_CREAT | O_WRONLY | O_APPEND, S_IWUSR
			| S_IRUSR);

	//init client connection whereby TRUNKS will send their "commands"
	host_port = (char **) calloc(2, sizeof(char *));
	if (host_port == NULL)
		return EXIT_FAILURE;

	host_port[0] = (char *) calloc(10, sizeof(char));
	host_port[1] = (char *) calloc(5, sizeof(char));
	strcpy(host_port[0], "localhost");
	strcpy(host_port[1], "8712");

	conn = initClient(2, (char**) host_port);
	if (conn == NULL) {
		perror("No communication channel available");
		return EXIT_FAILURE;
	}

	lock = initlock();
	if (lock == NULL)
		return EXIT_FAILURE;

	fifoETHEROUT = lock->qOUT;
	if (fifoETHEROUT == NULL)
		return EXIT_FAILURE;
	fifoETHERIN = lock->qIN;
	if (fifoETHERIN == NULL)
		return EXIT_FAILURE;

	a1 = newRWargs(lock, 1, ETHER1, CAPACITYETHER, fifoETHEROUT, fifoETHERIN);
	if (a1 == NULL)
		return EXIT_FAILURE;
	pthread_create(&w1, NULL, runETHER, a1);

	lockTr1 = initTRUNK();
	lockTr2 = initTRUNK();
	lockTr3 = initTRUNK();
	lockTr4 = initTRUNK();

	//assigns the communication channel to each&every TRUNK
	lockTr1->conn = conn;
	lockTr2->conn = conn;
	lockTr3->conn = conn;
	lockTr4->conn = conn;

	a2 = newRWargsTr(lockTr1, 1, TRUNK1, CAPACITYTRUNK, fifoETHEROUT,
			fifoETHERIN);
	pthread_create(&r1, NULL, runTRUNK, a2);
	a3 = newRWargsTr(lockTr2, 2, TRUNK2, CAPACITYTRUNK, fifoETHEROUT,
			fifoETHERIN);
	pthread_create(&r2, NULL, runTRUNK, a3);
	a4 = newRWargsTr(lockTr3, 3, TRUNK3, CAPACITYTRUNK, fifoETHEROUT,
			fifoETHERIN);
	pthread_create(&r3, NULL, runTRUNK, a4);
	a5 = newRWargsTr(lockTr4, 4, TRUNK4, CAPACITYTRUNK, fifoETHEROUT,
			fifoETHERIN);
	pthread_create(&r4, NULL, runTRUNK, a5);

	/* Wait for end. of threads*/
	{
		printf("Terminating Threads.. \n");
		/* Join threads. */
		pthread_join(w1, NULL);
		pthread_join(r1, NULL);
		pthread_join(r2, NULL);
		pthread_join(r3, NULL);
		pthread_join(r4, NULL);
	}

	/* Cleanup*/
	close((int) fd);
	free(host_port);
	freeClient(conn);
	free(a1);
	free(a2);
	free(a3);
	free(a4);
	free(a5);
	queueDelete(lock->qOUT);
	queueDelete(lock->qIN);
	free(lock);
	free(lockTr1), free(lockTr2), free(lockTr3), free(lockTr4);
	printf("Main completed\n");
	return EXIT_SUCCESS;
}

/**
 * @brief  Creates new arguments for ETHER threads
 *
 * @param[in] l the ETHER object
 * @param[in] i the ETHER id
 * @param[in] d Time delay in ETHER generation/processing
 * @param[in] capac Processing Capacity for ETHER objects
 * @param[in] fifoETHEROUT output queue where every ETHER generates data
 * @param[in] fifoIN input queue where every ETHER retrieves data to process
 * @return (argumentos *) a pointer to arguments created
 */
argumentos *newRWargs(ETHER *l, int i, long d, unsigned int capac,
		queue* fifoETHEROUT, queue* fifoIN) {
	argumentos *args;

	args = malloc(sizeof(argumentos));
	if (args == NULL)
		return (NULL);

	args->lock = l;
	args->id = i;
	args->delay = d;
	args->Capacity = capac;
	args->fifoOUT = fifoETHEROUT, args->fifoIN = fifoIN;
	return (args);
}

/**
 * @brief  Creates new arguments for TRUNK threads
 *
 * @param[in] l the TRUNK object
 * @param[in] i the TRUNK id
 * @param[in] d Time delay in TRUNK generation/processing
 * @param[in] capac Processing Capacity for TRUNK objects
 * @param[in] fifoETHEROUT input queue where every TRUNK reads data to process
 * @param[in] fifoIN output queue where every TRUNK posts data for ETHER to process.
 * @return (argumentosTRUNK *) a pointer to arguments created
 */
argumentosTRUNK *newRWargsTr(TRUNK *l, int i, long d, unsigned int capac,
		queue* fifoIN, queue* fifoETHEROUT) {

	argumentosTRUNK *args;

	args = (argumentosTRUNK *) malloc(sizeof(argumentosTRUNK));
	if (args == NULL)
		return (NULL);

	args->lock = l;
	args->id = i;
	args->delay = d;
	args->Capacity = capac, args->fifoIN = fifoETHEROUT, args->fifoOUT = fifoIN;

	return (args);
}

/**
 * @brief Trunk thread
 * @param args
 */
void *runTRUNK(void *args) {

	argumentosTRUNK *a = (argumentosTRUNK *) args;
	queue *fifoIN, *fifoOUT;
	int i, o;
	int d;
	char *buf_ignore = NULL;

	a->lock->id = a->id;
	a->lock->Capacity = a->Capacity;
	a->lock->delay = a->delay;
	a->lock->qIN = a->fifoIN;
	a->lock->qOUT = a->fifoOUT;

	fifoIN = a->fifoIN;
	fifoOUT = a->fifoOUT;

	printf("<--runTRUNK%d\n", a->id);

	//processes capacity
	for (i = 0; i < a->lock->Capacity; i++) {

		readlockTr(a->lock, a->lock->id);

		//Takes from ETHER's fifoOUT
		queueDel(fifoOUT, &d);
		//adds to internal Buffer
		//processes internal Buffer
		//TODO processes internal Buffer (here no internal buffer for Trunk)
		//sending to a server process and taking back the response
		//blocking while not receiving back the response from the remote device.
		buf_ignore = trunk_send(a->lock->conn, d + 1);
		buf_ignore = buf_ignore;//avoids warning

		printf("\nTRUNK%d: received %d.\n", a->lock->id, d + 1);

		readunlockTr(a->lock);

		//delay: Let's say the processing time needed by the real device
		usleep(a->lock->delay);
	}

	//writes response to ETHER's fifoIN
	for (o = 0; o < a->lock->Capacity; o++) {

		writelockTr(a->lock, a->lock->id);

		queueAdd(fifoIN, o + 1);
		printf("TRUNK%d: sent %d.\n", a->lock->id, o + 1);

		writeunlockTr(a->lock);

		//delay
		usleep(a->lock->delay);
	}

	printf("TRUNK %d: Finished.\n", a->id);

	return (NULL);
}

/**
 * Ether thread
 * @param args
 */
void *runETHER(void *args) {

	argumentos *a;
	queue *fifoOUT, *fifoIN;
	int o;
	int i;
	int d;

	a = (argumentos *) args;
	a->lock->id = a->id;
	a->lock->Capacity = a->Capacity;
	a->lock->delay = a->delay;
	a->lock->qOUT = a->fifoOUT;
	fifoOUT = (queue*) (a->fifoOUT);
	a->lock->qIN = a->fifoIN;
	fifoIN = (queue*) (a->fifoIN);

	printf("--> runETHER %d\n", a->id);

	for (o = 0; o < a->lock->Capacity; o++) {

		//generates capacity

		writelock(a->lock, a->id);
		queueAdd(fifoOUT, o + 1);
		printf("ETHER %d: Wrote %d\n", a->id, o + 1);
		writeunlock(a->lock);

	}

	//ETHER has generated the entire output, Notify
	printf("ETHER %d: Finished generation.\n", a->id);

	//reads qIN until Capacity to process results from TRUNKS
	for (i = 0; i < a->lock->Capacity; i++) {

		//reads capacity

		readlock(a->lock, a->id);
		queueDel(fifoIN, &d);
		printf("ETHER %d: read %d\n", a->id, d);
		readunlock(a->lock);
		sig_send(SIGRTMIN_1, d);
	}

	//ETHER has consumed the entire input, Notify
	printf("ETHER %d: Finishing...\n", a->id);
	printf("ETHER %d: Finished Processing.\n", a->id);

	return (NULL);
}

/**
 * @brief Initializes ETHER Struct
 * @return ETHER* struct
 */
ETHER *initlock(void) {
	ETHER *lock;

	lock = (ETHER *) malloc(sizeof(ETHER));
	if (lock == NULL)
		return (NULL);

	lock->qOUT = queueInit();
	if (lock->qOUT == NULL) {
		fprintf(stderr, "initlock: Main Queue Init failed.\n");
		return (NULL);
	}
	lock->qIN = queueInit();
	if (lock->qIN == NULL) {
		fprintf(stderr, "initlock: qIN Init failed.\n");
		return (NULL);
	}

	lock->id = 0;

	lock->Capacity = CAPACITYETHER;
	lock->delay = ETHER1;

	return (lock);
}

/**
 *
 * @return TRUNK*
 */
TRUNK *initTRUNK(void) {
	TRUNK *lock;

	lock = (TRUNK *) malloc(sizeof(TRUNK));
	if (lock == NULL)
		return (NULL);

	lock->id = 0;

	lock->Capacity = CAPACITYTRUNK;
	lock->delay = TRUNK1;

	lock->qOUT = NULL;
	lock->qIN = NULL;

	return (lock);
}

/**
 * @brief Used by TRUNK
 * @param lock
 * @param id
 */
void readlockTr(TRUNK *lock, int id) {

	pthread_mutex_lock(lock->qOUT->mut);

	while (lock->qOUT->empty) {
		lock->qOUT->TRUNKs++;
		printf("TRUNK %d blocked reading. queue qOUT Empty\n", id);
		pthread_cond_wait(lock->qOUT->readOK, lock->qOUT->mut);
		printf("TRUNK %d unblocked.(queue qOUT not empty)\n", id);
	}
	return;
}
void readunlockTr(TRUNK *lock) {
	lock->qOUT->TRUNKs--;
	pthread_mutex_unlock(lock->qOUT->mut);
	pthread_cond_signal(lock->qOUT->writeOK);

}
void writelockTr(TRUNK *lock, int id) {

	pthread_mutex_lock(lock->qIN->mut);

	while (lock->qIN->full) {
		lock->qIN->TRUNKs++;
		printf("TRUNK %d blocked.\n", id);
		pthread_cond_wait(lock->qIN->writeOK, lock->qIN->mut);
		printf("TRUNK %d unblocked.\n", id);
	}
	return;
}

void writeunlockTr(TRUNK *lock) {

	pthread_mutex_unlock(lock->qIN->mut);
	lock->qIN->TRUNKs--;
	pthread_cond_signal(lock->qIN->readOK);

}

/**
 * @brief used by ETHER
 * @param lock
 * @param id
 */

void writelock(ETHER *lock, int id) {

	pthread_mutex_lock(lock->qOUT->mut);

	while (lock->qOUT->full) {
		lock->qOUT->ETHERs++;
		printf("ETHER %d blocked.\n", id);
		pthread_cond_wait(lock->qOUT->writeOK, lock->qOUT->mut);
		printf("ETHER %d unblocked.\n", id);
	}
	return;
}

void writeunlock(ETHER *lock) {

	pthread_mutex_unlock(lock->qOUT->mut);
	lock->qOUT->ETHERs--;
	pthread_cond_signal(lock->qOUT->readOK);

}
void readlock(ETHER *lock, int id) {

	pthread_mutex_lock(lock->qIN->mut);

	while (lock->qIN->empty) {

		lock->qIN->ETHERs++;
		printf("ETHER %d blocked reading from qIN. queue Empty\n", id);
		pthread_cond_wait(lock->qIN->readOK, lock->qIN->mut);
		printf("ETHER %d unblocked.(queue not empty)\n", id);
	}
	return;
}
void readunlock(ETHER *lock) {

	lock->qIN->ETHERs--;
	pthread_mutex_unlock(lock->qIN->mut);
	pthread_cond_signal(lock->qIN->writeOK);

}

void deletelock(ETHER *lock) {
	pthread_mutex_destroy(lock->qOUT->mut);
	pthread_cond_destroy(lock->qOUT->readOK);
	pthread_cond_destroy(lock->qOUT->writeOK);
	free(lock);

	return;
}

/**
 * @brief FIFO queue structure
 * @return
 */
queue *queueInit(void) {
	queue *q;

	q = (queue *) malloc(sizeof(queue));
	if (q == NULL)
		return (NULL);

	q->empty = 1;
	q->full = 0;
	q->head = 0;
	q->tail = 0;
	q->TRUNKs = 0;
	q->ETHERs = 0;
	q->waiting2Write = 0;
	q->waiting2Read = 0;

	q->mut = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(q->mut, NULL);

	q->writeOK = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
	if (q->writeOK == NULL) {
		free(q->mut);
		free(q);
		return (NULL);
	}

	q->readOK = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
	if (q->readOK == NULL) {
		free(q->writeOK);
		free(q->mut);
		free(q);
		return (NULL);
	}

	pthread_cond_init(q->writeOK, NULL);
	pthread_cond_init(q->readOK, NULL);

	return (q);
}

void queueDelete(queue *q) {
	pthread_mutex_destroy(q->mut);
	free(q->mut);
	pthread_cond_destroy(q->writeOK);
	free(q->writeOK);
	pthread_cond_destroy(q->readOK);
	free(q->readOK);
	free(q);
}

void queueAdd(queue *q, int in) {
	q->buf[q->tail] = in;
	q->tail++;
	if (q->tail == QUEUESIZE)
		q->tail = 0;
	if (q->tail == q->head)
		q->full = 1;
	q->empty = 0;

	return;
}

void queueDel(queue *q, int *out) {
	*out = q->buf[q->head];

	q->head++;
	if (q->head == QUEUESIZE)
		q->head = 0;
	if (q->head == q->tail)
		q->empty = 1;
	q->full = 0;

	return;
}

