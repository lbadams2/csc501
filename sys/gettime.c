/* gettime.c - gettime */

#include <conf.h>
#include <kernel.h>
#include <date.h>

extern int getutim(unsigned long *);

/*------------------------------------------------------------------------
 *  gettime  -  get local time in seconds past Jan 1, 1970
 *------------------------------------------------------------------------
 */
extern unsigned long ctr1000;
extern void updatesysarr(char* name, unsigned long duration);

SYSCALL	gettime(long *timvar)
{
    unsigned long start = ctr1000;
    /* long	now; */

	/* FIXME -- no getutim */
    unsigned long duration = start - ctr1000;
	updatesysarr("gettime", duration);
    return OK;
}
