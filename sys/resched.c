/* resched.c  -  resched */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <stdio.h>
#include <math.h>
#include <sched.h>

unsigned long currSP;	/* REAL sp of current process */
int curr_sched_class;

extern int ctxsw(int, int, int, int);

void print_proctab() {
        int i;
        struct pentry* pptr;
        for(i = 0; i < NPROC; i++) {
                pptr = &proctab[i];
                kprintf("proc %d priority %d name %s\n", i, pptr->pprio, pptr->pname);
        }
        kprintf("\n\n");
}

void printqueue() {
        int i;
        struct qent* qptr;
        for(i = 0; i < NQENT; i++) {
                qptr = &q[i];
                kprintf("q index %d next %d prev %d key %d\n", i, qptr->qnext, qptr->qprev, qptr->qkey);
        }
}

void setschedclass (int sched_class) {
	curr_sched_class = sched_class;
	if(sched_class == LINUXSCHED)
		init_epoch();
}

int getschedclass() {
	return curr_sched_class;
}

// need to initialize new processes in ready
int get_linux_proc() {
	int goodness = 0, tmp = 0, proc = -1;
	struct pentry* pptr;
	int i;
	for(i = 0; i < NPROC; i++) {
		pptr = &proctab[i];
		tmp = pptr->quantum + pptr->eprio;
		if(tmp > goodness && pptr->isnew == 0 && pptr->pstate == PRREADY && pptr->quantum > 0) {
			goodness = tmp;
			proc = i;
		}  // else maybe dequeue
	}
	//if(proc == -1)
	//proc = handle_null(prev);
	kprintf("chose linux proc %d goodness %d\n", proc, goodness);
	return proc;
}

void add_round_robin_exp(struct pentry* pptr) {
	int cur = rdyhead;
	struct pentry* tmp;
	struct qent* qptr;
	while(cur != EMPTY) {
		tmp = &proctab[cur];
		qptr = &q[cur];
		if(tmp->pprio == pptr->pprio && strcmp(tmp->pname, pptr->pname) != 0)
			pptr->rr_next = tmp;
		else
			pptr->rr_next = NULL;
		cur = qptr->qnext;
	}
}

void add_round_robin_lx(struct pentry* pptr) {
	struct pentry* tmp;
	int i;
	for(i = 0; i < NPROC; i++) {
		tmp = &proctab[i];
		if((tmp->eprio + tmp->quantum) == (pptr->eprio + pptr->quantum) && strcmp(tmp->pname, pptr->pname) != 0)
			pptr->rr_next = tmp;
		else
			pptr->rr_next = NULL;
	}
}

int get_round_robin(struct pentry* optr, struct pentry* nptr) {
	if(optr->rr_next != NULL) {
		nptr = optr->rr_next;
		optr->rr_next = NULL;
		int i;
		for(i = 0; i < NPROC; i++)
			if(nptr == &proctab[i])
				break;
		currpid = dequeue(i);
		return 1;
	}
	else
		return 0;
}

void sched_exp_dist() {
	register struct	pentry	*optr;	/* pointer to old process entry */
	register struct	pentry	*nptr;	/* pointer to new process entry */
	optr= &proctab[currpid];
	if(optr->pstate == PRCURR) {
		optr->pstate = PRREADY;
		insert(currpid,rdyhead,optr->pprio);
	}
	// should this be removed from queue like getlast?
	int rr_val = get_round_robin(optr, nptr);
	if(rr_val == 0) {
		double exp_rand = expdev(.1);
		nptr = &proctab[ (currpid = get_exp_proc(exp_rand, rdyhead)) ];	
	}
	add_round_robin_exp(nptr);
	nptr->pstate = PRCURR;
	ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
}

void linux_sched() {
	register struct	pentry	*optr;	/* pointer to old process entry */
	register struct	pentry	*nptr;	/* pointer to new process entry */
	optr= &proctab[currpid];
	optr->quantum--;
	// only reschedule if called from sleep or quantum is 0
	if(strcmp(optr->pname, "proc C") == 0)
                kprintf("proc c quantum is %d\n", optr->quantum);
	if(optr->quantum <= 0 || optr->pstate != PRCURR) {
		//kprintf("proc c quantum is %d sp is %d state is %d base is %d stk len is %d limit is %d kin is %d\n", optr->quantum, optr->pesp, optr->pstate, optr->pbase, optr->pstklen, optr->plimit, optr->pnxtkin);
		kprintf("%s quantum is 0 or yielded quantum %d state %d\n", optr->pname, optr->quantum, optr->pstate);
		if(optr->pstate == PRCURR) { // maybe don't mark as ready and insert
			optr->pstate = PRREADY;
			insert(currpid,rdyhead,optr->pprio);
		}
		int rr_val = get_round_robin(optr, nptr);
		if(rr_val == 0) {
			int val = get_linux_proc();
			if(val < 0) {
				init_epoch();
				// may need to re initialize ready queue
				val = get_linux_proc();
			}
			if(val < 0) { // only null proc is ready
				nptr = &proctab[NULLPROC];
				currpid = dequeue(NULLPROC);
			} else {
				currpid = dequeue(val);
				nptr = &proctab[currpid];
			}
		}
		add_round_robin_lx(nptr);
		nptr->pstate = PRCURR;
		nptr->has_run = 1;
		ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
	} else if(optr->rr_next != NULL) {
		// not sure what to do here, for now let current process finish
	}
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
	if(curr_sched_class == EXPDISTSCHED) {
		sched_exp_dist();
	} else if(curr_sched_class == LINUXSCHED) {
		linux_sched();
	} else {
		register struct	pentry	*optr;	/* pointer to old process entry */
		register struct	pentry	*nptr;	/* pointer to new process entry */
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
	#ifdef	RTCLOCK
		preempt = QUANTUM;		/* reset preemption counter	*/
	#endif
	return OK;
}
