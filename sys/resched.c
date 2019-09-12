/* resched.c  -  resched */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <math.h>
#include <sched.h>

unsigned long currSP;	/* REAL sp of current process */
int curr_sched_class;

extern int ctxsw(int, int, int, int);

void setschedclass (int sched_class) {
	curr_sched_class = sched_class;
}

int getschedclass() {
	return curr_sched_class;
}

// need to initialize new processes in ready
int get_linux_proc(int head) {
	int cur = head, goodness = -1, tmp = 0, proc = -1;
	struct qent* qptr = &q[cur];
	struct pentry* pptr;
	while(cur != EMPTY) {
		pptr = &proctab[cur];
		tmp = pptr->quantum + pptr->eprio;
		if(tmp > goodness && pptr->isnew == 0 && pptr->quantum > 0) {
			goodness = tmp;
			proc = cur;
		}  // else maybe dequeue
		cur = qptr->qnext;
	}
	return proc;
}

/*-----------------------------------------------------------------------
 * resched  --  reschedule processor to highest priority ready process
 *
 * Notes:	Upon entry, currpid gives current process id.
 *		Proctab[currpid].pstate gives correct NEXT state for
 *			current process if other than PRREADY.
 *------------------------------------------------------------------------
 */

// *********** need to implement the round robin portion and null process *****************
int resched()
{
	register struct	pentry	*optr;	/* pointer to old process entry */
	register struct	pentry	*nptr;	/* pointer to new process entry */
	if(curr_sched_class == EXPDISTSCHED) {
		optr= &proctab[currpid];
		double exp_rand = expdev(.1);
		optr->pstate = PRREADY;
		insert(currpid,rdyhead,optr->pprio);
		// should this be removed from queue like getlast?
		nptr = &proctab[ (currpid = get_exp_proc(exp_rand, rdyhead)) ];
		nptr->pstate = PRCURR;
		ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
		return OK;
	} else if(curr_sched_class == LINUXSCHED) {
		optr= &proctab[currpid];
		optr->quantum--;
		
		// only reschedule if called from sleep or quantum is 0
		if(optr->quantum == 0) {
			int val = get_linux_proc(rdyhead);
			if(val > -1) {
				optr->pstate = PRREADY;
				insert(currpid,rdyhead,optr->pprio);
				currpid = dequeue(val);
				// should this be removed from queue like getlast?
				nptr = &proctab[currpid];
				nptr->pstate = PRCURR;
				ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
				return OK;
			} else {
				init_epoch();
				// may need to re initialize ready queue
				val = get_linux_proc(rdyhead);
				optr->pstate = PRREADY;
				insert(currpid,rdyhead,optr->pprio);
				currpid = dequeue(val);
				// should this be removed from queue like getlast?
				nptr = &proctab[currpid];
				nptr->pstate = PRCURR;
				ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
				return OK;
			}
		}
	} else {
		/* no switch needed if current process priority higher than next*/

		if ( ( (optr= &proctab[currpid])->pstate == PRCURR) &&
		(lastkey(rdytail)<optr->pprio)) {
			return(OK);
		}
		
		/* force context switch */
		// put current process back in ready queue, ordered by priority
		if (optr->pstate == PRCURR) {
			optr->pstate = PRREADY;
			insert(currpid,rdyhead,optr->pprio);
		}

		/* remove highest priority process at end of ready list */

		nptr = &proctab[ (currpid = getlast(rdytail)) ];
		nptr->pstate = PRCURR;		/* mark it currently running	*/
	#ifdef	RTCLOCK
		preempt = QUANTUM;		/* reset preemption counter	*/
	#endif
		
		ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
		
		/* The OLD process returns here when resumed. */
		return OK;
	}
}
