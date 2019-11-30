/* chprio.c - chprio */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>


void update_wq_ch(int, int);
/*------------------------------------------------------------------------
 * chprio  --  change the scheduling priority of a process
 *------------------------------------------------------------------------
 */
SYSCALL chprio(int pid, int newprio)
{
	STATWORD ps;    
	struct	pentry	*pptr;

	disable(ps);
	if (isbadpid(pid) || newprio<=0 ||
	    (pptr = &proctab[pid])->pstate == PRFREE) {
		restore(ps);
		return(SYSERR);
	}
	//int lowest_prio = get_lowest_prio(pptr);
	if(newprio < pptr->pinh) {
		restore(ps);
		return pptr->pinh;
	}
	if(pptr->wait_lock > -1) { // if proc is in a wait queue
		update_wq_ch(pptr->wait_lock, newprio);
	}
	pptr->pprio = newprio;
	restore(ps);
	return(newprio);
}

void update_wq_ch(int ldes, int newprio) {	
	lentry *lptr = &locktab[ldes];
	int pid = getpid();
	struct pentry *pptr = &proctab[pid];
	if(newprio > lptr->lprio) {
		lptr->lprio = newprio;
		prio_inh(lptr, lptr->lprio);
		return;
	}
	if(pptr->pprio == lptr->lprio) { // may need to change lprio
		struct qent *wqptr = lptr->wq;
		int next = wqptr[WQHEAD].qnext;
		int max_prio = -1;
		while(next != WQTAIL) {
			if(next == pid) {
				next = wqptr[next].qnext;
				continue;
			}
			pptr = &proctab[next];
			if(pptr->pprio > max_prio)
				max_prio = pptr->pprio;
			next = wqptr[next].qnext;
		}
		if(max_prio > newprio)
			lptr->lprio = max_prio;
		else
			lptr->lprio = newprio;
		prio_inh(lptr, lptr->lprio);
		return;
	}
	if(pptr->pprio < lptr->lprio && newprio < lptr->lprio) { // don't need to do anything

	}
}

/*
int get_lowest_prio(struct pentry *pptr) {
	int i, max_prio = 0;
	lentry *lptr;
	for(i = 0; i < NLOCKS; i++) {
		if(pptr->locks_held[i] != -1) {
            lptr = &locktab[i];
            if(lptr->lprio > max_prio)
                max_prio = lptr->lprio;
        }
	}
	return max_prio;
}*/