/* getpid.c - getpid */

#include <conf.h>
#include <kernel.h>
#include <proc.h>

/*------------------------------------------------------------------------
 * getpid  --  get the process id of currently executing process
 *------------------------------------------------------------------------
 */
extern unsigned long ctr1000;
extern void updatesysarr(char* name, unsigned long duration);

SYSCALL getpid()
{
	unsigned long start = ctr1000;
	unsigned long duration = start - ctr1000;
	updatesysarr("getpid", duration);
	return(currpid);
}
