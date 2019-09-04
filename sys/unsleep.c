/* unsleep.c - unsleep */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <sleep.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 * unsleep  --  remove  process from the sleep queue prematurely
 *------------------------------------------------------------------------
 */
extern unsigned long ctr1000;
extern void updatesysarr(char* name, unsigned long duration);
extern int track_sys_calls;

SYSCALL	unsleep(int pid)
{
	unsigned long start = ctr1000;
	STATWORD ps;    
	struct	pentry	*pptr;
	struct	qent	*qptr;
	int	remain;
	int	next;

        disable(ps);
	if (isbadpid(pid) ||
	    ( (pptr = &proctab[pid])->pstate != PRSLEEP &&
	     pptr->pstate != PRTRECV) ) {
		restore(ps);
		return(SYSERR);
	}
	qptr = &q[pid];
	remain = qptr->qkey;
	if ( (next=qptr->qnext) < NPROC)
		q[next].qkey += remain;
	dequeue(pid);
	if ( (next=q[clockq].qnext) < NPROC)
		sltop = (int *) & q[next].qkey;
	else
		slnempty = FALSE;
        restore(ps);
	unsigned long duration = ctr1000 - start;
	if(track_sys_calls == 1)
		updatesysarr("unsleep", duration);
	return(OK);
}
