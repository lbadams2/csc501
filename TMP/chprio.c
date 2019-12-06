/* chprio.c - chprio */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <lock.h>
#include <stdio.h>


void update_wq_ch(int, int);
/*------------------------------------------------------------------------
 * chprio  --  change the scheduling priority of a process
 *------------------------------------------------------------------------
 */
// new prio doesn't take affect immediately, takes affect on next call to ready
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
	int head, tail;
	int max_prio = -1;
	if(newprio > lptr->lprio) {
		lptr->lprio = newprio;
		prio_inh(lptr, lptr->lprio);
		return;
	}

	// if lprio was equal to current proc being reduced, the reduction may or may not still be greater than all others waiting
	if(pptr->pprio == lptr->lprio) { // may need to change lprio
		// need to check both queues
		head = get_wq_head(ldes);
		tail = head + 1;
		int next = head;
		while(next != tail) {
			if(next == pid) {
				next = wq[next].qnext;
				continue;
			}
			pptr = &proctab[next];
			if(pptr->pprio > max_prio)
				max_prio = pptr->pprio;
			next = wq[next].qnext;
		}

		// need to check both queues
		/*
		head = get_wq_head(ldes, WRITE);
		tail = head + 1;
		next = head;
		while(next != tail) {
			if(next == pid) {
				next = wq[next].qnext;
				continue;
			}
			pptr = &proctab[next];
			if(pptr->pprio > max_prio)
				max_prio = pptr->pprio;
			next = wq[next].qnext;
		}
		*/
		if(max_prio > newprio)
			lptr->lprio = max_prio;
		else
			lptr->lprio = newprio;
		prio_inh(lptr, lptr->lprio);
		return;
	}
	// if lprio was greater than current proc don't need to change anything
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