#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

int is_valid_lock(int);
void unset(int, lentry *);
int is_write(int);
void set_prio();
void update_wq_release(int);

// think about other vars in proc and lock not unsetting currently
int releaseall(nlocks, locks) 
	int nlocks;
	long locks;
{
    STATWORD ps;
    unsigned long	*a;		/* points to list of args	*/
    a = (unsigned long *)(&locks) + (nlocks-1); /* last argument	*/
    unsigned long ldes;
    disable(ps);
    int inval = 0, iswrt = 0;
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
            sem_post(lptr, ldes, WRITE, 0);
        } else {
            lptr->readers--;
            if(lptr->readers == 0) {
                sem_post(lptr, ldes, WRITE, 0);
            }
            lptr->bin_lock--;
            sem_post(lptr, ldes, READ, 0);
        }
        update_wq_release(ldes);
        //if(proc < NPROC)
        //    ready(proc, 0);
    }
    set_prio();    
    restore(ps);
    if(inval)
        return SYSERR;
    return(OK);
}

// set proc prio to max prio of any procs waiting on any locks proc still holds
void set_prio() {
    int pid = getpid();
    struct pentry *pptr = &proctab[pid];
    int i, max_prio = 0;
    lentry *lptr;
    for(i = 0; i < NLOCKS; i++) {
        if(pptr->locks_held[i] != -1) {
            lptr = &locktab[i];
            if(lptr->lprio > max_prio)
                max_prio = lptr->lprio;
        }
    }
    if(max_prio > 0) {
        pptr->pprio = max_prio;
        pptr->pinh = max_prio;
    }
    else { // no procs waiting on any locks held by currpid
        pptr->pinh = -1;
        pptr->pprio = pptr->oprio;
    }
}

int is_write(int ldes) {
    int pid = getpid();
    struct pentry *pptr = &proctab[pid];
    unsigned long long rwfl = pptr->rw_lflags;
    rwfl = (rwfl >> ldes) & 0x1;
    return rwfl;
}

void unset(int ldes, lentry *lptr) {
    int pid = getpid();
    struct pentry *pptr = &proctab[pid];
    pptr->locks_held[ldes] = -1;
    //unsigned long lh = pptr->locks_held;
    //lh = ~(1 << ldes) & lh;
    //pptr->locks_held = lh;

    unsigned long long ph = lptr->procs_holding;
    ph = ~(1 << pid) & ph;
    lptr->procs_holding = ph;
}

int is_valid_lock(int ldes) {
    int pid = getpid();
    struct pentry *pptr = &proctab[pid];
    if(pptr->locks_held[ldes] != -1)
        return 1;
    else
        return 0;
    //unsigned long lh = pptr->locks_held;
    //lh = (lh >> ldes) & 0x1;
    //return lh;
}

void update_wq_release(int ldes) {	
    int pid = getpid();
    struct pentry *pptr = &proctab[pid];
	lentry *lptr = &locktab[ldes];
	if(pptr->pprio == lptr->lprio) { // may need to change lprio
		update_lprio(ldes);
	}
}
