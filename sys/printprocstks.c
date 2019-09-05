#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>

static unsigned long *sp;

void printprocstks(int priority) {
    struct pentry *proc;
    int i;
    //size_t len = sizeof(proctab) / sizeof(pentry);
    kprintf("\nvoid printprocstks(int priority)\n");
    for (i = 0; i < NPROC; i++) {
        proc = &proctab[i];
        int pri = proc->pprio; // process priority
        if(pri > priority) {
            WORD stack_base = proc->pbase; // stack base
            int size = proc->pstklen; // stack size
            WORD limit = proc->plimit; // stack limit
            char* name = proc->pname; // process name

            if(i == currpid)
                asm("movl %esp,sp");
            else
                sp = (unsigned long *)proc->pesp;
            kprintf("Process [%s]\n", name);
            kprintf("\tpid: %d\n", i);
            kprintf("\tpriority: %d\n", pri);
            kprintf("\tbase: 0x%08x\n", stack_base);
            kprintf("\tlimit: 0x%08x\n", limit);
            kprintf("\tlen: %d\n", size);
            kprintf("\tpointer: 0x%08x\n", sp);
        }
    }
}