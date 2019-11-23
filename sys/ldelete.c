#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <lock.h>

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
    while( (pid=dequeue_wq(ldes)) != WQTAIL) {
        proctab[pid].pwaitret = DELETED;        
        ready(pid, 0);
    }
    resched();
	restore(ps);
	return(OK);
}