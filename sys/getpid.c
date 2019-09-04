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
extern int track_sys_calls;

SYSCALL getpid()
{
	unsigned long start = ctr1000;
	unsigned long duration = ctr1000 - start;
	if(track_sys_calls == 1)
		updatesysarr("getpid", duration);
	return(currpid);
}
