/* resume.c - resume */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 * resume  --  unsuspend a process, making it ready; return the priority
 *------------------------------------------------------------------------
 */
extern unsigned long ctr1000;
extern void updatesysarr(char* name, unsigned long duration);
extern int track_sys_calls;

SYSCALL resume(int pid)
{
	unsigned long start = ctr1000;
	STATWORD ps;    
	struct	pentry	*pptr;		/* pointer to proc. tab. entry	*/
	int	prio;			/* priority to return		*/

	disable(ps);
	if (isbadpid(pid) || (pptr= &proctab[pid])->pstate!=PRSUSP) {
		restore(ps);
		return(SYSERR);
	}
	prio = pptr->pprio;
	ready(pid, RESCHYES);
	restore(ps);
	unsigned long duration = ctr1000 - start;
	if(track_sys_calls == 1)
		updatesysarr("resume", duration);
	return(prio);
}
