/* scount.c - scount */

#include <conf.h>
#include <kernel.h>
#include <sem.h>

/*------------------------------------------------------------------------
 *  scount  --  return a semaphore count
 *------------------------------------------------------------------------
 */
extern unsigned long ctr1000;
extern void updatesysarr(char* name, unsigned long duration);
extern int track_sys_calls;

SYSCALL scount(int sem)
{
	unsigned long start = ctr1000;
extern	struct	sentry	semaph[];

	if (isbadsem(sem) || semaph[sem].sstate==SFREE)
		return(SYSERR);
	unsigned long duration = ctr1000 - start;
	if(track_sys_calls == 1)
		updatesysarr("scount", duration);
	return(semaph[sem].semcnt);
}
