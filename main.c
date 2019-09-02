/* user.c - main */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 *  main  --  user main program
 *------------------------------------------------------------------------
 */

//extern long zfunction(long param);
extern void syscallsummary_start();
extern void syscallsummary_stop()
extern void printsyscallsummary();

long mask_shift(long num) {
    u_int32_t mask = 0xfffc01ff;
    long result = num & mask;
    result = result >> 8;
    return result;
}

int main()
{
        kprintf("\n\nHello World, Xinu lives\n\n");
        //zfunction(156);
        //printtos();
        //printprocstks(0);
        syscallsummary_start();
        getpid();
        syscallsummary_stop();
        printsyscallsummary();
        return 0;
}