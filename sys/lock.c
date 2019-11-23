#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <lock.h>

// lock is passed to create as one of the dynamic args
// do not return until process acquires the lock
int lock(int ldes, int type, int priority) {
    STATWORD ps;
    lentry *lptr = &locktab[ldes];
    if(lptr->status == LDELETED)
        return SYSERR;
    int pid = getpid();
    struct pentry *pptr = &proctab[pid];
    struct pentry *tmp;
    int wait_ret;
    disable(ps);
    if(lptr->procs_holding == 0) { // lock is free        
        if(type == READ) {
            lptr->bin_lock--;
            lptr->readers++;
            if(lptr->readers == 1) { // if only reader must acquire write lock too
                //restore(ps);
                //lwait(lptr, ldes, priority, WRITE);
                //disable(ps);
                lptr->write_lock--;
            }
        } else {
            lptr->write_lock--;
        }
        set_bit(pid, lptr);
        pptr->lock_type = 0; // not waiting on lock
        set_proc_bit(ldes, pptr, type, lptr->create_pid);
        sem_post(lptr, ldes, READ);
    } else { // lock is not free
        // do not wait on lock if its previously been acquired and has been recreated by another proc
        int pid_create = pptr->locks_held[ldes];
        if(pid_create != -1 && pid_create != lptr->create_pid)
            return SYSERR;
        
        if(lptr->procs_holding == 1 && lptr->readers == 0) { // lock is held for writing, must wait
            //restore(ps);
            prio_inh(lptr, pptr->pprio);
            wait_ret = lwait(lptr, ldes, priority, type);
            //disable(ps);
            set_bit(pid, lptr);
            pptr->lock_type = 0; // not waiting on lock
            set_proc_bit(ldes, pptr, type, lptr->create_pid);
            if(type == READ) {
                lptr->readers++;
                if(lptr->readers == 1) { 
                    //lwait(lptr, ldes, priority, WRITE);
                    lptr->write_lock--;
                }
                sem_post(lptr, ldes, READ);
            } else 
                //sem_post(lptr, ldes, WRITE);
            
        } else { // lock is held for reading
            if(type == READ) {
                int wait = 0;
                unsigned int proc = 0;
                int next = lptr->wq[WQHEAD].qnext;
                while(next != WQTAIL) {
                    tmp = &proctab[next];
                    // if proc is waiting on lock, is writer, and has higher priority new proc must wait
                    if(tmp->lock_type == WRITE && pptr->pprio < tmp->pprio) { 
                        wait = 1;
                        //restore(ps);
                        // don't need prio_inh here because there is already process waiting with higher prio
                        lwait(lptr, ldes, priority, type);
                        //disable(ps); // should wake up here from signal indicating it can acquire lock                    
                        lptr->readers++; // can acquire lock and begin reading, just means it can proceed with its process
                        // lock is held for reading so don't need to acquire write lock
                        set_bit(pid, lptr);
                        pptr->lock_type = 0; // not waiting on lock
                        set_proc_bit(ldes, pptr, type, lptr->create_pid);
                        sem_post(lptr, ldes, READ);
                        restore(ps);
                        // need to signal other procs before returning
                        return 0;
                    }
                    next = lptr->wq[next].qnext;
                }
                // if here current proc has higher priority than waiting writer or only readers waiting on lock
                lptr->readers++;
                set_bit(pid, lptr);
                pptr->lock_type = 0; // not waiting on lock
                set_proc_bit(ldes, pptr, type, lptr->create_pid);
                sem_post(lptr, ldes, READ);
        } else { // trying to acquire write lock and a proc already holds the lock, must wait
                //restore(ps);
                prio_inh(lptr, pptr->pprio);
                lwait(lptr, ldes, priority, type);
                //disable(ps);
                set_bit(pid, lptr);
                pptr->lock_type = 0; // not waiting on lock
                set_proc_bit(ldes, pptr, type, lptr->create_pid);
                //sem_post(lptr, ldes, WRITE);
            }
        }
    }
    restore(ps);
    // if read signal other procs before returning
    return 0;
}

//int get_bit(int bit_pos, lentry *lptr) {
//    return (1 << bit_pos) & lptr->procs_holding;
//}

// only needs to be called when a reader is waiting on a writer or vice versa, doesn't need to be called for multiple readers
void prio_inh(lentry *lptr, int prio) {
    int i, tmp = 0;
    struct pentry *hold_pptr;
    for(i = 0; i < NPROC; i++) {
        tmp = lptr->procs_holding >> i;
        tmp = tmp & 0x1;
        if(tmp == 1) {
            hold_pptr = &proctab[i];
            if(prio > hold_pptr->pprio) {
                hold_pptr->pinh = prio;
                hold_pptr->oprio = hold_pptr->pprio;
                hold_pptr->pprio = prio;
                // if hold_pptr is waiting on any locks need to increase holding procs prio there too
                if(hold_pptr->wait_lock >= 0) {
                    lentry *nlptr = &locktab[hold_pptr->wait_lock];
                    prio_inh(nlptr, prio);
                }
            }
        }
    }
}

void set_bit(int bit_ix, lentry *lptr) {
    unsigned int tmp = lptr->procs_holding;
    tmp = (1 << bit_ix) | tmp;
    lptr->procs_holding = tmp;
}

void set_proc_bit(int ldes, struct pentry *pptr, int lock_type, int create_pid) {
    pptr->locks_held[ldes] = create_pid;
    //unsigned long tmp = pptr->locks_held;
    //tmp = (1 << ldes) | tmp;
    //pptr->locks_held = tmp;

    if(lock_type == WRITE) {
        tmp = pptr->rw_lflags;
        tmp = (1 << ldes) | tmp;
        pptr->rw_lflags = tmp;
    }
}

// make sure head of queue has greatest key
void enqueue_wq(int ldes, int proc, int prio, struct pentry *pptr) {
    lentry *lptr = &locktab[ldes];
    if(pptr->pprio > lptr->lprio) {
        lptr->lprio = pptr->pprio;
    }
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
    if(head_proc != WQTAIL) {
        int next = &lptr->wq[head_proc].qnext;
        wqptr[next].qprev = WQHEAD;
        head->qnext = next;
    }
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


void sem_post(lentry * lptr, int ldes, int lock_type) {
    int proc;
    if(lock_type == READ) { // bin sem
        lptr->bin_lock++;
        proc = dequeue_wq(ldes);
        if(proc < NPROC)
            ready(proc, RESCHYES);
    } else { // write sem
        lptr->write_lock++;
        proc = dequeue_wq(ldes);
        if(proc < NPROC)
            ready(proc, RESCHYES);
    }
}