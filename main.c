/* user.c - main */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 *  main  --  user main program
 *------------------------------------------------------------------------
 */

extern long zfunction(long param);

long mask_shift(long num) {
    u_int32_t mask = 0xfffc01ff;
    long result = num & mask;
    result = result >> 8;
    return result;
}

int main()
{
        kprintf("\n\nHello World, Xinu lives\n\n");
        zfunction(156);
        return 0;
}