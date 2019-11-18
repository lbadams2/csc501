#include <conf.h>
#include <kernel.h>
#include <lock.h>

// need to think about using disable and restore
// need to create semaphores
int lcreate() {
    int i;
    lentry *lptr;
    for(i = 0; i < NLOCKS; i++) {
        lptr = &locktab[i];
        if(lptr->procs_holding == 0)
            break;
    }
    if(i < NLOCKS)
        return i;
    else
        return SYSERR;
}