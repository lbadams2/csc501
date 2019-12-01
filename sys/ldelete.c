#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

int ldelete(int ldes) {
    STATWORD ps;    
	int	pid;
	lentry	*lptr;

	disable(ps);
	lptr = &locktab[ldes];
    lptr->status = LDELETED;
    lptr->procs_holding = 0;
    lptr->readers = 0;
    lptr->lprio = -1;
    lptr->bin_lock = 1;
    lptr->write_lock = 1;
    lptr->create_pid = -1;
    int head = get_wq_head(ldes, READ);
	int tail = head + 1;
    while( (pid=dequeue_wq(ldes, READ)) != tail) {
        proctab[pid].pwaitret = LDELETED;        
        ready(pid, 0);
    }
    head = get_wq_head(ldes, WRITE);
	tail = head + 1;
    while( (pid=dequeue_wq(ldes, WRITE)) != tail) {
        proctab[pid].pwaitret = LDELETED;        
        ready(pid, 0);
    }
    resched();
	restore(ps);
	return(OK);
}
