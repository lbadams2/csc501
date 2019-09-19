/* ready.c - ready */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>

/*------------------------------------------------------------------------
 * ready  --  make a process eligible for CPU service
 *------------------------------------------------------------------------
 */
int ready(int pid, int resch)
{
	register struct	pentry	*pptr;

	if (isbadpid(pid))
		return(SYSERR);
	pptr = &proctab[pid];
	pptr->pstate = PRREADY;
	//pptr->isnew = 1;
	//pptr->has_run_epch = 0;
	//pptr->quantum = 0;
	//pptr->rr_next = NULL;
	insert(pid,rdyhead,pptr->pprio);
	if (resch)
		resched();
	return(OK);
}
