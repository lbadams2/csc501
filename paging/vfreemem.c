/* vfreemem.c - vfreemem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>

extern struct pentry proctab[];
/*------------------------------------------------------------------------
 *  vfreemem  --  free a virtual memory block, returning it to vmemlist
 *------------------------------------------------------------------------
 */
SYSCALL	vfreemem(block, size)
	struct	vmblock	*block;
	unsigned size;
{
	int pid = getpid();
	struct pentry *pptr = proctab[pid];
	struct vmblock *vmb;
	int i;
	for(i = 0; i < 8; i++) {
		vmb = pptr->vmemlist[i];
		if(vmb->start == block->start) {
			vmb->start = NULL;
			vmb->npages = 0;
		}
	}
	return(OK);
}
