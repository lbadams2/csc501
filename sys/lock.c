#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <lock.h>

// lock is passed to create as one of the dynamic args
int lock(int ldes, int type, int priority) {
    lentry *lptr = &locktab[ldes];
    int pid = getpid();
    struct pentry *pptr = &proctab[pid];
    struct pentry *tmp;
    if(lptr->procs_holding == 0) { // lock is free
        if(type == READ) {
            int i, wait = 0;
            unsigned int proc = 0;
            for(i = 0; i < NPROC; i++) {
                proc = get_bit(i); // get ith bit
                tmp = &proctab[i];
                // if proc has lock, is writer, and has higher priority new proc must wait
                if(proc == 1 && tmp->lock_type == WRITE && pptr->pprio < tmp->pprio) { 
                    wait = 1;
                    lwait(lptr, sem, ldes, prio, lock_type);
                    wait = 0; // should wake up here
                }
            }
            if(!wait) { // can acquire lock and begin reading
                lptr->readers++;
                set_bit(pid, lptr);

            }
            
        } else {

        }
    } else {
        enqueue_wq(ldes, pid, priority);
    }
}

int get_bit(int bit_pos, lentry *lptr) {
    return (1 << bit_pos) & lptr->procs_holding;
}

void set_bit(int bit_ix, lentry *lptr) {
    int tmp = lptr->procs_holding;
    tmp = (1 << bit_ix) | tmp;
    lptr->procs_holding = tmp;
}

// make sure head of queue has greatest key
void enqueue_wq(int ldes, int proc, int prio) {
    lentry *lptr = &locktab[ldes];
    struct qent *wqptr = lptr->wq;
    int next = wqptr[WQHEAD].qnext;
    int prev;
    while(wqptr[next].qkey < prio)
        next = wqptr[next].qnext;

    wqptr[proc].qnext = next;
    wqptr[proc].qprev = prev = wqptr[next].qprev;
    wqptr[proc].qkey = prio;
    wqptr[prev].qnext = proc;
    wqptr[next].qprev = proc;
}

void remove_wq(int ldes, int proc) {
    lentry *lptr = &locktab[ldes];
    struct qent *wqptr = lptr->wq;
    int prev = wqptr[proc].qprev;
    int next = wqptr[proc].qnext;
    wqptr[prev].qnext = next;
    wqptr[next].qprev = prev;
}

int dequeue_wq(int ldes) {
    lentry *lptr = &locktab[ldes];
    struct qent *head = &lptr->wq[WQHEAD];
    int head_proc = head->qnext;
    int next = &lptr->wq[head_proc].qnext;
    wqptr[next].qprev = WQHEAD;
    head->qnext = next;
    return head_proc;
}

void sem_wait(lentry *lptr, int sem, int prio, int lock_type, int ldes) {
    if(sem == 0) { // bin sem
        lptr->bin_lock--;
        if(lptr->bin_lock < 0) { // wait
            lwait(lptr, sem, ldes, prio, lock_type);
        }
    } else { // write sem
        lptr->write_lock--;
        if(lptr->write_lock < 0) { // wait
            lwait(lptr, sem, ldes, prio, lock_type);
        }
    }

}


void sem_post(lentry *, int) {
    if(sem == 0) { // bin sem
        lptr->bin_lock++;
        if(lptr->wq[WQHEAD].qnext != WQTAIL) { // wait queue not empty, wake proc

        }
    } else { // write sem
        lptr->write_lock++;
        if(lptr->wq[WQHEAD].qnext != WQTAIL) { // wait queue not empty, wake proc

        }
    }
}