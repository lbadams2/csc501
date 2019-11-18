#include <conf.h>
#include <kernel.h>
#include <lock.h>

lentry locktab[NLOCKS];
//struct qent lq[NLOCKS + 2];

// in intialize when initializing semaphore q, every sem gets it own q by getting its own head and tail
// they all point to the procs which will be the first 30 entries of the q
// its head and tail never change, their next and prev values can, once you get next from head the remaining next vals will follow order of proc prio
// each sem q is priority q pointing to same process ready q, they are just slices of that q with different heads and tails
void linit() {
	int i;
    struct qent *hptr, *tptr;
	for(i = 0; i < NLOCKS; i++) {
		locktab[i].lprio = -1;
		locktab[i].procs_holding = 0;
        hptr = &locktab[i].wq[WQHEAD];
        tptr = &locktab[i].wq[WQTAIL];
        hptr->qnext = tindex;
        hptr->qprev = EMPTY;
        hptr->qkey  = MININT;
        tptr->qnext = EMPTY;
        tptr->qprev = hindex;
        tptr->qkey  = MAXINT;
	}
}