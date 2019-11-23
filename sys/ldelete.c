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
    while( (pid=dequeue_wq(ldes)) != WQTAIL) {
        proctab[pid].pwaitret = DELETED;
        ready(pid,RESCHNO);
    }
    resched();
	restore(ps);
	return(OK);
}