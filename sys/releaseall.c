#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <lock.h>

// think about other vars in proc and lock not unsetting currently
int releaseall(int nlocks, long	locks) {
    STATWORD ps;
    unsigned long	*a;		/* points to list of args	*/
    a = (unsigned long *)(&locks) + (nlocks-1); /* last argument	*/
    unsigned long ldes;
    disable(ps);
    int inval = 0, iswrt = 0, proc = NPROC;
    lentry *lptr;
    for ( ; nlocks > 0 ; nlocks--) {
        ldes = *a--;
        int val = is_valid_lock(ldes);
        if(!val) {
            inval = 1;
            continue;
        }
        lptr = &locktab[ldes];
        unset(ldes, lptr);
        iswrt = is_write(ldes);
        if(iswrt) {
            lptr->write_lock--;
        } else {
            lptr->readers--;
            if(lptr->readers == 0)
                lptr->write_lock--;
            lptr->bin_lock--;
        }
        proc = dequeue_wq(ldes);
        if(proc < NPROC)
            ready(proc, 0);
    }
    restore(ps);
    if(inval)
        return SYSERR;
    return(OK);
}

int is_write(int ldes) {
    int pid = getpid();
    struct pentry *pptr = &proctab[pid];
    unsigned long rwfl = pptr->rw_lflags;
    rwfl = (rwfl >> ldes) & 0x1;
    return rwfl;
}

void unset(int ldes, lentry *lptr) {
    int pid = getpid();
    struct pentry *pptr = &proctab[pid];
    unsigned long lh = pptr->locks_held;
    lh = ~(1 << ldes) & lh;
    pptr->locks_held = lh;

    unsigned int ph = lptr->procs_holding;
    ph = ~(1 << pid) & ph;
    lptr->procs_holding = ph;
}

int is_valid_lock(int ldes) {
    int pid = getpid();
    struct pentry *pptr = &proctab[pid];
    unsigned long lh = pptr->locks_held;
    lh = (lh >> ldes) & 0x1;
    return lh;
}