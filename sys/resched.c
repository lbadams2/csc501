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
int rr_test_ix = 0;
int rrq[NPROC];
int rrq_front = -1;
int rrq_back = -1;
int rrq_size = -1;

extern int ctxsw(int, int, int, int);

void rr_enqueue(int proc) {
	if(size < NPROC) {
		if(size < 0) {
			rrq[0] = proc;
			rrq_front = rrq_back = 0;
			rrq_size = 1;
		} else if(rrq_back == NPROC -1) {
			rrq[0] = proc;
			rrq_back = 0;
			rrq_size++;
		} else {
			rrq[rrq_back++] = proc;
			rrq_size++;
		}
	}
}

int rr_dequeue() {
	if(rrq_size < 0) {
		return -1;
	} else {
		rrq_size--;
		return rrq[rrq_front++];
	}
}

int rr_isempty() {
	return rrq_size < 0;
}

void print_proctab() {
        int i;
        struct pentry* pptr;
        for(i = 0; i < NPROC; i++) {
                pptr = &proctab[i];
                kprintf("proc %d name %s priority %d quantum %d\n", i, pptr->pname, pptr->pprio, pptr->quantum);
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
	//kprintf("chose linux proc %d goodness %d\n", proc, goodness);
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

// 0 nullproc, 46, 47, 48, 49
void add_rr_test(struct pentry* pptr) {
	int i = rr_test_ix++ % 5;
	int proc;
	if(i == 0)
		proc = 0;
	else if(i == 1)
		proc = 46;
	else if(i == 2)
		proc = 47;
	else if(i == 3)
		proc = 48;
	else
		proc = 49;
	struct pentry* rrptr = &proctab[proc];
	if(rrptr->pstate == PRREADY) {
		pptr->rr_next = rrptr;
		kprintf("added rr %s\n", rrptr->pname);
	}
}

int get_round_robin(struct pentry* optr, struct pentry** nptr) {
	//kprintf("in round robin\n");
	if(optr->rr_next != NULL && optr->rr_next->pstate == PRREADY) {
		nptr = optr->rr_next;
		optr->rr_next = NULL;
		int i;
		for(i = 0; i < NPROC; i++)
			if(nptr == &proctab[i])
				break;
		currpid = dequeue(i);
		kprintf("chose round robin %s pid %d\n", (*nptr)->pname, currpid);
		return 1;
	}
	else
		return 0;
}

int sched_exp_dist() {
	struct	pentry	*optr;	/* pointer to old process entry */
	struct	pentry	*nptr;	/* pointer to new process entry */
	optr= &proctab[currpid];
	if(optr->pstate == PRCURR) {
		optr->pstate = PRREADY;
		insert(currpid,rdyhead,optr->pprio);
	}
	// should this be removed from queue like getlast?
	int rr_val = get_round_robin(optr, &nptr);
	if(rr_val == 0) {
		double exp_rand = expdev(.1);
		//kprintf("rand val is %d\n", (int)exp_rand);
		currpid = get_exp_proc(exp_rand, rdyhead);
		//kprintf("currpid is %d\n", currpid);
		nptr = &proctab[currpid];	
	}
	//add_round_robin_exp(nptr);
	add_rr_test(nptr);
	nptr->pstate = PRCURR;
	kprintf("about to ctxsw old %s new %s new rr %s\n", optr->pname, nptr->pname, nptr->rr_next->pname);
	#ifdef  RTCLOCK
        preempt = QUANTUM;              /* reset preemption counter     */
    #endif
	ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
	return OK;
}

void update_optr(struct pentry* optr) {
	if(optr->pstate == PRCURR) { // maybe don't mark as ready and insert
		optr->pstate = PRREADY;
		insert(currpid,rdyhead,optr->pprio);
	}
}

int linux_sched() {
	STATWORD ps;
	disable(ps);
	register struct	pentry	*optr;	/* pointer to old process entry */
	register struct	pentry	*nptr;	/* pointer to new process entry */
	optr= &proctab[currpid];
	optr->quantum--;
	if(optr->quantum <= -10) {
		//STATWORD ps;
		//disable(ps);
		print_proctab();
		shutdown();
		//restore(ps);
	}
	// only reschedule if called from sleep or quantum is 0
	//if(strcmp(optr->pname, "proc C") == 0 || strcmp(optr->pname, "main") == 0)
                //kprintf("%s quantum is %d\n", optr->pname, optr->quantum);
	if(optr->quantum <= 0 || optr->pstate != PRCURR) {
		//kprintf("proc c quantum is %d sp is %d state is %d base is %d stk len is %d limit is %d kin is %d\n", optr->quantum, optr->pesp, optr->pstate, optr->pbase, optr->pstklen, optr->plimit, optr->pnxtkin);
		//kprintf("%s quantum is 0 or yielded quantum %d state %d currpid %d\n", optr->pname, optr->quantum, optr->pstate, currpid);
		
		//int rr_val = get_round_robin(optr, &nptr);
		int rr_val = 0;
		if(rr_val == 0) {
			int val = get_linux_proc();
			//kprintf("val after first get proc %d\n", val);
			if(val < 0) {
				init_epoch();
				// may need to re initialize ready queue
				val = get_linux_proc();
				//kprintf("val after init epoch %d\n", val);
				if(val < 0) { // only null proc is ready
					update_optr(optr);
					nptr = &proctab[NULLPROC];
					currpid = dequeue(NULLPROC);
					//kprintf("dequeued null proc currpid %d val %d\n", currpid, val);
				} else {
					update_optr(optr);
					currpid = dequeue(val);
					//kprintf("set currentpid to %d\n", currpid);
					nptr = &proctab[currpid];
				}
			} else {
				update_optr(optr);
				currpid = dequeue(val);
				//kprintf("set currentpid to %d\n", currpid);
				nptr = &proctab[currpid];
			}
		}
		add_round_robin_lx(nptr);
		nptr->pstate = PRCURR;
		nptr->has_run = 1;
		//if(strcmp(nptr->pname, "main") == 0 || strcmp(nptr->pname, "proc C") == 0)
		//	kprintf("chose %s currpid %d\n", nptr->pname, currpid);
	#ifdef  RTCLOCK
                preempt = QUANTUM;              /* reset preemption counter     */
        #endif
		ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
		restore(ps);
		return OK;
	} else if(optr->rr_next != NULL) {
		kprintf("round robin not null\n");
		restore(ps);
		return OK;
	} else {
		restore(ps);
		return OK;
	}
	//kprintf("fell off linux sched\n");
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
	int ret_val = 0;
	if(curr_sched_class == EXPDISTSCHED) {
		//kprintf("calling exp dist\n");
		ret_val = sched_exp_dist();
	} else if(curr_sched_class == LINUXSCHED) {
		ret_val = linux_sched();
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
	//#ifdef	RTCLOCK
	//	preempt = QUANTUM;		/* reset preemption counter	*/
	//#endif
	return ret_val;
}
