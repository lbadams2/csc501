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

int main()
{
        kprintf("\n\nHello World, Xinu lives\n\n");
        zfunction(156);
        return 0;
}