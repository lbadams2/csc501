#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <lock.h>
#include <stdio.h>

void set_bit(int, lentry *);
void set_proc_bit(int, struct pentry *, int, int);

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
        kprintf("pid: %d lock is free\n", pid);
        if(type == READ) {
            kprintf("pid: %d read lock\n", pid);
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
        if(type == READ) {
            kprintf("pid: %d about to call sem post\n", pid);
            sem_post(lptr, ldes, READ, 1);
        }
    } else { // lock is not free
        // do not wait on lock if its previously been acquired and has been recreated by another proc
        int pid_create = pptr->locks_held[ldes];
        if(pid_create != -1 && pid_create != lptr->create_pid)
            return SYSERR;
        
        if(lptr->write_lock == 0 && lptr->readers == 0) { // lock is held for writing, must wait
            //restore(ps);
            //prio_inh(lptr, pptr->pprio);
            wait_ret = lwait(lptr, ldes, priority, type);
            //disable(ps);
            set_bit(pid, lptr);
            pptr->lock_type = 0; // not waiting on lock
            set_proc_bit(ldes, pptr, type, lptr->create_pid);
            if(type == READ) {
                lptr->bin_lock--;
                lptr->readers++;
                if(lptr->readers == 1) { 
                    //lwait(lptr, ldes, priority, WRITE);
                    lptr->write_lock--;
                }
                sem_post(lptr, ldes, READ, 1);
            } else {
                //sem_post(lptr, ldes, WRITE);
            }
            
        } else { // lock is held for reading
            if(type == READ) {
                kprintf("pid: %d lock held for reading\n", pid);
                lqent *lqhead;
                int head = get_wq_head(ldes);
                int tail = head + 1;
                int wlprio;
                //struct pentry *tmp;
                lqhead = &wq[head];
                int next = lqhead->qnext;
                while(next != tail) {
                    //kprintf("pid: %d there is a proc waiting to write\n", pid);
                    wlprio = wq[next].qkey;
                    tmp = &proctab[next];
                    // if proc is waiting on lock, is writer, and has higher priority new proc must wait
                    if(tmp->lock_type == WRITE && priority < wlprio) { 
                        kprintf("pid: %d lock prio less than waiting writer\n");
                        //restore(ps);
                        // don't need prio_inh here because there is already process waiting with higher prio
                        lwait(lptr, ldes, priority, type);
                        //disable(ps); // should wake up here from signal indicating it can acquire lock                    
                        lptr->readers++; // can acquire lock and begin reading, just means it can proceed with its process
                        // lock is held for reading so don't need to acquire write lock
                        set_bit(pid, lptr);
                        pptr->lock_type = 0; // not waiting on lock
                        set_proc_bit(ldes, pptr, type, lptr->create_pid);
                        sem_post(lptr, ldes, READ, 1);
                        restore(ps);
                        // need to signal other procs before returning
                        return 0;
                    }
                    next = wq[next].qnext;
                }
                // if here current proc has higher priority than waiting writer or only readers waiting on lock
                lptr->bin_lock--;
                lptr->readers++;
                set_bit(pid, lptr);
                pptr->lock_type = 0; // not waiting on lock
                set_proc_bit(ldes, pptr, type, lptr->create_pid);
                kprintf("pid: %d about to call sem post\n", pid);
                sem_post(lptr, ldes, READ, 1);
		        kprintf("pid %d done with sem post\n", pid);
        } else { // trying to acquire write lock and a proc already holds the lock, must wait
                //restore(ps);
                kprintf("pid: %d trying to acquire lock for writing\n", pid);
                //prio_inh(lptr, pptr->pprio);
                lwait(lptr, ldes, priority, type);
                //disable(ps);
                set_bit(pid, lptr);
                pptr->lock_type = 0; // not waiting on lock
                set_proc_bit(ldes, pptr, type, lptr->create_pid);
                //sem_post(lptr, ldes, WRITE);
            }
        }
    }
    kprintf("pid: %d about to return from lock\n", pid);
    restore(ps);
    // if read signal other procs before returning
    return 0;
}

