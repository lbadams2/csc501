/* vgetmem.c - vgetmem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>
#include <paging.h>

extern struct pentry proctab[];
unsigned long getvhp(struct pentry *pptr, unsigned int nbytes);
fr_map_t frm_tab[NFRAMES];
pt_t *create_pt();

/*------------------------------------------------------------------------
 * vgetmem  --  allocate virtual heap storage, returning lowest WORD address
 *------------------------------------------------------------------------
 */
// vcreate maps pid and heap size to backing store, inits a block in vmemlist of size heap size
// sets address of block to vpno and sets proc to vpno, vpno is set to 4th entry of pde and 0th pte
// vgetmem needs to get a free block of nbytes and leave the leftover in the list
// the address of that block will be the virtual address returned, i think don't create page table yet
// if nbytes is greater than original heap size passed to vcreate, need to find another backing store and change vals in proc
WORD	*vgetmem(nbytes)
	unsigned nbytes;
{
	int pid = getpid();
	struct pentry *pptr = &proctab[pid];
	int npages = (nbytes + (NBPG -1)) / NBPG;
	int i;
	unsigned long va = 0;
	for(i = 0; i < 8; i++) {
		if(pptr->vhpnpages[i] >= npages) {
			va = pptr->vhpno[i] << 12;
			break;
		}
	}
	if(i == 8) {
		// page fault
		// need to find another backing store for additional storage, create new mapping
		return NULL;
	}
	//virt_addr_t vaddr = getvhp(pptr, npages);

	// need to create page tables when page first touched, not sure if that's here
	/*
	struct pd_t *pd = (struct pd_t *)pptr->pdbr;
	pd = pd + 4; // skip over global page tables
	int i;	
	int num_page_tables = (npages + (1024 -1)) / 1024;
	for(i = 0; i < num_page_tables; i++) {
		struct pt_t *pt = create_page_table(i, bs_id);
		pd->pd_base = pt;
		pd->pd_pres = 1;
		pd++;
	}
	*/
	return (WORD *)va;
}

virt_addr_t get_virt_addr(struct mblock *p) {
	virt_addr_t vaddr;
	unsigned long baddr = (unsigned long)p;
	int ith_pg_tab = (baddr - 4096) / 1024; // offset into pd
	int ith_page = baddr % 1024; // offset into pt
	int offset = 0; // offset into page
	vaddr.pd_offset = ith_pg_tab;
	vaddr.pt_offset = ith_page;
	vaddr.pg_offset = offset;
	return vaddr;
}

// needs to use virtual address from vpno, not 4096 and beyond
// addresses in vmemlist correspond to vpno (pages) shouldn't need get_virt_addr
// this should probably call get_frm ()
unsigned long getvhp(struct pentry *pptr, unsigned int npages) {
	struct	mblock	*p, *q, *leftover;	
	//virt_addr_t vaddr;
	p = pptr->vmemlist;
	//unsigned int nbytes = hsize * NBPG;
	//nbytes = (unsigned int) roundmb(nbytes);
	if(p->mnext== (struct mblock *) NULL) {
		if(p->mlen < npages)
			return( SYSERR );
		else {
			leftover = (struct mblock *)( (unsigned)p + npages );
			leftover->mnext = NULL;
			leftover->mlen = p->mlen - npages;
			//vaddr = get_virt_addr(p);
			//vaddr = (virt_addr_t)p;
			return( (unsigned long)p );
		}
	}
	else {
		for (q= p,p=p->mnext ; p != (struct mblock *) NULL; q=p,p=p->mnext)
			// if block is exactly right size return it and remove it from list
			if ( p->mlen == npages) {
				q->mnext = p->mnext;
				//vaddr = get_virt_addr(p);
				//vaddr = (virt_addr_t)p;
				return( (unsigned long)p );
			} else if ( p->mlen > npages ) {
				// create new block starting from the memory chosen block leaves off at
				leftover = (struct mblock *)( (unsigned)p + npages );
				q->mnext = leftover;
				leftover->mnext = p->mnext;
				leftover->mlen = p->mlen - npages;
				//vaddr = get_virt_addr(p);
				//vaddr = (virt_addr_t)p;
				return( (unsigned long) );
			}
	}
	return( SYSERR );
}

// each page table has 1024 32 bit entries = 4 KB = size of page
// map virtual address to location in backing store
// need PTE for each page
// to map all 4 GB of memory takes 4 MB of page tables - 2^32/2^12(size of page) = 2^20 pages(and PTEs) * 4 (size of PTE) = 2^22 = 4 MB
// page tables are created on demand when a page is first touched (mapped by the process)
pt_t *create_pt() {
	int i, avail;
	get_frm(&avail);
	fr_map_t *frm = &frm_tab[avail];
    frm->fr_status = FRM_MAPPED;
    frm->fr_pid = getpid();
    frm->fr_refcnt = 1;
    frm->fr_type = FR_TBL;
    frm->fr_dirty = 0;
    frm->fr_vpno = avail + FRAME0;
	unsigned long frm_addr = frm->fr_vpno * NBPG;
  	pt_t *pt = (pt_t *)frm_addr;
	//struct pt_t *pt =  (struct pt_t *)getmem(sizeof(struct pt_t) * 1024); // this address needs to be divisible by NBPG
	//unsigned long bs_base_addr = BACKING_STORE_BASE + bs_id*BACKING_STORE_UNIT_SIZE + (pt_ix * NBPG * 1024);
	for(i = 0; i < 1024; i++) {
		pt->pt_pres = 0;
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