/* vgetmem.c - vgetmem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>
#include <paging.h>

extern struct pentry proctab[];
WORD *getvhp(struct pentry *pptr, unsigned int nbytes);
/*------------------------------------------------------------------------
 * vgetmem  --  allocate virtual heap storage, returning lowest WORD address
 *------------------------------------------------------------------------
 */
WORD	*vgetmem(nbytes)
	unsigned nbytes;
{
	int pid = getpid();
	struct pentry *pptr = &proctab[pid];
	if(pptr->hsize * NBPG < nbytes) {
		// page fault
	}
	virt_addr_t vaddr = getvhp(pptr, nbytes);
	// get from backing store, create page table, move into memory?
	struct pd_t *pd = pptr->pdbr;
	// create page table
	// page is 4 KB
	unsigned int pd_off = vaddr.pd_offset;
	if(pd->pd_pres == 0) {

		struct pt_t *pt = getmem(sizeof(struct pt_t));
		pd->pd_pres = 1;
		pd->pd_base = pt;

	} else {
		struct pt_t *pt = pd->pd_base;

	}
	return( SYSERR );
}


virt_addr_t getvhp(struct pentry *pptr, unsigned int nbytes) {
	struct	mblock	*p, *q, *leftover;
	p = pptr->vmemlist;
	//unsigned int nbytes = hsize * NBPG;
	nbytes = (unsigned int) roundmb(nbytes);
	if(p->mnext== (struct mblock *) NULL) {
		if(p->mlen < nbytes)
			return( (virt_addr_t)SYSERR );
		else {
			leftover = (struct mblock *)( (unsigned)p + nbytes );
			leftover->mnext = NULL;
			leftover->mlen = p->mlen - nbytes;
			return( (virt_addr_t)p );
		}
	}
	else {
		for (q= p,p=p->mnext ; p != (struct mblock *) NULL; q=p,p=p->mnext)
			// if block is exactly right size return it and remove it from list
			if ( p->mlen == nbytes) {
				q->mnext = p->mnext;
				return( (virt_addr_t)p );
			} else if ( p->mlen > nbytes ) {
				// create new block starting from the memory chosen block leaves off at
				leftover = (struct mblock *)( (unsigned)p + nbytes );
				q->mnext = leftover;
				leftover->mnext = p->mnext;
				leftover->mlen = p->mlen - nbytes;
				return( (virt_addr_t)p );
			}
	}
	return( (virt_addr_t)SYSERR );
}