//int get_bit(int bit_pos, lentry *lptr) {
//    return (1 << bit_pos) & lptr->procs_holding;
//}

// only needs to be called when a reader is waiting on a writer or vice versa, doesn't need to be called for multiple readers
// increase priority of proc holding lock if procs waiting have higher priority
void prio_inh(lentry *lptr, int prio) {
    kprintf("pid: %d in prio inh\n", currpid);
    int i;
    unsigned long long tmp = 0;
    struct pentry *hold_pptr;
    for(i = 0; i < NPROC; i++) {        
        tmp = lptr->procs_holding >> i;
        tmp = tmp & 0x1;
        if(tmp == 1) {
            kprintf("pid: %d in prio inh proc %d holding lock\n", currpid, i);
            hold_pptr = &proctab[i];
            if(prio > hold_pptr->pprio) {
                kprintf("pid: %d prio greater than holding proc prio\n", currpid);
                hold_pptr->pinh = prio;
                if(hold_pptr->oprio == -1)
                    hold_pptr->oprio = hold_pptr->pprio;
                hold_pptr->pprio = prio;
                // if hold_pptr is waiting on any locks need to increase holding procs prio there too
                if(hold_pptr->wait_lock >= 0) {
                    lentry *nlptr = &locktab[hold_pptr->wait_lock];
                    kprintf("pid: %d about to make recursive call to prio inh %d waiting on %d\n", currpid, i, hold_pptr->wait_lock);
                    prio_inh(nlptr, prio);
                }
            }
        }
    }
}

void print_holding_procs(lentry *lptr) {
    int i;
    unsigned long long tmp = 0;
    for(i = 0; i < NPROC; i++) {
        tmp = lptr->procs_holding >> i;
        tmp = tmp & 0x1;
        if(tmp == 1)
            kprintf("proc %d holding lock\n", i);
    }
}

void set_bit(int bit_ix, lentry *lptr) {
    kprintf("pid: %d setting bit %d in procs holding\n", currpid, bit_ix);
    kprintf("procs holding before setting bit size of ull %d:\n", sizeof(unsigned long long));
    print_holding_procs(lptr);
    unsigned long long tmp = lptr->procs_holding;
    tmp = (1ULL << bit_ix) | tmp;
    lptr->procs_holding = tmp;
    kprintf("procs holding after setting bit:\n");
    print_holding_procs(lptr);
}

// set this to remember what proc created lock when acquiring proc first acquired it
// if proc tries to wait on lock that's been deleted and recreated return error
void set_proc_bit(int ldes, struct pentry *pptr, int lock_type, int create_pid) {
    pptr->locks_held[ldes] = create_pid;
    //unsigned long tmp = pptr->locks_held;
    //tmp = (1 << ldes) | tmp;
    //pptr->locks_held = tmp;
    unsigned long long tmp;
    if(lock_type == WRITE) {
        tmp = pptr->rw_lflags;
        tmp = (1UL << ldes) | tmp;
        pptr->rw_lflags = tmp;
    }
}

int get_wq_head(int ldes) {
    int head = (ldes * 2) + NLOCKS;
    /*
    if(type == READ)
        head = (ldes * 4) + NLOCKS;
    else
        head = (ldes * 4) + NLOCKS + 2;
    */
    return head;
}

void print_wq(int ldes) {
    int next = get_wq_head(ldes);
    int tail = next + 1;
    int prio;
    kprintf("print wq read head %d prio %d\n", next, wq[next].qkey);
    while(wq[next].qnext != tail) {        
        next = wq[next].qnext;
        prio = wq[next].qkey;
        kprintf("print wq read next %d prio %d\n", next, prio);
    }
    /*
    next = get_wq_head(ldes, WRITE);
    tail = next + 1;
    kprintf("print wq write head %d prio %d\n", next, wq[next].qkey);
    while(wq[next].qnext != tail) {        
        next = wq[next].qnext;
        prio = wq[next].qkey;
        kprintf("print wq write next %d prio %d\n", next, prio);
    }
    */
}

