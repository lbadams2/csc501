#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

void remove_rq();

// need to remove proc from ready list like enqueue does in wait
SYSCALL lwait(lentry *lptr, int ldes, int prio, int lock_type) {
    STATWORD ps;    
	struct	pentry	*pptr;

	disable(ps);
    if(lock_type == READ) { // bin sem
        lptr->bin_lock--;
        if (lptr->bin_lock >= 0) {
            kprintf("pid: %d returning syserr bin lock %d\n", currpid, lptr->bin_lock);
            restore(ps);
            return(SYSERR);
	    }
    } else { // write sem
        lptr->write_lock--;
        if (lptr->write_lock >= 0) {
            kprintf("pid: %d returning syserr write lock %d\n", currpid, lptr->write_lock);
            restore(ps);
            return(SYSERR);
	    }
    }
	(pptr = &proctab[currpid])->pstate = PRWAIT;
    pptr->wait_lock = ldes;
    pptr->lock_type = lock_type;
    enqueue_wq(ldes, currpid, prio, lock_type, pptr);
    remove_rq();
    pptr->pwaitret = OK;
    resched();
    restore(ps);
    return pptr->pwaitret;
}

void remove_rq() {
    int pid = getpid();
    kprintf("pid: %d removing itself from ready queue\n", pid);
    struct qent *curr, *next, *prev;
    curr = &q[currpid];
    next = &q[curr->qnext];
    prev = &q[curr->qprev];
    next->qprev = curr->qprev;
    prev->qnext = curr->qnext;
    curr->qnext = -1;
    curr->qprev = -1;
}