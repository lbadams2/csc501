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
	struct pd_t *pd = (struct pd_t *)pptr->pdbr;
	pd = pd + 4; // skip over global page tables
	int i;
	// need to create page tables when page first touched, not sure if that's here
	int npages = (nbytes + (NBPG -1)) / NBPG;
	int num_page_tables = (npages + (1024 -1)) / 1024;
	for(i = 0; i < num_page_tables; i++) {
		struct pt_t *pt = create_page_table(i, bs_id);
		pd->pd_base = pt;
		pd->pd_pres = 1;
		pd++;
	}
	
	return (WORD *)virt_addr_t;
	//return( SYSERR );
}

virt_addr_t get_virt_addr(struct mblock *p) {
	virt_addr_t vaddr;
	int ith_pg_tab = (p - 4096) / 1024; // offset into pd
	int ith_page = p % 1024; // offset into pt
	int offset = 0; // offset into page
	vaddr.pd_offset = ith_pg_tab;
	vaddr.pt_offset = ith_page;
	vaddr.pg_offset = 0;
	return vaddr;
}

// need pde for every page table
virt_addr_t getvhp(struct pentry *pptr, unsigned int nbytes) {
	struct	mblock	*p, *q, *leftover;	
	virt_addr_t vaddr;
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
			vaddr = get_virt_addr(p);
			return( vaddr );
		}
	}
	else {
		for (q= p,p=p->mnext ; p != (struct mblock *) NULL; q=p,p=p->mnext)
			// if block is exactly right size return it and remove it from list
			if ( p->mlen == nbytes) {
				q->mnext = p->mnext;
				vaddr = get_virt_addr(p);
				return( vaddr );
			} else if ( p->mlen > nbytes ) {
				// create new block starting from the memory chosen block leaves off at
				leftover = (struct mblock *)( (unsigned)p + nbytes );
				q->mnext = leftover;
				leftover->mnext = p->mnext;
				leftover->mlen = p->mlen - nbytes;
				vaddr = get_virt_addr(p);
				return( vaddr );
			}
	}
	return( (virt_addr_t)SYSERR );
}

// each page table has 1024 32 bit entries = 4 KB = size of page
// map virtual address to location in backing store
// need PTE for each page
// to map all 4 GB of memory takes 4 MB of page tables - 2^32/2^12(size of page) = 2^20 pages(and PTEs) * 4 (size of PTE) = 2^22 = 4 MB
// page tables are created on demand when a page is first touched (mapped by the process)
struct pt_t *create_page_table(int pt_ix, int bs_id) {
	int i;
	// should call get_frm here to ensure the address is on a 
	struct pt_t *pt =  (struct pt_t *)getmem(sizeof(struct pt_t) * 1024); // this address needs to be divisible by NBPG
	//unsigned long bs_base_addr = BACKING_STORE_BASE + bs_id*BACKING_STORE_UNIT_SIZE + (pt_ix * NBPG * 1024);
	for(i = 0; i < 1024; i++) {
		pt->pt_pres = 1;
		pt->pt_write = 1;
		pt->pt_user = 0;
		pt->pt_pwt = 0;
		pt->pt_pcd = 0;
		pt->pt_acc = 0;
		pt->pt_dirty = 0;
		pt->pt_mbz = 0;
		pt->pt_global = 0;
		pt->pt_avail = 0;
		
		//unsigned int bs_phy_addr = bs_base_addr + i*NBPG;
		//pt->pt_base = bs_phy_addr; // this should be location in backing store or in free frames
								   // top 20 bits, should be in address divisible by page size (all frames are located in such an address) 
		int avail;
		// global page tables cover frames 0 - 4095, get_frm would return frame between 1024 and 2047
		// pt base needs to be address of page in physical memory, offset added to get specific address
		//int ret = get_frm(&avail);
		//pt->pt_base = avail * NBPG; // address of frame (page)
		pt->pt_base = NULL;
		pt++;
	}
	return pt - 1024;
}

// upper 10 bits offset into page directory (0 -1023 indexes into array)
// middle 10 bits offset into page table (0 - 1023 entries)
// last 12 bits offset into page (0 - 4095 bytes in page)

// virtual address needs to reflect this