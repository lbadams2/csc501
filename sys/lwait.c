#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <lock.h>
#include <stdio.h>

SYSCALL lwait(lentry *lptr, int sem, int ldes, int prio, int lock_type) {
    STATWORD ps;    
	struct	pentry	*pptr;

	disable(ps);
    if(sem == 0) { // bin sem
        if (lptr->bin_lock >= 0) {
            restore(ps);
            return(SYSERR);
	    }
    } else { // write sem
        if (lptr->write_lock >= 0) {
            restore(ps);
            return(SYSERR);
	    }
    }
	(pptr = &proctab[currpid])->pstate = PRWAIT;
    pptr->wait_lock = ldes;
    pptr->lock_type = lock_type;
    enqueue_wq(ldes, currpid, prio);
    pptr->pwaitret = OK;
    resched();
    restore(ps);
    return pptr->pwaitret;
}