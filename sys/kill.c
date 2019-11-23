/* kill.c - kill */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 * kill  --  kill a process and remove it from the system
 *------------------------------------------------------------------------
 */
SYSCALL kill(int pid)
{
	STATWORD ps;    
	struct	pentry	*pptr;		/* points to proc. table for pid*/
	int	dev;

	disable(ps);
	if (isbadpid(pid) || (pptr= &proctab[pid])->pstate==PRFREE) {
		restore(ps);
		return(SYSERR);
	}
	if (--numproc == 0)
		xdone();

	dev = pptr->pdevs[0];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->pdevs[1];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->ppagedev;
	if (! isbaddev(dev) )
		close(dev);
	
	send(pptr->pnxtkin, pid);

	freestk(pptr->pbase, pptr->pstklen);

	if(pptr->wait_lock > -1) { 
		update_wq(pptr->wait_lock, pptr);
		remove_wq(pptr->wait_lock, pid);
	}

	switch (pptr->pstate) {

	case PRCURR:	pptr->pstate = PRFREE;	/* suicide */
			resched();

	case PRWAIT:	semaph[pptr->psem].semcnt++;

	case PRREADY:	dequeue(pid);
			pptr->pstate = PRFREE;
			break;

	case PRSLEEP:
	case PRTRECV:	unsleep(pid);
						/* fall through	*/
	default:	pptr->pstate = PRFREE;
	}
	restore(ps);
	return(OK);
}


void update_wq(int ldes, struct pentry *pptr) {	
	lentry *lptr = &locktab[ldes];

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
		if(max_prio != lptr->lprio)
			lptr->lprio = max_prio
		prio_inh(lptr, lptr->lprio);
	}
}