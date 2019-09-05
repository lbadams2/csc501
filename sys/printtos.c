
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>

static unsigned long *sp, *fp;
void printtos() {
        //unsigned long    *sp, *fp;
        asm("movl %esp,sp");
        asm("movl %ebp,fp");
        kprintf("Before[%08x]: %08x\n", fp, *fp);
        kprintf("After [%08x]: %08x\n", sp, *sp);
        kprintf("\telement[%08x] = %08x", sp - 1, *(sp - 1));
        kprintf("\telement[%08x] = %08x", sp - 2, *(sp - 2));
        kprintf("\telement[%08x] = %08x", sp - 3, *(sp - 3));
        kprintf("\telement[%08x] = %08x", sp - 4, *(sp - 4));
}