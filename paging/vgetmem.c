/* vgetmem.c - vgetmem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>
#include <paging.h>

extern struct pentry proctab[];
/*------------------------------------------------------------------------
 * vgetmem  --  allocate virtual heap storage, returning lowest WORD address
 *------------------------------------------------------------------------
 */
WORD	*vgetmem(nbytes)
	unsigned nbytes;
{
	int pid = getpid();
	struct pentry *pptr = &proctab[pid];
	// get from backing store, create page table, move into memory?
	// may create a page fault
	struct *pd_t pd = pptr->pdbr;
	// create page table
	if(pd->pd_pres == 0) {

	} else {
		struct *pt_t pt = pd->pd_base;
		
	}
	return( SYSERR );
}


