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
	int i;
	struct vmblock *vmb;
	int pid = getpid();
	int npages = (nbytes + (NBPG -1)) / NBPG;
	struct pentry *pptr = &proctab[pid];
	int store, pageth, avail = -1;
	// find heap with enough space
	for(i = 0; i < 8; i++) {
		vmb = &pptr->vmemlist[i];
		if(vmb->npages >= npages) {
			// use bsm lookup to make sure its not mapped
			bsm_lookup(pid, vmb->start, &store, &pageth);
			if(store == -1) {
				get_bsm(&avail);
				if(avail == -1) {
					kprintf("no backing store available");
					return NULL;
				}
				pptr->store[avail] = 1;
				int vpno = vmb->start >> 12;
				bsm_map(pid, vpno, avail, npages);
				break;
			}
		}
	}
	if(avail == -1) {
		kprintf("no backing store available");
		return NULL;
	}
	return vmb->start;
}


