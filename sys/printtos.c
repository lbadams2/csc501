
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>

static unsigned long *sp, *fp;
void printtos() {
        //unsigned long *sp, *fp;
        int x = 1;
        int y = 2;
        asm("movl %esp,sp");
        asm("movl %ebp,fp");
        kprintf("Before[0x%08x]: 0x%08x\n", fp, *fp); // address of fp is stack address not contents of fp
        kprintf("After [0x%08x]: 0x%08x\n", sp, *sp); // address of sp is stack address not contents of sp
        kprintf("\telement[0x%08x] = 0x%08x\n", sp - 1, *(sp - 1));
        kprintf("\telement[0x%08x] = 0x%08x\n", sp - 2, *(sp - 2));
        kprintf("\telement[0x%08x] = 0x%08x\n", sp - 3, *(sp - 3));
        kprintf("\telement[0x%08x] = 0x%08x\n", sp - 4, *(sp - 4));
}