#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>

// need to think about using disable and restore
// need to create semaphores
int lcreate() {
    int i;
    lentry *lptr;
    for(i = 0; i < NLOCKS; i++) {
        lptr = &locktab[i];
        if(lptr->status == LFREE || lptr->status == LDELETED) {
            lptr->status = LACTIVE;
            lptr->create_pid = getpid();
            break;
        }
    }
    if(i < NLOCKS)
        return i;
    else
        return SYSERR;
}
