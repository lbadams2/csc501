#ifndef _LOCK_H_
#define _LOCK_H_

#define NLOCKS  50
#define DELETED 0
#define READ    1
#define WRITE   2

#define WQHEAD  30
#define WQTAIL  31

typedef int sem_t;

void linit();
int lcreate(); // create new lock
int ldelete(int);
int lock(int, int, int); // acquire or wait on existing lock
int releaseall(int, long);
SYSCALL lsignal(lentry *, int, int);
SYSCALL lwait(lentry *, int, int, int);

void enqueue_wq(int, int, int);
void remove_wq(int, int);
int dequeue_wq(int);


// multiple readers can hold lock, write lock is exclusive
// process can only be inside single wait queue at a time
typedef struct {
    sem_t bin_lock; // semaphore id of binary lock
    sem_t write_lock; // one writer
    int readers; // number of readers
    int lprio; // max scheduling prio of all procs waiting in wq
    unsigned int procs_holding; // bitmask of procs holding lock, ids [0 - 29]
    struct qent wq[NPROC + 2]; // wait queue, ordered by priority passed to lock(), not scheduling prio

    // increase priority of low priority proc holding lock to prio of high prio waiting on lock (use procs_holding)
    // in situation where a higher priority writer is waiting on reader for example
    // don't need to maintain queue for priority inversion, just let exisiting q handle it
} lentry;

void sem_wait(lentry *, int); // int is 0 or 1 for bin or write
void sem_post(lentry *, int, int); // int is 0 or 1 for bin or write

extern lentry locktab[];
//extern	struct	qent lq[];

#endif