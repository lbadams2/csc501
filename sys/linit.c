#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <lock.h>

lentry locktab[NLOCKS];
//struct qent lq[NLOCKS + 2];

// in intialize when initializing semaphore q, every sem gets it own q by getting its own head and tail
// they all point to the procs which will be the first 30 entries of the q
// its head and tail never change, their next and prev values can, once you get next from head the remaining next vals will follow order of proc prio
// each sem q is priority q pointing to same process ready q, they are just slices of that q with different heads and tails
void linit() {
	int i;
    lqent *hptr, *tptr;
    int lqhead = 50;
	for(i = 0; i < NLOCKS; i++) {
		locktab[i].lprio = -1;
		locktab[i].procs_holding = 0;
        locktab[i].status = LFREE;
        locktab[i].bin_lock = 1;
        locktab[i].write_lock = 1;
        locktab[i].create_pid = -1;
        locktab[i].bin_head = lqhead++;
        locktab[i].bin_tail = lqhead++;
        hptr = &wq[lqhead - 2];
        tptr = &wq[lqhead - 1];
        hptr->qnext = lqhead - 1;
        hptr->qprev = EMPTY;
        hptr->qkey  = MAXINT;
        tptr->qnext = EMPTY;
        tptr->qprev = lqhead - 2;
        tptr->qkey  = -1;
        locktab[i].write_head = lqhead++;
        locktab[i].write_tail = lqhead++;
        hptr = &wq[lqhead - 2];
        tptr = &wq[lqhead - 1];
        hptr->qnext = lqhead - 1;
        hptr->qprev = EMPTY;
        hptr->qkey  = MAXINT;
        tptr->qnext = EMPTY;
        tptr->qprev = lqhead - 2;
        tptr->qkey  = -1;
        /*
        hptr = &locktab[i].wq[WQHEAD];
        tptr = &locktab[i].wq[WQTAIL];
        hptr->qnext = WQTAIL;
        hptr->qprev = EMPTY;
        hptr->qkey  = MAXINT;
        tptr->qnext = EMPTY;
        tptr->qprev = WQHEAD;
        tptr->qkey  = -1;
        */
	}
}
