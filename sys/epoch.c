#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <stdio.h>
#include <sched.h>

void init_epoch() {
    STATWORD ps;
    int i;
    struct pentry* pptr;
    disable(ps);
    for(i = 0; i < NPROC; i++) {
        pptr = &proctab[i];
        pptr->eprio = pptr->pprio;
        pptr->isnew = 0;
        if(pptr->has_run == 0 || pptr->quantum == 0)
            pptr->quantum = pptr->pprio;
        else if(pptr->has_run == 1 && pptr->quantum > 0)
            pptr->quantum = pptr->quantum / 2 + pptr->pprio;
        //pptr->has_run_epch = 0;
        kprintf("init epoch initialized %s quantum %d\n", pptr->pname, pptr->quantum);
    }
    restore(ps);
}