// make sure head of queue has greatest key
void enqueue_wq(int ldes, int proc, int prio, struct pentry *pptr) {
    lentry *lptr = &locktab[ldes];
    if(pptr->pprio > lptr->lprio) {
        lptr->lprio = pptr->pprio;
    }

    int next = 0, prev;
    next = get_wq_head(ldes);
    while(wq[next].qkey >= prio)
        next = wq[next].qnext;
    wq[proc].qnext = next;
    wq[proc].qprev = prev = wq[next].qprev;
    wq[proc].qkey = prio;
    wq[prev].qnext = proc;
    wq[next].qprev = proc;
    print_wq(ldes);
    update_lprio(ldes);
}

void remove_wq(int ldes, int proc) {
    kprintf("pid: %d removing itself from lock %d wq\n", currpid, ldes);
    int prev = wq[proc].qprev;
    int next = wq[proc].qnext;
    wq[prev].qnext = next;
    wq[next].qprev = prev;
}

// want to skip current pid when release(shouldn't ever happen) and kill
// if calling from sem_post lock is being released so current proc shouldn't be in wait queue
void update_lprio(int ldes) {
    int head, tail, next, max_prio = -1;
    int pid = getpid();
    struct pentry *pptr = &proctab[pid];
    lentry *lptr = &locktab[ldes];
    head = get_wq_head(ldes);
    tail = head + 1;
    next = head;
    while(next != tail) {
        if(next == pid) {
            next = wq[next].qnext;
            continue;
        }
        pptr = &proctab[next];
        if(pptr->pprio > max_prio)
            max_prio = pptr->pprio;
        next = wq[next].qnext;
    }

    // need to check both queues
    /*
    head = get_wq_head(ldes, WRITE);
    tail = head + 1;
    next = head;
    while(next != tail) {
        if(next == pid) {
            next = wq[next].qnext;
            continue;
        }
        pptr = &proctab[next];
        if(pptr->pprio > max_prio)
            max_prio = pptr->pprio;
        next = wq[next].qnext;
    }
    */
    
    if(max_prio != lptr->lprio) {
        kprintf("pid: %d new max prio %d\n", pid, max_prio);
        lptr->lprio = max_prio;
        prio_inh(lptr, lptr->lprio);
    } else {
        kprintf("pid: %d same max prio %d\n", pid, max_prio);
    }
}

// need to adjust priority inversion
int dequeue_wq(int ldes) {
    int head = 0, tail = 0, next = 0, dq = 0;
    lqent *lqhead;
    head = get_wq_head(ldes);
    tail = head + 1;
    lqhead = &wq[head];
    dq = lqhead->qnext;
    if(dq != tail) {
        next = wq[dq].qnext;
        wq[next].qprev = head;
        lqhead->qnext = next;
    }
    lentry *lptr = &locktab[ldes];
    struct pentry *pptr = &proctab[dq];
    if(pptr->pprio == lptr->lprio) {
        update_lprio(ldes);
    }
    

    return dq;
}

void sem_wait(lentry *lptr, int sem, int prio, int lock_type, int ldes) {
    if(sem == 0) { // bin sem
        lptr->bin_lock--;
        if(lptr->bin_lock < 0) { // wait
            lwait(lptr, ldes, prio, lock_type);
        }
    } else { // write sem
        lptr->write_lock--;
        if(lptr->write_lock < 0) { // wait
            lwait(lptr, ldes, prio, lock_type);
        }
    }

}


void sem_post(lentry * lptr, int ldes, int lock_type, int do_resched) {
    int proc;
    int pid = getpid();
    if(lock_type == READ) { // bin sem
        lptr->bin_lock++;
        // only wake waiting procs if last reader
        if(lptr->readers == 0) {
            proc = dequeue_wq(ldes);
            kprintf("pid: %d dequeued proc %d\n", pid, proc);
            if(proc < NPROC)
                ready(proc, do_resched);
        }
    } else { // write sem
        lptr->write_lock++;
        // always wake waiting procs if writer
        proc = dequeue_wq(ldes);
        if(proc < NPROC)
            ready(proc, do_resched);
    }
    kprintf("pid: %d about to return from sem post\n", pid);
}
