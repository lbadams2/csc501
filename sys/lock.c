#include <conf.h>
#include <kernel.h>
#include <lock.h>


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