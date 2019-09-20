#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <sched.h>

int rrq[NPROC];
int rrq_front = 0;
int rrq_back = 0;
int rrq_size = 0;

void init_rrq() {
    int i;
    for(i = 0; i < NPROC; i++)
        rrq[i] = -1;
}

void rr_enqueue(int proc) {
	if(rrq_size < NPROC) {
		rrq_back = (rrq_back + 1) % NPROC;
        rrq[rrq_back] = proc;
        rrq_size++;
	}
}

int rr_dequeue() {
	if(rrq_size == 0)
		return -1;
	else {
        int proc = rrq[rrq_front];
        struct pentry* pptr = &proctab[proc];
        if(pptr->pstate == PRREADY) {
            rrq_size--;
            rrq_front = (rrq_front + 1) % NPROC;
            return proc;
        } else
		    return -1;
	}
}

int rr_isempty() {
    if(rrq_size == 0)
        return 1;
    else
        return 0;
}

int rr_contains(int proc) {
    if(rrq_size == 0)
        return 0;
	int contains = 0, i = 0;
    if(rrq_front > rrq_back) {
        for(i = rrq_front; i < NPROC; i++)
            if(rrq[i] == proc)
                return 1;
        for(i = 0; i <= rrq_back; i++)
            if(rrq[i] == proc)
                return 1;
    } else {
        for(i = rrq_front; i <= rrq_back; i++)
            if(rrq[i] == proc)
                return 1;
    }
    return contains;
}