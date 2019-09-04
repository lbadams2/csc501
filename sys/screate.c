/* screate.c - screate, newsem */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <sem.h>
#include <stdio.h>

LOCAL int newsem();

/*------------------------------------------------------------------------
 * screate  --  create and initialize a semaphore, returning its id
 *------------------------------------------------------------------------
 */
extern unsigned long ctr1000;
extern void updatesysarr(char* name, unsigned long duration);
extern int track_sys_calls;

SYSCALL screate(int count)
{
	unsigned long start = ctr1000;
	STATWORD ps;    
	int	sem;

	disable(ps);
	if ( count<0 || (sem=newsem())==SYSERR ) {
		restore(ps);
		return(SYSERR);
	}
	semaph[sem].semcnt = count;
	/* sqhead and sqtail were initialized at system startup */
	restore(ps);
	unsigned long duration = ctr1000 - start;
	if(track_sys_calls == 1)
		updatesysarr("screate", duration);
	return(sem);
}

/*------------------------------------------------------------------------
 * newsem  --  allocate an unused semaphore and return its index
 *------------------------------------------------------------------------
 */
LOCAL int newsem()
{
	int	sem;
	int	i;

	for (i=0 ; i<NSEM ; i++) {
		sem=nextsem--;
		if (nextsem < 0)
			nextsem = NSEM-1;
		if (semaph[sem].sstate==SFREE) {
			semaph[sem].sstate = SUSED;
			return(sem);
		}
	}
	return(SYSERR);
}
