#ifndef _LOCK_H_
#define _LOCK_H_

#define NLOCKS  50
#define READ    1
#define WRITE   2
#define LFREE   0
#define LACTIVE 1
#define LDELETED 2

#define WQHEAD  NPROC
#define WQTAIL  NPROC + 1

typedef int sem_t;

typedef struct {		/* one for each process plus two for	*/
				/* each list				*/
	int	qkey;		/* key on which the queue is ordered	*/
	int	qnext;		/* pointer to next process or tail	*/
	int	qprev;		/* pointer to previous process or head	*/
} lqent;

// multiple readers can hold lock, write lock is exclusive
// process can only be inside single wait queue at a time
typedef struct {
    sem_t bin_lock; // semaphore id of binary lock
    sem_t write_lock; // one writer
    int status; // created or deleted/never created
    int readers; // number of readers
    int lprio; // max scheduling prio of all procs waiting in wq
    int create_pid; // pid of creating process
    unsigned long long procs_holding; // bitmask of procs holding lock, ids [0 - 29]
    // could do this one queue for all locks because lock is only allowed to be in one wait queue at a time
    lqent wq[NPROC + 2]; // wait queue, ordered by priority passed to lock(), not scheduling prio, 30 is NPROC

    // increase priority of low priority proc holding lock to prio of high prio waiting on lock (use procs_holding)
    // in situation where a higher priority writer is waiting on reader for example
    // don't need to maintain queue for priority inversion, just let exisiting q handle it
} lentry;

void linit();
int lcreate(); // create new lock
int ldelete(int);
int lock(int, int, int); // acquire or wait on existing lock
int releaseall(int, long);
SYSCALL lsignal(lentry *, int, int);
SYSCALL lwait(lentry *, int, int, int);
void prio_inh(lentry *, int);

void enqueue_wq(int, int, int, struct pentry *);
void remove_wq(int, int);
int dequeue_wq(int);


void sem_wait(lentry *, int, int, int, int); // int is 0 or 1 for bin or write
void sem_post(lentry *, int, int); // int is 0 or 1 for bin or write

extern lentry locktab[];
//extern	struct	qent lq[];

#endif