/* signal.c - signal */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 * signal  --  signal a semaphore, releasing one waiting process
 *------------------------------------------------------------------------
 */
SYSCALL lsignal(lentry *lptr, int ldes, int lock_type)
{
	STATWORD ps;

	disable(ps);
    int proc;
    if(lock_type == READ) {
        lptr->readers--;
        if(lptr->readers == 0) {
            lptr->write_lock++;
        }
        lptr->bin_lock++;
        proc = dequeue_wq(ldes);
        if(proc < NPROC)
            ready(proc, RESCHYES);
    } else {
        lptr->write_lock++;
        proc = dequeue_wq(ldes);
        if(proc < NPROC)
            ready(proc, RESCHYES);
    }
    restore(ps);
}