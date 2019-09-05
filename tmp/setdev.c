/* setdev.c - setdev */

#include <conf.h>
#include <kernel.h>
#include <proc.h>

/*------------------------------------------------------------------------
 *  setdev  -  set the two device entries in the process table entry
 *------------------------------------------------------------------------
 */
extern unsigned long ctr1000;
extern void updatesysarr(char* name, unsigned long duration);
extern int track_sys_calls;

SYSCALL	setdev(int pid, int dev1, int dev2)
{
	unsigned long start = ctr1000;
	short	*nxtdev;

	if (isbadpid(pid))
		return(SYSERR);
	nxtdev = (short *) proctab[pid].pdevs;
	*nxtdev++ = dev1;
	*nxtdev = dev2;
	unsigned long duration = ctr1000 - start;
	if(track_sys_calls == 1)
		updatesysarr("setdev", duration);
	return(OK);
}
